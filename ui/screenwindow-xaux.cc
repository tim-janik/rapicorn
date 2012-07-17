// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <ui/utilities.hh>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>

#define XDEBUG(...)     RAPICORN_KEY_DEBUG ("X11", __VA_ARGS__)

namespace { // Anon
using namespace Rapicorn;

// == X11 Error Handling ==
static XErrorHandler xlib_error_handler = NULL;
static XErrorEvent *xlib_error_trap = NULL;
static int
x11_error (Display *error_display, XErrorEvent *error_event)
{
  if (xlib_error_trap)
    {
      *xlib_error_trap = *error_event;
      return 0;
    }
  int result = -1;
  try
    {
      atexit (abort);
      result = xlib_error_handler (error_display, error_event);
    }
  catch (...)
    {
      // ignore C++ exceptions from atexit handlers
    }
  abort();
  return result;
}

static void
x11_trap_errors (XErrorEvent *trapped_event)
{
  assert_return (xlib_error_trap == NULL);
  xlib_error_trap = trapped_event;
  trapped_event->error_code = 0;
}

static int
x11_untrap_errors()
{
  assert_return (xlib_error_trap != NULL, -1);
  const int error_code = xlib_error_trap->error_code;
  xlib_error_trap = NULL;
  return error_code;
}

static bool
x11_check_shared_image (Display *display, Visual *visual, int depth)
{
  bool has_shared_mem = 0;
  XShmSegmentInfo shminfo = { 0 /*shmseg*/, -1 /*shmid*/, (char*) -1 /*shmaddr*/, True /*readOnly*/ };
  XImage *ximage = XShmCreateImage (display, visual, depth, ZPixmap, NULL, &shminfo, 1, 1);
  if (ximage)
    {
      shminfo.shmid = shmget (IPC_PRIVATE, ximage->bytes_per_line * ximage->height, IPC_CREAT | 0600);
      if (shminfo.shmid != -1)
        {
          shminfo.shmaddr = (char*) shmat (shminfo.shmid, NULL, SHM_RDONLY);
          if (ptrdiff_t (shminfo.shmaddr) != -1)
            {
              XErrorEvent dummy = { 0, };
              x11_trap_errors (&dummy);
              Bool result = XShmAttach (display, &shminfo);
              XSync (display, False); // forces error delivery
              if (!x11_untrap_errors())
                {
                  // if we got here uccessfully, shared memory works
                  has_shared_mem = result != 0;
                }
            }
          shmctl (shminfo.shmid, IPC_RMID, NULL); // delete the shm segment upon last detaching process
          // cleanup
          if (has_shared_mem) // FIXME: test
            XShmDetach (display, &shminfo);
          if (ptrdiff_t (shminfo.shmaddr) != -1)
            shmdt (shminfo.shmaddr);
        }
      XDestroyImage (ximage);
    }
  return has_shared_mem;
}

static __attribute__ ((unused)) const char*
window_state (int wm_state)
{
  switch (wm_state)
    {
    case WithdrawnState:        return "WithdrawnState";
    case NormalState:           return "NormalState";
    case IconicState:           return "IconicState";
    default:                    return "Unknown";
    }
}

static const char*
notify_mode (int notify_type)
{
  switch (notify_type)
    {
    case NotifyNormal:          return "Normal";
    case NotifyGrab:            return "Grab";
    case NotifyUngrab:          return "Ungrab";
    case NotifyWhileGrabbed:    return "WhileGrabbed";
    default:                    return "Unknown";
    }
}

static const char*
notify_detail (int notify_type)
{
  switch (notify_type)
    {
    case NotifyAncestor:         return "Ancestor";
    case NotifyVirtual:          return "Virtual";
    case NotifyInferior:         return "Inferior";
    case NotifyNonlinear:        return "Nonlinear";
    case NotifyNonlinearVirtual: return "NonlinearVirtual";
    case NotifyPointer:          return "NotifyPointer";
    case NotifyPointerRoot:      return "NotifyPointerRoot";
    case NotifyDetailNone:       return "NotifyDetailNone";
    default:                     return "Unknown";
    }
}

static const char*
visibility_state (int visibility_type)
{
  switch (visibility_type)
    {
    case VisibilityUnobscured:          return "VisibilityUnobscured";
    case VisibilityPartiallyObscured:   return "VisibilityPartiallyObscured";
    case VisibilityFullyObscured:       return "VisibilityFullyObscured";
    default:                            return "Unknown";
    }
}

template<class Data, class X11Context> static vector<Data>
x11_get_property_data (X11Context &x11context, Window window, const String &property_name, Atom *property_type = NULL)
{
  XErrorEvent dummy = { 0, };
  x11_trap_errors (&dummy);
  int format_returned = 0;
  Atom type_returned = 0;
  unsigned long nitems_return = 0, bytes_after_return = 0;
  uint8 *prop_data = NULL;
  int abort = XGetWindowProperty (x11context.display, window, x11context.atom (property_name), 0, 8 * 1024, False,
                                  AnyPropertyType, &type_returned, &format_returned, &nitems_return,
                                  &bytes_after_return, &prop_data) != Success;
  if (x11_untrap_errors() || type_returned == None)
    abort++;
  if (!abort && bytes_after_return)
    {
      XDEBUG ("XGetWindowProperty(%s): property size exceeds buffer by %lu bytes", CQUOTE (property_name), bytes_after_return);
      abort++;
    }
  if (!abort && sizeof (Data) * 8 != format_returned)
    {
      XDEBUG ("XGetWindowProperty(%s): property format mismatch: expected=%zu returned=%u", CQUOTE (property_name), sizeof (Data) * 8, format_returned);
      abort++;
    }
  vector<Data> datav;
  if (!abort && prop_data && nitems_return && format_returned)
    {
      if (format_returned == 32)
        {
          const unsigned long *pulong = (const unsigned long*) prop_data;
          for (uint i = 0; i < nitems_return; i++)
            datav.push_back (pulong[i]);
        }
      else if (format_returned == 16)
        {
          const uint16 *puint16 = (const uint16*) prop_data;
          for (uint i = 0; i < nitems_return; i++)
            datav.push_back (puint16[i]);
        }
      else if (format_returned == 8)
        {
          const uint8 *puint8 = (const uint8*) prop_data;
          for (uint i = 0; i < nitems_return; i++)
            datav.push_back (puint8[i]);
        }
      else
        XDEBUG ("XGetWindowProperty(%s): unknown property data format with %d bits", CQUOTE (property_name), format_returned);
    }
  if (prop_data)
    XFree (prop_data);
  if (property_type)
    *property_type = type_returned;
  return datav;
}

template<class X11Context> static String
x11_get_string_property (X11Context &x11context, Window window, const String &property_name, Atom *property_type = NULL)
{
  Atom ptype = 0;
  vector<char> datav = x11_get_property_data<char> (x11context, window, property_name, &ptype);
  String rstring;
  if (datav.size() && (ptype == x11context.atom ("STRING") || ptype == x11context.atom ("COMPOUND_TEXT")))
    {
      XTextProperty xtp;
      xtp.format = 8;
      xtp.nitems = datav.size();
      xtp.value = (uint8*) datav.data();
      xtp.encoding = ptype;
      char **tlist = NULL;
      int count = 0, res = Xutf8TextPropertyToTextList (x11context.display, &xtp, &tlist, &count);
      if (res != XNoMemory && res != XLocaleNotSupported && res != XConverterNotFound && count && tlist && tlist[0])
        rstring = String (tlist[0]);
      if (tlist)
        XFreeStringList (tlist);
    }
  else if (datav.size() && ptype == x11context.atom ("UTF8_STRING"))
    rstring = String (datav.data(), datav.size());
  else
    XDEBUG ("XGetWindowProperty(%s): unknown string property format: %s", CQUOTE (property_name), x11context.atom (ptype).c_str());
  if (property_type)
    *property_type = ptype;
  return rstring;
}

enum XPEmpty { KEEP_EMPTY, DELETE_EMPTY };

template<class X11Context> static bool
set_text_property (X11Context &x11context, Window window, const String &property, XICCEncodingStyle ecstyle,
                   const String &value, XPEmpty when_empty = KEEP_EMPTY)
{
  bool success = true;
  if (when_empty == DELETE_EMPTY && value.empty())
    XDeleteProperty (x11context.display, window, x11context.atom (property));
  else if (ecstyle == XUTF8StringStyle)
    XChangeProperty (x11context.display, window, x11context.atom (property), x11context.atom ("UTF8_STRING"), 8,
                     PropModeReplace, (uint8*) value.c_str(), value.size());
  else
    {
      char *text = const_cast<char*> (value.c_str());
      XTextProperty xtp = { 0, };
      const int result = Xutf8TextListToTextProperty (x11context.display, &text, 1, ecstyle, &xtp);
      if (0)
        printerr ("XUTF8CONVERT: target=%s len=%zd result=%d: %s -> %s\n", x11context.atom(xtp.encoding).c_str(), value.size(), result, text, xtp.value);
      if (result >= 0 && xtp.nitems && xtp.value)
        XChangeProperty (x11context.display, window, x11context.atom (property), xtp.encoding, xtp.format,
                         PropModeReplace, xtp.value, xtp.nitems);
      else
        success = false;
      if (xtp.value)
        XFree (xtp.value);
    }
  return success;
}

} // Anon
