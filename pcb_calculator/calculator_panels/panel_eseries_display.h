/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_ESERIES_DISPLAY_H
#define PANEL_ESERIES_DISPLAY_H

#include <eseries.h>
#include "panel_eseries_display_base.h"

class PCB_CALCULATOR_SETTINGS;


class PANEL_ESERIES_DISPLAY : public PANEL_ESERIES_DISPLAY_BASE
{
public:
    PANEL_ESERIES_DISPLAY( wxWindow* parent, wxWindowID id = wxID_ANY,
                           const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL,
                           const wxString& name = wxEmptyString );
    ~PANEL_ESERIES_DISPLAY();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

private:
    /*! \brief Recalculate colours used to highlight the E-series columns.
     *
     * Each colour is a pair of colours which are used in alternate rows
     * to make the table easier to follow.
     */
    void recalculateColumnColours( bool aDarkModeOn );

    /*! \brief Fill small E-series tree with values.
     *
     * Contains values from E1, E3, E6 and E12 series.
     */
    void populateE112Tree();

    /*! \brief Fill larger E-series tree with values.
     *
     * Contains values from E24, E48, E96 series.
     */
    void populateE2496Tree();

    /*! \brief Colour small E-series tree according to current theme.
     *
     * Contains values from E1, E3, E6 and E12 series.
     */
    void recolourE112Tree();

    /*! \brief Colour large E-series tree according to current theme.
     *
     * Contains values from E24, E48, E96 series.
     */
    void recolourE2496Tree();

    /*! \brief Adjustment factor to create alternating R-series table entry colours.
     *
     * This is to make the table easier to read.
     * This value is passed to wxColour::ChangeLightness().
     */
    constexpr static int s_altAdjustValue = 125;

    /*! \brief Adjustment factor to create darker grid cells in dark theme.
     *
     * Without this the light numbers on the grid backgrounds are difficult to read.
     * This value is passed to wxColour::ChangeLightness().
     */
    constexpr static int s_darkAdjustValue = 78;

    /*! \brief Colour for E1 column in light theme. Passed to wxColour constructor.
     *
     * HTML honeydew
     */
    constexpr static uint32_t s_cE1BGR = 0xf0fff0;

    /*! \brief Colour for E3 column in light theme. Passed to wxColour constructor.
     *
     * HTML palegreen
     */
    constexpr static uint32_t s_cE3BGR = 0x98fb98;

    /*! \brief Colour for E6 column in light theme. Passed to wxColour constructor.
     *
     * HTML cornflowerblue
     */
    constexpr static uint32_t s_cE6BGR = 0xed9564;

    /*! \brief Colour for E12 column in light theme. Passed to wxColour constructor.
     *
     * HTML plum
     */
    constexpr static uint32_t s_cE12BGR = 0xdda0dd;

    /*! \brief Colour for E24 column in light theme. Passed to wxColour constructor.
     *
     * HTML skyblue
     */
    constexpr static uint32_t s_cE24BGR = 0xebce87;

    /*! \brief Colour for E48 column in light theme. Passed to wxColour constructor.
     *
     * HTML olivedrab
     */
    constexpr static uint32_t s_cE48BGR = 0x23e86b;

    /*! \brief Colour for E96 column in light theme. Passed to wxColour constructor.
     *
     * HTML lightsalmon
     */
    constexpr static uint32_t s_cE96BGR = 0x7aa0ff;

    /*! \brief Calculated colour for E1 column in current (light,dark) theme. */
    wxColour m_colourE1Column;

    /*! \brief Calculated colours for E3 column in current (light,dark) theme. */
    wxColour m_colourE3Pair[2];

    /*! \brief Calculated colours for E6 column in current (light,dark) theme. */
    wxColour m_colourE6Pair[2];

    /*! \brief Calculated colours for E12 column in current (light,dark) theme. */
    wxColour m_colourE12Pair[2];

    /*! \brief Calculated colours for E24 column in current (light,dark) theme. */
    wxColour m_colourE24Pair[2];

    /*! \brief Calculated colours for E48 column in current (light,dark) theme. */
    wxColour m_colourE48Pair[2];

    /*! \brief Calculated colours for E96 column in current (light,dark) theme. */
    wxColour m_colourE96Pair[2];

    /*! \brief Calculated matching colour for empty columns. Same as background of labels */
    wxColour s_colourMatching;
};

#endif
