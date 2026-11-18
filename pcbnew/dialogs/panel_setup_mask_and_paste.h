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

#include <board.h>
#include <widgets/unit_binder.h>
#include <widgets/margin_offset_binder.h>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_setup_mask_and_paste_base.h>

class PCB_EDIT_FRAME;
class BOARD_DESIGN_SETTINGS;


class PANEL_SETUP_MASK_AND_PASTE : public PANEL_SETUP_MASK_AND_PASTE_BASE
{
public:
    /**
     * @param aSettings is where this page reads and writes its values. Board Setup passes a copy
     *                  that reaches the board only when the dialog is accepted.
     */
    PANEL_SETUP_MASK_AND_PASTE( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame, BOARD_DESIGN_SETTINGS& aSettings );
    ~PANEL_SETUP_MASK_AND_PASTE( ) { };

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /// Copies every value this page edits, which is what the Solder Mask/Paste import group carries
    static void CopySettings( const BOARD_DESIGN_SETTINGS& aFrom, BOARD_DESIGN_SETTINGS& aTo );

    /**
     * Reads the values a rule page sets back into their fields. This page only shows those, so
     * it has to be told when the page that sets one has written it.
     */
    void RefreshDefaults();

    /**
     * Writes the values a rule page also sets to the staged settings, as leaving the page does,
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
    /// The field of the first value out of range, with the reason in aMessage
    wxWindow* firstInvalidField( wxString* aMessage ) const;

    /// Writes the shared fields changed since they were read, returning true when one was
    bool writeEditedDefaults();

    PCB_EDIT_FRAME*         m_Frame;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

    UNIT_BINDER             m_maskExpansion;
    UNIT_BINDER             m_maskMinWidth;
    UNIT_BINDER             m_maskToCopperClearance;
    MARGIN_OFFSET_BINDER    m_pasteMargin;

    /// The rule pages that also hold these values, where they do
    wxString m_maskExpansionPage;
    wxString m_maskMinWidthPage;
    wxString m_pasteMarginPage;

    /// The values as this page last read them, so an untouched field does not write back over
    /// an edit made on the page that also holds it
    int m_maskExpansionLoaded = 0;
    int m_maskMinWidthLoaded = 0;
    int m_pasteMarginLoaded = 0;
};


