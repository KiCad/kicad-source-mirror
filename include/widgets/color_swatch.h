/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef COLOR_SWATCH__H_
#define COLOR_SWATCH__H_

#include <wx/wx.h>

#include <common.h>

#include <gal/color4d.h>


/**
 * Class representing a simple color swatch, of the kind used to
 * set layer colors
 */
class COLOR_SWATCH: public wxPanel
{
public:

    /**
     * Construct a COLOR_SWATCH
     *
     * @param aParent parent window
     * @param aColor initial swatch color
     * @param aID id to use when sending swatch events
     */
    COLOR_SWATCH( wxWindow* aParent, KIGFX::COLOR4D aColor, int aID,
                  bool aArbitraryColors );

    /**
     * Set the current swatch color directly.
     */
    void SetSwatchColor( KIGFX::COLOR4D aColor, bool sendEvent );

    /**
     * @return the current swatch color
     */
    KIGFX::COLOR4D GetSwatchColor() const;

    /**
     * Prompt for a new colour, using the colour picker dialog.
     *
     * A colour change event will be sent if it's set.
     */
    void GetNewSwatchColor();

private:

    /**
     * Pass unwanted events on to listeners of this object
     */
    void rePostEvent( wxEvent& aEvt );

    ///> Can the swatch have any color, or only preset ones?
    bool m_arbitraryColors;

    ///> The current colour of the swatch
    KIGFX::COLOR4D m_color;

    ///> Handle of the actual swatch shown
    wxStaticBitmap* m_swatch;
};


/**
 * Event signalling a swatch has changed color
 */
wxDECLARE_EVENT(COLOR_SWATCH_CHANGED, wxCommandEvent);

#endif // COLOR_SWATCH__H_
