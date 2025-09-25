/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
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

#pragma once

#include <fields_grid_table.h>
#include <widgets/unit_binder.h>
#include <dialog_sheet_properties_base.h>
#include <sch_sheet.h>

class SCH_SHEET;
class SCH_EDIT_FRAME;


class DIALOG_SHEET_PROPERTIES : public DIALOG_SHEET_PROPERTIES_BASE
{
public:
    DIALOG_SHEET_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_SHEET* aSheet, bool* aIsUndoable,
                             bool* aClearAnnotationNewItems, bool* aUpdateHierarchyNavigator,
                             wxString* aSourceSheetFilename );

    ~DIALOG_SHEET_PROPERTIES() override;

private:
    bool onSheetFilenameChanged( const wxString& aNewFilename );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    // event handlers
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

private:
    SCH_EDIT_FRAME*    m_frame;
    SCH_SHEET*         m_sheet;
    bool*              m_isUndoable;
    bool*              m_clearAnnotationNewItems;
    bool*              m_updateHierarchyNavigator;
    wxString*          m_sourceSheetFilename;

    int                m_delayedFocusRow;
    int                m_delayedFocusColumn;
    std::bitset<64>    m_shownColumns;

    FIELDS_GRID_TABLE* m_fields;
    UNIT_BINDER        m_borderWidth;

    SCH_SHEET          m_dummySheet;
    SCH_FIELD          m_dummySheetNameField;
};
