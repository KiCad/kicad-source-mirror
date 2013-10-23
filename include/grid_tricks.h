/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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


#include <wx/grid.h>
#include <wx/event.h>


/**
 * Class GRID_TRICKS
 * is used to add cut, copy, and paste to an otherwise unmodied wxGrid instance.
 */
class GRID_TRICKS : public wxEvtHandler
{
public:

    GRID_TRICKS( wxGrid* aGrid );


protected:
    wxGrid* m_grid;     ///< I don't own the grid, but he owns me

    // row & col "selection" acquisition
    // selected area by cell coordinate and count
    int     m_sel_row_start;
    int     m_sel_col_start;
    int     m_sel_row_count;
    int     m_sel_col_count;

    /// If the cursor is not on a valid cell, because there are no rows at all, return -1,
    /// else return a 0 based column index.
    int getCursorCol() const
    {
        return m_grid->GetGridCursorCol();
    }

    /// If the cursor is not on a valid cell, because there are no rows at all, return -1,
    /// else return a 0 based row index.
    int getCursorRow() const
    {
        return m_grid->GetGridCursorRow();
    }

    /// Puts the selected area into a sensible rectangle of m_sel_{row,col}_{start,count} above.
    void getSelectedArea();

    static bool isCtl( int aChar, const wxKeyEvent& e )
    {
        return e.GetKeyCode() == aChar && e.ControlDown() && !e.AltDown() && !e.ShiftDown() && !e.MetaDown();
    }

    void onGridCellRightClick( wxGridEvent& event )
    {
        showPopupMenu();
    }

    void onRightDown( wxMouseEvent& event )
    {
        showPopupMenu();
    }

    virtual void showPopupMenu();

    // the user clicked on a popup menu choice:
    void onPopupSelection( wxCommandEvent& event );

    void onKeyDown( wxKeyEvent& ev );

    virtual void paste_clipboard();

    virtual void paste_text( const wxString& cb_text );

    virtual void cutcopy( bool doCut );
};

