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

#ifndef VIA_CROSS_SECTION_PREVIEW_H
#define VIA_CROSS_SECTION_PREVIEW_H

#include <wx/panel.h>
#include <widgets/via_cross_section_geometry.h>


/**
 * Draws a cross section of a via from its protection and machining settings.
 *
 * The shapes come from VIA_CROSS_SECTION_GEOMETRY. This panel only scales them to its size
 * and paints them, with a mixed value hatched.
 */
class VIA_CROSS_SECTION_PREVIEW : public wxPanel
{
public:
    using STATE = VIA_CROSS_SECTION_GEOMETRY::STATE;
    using POST_MACHINING = VIA_CROSS_SECTION_GEOMETRY::POST_MACHINING;

    VIA_CROSS_SECTION_PREVIEW( wxWindow* aParent, wxWindowID aId = wxID_ANY, const wxPoint& aPos = wxDefaultPosition,
                               const wxSize& aSize = wxDefaultSize, long aStyle = 0 );

    void SetState( const STATE& aState );

private:
    void onPaint( wxPaintEvent& aEvent );

    STATE m_state;
};

#endif
