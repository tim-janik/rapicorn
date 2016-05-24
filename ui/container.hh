// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CONTAINER_HH__
#define __RAPICORN_CONTAINER_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ResizeContainerImpl;

// == FocusIndicator ==
class FocusIndicator : public virtual Aida::ImplicitBase {
public:
  virtual void  focusable_container_change (ContainerImpl&) = 0;
};

// == Container ==
struct ContainerImpl : public virtual WidgetImpl, public virtual ContainerIface {
  friend              class WidgetImpl;
  friend              class WindowImpl;
  void                uncross_descendant    (WidgetImpl          &descendant);
  size_t              widget_cross_link     (WidgetImpl           &owner,
                                             WidgetImpl           &link,
                                             const WidgetSlot &uncross);
  void                widget_cross_unlink   (WidgetImpl           &owner,
                                             WidgetImpl           &link,
                                             size_t              link_id);
  void                widget_uncross_links  (WidgetImpl           &owner,
                                             WidgetImpl           &link);
  WidgetGroup*        retrieve_widget_group (const String &group_name, WidgetGroupType group_type, bool force_create);
protected:
  virtual            ~ContainerImpl     ();
  virtual void        fabricated        () override;
  virtual void        do_changed        (const String &name) override;
  virtual void        add_child         (WidgetImpl           &widget) = 0;
  virtual void        repack_child      (WidgetImpl &widget, const PackInfo &orig, const PackInfo &pnew);
  virtual void        remove_child      (WidgetImpl           &widget) = 0;
  virtual void        unparent_child    (WidgetImpl           &widget);
  virtual void        dispose_widget    (WidgetImpl           &widget);
  virtual void        hierarchy_changed (WidgetImpl           *old_toplevel);
  virtual bool        move_focus        (FocusDir              fdir);
  void                stash_child       (WidgetImpl &child, bool);
  virtual void        focus_lost        ()                              { set_focus_child (NULL); }
  virtual void        set_focus_child   (WidgetImpl *widget);
  virtual void        scroll_to_child   (WidgetImpl &widget);
  virtual void        dump_test_data    (TestStream &tstream);
  static Allocation   layout_child      (WidgetImpl &child, const Allocation &carea);
  static void         layout_child_allocation (WidgetImpl &child, const Allocation &carea);
  static Requisition  size_request_child (WidgetImpl &child, bool *hspread = NULL, bool *vspread = NULL);
  virtual void        selectable_child_changed (WidgetChain &chain);
  void                set_child_parent (WidgetImpl &child, ContainerImpl *parent) { child.set_parent (parent); }
public:
  virtual WidgetImplP*  begin             () const = 0;
  virtual WidgetImplP*  end               () const = 0;
  WidgetImpl*  get_focus_child            () const;
  void         register_focus_indicator   (FocusIndicator&);
  void         unregister_focus_indicator (FocusIndicator&);
  virtual void foreach_recursive          (const std::function<void (WidgetImpl&)> &f) override;
  void                  child_container (ContainerImpl  *child_container);
  ContainerImpl&        child_container ();
  virtual size_t        n_children      () = 0;
  virtual WidgetImpl*   nth_child       (size_t nth) = 0;
  bool                  has_children    ()                              { return 0 != n_children(); }
  bool                  remove          (WidgetImpl           &widget);
  bool                  remove          (WidgetImpl           *widget)  { assert_return (widget != NULL, 0); return remove (*widget); }
  void                  add             (WidgetImpl                   &widget);
  void                  add             (WidgetImpl                   *widget);
  void                   point_descendants (Point widget_point, std::vector<WidgetImplP> &stack) const;
  virtual ContainerImpl* as_container_impl ()                           { return this; }
  void                   debug_tree        (String indent = String());
  // ContainerIface
  virtual WidgetIfaceP create_widget    (const String &widget_identifier, const StringSeq &args) override;
  virtual void         remove_widget    (WidgetIface &child) override;
};

// == Single Child Container ==
class SingleContainerImpl : public virtual ContainerImpl {
  WidgetImplP           child_widget;
protected:
  virtual void          size_request            (Requisition &requisition);
  virtual void          size_allocate           (Allocation area, bool changed);
  virtual void          render                  (RenderContext &rcontext) {}
  WidgetImpl&           get_child               () { critical_unless (child_widget != NULL); return *child_widget; }
  virtual              ~SingleContainerImpl     ();
  virtual WidgetImplP*  begin                   () const override;
  virtual WidgetImplP*  end                     () const override;
  virtual size_t        n_children              () { return child_widget ? 1 : 0; }
  virtual WidgetImpl*   nth_child               (size_t nth) { return nth == 0 ? child_widget.get() : NULL; }
  bool                  has_visible_child       () { return child_widget && child_widget->visible(); }
  bool                  has_drawable_child      () { return child_widget && child_widget->drawable(); }
  virtual void          add_child               (WidgetImpl   &widget);
  virtual void          remove_child            (WidgetImpl   &widget);
  explicit              SingleContainerImpl     ();
};

// == AncestryCache ==
struct WidgetImpl::AncestryCache {
  ResizeContainerImpl *resize_container;
  ViewportImpl        *viewport;
  WindowImpl          *window;
  constexpr AncestryCache() : resize_container (NULL), viewport (NULL), window (NULL) {}
};

// == Resize Container ==
class ResizeContainerImpl : public virtual SingleContainerImpl {
  AncestryCache         ancestry_cache_;
  virtual void          widget_invalidate       (WidgetFlag mask) override;
protected:
  virtual const AncestryCache* fetch_ancestry_cache () override;
  virtual void          hierarchy_changed       (WidgetImpl *old_toplevel) override;
  explicit              ResizeContainerImpl     ();
  virtual              ~ResizeContainerImpl     ();
};

// == Multi Child Container ==
class MultiContainerImpl : public virtual ContainerImpl {
  std::vector<WidgetImplP> widgets;
protected:
  virtual              ~MultiContainerImpl      ();
  virtual void          render                  (RenderContext &rcontext) override {}
  virtual void          add_child               (WidgetImpl   &widget) override;
  virtual void          remove_child            (WidgetImpl   &widget) override;
  explicit              MultiContainerImpl      ();
public:
  virtual WidgetImplP*  begin                   () const override;
  virtual WidgetImplP*  end                     () const override;
  virtual size_t        n_children              () override           { return widgets.size(); }
  virtual WidgetImpl*   nth_child               (size_t nth) override { return nth < widgets.size() ? widgets[nth].get() : NULL; }
  void                  raise_child             (WidgetImpl   &widget);
  void                  lower_child             (WidgetImpl   &widget);
  void                  remove_all_children     ();
};

} // Rapicorn

#endif  /* __RAPICORN_CONTAINER_HH__ */
