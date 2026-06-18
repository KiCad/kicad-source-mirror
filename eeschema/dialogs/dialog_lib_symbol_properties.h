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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#pragma once

#include <memory>

#include <fields_grid_table.h>
#include <widgets/unit_binder.h>
#include <dialog_lib_symbol_properties_base.h>


class SYMBOL_EDIT_FRAME;
class LIB_SYMBOL;
class LISTBOX_TRICKS;
class PANEL_EMBEDDED_FILES;
class PANEL_SYMBOL_PIN_MAP;
class WX_GRID;


class DIALOG_LIB_SYMBOL_PROPERTIES: public DIALOG_LIB_SYMBOL_PROPERTIES_BASE
{
public:
    DIALOG_LIB_SYMBOL_PROPERTIES( SYMBOL_EDIT_FRAME* parent, LIB_SYMBOL* aLibEntry );
    ~DIALOG_LIB_SYMBOL_PROPERTIES();

    /// Switch the notebook to the Pin Map page (issue #2282).
    void SelectPinMapPage();

protected:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    void onPowerCheckBox( wxCommandEvent& aEvent ) override;
    void OnText( wxCommandEvent& event ) override;
  	void OnCombobox( wxCommandEvent& event ) override;
  	void OnCheckBox( wxCommandEvent& event ) override;
    void OnUnitSpinCtrl( wxSpinEvent& event ) override;
    void OnUnitSpinCtrlText( wxCommandEvent& event ) override;
    void OnUnitSpinCtrlKillFocus( wxFocusEvent& event ) override;
    void OnUnitSpinCtrlEnter( wxCommandEvent& event ) override;
    void OnBodyStyle( wxCommandEvent& event ) override;

private:
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnAddBodyStyle( wxCommandEvent& event ) override;
    void OnBodyStyleMoveUp( wxCommandEvent& event ) override;
    void OnBodyStyleMoveDown( wxCommandEvent& event ) override;
    void OnDeleteBodyStyle( wxCommandEvent& event ) override;
    void OnSymbolNameKillFocus( wxFocusEvent& event ) override;
    void OnSymbolNameText( wxCommandEvent& event ) override;
    void OnAddFootprintFilter( wxCommandEvent& event ) override;
    void OnEditFootprintFilter( wxCommandEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnGridCellChanged( wxGridEvent& event );
    void OnGridMotion( wxMouseEvent& event );
    void OnEditSpiceModel( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnCancelButtonClick( wxCommandEvent& event ) override;
    void OnPageChanging( wxNotebookEvent& event ) override;
    void OnFpFilterDClick( wxMouseEvent& event ) override;
    void OnAddJumperGroup( wxCommandEvent& event ) override;
    void OnRemoveJumperGroup( wxCommandEvent& event ) override;

    bool updateUnitCount();
    void syncControlStates( bool aIsAlias );
    void addInheritedFields( const std::shared_ptr<LIB_SYMBOL>& aParent );

public:
    SYMBOL_EDIT_FRAME* m_Parent;
    LIB_SYMBOL*        m_libEntry;

    FIELDS_GRID_TABLE* m_fields;
    std::set<wxString> m_addedTemplateFields;

    UNIT_BINDER        m_pinNameOffset;

    wxControl*         m_delayedFocusCtrl;
    WX_GRID*           m_delayedFocusGrid;
    int                m_delayedFocusRow;
    int                m_delayedFocusColumn;
    int                m_delayedFocusPage;
    wxString           m_delayedErrorMessage;

    std::bitset<64>    m_shownColumns;

    PANEL_EMBEDDED_FILES* m_embeddedFiles;
    PANEL_SYMBOL_PIN_MAP* m_pinMapPanel;

private:
    static int m_lastOpenedPage;    // To remember the last notebook selection

    /// When set by SelectPinMapPage(), TransferDataToWindow opens the Pin Map page instead of the
    /// remembered last page (issue #2282).
    bool m_forcePinMapPage = false;

    enum class LAST_LAYOUT {
        NONE,
        ALIAS,
        PARENT
    };

    static LAST_LAYOUT m_lastLayout;

    std::unique_ptr<LISTBOX_TRICKS> m_fpFilterTricks;
};
