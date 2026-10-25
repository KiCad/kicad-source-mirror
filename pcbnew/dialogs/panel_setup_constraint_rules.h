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

#ifndef PANEL_SETUP_CONSTRAINT_RULES_H
#define PANEL_SETUP_CONSTRAINT_RULES_H

#include <functional>
#include <optional>
#include <utility>
#include <vector>
#include <wx/panel.h>

#include <drc/rule_editor/drc_rule_editor_enums.h>

class wxScrolledWindow;
class wxStaticText;
class BOARD;
class BOARD_DESIGN_SETTINGS;
class CONSTRAINT_RULES_LIST;
class DRC_RULE_DOCUMENT;
class NETCLASS;
class PCB_EDIT_FRAME;


struct CONSTRAINT_DEFAULT_FIELD
{
    wxString label;
    int BOARD_DESIGN_SETTINGS::* member;
    wxString                     settingName;
};


/**
 * One constraint on a Design Rules page: its board default, the net class values that apply
 * to it, and the rules that override it.
 */
struct CONSTRAINT_SECTION
{
    wxString                                             title;
    std::vector<CONSTRAINT_DEFAULT_FIELD>                defaults;
    std::optional<DRC_RULE_EDITOR_CONSTRAINT_NAME>       ruleType;
    std::function<std::optional<int>( const NETCLASS& )> netclassValue;

    /// The page that owns this section's defaults, linked to from the section.
    wxString settingsPage = wxEmptyString;
};


class PANEL_SETUP_CONSTRAINT_RULES : public wxPanel
{
public:
    enum class FAMILY
    {
        CLEARANCE,
        TRACKS_AND_VIAS,
        HOLES,
        SILKSCREEN,
        SOLDER_MASK,
        HIGH_SPEED,
        FOOTPRINTS
    };

    PANEL_SETUP_CONSTRAINT_RULES( wxWindow* aParent, PCB_EDIT_FRAME* aFrame, DRC_RULE_DOCUMENT* aDocument,
                                  FAMILY aFamily, size_t aSection );

    /**
     * The sections of one family, in the order they are shown in the tree.
     */
    static std::vector<CONSTRAINT_SECTION> SectionsFor( FAMILY aFamily );

    static bool RulesCanOverride( const wxString& aSetting, DRC_RULE_EDITOR_CONSTRAINT_NAME* aType, wxString* aPage );

    void ImportSettingsFrom( BOARD* aBoard );

    void RefreshFromDocument();

    bool TransferDataToWindow() override;

    /**
     * The rule form is always on screen, so its fields are empty until a rule is being written.
     * Their validators must not decide whether the page can be left, which is what the base
     * class would use them for. The form reports its own errors when it is applied.
     */
    bool Validate() override { return true; }

private:
    void buildSection( size_t aIndex );
    void updateNetclassLabels();
    void wrapLabels();
    void showPage( const wxString& aTitle );
    void updateDefaultLabels();

    PCB_EDIT_FRAME*        m_frame;
    BOARD_DESIGN_SETTINGS* m_settings;
    DRC_RULE_DOCUMENT*     m_document;
    wxScrolledWindow*      m_scrolled;

    std::vector<CONSTRAINT_SECTION>               m_sections;
    std::vector<CONSTRAINT_DEFAULT_FIELD>         m_fields;
    std::vector<wxStaticText*>                    m_defaultLabels;
    std::vector<std::optional<int>>               m_importedDefaults;
    std::vector<std::pair<size_t, wxStaticText*>> m_netclassLabels;
    std::vector<wxString>                         m_netclassTexts;
    wxStaticText*                                 m_note;
    wxString                                      m_noteText;
    /// One entry per section, null where the section has no rules of its own.
    std::vector<CONSTRAINT_RULES_LIST*> m_lists;
};

#endif // PANEL_SETUP_CONSTRAINT_RULES_H
