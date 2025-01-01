/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#ifndef NUMBER_BADGE_H_
#define NUMBER_BADGE_H_

#include <widgets/ui_common.h>
#include <wx/dcclient.h>
#include <wx/panel.h>
#include <kicommon.h>


/**
 * A simple UI element that puts a number on top of a colored rounded rectangle with a fill
 * color that shows the severity of the reports the number is counting (e.g. green, yellow, red).
 * This badge will also automatically truncate the displayed number to the set maximum and display
 * "+" at the end to represent it is truncated.
 */
class KICOMMON_API NUMBER_BADGE : public wxPanel
{
public:
    /**
     * Create a number badge with 10pt font and a maximum number of 1000.
     */
    NUMBER_BADGE( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos,
                  const wxSize& aSize, int aStyles );

    /**
     * Update the number displayed on the badge.
     *
     * Severity to badge color mapping:
     *  - RPT_SEVERITY_ERROR = red badge
     *  - RPT_SEVERITY_WARNING = yellow badge
     *  - RPT_SEVERITY_ACTION = green badge
     *  - RPT_SEVERITY_EXCLUSION = light grey badge
     *  - RPT_SEVERITY_INFO = light grey badge
     *
     * @param aNumber is the new number to display.
     * @param aSeverity is the new severity of the badge.
     */
    void UpdateNumber( int aNumber, SEVERITY aSeverity );

    /**
     * Set the maximum number to be shown on the badge. Any numbers greater than this
     * will be displayed as the maximum number followed by "+".
     *
     * @param aMax is the maximum number
     */
    void SetMaximumNumber( int aMax );

    /**
     * Set the text size to use on the badge.
     *
     * @param aSize is the text size (in pt) to use on the badge
     */
    void SetTextSize( int aSize );

protected:
    /**
     * Helper function to compute the size of the badge
     */
    void computeSize();

    /**
     * Handler that actually paints the badge and the text.
     */
    void onPaint( wxPaintEvent& aEvt );

    int      m_textSize;        // The text size to use
    int      m_maxNumber;       // The maximum number allowed to be shown on the badge

    int      m_currentNumber;   // The current number to display
    bool     m_showBadge;       // If true, displays the actual badge otherwise it is invisible
    wxColour m_badgeColour;     // The color of the badge
    wxColour m_textColour;      // The color of the text on the badge
};

#endif
