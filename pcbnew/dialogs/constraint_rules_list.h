/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef CONSTRAINT_RULES_LIST_H
#define CONSTRAINT_RULES_LIST_H

#include <functional>
#include <memory>
#include <vector>
#include <wx/panel.h>

#include <drc/rule_editor/drc_rule_editor_enums.h>

class wxButton;
class wxBoxSizer;
class wxListCtrl;
class wxStaticText;
class BOARD;
class DRC_RE_BASE_CONSTRAINT_DATA;
class DRC_RULE_DOCUMENT;
class PANEL_DRC_RULE_EDITOR;
class PCB_EDIT_FRAME;
struct DRC_RE_LOADED_PANEL_ENTRY;

/**
 * The rules of one constraint type, in evaluation order, with an editor that opens in place.
 *
 * Edits go to the shared DRC_RULE_DOCUMENT. Nothing is written to disk here.
 */
class CONSTRAINT_RULES_LIST : public wxPanel
{
public:
    CONSTRAINT_RULES_LIST( wxWindow* aParent, PCB_EDIT_FRAME* aFrame, DRC_RULE_DOCUMENT* aDocument,
                           DRC_RULE_EDITOR_CONSTRAINT_NAME aType, const wxString& aTitle );

    void RefreshRules();

    /**
     * A window whose label is the section title. The list appends the rule count to it.
     */
    void SetTitleWindow( wxWindow* aWindow ) { m_titleWindow = aWindow; }

    /**
     * Called after this list changed the document. The page uses it to refresh every list,
     * since the entries are shared.
     */
    void SetChangeCallback( std::function<void()> aCallback ) { m_onChanged = std::move( aCallback ); }

private:
    void onPrimary( wxCommandEvent& aEvent );
    void onNewRule( wxCommandEvent& aEvent );
    void onMoveUp( wxCommandEvent& aEvent );
    void onMoveDown( wxCommandEvent& aEvent );
    void onDuplicate( wxCommandEvent& aEvent );
    void onRemove( wxCommandEvent& aEvent );

    void buildEditor( const std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>& aData,
                      DRC_RULE_EDITOR_CONSTRAINT_NAME                     aPanelType );
    void showNewRule();
    void showRule( int aIndex );
    void addRule();
    void applyRule();
    int  indexOf( const std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>& aData ) const;
    int  editingIndex() const;
    void notifyChanged();
    int  showMatches();

    int  selectedIndex() const;
    void selectIndex( int aIndex );
    void updateButtons();
    void relayout();
    void reportError( const wxString& aMessage );

    void fitColumns();

    static wxString layersText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry );
    wxString        constraintText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry ) const;
    static wxString severityText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry );
    wxString        sharedWithText( int aIndex ) const;
    wxString        inUserUnits( const wxString& aClause ) const;

    PCB_EDIT_FRAME*                 m_frame;
    DRC_RULE_DOCUMENT*              m_document;
    DRC_RULE_EDITOR_CONSTRAINT_NAME m_type;
    wxString                        m_title;
    wxWindow*                       m_titleWindow;

    wxListCtrl*   m_list;
    wxStaticText* m_emptyLabel;
    wxButton*     m_primaryBtn;
    wxButton*     m_newBtn;
    wxButton*     m_upBtn;
    wxButton*     m_downBtn;
    wxButton*     m_duplicateBtn;
    wxButton*     m_removeBtn;

    wxPanel*                                     m_editorHost;
    wxBoxSizer*                                  m_editorSizer;
    PANEL_DRC_RULE_EDITOR*                       m_editor;
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> m_editingData;
    DRC_RULE_EDITOR_CONSTRAINT_NAME              m_editingType;
    std::function<void()>                        m_onChanged;
    bool                                         m_refreshing = false;

    std::vector<int> m_rowToIndex;
};

#endif // CONSTRAINT_RULES_LIST_H
