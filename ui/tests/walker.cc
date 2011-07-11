/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include <rcore/testutils.hh>
#include <rapicorn.hh>
#include <stdio.h>

namespace {
using namespace Rapicorn;

static void
test_walker()
{
  printf ("%s: ", __func__);
  static int values[] = { 3, 2, 1, };
  std::vector<int*> cells;
  for (uint i = 0; i < sizeof (values) / sizeof (values[0]); i++)
    cells.push_back (values + i);
  /* we need to test the various types of const combinations here */
  /* int* */
  for (Walker<int*> wp = walker (cells); wp.has_next(); wp++) printf ("%d", **wp);
  // for (Walker<int*> wp (cells); wp.has_next(); wp++) printf ("%d", **wp);
  printf (" ");
  /* *(int*) */
  for (Walker<int> wp = value_walker (cells); wp.has_next(); wp++) printf ("%d", *wp);
  // for (ValueWalker<int> wp (cells); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* int*const */
  const std::vector<int*> &cells_const = *(std::vector<int*>*) &cells;
  for (Walker<int*const> wp = walker (cells_const); wp.has_next(); wp++) printf ("%d", **wp);
  // for (Walker<int*const> wp (cells_const); wp.has_next(); wp++) printf ("%d", **wp);
  printf (" ");
  /* *(int*const) */
  for (Walker<int> wp = value_walker (cells_const); wp.has_next(); wp++) printf ("%d", *wp);
  // for (ValueWalker<int> wp (cells_const); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* const int* */
  std::vector<const int*> &const_cells = *reinterpret_cast<std::vector<const int*>*> (&cells);
  for (Walker<const int*> wp = walker (const_cells); wp.has_next(); wp++) printf ("%d", **wp);
  // for (Walker<const int*> wp (const_cells); wp.has_next(); wp++) printf ("%d", **wp);
  printf (" ");
  /* *(const int*) */
  for (Walker<const int> wp = value_walker (const_cells); wp.has_next(); wp++) printf ("%d", *wp);
  // for (ValueWalker<const int> wp (const_cells); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* const int*const */
  const std::vector<const int*> &const_cells_const = *const_cast<const std::vector<const int*>*>
                                                     (reinterpret_cast<std::vector<const int*>*> (&cells));
  for (Walker<const int*const> wp = walker (const_cells_const); wp.has_next(); wp++) printf ("%d", **wp);
  // for (Walker<const int*const> wp (const_cells_const); wp.has_next(); wp++) printf ("%d", **wp);
  printf (" ");
  /* *(const int*const) */
  for (Walker<const int> wp = value_walker (const_cells_const); wp.has_next(); wp++) printf ("%d", *wp);
  // for (ValueWalker<const int> wp (const_cells_const); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* iter (int) */
  for (Walker<int> wp = walker (pointer_iterator (&values[0]), pointer_iterator (&values[3])); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* iter (const int) */
  for (Walker<const int> wp = walker (pointer_iterator ((const int*) &values[0]), pointer_iterator ((const int*) &values[3])); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* *iter (int*) */
  static int *pointers[] = { &values[0], &values[1], &values[2], &values[1], &values[0] };
  for (Walker<int> wp = value_walker (pointer_iterator (&pointers[0]), pointer_iterator (&pointers[3])); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* *iter (const int*) */
  for (Walker<const int> wp = value_walker (pointer_iterator ((const int**) &pointers[0]), pointer_iterator ((const int**) &pointers[3])); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* *iter (int*const) */
  for (Walker<int> wp = value_walker (pointer_iterator ((int*const*) &pointers[0]), pointer_iterator ((int*const*) &pointers[3])); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* *iter (const int*const) */
  for (Walker<const int> wp = value_walker (pointer_iterator ((const int*const*) &pointers[0]), pointer_iterator ((const int*const*) &pointers[3])); wp.has_next(); wp++) printf ("%d", *wp);
  printf (" ");
  /* done */
  printf ("\n");
}
REGISTER_LOGTEST ("General/Walker Iterator Types", test_walker);

extern "C" int
main (int   argc,
      char *argv[])
{
  init_test_app (String ("Rapicorn/") + RAPICORN__FILE__, &argc, argv);
  return Test::run();
}

} // anon
