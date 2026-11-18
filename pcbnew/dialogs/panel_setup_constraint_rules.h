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
#include <memory>
#include <optional>
#include <utility>
#include <map>
#include <vector>
#include <wx/panel.h>
#include <wx/timer.h>

#include <board_design_settings.h>
#include <drc/drc_constraint_types.h>
#include <drc/rule_editor/drc_rule_editor_enums.h>
#include <widgets/unit_binder.h>

class wxScrolledWindow;
class wxStaticText;
class wxTextCtrl;
class BOARD;
class CONSTRAINT_RULES_LIST;
class DRC_RULE_DOCUMENT;
class FIELD_WARNING_ICON;
class NETCLASS;
class NET_SETTINGS;
class PCB_EDIT_FRAME;


/// What a board default measures, which decides how its field is read and written
enum class CONSTRAINT_VALUE
{
    /// A distance in board units, shown in the user's units
    LENGTH,

    /// A plain number, which no change of units rescales
    COUNT
};


// MSVC sizes a member pointer by what it knows of the class, so BOARD_DESIGN_SETTINGS must be
// complete here or this struct has a different layout in different files
struct CONSTRAINT_DEFAULT_FIELD
{
    wxString label;
    int BOARD_DESIGN_SETTINGS::* member;
    wxString                     settingName;


    CONSTRAINT_VALUE value = CONSTRAINT_VALUE::LENGTH;
};


/// What a net class value does to a constraint, which is not always raise its minimum
enum class NETCLASS_CONSTRAINT_PART
{
    /// Raises the minimum, as a net class clearance does
    MINIMUM,

    /// Asks for a value without raising the minimum, as a net class track width does
    OPTIMUM
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

    /// The group this section's defaults are imported with.
    wxString settingsPage = wxEmptyString;

    /// What a net class value does to this constraint, where a class can set one.
    NETCLASS_CONSTRAINT_PART netclassPart = NETCLASS_CONSTRAINT_PART::MINIMUM;
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
        FOOTPRINT_RULES
    };

    /**
     * @param aSettings is where this page reads and writes its defaults. Board Setup passes a copy
     *                  that reaches the board only when the dialog is accepted.
     */
    PANEL_SETUP_CONSTRAINT_RULES( wxWindow* aParent, PCB_EDIT_FRAME* aFrame, DRC_RULE_DOCUMENT* aDocument,
                                  BOARD_DESIGN_SETTINGS& aSettings, const CONSTRAINT_SECTION& aSection );

    /**
     * The sections of one family, in the order they are shown in the tree.
     */
    static std::vector<CONSTRAINT_SECTION> SectionsFor( FAMILY aFamily );

    /// Whether rules can set a value other than this board default, and the DRC constraint that does it
    static bool RulesCanOverride( const wxString& aSetting, DRC_CONSTRAINT_T* aConstraint );

    /// The title of the page that edits rules of this type, or empty when no page does
    static wxString PageTitleFor( DRC_RULE_EDITOR_CONSTRAINT_NAME aType );

    /// The net class values set for a constraint, grouped by value so equal classes list together.
    static std::map<int, std::vector<wxString>> NetclassValuesFor( const wxString&     aSetting,
                                                                   const NET_SETTINGS& aNetSettings );

    /// Opens one of this page's rules in its editor, when the page holds that rule type.
    void ShowRule( DRC_RULE_EDITOR_CONSTRAINT_NAME aType, int aIndex );

    /// The title of the page that edits a board default, or empty where no rule page edits one
    static wxString PageEditingDefault( const wxString& aSetting );

    /**
     * Puts a link to another page holding the same value at the field's right edge. The field
     * stays editable: the value can be set in either place.
     *
     * @param aSlot is where the link sits among the icons at the field's right edge.
     */
    static void LinkToPage( wxWindow* aField, const wxString& aPage, int aSlot = 1 );

    /**
     * Every board default a rule page edits, each with the group it is imported with.
     *
     * A default is written by the page that edits it, so an import has to reach the board
     * itself for a page that was never opened.
     */
    static std::vector<std::pair<wxString, CONSTRAINT_DEFAULT_FIELD>> EditedDefaults();

    /**
     * Reads the board defaults back into their fields, after something other than this page
     * changed them.
     */
    void RefreshDefaults();

    /**
     * Writes the defaults edited here to the staged settings, as leaving the page does, unless
     * one is out of range.
     *
     * @return true when a value was written.
     */
    bool StageEditedDefaults();

    /// The Import Settings group this page's defaults travel with.
    wxString ImportGroup() const;

    void RefreshFromDocument();

    /// The label of the button that adds or edits a rule on this page, when it is shown
    wxString GetSearchActionLabel() const;

    /**
     * Append what aSection shows whether or not its page is built: the board defaults in aSettings,
     * the net class values and the rules of aDocument at aRuleIndices.
     */
    static void AppendSearchTerms( const CONSTRAINT_SECTION& aSection, PCB_EDIT_FRAME* aFrame,
                                   const BOARD_DESIGN_SETTINGS& aSettings, const DRC_RULE_DOCUMENT& aDocument,
                                   const std::vector<int>& aRuleIndices, std::vector<wxString>& aTerms );

    bool TransferDataToWindow() override;

    /**
     * Called when the dialog is accepted. A rule left in the form has not reached the document,
     * so this is the last chance to add it.
     */
    bool TransferDataFromWindow() override;

    /**
     * The rule form is always on screen, so its fields are empty until a rule is being written.
     * Their validators must not decide whether the page can be left, which is what the base
     * class would use them for. The form reports its own errors when it is applied. What this
     * does check is whether a rule is waiting to be added, since leaving the page would drop it.
     */
    bool Validate() override;

private:
    /// The section whose board defaults include a setting, where one holds it
    static std::optional<CONSTRAINT_SECTION> sectionForSetting( const wxString& aSetting );

    void buildSection();
    void wrapLabels();
    void showPage( const wxString& aTitle );

    /// Writes the default fields to the board, or reports the first value out of range
    bool applyDefaults();

    /// The field of the first default out of range, with the reason in aMessage
    wxTextCtrl* firstInvalidDefault( wxString* aMessage ) const;

    /// Writes the fields changed since they were read, returning true when one was
    bool writeEditedDefaults();

    /// Copies the defaults this page edits into aSettings
    void writeDefaults( BOARD_DESIGN_SETTINGS& aSettings ) const;

    /// Shows a warning icon, with the reason as its tooltip, over each default whose value has an issue
    void updateWarnings();

    /**
     * Offers what to do with a rule the form holds. False while the offer stands, which is what
     * holds the page until it is answered.
     */
    bool confirmPendingRule();

    PCB_EDIT_FRAME*        m_frame;
    BOARD_DESIGN_SETTINGS* m_settings;
    DRC_RULE_DOCUMENT*     m_document;
    wxScrolledWindow*      m_scrolled;

    /// One board default, as this page shows it
    struct DEFAULT_FIELD
    {
        wxString settingName;
        int BOARD_DESIGN_SETTINGS::* member;
        wxTextCtrl*                  ctrl;
        FIELD_WARNING_ICON*          warning;
        std::unique_ptr<UNIT_BINDER> binder;


        /// The value as this page last read it, so an untouched field does not write back over
        /// an edit made to the same value on another page
        int loaded = 0;
    };

    CONSTRAINT_SECTION         m_section;
    std::vector<DEFAULT_FIELD> m_defaults;
    wxStaticText*              m_note;
    wxString                   m_noteText;
    wxStaticText*              m_defaultsNote;
    wxString                   m_defaultsNoteText;
    wxTimer                    m_validationTimer;
    /// Null until the section is built, and where a section carries no rules of its own.
    CONSTRAINT_RULES_LIST* m_list;
};

#endif // PANEL_SETUP_CONSTRAINT_RULES_H
