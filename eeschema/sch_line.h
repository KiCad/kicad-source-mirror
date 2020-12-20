/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

class NETLIST_OBJECT_LIST;


/**
 * Segment description base class to describe items which have 2 end points (track, wire,
 * draw line ...)
 */
class SCH_LINE : public SCH_ITEM
{
    bool    m_startIsDangling;  ///< True if start point is not connected.
    bool    m_endIsDangling;    ///< True if end point is not connected.
    wxPoint m_start;            ///< Line start point
    wxPoint m_end;              ///< Line end point
    STROKE_PARAMS m_stroke;     ///< Line stroke properties.

public:

    static const enum wxPenStyle PenStyle[];

    SCH_LINE( const wxPoint& pos = wxPoint( 0, 0 ), int layer = LAYER_NOTES );

    SCH_LINE( const VECTOR2D& pos, int layer = LAYER_NOTES ) :
        SCH_LINE( wxPoint( pos.x, pos.y ), layer )
    {}

    SCH_LINE( const SCH_LINE& aLine );

    ~SCH_LINE() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_LINE_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_LINE" );
    }

    bool IsType( const KICAD_T aScanTypes[] ) const override
    {
        if( SCH_ITEM::IsType( aScanTypes ) )
            return true;

        for( const KICAD_T* p = aScanTypes; *p != EOT; ++p )
        {
            if( *p == SCH_LINE_LOCATE_WIRE_T && m_layer == LAYER_WIRE )
                return true;
            else if ( *p == SCH_LINE_LOCATE_BUS_T && m_layer == LAYER_BUS )
                return true;
            else if ( *p == SCH_LINE_LOCATE_GRAPHIC_LINE_T && m_layer == LAYER_NOTES )
                return true;
        }

        return false;
    }

    bool IsEndPoint( const wxPoint& aPoint ) const
    {
        return aPoint == m_start || aPoint == m_end;
    }

    bool IsNull() const { return m_start == m_end; }

    wxPoint GetStartPoint() const { return m_start; }
    void SetStartPoint( const wxPoint& aPosition ) { m_start = aPosition; }

    wxPoint GetEndPoint() const { return m_end; }
    void SetEndPoint( const wxPoint& aPosition ) { m_end = aPosition; }

    PLOT_DASH_TYPE GetDefaultStyle() const;

    void           SetLineStyle( const PLOT_DASH_TYPE aStyle );
    void           SetLineStyle( const int aStyleId );
    PLOT_DASH_TYPE GetLineStyle() const;

    /// @return the style that the line should be drawn in
    /// this might be set on the line or inherited from the line's netclass
    PLOT_DASH_TYPE GetEffectiveLineStyle() const;

    /// @return the style name from the style id
    /// (mainly to write it in .sch file)
    static const char* GetLineStyleName( PLOT_DASH_TYPE aStyle );

    /// @return the style id from the style  name
    /// (mainly to read style from .sch file)
    static PLOT_DASH_TYPE GetLineStyleByName( const wxString& aStyleName );

    void SetLineColor( const COLOR4D& aColor );

    void SetLineColor( const double r, const double g, const double b, const double a );

    /// Returns COLOR4D::UNSPECIFIED if a custom color hasn't been set for this line
    COLOR4D GetLineColor() const;

    void SetLineWidth( const int aSize );

    virtual bool HasLineStroke() const override { return true; }
    virtual STROKE_PARAMS GetStroke() const override { return m_stroke; }
    virtual void SetStroke( const STROKE_PARAMS& aStroke ) override { m_stroke = aStroke; }

    bool IsStrokeEquivalent( const SCH_LINE* aLine )
    {
        if( m_stroke.GetWidth() != aLine->GetStroke().GetWidth() )
            return false;

        if( m_stroke.GetColor() != aLine->GetStroke().GetColor() )
            return false;

        PLOT_DASH_TYPE style_a = m_stroke.GetPlotStyle();
        PLOT_DASH_TYPE style_b = aLine->GetStroke().GetPlotStyle();

        return style_a == style_b
               || ( style_a == PLOT_DASH_TYPE::DEFAULT && style_b == PLOT_DASH_TYPE::SOLID )
               || ( style_a == PLOT_DASH_TYPE::SOLID   && style_b == PLOT_DASH_TYPE::DEFAULT );
    }

    /**
     * Test if the #SCH_LINE object uses the default stroke settings.
     *
     * The stroke settings include the line width, style, and color.
     *
     * @return True if the #SCH_LINE object uses the default stroke settings.
     */
    bool UsesDefaultStroke() const;

    int GetLineSize() const { return m_stroke.GetWidth(); }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    const EDA_RECT GetBoundingBox() const override;

    /**
     * @return The length of the line segment.
     */
    double GetLength() const;

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    int GetPenWidth() const override;

    void Move( const wxPoint& aMoveVector ) override;
    void MoveStart( const wxPoint& aMoveVector );
    void MoveEnd( const wxPoint& aMoveVector );

    void MirrorX( int aXaxis_position ) override;
    void MirrorY( int aYaxis_position ) override;
    void Rotate( wxPoint aPosition ) override;
    void RotateStart( wxPoint aPosition );
    void RotateEnd( wxPoint aPosition );

    /**
     * Check line against \a aLine to see if it overlaps and merge if it does.
     *
     * This method will return an equivalent of the union of line and \a aLine if the
     * two lines overlap.  This method is used to merge multiple line segments into a single
     * line.
     *
     * @param aScreen - the current screen
     * @param aLine - Line to compare.
     * @param aCheckJunctions - indicates we need to check for a junction if the two segments
     *                          are colinear and touch
     * @return New line that combines the two or NULL on non-overlapping segments.
     */
    SCH_LINE* MergeOverlap( SCH_SCREEN* aScreen, SCH_LINE* aLine, bool aCheckJunctions );

    /**
     * Check if two lines are in the same quadrant as each other, using a reference point as
     * the origin
     *
     * @param aLine - Line to compare
     * @param aPosition - Point to reference against lines
     * @return true if lines are mostly in different quadrants of aPosition, false otherwise
     */
    bool IsSameQuadrant( const SCH_LINE* aLine, const wxPoint& aPosition ) const;

    bool IsParallel( const SCH_LINE* aLine ) const;

    void GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                              const SCH_SHEET_PATH* aPath = nullptr ) override;

    bool IsStartDangling() const { return m_startIsDangling; }
    bool IsEndDangling() const { return m_endIsDangling; }
    bool IsDangling() const override { return m_startIsDangling || m_endIsDangling; }

    bool IsConnectable() const override;

    std::vector<wxPoint> GetConnectionPoints() const override;

    void GetSelectedPoints( std::vector< wxPoint >& aPoints ) const;

    bool CanConnect( const SCH_ITEM* aItem ) const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    bool operator <( const SCH_ITEM& aItem ) const override;

    wxPoint GetPosition() const override { return m_start; }
    void SetPosition( const wxPoint& aPosition ) override;

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override
    {
        return ( GetStartPoint() == aPos && IsStartDangling() )
               || ( GetEndPoint() == aPos && IsEndDangling() );
    }

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) override;

    EDA_ITEM* Clone() const override;

    void SwapData( SCH_ITEM* aItem ) override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    /**
     * Returns if the line is a graphic (non electrical line)
     *
     * Currently, anything on the internal NOTES layer is a graphic line
     */
    bool IsGraphicLine() const;

    /**
     * Returns true if the line is a wire.
     *
     * @return true if this line is on the wire layer.
     */
    bool IsWire() const;

private:
    bool doIsConnected( const wxPoint& aPosition ) const override;
};


#endif    // _SCH_LINE_H_
