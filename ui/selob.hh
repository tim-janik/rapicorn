// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_SELOB_HH__
#define __RAPICORN_SELOB_HH__

#include <ui/selector.hh>
#include <ui/widget.hh>
#include <ui/models.hh>

namespace Rapicorn {
namespace Selector {

class SelobAllocator;

class SelobWidget : public Selob {
  WidgetImplP       widget_;
  SelobWidget      *parent_;
  int64           n_children_;
  SelobAllocator &allocator_;
  friend class SelobAllocator;
  explicit             SelobWidget       (SelobAllocator &allocator, WidgetImpl &widget);
  void                 cache_parent    ();
  void                 cache_n_children();
public:
  virtual             ~SelobWidget       ();
  virtual String       get_id          ();
  virtual String       get_type        ();
  virtual ConstTypes&  get_type_list   ();
  virtual bool         has_property    (const String &name);
  virtual String       get_property    (const String &name);
  virtual Selob*       get_parent      ();
  virtual Selob*       get_sibling     (int64 dir);
  virtual bool         has_children    ();
  virtual int64        n_children      ();
  virtual Selob*       get_child       (int64 index);
  virtual bool         is_nth_child    (int64 nth1based);
  virtual Selob*       pseudo_selector (const String &ident, const String &arg, String &error);
};

class SelobListModel : public Selob {
  ListModelIface      &lmodel_;
  StringVector         type_list_;
  String               row_constraint_, col_constraint_, value_constraint_, type_constraint_;
  uint                 f_row_constraint : 1, f_col_constraint : 1, f_value_constraint : 1, f_type_constraint : 1;
  explicit             SelobListModel  (SelobAllocator &allocator, ListModelIface &lmodel);
public:
  virtual             ~SelobListModel  ();
  virtual String       get_id          ();
  virtual String       get_type        ();
  virtual ConstTypes&  get_type_list   ();
  virtual bool         has_property    (const String &name)     { return false; }
  virtual String       get_property    (const String &name)     { return ""; }
  virtual Selob*       get_parent      ()                       { return NULL; }
  virtual Selob*       get_sibling     (int64 dir)              { return NULL; }
  virtual bool         has_children    ()                       { return false; }
  virtual int64        n_children      ()                       { return 0; }
  virtual Selob*       get_child       (int64 index)            { return NULL; }
  virtual bool         is_nth_child    (int64 nth1based)        { return false; }
  virtual Selob*       pseudo_selector (const String &ident, const String &arg, String &error);
};

class SelobAllocator {
  vector<SelobWidget*> selobs_;
public:
  explicit               SelobAllocator  ();
  virtual               ~SelobAllocator  ();
  SelobWidget*             widget_selob      (WidgetImpl &widget);
  WidgetImpl*              selob_widget      (Selob    &selob);
  static SelobAllocator* selob_allocator (Selob    &selob);
};

} // Selector
} // Rapicorn

#endif /* __RAPICORN_SELOB_HH__ */
