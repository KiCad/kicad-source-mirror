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


#ifndef KICAD_PANEL_SETUP_SEVERITIES_H
#define KICAD_PANEL_SETUP_SEVERITIES_H

#include <map>
#include <wx/panel.h>

class PAGED_DIALOG;
class EDA_DRAW_FRAME;
class wxRadioBox;
class wxRadioButton;


class PANEL_SETUP_SEVERITIES : public wxPanel
{
public:
    /**
     * Create the severities setup panel.
     *
     * @param aItems is a list of error types that can have a severity.  Must have one or more!
     * @param aSeverities is a map of error code to severity
     * @param aPinMapSpecialCase is used to special-case the ERCE_PIN_TO_PIN_WARNING
     */
    PANEL_SETUP_SEVERITIES( wxWindow* aParentWindow,
                            std::vector<std::reference_wrapper<RC_ITEM>> aItems,
                            std::map<int, SEVERITY>& aSeverities,
                            RC_ITEM* aPinMapSpecialCase = nullptr );

    void ImportSettingsFrom( std::map<int, SEVERITY>& aSettings );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void checkReload();

private:
    std::map<int, SEVERITY>& m_severities;

    /// A list of item templates (to get descriptive text and error codes from)
    std::vector<std::reference_wrapper<RC_ITEM>> m_items;

    /// For ERC settings; a pointer to ERC_ITEM::pinTableConflict
    RC_ITEM* m_pinMapSpecialCase;

    std::map<int, wxRadioButton*[4]> m_buttonMap;   // map from DRC error code to button group

    std::map<int, SEVERITY> m_lastLoaded;
};

#endif // KICAD_PANEL_SETUP_SEVERITIES_H
