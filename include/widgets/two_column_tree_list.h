/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file two_column_tree_list.h
 */

#include <wx/treelist.h>
#include <map>

#ifndef __two_column_tree_list__
#define __two_column_tree_list__

/**
 * Modified wxTreeListCtrl designed for use with two columns, with better
 * column resizing. wxTreeListCtrl accumulates error as columns resize,
 * leading to columns with a totally wrong size.
 */
class TWO_COLUMN_TREE_LIST : public wxTreeListCtrl
{
    public:
        /**
         * Create a TWO_COLUMN_TREE_LIST.
         */
        TWO_COLUMN_TREE_LIST( wxWindow* aParent, wxWindowID aID,
                const wxPoint & aPos = wxDefaultPosition,
                const wxSize & aSize = wxDefaultSize,
                long aStyle = wxTL_DEFAULT_STYLE,
                const wxString & aName = wxTreeListCtrlNameStr );


        /**
         * Set the column number that will "rubber-band" (expand with available space).
         * As this is a TWO column tree list, this must be zero or one.
         */
        void SetRubberBandColumn( int aRubberBandColumn )
        {
            m_rubber_band_column = aRubberBandColumn;
        }

        /**
         * Set the minimum width of the non-rubber-band column.
         */
        void SetClampedMinWidth( int aClampedMinWidth )
        {
            m_clamped_min_width = aClampedMinWidth;
        }

        /**
         * Recompute column sizes. This should be called after adding columns.
         * There is no need to call this in an OnSize handler - this CALLS the
         * OnSize handler.
         */
        void AutosizeColumns();

        /**
         * Override buggy wxTreeListCtrl size handler.
         */
        void OnSize( wxSizeEvent& aEvent );

    protected:

        /**
         * Memoized version of wx WidthFor(), which returns the width in pixels
         * required to display a string. This function is REALLY SLOW on macOS.
         */
        int MemoWidthFor( const wxString& aStr );

        int m_rubber_band_column;
        int m_clamped_min_width;

        static std::map<wxString, int> m_width_cache;
};

#endif // __two_column_tree_list__
