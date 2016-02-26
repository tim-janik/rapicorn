// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
      row.set (string_format ("%u) %s %s %d %u", i, e->d_name,
                              i % 10 == 0 ? string_multiply ("<br/>", 1 + i / 10).c_str() : "",
                              e->d_type, e->d_ino));
      lstore.insert (lstore.count(), row);
      e = readdir (d);
      i++;
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
  lrelay_ = lrelay_.down_cast (lrelay_.__aida_null_handle__());
}

void
ListModelBinding::refill (const UpdateRequest &urequest)
{
  if (urequest.kind == UpdateKind::READ)
    {
      AnySeq aseq;
      for (ssize_t i = urequest.rowspan.start; i < urequest.rowspan.start + urequest.rowspan.length; i++)
        aseq.append_back() = store.row (i);
      lrelay_.fill (urequest.rowspan.start, aseq);
    }
}

static bool
app_bind_list_store (ListStore &store, ListModelH *lm = NULL)
{
  if (lm)
    *lm = ListModelH(); // NULL/empty
  ListModelRelayH lrelay_ = ApplicationH::the().create_list_model_relay();
  if (lrelay_ == NULL)
    return false;
  if (lm)
    *lm = lrelay_.model();
  ListModelBinding *lbinding = new ListModelBinding (store, lrelay_);
  (void) lbinding; // ownership is kept via DataKey on ListStore
  return true;
}

static void
fill_test_store (ListStore &lstore)
{
  lstore.clear();
  for (uint i = 0; i < 20; i++)
    {
      String s;
      if (i && (i % 10) == 0)
        s = string_format ("* %u SMALL ROW (watch scroll direction)", i);
      else
        s = string_format ("|<br/>| <br/>| %u<br/>|<br/>|", i);
      Any row;
      row.set (s);
      lstore.insert (lstore.count(), row);
    }
}

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  ApplicationH app = init_app ("RapicornFileView", &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("fileview.xml", argv[0]);

  // create and bind list store
  ListStore &store = *new ListStore();
  ListModelH lmh;
  bool bsuccess = app_bind_list_store (store, &lmh);
  assert (bsuccess);

  // create, bind and fill testing list store
  ListStore &test_store = *new ListStore();
  bsuccess = app_bind_list_store (test_store);
  assert (bsuccess);
  fill_test_store (test_store);

  // create main window
  WindowH window = app.create_window ("main-dialog");
  WidgetListH wl = WidgetListH::down_cast (window.query_selector (".WidgetList"));
  assert (wl != NULL);
  wl.bind_model (lmh, "WidgetListRow");

  window.show();

  // load directory data
  fill_store (store, ".");

  // run event loops while windows are on screen
  return app.run_and_exit();
}

} // anon
