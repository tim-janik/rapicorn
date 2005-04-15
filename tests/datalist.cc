/* Tests
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;
using Rapicorn::uint;

class MyKey : public DataKey<int> {
  void destroy (int i)
  {
    printf ("%s: delete %d;\n", STRFUNC, i);
  }
  int  fallback()
  {
    return -1;
  }
};

class StringKey : public DataKey<String> {
  void destroy (String s)
  {
    printf ("%s: delete \"%s\";\n", STRFUNC, s.c_str());
  }
};

static void
data_list_test ()
{
  MyKey intkey;
  StringKey strkey;

  DataListContainer r;
  printf ("Data List Test:\n");
  printf ("set_data(\"%s\")\n", "otto");        r.set_data (&strkey, String ("otto"));  printf ("data=\"%s\"\n", r.get_data (&strkey).c_str());
  printf ("(fallback=-1)\n");                                                           printf ("data=%d\n", r.get_data (&intkey));
  printf ("swap_data(%d)=%d\n", 4, r.swap_data (&intkey, 4));                           printf ("data=%d\n", r.get_data (&intkey));
  printf ("set_data(%d)\n", 5);                 r.set_data (&intkey, 5);                printf ("data=%d\n", r.get_data (&intkey));
  printf ("swap_data(%d)=%d\n", 6, r.swap_data (&intkey, 6));                           printf ("data=%d\n", r.get_data (&intkey));
  printf ("swap_data(%d)=%d\n", 6, r.swap_data (&intkey, 6));                           printf ("data=%d\n", r.get_data (&intkey));
  printf ("swap_data()=%d\n", r.swap_data (&intkey));                                   printf ("data=%d\n", r.get_data (&intkey));
  printf ("set_data(%d)\n", 8);                 r.set_data (&intkey, 8);                printf ("data=%d\n", r.get_data (&intkey));
  printf ("delete_data()\n");                   r.delete_data (&intkey);                printf ("data=%d\n", r.get_data (&intkey));
  printf ("set_data(%d)\n", 9);                 r.set_data (&intkey, 9);                printf ("data=%d\n", r.get_data (&intkey));
}

extern "C" int
main (int   argc,
      char *argv[])
{
  printf ("TEST: %s:\n", basename (argv[0]));
  data_list_test();
  return 0;
}

} // anon
