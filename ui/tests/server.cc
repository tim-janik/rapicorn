/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
using namespace Rapicorn;

static void
test_server_smart_handle()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  ApplicationIface *ab = &app;
  Aida::FieldBuffer8 fb (4);
  fb.add_object (uint64 ((BaseObject*) ab));
  // FIXME: Aida::Coupler &c = *rope_thread_coupler();
  // c.reader.reset (fb);
  ApplicationImpl *am = dynamic_cast<ApplicationImpl*> (ab);
  assert (am == &app);
  ApplicationIface *ai = am;
  assert (ai == ab);
}
REGISTER_UITHREAD_TEST ("Server/Smart Handle", test_server_smart_handle);

static void
test_stock_resources()
{
  String s;
  s = Stock::stock_label ("broken-image");
  TASSERT (s.empty() == false);
  s = Stock::stock_string ("broken-image", "image");
  TASSERT (s.empty() == false);
  Blob b = Stock::stock_image ("broken-image");
  TASSERT (b && b.size() > 16);
  b = Stock::stock_image (" .no.. +such+ -image- ~hCZ75jv27j");
  TASSERT (!b && errno != 0);
}
REGISTER_UITHREAD_TEST ("Server/Stock Resources", test_stock_resources);

static void
test_application_xurl()
{
  ApplicationImpl &app = ApplicationImpl::the();
  bool success;

  struct Dummy : public ListModelRelayImpl { using ListModelRelayImpl::create_list_model_relay; };
  ListModelRelayIface &lmr1 = Dummy::create_list_model_relay ();
  ListModelIface      &lm1  = *lmr1.model();
  ListModelRelayIface &lmr2 = Dummy::create_list_model_relay ();
  ListModelIface      &lm2  = *lmr2.model();
  ListModelRelayIface &lmr3 = Dummy::create_list_model_relay ();
  ListModelIface      &lm3  = *lmr3.model();
  ListModelIface *lmi;
  String path;

  // model1 + lmr1 tests
  lmi = app.xurl_find ("//local/data/unknown");                 TASSERT (lmi == NULL);
  lmi = app.xurl_find ("//local/data/model1");                  TASSERT (lmi == NULL);
  path = app.xurl_path (lm1);                                   TASSERT (path == "");
  success = app.xurl_add ("//local/data/model1", lm1);          TASSERT (success == true);
  path = app.xurl_path (lm1);                                   TASSERT (path == "//local/data/model1");
  success = app.xurl_add ("//local/data/model1", lm1);          TASSERT (success == false);
  success = app.xurl_add ("//local/data/model1", lm2);          TASSERT (success == false);
  success = app.xurl_add ("//local/data/unknown", lm1);         TASSERT (success == false);
  lmi = app.xurl_find ("//local/data/model1");                  TASSERT (lmi == &lm1);
  success = app.xurl_sub (lm1);                                 TASSERT (success == true);
  success = app.xurl_sub (lm1);                                 TASSERT (success == false);
  path = app.xurl_path (lm1);                                   TASSERT (path == "");
  lmi = app.xurl_find ("//local/data/model1");                  TASSERT (lmi == NULL);
  success = app.xurl_add ("//local/data/model1", lm1);          TASSERT (success == true);
  path = app.xurl_path (lm1);                                   TASSERT (path == "//local/data/model1");

  // model2 + lmr2 tests
  lmi = app.xurl_find ("//local/data/model2");                  TASSERT (lmi == NULL);
  path = app.xurl_path (lm2);                                   TASSERT (path == "");
  success = app.xurl_sub (lm2);                                 TASSERT (success == false);
  success = app.xurl_add ("//local/data/model2", lm2);          TASSERT (success == true);
  path = app.xurl_path (lm2);                                   TASSERT (path == "//local/data/model2");
  success = app.xurl_add ("//local/data/model2", lm1);          TASSERT (success == false);
  success = app.xurl_add ("//local/data/model2", lm3);          TASSERT (success == false);
  success = app.xurl_add ("//local/data/unknown", lm2);         TASSERT (success == false);
  lmi = app.xurl_find ("//local/data/model2");                  TASSERT (lmi == &lm2);
  success = app.xurl_sub (lm2);                                 TASSERT (success == true);
  success = app.xurl_sub (lm2);                                 TASSERT (success == false);
  lmi = app.xurl_find ("//local/data/model2");                  TASSERT (lmi == NULL);
  success = app.xurl_add ("//local/data/model2", lm2);          TASSERT (success == true);
  lmi = app.xurl_find ("//local/data/model2");                  TASSERT (lmi == &lm2);

  // model3 + lmr3 tests
  lmi = app.xurl_find ("//local/data/model3");                  TASSERT (lmi == NULL);
  path = app.xurl_path (lm3);                                   TASSERT (path == "");
  success = app.xurl_add ("//local/data/model3", lm3);          TASSERT (success == true);
  path = app.xurl_path (lm3);                                   TASSERT (path == "//local/data/model3");
  success = app.xurl_add ("//local/data/unknown", lm3);         TASSERT (success == false);
  lmi = app.xurl_find ("//local/data/model3");                  TASSERT (lmi == &lm3);

  // removal checks
  lmi = app.xurl_find ("//local/data/model1");                  TASSERT (lmi == &lm1);
  unref (ref_sink (lmr1));
  lmi = app.xurl_find ("//local/data/model1");                  TASSERT (lmi == NULL);

  lmi = app.xurl_find ("//local/data/model2");                  TASSERT (lmi == &lm2);
  success = app.xurl_sub (lm2);                                 TASSERT (success == true);
  success = app.xurl_sub (lm2);                                 TASSERT (success == false);
  unref (ref_sink (lmr2));
  lmi = app.xurl_find ("//local/data/model2");                  TASSERT (lmi == NULL);

  lmi = app.xurl_find ("//local/data/model3");                  TASSERT (lmi == &lm3);
  unref (ref_sink (lmr3));
  lmi = app.xurl_find ("//local/data/model3");                  TASSERT (lmi == NULL);
}
REGISTER_UITHREAD_TEST ("Server/Application XUrl Map", test_application_xurl);

static void
test_idl_enums()
{
  Aida::TypeCode at = Aida::TypeMap::lookup ("Rapicorn::AnchorType");
  assert (at.kind() == Aida::ENUM);
  assert (at.name() == "Rapicorn::AnchorType");
  assert (at == Aida::TypeCode::from_enum<Rapicorn::AnchorType>());
  Aida::TypeCode::EnumValue ev;
  ev = at.enum_find (0); assert (ev.ident == String ("ANCHOR_NONE"));
  ev = at.enum_find ("ANCHOR_CENTER"); assert (ev.value == 1);
  assert (at.enum_combinable() == false);
  assert (at.enum_string (ANCHOR_NORTH) == "ANCHOR_NORTH");
  uint64 amask = at.enum_parse ("south-west");
  assert (amask == ANCHOR_SOUTH_WEST);
  Aida::TypeCode st = Aida::TypeCode::from_enum<StateType>();
  assert (st.kind() == Aida::ENUM);
  assert (st.name() == "Rapicorn::StateType");
  assert (st.enum_combinable() == true);
  uint64 smask = st.enum_parse ("STATE_INSENSITIVE|STATE_PRELIGHT|STATE_IMPRESSED");
  assert (smask == (STATE_INSENSITIVE | STATE_PRELIGHT | STATE_IMPRESSED));
  assert (st.enum_string (STATE_INSENSITIVE) == "STATE_INSENSITIVE");
  assert (st.enum_string (STATE_INSENSITIVE|STATE_IMPRESSED) == "STATE_IMPRESSED|STATE_INSENSITIVE");
}
REGISTER_UITHREAD_TEST ("Server/IDL Enums", test_idl_enums);

static void
test_type_codes()
{
  ApplicationImpl &app = ApplicationImpl::the();
  Aida::TypeCode tc = app.__aida_type_code__();
  assert (tc.kind() == Aida::INSTANCE);
  assert (tc.name() == "Rapicorn::Application");
  assert (tc.untyped() == false);
  Pixbuf pixbuf;
  tc = pixbuf.__aida_type_code__();
  assert (tc.kind() == Aida::RECORD);
  assert (tc.name() == "Rapicorn::Pixbuf");
  assert (tc.untyped() == false);
  assert (tc.field_count() == 3);
  assert (tc.field (0).name() == "row_length"); (void) pixbuf.row_length;
  assert (tc.field (1).name() == "pixels");     (void) pixbuf.pixels;
  assert (tc.field (2).name() == "variables");  (void) pixbuf.variables;
}
REGISTER_UITHREAD_TEST ("Server/IDL Type Codes", test_type_codes);
