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


#pragma once

#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_setup_constraint_rules.h>
#include "panel_setup_formatting.h"

class BOARD;
class BOARD_DESIGN_SETTINGS;
class PCB_EDIT_FRAME;
class DRC_RULE_DOCUMENT;
class PANEL_SETUP_CONSTRAINTS;
class PANEL_SETUP_CONSTRAINT_RULES;
class PANEL_SETUP_LAYERS;
class PANEL_SETUP_DEFAULTS;
class PANEL_SETUP_NETCLASSES;
class PANEL_SETUP_RULES;
class PANEL_SETUP_TRACKS_AND_VIAS;
class PANEL_SETUP_MASK_AND_PASTE;
class PANEL_SETUP_ZONES;
class PANEL_SETUP_BOARD_STACKUP;
class PANEL_SETUP_BOARD_FINISH;
class PANEL_SETUP_SEVERITIES;
class PANEL_TEXT_VARIABLES;
class PANEL_SETUP_TUNING_PROFILES;


class DIALOG_BOARD_SETUP : public PAGED_DIALOG
{
public:
    DIALOG_BOARD_SETUP( PCB_EDIT_FRAME* aFrame, wxWindow* aWindow = nullptr );
    ~DIALOG_BOARD_SETUP();

protected:
    // event handlers
    void onPageChanged( wxBookCtrlEvent& aEvent ) override;
    void onAuxiliaryAction( wxCommandEvent& aEvent ) override;
    void onCancelButton( wxCommandEvent& aEvent );
    void onCloseWindow( wxCloseEvent& aEvent );

    void AppendSearchTerms( size_t aPage, std::vector<wxString>& aTerms ) const override;
    void BeginSearchPass() override;

    /// The rules the search reads, which is the text editor's draft while it is being typed
    const DRC_RULE_DOCUMENT& searchDocument() const;

    void refreshRulePage( size_t aPage );

    /// Reads the staged settings back into the open pages of one Import Settings group
    void refreshStagedPages( const wxString& aGroup );
    void forEachOpenRulePage( const std::function<void( PANEL_SETUP_CONSTRAINT_RULES* )>& aAction );

    bool TransferDataFromWindow() override;

    PCB_EDIT_FRAME*                     m_frame;
    PANEL_SETUP_LAYERS*                 m_layers;
    PANEL_SETUP_BOARD_FINISH*           m_boardFinish;
    PANEL_SETUP_BOARD_STACKUP*          m_physicalStackup;
    PANEL_SETUP_ZONES*                  m_zones;
    PANEL_SETUP_TUNING_PROFILES*        m_tuningProfiles;
    PANEL_SETUP_NETCLASSES*             m_netClasses;

private:
    bool canDiscardChanges();

    size_t m_currentPage;              // the current page index
    size_t m_layersPage;
    size_t m_physicalStackupPage;
    size_t m_boardFinishPage;
    size_t m_defaultsPage;
    size_t m_formattingPage;
    size_t m_textVarsPage;
    size_t m_drillChartPage;
    size_t m_customRulesPage;
    size_t m_tracksAndViasPage;
    size_t m_viaStacksPage;
    size_t m_zonesPage;
    size_t m_teardropsPage;
    size_t m_tuningPatternsPage;
    size_t m_netclassesPage;
    size_t m_componentClassesPage;
    size_t m_severitiesPage;
    size_t m_embeddedFilesPage;
    size_t m_tuningProfilesPage;
    bool   m_initComplete;

    std::unique_ptr<DRC_RULE_DOCUMENT> m_ruleDocument;

    /// The constraint, mask and rule pages edit one value from several pages, so they share this
    /// copy, which an import also writes and which reaches the board only on OK
    std::unique_ptr<BOARD_DESIGN_SETTINGS> m_stagedSettings;
    std::map<size_t, CONSTRAINT_SECTION>   m_ruleSearchSections;

    /// The rules of each rule page, sorted once per search query
    mutable std::optional<std::map<DRC_RULE_EDITOR_CONSTRAINT_NAME, std::vector<int>>> m_searchRuleIndices;
};
