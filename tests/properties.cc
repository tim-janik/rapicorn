/* Tests
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
//#define TEST_VERBOSE
#include <rapicorn/birnettests.h>
#include <rapicorn/rapicorn.hh>

#define MakeProperty    RAPICORN_MakeProperty

namespace {
using namespace Rapicorn;

class PropertyHost {
  bool          bool_prop ()             		{ return false; }
  void          bool_prop (bool v)       		{}
  bool          const_bool_prop () const 		{ return false; }
  void          const_bool_prop (bool v) 		{}
  uint          uint_prop ()             		{ return 0; }
  void          uint_prop (uint v)       		{}
  uint          const_uint_prop () const 		{ return 0; }
  void          const_uint_prop (uint v) 		{}
  int           int_prop ()              		{ return 0; }
  void          int_prop (int v)         		{}
  int           const_int_prop () const  		{ return 0; }
  void          const_int_prop (int v)          	{}
  double        double_prop ()               		{ return 0; }
  void          double_prop (double v)       		{}
  double        const_double_prop () const   		{ return 0; }
  void          const_double_prop (double v) 		{}
  float         float_prop ()                		{ return 0; }
  void          float_prop (float v)         		{}
  float         const_float_prop () const    		{ return 0; }
  void          const_float_prop (float v)   		{}
  Point         point_prop ()             		{ return Point (7, 7); }
  void          point_prop (Point v)       		{}
  Point         const_point_prop () const      		{ return Point (7, 7); }
  void          const_point_prop (Point v)     		{}
  String        string_prop ()               		{ return ""; }
  void          string_prop (const String &v)      	{}
  String        const_string_prop () const   		{ return ""; }
  void          const_string_prop (const String &v) 	{}
  FrameType     enum_prop ()                    	{ return FRAME_NONE; }
  void          enum_prop (FrameType ft)        	{}
  FrameType     const_enum_prop () const        	{ return FRAME_NONE; }
  void          const_enum_prop (FrameType ft)  	{}
public:
  virtual      ~PropertyHost () {}
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (PropertyHost, bool_prop,            "Label", "Blurb", false, "rw"),
      MakeProperty (PropertyHost, const_bool_prop,      "Label", "Blurb", false, "rw"),
      MakeProperty (PropertyHost, uint_prop,            "Label", "Blurb", 50, 0, 100, 5, "rw"),
      MakeProperty (PropertyHost, const_uint_prop,      "Label", "Blurb", 50, 0, 100, 5, "rw"),
      MakeProperty (PropertyHost, int_prop,             "Label", "Blurb", 50, 0, 100, 5, "rw"),
      MakeProperty (PropertyHost, const_int_prop,       "Label", "Blurb", 50, 0, 100, 5, "rw"),
      MakeProperty (PropertyHost, double_prop,          "Label", "Blurb", 50.5, 0, 100, 0.5, "rw"),
      MakeProperty (PropertyHost, const_double_prop,    "Label", "Blurb", 50.5, 0, 100, 0.5, "rw"),
      MakeProperty (PropertyHost, float_prop,           "Label", "Blurb", 50.5, 0, 100, 0.5, "rw"),
      MakeProperty (PropertyHost, const_float_prop,     "Label", "Blurb", 50.5, 0, 100, 0.5, "rw"),
      MakeProperty (PropertyHost, point_prop,           "Label", "Blurb", Point (1,1), Point (0,0), Point (10,10), "rw"),
      MakeProperty (PropertyHost, string_prop,          "Label", "Blurb", "", "rw"),
      MakeProperty (PropertyHost, const_string_prop,    "Label", "Blurb", "", "rw"),
      MakeProperty (PropertyHost, enum_prop,            "Label", "Blurb", FRAME_NONE, "rw"),
      MakeProperty (PropertyHost, const_enum_prop,      "Label", "Blurb", FRAME_NONE, "rw"),
    };
    static const PropertyList property_list (properties);
    return property_list;
  }
};

static void
property_test()
{
  PropertyHost ph;
  printf ("created %d properties.\n", ph.list_properties().n_properties);
}

extern "C" int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  /* initialize rapicorn */
  //rapicorn_init_with_gtk_thread (&argc, &argv, NULL);
  
  property_test();
  return 0;
}

} // anon
