// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/item.hh>

namespace Rapicorn {

class Image : public virtual WidgetImpl {
  virtual String        image_file      () const { RAPICORN_ASSERT_UNREACHED(); }
  virtual String        stock_pixmap    () const { RAPICORN_ASSERT_UNREACHED(); }
protected:
  const PropertyList&   _property_list ();
public:
  virtual void             pixbuf       (const Pixbuf &pixbuf) = 0;
  virtual Pixbuf           pixbuf       (void) = 0;
  virtual void /*errno*/   stock_pixmap (const String &stock_name) = 0;
  virtual void /*errno*/   image_file   (const String &filename) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_HH__ */
