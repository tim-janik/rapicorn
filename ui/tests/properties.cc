/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include <rcore/testutils.hh>
#include <rapicorn.hh>
#include <stdio.h>

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
      MakeProperty (PropertyHost, bool_prop,            "Label", "Blurb", "rw"),
      MakeProperty (PropertyHost, const_bool_prop,      "Label", "Blurb", "rw"),
      MakeProperty (PropertyHost, uint_prop,            "Label", "Blurb", 0, 100, 5, "rw"),
      MakeProperty (PropertyHost, const_uint_prop,      "Label", "Blurb", 0, 100, 5, "rw"),
      MakeProperty (PropertyHost, int_prop,             "Label", "Blurb", 0, 100, 5, "rw"),
      MakeProperty (PropertyHost, const_int_prop,       "Label", "Blurb", 0, 100, 5, "rw"),
      MakeProperty (PropertyHost, double_prop,          "Label", "Blurb", 0, 100, 0.5, "rw"),
      MakeProperty (PropertyHost, const_double_prop,    "Label", "Blurb", 0, 100, 0.5, "rw"),
      MakeProperty (PropertyHost, float_prop,           "Label", "Blurb", 0, 100, 0.5, "rw"),
      MakeProperty (PropertyHost, const_float_prop,     "Label", "Blurb", 0, 100, 0.5, "rw"),
      MakeProperty (PropertyHost, point_prop,           "Label", "Blurb", Point (0,0), Point (10,10), "rw"),
      MakeProperty (PropertyHost, string_prop,          "Label", "Blurb", "rw"),
      MakeProperty (PropertyHost, const_string_prop,    "Label", "Blurb", "rw"),
      MakeProperty (PropertyHost, enum_prop,            "Label", "Blurb", "rw"),
      MakeProperty (PropertyHost, const_enum_prop,      "Label", "Blurb", "rw"),
    };
    static const PropertyList property_list (properties);
    return property_list;
  }
};

static void
property_test()
{
  PropertyHost ph;
  // printf ("created %d properties.\n", ph.list_properties().n_properties);
  TASSERT (ph.list_properties().n_properties == 15);
}
REGISTER_TEST ("Objects/Property Test", property_test);

} // anon
