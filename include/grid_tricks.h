/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _GRID_TRICKS_H_
#define _GRID_TRICKS_H_


#include <bitset>
#include <functional>

#include <wx/grid.h>
#include <wx/event.h>
#include <wx/menu.h>
#include <widgets/wx_grid.h>

#define GRIDTRICKS_MAX_COL 50

enum
{
    GRIDTRICKS_FIRST_ID = 901,
    GRIDTRICKS_ID_CUT,
    GRIDTRICKS_ID_COPY,
    GRIDTRICKS_ID_DELETE,
    GRIDTRICKS_ID_PASTE,
    GRIDTRICKS_ID_SELECT,

    GRIDTRICKS_FIRST_CLIENT_ID = 1101,  // reserve IDs for sub-classes
    GRID_TRICKS_LAST_CLIENT_ID = 2100,

    GRIDTRICKS_FIRST_SHOWHIDE,          // reserve IDs for show/hide-column-n

    GRIDTRICKS_LAST_ID = GRIDTRICKS_FIRST_SHOWHIDE + GRIDTRICKS_MAX_COL
};


/**
 * Add mouse and command handling (such as cut, copy, and paste) to a #WX_GRID instance.
 */
class GRID_TRICKS : public wxEvtHandler
{
public:
    explicit GRID_TRICKS( WX_GRID* aGrid );

    GRID_TRICKS( WX_GRID* aGrid, std::function<void( wxCommandEvent& )> aAddHandler );

    /**
     * Enable the tooltip for a column.
     *
     * The tooltip is read from the string contained in the cell data.
     *
     * @param aCol is the column to use
     * @param aEnable is true to enable the tooltip (default)
     */
    void SetTooltipEnable( int aCol, bool aEnable = true )
    {
        m_tooltipEnabled[aCol] = aEnable;
    }

    /**
     * Query if the tooltip for a column is enabled
     *
     * @param aCol is the column to query
     * @return if the tooltip is enabled for the column
     */
    bool GetTooltipEnabled( int aCol )
    {
        return m_tooltipEnabled[aCol];
    }

protected:
    /// Allow various conspiracies between the two tricks handlers
    friend class SCINTILLA_TRICKS;

    /// Shared initialization for various ctors.
    void init();

    /// Puts the selected area into a sensible rectangle of m_sel_{row,col}_{start,count} above.
    void getSelectedArea();

    virtual void onGridCellLeftClick( wxGridEvent& event );
    void onGridCellLeftDClick( wxGridEvent& event );
    void onGridCellRightClick( wxGridEvent& event );
    void onGridLabelLeftClick( wxGridEvent& event );
    void onGridLabelRightClick( wxGridEvent& event );
    void onPopupSelection( wxCommandEvent& event );
    void onKeyDown( wxKeyEvent& event );
    void onCharHook( wxKeyEvent& event );
    void onUpdateUI( wxUpdateUIEvent& event );
    void onGridMotion( wxMouseEvent& event );

    virtual bool handleDoubleClick( wxGridEvent& aEvent );
    virtual void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent );
    virtual void doPopupSelection( wxCommandEvent& event );

    bool isTextEntry( int aRow, int aCol );
    bool isCheckbox( int aRow, int aCol );
    bool isReadOnly( int aRow, int aCol );

    virtual bool toggleCell( int aRow, int aCol, bool aPreserveSelection = false );
    bool showEditor( int aRow, int aCol );

    virtual void paste_clipboard();
    virtual void paste_text( const wxString& cb_text );
    virtual void cutcopy( bool doCopy, bool doDelete );

protected:
    WX_GRID* m_grid;     ///< I don't own the grid, but he owns me

    // row & col "selection" acquisition
    // selected area by cell coordinate and count
    int      m_sel_row_start;
    int      m_sel_col_start;
    int      m_sel_row_count;
    int      m_sel_col_count;

    std::function<void( wxCommandEvent& )> m_addHandler;

    std::bitset<GRIDTRICKS_MAX_COL>        m_tooltipEnabled;

    bool                                   m_enableSingleClickEdit;
    bool                                   m_multiCellEditEnabled;
};

#endif  // _GRID_TRICKS_H_
