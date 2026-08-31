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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCB_DRILL_MAP_H
#define PCB_DRILL_MAP_H

#include <board_item.h>
#include <drill/drill_span.h>
#include <geometry/shape_poly_set.h>

#include <memory>
#include <mutex>

class BOARD;

namespace KIGFX
{
class VIEW;
}


/**
 * Turns on drill symbols at the holes, for one layer.
 *
 * This item is configuration, not artwork. The symbols themselves are drawn by the pads and
 * vias that own the holes, which keeps them in the view's spatial index and means a single
 * hole edit repaints one hole rather than the whole board. The map contributes only an
 * offset, applied to every mark, so the whole field can be slid off the outline the way a
 * fabrication drawing wants it without any mark losing track of its hole.
 */
class PCB_DRILL_MAP : public BOARD_ITEM
{
public:
    PCB_DRILL_MAP( BOARD_ITEM* aParent );

    /**
     * Spelled out because the outline cache's lock is not copyable. The cache itself is
     * derived from the board, so a copy simply starts empty.
     */
    PCB_DRILL_MAP( const PCB_DRILL_MAP& aOther );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_DRILL_MAP_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "PCB_DRILL_MAP" ); }

    EDA_ITEM* Clone() const override { return new PCB_DRILL_MAP( *this ); }

    /**
     * The map has no place of its own. It draws marks at the holes.
     *
     * Position is therefore an offset applied to every mark, and zero means the marks sit
     * exactly on their holes. Moving the map slides the whole set away from the board, which
     * is how a fabrication drawing puts the symbol field beside the outline rather than on
     * top of it.
     */
    VECTOR2I GetPosition() const override { return m_offset; }
    void SetPosition( const VECTOR2I& aPos ) override { m_offset = aPos; }

    void Move( const VECTOR2I& aMoveVector ) override { m_offset += aMoveVector; }

    const VECTOR2I& GetOffset() const { return m_offset; }
    void SetOffset( const VECTOR2I& aOffset ) { m_offset = aOffset; }

    int  GetOffsetX() const { return m_offset.x; }
    void SetOffsetX( int aX ) { m_offset.x = aX; }
    int  GetOffsetY() const { return m_offset.y; }
    void SetOffsetY( int aY ) { m_offset.y = aY; }

    int  GetSymbolSize() const { return m_symbolSize; }
    void SetSymbolSize( int aSize );
    int  GetSymbolExtent() const;

    /**
     * Configuration operations, deliberately not geometry transforms.
     */
    void Rotate( const VECTOR2I& aCentre, const EDA_ANGLE& aAngle ) override;
    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    bool GetOutlineSlots() const { return m_outlineSlots; }
    void SetOutlineSlots( bool aOn ) { m_outlineSlots = aOn; }

    bool GetGuideCross() const { return m_guideCross; }
    void SetGuideCross( bool aOn ) { m_guideCross = aOn; }

    const DRILL_SPAN& GetSpan() const { return m_span; }
    void SetSpan( const DRILL_SPAN& aSpan ) { m_span = aSpan; }

    bool GetAllSpans() const { return m_allSpans; }
    void SetAllSpans( bool aAll ) { m_allSpans = aAll; }

    /**
     * The span as an index into the board's spans, with -1 for every span.
     *
     * The properties panel wants one choice list rather than a flag and a struct, and the
     * enumeration follows the stackup, so an index only shifts under a stackup edit that has
     * invalidated the stored span anyway.
     */
    int  GetSpanChoice() const;
    void SetSpanChoice( int aIndex );

    const BOX2I GetBoundingBox() const override;

    /**
     * The board outline displaced by this map's offset.
     *
     * Cached against the board's outline generation, because the painter asks for it on every
     * repaint and rebuilding it costs a Simplify() over every Edge.Cuts item. Handed out as a
     * shared immutable snapshot rather than a reference, because hit testing runs on the UI
     * thread while painting runs on its own and a caller must not be reading an outline that
     * a later rebuild replaces.
     */
    std::shared_ptr<const SHAPE_POLY_SET> GetBoardOutlines() const;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    std::vector<int> ViewGetLayers() const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const BOARD_ITEM& aOther ) const override;

    void Serialize( google::protobuf::Any& aContainer ) const override;
    bool Deserialize( const google::protobuf::Any& aContainer ) override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    void swapData( BOARD_ITEM* aImage ) override;

private:
    VECTOR2I   m_offset;
    int        m_symbolSize;
    DRILL_SPAN m_span;
    bool       m_allSpans;
    bool       m_outlineSlots;
    bool       m_guideCross;

    mutable std::shared_ptr<const SHAPE_POLY_SET> m_outlineCache;
    mutable VECTOR2I                              m_outlineCacheOffset;
    mutable uint64_t                              m_outlineCacheGeneration;
    mutable std::mutex                            m_outlineCacheMutex;
};


/**
 * Repaint every drill map after an Edge.Cuts edit.
 *
 * A map draws the board outline displaced by its own offset, but it is not itself part of the
 * commit that changed the edge cuts, so nothing else tells the view its geometry has moved.
 */
void RefreshDrillMapOutlines( const BOARD& aBoard, KIGFX::VIEW* aView );

#endif // PCB_DRILL_MAP_H
