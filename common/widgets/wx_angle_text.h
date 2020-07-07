/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef _WX_ANGLE_TEXT_
#define _WX_ANGLE_TEXT_

#include <eda_rect.h>
#include <wx/panel.h>



class WX_ANGLE_TEXT : public wxPanel
{
public:
    WX_ANGLE_TEXT( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                   const wxPoint& aPos, double aAngle );

    /**
     * Get the bounding box that this angled text will take up on a certain window.
     *
     * @param aWindow is the wxWindow the text will go on
     * @param aLabel is the text string
     * @param aAngle is the angle of the text
     */
    static EDA_RECT GetBoundingBox( wxWindow* aWindow, const wxString& aLabel, double aAngle );

protected:
    void OnEraseBackground( wxEraseEvent& WXUNUSED( aEvent ) );
    void OnPaint( wxPaintEvent& WXUNUSED( aEvent ) );

    wxString m_label;
    double   m_angle;
};

#endif /*_WX_ANGLE_TEXT_*/
