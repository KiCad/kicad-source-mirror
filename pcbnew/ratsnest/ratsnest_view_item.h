/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

/**
 * @brief Class that draws missing connections on a PCB.
 */

#ifndef RATSNEST_VIEW_ITEM_H
#define RATSNEST_VIEW_ITEM_H

#include <memory>
#include <eda_item.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <project/net_settings.h>

class GAL;
class CONNECTIVITY_DATA;


class RATSNEST_VIEW_ITEM : public EDA_ITEM
{
public:
    RATSNEST_VIEW_ITEM( std::shared_ptr<CONNECTIVITY_DATA> aData );

    /// @copydoc VIEW_ITEM::ViewBBox()
    const BOX2I ViewBBox() const override;

    /// @copydoc VIEW_ITEM::ViewDraw()
    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    std::vector<int> ViewGetLayers() const override;

    /**
     * Return true when a ratsnest line between @a aSource and @a aTarget can contribute to
     * @a aViewport.
     *
     * The ratsnest is a single entry in the #VIEW R-tree with an infinite bounding box, so the
     * view's own spatial culling stops at the item and never reaches the individual lines.  They
     * are tested here instead.
     *
     * @param aSource is one end of the line in world coordinates.
     * @param aTarget is the other end of the line in world coordinates.
     * @param aViewport is the world coordinate area being repainted.
     * @param aCurved is true when the line is drawn as a curve, which bulges off its chord.
     * @return true if the line has to be drawn.
     */
    static bool LineInViewport( const VECTOR2I& aSource, const VECTOR2I& aTarget,
                                const BOX2D& aViewport, bool aCurved );

    /**
     * Return the doubled Bezier control point used to bow a curved ratsnest line.
     */
    static VECTOR2D CurveControlPoint( const VECTOR2D& aSource, const VECTOR2D& aTarget );

    bool HitTest( const VECTOR2I& aPoint, int aAccuracy = 0 ) const override
    {
        return false;   // Not selectable
    }

#if defined(DEBUG)
    /// @copydoc EDA_ITEM::Show()
    void Show( int x, std::ostream& st ) const override { }
#endif

    virtual wxString GetClass() const override
    {
        return wxT( "RATSNEST_VIEW_ITEM" );
    }

protected:
    std::shared_ptr<CONNECTIVITY_DATA> m_data;      ///< Object containing ratsnest data.
};


#endif /* RATSNEST_VIEW_ITEM_H */
