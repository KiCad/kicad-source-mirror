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
#include <utility>
#include <vector>

#include <optional>

#include <drc/rule_editor/drc_rule_editor_enums.h>

#include "constraint_rules_list_base.h"
#include "panel_setup_constraint_rules.h"

class BOARD;
class DRC_RE_BASE_CONSTRAINT_DATA;
class DRC_RULE_DOCUMENT;
class NETCLASS;
class PANEL_DRC_RULE_EDITOR;
class PCB_EDIT_FRAME;
class UNITS_PROVIDER;
struct DRC_RE_LOADED_PANEL_ENTRY;

/**
 * The rules of one constraint type, in evaluation order, with an editor that opens in place.
 *
 * Edits go to the shared DRC_RULE_DOCUMENT. Nothing is written to disk here.
 */
class CONSTRAINT_RULES_LIST : public CONSTRAINT_RULES_LIST_BASE
{
public:
    CONSTRAINT_RULES_LIST( wxWindow* aParent, PCB_EDIT_FRAME* aFrame, DRC_RULE_DOCUMENT* aDocument,
                           DRC_RULE_EDITOR_CONSTRAINT_NAME aType, const wxString& aTitle );

    void RefreshRules();

    /**
     * Lists the net class values for this constraint above the rules, where a class can set one.
     *
     * DRC reads a net class value before any rule the user writes here, so the values belong in
     * the same list, in the order they are read.
     */
    void SetNetclasses( std::function<std::optional<int>( const NETCLASS& )> aValue, NETCLASS_CONSTRAINT_PART aPart );

    /**
     * A window whose label is the section title. The list appends the rule count to it.
     */
    void SetTitleWindow( wxWindow* aWindow ) { m_titleWindow = aWindow; }

    /// The board defaults this constraint falls back to, each with the name the note gives it
    void SetDefaults( std::vector<std::pair<wxString, std::function<wxString()>>> aDefaults )
    {
        m_defaults = std::move( aDefaults );
    }

    /// Rebuilds the line saying what the rule being edited displaces
    void RefreshOverrideNote() { showOverrideNote(); }

    /**
     * True while the form holds typing that has not been added to the list.
     */
    bool HasPendingRule() const;

    /**
     * Adds or updates the rule the form holds. False if it does not validate, with the reason
     * already reported.
     */
    bool ApplyPendingRule();

    /**
     * Reloads the form from the document, dropping whatever was typed.
     */
    void DiscardPendingRule();

    /**
     * Asks what to do with a rule left in the form. False if the user cancels or the rule does
     * not validate, and the form is left as it was.
     */
    bool ConfirmPendingRule();

    /// Selects a rule and opens it in the editor. False if a rule left in the form kept it closed
    bool EditRule( int aIndex );

    /**
     * Called after this list changed the document. The page uses it to refresh every list,
     * since the entries are shared.
     */
    void SetChangeCallback( std::function<void()> aCallback ) { m_onChanged = std::move( aCallback ); }

    /// The constraint a rule sets, written the way the rule table shows it
    static wxString ConstraintText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry, DRC_RULE_EDITOR_CONSTRAINT_NAME aType,
                                    UNITS_PROVIDER* aUnits );

private:
    void onPrimary( wxCommandEvent& aEvent );
    void onNewRule( wxCommandEvent& aEvent );
    void onMoveUp( wxCommandEvent& aEvent );
    void onMoveDown( wxCommandEvent& aEvent );
    void onDuplicate( wxCommandEvent& aEvent );
    void onRemove( wxCommandEvent& aEvent );

    void moveSelected( int aDelta );

    /// Copies everything the form edits, the rule name included
    static void copyRuleData( DRC_RE_BASE_CONSTRAINT_DATA& aTarget, const DRC_RE_BASE_CONSTRAINT_DATA& aSource );

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

    void showOverrideNote();

    /// Fills the leading rows with the net class values, and returns how many it added
    int addNetclassRows();

    static wxString layersText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry );
    static wxString severityText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry );
    static wxString inUserUnits( const wxString& aClause, UNITS_PROVIDER* aUnits );
    wxString        sharedWithText( int aIndex ) const;

    PCB_EDIT_FRAME*                                 m_frame;
    DRC_RULE_DOCUMENT*                              m_document;
    DRC_RULE_EDITOR_CONSTRAINT_NAME                 m_type;
    wxString                                        m_title;
    wxWindow*                                       m_titleWindow;
    std::vector<std::pair<wxString, std::function<wxString()>>> m_defaults;

    PANEL_DRC_RULE_EDITOR*                       m_editor;
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> m_editingData;
    DRC_RULE_EDITOR_CONSTRAINT_NAME              m_editingType;
    std::function<void()>                        m_onChanged;
    bool                                         m_refreshing = false;
    int                                          m_fittedWidth = -1;

    std::function<std::optional<int>( const NETCLASS& )> m_netclassValue;
    NETCLASS_CONSTRAINT_PART                             m_netclassPart = NETCLASS_CONSTRAINT_PART::MINIMUM;

    /// The document index each row shows, or NETCLASS_ROW where the row is a net class value
    std::vector<int> m_rowToIndex;

    /// What m_rowToIndex holds for a row that is not a rule of this document
    static constexpr int NETCLASS_ROW = -1;
};

#endif // CONSTRAINT_RULES_LIST_H
