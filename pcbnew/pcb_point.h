/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <board_item.h>


/**
 * A PCB_POINT is a 0-dimensional point that is used to mark a position on a PCB,
 * or more usually a footprint for some purpose. This can be, for example:
 *
 * * As a defined snap anchor for component alignment
 * * As a routing snap point in a custom pad
 *
 * For now, the point has only location, and a size. The size is the size of
 * the displayed point in board space. Having the points a constant size in screen
 * space regardless of zoom seems plasible, but it means at low zoom, the board will
 * be full of "huge" markers. So allow users to set how big they are as needed.
 */
class PCB_POINT : public BOARD_ITEM
{
public:

    PCB_POINT( BOARD_ITEM* aParent );

    /**
     * Construct a point at the given location.
     */
    PCB_POINT( BOARD_ITEM* aParent, const VECTOR2I& aPos, const int aSize);

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_POINT_T == aItem->Type();
    }

    void     SetPosition( const VECTOR2I& aPos ) override { m_pos = aPos; }
    VECTOR2I GetPosition() const override { return m_pos; }

    void SetSize( const int aSize ) { m_size = aSize; }
    int  GetSize() const { return m_size; }

    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_pos += aMoveVector;
    }

    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;
    std::vector<int> ViewGetLayers() const override;

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    wxString GetClass() const override
    {
        return wxT( "PCB_POINT" );
    }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    // Virtual function
    const BOX2I GetBoundingBox() const override;

    std::shared_ptr<SHAPE>
    GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash = FLASHING::DEFAULT ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * Convert the shape to a closed polygon.
     *
     * Used in filling zones calculations.  Circles and arcs are approximated by segments.
     *
     * @param aBuffer is a buffer to store the polygon.
     * @param aClearance is the clearance around the pad.
     * @param aError is the maximum deviation from a true arc.
     * @param aErrorLoc whether any approximation error should be placed inside or outside
     * @param ignoreLineWidth is used for edge cut items where the line width is only for
     *                        visualization
     */
    void TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                  int aError, ERROR_LOC aErrorLoc,
                                  bool ignoreLineWidth = false ) const override;

    double Similarity( const BOARD_ITEM& aBoardItem ) const override;

    bool operator==( const PCB_POINT& aOther ) const;
    bool operator==( const BOARD_ITEM& aBoardItem ) const override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

private:
    // Center of the point
    VECTOR2I m_pos;
    // Visual size of the point in board space
    int m_size;
};
