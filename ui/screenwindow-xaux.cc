// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <ui/utilities.hh>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
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
