// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rcore/testutils.hh>
#include <ui/uithread.hh>

namespace {
using namespace Rapicorn;

static void
test_any_seq ()
{
  AnySeq aseq;                          assert (aseq.size() == 0);
  aseq.append_back() <<= "first";       assert (aseq.size() == 1);
  aseq.append_back() <<= "second";      assert (aseq.size() == 2);
  aseq.resize (5);                      assert (aseq.size() == 5);
  aseq[2] <<= 7;                        assert (aseq[2].as_int() == 7);  assert (aseq[2].as_string() == "7");
  aseq[3] <<= 17.5;                     assert (aseq[3].as_int() == 17); assert (aseq[3].as_string() == "17.5");
  aseq[4] <<= "fifth";                  assert (aseq[4].as_string() == "fifth");
  aseq.append_back() <<= Any();         assert (aseq.size() == 6);
  assert (aseq[1].as_string() == "second");
  int i; aseq[2] >>= i;                 assert (i == 7);
  double d; aseq[3] >>= d;              assert (d > 17.4 && d < 17.6);
  String s; aseq[4] >>= s;              assert (s == "fifth");
  const Any *a; aseq[5] >>= a;          assert (*a == Any());
  aseq.clear();                         assert (aseq.size() == 0);
#if 0 // FIXME: unimplemented
  struct TestObject : public virtual BaseObject {};
  TestObject *to = new TestObject();
  ref_sink (to);
  a1.push_head (*to);
  unref (to);
#endif
}
REGISTER_UITHREAD_TEST ("Stores/AnySeq basics", test_any_seq);

static String
any_seq_seq_to_string (const AnySeqSeq &ss, const String &joiner = "")
{
  String s;
  for (size_t i = 0; i < ss.size(); i++)
    {
      if (!s.empty())
        s += joiner;
      for (size_t j = 0; j < ss[i].size(); j++)
        s += ss[i][j].as_string();
    }
  return s;
}

static void
test_any_seq_seq ()
{
  AnySeqSeq ss;
  const char *board[][8] = {
    { "R","N","B","Q","K","B","N","R" },
    { "P","P","P","P","P","P","P","P" },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { "p","p","p","p","p","p","p","p" },
    { "r","n","b","q","k","b","n","r" }
  };
  ss.resize (RAPICORN_ARRAY_SIZE (board));
  for (size_t i = 0; i < RAPICORN_ARRAY_SIZE (board); i++)
    for (size_t j = 0; j < RAPICORN_ARRAY_SIZE (board[i]); j++)
      ss[i].append_back() <<= board[i][j];
  String sss = any_seq_seq_to_string (ss, "\n");
  if (Test::verbose())
    printout ("chess0:\n%s\n\"%s\"\n", sss.c_str(), string_to_cescape (sss).c_str());
  assert (sss == "RNBQKBNR\nPPPPPPPP\n        \n        \n        \n        \npppppppp\nrnbqkbnr");
  ss[3][4] <<= ss[1][4].as_string();
  ss[1][4] <<= " ";
  sss = any_seq_seq_to_string (ss, "\n");
  if (Test::verbose())
    printout ("chess1:\n%s\n\"%s\"\n", sss.c_str(), string_to_cescape (sss).c_str());
  assert (sss == "RNBQKBNR\nPPPP PPP\n        \n    P   \n        \n        \npppppppp\nrnbqkbnr");
}
REGISTER_UITHREAD_TEST ("Stores/AnySeqSeq Chess", test_any_seq_seq);

static uint store_inserted_counter = 0;
static void
store_inserted (int first, int last)
{
  store_inserted_counter++;
}
static uint store_changed_counter = 0;
static void
store_changed (int first, int last)
{
  store_changed_counter++;
}

static void
test_basic_memory_store ()
{
  MemoryListStore *store = new MemoryListStore (1);
  ref_sink (store);
  // check model/store identity (for memory stores)
  ListModelIface &m1 = *store;
  assert (&m1 == store);
  store->sig_inserted += slot (store_inserted);
  store->sig_changed += slot (store_changed);
  // basic store assertions
  assert (store->size() == 0);
  // insert first row
  AnySeq row;
  row.append_back() <<= "first";
  uint last_counter = store_inserted_counter;
  store->insert (0, row);
  assert (store_inserted_counter > last_counter);
  assert (store->size() == 1);
  assert (store->row (0)[0].as_string() == "first");
  last_counter = store_changed_counter;
  row[0] <<= "foohoo";
  store->update (0, row);
  assert (store_changed_counter > last_counter);
  assert (store->row (0)[0].as_string() == "foohoo");
  last_counter = store_inserted_counter;
  row[0] <<= 2;
  store->insert (1, row);
  assert (store_inserted_counter > last_counter);
  assert (store->row (1)[0].as_int() == 2);
  unref (store);
}
REGISTER_UITHREAD_TEST ("Stores/Basic memory store", test_basic_memory_store);

static String
stringify_model (ListModelIface &model)
{
  String s = "[\n";
  for (int i = 0; i < model.size(); i++)
    {
      AnySeq row = model.row (i);
      s += "  (";
      for (uint j = 0; j < row.size(); j++)
        s += string_printf ("%s%s", j ? "," : "", row[j].as_string().c_str());
      s += "),\n";
    }
  s += "]";
  return s;
}

static void
test_store_modifications ()
{
  MemoryListStore &store = *new MemoryListStore (4);
  ref_sink (store);
  for (uint i = 0; i < 4; i++)
    {
      AnySeq row;
      for (uint j = 0; j < 4; j++)
        row.append_back() <<= string_printf ("%02x", 16 * (i + 1) + j + 1);
      store.insert (-1, row);
    }
  AnySeq row;
  row.resize (4);
  // store modifications
  row[0] <<= "Newly_appended_last_row";
  store.insert (-1, row);
  row[0] <<= "Newly_prepended_first_row";
  store.insert (0, row);

  row = store.row (1);
  row[3] <<= "Extra_text_added_to_row_1";
  store.update (1, row);

  store.remove (2);
  row.clear();
  row.resize (4);
  row[0] <<= "Replacement_for_removed_row_2";
  store.insert (2, row);

  // test verification
  String e = "[\n"
             "  (Newly_prepended_first_row,,,),\n"
             "  (11,12,13,Extra_text_added_to_row_1),\n"
             "  (Replacement_for_removed_row_2,,,),\n"
             "  (31,32,33,34),\n"
             "  (41,42,43,44),\n"
             "  (Newly_appended_last_row,,,),\n"
             "]";
  String s = stringify_model (store);
  if (Test::verbose())
    printout ("%s: model:\n%s\n", __func__, s.c_str());
  assert (e == s);
  // cleanup
  unref (store);
}
REGISTER_UITHREAD_TEST ("Stores/Memory Store Modifications", test_store_modifications);

} // Anon
