/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PANEL_DRC_RULE_EDITOR_H
#define PANEL_DRC_RULE_EDITOR_H

#include <wx/wx.h>

#include <lset.h>
#include <lseq.h>
#include <variant>

#include "panel_drc_rule_editor_base.h"
#include "drc_rule_editor_enums.h"
#include "drc_rule_editor_utils.h"
#include "drc_re_content_panel_base.h"
#include "drc_re_base_constraint_data.h"
#include <dialogs/rule_editor_data_base.h>


class SCINTILLA_TRICKS;
class HTML_MESSAGE_BOX;
class DRC_RE_CONDITION_GROUP_PANEL;

class PANEL_DRC_RULE_EDITOR : public PANEL_DRC_RULE_EDITOR_BASE,
                              public DRC_RULE_EDITOR_CONTENT_PANEL_BASE
{
public:
    PANEL_DRC_RULE_EDITOR( wxWindow* aParent, BOARD* aBoard, DRC_RULE_EDITOR_CONSTRAINT_NAME aConstraintType,
                           std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> aConstraintData );

    ~PANEL_DRC_RULE_EDITOR() override;

    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

    auto GetConstraintData() { return m_constraintData; }

    bool IsModified() const { return m_modified; }

    void SetSaveCallback( std::function<void( int aNodeId )> aCallBackSave )
    {
        m_callBackSave = aCallBackSave;
    }

    void SetRuleNameValidationCallback(
            std::function<bool( int aNodeId, const wxString& aRuleName )> aCallbackRuleNameValidation )
    {
        m_callBackRuleNameValidation = aCallbackRuleNameValidation;
    }

    void SetShowMatchesCallBack( std::function<int( int aNodeId )> aCallBackShowMatches )
    {
        m_callBackShowMatches = aCallBackShowMatches;
    }

    /**
     * The rule editor that hosts aChild, or nullptr when it is not inside one.
     */
    static PANEL_DRC_RULE_EDITOR* FindEditor( wxWindow* aChild );

    bool GetIsValidationSucceeded() { return m_validationSucceeded; }

    wxString GetValidationMessage() { return m_validationMessage; }

    bool ValidateInputs( int* aErrorCount, wxString* aValidationMessage ) override;

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override;

    void OnEnterKey( wxCommandEvent& aEvent ) override;

    void Save( wxCommandEvent& aEvent );

private:
    DRC_RULE_EDITOR_CONTENT_PANEL_BASE* getConstraintPanel( wxWindow* aParent,
        const DRC_RULE_EDITOR_CONSTRAINT_NAME& aConstraintType );

    wxString buildLayerClause() const;

    void onFieldChanged( wxEvent& aEvent );

    void clearMatchReport();

    void relayoutHost();

    void giveConstraintColumnTheWidth();

    /**
     * Handles the save button click event, validating inputs and invoking the save callback if valid.
     *
     * @param aEvent The event triggered by the save button click.
     */
    void onSaveButtonClicked( wxCommandEvent& aEvent );

    /**
     * Handles character addition in the Scintilla text control, performing auto-complete and context-sensitive
     * operations.
     *
     * @param aEvent The event triggered when a character is added.
     */
    void onScintillaCharAdded( wxStyledTextEvent& aEvent );

    /**
     * Displays a modeless help window with syntax and rule documentation.
     *
     * @param aEvent The event triggered by the hyperlink click.
     */
    void onSyntaxHelp( wxHyperlinkEvent& aEvent ) override;

    /**
     * Checks the syntax of the DRC rule condition and reports any errors.
     *
     * @param aEvent The event triggered by the syntax check action.
     */
    void onCheckSyntax( wxCommandEvent& aEvent ) override;

    /**
     * Handles clicks on error links in the syntax error report and navigates to the error location.
     *
     * @param aEvent The event triggered when an error link is clicked.
     */
    void onErrorLinkClicked( wxHtmlLinkEvent& aEvent ) override;

    /**
     * Handles right-click context menu actions for text editing (undo, redo, cut, copy, paste, delete, select all,
     * zoom).
     *
     * @param aEvent The event triggered by the right-click menu.
     */
    void onContextMenu( wxMouseEvent& aEvent ) override;

    void onShowMatchesButtonClicked( wxCommandEvent& aEvent );

private:
    void populateLayerSelector( DRC_LAYER_CATEGORY aCategory );
    std::vector<PCB_LAYER_ID> getSelectedLayers();
    wxString                  getSelectedLayerSource() const;
    void setSelectedLayers( const std::vector<PCB_LAYER_ID>& aLayers,
                            const wxString& aLayerSource = wxEmptyString );

    wxButton*         m_btnShowMatches;
    wxStaticText*     m_matchReport;
    std::vector<int>  m_validLayers;
    LSEQ              m_layerList;
    BOARD*            m_board;
    bool              m_validationSucceeded;
    bool              m_modified;
    wxString          m_validationMessage;

    std::unique_ptr<SCINTILLA_TRICKS>            m_scintillaTricks;
    wxChoice*                                    m_layerListChoiceCtrl;
    std::vector<int>                             m_layerIDs;   // PCB_LAYER_ID or DRC_LAYER_SELECTOR_ID
    DRC_LAYER_CATEGORY                           m_layerCategory;
    DRC_RULE_EDITOR_CONSTRAINT_NAME              m_constraintType;
    DRC_RULE_EDITOR_CONTENT_PANEL_BASE*          m_constraintPanel;
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> m_constraintData;
    DRC_RE_CONDITION_GROUP_PANEL*                m_conditionGroupPanel;

    std::function<void( int aNodeId )>                            m_callBackSave;
    std::function<bool( int aNodeId, const wxString& aRuleName )> m_callBackRuleNameValidation;
    std::function<int( int aNodeId )> m_callBackShowMatches;

    HTML_MESSAGE_BOX* m_helpWindow;
};

#endif // PANEL_DRC_RULE_EDITOR_H
