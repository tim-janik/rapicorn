/* Rapicorn
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
 *
 * - This file is derived from GtkTable code which is
 *   Copyright (C) 1996-2002 by the GTK+ project.
 */
#include "tableimpl.hh"
#include "factory.hh"

#include <algorithm>

namespace Rapicorn {

static uint
left_attach (const ItemImpl::PackInfo &pi)
{
  return iround (MAX (0, pi.hposition));
}

static uint
right_attach (const ItemImpl::PackInfo &pi)
{
  double r = pi.hposition + pi.hspan;
  double l = left_attach (pi);
  return iround (MAX (l + 1, r));
}

static uint
bottom_attach (const ItemImpl::PackInfo &pi)
{
  return iround (MAX (0, pi.vposition));
}

static uint
top_attach (const ItemImpl::PackInfo &pi)
{
  double t = pi.vposition + pi.vspan;
  double b = bottom_attach (pi);
  return iround (MAX (b + 1, t));
}

TableImpl::TableImpl() :
  default_row_spacing (0),
  default_col_spacing (0),
  homogeneous_items (false)
{
  resize_table (1, 1);
}

bool
TableImpl::is_row_used (uint row)
{
  if (row < rows.size())
    for (ChildWalker cw = local_children(); cw.has_next(); cw++)
      {
        const PackInfo &pi = cw->pack_info();
        if (row >= bottom_attach (pi) && row < top_attach (pi))
          return true;
      }
  return false;
}

bool
TableImpl::is_col_used (uint col)
{
  if (col < cols.size())
    for (ChildWalker cw = local_children(); cw.has_next(); cw++)
      {
        const PackInfo &pi = cw->pack_info();
        if (col >= left_attach (pi) && col < right_attach (pi))
          return true;
      }
  return false;
}

void
TableImpl::resize_table (uint n_cols,
                         uint n_rows)
{
  n_rows = MAX (n_rows, 1);
  n_cols = MAX (n_cols, 1);
  if (n_rows == rows.size() && n_cols == cols.size())
    return;
  /* grow as children require */
  if (n_rows < rows.size() || n_cols < cols.size())
    for (ChildWalker cw = local_children(); cw.has_next(); cw++)
      {
        const PackInfo &pi = cw->pack_info();
        n_rows = MAX (n_rows, top_attach (pi));
        n_cols = MAX (n_cols, right_attach (pi));
      }
  /* resize rows and cols */
  bool need_invalidate = false;
  if (n_rows != rows.size())
    {
      uint i = rows.size();
      rows.resize (n_rows);
      for (; i < rows.size(); i++)
        rows[i].spacing = default_row_spacing;
      need_invalidate = true;
    }
  if (n_cols != cols.size())
    {
      uint i = cols.size();
      cols.resize (n_cols);
      for (; i < cols.size(); i++)
        cols[i].spacing = default_col_spacing;
      need_invalidate = true;
    }
  if (need_invalidate)
    invalidate();
}

void
TableImpl::insert_rows (uint first_row, uint n_rows)
{
  if (!n_rows)
    return;
  resize_table (cols.size(), rows.size() + n_rows);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      const PackInfo &pi = cw->pack_info();
      if (bottom_attach (pi) >= first_row)
        cw->vposition (cw->vposition() + n_rows);
      else if (first_row < top_attach (pi))
        cw->vspan (cw->vspan() + n_rows);
    }
}

void
TableImpl::insert_cols (uint first_col, uint n_cols)
{
  if (!n_cols)
    return;
  resize_table (cols.size() + n_cols, rows.size());
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      const PackInfo &pi = cw->pack_info();
      if (left_attach (pi) >= first_col)
        cw->hposition (cw->hposition() + n_cols);
      else if (first_col < right_attach (pi))
        cw->hspan (cw->hspan() + n_cols);
    }
}

void
TableImpl::col_spacing (uint cspacing)
{
  default_col_spacing = cspacing;
  for (uint col = 0; col < cols.size(); col++)
    cols[col].spacing = default_col_spacing;
  invalidate();
}

void
TableImpl::row_spacing (uint rspacing)
{
  default_row_spacing = rspacing;
  for (uint row = 0; row < rows.size(); row++)
    rows[row].spacing = default_row_spacing;
  invalidate();
}

TableImpl::~TableImpl()
{}

void
TableImpl::repack_child (ItemImpl       &item,
                         const PackInfo &orig,
                         const PackInfo &pnew)
{
  uint n_cols = right_attach (pnew), n_rows = top_attach (pnew);
  if (n_cols > cols.size() || n_rows > rows.size())
    resize_table (n_cols, n_rows);
  MultiContainerImpl::repack_child (item, orig, pnew);
}

void
TableImpl::size_request (Requisition &requisition)
{
  size_request_init ();
  size_request_pass1 ();
  size_request_pass2 ();
  size_request_pass3 ();
  size_request_pass2 ();
  
  for (uint col = 0; col < cols.size(); col++)
    requisition.width += cols[col].requisition;
  for (uint col = 0; col + 1 < cols.size(); col++)
    requisition.width += cols[col].spacing;
  for (uint row = 0; row < rows.size(); row++)
    requisition.height += rows[row].requisition;
  for (uint row = 0; row + 1 < rows.size(); row++)
    requisition.height += rows[row].spacing;
}

void
TableImpl::size_allocate (Allocation area)
{
  allocation (area);
  size_allocate_init ();
  size_allocate_pass1 ();
  size_allocate_pass2 ();
}

const PropertyList&
Table::list_properties()
{
  static Property *properties[] = {
    MakeProperty (Table, homogeneous, _("Homogeneous"), _("Whether all children get the same size"), "rw"),
    MakeProperty (Table, col_spacing, _("Column Spacing"), _("The amount of space between two consecutive columns"), 0, 65535, 10, "rw"),
    MakeProperty (Table, row_spacing, _("Row Spacing"), _("The amount of space between two consecutive rows"), 0, 65535, 10, "rw"),
  };
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

void
TableImpl::size_request_init()
{
  for (uint row = 0; row < rows.size(); row++)
    {
      rows[row].requisition = 0;
      rows[row].expand = false;
    }
  for (uint col = 0; col < cols.size(); col++)
    {
      cols[col].requisition = 0;
      cols[col].expand = false;
    }
  
  bool chspread = false, cvspread = false;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      /* size request all children */
      if (!cw->allocatable())
        continue;
      const PackInfo &pi = cw->pack_info();
      chspread |= cw->hspread();
      cvspread |= cw->vspread();
      /* expand cols with single-column expand children */
      if (left_attach (pi) + 1 == right_attach (pi) && cw->hexpand())
        cols[left_attach (pi)].expand = true;
      /* expand rows with single-column expand children */
      if (bottom_attach (pi) + 1 == top_attach (pi) && cw->vexpand())
        rows[bottom_attach (pi)].expand = true;
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
TableImpl::size_request_pass1()
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      if (!cw->allocatable())
        continue;
      Requisition crq = cw->size_request();
      const PackInfo &pi = cw->pack_info();
      /* fetch requisition from single-column children */
      if (left_attach (pi) + 1 == right_attach (pi))
        {
          uint width = iround (crq.width + pi.left_spacing + pi.right_spacing);
          cols[left_attach (pi)].requisition = MAX (cols[left_attach (pi)].requisition, width);
        }
      /* fetch requisition from single-row children */
      if (bottom_attach (pi) + 1 == top_attach (pi))
        {
          uint height = iround (crq.height + pi.bottom_spacing + pi.top_spacing);
          rows[bottom_attach (pi)].requisition = MAX (rows[bottom_attach (pi)].requisition, height);
        }
    }
}

void
TableImpl::size_request_pass2()
{
  if (homogeneous())
    {
      uint max_width = 0;
      uint max_height = 0;
      /* maximise requisition */
      for (uint col = 0; col < cols.size(); col++)
        max_width = MAX (max_width, cols[col].requisition);
      for (uint row = 0; row < rows.size(); row++)
        max_height = MAX (max_height, rows[row].requisition);
      /* assign equal requisitions */
      for (uint col = 0; col < cols.size(); col++)
        cols[col].requisition = max_width;
      for (uint row = 0; row < rows.size(); row++)
        rows[row].requisition = max_height;
    }
}

void
TableImpl::size_request_pass3()
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      if (!cw->allocatable())
        continue;
      const PackInfo &pi = cw->pack_info();
      /* request remaining space for multi-column children */
      if (left_attach (pi) + 1 != right_attach (pi))
        {
          Requisition crq = cw->size_request();
          /* Check and see if there is already enough space for the child. */
          uint width = 0;
          for (uint col = left_attach (pi); col < right_attach (pi); col++)
            {
              width += cols[col].requisition;
              if (col + 1 < right_attach (pi))
                width += cols[col].spacing;
            }
          /* If we need to request more space for this child to fill
           *  its requisition, then divide up the needed space amongst the
           *  columns it spans, favoring expandable columns if any.
           */
          if (width < crq.width + pi.left_spacing + pi.right_spacing)
            {
              bool force_expand = false;
              uint n_expand = 0;
              width = iround (crq.width + pi.left_spacing + pi.right_spacing - width);
              for (uint col = left_attach (pi); col < right_attach (pi); col++)
                if (cols[col].expand)
                  n_expand++;
              if (n_expand == 0)
                {
                  n_expand = right_attach (pi) - left_attach (pi);
                  force_expand = true;
                }
              for (uint col = left_attach (pi); col < right_attach (pi); col++)
                if (force_expand || cols[col].expand)
                  {
                    uint extra = width / n_expand;
                    cols[col].requisition += extra;
                    width -= extra;
                    n_expand--;
                  }
            }
        }
      /* request remaining space for multi-row children */
      if (bottom_attach (pi) + 1 != top_attach (pi))
        {
          Requisition crq = cw->size_request();
          /* Check and see if there is already enough space for the child. */
          uint height = 0;
          for (uint row = bottom_attach (pi); row < top_attach (pi); row++)
            {
              height += rows[row].requisition;
              if (row + 1 < top_attach (pi))
                height += rows[row].spacing;
            }
          /* If we need to request more space for this child to fill
           *  its requisition, then divide up the needed space amongst the
           *  rows it spans, favoring expandable rows if any.
           */
          if (height < crq.height + pi.bottom_spacing + pi.top_spacing)
            {
              bool force_expand = false;
              uint n_expand = 0;
              height = iround (crq.height + pi.bottom_spacing + pi.top_spacing - height);
              for (uint row = bottom_attach (pi); row < top_attach (pi); row++)
                if (rows[row].expand)
                  n_expand++;
              if (n_expand == 0)
                {
                  n_expand = top_attach (pi) - bottom_attach (pi);
                  force_expand = true;
                }
              for (uint row = bottom_attach (pi); row < top_attach (pi); row++)
                if (force_expand || rows[row].expand)
                  {
                    uint extra = height / n_expand;
                    rows[row].requisition += extra;
                    height -= extra;
                    n_expand--;
                  }
            }
        }
    }
}

void
TableImpl::size_allocate_init()
{
  /* Initialize the rows and cols.
   *  By default, rows and cols do not expand and do shrink.
   *  Those values are modified by the children that occupy
   *  the rows and cols.
   */
  for (uint col = 0; col < cols.size(); col++)
    {
      cols[col].allocation = cols[col].requisition;
      cols[col].need_expand = false;
      cols[col].need_shrink = true;
      cols[col].expand = false;
      cols[col].shrink = true;
      cols[col].empty = true;
    }
  for (uint row = 0; row < rows.size(); row++)
    {
      rows[row].allocation = rows[row].requisition;
      rows[row].need_expand = false;
      rows[row].need_shrink = true;
      rows[row].expand = false;
      rows[row].shrink = true;
      rows[row].empty = true;
    }
  /* adjust the row and col flags from expand/shrink flags of single row/col children */
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      if (!cw->allocatable())
        continue;
      const PackInfo &pi = cw->pack_info();
      if (left_attach (pi) + 1 == right_attach (pi))
        {
          cols[left_attach (pi)].expand |= cw->hexpand();
          cols[left_attach (pi)].shrink &= cw->hshrink();
          cols[left_attach (pi)].empty = false;
        }
      if (bottom_attach (pi) + 1 == top_attach (pi))
        {
          rows[bottom_attach (pi)].expand |= cw->vexpand();
          rows[bottom_attach (pi)].shrink &= cw->vshrink();
          rows[bottom_attach (pi)].empty = false;
        }
    }
  /* adjust the row and col flags from expand/shrink flags of multi row/col children */
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      if (!cw->allocatable())
        continue;
      const PackInfo &pi = cw->pack_info();
      if (left_attach (pi) + 1 != right_attach (pi))
        {
          uint col;
          for (col = left_attach (pi); col < right_attach (pi); col++)
            cols[col].empty = false;
          if (cw->hexpand())
            {
              for (col = left_attach (pi); col < right_attach (pi); col++)
                if (cols[col].expand)
                  break;
              if (col >= right_attach (pi)) /* no expand col found */
                for (col = left_attach (pi); col < right_attach (pi); col++)
                  cols[col].need_expand = true;
            }
          if (!cw->hshrink())
            {
              for (col = left_attach (pi); col < right_attach (pi); col++)
                if (!cols[col].shrink)
                  break;
              if (col >= right_attach (pi)) /* no shrink col found */
                for (col = left_attach (pi); col < right_attach (pi); col++)
                  cols[col].need_shrink = false;
            }
        }
      if (bottom_attach (pi) + 1 != top_attach (pi))
        {
          uint row;
          for (row = bottom_attach (pi); row < top_attach (pi); row++)
            rows[row].empty = false;
          if (cw->vexpand())
            {
              for (row = bottom_attach (pi); row < top_attach (pi); row++)
                if (rows[row].expand)
                  break;
              if (row >= top_attach (pi)) /* no expand row found */
                for (row = bottom_attach (pi); row < top_attach (pi); row++)
                  rows[row].need_expand = true;
            }
          if (!cw->vshrink())
            {
              for (row = bottom_attach (pi); row < top_attach (pi); row++)
                if (!rows[row].shrink)
                  break;
              if (row >= top_attach (pi)) /* no shrink row found */
                for (row = bottom_attach (pi); row < top_attach (pi); row++)
                  rows[row].need_shrink = false;
            }
        }
    }
  /* Loop over the columns and set the expand and shrink values
   *  if the column can be expanded or shrunk.
   */
  for (uint col = 0; col < cols.size(); col++)
    if (cols[col].empty)
      {
        cols[col].expand = false;
        cols[col].shrink = false;
      }
    else
      {
        cols[col].expand |= cols[col].need_expand;
        cols[col].shrink &= cols[col].need_shrink;
      }
  /* Loop over the rows and set the expand and shrink values
   *  if the row can be expanded or shrunk.
   */
  for (uint row = 0; row < rows.size(); row++)
    if (rows[row].empty)
      {
        rows[row].expand = false;
        rows[row].shrink = false;
      }
    else
      {
        rows[row].expand |= rows[row].need_expand;
        rows[row].shrink &= rows[row].need_shrink;
      }
}

bool
TableImpl::RowCol::lesser_allocation (const TableImpl::RowCol *const &v1,
                                      const TableImpl::RowCol *const &v2)
{
  return v1->allocation < v2->allocation;
}

void
TableImpl::size_allocate_pass1 ()
{
  /* If we were allocated more space than we requested
   *  then we have to expand any expandable rows and columns
   *  to fill in the extra space.
   */
  Allocation area = allocation();
  const int real_width = iround (area.width);
  const int real_height = iround (area.height);
  if (homogeneous())
    {
      int nexpand, extra;
      if (!has_children())
        nexpand = 1;
      else
        {
          nexpand = 0;
          for (uint col = 0; col < cols.size(); col++)
            if (cols[col].expand)
              {
                nexpand += 1;
                break;
              }
        }
      if (nexpand)
        {
          int width = real_width;
          for (uint col = 0; col + 1 < cols.size(); col++)
            width -= cols[col].spacing;
          for (uint col = 0; col < cols.size(); col++)
            {
              extra = width / (cols.size() - col);
              cols[col].allocation = MAX (1, extra);
              width -= extra;
            }
        }
    }
  else
    {
      int width = 0, nexpand = 0, nshrink = 0, extra;
      for (uint col = 0; col < cols.size(); col++)
        {
          width += cols[col].requisition;
          nexpand += cols[col].expand;
          nshrink += cols[col].shrink;
        }
      for (uint col = 0; col + 1 < cols.size(); col++)
        width += cols[col].spacing;
      /* distribute extra space that we were allocated beyond requisition */
      if (width < real_width && nexpand >= 1)
        {
          width = real_width - width;
          for (uint jcol = 0; jcol < cols.size(); jcol++)
            {
              uint col = cols.size() / 2 + ((1 + jcol) >> 1) * (1 & jcol ? -1 : +1); // start distributing in the center
              if (cols[col].expand)
                {
                  extra = width / nexpand;
                  cols[col].allocation += extra;
                  width -= extra;
                  nexpand -= 1;
                }
            }
        }
      /* shrink until we fit the size given */
      if (width > real_width)
        {
          uint total_nshrink = nshrink;
          extra = width - real_width;
          /* shrink shrinkable columns */
          while (total_nshrink > 0 && extra > 0)
            {
              nshrink = total_nshrink;
              for (uint jcol = 0; jcol < cols.size(); jcol++)
                {
                  uint col = cols.size() / 2 + ((1 + jcol) >> 1) * (1 & jcol ? -1 : +1); // start distributing in the center
                  if (cols[col].shrink)
                    {
                      int allocation = cols[col].allocation;
                      cols[col].allocation = MAX (1, allocation - extra / nshrink);
                      extra -= allocation - cols[col].allocation;
                      nshrink -= 1;
                      if (cols[col].allocation < 2)
                        {
                          total_nshrink -= 1;
                          cols[col].shrink = false;
                        }
                    }
                }
            }
          /* shrink all columns (last resort) */
          if (extra && cols.size())
            {
              double allocation_max = 1.0 * width;
              vector<RowCol*> svec;
              for (uint jcol = 0; jcol < cols.size(); jcol++)
                {
                  uint col = cols.size() / 2 + ((1 + jcol) >> 1) * (1 & jcol ? -1 : +1); // start distribution in the center
                  svec.push_back (&cols[col]);
                }
              reverse (svec.begin(), svec.end());                                       // swap edge vs. center
              stable_sort (svec.begin(), svec.end(), RowCol::lesser_allocation);        // smaller columns first
              reverse (svec.begin(), svec.end());                                       // bigger columns first, from center to egde
              while (extra && svec[0]->allocation)
                {
                  int base_extra = extra;
                  for (uint i = 0; i < svec.size() && extra; i++)
                    if (svec[i]->allocation)
                      {
                        int allocation = svec[i]->allocation;
                        int shrink = ifloor (base_extra * allocation / allocation_max); // shrink in proportion to size
                        svec[i]->allocation -= CLAMP (shrink, 1, allocation);
                        extra -= allocation - svec[i]->allocation;
                      }
                }
            }
        }
    }
  if (homogeneous())
    {
      int nexpand, extra;
      if (!has_children())
        nexpand = 1;
      else
        {
          nexpand = 0;
          for (uint row = 0; row < rows.size(); row++)
            if (rows[row].expand)
              {
                nexpand += 1;
                break;
              }
        }
      if (nexpand)
        {
          int height = real_height;
          for (uint row = 0; row + 1 < rows.size(); row++)
            height -= rows[row].spacing;
          for (uint row = 0; row < rows.size(); row++)
            {
              extra = height / (rows.size() - row);
              rows[row].allocation = MAX (1, extra);
              height -= extra;
            }
        }
    }
  else
    {
      int height = 0, nexpand = 0, nshrink = 0, extra;
      for (uint row = 0; row < rows.size(); row++)
        {
          height += rows[row].requisition;
          nexpand += rows[row].expand;
          nshrink += rows[row].shrink;
        }
      for (uint row = 0; row + 1 < rows.size(); row++)
        height += rows[row].spacing;
      /* distribute extra space that we were allocated beyond requisition */
      if (height < real_height && nexpand >= 1)
        {
          height = real_height - height;
          for (uint jrow = 0; jrow < rows.size(); jrow++)
            {
              uint row = rows.size() / 2 + ((1 + jrow) >> 1) * (1 & jrow ? -1 : +1); // start distributing in the center
              if (rows[row].expand)
                {
                  extra = height / nexpand;
                  rows[row].allocation += extra;
                  height -= extra;
                  nexpand -= 1;
                }
            }
        }
      /* shrink until we fit the size given */
      if (height > real_height)
        {
          uint total_nshrink = nshrink;
          extra = height - real_height;
          /* shrink shrinkable rows */
          while (total_nshrink > 0 && extra > 0)
            {
              nshrink = total_nshrink;
              for (uint jrow = 0; jrow < rows.size(); jrow++)
                {
                  uint row = rows.size() / 2 + ((1 + jrow) >> 1) * (1 & jrow ? -1 : +1); // start distributing in the center
                  if (rows[row].shrink)
                    {
                      int allocation = rows[row].allocation;
                      rows[row].allocation = MAX (1, allocation - extra / nshrink);
                      extra -= allocation - rows[row].allocation;
                      nshrink -= 1;
                      if (rows[row].allocation < 2)
                        {
                          total_nshrink -= 1;
                          rows[row].shrink = false;
                        }
                    }
                }
            }
          /* shrink all rows (last resort) */
          if (extra && rows.size())
            {
              double allocation_max = 1.0 * height;
              vector<RowCol*> svec;
              for (uint jrow = 0; jrow < rows.size(); jrow++)
                {
                  uint row = rows.size() / 2 + ((1 + jrow) >> 1) * (1 & jrow ? -1 : +1); // start distribution in the center
                  svec.push_back (&rows[row]);
                }
              reverse (svec.begin(), svec.end());                                       // swap edge vs. center
              stable_sort (svec.begin(), svec.end(), RowCol::lesser_allocation);        // smaller rows first
              reverse (svec.begin(), svec.end());                                       // bigger rows first, from center to egde
              while (extra && svec[0]->allocation)
                {
                  int base_extra = extra;
                  for (uint i = 0; i < svec.size() && extra; i++)
                    if (svec[i]->allocation)
                      {
                        int allocation = svec[i]->allocation;
                        int shrink = ifloor (base_extra * allocation / allocation_max); // shrink in proportion to size
                        svec[i]->allocation -= CLAMP (shrink, 1, allocation);
                        extra -= allocation - svec[i]->allocation;
                      }
                }
            }
        }
    }
}

void
TableImpl::size_allocate_pass2 ()
{
  Allocation area = allocation(), child_area;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      if (!cw->allocatable())
        continue;
      const PackInfo &pi = cw->pack_info();
      Requisition crq = cw->size_request();
      int x = ifloor (area.x);
      for (uint col = 0; col < left_attach (pi); col++)
        x += cols[col].allocation + cols[col].spacing;
      int max_width = 0;
      for (uint col = left_attach (pi); col < right_attach (pi); col++)
        {
          max_width += cols[col].allocation;
          if (col + 1 < right_attach (pi))
            max_width += cols[col].spacing;
        }
      int y = ifloor (area.y);
      for (uint row = 0; row < bottom_attach (pi); row++)
        y += rows[row].allocation + rows[row].spacing;
      int max_height = 0;
      for (uint row = bottom_attach (pi); row < top_attach (pi); row++)
        {
          max_height += rows[row].allocation;
          if (row + 1 < top_attach (pi))
            max_height += rows[row].spacing;
        }
      /* max possible child size */
      child_area.width = max_width;
      child_area.x = x;
      child_area.height = max_height;
      child_area.y = y;
      /* flip layout horizontally */
      if (false)
        child_area.x = area.x + area.width - (child_area.x - area.x) - child_area.width;
      /* constrain child allocation to table */
      if (child_area.x + child_area.width > area.x + area.width)
        child_area.width -= child_area.x + child_area.width - area.x - area.width;
      if (child_area.y + child_area.height > area.y + area.height)
        child_area.height -= child_area.y + child_area.height - area.y - area.height;
      /* subtract child spacings */
      child_area.width -= MIN (child_area.width, pi.left_spacing + pi.right_spacing);
      child_area.x += MIN (child_area.width, pi.left_spacing);
      child_area.height -= MIN (child_area.height, pi.bottom_spacing + pi.top_spacing);
      child_area.y += MIN (child_area.height, pi.bottom_spacing);
      /* align and scale */
      if (child_area.width > crq.width)
        {
          int width = iround (crq.width + pi.hscale * (child_area.width - crq.width));
          child_area.x += iround (pi.halign * (child_area.width - width));
          child_area.width = width;
        }
      if (child_area.height > crq.height)
        {
          int height = iround (crq.height + pi.vscale * (child_area.height - crq.height));
          child_area.y += iround (pi.valign * (child_area.height - height));
          child_area.height = height;
        }
      /* allocate child */
      cw->set_allocation (child_area);
    }
}

static const ItemFactory<TableImpl> table_factory ("Rapicorn::Factory::Table");

} // Rapicorn
