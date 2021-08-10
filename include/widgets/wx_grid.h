/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_WX_GRID_H
#define KICAD_WX_GRID_H

#include <wx/grid.h>

class WX_GRID : public wxGrid
{
public:
    // Constructor has to be wxFormBuilder-compatible
    WX_GRID( wxWindow *parent, wxWindowID id,
             const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
             long style = wxWANTS_CHARS, const wxString& name = wxGridNameStr );

    ~WX_GRID() override;

    /**
     * Hide wxGrid's SetColLabelSize() method with one which makes sure the size is tall
     * enough for the system GUI font.
     * @param height
     */
    void SetColLabelSize( int aHeight );

    /**
     * Get a tokenized string containing the shown column indexes.
     * Tokens are separated by spaces.
     */
    wxString GetShownColumns();

    /**
     * Show/hide the grid columns based on a tokenized string of shown column indexes.
     */
    void ShowHideColumns( const wxString& shownColumns );

    /**
     * Hide wxGrid's SetTable() method with one which doesn't mess up the grid column
     * widths when setting the table.
     */
    void SetTable( wxGridTableBase* table, bool aTakeOwnership = false );

    /**
     * Work-around for a bug in wxGrid which crashes when deleting the table if the
     * cell edit control was not closed.
     */
    void DestroyTable( wxGridTableBase* aTable );

    /**
     * Close any open cell edit controls.
     * @param aQuietMode if true don't send events (ie: for row/col delete operations)
     * @return false if validation failed
     */
    bool CommitPendingChanges( bool aQuietMode = false );

    /**
     * Calculates the specified column based on the actual size of the text
     * on screen.  Will return the maximum value of all calculated widths.
     * @param aCol - Integer value of the column to resize.  Specify -1 for the row labels.
     * @param aHeader - Include the header in the width calculation
     * @param aContents - Include the full contents of the column
     * @param aKeep - Use the current size as a minimum value
     * @return The new size of the column
     */
    int GetVisibleWidth( int aCol, bool aHeader = true, bool aContents = false, bool aKeep = true );

    /**
     * Ensure the height of the row displaying the column labels is enough, even
     * if labels are multiline texts
     */
    void EnsureColLabelsVisible();

    /**
     * WxWidgets has a bunch of bugs in its handling of wxGrid mouse events which close cell
     * editors right after opening them.  Helpfully, it already has a bunch of work-arounds in
     * place (such as the SetInSetFocus() hack), including one to make slow clicks work.  We
     * re-purpose this hack to work-around the bugs when we want to open an editor.
     */
    void ShowEditorOnMouseUp() { m_waitForSlowClick = true; }

    /**
     * wxWidgets recently added an ASSERT which fires if the position is greater than or equal
     * to the number of rows (even if the delete count is 0).  Needless to say, this makes using
     * DeleteRows for clearing a lot more cumbersome so we add a helper here.
     */
    void ClearRows()
    {
        if( GetNumberRows() )
            DeleteRows( 0, GetNumberRows() );
    }

protected:
    /**
     * A re-implementation of wxGrid::DrawColLabel which left-aligns the first column when
     * there are no row labels.
     */
    void DrawColLabel( wxDC& dc, int col ) override;

    void onGridColMove( wxGridEvent& aEvent );

    bool m_weOwnTable;
};

#endif //KICAD_WX_GRID_H
