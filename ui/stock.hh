// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_STOCK_HH__
#define __RAPICORN_STOCK_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

// == Stock ==
class Stock {
  const String  stock_id_;
protected:
  String        element         (const String &key) const;
public:
  struct Icon {
    String resource, element;
  };
  explicit      Stock           (const String &stock_id);
  String        label           () const;
  String        tooltip         () const;
  String        accelerator     () const;
  Icon          icon            () const;
};

} // Rapicorn

#endif // __RAPICORN_STOCK_HH__
