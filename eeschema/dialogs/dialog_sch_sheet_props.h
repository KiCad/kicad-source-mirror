/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2014-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef __dialog_sch_sheet_props__
#define __dialog_sch_sheet_props__

#include <fields_grid_table.h>
#include <widgets/unit_binder.h>
#include <dialog_sch_sheet_props_base.h>


class SCH_SHEET;
class SCH_EDIT_FRAME;


class DIALOG_SCH_SHEET_PROPS : public DIALOG_SCH_SHEET_PROPS_BASE
{
public:
    DIALOG_SCH_SHEET_PROPS( SCH_EDIT_FRAME* aParent, SCH_SHEET* aSheet,
                            bool* aClearAnnotationNewItems );

    ~DIALOG_SCH_SHEET_PROPS() override;

private:
    SCH_EDIT_FRAME* m_frame;
    SCH_SHEET*      m_sheet;
    bool*           m_clearAnnotationNewItems;

    int             m_width;
    int             m_delayedFocusRow;
    int             m_delayedFocusColumn;
    wxString        m_shownColumns;

    FIELDS_GRID_TABLE<SCH_FIELD>* m_fields;
    UNIT_BINDER                   m_borderWidth;

    bool onSheetFilenameChanged( const wxString& aNewFilename );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    // event handlers
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnSizeGrid( wxSizeEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnInitDlg( wxInitDialogEvent& event ) override;

    void AdjustGridColumns( int aWidth );
};

#endif // __dialog_sch_sheet_props__
