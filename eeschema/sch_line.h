/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#ifndef _SCH_LINE_H_
#define _SCH_LINE_H_

#include <sch_item.h>
#include <wx/pen.h>     // for wxPenStyle
#include <list>         // for std::list
#include <geometry/seg.h>
#include <math/vector3.h>

class NETLIST_OBJECT_LIST;


/**
 * Segment description base class to describe items which have 2 end points (track, wire,
 * draw line ...)
 */
class SCH_LINE : public SCH_ITEM
{
public:
    static const enum wxPenStyle PenStyle[];

    SCH_LINE( const VECTOR2I& pos = VECTOR2I( 0, 0 ), int layer = LAYER_NOTES );

    SCH_LINE( const VECTOR2D& pos, int layer = LAYER_NOTES ) :
            SCH_LINE( VECTOR2I( pos.x, pos.y ), layer )
    {}

    SCH_LINE( const SCH_LINE& aLine );

    ~SCH_LINE() { }

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_LINE_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_LINE" );
    }

    wxString GetFriendlyName() const override;

    bool IsType( const std::vector<KICAD_T>& aScanTypes ) const override
    {
        if( SCH_ITEM::IsType( aScanTypes ) )
            return true;

        for( KICAD_T scanType : aScanTypes  )
        {
            if( scanType == SCH_ITEM_LOCATE_WIRE_T && m_layer == LAYER_WIRE )
                return true;

            if ( scanType == SCH_ITEM_LOCATE_BUS_T && m_layer == LAYER_BUS )
                return true;

            if ( scanType == SCH_ITEM_LOCATE_GRAPHIC_LINE_T && m_layer == LAYER_NOTES )
                return true;
        }

        return false;
    }

    bool IsEndPoint( const VECTOR2I& aPoint ) const override
    {
        return aPoint == m_start || aPoint == m_end;
    }

    int GetAngleFrom( const VECTOR2I& aPoint ) const;
    int GetReverseAngleFrom( const VECTOR2I& aPoint ) const;

    /**
     * Get the angle between the start and end lines.
     *
     * @return Line angle in radians.
     */
    inline EDA_ANGLE Angle() const
    {
        return ( EDA_ANGLE( (VECTOR2I) m_end - (VECTOR2I) m_start ) );
    }

    /**
     * Save the current line angle.
     *
     * Useful when dragging a line and its important to be able to restart the line from length
     * 0 in the correct direction.
     */
    inline void StoreAngle()
    {
        if( !IsNull() )
            m_storedAngle = Angle();
    }

    inline void StoreAngle( const EDA_ANGLE& aAngle ) { m_storedAngle = aAngle; }

    /**
     * Return the angle stored by StoreAngle().
     *
     * @return Stored angle in radians.
     */
    inline EDA_ANGLE GetStoredAngle() const { return m_storedAngle; }

    /**
     * Check if line is orthogonal (to the grid).
     *
     * @return True if orthogonal, false if not or the line is zero length.
     */
    inline bool IsOrthogonal() const { return Angle().IsCardinal(); }

    bool IsNull() const { return m_start == m_end; }

    VECTOR2I GetStartPoint() const { return m_start; }
    void     SetStartPoint( const VECTOR2I& aPosition ) { m_start = aPosition; }
    int      GetStartX() const { return m_start.x; }
    void     SetStartX( int aX ) { SetStartPoint( VECTOR2I( aX, m_start.y ) ); }
    int      GetStartY() const { return m_start.y; }
    void     SetStartY( int aY ) { SetStartPoint( VECTOR2I( m_start.x, aY ) ); }

    VECTOR2I GetMidPoint() const { return ( m_start + m_end ) / 2; }

    VECTOR2I GetEndPoint() const { return m_end; }
    void     SetEndPoint( const VECTOR2I& aPosition ) { m_end = aPosition; }
    int      GetEndX() const { return m_end.x; }
    void     SetEndX( int aX ) { SetEndPoint( VECTOR2I( aX, m_end.y ) ); }
    int      GetEndY() const { return m_end.y; }
    void     SetEndY( int aY ) { SetEndPoint( VECTOR2I( m_end.x, aY ) ); }

    /**
     * Get the geometric aspect of the wire as a SEG
     */
    SEG GetSeg() const
    {
        return SEG{ m_start, m_end };
    }

    void SetLastResolvedState( const SCH_ITEM* aItem ) override
    {
        const SCH_LINE* aLine = dynamic_cast<const SCH_LINE*>( aItem );

        if( aLine )
        {
            m_stroke = aLine->GetStroke();
            m_lastResolvedLineStyle = aLine->m_lastResolvedLineStyle;
            m_lastResolvedWidth = aLine->m_lastResolvedWidth;
            m_lastResolvedColor = aLine->m_lastResolvedColor;
        }
    }

    void       SetLineStyle( const LINE_STYLE aStyle );
    LINE_STYLE GetLineStyle() const;

    /// @return the style that the line should be drawn in
    /// this might be set on the line or inherited from the line's netclass
    LINE_STYLE GetEffectiveLineStyle() const;

    // Special Getter/Setters for properties panel.  Required because it uses #WIRE_STYLE instead
    // of #LINE_STYLE.  (The two enums are identical, but we expose "default" in the #WIRE_STYLE
    // property while we don't with the LINE_STYLE property.)
    void       SetWireStyle( const WIRE_STYLE aStyle ) { SetLineStyle( (LINE_STYLE) aStyle ); }
    WIRE_STYLE GetWireStyle() const { return (WIRE_STYLE) GetLineStyle(); }


    void SetLineColor( const COLOR4D& aColor );

    void SetLineColor( const double r, const double g, const double b, const double a );

    /// Return #COLOR4D::UNSPECIFIED if a custom color hasn't been set for this line.
    COLOR4D GetLineColor() const;

    void SetLineWidth( const int aSize );
    int GetLineWidth() const { return m_stroke.GetWidth(); }

    virtual bool HasLineStroke() const override { return true; }
    virtual STROKE_PARAMS GetStroke() const override { return m_stroke; }
    virtual void SetStroke( const STROKE_PARAMS& aStroke ) override { m_stroke = aStroke; }

    bool IsStrokeEquivalent( const SCH_LINE* aLine )
    {
        if( m_stroke.GetWidth() != aLine->GetStroke().GetWidth() )
            return false;

        if( m_stroke.GetColor() != aLine->GetStroke().GetColor() )
            return false;

        LINE_STYLE style_a = m_stroke.GetLineStyle();
        LINE_STYLE style_b = aLine->GetStroke().GetLineStyle();

        return style_a == style_b
               || ( style_a == LINE_STYLE::DEFAULT && style_b == LINE_STYLE::SOLID )
               || ( style_a == LINE_STYLE::SOLID   && style_b == LINE_STYLE::DEFAULT );
    }

    std::vector<int> ViewGetLayers() const override;

    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

    const BOX2I GetBoundingBox() const override;

    /**
     * @return The length of the line segment.
     */
    double GetLength() const;
    void   SetLength( double aLength );

    int GetPenWidth() const override;

    void Move( const VECTOR2I& aMoveVector ) override;
    void MoveStart( const VECTOR2I& aMoveVector );
    void MoveEnd( const VECTOR2I& aMoveVector );

    void MirrorVertically( int aCenter ) override;
    void MirrorHorizontally( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    /**
     * Check line against \a aLine to see if it overlaps and merge if it does.
     *
     * This method will return an equivalent of the union of line and \a aLine if the
     * two lines overlap.  This method is used to merge multiple line segments into a single
     * line.
     *
     * @param aScreen is the current screen.
     * @param aLine is the line to compare.
     * @param aCheckJunctions is used to indicate if we need to check for a junction if the two
     *                        segments are colinear and touch.
     * @return New line that combines the two or NULL on non-overlapping segments.
     */
    SCH_LINE* MergeOverlap( SCH_SCREEN* aScreen, SCH_LINE* aLine, bool aCheckJunctions );

    /**
     * Break this segment into two at the specified point.
     *
     * @note No checks are made to verify if aPoint is contained within the segment. That is
     * the responsibility of the caller.
     *
     * @note It is the responsibility of the caller to add the newly created segment
     * to the screen.
     *
     * @param aPoint Point at which to break the segment
     * @return The newly created segment.
     */
    SCH_LINE* BreakAt( SCH_COMMIT* aCommit, const VECTOR2I& aPoint );

    /**
     * This version should only be used when importing files.  It cannot handle breaking wires
     * that are part of groups (as it has no commit to modify the parent group within).
     */
    SCH_LINE* NonGroupAware_BreakAt( const VECTOR2I& aPoint );

    bool IsParallel( const SCH_LINE* aLine ) const;

    /**
     * For wires only:
     * @return true if a wire can accept a hop over arc shape
     * (when 2 wires are crossing, only one must accept the hop over arc)
     */
    bool ShouldHopOver( const SCH_LINE* aLine ) const;

    /**
     * For wires only: build the list of points to draw the shape using segments and 180 deg arcs
     * Points are VECTOR3D, with Z coord used as flag:
     * for segments: start point and end point have the Z coord = 0
     * for arcs (hop over): start point middle point and end point have the Z coord = 1
     * @return the list of points
     * @param aScreen is the current screen to draw/plot
     * @param aArcRadius is the radius of the hop over arc
     */
    std::vector<VECTOR3I> BuildWireWithHopShape( const SCH_SCREEN* aScreen, double aArcRadius ) const;

    void GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                              std::vector<DANGLING_END_ITEM>& aItemListByPos,
                              const SCH_SHEET_PATH*           aPath = nullptr ) override;

    bool IsStartDangling() const { return m_startIsDangling; }
    bool IsEndDangling() const { return m_endIsDangling; }
    bool IsDangling() const override { return m_startIsDangling || m_endIsDangling; }

    bool IsConnectable() const override;

    bool HasConnectivityChanges( const SCH_ITEM* aItem,
                                 const SCH_SHEET_PATH* aInstance = nullptr ) const override;

    std::vector<VECTOR2I> GetConnectionPoints() const override;

    bool ConnectionPropagatesTo( const EDA_ITEM* aItem ) const override;

    void GetSelectedPoints( std::vector<VECTOR2I>& aPoints ) const;

    bool CanConnect( const SCH_ITEM* aItem ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    bool operator <( const SCH_ITEM& aItem ) const override;

    VECTOR2I GetPosition() const override { return m_start; }
    void     SetPosition( const VECTOR2I& aPosition ) override;
    VECTOR2I GetSortPosition() const override { return GetMidPoint(); }

    bool IsPointClickableAnchor( const VECTOR2I& aPos ) const override
    {
        return ( GetStartPoint() == aPos && IsStartDangling() )
               || ( GetEndPoint() == aPos && IsEndDangling() );
    }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    EDA_ITEM* Clone() const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    const wxString& GetOperatingPoint() const { return m_operatingPoint; }
    void SetOperatingPoint( const wxString& aText ) { m_operatingPoint = aText; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    /**
     * Return if the line is a graphic (non electrical line)
     *
     * Currently, anything on the internal NOTES layer is a graphic line
     */
    bool IsGraphicLine() const;

    /**
     * Return true if the line is a wire.
     *
     * @return true if this line is on the wire layer.
     */
    bool IsWire() const;

    /**
     * Return true if the line is a bus.
     *
     * @return true if this line is on the bus layer.
     */
    bool IsBus() const;

    double Similarity( const SCH_ITEM& aOther ) const override;

    bool operator==( const SCH_ITEM& aOther ) const override;

protected:
    void swapData( SCH_ITEM* aItem ) override;

private:
    bool doIsConnected( const VECTOR2I& aPosition ) const override;

private:
    bool               m_startIsDangling;  ///< True if start point is not connected.
    bool               m_endIsDangling;    ///< True if end point is not connected.
    VECTOR2I           m_start;            ///< Line start point
    VECTOR2I           m_end;              ///< Line end point
    EDA_ANGLE          m_storedAngle;      ///< Stored angle
    STROKE_PARAMS      m_stroke;           ///< Line stroke properties.

    // If real-time connectivity gets disabled (due to being too slow on a particular
    // design), we can no longer rely on getting the NetClass to find netclass-specific
    // linestyles, linewidths and colors.
    mutable LINE_STYLE m_lastResolvedLineStyle;
    mutable int        m_lastResolvedWidth;
    mutable COLOR4D    m_lastResolvedColor;

    wxString           m_operatingPoint;
};


#ifndef SWIG
DECLARE_ENUM_TO_WXANY( WIRE_STYLE );
#endif


#endif    // _SCH_LINE_H_
