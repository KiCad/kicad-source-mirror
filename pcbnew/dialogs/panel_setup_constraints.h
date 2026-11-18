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


#ifndef PANEL_SETUP_CONSTRAINTS_H
#define PANEL_SETUP_CONSTRAINTS_H

#include <vector>

#include <wx/timer.h>

#include <board_design_settings.h>
#include <widgets/unit_binder.h>
#include <panel_setup_constraints_base.h>

class DRC_RULE_DOCUMENT;
class FIELD_WARNING_ICON;
class PAGED_DIALOG;
class PCB_EDIT_FRAME;
class wxCommandEvent;
class wxPopupTransientWindow;


class PANEL_SETUP_CONSTRAINTS : public PANEL_SETUP_CONSTRAINTS_BASE
{
public:
    /**
     * @param aSettings is where this page reads and writes its values. Board Setup passes a copy
     *                  that reaches the board only when the dialog is accepted.
     */
    PANEL_SETUP_CONSTRAINTS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame, DRC_RULE_DOCUMENT* aDocument,
                             BOARD_DESIGN_SETTINGS& aSettings );
    ~PANEL_SETUP_CONSTRAINTS( ) override { };

    /// Copies every value this page edits, which is what the Constraints import group carries
    static void CopySettings( const BOARD_DESIGN_SETTINGS& aFrom, BOARD_DESIGN_SETTINGS& aTo );

    /**
     * Reads the values a rule page edits back into their fields. This page only shows those, so
     * it has to be told when the page that edits one has written it.
     */
    void RefreshDefaults();

    /**
     * Writes the values a rule page also edits to the staged settings, as leaving the page does,
     * unless a value here is out of range.
     *
     * @return true when a value was written.
     */
    bool StageEditedDefaults();

    /**
     * Each field left untouched here whose staged value another page has changed, as the text it
     * shows paired with the text it will show once read again.
     */
    std::vector<std::pair<wxString, wxString>> StaleSearchValues() const;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void applyValues( BOARD_DESIGN_SETTINGS& aSettings ) const;

    /// The field of the first value out of range, with the reason in aMessage
    wxWindow* firstInvalidField( wxString* aMessage ) const;

    /// Writes the shared fields changed since they were read, returning true when one was
    bool writeEditedDefaults();

    /// Show a warning icon, with the reason as its tooltip, over each field whose value has an issue.
    void updateWarnings();

    /// Pop up what a board constraint is read with, each entry a link to where it is set.
    void showOverridingRules( const wxString& aSetting, wxWindow* aAnchor, bool aFromHover );

    /// Close a popup opened by hovering once the pointer has left both it and its icon.
    void closeHoveredPopup();

    struct CONSTRAINT_FIELD
    {
        wxString     m_Setting;
        wxWindow*    m_Ctrl;
        UNIT_BINDER* m_Binder = nullptr;
        int BOARD_DESIGN_SETTINGS::* m_Member = nullptr;
        FIELD_WARNING_ICON*          m_Warning = nullptr;

        /// Opens the list of what this value is read with, where anything else can set it
        FIELD_WARNING_ICON* m_Override = nullptr;

        /// The value as this page last read it, so an untouched field does not write back over
        /// an edit made to the same value on the page that also holds it
        int m_Loaded = 0;
    };

public:
    UNIT_BINDER             m_minClearance;
    UNIT_BINDER             m_minConn;
    UNIT_BINDER             m_trackMinWidth;
    UNIT_BINDER             m_viaMinAnnulus;
    UNIT_BINDER             m_viaMinSize;
    UNIT_BINDER             m_throughHoleMin;
    UNIT_BINDER             m_uviaMinSize;
    UNIT_BINDER             m_uviaMinDrill;
    UNIT_BINDER             m_holeToHoleMin;
    UNIT_BINDER             m_holeClearance;
    UNIT_BINDER             m_edgeClearance;
    UNIT_BINDER             m_silkClearance;
    UNIT_BINDER             m_minGrooveWidth;
    UNIT_BINDER             m_minTextHeight;
    UNIT_BINDER             m_minLineWidth;
    UNIT_BINDER             m_maxError;

private:
    PCB_EDIT_FRAME*               m_Frame;
    BOARD_DESIGN_SETTINGS*        m_BrdSettings;
    DRC_RULE_DOCUMENT*            m_document;

    /// The rule page that also holds the spoke count, a spin control rather than a bound field
    wxString                      m_spokeCountPage;

    /// The spoke count as this page last read it
    int                           m_spokeCountLoaded = 0;
    std::vector<CONSTRAINT_FIELD> m_fields;
    wxTimer                       m_validationTimer;

    /// The popup a hover or a click opened, while it is up
    wxPopupTransientWindow* m_popup = nullptr;

    /// The icon that popup belongs to, which the pointer may return to without closing it
    wxWindow* m_popupAnchor = nullptr;

    /// A popup a click opened stays until it is dismissed; one a hover opened follows the pointer
    bool m_popupFromHover = false;

    /// The field a hover is waiting to open, and the icon it would hang under
    wxString  m_hoverSetting;
    wxWindow* m_hoverAnchor = nullptr;

    wxTimer m_hoverTimer;
    wxTimer m_closeTimer;

    /// The field whose own help is held back while a popup covers it, and the text to give back
    wxWindow* m_mutedField = nullptr;
    wxString  m_mutedTip;
};

#endif //PANEL_SETUP_CONSTRAINTS_H
