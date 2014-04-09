// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_IMAGE_FRAME_HH__
#define __RAPICORN_IMAGE_FRAME_HH__

#include <ui/container.hh>

namespace Rapicorn {

class ImageFrame : public virtual ContainerImpl {
protected:
  virtual const PropertyList& __aida_properties__ ();
public:
  virtual String element        () const = 0;
  virtual void   element        (const String &id) = 0;
  virtual bool   overlap_child  () const = 0;
  virtual void   overlap_child  (bool overlap) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_FRAME_HH__ */
