// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rapicorn.hh>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include "liststore.cc"

namespace {
using namespace Rapicorn;

static void
fill_store (ListStore &lstore, const String &dirname)
{
  DIR *d = opendir (dirname.c_str());
  lstore.clear();
  if (!d)
    {
      critical ("failed to access directory: %s: %s", dirname.c_str(), ::strerror (errno));
      return;
    }
  struct dirent *e = readdir (d);
  size_t i = 0;
  while (e)
    {
#if 0 // FIXME: need proper record support in Aida::Any
      AnySeq row;
      row.append_back() <<= ++i;
      row.append_back() <<= e->d_ino;
      row.append_back() <<= e->d_type;
      row.append_back() <<= e->d_name;
#endif
      Any row;
      row <<= string_printf ("%zu) %s\n      %d %lu", i++, e->d_name, e->d_type, e->d_ino);
      lstore.insert (lstore.count(), row);
      e = readdir (d);
    }
  closedir (d);
}

class ListModelBinding {
  class BindingKey : public DataKey<ListModelBinding*> {
    virtual void destroy (ListModelBinding *lbinding)
    { delete (lbinding); }
  };
  static BindingKey lm_binding_key;
  void disconnect ();
  ListStore &store;
  size_t conid_updates_;
  ListModelRelayH lrelay_;
  size_t conid_refill_;
  void refill (const UpdateRequest &urequest);
public:
  ListModelBinding (ListStore &init_store, ListModelRelayH init_lrelay);
  ~ListModelBinding();
};
ListModelBinding::BindingKey ListModelBinding::lm_binding_key;

ListModelBinding::ListModelBinding (ListStore &init_store, ListModelRelayH init_lrelay) :
  store (init_store), lrelay_ (init_lrelay)
{
  assert_return (lrelay_ != NULL);
  store.set_data (&lm_binding_key, this);
  conid_updates_ = store.sig_updates() += slot (lrelay_, &ListModelRelayH::update);
  conid_refill_  = lrelay_.sig_refill() += Aida::slot (*this, &ListModelBinding::refill);
}

ListModelBinding::~ListModelBinding()
{
  if (lrelay_ != NULL)
    disconnect();
}

void
ListModelBinding::disconnect()
{
  assert_return (lrelay_ != NULL);
  store.sig_updates() -= conid_updates_;
  lrelay_.sig_refill() -= conid_refill_;
  lrelay_ = lrelay_.down_cast (lrelay_._null_handle());
}

void
ListModelBinding::refill (const UpdateRequest &urequest)
{
  if (urequest.kind == UPDATE_READ)
    {
      AnySeq aseq;
      for (ssize_t i = urequest.rowspan.start; i < urequest.rowspan.start + urequest.rowspan.length; i++)
        aseq.append_back() = store.row (i);
      lrelay_.fill (urequest.rowspan.start, aseq);
    }
}

static bool
app_bind_list_store (ListStore &store, const String &path)
{
  ListModelRelayH lrelay_ = ApplicationH::the().create_list_model_relay (path);
  if (lrelay_ == NULL)
    return false;
  ListModelBinding *lbinding = new ListModelBinding (store, lrelay_);
  (void) lbinding; // ownership is kept via DataKey on ListStore
  return true;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  ApplicationH app = init_app ("RapicornFileView", &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("RapicornFileView", "fileview.xml", argv[0]);

  // create and bind list store
  ListStore &store = *new ListStore();
  bool bsuccess = app_bind_list_store (store, "//local/data/fileview/main");
  assert (bsuccess);

  // create main window
  WindowH window = app.create_window ("RapicornFileView:main-dialog", Strings ("list-model=//local/data/fileview/main"));
  window.show();

  // load directory data
  fill_store (store, ".");

  // run event loops while windows are on screen
  return app.run_and_exit();
}

} // anon
