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

#pragma once

#include <fields_grid_table.h>
#include <widgets/unit_binder.h>
#include <sch_validators.h>
#include <dialog_label_properties_base.h>


class SCH_EDIT_FRAME;
class HTML_MESSAGE_BOX;


class DIALOG_LABEL_PROPERTIES : public DIALOG_LABEL_PROPERTIES_BASE
{
public:
    DIALOG_LABEL_PROPERTIES( SCH_EDIT_FRAME* parent, SCH_LABEL_BASE* aLabel, bool aNew );
    ~DIALOG_LABEL_PROPERTIES();

    FIELDS_GRID_TABLE* GetFieldsGridTable() { return m_fields; }

    void SetLabelList( std::list<std::unique_ptr<SCH_LABEL_BASE>>* aLabelList )
    {
        m_labelList = aLabelList;
    }

private:
    /**
     * wxEVT_COMMAND_ENTER event handler for single-line control.
     */
    void OnEnterKey( wxCommandEvent& aEvent ) override;
    void OnValueCharHook( wxKeyEvent& aEvent ) override;
    void OnCBValueCharHook( wxKeyEvent& aEvent ) override;
    void OnFormattingHelp( wxHyperlinkEvent& aEvent ) override;

    void onSpinButton( wxCommandEvent &aEvent );

    // event handlers
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void onMultiLabelCheck( wxCommandEvent& aEvent ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    SCH_EDIT_FRAME*       m_Parent;
    int                   m_delayedFocusRow;
    int                   m_delayedFocusColumn;

    SCH_LABEL_BASE*       m_currentLabel;
    wxTextEntry*          m_activeTextEntry;
    SCH_NETNAME_VALIDATOR m_netNameValidator;

    FIELDS_GRID_TABLE*    m_fields;
    std::bitset<64>       m_shownColumns;

    UNIT_BINDER           m_textSize;

    HTML_MESSAGE_BOX*     m_helpWindow;
    wxArrayString         m_existingLabelArray;
    // To store the previous value of the text typed in label combo
    wxString              m_previousLabelText;

    std::list<std::unique_ptr<SCH_LABEL_BASE>>* m_labelList;

    bool m_multilineAllowed;    // set to true when a multine set of labels is in edit
};
