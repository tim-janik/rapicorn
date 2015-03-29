/* This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0 */
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
using namespace Rapicorn;

static void
test_server_remote_handle()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_RemoteHandle once C++ bindings are ready
  ApplicationIface *ab = &app;
  ApplicationImpl *am = dynamic_cast<ApplicationImpl*> (ab);
  assert (am == &app);
  ApplicationIface *ai = am;
  assert (ai == ab);
}
REGISTER_UITHREAD_TEST ("Server/Remote Handle", test_server_remote_handle);

static void
test_stock_resources()
{
  String s;
  s = Stock ("broken-image").label();
  TASSERT (s.empty() == false);
  String icon = Stock ("broken-image").icon();
  TASSERT (icon != "");
  Blob b = Res (icon);
  TASSERT (b && b.size() > 16);
  icon = Stock (" .no.. +such+ -image- ~hCZ75jv27j").icon();
  TASSERT (icon == "");
}
REGISTER_UITHREAD_TEST ("Server/Stock Resources", test_stock_resources);

static void
test_application_list_model_relay()
{
  ApplicationImpl &app = ApplicationImpl::the();
  ListModelRelayIfaceP lmr = app.create_list_model_relay();
  TASSERT (lmr);
  ListModelIfaceP model = lmr->model();
  TASSERT (model);
  TASSERT (model->count() == 0);
  AnySeq aseq;
  TASSERT (aseq.size() == 0);
  aseq.append_back() <<= "cell";
  TASSERT (aseq.size() == 1);
  lmr->update (UpdateRequest (UPDATE_INSERTION, UpdateSpan (0, 1)));
  TASSERT (model->count() == 1);
  Any a = model->row (0);
  TASSERT (a.kind() == Aida::UNTYPED);
  lmr->fill (0, aseq);
  a = model->row (0);
  TASSERT (a.as_string() == "cell");
}
REGISTER_UITHREAD_TEST ("Server/Application ListModelRelay", test_application_list_model_relay);

static void
test_idl_enums()
{
  const Aida::EnumValue *avalues = Aida::enum_value_list<AnchorType>();
  const Aida::EnumValue *ev;
  ev = enum_value_find (avalues, 0); assert (ev && ev->ident == String ("ANCHOR_NONE"));
  ev = enum_value_find (avalues, "ANCHOR_CENTER"); assert (ev && ev->value == 1);
  // assert (avalues.enum_combinable() == false);
  ev = enum_value_find (avalues, "ANCHOR_NORTH");
  assert (ev && ev->ident == String ("ANCHOR_NORTH"));
  // uint64 amask = avalues.enum_parse ("south-west");
  // assert (amask == ANCHOR_SOUTH_WEST);
  const Aida::EnumValue *svalues = Aida::enum_value_list<StateType>();
  ev = enum_value_find (svalues, STATE_INSENSITIVE); assert (ev && ev->ident == String ("STATE_INSENSITIVE"));
  // assert (svalues.enum_combinable() == true);
  //uint64 smask = st.enum_parse ("STATE_INSENSITIVE|STATE_PRELIGHT|STATE_ACTIVE");
  //assert (smask == (STATE_INSENSITIVE | STATE_PRELIGHT | STATE_ACTIVE));
  //assert (st.enum_string (STATE_INSENSITIVE|STATE_ACTIVE) == "STATE_ACTIVE|STATE_INSENSITIVE");
}
REGISTER_UITHREAD_TEST ("Server/IDL Enums", test_idl_enums);

#if 0 // unused
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
#endif
