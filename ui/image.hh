// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class Image : public virtual WidgetImpl {
protected:
  const PropertyList&   __aida_properties__ ();
public:
  virtual void          pixbuf  (const Pixbuf &pixbuf) = 0;
  virtual Pixbuf        pixbuf  () = 0;
  virtual void          source  (const String &uri) = 0;
  virtual String        source  () const = 0;
  virtual void          stock   (const String &stock_id) = 0;
  virtual String        stock   () const = 0;
};

} // Rapicorn

#endif  // __RAPICORN_IMAGE_HH__
