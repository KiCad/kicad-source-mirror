/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
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
#include <pcb_shape.h>
#include <rc_item.h>
#include <marker_base.h>

class DRC_ITEM;
class MSG_PANEL_ITEM;


class PCB_MARKER : public BOARD_ITEM, public MARKER_BASE
{
public:
    PCB_MARKER( std::shared_ptr<RC_ITEM> aItem, const VECTOR2I& aPos, int aLayer = F_Cu );

    ~PCB_MARKER();

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_MARKER_T == aItem->Type();
    }

    const KIID GetUUID() const override { return m_Uuid; }

    wxString SerializeToString() const;

    static PCB_MARKER* DeserializeFromString( const wxString& data );

    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_Pos += aMoveVector;
    }

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    VECTOR2I GetPosition() const override { return m_Pos; }
    void     SetPosition( const VECTOR2I& aPos ) override { m_Pos = aPos; }

    VECTOR2I GetCenter() const override
    {
        return GetPosition();
    }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override
    {
        if( GetMarkerType() == MARKER_RATSNEST )
            return false;
        else
            return HitTestMarker( aPosition, aAccuracy );
    }

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override
    {
        if( GetMarkerType() == MARKER_RATSNEST )
            return false;

        return HitTestMarker( aRect, aContained, aAccuracy );
    }

    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override
    {
        if( GetMarkerType() == MARKER_RATSNEST )
            return false;

        return HitTestMarker( aPoly, aContained );
    }

    EDA_ITEM* Clone() const override
    {
        return new PCB_MARKER( *this );
    }

    GAL_LAYER_ID GetColorLayer() const;

    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer,
                                              FLASHING aFlash = FLASHING::DEFAULT ) const override;

    void TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                  int aError, ERROR_LOC aErrorLoc, bool ignoreLineWidth ) const override;


    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override
    {
        return BOARD_ITEM::Matches( m_rcItem->GetErrorMessage( true ), aSearchData );
    }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    void SetZoom( double aZoomFactor ) const;

    const BOX2I ViewBBox() const override;

    const BOX2I GetBoundingBox() const override;

    std::vector<int> ViewGetLayers() const override;

    SEVERITY GetSeverity() const override;

    double Similarity( const BOARD_ITEM& aBoardItem ) const override
    {
        return 0.0;
    }

    bool operator==( const BOARD_ITEM& aBoardItem ) const override
    {
        return false;
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

    /** Get class name
     * @return  string "PCB_MARKER"
     */
    virtual wxString GetClass() const override
    {
        return wxT( "PCB_MARKER" );
    }

    std::vector<PCB_SHAPE> GetShapes() const;

    void SetPath( const std::vector<PCB_SHAPE>& aShapes, const VECTOR2I& aStart, const VECTOR2I& aEnd )
    {
        m_pathShapes = aShapes;
        m_pathStart = aStart;
        m_pathEnd = aEnd;
    }

protected:
    KIGFX::COLOR4D getColor() const override;

protected:
    std::vector<PCB_SHAPE> m_pathShapes; // Shown on LAYER_DRC_SHAPES
    VECTOR2I               m_pathStart;
    VECTOR2I               m_pathEnd;
    int                    m_pathLength;
};
