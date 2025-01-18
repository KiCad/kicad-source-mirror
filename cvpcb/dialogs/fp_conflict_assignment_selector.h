/**
 * @file fp_conflict_assignment_selector.h
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <fp_conflict_assignment_selector_base.h>


class DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR : public DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE
{
public:
    DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR( wxWindow* parent );

    /**
     * Add a line to the selection list.
     *
     * @param aRef is the component reference text.
     * @param aFpSchName is the fpid text from the netlist.
     * @param aFpCmpName is the fpid text from the .cmp file.
     */
    void Add( const wxString& aRef, const wxString& aFpSchName, const wxString& aFpCmpName );

    /**
     * @param aReference is the component schematic reference.
     * @retval  0 for fpid text from the netlist.
     * @retval  1 for fpid text from the cmp file.
     * @retval -1 on error.
     */
    int GetSelection( const wxString& aReference );

private:
    void OnSize( wxSizeEvent& event ) override;

    // Virtual: called when clicking on the column title:
    // when it is a column choice, set all item choices.
    void OnColumnClick( wxListEvent& event ) override;

    void OnItemClicked( wxMouseEvent& event ) override;

    void OnCancelClick( wxCommandEvent& event ) override { EndModal( wxID_CANCEL ); }
    void OnOKClick( wxCommandEvent& event ) override { EndModal( wxID_OK ); }

    void recalculateColumns();

    enum COL_ID
    {
        COL_REF, COL_FPSCH, COL_SELSCH, COL_SELCMP, COL_FPCMP,
        COL_COUNT
    };

    int m_lineCount;
};
