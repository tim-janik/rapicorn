// Licensed GNU LGPLv2+: http://www.gnu.org/licenses/lgpl.html
// This file is derived from GtkTable code
// Copyright (C) 1996-2002, GNU LGPLv2+ by the GTK+ project
// Copyright (C) 2005-2014, MPL-2.0 Tim Janik
#include "table.hh"
#include "factory.hh"
#include <algorithm>

namespace Rapicorn {

// == TableImpl ==
void
TableImpl::homogeneous (bool h)
{
  if (h != homogeneous())
    {
      TableLayoutImpl::homogeneous (h);
      changed ("homogeneous");
    }
}

void
TableImpl::col_spacing (int spacing)
{
  if (spacing != col_spacing())
    {
      TableLayoutImpl::col_spacing (spacing);
      changed ("col_spacing");
    }
}

void
TableImpl::row_spacing (int spacing)
{
  if (spacing != row_spacing())
    {
      TableLayoutImpl::row_spacing (spacing);
      changed ("row_spacing");
    }
}

void
TableImpl::resize (int ncols, int nrows)
{
  const int old_cols = n_cols(), old_rows = n_rows();
  resize_table (ncols, nrows);
  if (old_cols != n_cols())
    changed ("n_cols");
  if (old_rows != n_rows())
    changed ("n_rows");
}

void
TableImpl::insert_cols (int first_col, int ncols)
{
  const int old_cols = n_cols();
  TableLayoutImpl::expand_table (first_col, ncols, UINT_MAX, 0);
  if (old_cols != n_cols())
    changed ("n_cols");
}

void
TableImpl::insert_rows (int first_row, int nrows)
{
  const int old_rows = n_rows();
  TableLayoutImpl::expand_table (UINT_MAX, 0, first_row, nrows);
  if (old_rows != n_rows())
    changed ("n_rows");
}

// == TableLayoutImpl ==
static uint
left_attach (const PackInfo &pi)
{
  return iround (MAX (0, pi.hposition));
}

static uint
right_attach (const PackInfo &pi)
{
  double r = pi.hposition + pi.hspan;
  double l = left_attach (pi);
  return iround (MAX (l + 1, r));
}

static uint
bottom_attach (const PackInfo &pi)
{
  return iround (MAX (0, pi.vposition));
}

static uint
top_attach (const PackInfo &pi)
{
  double t = pi.vposition + pi.vspan;
  double b = bottom_attach (pi);
  return iround (MAX (b + 1, t));
}

TableLayoutImpl::TableLayoutImpl() :
  default_col_spacing_ (0), default_row_spacing_ (0), homogeneous_widgets_ (false)
{
  resize_table (1, 1);
}

bool
TableLayoutImpl::is_row_used (int srow)
{
  const uint row = srow;
  if (row < rows_.size())
    for (auto child : *this)
      {
        const PackInfo &pi = child->pack_info();
        if (row >= bottom_attach (pi) && row < top_attach (pi))
          return true;
      }
  return false;
}

bool
TableLayoutImpl::is_col_used (int scol)
{
  const uint col = scol;
  if (col < cols_.size())
    for (auto child : *this)
      {
        const PackInfo &pi = child->pack_info();
        if (col >= left_attach (pi) && col < right_attach (pi))
          return true;
      }
  return false;
}

void
TableLayoutImpl::resize_table (uint n_cols, uint n_rows)
{
  n_rows = MAX (n_rows, 1);
  n_cols = MAX (n_cols, 1);
  if (n_rows == rows_.size() && n_cols == cols_.size())
    return;
  // grow as children require
  if (n_rows < rows_.size() || n_cols < cols_.size())
    for (auto child : *this)
      {
        const PackInfo &pi = child->pack_info();
        n_rows = MAX (n_rows, top_attach (pi));
        n_cols = MAX (n_cols, right_attach (pi));
      }
  // resize rows and cols
  bool need_row_invalidate = false, need_col_invalidate = false;
  if (n_rows != rows_.size())
    {
      uint i = rows_.size();
      rows_.resize (n_rows);
      for (; i < rows_.size(); i++)
        rows_[i].spacing = default_row_spacing_;
      need_row_invalidate = true;
    }
  if (n_cols != cols_.size())
    {
      uint i = cols_.size();
      cols_.resize (n_cols);
      for (; i < cols_.size(); i++)
        cols_[i].spacing = default_col_spacing_;
      need_col_invalidate = true;
    }
  if (need_row_invalidate || need_col_invalidate)
    invalidate_requisition();
}

void
TableLayoutImpl::expand_table (const uint first_col, const uint n_cols, const uint first_row, const uint n_rows)
{
  resize_table (cols_.size() + n_cols, rows_.size() + n_rows);
  for (auto child : *this)
    {
      const PackInfo &pi = child->pack_info();
      if (left_attach (pi) >= first_col)
        child->hposition (child->hposition() + n_cols);
      else if (first_col < right_attach (pi))
        child->hspan (child->hspan() + n_cols);
      if (bottom_attach (pi) >= first_row)
        child->vposition (child->vposition() + n_rows);
      else if (first_row < top_attach (pi))
        child->vspan (child->vspan() + n_rows);
    }
}

void
TableLayoutImpl::homogeneous (bool h)
{
  if (homogeneous_widgets_ != h)
    {
      homogeneous_widgets_ = h;
      invalidate_size();
    }
}

void
TableLayoutImpl::col_spacing (uint16 cspacing)
{
  default_col_spacing_ = cspacing;
  for (uint col = 0; col < cols_.size(); col++)
    cols_[col].spacing = default_col_spacing_;
  invalidate_requisition();
}

void
TableLayoutImpl::row_spacing (uint16 rspacing)
{
  default_row_spacing_ = rspacing;
  for (uint row = 0; row < rows_.size(); row++)
    rows_[row].spacing = default_row_spacing_;
  invalidate_requisition();
}

TableLayoutImpl::~TableLayoutImpl()
{}

void
TableLayoutImpl::repack_child (WidgetImpl &widget, const PackInfo &orig, const PackInfo &pnew)
{
  uint n_cols = right_attach (pnew), n_rows = top_attach (pnew);
  if (n_cols > cols_.size() || n_rows > rows_.size())
    resize_table (n_cols, n_rows);
  MultiContainerImpl::repack_child (widget, orig, pnew);
}

void
TableLayoutImpl::size_request (Requisition &requisition)
{
  size_request_init ();
  size_request_pass1 ();
  size_request_pass2 ();
  size_request_pass3 ();
  size_request_pass2 ();

  for (uint col = 0; col < cols_.size(); col++)
    requisition.width += cols_[col].requisition;
  for (uint col = 0; col + 1 < cols_.size(); col++)
    requisition.width += cols_[col].spacing;
  for (uint row = 0; row < rows_.size(); row++)
    requisition.height += rows_[row].requisition;
  for (uint row = 0; row + 1 < rows_.size(); row++)
    requisition.height += rows_[row].spacing;
}

void
TableLayoutImpl::size_allocate (Allocation area, bool changed)
{
  size_allocate_init ();
  size_allocate_pass1 ();
  size_allocate_pass2 ();
}

void
TableLayoutImpl::size_request_init()
{
  for (uint row = 0; row < rows_.size(); row++)
    {
      rows_[row].requisition = 0;
      rows_[row].expand = false;
    }
  for (uint col = 0; col < cols_.size(); col++)
    {
      cols_[col].requisition = 0;
      cols_[col].expand = false;
    }

  bool chspread = false, cvspread = false;
  for (auto child : *this)
    {
      // size request all children
      if (!child->visible())
        continue;
      const PackInfo &pi = child->pack_info();
      chspread |= child->hspread();
      cvspread |= child->vspread();
      // expand cols with single-column expand children
      if (left_attach (pi) + 1 == right_attach (pi) && child->hexpand())
        cols_[left_attach (pi)].expand = true;
      // expand rows with single-column expand children
      if (bottom_attach (pi) + 1 == top_attach (pi) && child->vexpand())
        rows_[bottom_attach (pi)].expand = true;
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
TableLayoutImpl::size_request_pass1()
{
  for (auto child : *this)
    {
      if (!child->visible())
        continue;
      Requisition crq = child->requisition();
      const PackInfo &pi = child->pack_info();
      /* fetch requisition from single-column children */
      if (left_attach (pi) + 1 == right_attach (pi))
        {
          uint width = crq.width + pi.left_spacing + pi.right_spacing;
          cols_[left_attach (pi)].requisition = MAX (cols_[left_attach (pi)].requisition, width);
        }
      /* fetch requisition from single-row children */
      if (bottom_attach (pi) + 1 == top_attach (pi))
        {
          uint height = crq.height + pi.bottom_spacing + pi.top_spacing;
          rows_[bottom_attach (pi)].requisition = MAX (rows_[bottom_attach (pi)].requisition, height);
        }
    }
}

void
TableLayoutImpl::size_request_pass2()
{
  if (homogeneous())
    {
      uint max_width = 0;
      uint max_height = 0;
      /* maximise requisition */
      for (uint col = 0; col < cols_.size(); col++)
        max_width = MAX (max_width, cols_[col].requisition);
      for (uint row = 0; row < rows_.size(); row++)
        max_height = MAX (max_height, rows_[row].requisition);
      /* assign equal requisitions */
      for (uint col = 0; col < cols_.size(); col++)
        cols_[col].requisition = max_width;
      for (uint row = 0; row < rows_.size(); row++)
        rows_[row].requisition = max_height;
    }
}

void
TableLayoutImpl::size_request_pass3()
{
  for (auto child : *this)
    {
      if (!child->visible())
        continue;
      const PackInfo &pi = child->pack_info();
      /* request remaining space for multi-column children */
      if (left_attach (pi) + 1 != right_attach (pi))
        {
          Requisition crq = child->requisition();
          /* Check and see if there is already enough space for the child. */
          int width = 0;
          for (uint col = left_attach (pi); col < right_attach (pi); col++)
            {
              width += cols_[col].requisition;
              if (col + 1 < right_attach (pi))
                width += cols_[col].spacing;
            }
          /* If we need to request more space for this child to fill
           *  its requisition, then divide up the needed space amongst the
           *  columns it spans, favoring expandable columns if any.
           */
          if (width < crq.width + pi.left_spacing + pi.right_spacing)
            {
              bool force_expand = false;
              uint n_expand = 0;
              width = crq.width + pi.left_spacing + pi.right_spacing - width;
              for (uint col = left_attach (pi); col < right_attach (pi); col++)
                if (cols_[col].expand)
                  n_expand++;
              if (n_expand == 0)
                {
                  n_expand = right_attach (pi) - left_attach (pi);
                  force_expand = true;
                }
              for (uint col = left_attach (pi); col < right_attach (pi); col++)
                if (force_expand || cols_[col].expand)
                  {
                    uint extra = width / n_expand;
                    cols_[col].requisition += extra;
                    width -= extra;
                    n_expand--;
                  }
            }
        }
      /* request remaining space for multi-row children */
      if (bottom_attach (pi) + 1 != top_attach (pi))
        {
          Requisition crq = child->requisition();
          /* Check and see if there is already enough space for the child. */
          int height = 0;
          for (uint row = bottom_attach (pi); row < top_attach (pi); row++)
            {
              height += rows_[row].requisition;
              if (row + 1 < top_attach (pi))
                height += rows_[row].spacing;
            }
          /* If we need to request more space for this child to fill
           *  its requisition, then divide up the needed space amongst the
           *  rows it spans, favoring expandable rows if any.
           */
          if (height < crq.height + pi.bottom_spacing + pi.top_spacing)
            {
              bool force_expand = false;
              uint n_expand = 0;
              height = crq.height + pi.bottom_spacing + pi.top_spacing - height;
              for (uint row = bottom_attach (pi); row < top_attach (pi); row++)
                if (rows_[row].expand)
                  n_expand++;
              if (n_expand == 0)
                {
                  n_expand = top_attach (pi) - bottom_attach (pi);
                  force_expand = true;
                }
              for (uint row = bottom_attach (pi); row < top_attach (pi); row++)
                if (force_expand || rows_[row].expand)
                  {
                    uint extra = height / n_expand;
                    rows_[row].requisition += extra;
                    height -= extra;
                    n_expand--;
                  }
            }
        }
    }
}

void
TableLayoutImpl::size_allocate_init()
{
  /* Initialize the rows and cols.
   *  By default, rows and cols do not expand and do shrink.
   *  Those values are modified by the children that occupy
   *  the rows and cols.
   */
  for (uint col = 0; col < cols_.size(); col++)
    {
      cols_[col].allocation = cols_[col].requisition;
      cols_[col].need_expand = false;
      cols_[col].need_shrink = true;
      cols_[col].expand = false;
      cols_[col].shrink = true;
      cols_[col].empty = true;
    }
  for (uint row = 0; row < rows_.size(); row++)
    {
      rows_[row].allocation = rows_[row].requisition;
      rows_[row].need_expand = false;
      rows_[row].need_shrink = true;
      rows_[row].expand = false;
      rows_[row].shrink = true;
      rows_[row].empty = true;
    }
  /* adjust the row and col flags from expand/shrink flags of single row/col children */
  for (auto child : *this)
    {
      if (!child->visible())
        continue;
      const PackInfo &pi = child->pack_info();
      if (left_attach (pi) + 1 == right_attach (pi))
        {
          cols_[left_attach (pi)].expand |= child->hexpand();
          cols_[left_attach (pi)].shrink &= child->hshrink();
          cols_[left_attach (pi)].empty = false;
        }
      if (bottom_attach (pi) + 1 == top_attach (pi))
        {
          rows_[bottom_attach (pi)].expand |= child->vexpand();
          rows_[bottom_attach (pi)].shrink &= child->vshrink();
          rows_[bottom_attach (pi)].empty = false;
        }
    }
  /* adjust the row and col flags from expand/shrink flags of multi row/col children */
  for (auto child : *this)
    {
      if (!child->visible())
        continue;
      const PackInfo &pi = child->pack_info();
      if (left_attach (pi) + 1 != right_attach (pi))
        {
          uint col;
          for (col = left_attach (pi); col < right_attach (pi); col++)
            cols_[col].empty = false;
          if (child->hexpand())
            {
              for (col = left_attach (pi); col < right_attach (pi); col++)
                if (cols_[col].expand)
                  break;
              if (col >= right_attach (pi)) /* no expand col found */
                for (col = left_attach (pi); col < right_attach (pi); col++)
                  cols_[col].need_expand = true;
            }
          if (!child->hshrink())
            {
              for (col = left_attach (pi); col < right_attach (pi); col++)
                if (!cols_[col].shrink)
                  break;
              if (col >= right_attach (pi)) /* no shrink col found */
                for (col = left_attach (pi); col < right_attach (pi); col++)
                  cols_[col].need_shrink = false;
            }
        }
      if (bottom_attach (pi) + 1 != top_attach (pi))
        {
          uint row;
          for (row = bottom_attach (pi); row < top_attach (pi); row++)
            rows_[row].empty = false;
          if (child->vexpand())
            {
              for (row = bottom_attach (pi); row < top_attach (pi); row++)
                if (rows_[row].expand)
                  break;
              if (row >= top_attach (pi)) /* no expand row found */
                for (row = bottom_attach (pi); row < top_attach (pi); row++)
                  rows_[row].need_expand = true;
            }
          if (!child->vshrink())
            {
              for (row = bottom_attach (pi); row < top_attach (pi); row++)
                if (!rows_[row].shrink)
                  break;
              if (row >= top_attach (pi)) /* no shrink row found */
                for (row = bottom_attach (pi); row < top_attach (pi); row++)
                  rows_[row].need_shrink = false;
            }
        }
    }
  /* Loop over the columns and set the expand and shrink values
   *  if the column can be expanded or shrunk.
   */
  for (uint col = 0; col < cols_.size(); col++)
    if (cols_[col].empty)
      {
        cols_[col].expand = false;
        cols_[col].shrink = false;
      }
    else
      {
        cols_[col].expand |= cols_[col].need_expand;
        cols_[col].shrink &= cols_[col].need_shrink;
      }
  /* Loop over the rows and set the expand and shrink values
   *  if the row can be expanded or shrunk.
   */
  for (uint row = 0; row < rows_.size(); row++)
    if (rows_[row].empty)
      {
        rows_[row].expand = false;
        rows_[row].shrink = false;
      }
    else
      {
        rows_[row].expand |= rows_[row].need_expand;
        rows_[row].shrink &= rows_[row].need_shrink;
      }
}

void
TableLayoutImpl::size_allocate_pass1 ()
{
  /* If we were allocated more space than we requested
   *  then we have to expand any expandable rows and columns
   *  to fill in the extra space.
   */
  Allocation area = allocation();
  const int real_width = area.width;
  const int real_height = area.height;
  if (homogeneous())
    {
      int nexpand, extra;
      if (!has_children())
        nexpand = 1;
      else
        {
          nexpand = 0;
          for (uint col = 0; col < cols_.size(); col++)
            if (cols_[col].expand)
              {
                nexpand += 1;
                break;
              }
        }
      if (nexpand)
        {
          int width = real_width;
          for (uint col = 0; col + 1 < cols_.size(); col++)
            width -= cols_[col].spacing;
          for (uint col = 0; col < cols_.size(); col++)
            {
              extra = width / (cols_.size() - col);
              cols_[col].allocation = MAX (1, extra);
              width -= extra;
            }
        }
    }
  else
    {
      int width = 0, nexpand = 0, nshrink = 0, extra;
      for (uint col = 0; col < cols_.size(); col++)
        {
          width += cols_[col].requisition;
          nexpand += cols_[col].expand;
          nshrink += cols_[col].shrink;
        }
      for (uint col = 0; col + 1 < cols_.size(); col++)
        width += cols_[col].spacing;
      /* distribute extra space that we were allocated beyond requisition */
      if (width < real_width && nexpand >= 1)
        {
          width = real_width - width;
          for (uint jcol = 0; jcol < cols_.size(); jcol++)
            {
              uint col = cols_.size() / 2 + ((1 + jcol) >> 1) * (1 & jcol ? -1 : +1); // start distributing in the center
              if (cols_[col].expand)
                {
                  extra = width / nexpand;
                  cols_[col].allocation += extra;
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
              for (uint jcol = 0; jcol < cols_.size(); jcol++)
                {
                  uint col = cols_.size() / 2 + ((1 + jcol) >> 1) * (1 & jcol ? -1 : +1); // start distributing in the center
                  if (cols_[col].shrink)
                    {
                      int allocation = cols_[col].allocation;
                      cols_[col].allocation = MAX (1, allocation - extra / nshrink);
                      extra -= allocation - cols_[col].allocation;
                      nshrink -= 1;
                      if (cols_[col].allocation < 2)
                        {
                          total_nshrink -= 1;
                          cols_[col].shrink = false;
                        }
                    }
                }
            }
          /* shrink all columns (last resort) */
          if (extra && cols_.size())
            {
              double allocation_max = 1.0 * width;
              vector<RowCol*> svec;
              for (uint jcol = 0; jcol < cols_.size(); jcol++)
                {
                  uint col = cols_.size() / 2 + ((1 + jcol) >> 1) * (1 & jcol ? -1 : +1); // start distribution in the center
                  svec.push_back (&cols_[col]);
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
          for (uint row = 0; row < rows_.size(); row++)
            if (rows_[row].expand)
              {
                nexpand += 1;
                break;
              }
        }
      if (nexpand)
        {
          int height = real_height;
          for (uint row = 0; row + 1 < rows_.size(); row++)
            height -= rows_[row].spacing;
          for (uint row = 0; row < rows_.size(); row++)
            {
              extra = height / (rows_.size() - row);
              rows_[row].allocation = MAX (1, extra);
              height -= extra;
            }
        }
    }
  else
    {
      int height = 0, nexpand = 0, nshrink = 0, extra;
      for (uint row = 0; row < rows_.size(); row++)
        {
          height += rows_[row].requisition;
          nexpand += rows_[row].expand;
          nshrink += rows_[row].shrink;
        }
      for (uint row = 0; row + 1 < rows_.size(); row++)
        height += rows_[row].spacing;
      /* distribute extra space that we were allocated beyond requisition */
      if (height < real_height && nexpand >= 1)
        {
          height = real_height - height;
          for (uint jrow = 0; jrow < rows_.size(); jrow++)
            {
              uint row = rows_.size() / 2 + ((1 + jrow) >> 1) * (1 & jrow ? -1 : +1); // start distributing in the center
              if (rows_[row].expand)
                {
                  extra = height / nexpand;
                  rows_[row].allocation += extra;
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
              for (uint jrow = 0; jrow < rows_.size(); jrow++)
                {
                  uint row = rows_.size() / 2 + ((1 + jrow) >> 1) * (1 & jrow ? -1 : +1); // start distributing in the center
                  if (rows_[row].shrink)
                    {
                      int allocation = rows_[row].allocation;
                      rows_[row].allocation = MAX (1, allocation - extra / nshrink);
                      extra -= allocation - rows_[row].allocation;
                      nshrink -= 1;
                      if (rows_[row].allocation < 2)
                        {
                          total_nshrink -= 1;
                          rows_[row].shrink = false;
                        }
                    }
                }
            }
          /* shrink all rows (last resort) */
          if (extra && rows_.size())
            {
              double allocation_max = 1.0 * height;
              vector<RowCol*> svec;
              for (uint jrow = 0; jrow < rows_.size(); jrow++)
                {
                  uint row = rows_.size() / 2 + ((1 + jrow) >> 1) * (1 & jrow ? -1 : +1); // start distribution in the center
                  svec.push_back (&rows_[row]);
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
TableLayoutImpl::size_allocate_pass2 ()
{
  Allocation area = allocation(), child_area;
  for (auto child : *this)
    {
      if (!child->visible())
        continue;
      const PackInfo &pi = child->pack_info();
      Requisition crq = child->requisition();
      int x = area.x;
      for (uint col = 0; col < left_attach (pi); col++)
        x += cols_[col].allocation + cols_[col].spacing;
      int max_width = 0;
      for (uint col = left_attach (pi); col < right_attach (pi); col++)
        {
          max_width += cols_[col].allocation;
          if (col + 1 < right_attach (pi))
            max_width += cols_[col].spacing;
        }
      int y = area.y;
      for (uint row = 0; row < bottom_attach (pi); row++)
        y += rows_[row].allocation + rows_[row].spacing;
      int max_height = 0;
      for (uint row = bottom_attach (pi); row < top_attach (pi); row++)
        {
          max_height += rows_[row].allocation;
          if (row + 1 < top_attach (pi))
            max_height += rows_[row].spacing;
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
      child_area.y += MIN (child_area.height, pi.top_spacing);
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
      child->set_allocation (child_area);
    }
}

static const WidgetFactory<TableImpl> table_factory ("Rapicorn::Table");

} // Rapicorn
