// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "viewport.hh"
#include "factory.hh"

namespace Rapicorn {

ViewportImpl::ViewportImpl ()
{}

ViewportImpl::~ViewportImpl ()
{
  AncestryCache *ancestry_cache = const_cast<AncestryCache*> (ResizeContainerImpl::fetch_ancestry_cache());
  ancestry_cache->viewport = NULL;
}

const WidgetImpl::AncestryCache*
ViewportImpl::fetch_ancestry_cache ()
{
  AncestryCache *ancestry_cache = const_cast<AncestryCache*> (ResizeContainerImpl::fetch_ancestry_cache());
  ancestry_cache->viewport = this;
  return ancestry_cache;
}

} // Rapicorn
