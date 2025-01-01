/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef WX_PANEL_H
#define WX_PANEL_H

#include <wx/panel.h>
#include <gal/color4d.h>

class WX_PANEL : public wxPanel
{
public:
    WX_PANEL( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL,
              const wxString& name = wxEmptyString );

    ~WX_PANEL();

    void SetBorders( bool aLeft, bool aRight, bool aTop, bool aBottom )
    {
        m_leftBorder = aLeft;
        m_rightBorder = aRight;
        m_topBorder = aTop;
        m_bottomBorder = aBottom;
    }

    void SetBorderColor( const KIGFX::COLOR4D& aColor )
    {
        m_borderColor = aColor;
    }

private:
    void OnPaint( wxPaintEvent& event );

private:
    bool   m_leftBorder;
    bool   m_rightBorder;
    bool   m_topBorder;
    bool   m_bottomBorder;

    KIGFX::COLOR4D m_borderColor;
};


#endif //WX_PANEL_H
