/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 */
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
#include <stdio.h>

namespace {
using namespace Rapicorn;

class PropertyHost {
  bool          bool_prop () const       		{ return false; }
  void          bool_prop (bool v)       		{}
  bool          const_bool_prop () const 		{ return false; }
  void          const_bool_prop (bool v) 		{}
  uint          uint_prop () const       		{ return 0; }
  void          uint_prop (uint v)       		{}
  uint          const_uint_prop () const 		{ return 0; }
  void          const_uint_prop (uint v) 		{}
  int           int_prop () const        		{ return 0; }
  void          int_prop (int v)         		{}
  int           const_int_prop () const  		{ return 0; }
  void          const_int_prop (int v)          	{}
  double        double_prop () const         		{ return 0; }
  void          double_prop (double v)       		{}
  double        const_double_prop () const   		{ return 0; }
  void          const_double_prop (double v) 		{}
  Point         point_prop () const       		{ return Point (7, 7); }
  void          point_prop (Point v)       		{}
  Point         const_point_prop () const      		{ return Point (7, 7); }
  void          const_point_prop (Point v)     		{}
  String        string_prop () const         		{ return ""; }
  void          string_prop (const String &v)      	{}
  String        const_string_prop () const   		{ return ""; }
  void          const_string_prop (const String &v) 	{}
  DrawFrame     enum_prop () const              	{ return FRAME_NONE; }
  void          enum_prop (DrawFrame ft)        	{}
  DrawFrame     const_enum_prop () const        	{ return FRAME_NONE; }
  void          const_enum_prop (DrawFrame ft)  	{}
public:
  virtual      ~PropertyHost () {}
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      RAPICORN_AIDA_PROPERTY (PropertyHost, bool_prop,            "Label", "Blurb", "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, const_bool_prop,      "Label", "Blurb", "rw"),
      // RAPICORN_AIDA_PROPERTY (PropertyHost, uint_prop,         "Label", "Blurb", 0, 100, 5, "rw"),
      // RAPICORN_AIDA_PROPERTY (PropertyHost, const_uint_prop,   "Label", "Blurb", 0, 100, 5, "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, int_prop,             "Label", "Blurb", 0, 100, 5, "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, const_int_prop,       "Label", "Blurb", 0, 100, 5, "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, double_prop,          "Label", "Blurb", 0, 100, 0.5, "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, const_double_prop,    "Label", "Blurb", 0, 100, 0.5, "rw"),
      // RAPICORN_AIDA_PROPERTY (PropertyHost, point_prop,        "Label", "Blurb", Point (0,0), Point (10,10), "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, string_prop,          "Label", "Blurb", "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, const_string_prop,    "Label", "Blurb", "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, enum_prop,            "Label", "Blurb", "rw"),
      RAPICORN_AIDA_PROPERTY (PropertyHost, const_enum_prop,      "Label", "Blurb", "rw"),
    };
    static const PropertyList property_list (properties);
    return property_list;
  }
};

static void
property_test()
{
  PropertyHost ph;
  size_t n_properties = 0;
  Aida::Property **properties = ph.list_properties().list_properties (&n_properties);
  (void) properties;
  // printf ("created %d properties.\n", ph.list_properties().n_properties);
  TASSERT (n_properties == 13 - 3);
}
REGISTER_UITHREAD_TEST ("Objects/Property Test", property_test);

} // anon
