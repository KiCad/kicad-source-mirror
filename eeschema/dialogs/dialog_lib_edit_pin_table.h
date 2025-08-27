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

#include "dialog_lib_edit_pin_table_base.h"

#include <sch_pin.h>

enum COL_ORDER
{
    COL_PIN_COUNT,
    COL_NUMBER,
    COL_NAME,
    COL_TYPE,
    COL_SHAPE,
    COL_ORIENTATION,
    COL_NUMBER_SIZE,
    COL_NAME_SIZE,
    COL_LENGTH,
    COL_POSX,
    COL_POSY,
    COL_VISIBLE,
    COL_UNIT,
    COL_BODY_STYLE,

    COL_COUNT // keep as last
};


class ACTION_MENU;
class PIN_TABLE_DATA_MODEL;
class SYMBOL_EDIT_FRAME;


class DIALOG_LIB_EDIT_PIN_TABLE : public DIALOG_LIB_EDIT_PIN_TABLE_BASE
{
public:
    DIALOG_LIB_EDIT_PIN_TABLE( SYMBOL_EDIT_FRAME* parent, LIB_SYMBOL* aSymbol,
                               const std::vector<SCH_PIN*>& aSelectedPins );
    ~DIALOG_LIB_EDIT_PIN_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnColSort( wxGridEvent& aEvent );
    void OnAddRow( wxCommandEvent& event ) override;
    void OnDeleteRow( wxCommandEvent& event ) override;
    void OnSize( wxSizeEvent& event ) override;
    void OnCellEdited( wxGridEvent& event ) override;
    void OnCellSelected( wxGridEvent& event ) override;
    void OnRebuildRows( wxCommandEvent& event ) override;
    void OnGroupSelected( wxCommandEvent& event ) override;
    void OnFilterCheckBox( wxCommandEvent& event ) override;
    void OnFilterChoice( wxCommandEvent& event ) override;
    void OnImportButtonClick( wxCommandEvent& event ) override;
    void OnExportButtonClick( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnCancel( wxCommandEvent& event ) override;
    void OnClose( wxCloseEvent& event ) override;
    void AddPin( SCH_PIN* pin );
    void RemovePin( SCH_PIN* pin );
    bool IsDisplayGrouped();

protected:
    void updateSummary();
    void adjustGridColumns();

protected:
    SYMBOL_EDIT_FRAME*    m_editFrame;
    bool                  m_initialized = false;
    int                   m_originalColWidths[ COL_COUNT ];
    std::bitset<64>       m_columnsShown;
    LIB_SYMBOL*           m_symbol;
    std::vector<SCH_PIN*> m_pins;       // a copy of the pins owned by the dialog
    bool                  m_modified;   ///< true when there are unsaved changes
    wxSize                m_size;

    PIN_TABLE_DATA_MODEL* m_dataModel;

    std::unique_ptr<ACTION_MENU> m_menu;
};
