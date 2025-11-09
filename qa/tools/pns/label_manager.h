/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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


#ifndef __LABEL_MANAGER_H
#define __LABEL_MANAGER_H

#include <wx/wx.h>

#include <gal/graphics_abstraction_layer.h>
#include <gal/color4d.h>
#include <math/vector2d.h>
#include <math/box2.h>
#include <view/view_overlay.h>
#include <geometry/shape_line_chain.h>

class LABEL_MANAGER
{
public:
    struct LABEL
    {
        KIGFX::COLOR4D m_color;
        wxString       m_msg;
        VECTOR2I       m_target;
        BOX2I          m_bbox;
    };

    LABEL_MANAGER( KIGFX::GAL* aGal );
    ~LABEL_MANAGER();

    void Add( VECTOR2I target, wxString msg, KIGFX::COLOR4D color );
    void Add( const SHAPE_LINE_CHAIN& aL, KIGFX::COLOR4D color );
    void Redraw( KIGFX::VIEW_OVERLAY* aOvl );

private:
    VECTOR2I nearestBoxCorner( BOX2I b, VECTOR2I p );
    VECTOR2I boxMtv( BOX2I b1, BOX2I b2 );
    void     recalculate();

    KIGFX::GAL*        m_canvas;
    int                m_textSize = 100000;
    std::vector<LABEL> m_labels;
};

#endif
