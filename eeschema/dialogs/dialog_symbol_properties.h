/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#ifndef DIALOG_SYMBOL_PROPERTIES_H
#define DIALOG_SYMBOL_PROPERTIES_H

#include <dialog_symbol_properties_base.h>
#include <fields_grid_table.h>
#include <sch_pin.h>


class LIB_SYMBOL;
class SCH_PIN_TABLE_DATA_MODEL;
class SCH_EDIT_FRAME;
class PANEL_EMBEDDED_FILES;


// The dialog can be closed for several reasons.
enum SYMBOL_PROPS_RETVALUE
{
    SYMBOL_PROPS_WANT_UPDATE_SYMBOL,
    SYMBOL_PROPS_WANT_EXCHANGE_SYMBOL,
    SYMBOL_PROPS_EDIT_OK,
    SYMBOL_PROPS_EDIT_SCHEMATIC_SYMBOL,
    SYMBOL_PROPS_EDIT_LIBRARY_SYMBOL
};


/**
 * Dialog used to edit #SCH_SYMBOL objects in a schematic.
 *
 * This is derived from DIALOG_SYMBOL_PROPERTIES_BASE which is maintained by
 * wxFormBuilder.
 */
class DIALOG_SYMBOL_PROPERTIES : public DIALOG_SYMBOL_PROPERTIES_BASE
{
public:
    DIALOG_SYMBOL_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_SYMBOL* aSymbol );
    ~DIALOG_SYMBOL_PROPERTIES() override;

    SCH_EDIT_FRAME* GetParent();

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    // event handlers
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnEditSpiceModel( wxCommandEvent& event ) override;
    void OnPinTableColSort( wxGridEvent& aEvent );
    void OnPinTableCellEdited( wxGridEvent& event ) override;
    void OnSizePinsGrid( wxSizeEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnCancelButtonClick( wxCommandEvent& event ) override;
    void OnGridEditorShown( wxGridEvent& event ) override;
    void OnGridEditorHidden( wxGridEvent& event ) override;
    void OnUnitChoice( wxCommandEvent& event ) override;
    void OnCheckBox( wxCommandEvent& event ) override;
    void OnPageChanging( wxNotebookEvent& event ) override;

    void OnEditSymbol( wxCommandEvent&  ) override;
    void OnEditLibrarySymbol( wxCommandEvent&  ) override;
    void OnUpdateSymbol( wxCommandEvent&  ) override;
    void OnExchangeSymbol( wxCommandEvent&  ) override;

    void AdjustPinsGridColumns();
    void HandleDelayedFocus( wxCommandEvent& event );
    void HandleDelayedSelection( wxCommandEvent& event );

    virtual void onUpdateEditSymbol( wxUpdateUIEvent& event ) override;
    virtual void onUpdateEditLibrarySymbol( wxUpdateUIEvent& event ) override;

private:
    SCH_SYMBOL*               m_symbol;
    LIB_SYMBOL*               m_part;

    wxSize                    m_pinsSize;
    wxSize                    m_lastRequestedPinsSize;
    bool                      m_editorShown;
    std::bitset<64>           m_shownColumns;

    FIELDS_GRID_TABLE*        m_fields;
    SCH_PIN_TABLE_DATA_MODEL* m_dataModel;
    PANEL_EMBEDDED_FILES*     m_embeddedFiles;
};

#endif // DIALOG_SYMBOL_PROPERTIES_H
