/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * - This file is derived from GtkTable code which is
 *   Copyright (C) 1996-2002 by the GTK+ project.
 */
#include "tableimpl.hh"
#include "factory.hh"

namespace Rapicorn {

DataKey<TableImpl::Location> TableImpl::child_location_key;

TableImpl::Location
TableImpl::child_location (Item &child)
{
  Location loc = child.get_data (&child_location_key);
  loc.xexpand |= child.hexpand();
  loc.yexpand |= child.vexpand();
  return loc;
}

void
TableImpl::child_location (Item     &child,
                           Location  loc)
{
  loc.right_attach = MAX (loc.right_attach, loc.left_attach + 1);
  loc.top_attach = MAX (loc.top_attach, loc.bottom_attach + 1);
  child.set_data (&child_location_key, loc);
}

TableImpl::TablePacker::~TablePacker()
{}

TableImpl::TablePacker::TablePacker (Item &citem) :
  item (citem)
{}

void
TableImpl::TablePacker::update () /* fetch real pack properties */
{
  if (dynamic_cast<Table*> (item.parent()))
    loc = child_location (item);
}

void
TableImpl::TablePacker::commit ()/* assign pack properties */
{
  TableImpl *table = dynamic_cast<TableImpl*> (item.parent());
  if (table)
    {
      child_location (item, loc);
      table->resize_grow (loc.right_attach, loc.top_attach);
    }
}

const PropertyList&
TableImpl::TablePacker::list_properties()
{
  static Property *properties[] = {
    MakeProperty (TablePacker, left_attach,    _("Left Attach"),    _("Column index to attach the child's left side to"), 0u, 0u, 99999u, 5u, "rw"),
    MakeProperty (TablePacker, right_attach,   _("Right Attach"),   _("Column index to attach the child's right side to"), 1u, 1u, 100000u, 5u, "rw"),
    MakeProperty (TablePacker, bottom_attach,  _("Bottom Attach"),  _("Column index to attach the child's bottom side to"), 0u, 0u, 99999u, 5u, "rw"),
    MakeProperty (TablePacker, top_attach,     _("Top Attach"),     _("Column index to attach the child's top side to"), 1u, 1u, 100000u, 5u, "rw"),
    MakeProperty (TablePacker, left_padding,   _("Left Padding"),   _("Amount of padding to add at the child's left side"), 0u, 0u, 65535u, 3u, "rw"),
    MakeProperty (TablePacker, right_padding,  _("Right Padding"),  _("Amount of padding to add at the child's right side"), 0u, 0u, 65535u, 3u, "rw"),
    MakeProperty (TablePacker, bottom_padding, _("Bottom Padding"), _("Amount of padding to add at the child's bottom side"), 0u, 0u, 65535u, 3u, "rw"),
    MakeProperty (TablePacker, top_padding,    _("Top Padding"),    _("Amount of padding to add at the child's top side"), 0u, 0u, 65535u, 3u, "rw"),
    MakeProperty (TablePacker, hshrink,        _("Horizontal Shrink"), _("Whether the child may be shrunken horizontally"), false, "rw"),
    MakeProperty (TablePacker, vshrink,        _("Vertical Shrink"),   _("Whether the child may be shrunken vertically"), false, "rw"),
    MakeProperty (TablePacker, hfill,          _("Horizontal Fill"), _("Whether the child may fill all extra horizontal space"), true, "rw"),
    MakeProperty (TablePacker, vfill,          _("Vertical Fill"), _("Whether the child may fill all extra vertical space"), true, "rw"),
  };
  static const PropertyList property_list (properties);
  return property_list;
}

Container::Packer
TableImpl::create_packer (Item &item)
{
  if (item.parent() == this)
    return Packer (new TablePacker (item));
  else
    throw Exception ("foreign child: ", item.name());
}

TableImpl::TableImpl() :
  default_row_spacing (0),
  default_column_spacing (0),
  homogeneous_items (false)
{
  resize (1, 1);
}

bool
TableImpl::is_row_used (uint row)
{
  if (row < rows.size())
    for (ChildWalker cw = local_children(); cw.has_next(); cw++)
      {
        Location loc = child_location (*cw);
        if (row >= loc.bottom_attach && row < loc.top_attach)
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
        Location loc = child_location (*cw);
        if (col >= loc.left_attach && col < loc.right_attach)
          return true;
      }
  return false;
}

void
TableImpl::resize_grow (uint n_cols, uint n_rows)
{
  resize (MAX (n_cols, cols.size()), MAX (n_rows, rows.size()));
}

void
TableImpl::resize (uint n_cols, uint n_rows)
{
  n_rows = MAX (n_rows, 1);
  n_cols = MAX (n_cols, 1);
  if (n_rows == rows.size() && n_cols == cols.size())
    return;
  /* grow as children require */
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Location loc = child_location (*cw);
      n_rows = MAX (n_rows, loc.top_attach);
      n_cols = MAX (n_cols, loc.right_attach);
    }
  if (n_rows == rows.size() && n_cols == cols.size())
    return;
  /* resize rows and cols */
  if (n_rows != rows.size())
    {
      uint i = rows.size();
      rows.resize (n_rows);
      for (; i < rows.size(); i++)
        rows[i].spacing = default_row_spacing;
    }
  if (n_cols != cols.size())
    {
      uint i = cols.size();
      cols.resize (n_cols);
      for (; i < cols.size(); i++)
        cols[i].spacing = default_column_spacing;
    }
  invalidate();
}

void
TableImpl::insert_rows (uint first_row, uint n_rows)
{
  if (!n_rows)
    return;
  resize (cols.size(), rows.size() + n_rows);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Location loc = child_location (*cw);
      if (loc.bottom_attach >= first_row)
        {
          loc.bottom_attach += n_rows;
          loc.top_attach += n_rows;
          child_location (*cw, loc);
        }
      else if (first_row < loc.top_attach)
        {
          loc.top_attach += n_rows;
          child_location (*cw, loc);
        }
    }
}

void
TableImpl::insert_cols (uint first_col, uint n_cols)
{
  if (!n_cols)
    return;
  resize (cols.size() + n_cols, rows.size());
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Location loc = child_location (*cw);
      if (loc.left_attach >= first_col)
        {
          loc.left_attach += n_cols;
          loc.right_attach += n_cols;
          child_location (*cw, loc);
        }
      else if (first_col < loc.right_attach)
        {
          loc.right_attach += n_cols;
          child_location (*cw, loc);
        }
    }
}

void
TableImpl::column_spacing (uint cspacing)
{
  default_column_spacing = cspacing;
  for (uint col = 0; col < cols.size(); col++)
    cols[col].spacing = default_column_spacing;
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
  uint c = 0;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    if (cw->visible())
      c++;
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
TableImpl::list_properties()
{
  static Property *properties[] = {
    MakeProperty (Table, homogeneous,    _("Homogeneous"), _("Whether all children get the same size"), false, "rw"),
    MakeProperty (Table, column_spacing, _("Column Spacing"), _("The amount of space between two consecutive columns"), 0, 0, 65535, 10, "rw"),
    MakeProperty (Table, row_spacing,    _("Row Spacing"), _("The amount of space between two consecutive rows"), 0, 0, 65535, 10, "rw"),
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
      Requisition rq = cw->size_request();
      if (!cw->visible())
        continue;
      chspread |= cw->hspread();
      cvspread |= cw->vspread();
      Location loc = child_location (*cw);
      /* expand cols with single-column expand children */
      if (loc.left_attach + 1 == loc.right_attach && loc.xexpand)
        cols[loc.left_attach].expand = true;
      /* expand rows with single-column expand children */
      if (loc.bottom_attach + 1 == loc.top_attach && loc.yexpand)
        rows[loc.bottom_attach].expand = true;
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
TableImpl::size_request_pass1()
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Requisition crq = cw->size_request();
      if (!cw->visible())
        continue;
      Location loc = child_location (*cw);
      /* fetch requisition from single-column children */
      if (loc.left_attach + 1 == loc.right_attach)
        {
          uint width = iround (crq.width + loc.left_padding + loc.right_padding);
          cols[loc.left_attach].requisition = MAX (cols[loc.left_attach].requisition, width);
        }
      /* fetch requisition from single-row children */
      if (loc.bottom_attach + 1 == loc.top_attach)
        {
          uint height = iround (crq.height + loc.bottom_padding + loc.top_padding);
          rows[loc.bottom_attach].requisition = MAX (rows[loc.bottom_attach].requisition, height);
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
      Location loc = child_location (*cw);
      if (!cw->visible())
        continue;
      /* request remaining space for multi-column children */
      if (loc.left_attach + 1 != loc.right_attach)
        {
          Requisition crq = cw->size_request();
          /* Check and see if there is already enough space for the child. */
          uint width = 0;
          for (uint col = loc.left_attach; col < loc.right_attach; col++)
            {
              width += cols[col].requisition;
              if (col + 1 < loc.right_attach)
                width += cols[col].spacing;
            }
          /* If we need to request more space for this child to fill
           *  its requisition, then divide up the needed space amongst the
           *  columns it spans, favoring expandable columns if any.
           */
          if (width < crq.width + loc.left_padding + loc.right_padding)
            {
              bool force_expand = false;
              uint n_expand = 0;
              width = iround (crq.width + loc.left_padding + loc.right_padding - width);
              for (uint col = loc.left_attach; col < loc.right_attach; col++)
                if (cols[col].expand)
                  n_expand++;
              if (n_expand == 0)
                {
                  n_expand = loc.right_attach - loc.left_attach;
                  force_expand = true;
                }
              for (uint col = loc.left_attach; col < loc.right_attach; col++)
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
      if (loc.bottom_attach + 1 != loc.top_attach)
        {
          Requisition crq = cw->size_request();
          /* Check and see if there is already enough space for the child. */
          uint height = 0;
          for (uint row = loc.bottom_attach; row < loc.top_attach; row++)
            {
              height += rows[row].requisition;
              if (row + 1 < loc.top_attach)
                height += rows[row].spacing;
            }
          /* If we need to request more space for this child to fill
           *  its requisition, then divide up the needed space amongst the
           *  rows it spans, favoring expandable rows if any.
           */
          if (height < crq.height + loc.bottom_padding + loc.top_padding)
            {
              bool force_expand = false;
              uint n_expand = 0;
              height = iround (crq.height + loc.bottom_padding + loc.top_padding - height);
              for (uint row = loc.bottom_attach; row < loc.top_attach; row++)
                if (rows[row].expand)
                  n_expand++;
              if (n_expand == 0)
                {
                  n_expand = loc.top_attach - loc.bottom_attach;
                  force_expand = true;
                }
              for (uint row = loc.bottom_attach; row < loc.top_attach; row++)
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
      if (!cw->visible())
        continue;
      Location loc = child_location (*cw);
      if (loc.left_attach + 1 == loc.right_attach)
        {
          cols[loc.left_attach].expand |= loc.xexpand;
          cols[loc.left_attach].shrink &= loc.xshrink;
          cols[loc.left_attach].empty = false;
        }
      if (loc.bottom_attach + 1 == loc.top_attach)
        {
          rows[loc.bottom_attach].expand |= loc.yexpand;
          rows[loc.bottom_attach].shrink &= loc.yshrink;
          rows[loc.bottom_attach].empty = false;
        }
    }
  /* adjust the row and col flags from expand/shrink flags of multi row/col children */
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      if (!cw->visible())
        continue;
      Location loc = child_location (*cw);
      if (loc.left_attach + 1 != loc.right_attach)
        {
          uint col;
          for (col = loc.left_attach; col < loc.right_attach; col++)
            cols[col].empty = false;
          if (loc.xexpand)
            {
              for (col = loc.left_attach; col < loc.right_attach; col++)
                if (cols[col].expand)
                  break;
              if (col >= loc.right_attach) /* no expand col found */
                for (col = loc.left_attach; col < loc.right_attach; col++)
                  cols[col].need_expand = true;
            }
          if (!loc.xshrink)
            {
              for (col = loc.left_attach; col < loc.right_attach; col++)
                if (!cols[col].shrink)
                  break;
              if (col >= loc.right_attach) /* no shrink col found */
                for (col = loc.left_attach; col < loc.right_attach; col++)
                  cols[col].need_shrink = false;
            }
        }
      if (loc.bottom_attach + 1 != loc.top_attach)
        {
          uint row;
          for (row = loc.bottom_attach; row < loc.top_attach; row++)
            rows[row].empty = false;
          if (loc.yexpand)
            {
              for (row = loc.bottom_attach; row < loc.top_attach; row++)
                if (rows[row].expand)
                  break;
              if (row >= loc.top_attach) /* no expand row found */
                for (row = loc.bottom_attach; row < loc.top_attach; row++)
                  rows[row].need_expand = true;
            }
          if (!loc.yshrink)
            {
              for (row = loc.bottom_attach; row < loc.top_attach; row++)
                if (!rows[row].shrink)
                  break;
              if (row >= loc.top_attach) /* no shrink row found */
                for (row = loc.bottom_attach; row < loc.top_attach; row++)
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
              float allocation_max = 1.0 * width;
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
                        int shrink = base_extra * allocation / allocation_max;           // shrink in proportion to size
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
              float allocation_max = 1.0 * height;
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
                        int shrink = base_extra * allocation / allocation_max;           // shrink in proportion to size
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
      if (!cw->visible())
        continue;
      Location loc = child_location (*cw);
      Requisition crq = cw->size_request();
      int x = area.x;
      for (uint col = 0; col < loc.left_attach; col++)
        x += cols[col].allocation + cols[col].spacing;
      int max_width = 0;
      for (uint col = loc.left_attach; col < loc.right_attach; col++)
        {
          max_width += cols[col].allocation;
          if (col + 1 < loc.right_attach)
            max_width += cols[col].spacing;
        }
      int y = area.y;
      for (uint row = 0; row < loc.bottom_attach; row++)
        y += rows[row].allocation + rows[row].spacing;
      int max_height = 0;
      for (uint row = loc.bottom_attach; row < loc.top_attach; row++)
        {
          max_height += rows[row].allocation;
          if (row + 1 < loc.top_attach)
            max_height += rows[row].spacing;
        }
      if (loc.xfill)
        {
          child_area.width = max_width; // MAX (0, max_width - int (loc.left_padding + loc.right_padding));
          child_area.x = x; //  + loc.left_padding; // (max_width - child_area.width) / 2;
        }
      else
        {
          child_area.width = MIN (iround (crq.width), max_width);
          child_area.x = x + (max_width - child_area.width) / 2;
        }
      if (loc.yfill)
        {
          child_area.height = max_height; // MAX (0, max_height - int (loc.bottom_padding + loc.top_padding));
          child_area.y = y; //  + loc.bottom_padding; // (max_height - child_area.height) / 2;
        }
      else
        {
          child_area.height = MIN (iround (crq.height), max_height);
          child_area.y = y + (max_height - child_area.height) / 2;
        }
      if (false) /* flip layout horizontally */
        child_area.x = area.x + area.width - (child_area.x - area.x) - child_area.width;
      /* constrain child allocation to table */
      if (child_area.x + child_area.width > area.x + area.width)
        child_area.width -= child_area.x + child_area.width - area.x - area.width;
      if (child_area.y + child_area.height > area.y + area.height)
        child_area.height -= child_area.y + child_area.height - area.y - area.height;
      /* account for padding */
      if (loc.xfill)
        {
          child_area.width -= int (loc.left_padding + loc.right_padding);
          child_area.x += loc.left_padding;
        }
      if (loc.yfill)
        {
          child_area.height -= int (loc.bottom_padding + loc.top_padding);
          child_area.y += loc.bottom_padding;
        }
      /* allocate child */
      cw->set_allocation (child_area);
    }
}

static const ItemFactory<TableImpl> table_factory ("Rapicorn::Table");

} // Rapicorn
