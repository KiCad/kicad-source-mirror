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
#pragma once

#include <variant>
#include <vector>

#include <eda_item.h>
#include <gal/color4d.h>

#include <geometry/circle.h>
#include <geometry/half_line.h>
#include <geometry/line.h>
#include <geometry/seg.h>
#include <geometry/shape_arc.h>

namespace KIGFX
{

/**
 * Shows construction geometry for things like line extensions, arc centers, etc.
 */
class CONSTRUCTION_GEOM : public EDA_ITEM
{
public:
    // Supported items
    using DRAWABLE = std::variant<SEG, LINE, HALF_LINE, CIRCLE, SHAPE_ARC, VECTOR2I>;

    struct SNAP_GUIDE
    {
        SEG      Segment;
        COLOR4D  Color;
        int      LineWidth;
    };

    CONSTRUCTION_GEOM();

    wxString GetClass() const override { return wxT( "CONSTRUCTION_GEOM" ); }

    const BOX2I ViewBBox() const override;

    void ViewDraw( int aLayer, VIEW* aView ) const override;

    std::vector<int> ViewGetLayers() const override;

    void SetColor( const COLOR4D& aColor ) { m_color = aColor; }
    void SetPersistentColor( const COLOR4D& aColor ) { m_persistentColor = aColor; }

    void AddDrawable( const DRAWABLE& aItem, bool aIsPersistent, int aLineWidth = 1 );
    void SetSnapGuides( std::vector<SNAP_GUIDE> aGuides );
    void ClearDrawables();

    void SetSnapLine( const SEG& aLine ) { m_snapLine = aLine; }
    void ClearSnapLine() { m_snapLine.reset(); }

private:
    COLOR4D m_color;
    COLOR4D m_persistentColor;
    struct DRAWABLE_INFO
    {
        DRAWABLE Item;
        bool     IsPersistent;
        int      LineWidth;
    };

    // The items to draw
    std::vector<DRAWABLE_INFO> m_drawables;
    std::vector<SNAP_GUIDE>    m_snapGuides;

    // The snap line to draw
    std::optional<SEG> m_snapLine;
};

} // namespace KIGFX