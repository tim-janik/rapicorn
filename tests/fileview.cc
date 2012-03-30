// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rapicorn.hh>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

namespace {
using namespace Rapicorn;

#if 0 // FIXME: need remote model support
static void
fill_store (Store1       &s1,
            const String &dirname)
{
  DIR *d = opendir (dirname.c_str());
  s1.clear();
  if (!d)
    {
      critical ("failed to access directory: %s: %s", dirname.c_str(), strerror (errno));
      return;
    }
  struct dirent *e = readdir (d);
  while (e)
    {
      AnySeq row;
      row.append_back() <<= e->d_ino;
      row.append_back() <<= " | ";
      row.append_back() <<= e->d_type;
      row.append_back() <<= " | ";
      row.append_back() <<= e->d_name;
      s1.insert (-1, row);
      e = readdir (d);
    }
  closedir (d);
}

static Store1*
create_store ()
{
  Store1 *s1 = Store1::create_memory_store ("models/files",
                                            Plic::TypeMap::lookup ("string"), SELECTION_BROWSE);
  fill_store (*s1, ".");
  return s1;
}
#endif

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  Application app = init_app ("RapicornFileView", &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("RapicornFileView", "fileview.xml", argv[0]);

  // create main window
  // FIXME: Store1 *s1 = create_store();
  Window window = app.create_window ("RapicornFileView:main-dialog"); // FIXME: Args (""), Args ("ListModel=" + s1->model().plor_name()));
  window.show();

  // run event loops while windows are on screen
  return app.run_and_exit();
}

} // anon
