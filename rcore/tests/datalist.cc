// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>

namespace {
using namespace Rapicorn;

/// [DataListContainer-EXAMPLE]
// Declare a global key for custom floating point data members
static DataKey<double> float_data_key;

static void // Access data on any object derived from DataListContainer
data_member_access (DataListContainer &some_object)
{
  // Set the custom float member identified by float_data_key to 21.7
  some_object.set_data (&float_data_key, 21.7);

  // Read out the custom member
  double number = some_object.get_data (&float_data_key);
  assert (number > 21.5 && number < 22);

  // Swap custom member for 1.0
  const double old_float = some_object.swap_data (&float_data_key, 1.0);
  assert (old_float > 21 && old_float < 22);

  // Verify swapped member
  number = some_object.get_data (&float_data_key);
  assert (number == 1.0);

  // Delete custom member
  some_object.delete_data (&float_data_key);
  assert (some_object.get_data (&float_data_key) == 0.0);
}
/// [DataListContainer-EXAMPLE]

static void
data_list_container_example()
{
  DataListContainer some_object;
  data_member_access (some_object);
}
REGISTER_TEST ("Examples/DataListContainer", data_list_container_example);

class MyKey : public DataKey<int> {
  virtual void destroy (int i) override
  {
    Test::tprintout ("%s: delete %d;\n", STRFUNC, i);
  }
  int  fallback()
  {
    return -1;
  }
};

class StringKey : public DataKey<String> {
  virtual void destroy (String s) override
  {
    Test::tprintout ("%s: delete \"%s\";\n", STRFUNC, s.c_str());
  }
};

template<class DataListContainer> static void
data_list_test_strings (DataListContainer &r)
{
  static StringKey strkey;
  r.set_data (&strkey, String ("otto"));
  TASSERT (String ("otto") == r.get_data (&strkey).c_str());
  String dat = r.swap_data (&strkey, String ("RAPICORN"));
  TASSERT (String ("otto") == dat);
  TASSERT (String ("RAPICORN") == r.get_data (&strkey).c_str());
  r.delete_data (&strkey);
  TASSERT (String ("") == r.get_data (&strkey).c_str()); // fallback()
}

template<class DataListContainer> static void
data_list_test_ints (DataListContainer &r)
{
  static MyKey intkey;
  TASSERT (-1 == r.get_data (&intkey)); // fallback() == -1
  int dat = r.swap_data (&intkey, 4);
  TASSERT (dat == -1); // former value
  TASSERT (4 == r.get_data (&intkey));
  r.set_data (&intkey, 5);
  TASSERT (5 == r.get_data (&intkey));
  dat = r.swap_data (&intkey, 6);
  TASSERT (5 == dat);
  TASSERT (6 == r.get_data (&intkey));
  dat = r.swap_data (&intkey, 6);
  TASSERT (6 == dat);
  TASSERT (6 == r.get_data (&intkey));
  dat = r.swap_data (&intkey);
  TASSERT (6 == dat);
  TASSERT (-1 == r.get_data (&intkey)); // fallback()
  r.set_data (&intkey, 8);
  TASSERT (8 == r.get_data (&intkey));
  r.delete_data (&intkey);
  TASSERT (-1 == r.get_data (&intkey)); // fallback()
  r.set_data (&intkey, 9);
  TASSERT (9 == r.get_data (&intkey));
}

static void
data_list_test ()
{
  {
    DataListContainer r;
    data_list_test_strings (r);
  }

  {
    DataListContainer r;
    data_list_test_ints (r);
  }

  {
    DataListContainer r;
    data_list_test_strings (r);
    data_list_test_ints (r);
    data_list_test_strings (r);
  }
}
REGISTER_OUTPUT_TEST ("DataList Tests", data_list_test);

} // anon
