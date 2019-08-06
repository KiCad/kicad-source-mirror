/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_pad.h
 * @brief Pad object description
 */

#ifndef PAD_H_
#define PAD_H_

#include "zones.h"
#include <board_connected_item.h>
#include <class_board_item.h>
#include <config_params.h> // PARAM_CFG_ARRAY
#include <convert_to_biu.h>
#include <geometry/shape_poly_set.h>
#include <pad_shapes.h>
#include <pcbnew.h>

class DRAWSEGMENT;

enum CUST_PAD_SHAPE_IN_ZONE
{
    CUST_PAD_SHAPE_IN_ZONE_OUTLINE,
    CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL
};

class LINE_READER;
class EDA_3D_CANVAS;
class MODULE;
class EDGE_MODULE;
class TRACK;
class MSG_PANEL_INFO;

namespace KIGFX
{
    class VIEW;
}

// Helper class to store parameters used to draw a pad
class PAD_DRAWINFO
{
public:
    COLOR4D m_Color;              // color used to draw the pad shape , from pad layers and
                                  // visible layers
    COLOR4D m_HoleColor;          // color used to draw the pad hole
    COLOR4D m_NPHoleColor;        // color used to draw a pad Not Plated hole
    COLOR4D m_NoNetMarkColor;     // color used to draw a mark on pads having no net
    int m_PadClearance;           // clearance value, used to draw the pad area outlines
    wxSize m_Mask_margin;         // margin, used to draw solder paste when only one layer is shown
    bool m_Display_padnum;        // true to show pad number
    bool m_Display_netname;       // true to show net name
    bool m_ShowPadFilled;         // true to show pad as solid area, false to show pas in
                                  // sketch mode
    bool m_ShowNCMark;            // true to show pad not connected mark
    bool m_ShowNotPlatedHole;     // true when the pad hole in not plated, to draw a specific
                                  // pad shape
    bool m_IsPrinting;            // true to print, false to display on screen.
    wxPoint m_Offset;             // general draw offset

    PAD_DRAWINFO();
};

/** Helper class to handle a primitive (basic shape: polygon, segment, circle or arc)
 * to build a custom pad full shape from a set of primitives
 */
class PAD_CS_PRIMITIVE
{
public:
    STROKE_T m_Shape;   /// S_SEGMENT, S_ARC, S_CIRCLE, S_POLYGON only (same as DRAWSEGMENT)
    int m_Thickness;    /// thickness of segment or outline
                        /// For filled S_CIRCLE shape, thickness = 0.
                        // if thickness is not = 0 S_CIRCLE shape is a ring
    int m_Radius;       /// radius of a circle
    double m_ArcAngle;  /// angle of an arc, from its starting point, in 0.1 deg
    wxPoint m_Start;    /// is also the center of the circle and arc
    wxPoint m_End;      /// is also the start point of the arc
    wxPoint m_Ctrl1;    /// Bezier Control point 1
    wxPoint m_Ctrl2;    /// Bezier Control point 2
    std::vector<wxPoint> m_Poly;

    PAD_CS_PRIMITIVE( STROKE_T aShape ):
        m_Shape( aShape ), m_Thickness( 0 ), m_Radius( 0 ), m_ArcAngle( 0 )
    {
    }

    // Accessors (helpers for arc and circle shapes)
    wxPoint GetCenter() { return m_Start; }     /// returns the center of a circle or arc
    wxPoint GetArcStart() { return m_End; }     /// returns the start point of an arc

    // Geometric transform
    /** Move the primitive
     * @param aMoveVector is the deplacement vector
     */
    void Move( wxPoint aMoveVector );

    /**
     * Rotates the primitive about a point
     * @param aRotCentre center of rotation
     * @param aAngle angle in tenths of degree
     */
    void Rotate( const wxPoint& aRotCentre, double aAngle );

    /** Export the PAD_CS_PRIMITIVE parameters to a DRAWSEGMENT
     * useful to draw a primitive shape
     * @param aTarget is the DRAWSEGMENT to initialize
     */
    void ExportTo( DRAWSEGMENT* aTarget );

    /** Export the PAD_CS_PRIMITIVE parameters to a EDGE_MODULE
     * useful to convert a primitive shape to a EDGE_MODULE shape for editing in footprint editor
     * @param aTarget is the EDGE_MODULE to initialize
     */
    void ExportTo( EDGE_MODULE* aTarget );
};


class D_PAD : public BOARD_CONNECTED_ITEM
{
public:
    static int  m_PadSketchModePenSize; ///< Pen size used to draw pads in sketch mode
                                        ///< (mode used to print pads on silkscreen layer)

public:
    D_PAD( MODULE* parent );

    // Do not create a copy constructor & operator=.
    // The ones generated by the compiler are adequate.

    /* Default layers used for pads, according to the pad type.
     * this is default values only, they can be changed for a given pad
     */
    static LSET StandardMask();     ///< layer set for a through hole pad
    static LSET SMDMask();          ///< layer set for a SMD pad on Front layer
    static LSET ConnSMDMask();      ///< layer set for a SMD pad on Front layer
                                    ///< used for edge board connectors
    static LSET UnplatedHoleMask(); ///< layer set for a mechanical unplated through hole pad
    static LSET ApertureMask();     ///< layer set for an aperture pad

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_PAD_T == aItem->Type();
    }

    MODULE* GetParent() const { return (MODULE*) m_Parent; }

    /**
     * Imports the pad settings from aMasterPad.
     * The result is "this" has the same settinds (sizes, shapes ... )
     * as aMasterPad
     * @param aMasterPad = the template pad
     */
    void ImportSettingsFrom( const D_PAD& aMasterPad );

    /**
     * @return true if the pad has a footprint parent flipped
     * (on the back/bottom layer)
     */
    bool IsFlipped() const;

    /**
     * Set the pad name (sometimes called pad number, although
     * it can be an array reference like AA12).
     */
    void SetName( const wxString& aName )
    {
        m_name = aName;
    }

    /**
     * @return the pad name
     */
    const wxString& GetName() const
    {
        return m_name;
    }

    /**
     * Function IncrementPadName
     *
     * Increments the pad name to the next available name in the module.
     *
     * @param aSkipUnconnectable skips any pads that are not connectable (for example NPTH)
     * @param aFillSequenceGaps if true, the next reference in a sequence
     * like A1,A3,A4 will be A2. If false, it will be A5.
     * @return pad name incremented
     */
    bool IncrementPadName( bool aSkipUnconnectable, bool aFillSequenceGaps );

    bool PadNameEqual( const D_PAD* other ) const
    {
        return m_name == other->m_name; // hide tricks behind sensible API
    }

    /**
     * Function GetShape
     * @return the shape of this pad.
     */
    PAD_SHAPE_T GetShape() const                { return m_padShape; }
    void SetShape( PAD_SHAPE_T aShape )         { m_padShape = aShape; m_boundingRadius = -1; }

    void SetPosition( const wxPoint& aPos ) override { m_Pos = aPos; }
    const wxPoint GetPosition() const override { return m_Pos; }

    /**
     * Function GetAnchorPadShape
     * @return the shape of the anchor pad shape, for custom shaped pads.
     */
    PAD_SHAPE_T GetAnchorPadShape() const       { return m_anchorPadShape; }

    /**
     * @return the option for the custom pad shape to use as clearance area
     * in copper zones
     */
    CUST_PAD_SHAPE_IN_ZONE GetCustomShapeInZoneOpt() const
    {
        return m_customShapeClearanceArea;
    }

    /**
     * Set the option for the custom pad shape to use as clearance area
     * in copper zones
     * @param aOption is the clearance area shape CUST_PAD_SHAPE_IN_ZONE option
     */
    void SetCustomShapeInZoneOpt( CUST_PAD_SHAPE_IN_ZONE aOption )
    {
        m_customShapeClearanceArea = aOption;
    }

    /**
     * Function SetAnchorPadShape
     * Set the shape of the anchor pad for custm shped pads.
     * @param the shape of the anchor pad shape( currently, only
     * PAD_SHAPE_RECT or PAD_SHAPE_CIRCLE.
     */
    void SetAnchorPadShape( PAD_SHAPE_T aShape )
    {
        m_anchorPadShape = ( aShape ==  PAD_SHAPE_RECT ) ? PAD_SHAPE_RECT : PAD_SHAPE_CIRCLE;
        m_boundingRadius = -1;
    }

    /**
     * @return true if the pad is on any copper layer, false otherwise.
     * pads can be only on tech layers to build special pads.
     * they are therefore not always on a copper layer
     */
    bool IsOnCopperLayer() const override
    {
        return ( GetLayerSet() & LSET::AllCuMask() ) != 0;
    }

    void SetY( int y )                          { m_Pos.y = y; }
    void SetX( int x )                          { m_Pos.x = x; }

    void SetPos0( const wxPoint& aPos )         { m_Pos0 = aPos; }
    const wxPoint& GetPos0() const              { return m_Pos0; }

    void SetY0( int y )                         { m_Pos0.y = y; }
    void SetX0( int x )                         { m_Pos0.x = x; }

    void SetSize( const wxSize& aSize )         { m_Size = aSize;  m_boundingRadius = -1; }
    const wxSize& GetSize() const               { return m_Size; }

    void SetDelta( const wxSize& aSize )        { m_DeltaSize = aSize;  m_boundingRadius = -1; }
    const wxSize& GetDelta() const              { return m_DeltaSize; }

    void SetDrillSize( const wxSize& aSize )    { m_Drill = aSize; }
    const wxSize& GetDrillSize() const          { return m_Drill; }

    void SetOffset( const wxPoint& aOffset )    { m_Offset = aOffset; }
    const wxPoint& GetOffset() const            { return m_Offset; }

    /**
     * Has meaning only for free shape pads.
     * add a free shape to the shape list.
     * the shape can be
     *   a polygon (outline can have a thickness)
     *   a thick segment
     *   a filled circle or ring ( if thickness == 0, this is a filled circle, else a ring)
     *   a arc
     *   a curve
     */
    void AddPrimitive( const SHAPE_POLY_SET& aPoly, int aThickness );  ///< add a polygonal basic shape
    void AddPrimitive( const std::vector<wxPoint>& aPoly, int aThickness );  ///< add a polygonal basic shape
    void AddPrimitive( wxPoint aStart, wxPoint aEnd, int aThickness ); ///< segment basic shape
    void AddPrimitive( wxPoint aCenter, int aRadius, int aThickness ); ///< ring or circle basic shape
    void AddPrimitive( wxPoint aCenter, wxPoint aStart,
                        int aArcAngle, int aThickness );    ///< arc basic shape
    void AddPrimitive( wxPoint aStart, wxPoint aEnd, wxPoint aCtrl1,
                        wxPoint aCtrl2, int aThickness );              ///< curve basic shape


    bool GetBestAnchorPosition( VECTOR2I& aPos );

    /**
     * Merge all basic shapes, converted to a polygon in one polygon,
     * in m_customShapeAsPolygon
     * @return true if OK, false in there is more than one polygon
     * in m_customShapeAsPolygon
     * @param aMergedPolygon = the SHAPE_POLY_SET to fill.
     * if NULL, m_customShapeAsPolygon is the target
     * @param aCircleToSegmentsCount = number of segment to approximate a circle
     * (default = 32)
     * Note: The corners coordinates are relative to the pad position, orientation 0,
     */
    bool MergePrimitivesAsPolygon( SHAPE_POLY_SET* aMergedPolygon = NULL );

    /**
     * clear the basic shapes list
     */
    void DeletePrimitivesList();

    /**
     * When created, the corners coordinates are relative to the pad position, orientation 0,
     * in m_customShapeAsPolygon
     * CustomShapeAsPolygonToBoardPosition transform these coordinates to actual
     * (board) coordinates
     * @param aMergedPolygon = the corners coordinates, relative to aPosition and
     *  rotated by aRotation
     * @param aPosition = the position of the shape (usually the pad shape, but
     * not always, when moving the pad)
     * @param aRotation = the rotation of the shape (usually the pad rotation, but
     * not always, in DRC)
     */
    void CustomShapeAsPolygonToBoardPosition( SHAPE_POLY_SET * aMergedPolygon,
                                    wxPoint aPosition, double aRotation ) const;

    /**
     * Accessor to the basic shape list
     */
    const std::vector<PAD_CS_PRIMITIVE>& GetPrimitives() const { return m_basicShapes; }

    /**
     * Accessor to the custom shape as one polygon
     */
    const SHAPE_POLY_SET& GetCustomShapeAsPolygon() const { return m_customShapeAsPolygon; }

    void Flip( const wxPoint& aCentre, bool aFlipLeftRight ) override;

    /**
     * Flip the basic shapes, in custom pads
     */
    void FlipPrimitives();

    /**
     * Mirror the primitives about a coordinate
     *
     * @param aX the x coordinate about which to mirror
     */
    void MirrorXPrimitives( int aX );

    /**
     * Import to the basic shape list
     * @return true if OK, false if issues
     * (more than one polygon to build the polygon shape list)
     */
    bool SetPrimitives( const std::vector<PAD_CS_PRIMITIVE>& aPrimitivesList );

    /**
     * Add to the basic shape list
     * @return true if OK, false if issues
     * (more than one polygon to build the polygon shape list)
     */
    bool AddPrimitives( const std::vector<PAD_CS_PRIMITIVE>& aPrimitivesList );


    /**
     * Function SetOrientation
     * sets the rotation angle of the pad.
     * @param aAngle is tenths of degrees, but will soon be degrees.  If it is
     *  outside of 0 - 3600, then it will be normalized before being saved.
     */
    void SetOrientation( double aAngle );

    /**
     * Set orientation in degrees
     */
    void SetOrientationDegrees( double aOrientation ) { SetOrientation( aOrientation*10.0 ); }

    /**
     * Function GetOrientation
     * returns the rotation angle of the pad in tenths of degrees, but soon degrees.
     */
    double GetOrientation() const { return m_Orient; }
    double GetOrientationDegrees() const   { return m_Orient/10.0; }
    double GetOrientationRadians() const   { return m_Orient*M_PI/1800; }

    void SetDrillShape( PAD_DRILL_SHAPE_T aDrillShape )
        { m_drillShape = aDrillShape; }
    PAD_DRILL_SHAPE_T GetDrillShape() const     { return m_drillShape; }

    /**
     * Function GetOblongDrillGeometry calculates the start point, end point and width
     * of an equivalent segment which have the same position and width as the hole
     * Usefull to plot/draw oblong holes like segments with rounded ends
     * used in draw and plot functions
     * @param aStartPoint = first point of the equivalent segment, relative to the pad position.
     * @param aEndPoint = second point of the equivalent segment, relative to the pad position.
     * @param aWidth = width equivalent segment.
     */
    void GetOblongDrillGeometry( wxPoint& aStartPoint, wxPoint& aEndPoint, int& aWidth ) const;

    void SetLayerSet( LSET aLayerMask )         { m_layerMask = aLayerMask; }
    LSET GetLayerSet() const override           { return m_layerMask; }

    void SetAttribute( PAD_ATTR_T aAttribute );
    PAD_ATTR_T GetAttribute() const             { return m_Attribute; }

    // We don't currently have an attribute for APERTURE, and adding one will change the file
    // format, so for now just infer a copper-less pad to be an APERTURE pad.
    bool IsAperturePad() const                  { return ( m_layerMask & LSET::AllCuMask() ).none(); }

    void SetPadToDieLength( int aLength )       { m_LengthPadToDie = aLength; }
    int GetPadToDieLength() const               { return m_LengthPadToDie; }

    int GetLocalSolderMaskMargin() const        { return m_LocalSolderMaskMargin; }
    void SetLocalSolderMaskMargin( int aMargin ) { m_LocalSolderMaskMargin = aMargin; }

    int GetLocalClearance() const               { return m_LocalClearance; }
    void SetLocalClearance( int aClearance )    { m_LocalClearance = aClearance; }

    int GetLocalSolderPasteMargin() const       { return m_LocalSolderPasteMargin; }
    void SetLocalSolderPasteMargin( int aMargin ) { m_LocalSolderPasteMargin = aMargin; }

    double GetLocalSolderPasteMarginRatio() const { return m_LocalSolderPasteMarginRatio; }
    void SetLocalSolderPasteMarginRatio( double aRatio ) { m_LocalSolderPasteMarginRatio = aRatio; }


    /**
     * Function TransformShapeWithClearanceToPolygon
     * Convert the pad shape to a closed polygon
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aMaxError = Maximum error from true when converting arcs
     * @param ignoreLineWidth = used for edge cut items where the line width is only
     * for visualization
     */
    void TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer, int aClearanceValue,
            int aMaxError = ARC_HIGH_DEF, bool ignoreLineWidth = false ) const override;

    /**
     * Function GetClearance
     * returns the clearance in internal units.  If \a aItem is not NULL then the
     * returned clearance is the greater of this object's clearance and
     * aItem's clearance.  If \a aItem is NULL, then this objects clearance
     * is returned.
     * @param aItem is another BOARD_CONNECTED_ITEM or NULL
     * @return int - the clearance in internal units.
     */
    int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const override;

   // Mask margins handling:

    /**
     * Function GetSolderMaskMargin
     * @return the margin for the solder mask layer
     * usually > 0 (mask shape bigger than pad
     * For pads also on copper layers, the value (used to build a default shape) is
     * 1 - the local value
     * 2 - if 0, the parent footprint value
     * 3 - if 0, the global value
     * For pads NOT on copper layers, the value is the local value because there is
     * not default shape to build
     */
    int GetSolderMaskMargin() const;

    /**
     * Function GetSolderPasteMargin
     * @return the margin for the solder mask layer
     * usually < 0 (mask shape smaller than pad)
     * because the margin can be dependent on the pad size, the margin has a x and a y value
     *
     * For pads also on copper layers, the value (used to build a default shape) is
     * 1 - the local value
     * 2 - if 0, the parent footprint value
     * 3 - if 0, the global value
     *
     * For pads NOT on copper layers, the value is the local value because there is
     * not default shape to build
    */
    wxSize GetSolderPasteMargin() const;

    void SetZoneConnection( ZoneConnection aType ) { m_ZoneConnection = aType; }
    ZoneConnection GetZoneConnection() const;
    ZoneConnection GetLocalZoneConnection() const { return m_ZoneConnection; }

    void SetThermalWidth( int aWidth ) { m_ThermalWidth = aWidth; }
    int GetThermalWidth() const;

    void SetThermalGap( int aGap ) { m_ThermalGap = aGap; }
    int GetThermalGap() const;

    void Print( PCB_BASE_FRAME* aFrame, wxDC* aDC, const wxPoint& aOffset = ZeroOffset ) override;

    /**
     * Function PrintShape
     * basic function to print a pad.
     * <p>
     * This function is used by Print after calculation of parameters (color, ) final
     * orientation transforms are set.
     * </p>
     */
    void PrintShape( wxDC* aDC, PAD_DRAWINFO& aDrawInfo );

    /**
     * Function BuildPadPolygon
     * Has meaning only for polygonal pads (trapezoid and rectangular)
     * Build the Corner list of the polygonal shape,
     * depending on shape, extra size (clearance ...) and orientation
     * @param aCoord = a buffer to fill (4 corners).
     * @param aInflateValue = wxSize: the clearance or margin value. value > 0:
     *                        inflate, < 0 deflate
     * @param aRotation = full rotation of the polygon
     */
    void BuildPadPolygon( wxPoint aCoord[4], wxSize aInflateValue, double aRotation ) const;

    /**
     * Function GetRoundRectCornerRadius
     * Has meaning only for rounded rect pads
     * @return The radius of the rounded corners for this pad.
     */
    int GetRoundRectCornerRadius() const
    {
        return GetRoundRectCornerRadius( m_Size );
    }

    /**
     * Helper function GetRoundRectCornerRadius
     * Has meaning only for rounded rect pads
     * Returns the radius of the rounded corners of a rectangle
     * size aSize, using others setting of the pad
     * @param aSize = size of the of the round rect. Usually the pad size
     * but can be the size of the pad on solder mask or solder paste
     * @return The radius of the rounded corners for this pad size.
     */
    int GetRoundRectCornerRadius( const wxSize& aSize ) const;

    /**
     * Set the rounded rectangle radius ratio based on a given radius
     * @param aRadius = desired radius of curvature
     */
    void SetRoundRectCornerRadius( double aRadius );

    /**
     * Function BuildPadShapePolygon
     * Build the Corner list of the polygonal shape,
     * depending on shape, extra size (clearance ...) pad and orientation
     * This function is similar to TransformShapeWithClearanceToPolygon,
     * but the difference is BuildPadShapePolygon creates a polygon shape exactly
     * similar to pad shape, which a size inflated by aInflateValue
     * and TransformShapeWithClearanceToPolygon creates a more complex shape (for instance
     * a rectangular pad is converted in a rectangulr shape with ronded corners)
     * @param aCornerBuffer = a buffer to fill.
     * @param aInflateValue = the clearance or margin value.
     *              value > 0: inflate, < 0 deflate, = 0 : no change
     *              the clearance can have different values for x and y directions
     *              (relative to the pad)
     * @param aError = Maximum deviation of an arc from the polygon segment
     */
    void BuildPadShapePolygon(
            SHAPE_POLY_SET& aCornerBuffer, wxSize aInflateValue, int aError = ARC_HIGH_DEF ) const;

    /**
     * Function BuildPadDrillShapePolygon
     * Build the Corner list of the polygonal drill shape,
     * depending on shape pad hole and orientation
     * @param aCornerBuffer = a buffer to fill.
     * @param aInflateValue = the clearance or margin value.
     *              value > 0: inflate, < 0 deflate, = 0 : no change
     * @param aError = Maximum deviation of an arc from the polygon approximation
     * @return false if the pad has no hole, true otherwise
     */
    bool BuildPadDrillShapePolygon(
            SHAPE_POLY_SET& aCornerBuffer, int aInflateValue, int aError = ARC_HIGH_DEF ) const;

    /**
     * Function BuildSegmentFromOvalShape
     * Has meaning only for OVAL (and ROUND) pads
     * Build an equivalent segment having the same shape as the OVAL shape,
     * Useful in draw function and in DRC and HitTest functions,
     *  because segments are already well handled by track tests
     * @param aSegStart = the starting point of the equivalent segment relative to the shape
     *                    position.
     * @param aSegEnd = the ending point of the equivalent segment, relative to the shape position
     * @param aRotation = full rotation of the segment
     * @param aRotation = full rotation of the segment
     * @param aMargin = a margin around the shape (for instance mask margin)
     * @return the width of the segment
     */
    int BuildSegmentFromOvalShape( wxPoint& aSegStart, wxPoint& aSegEnd,
                                   double aRotation, const wxSize& aMargin ) const;

    /**
     * Function GetBoundingRadius
     * returns the radius of a minimum sized circle which fully encloses this pad.
     * The center is the pad position
     */
    int GetBoundingRadius() const
    {
        // Any member function which would affect this calculation should set
        // m_boundingRadius to -1 to re-trigger the calculation from here.
        // Currently that is only m_Size, m_DeltaSize, and m_padShape accessors.
        if( m_boundingRadius == -1 )
        {
            m_boundingRadius = boundingRadius();
        }

        return m_boundingRadius;
    }

    wxPoint ShapePos() const;

    /**
     * has meaning only for rounded rect pads
     * @return the scaling factor between the smaller Y or Y size and the radius
     * of the rounded corners.
     * Cannot be > 0.5
     * the normalized IPC-7351C value is 0.25
     */
    double GetRoundRectRadiusRatio() const
    {
        return m_padRoundRectRadiusScale;
    }

    /**
     * has meaning only for rounded rect pads
     * Set the scaling factor between the smaller Y or Y size and the radius
     * of the rounded corners.
     * Cannot be < 0.5 and obviously must be > 0
     * the normalized IPC-7351C value is 0.25
     */
    void SetRoundRectRadiusRatio( double aRadiusScale )
    {
        if( aRadiusScale < 0.0 )
            aRadiusScale = 0.0;

        m_padRoundRectRadiusScale = std::min( aRadiusScale, 0.5 );
    }

    /**
     * has meaning only for chamfered rect pads
     * @return the ratio between the smaller Y or Y size and the radius
     * of the rounded corners.
     * Cannot be > 0.5
     */
    double GetChamferRectRatio() const
    {
        return m_padChamferRectScale;
    }

    /**
     * has meaning only for chamfered rect pads
     * Set the ratio between the smaller Y or Y size and the radius
     * of the rounded corners.
     * Cannot be < 0.5 and obviously must be > 0
     */
    void SetChamferRectRatio( double aChamferScale )
    {
        if( aChamferScale < 0.0 )
            aChamferScale = 0.0;

        m_padChamferRectScale = std::min( aChamferScale, 0.5 );
    }

    /**
     * has meaning only for chamfered rect pads
     * @return the position of the chamfer for a 0 orientation
     */
    int GetChamferPositions() const { return m_chamferPositions; }

    /**
     * has meaning only for chamfered rect pads
     * set the position of the chamfer for a 0 orientation, one of
     * RECT_CHAMFER_TOP_LEFT, RECT_CHAMFER_TOP_RIGHT,
     * RECT_CHAMFER_BOTTOM_LEFT, RECT_CHAMFER_BOTTOM_RIGHT
     */
    void SetChamferPositions( int aChamferPositions )
    {
        m_chamferPositions = aChamferPositions;
    }

    /**
     * Function GetSubRatsnest
     * @return int - the netcode
     */
    int GetSubRatsnest() const                  { return m_SubRatsnest; }
    void SetSubRatsnest( int aSubRatsnest )     { m_SubRatsnest = aSubRatsnest; }

    void GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList ) override;

    bool IsOnLayer( PCB_LAYER_ID aLayer ) const override
    {
        return m_layerMask[aLayer];
    }

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    wxString GetClass() const override
    {
        return wxT( "PAD" );
    }

    // Virtual function:
    const EDA_RECT GetBoundingBox() const override;

    ///> Set absolute coordinates.
    void SetDrawCoord();

    //todo: Remove SetLocalCoord along with m_pos
    ///> Set relative coordinates.
    void SetLocalCoord();

    /**
     * Function Compare
     * compares two pads and return 0 if they are equal.
     * @return int - <0 if left less than right, 0 if equal, >0 if left greater than right.
     */
    static int Compare( const D_PAD* padref, const D_PAD* padcmp );

    void Move( const wxPoint& aMoveVector ) override
    {
        m_Pos += aMoveVector;
        SetLocalCoord();
    }

    void Rotate( const wxPoint& aRotCentre, double aAngle ) override;

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    /**
     * Function ShowPadShape
     * @return the name of the shape
     */
    wxString ShowPadShape() const;

    /**
     * Function ShowPadAttr
     * @return the name of the pad type (attribute) : STD, SMD ...
     */
    wxString ShowPadAttr() const;

    /**
     * Function AppendConfigs
     * appends to @a aResult the configuration setting accessors which will later
     * allow reading or writing of configuration file information directly into
     * this object.
     */
    void AppendConfigs( PARAM_CFG_ARRAY* aResult );

    EDA_ITEM* Clone() const override;

    /**
     * same as Clone, but returns a D_PAD item.
     * Useful mainly for pythons scripts, because Clone (virtual function)
     * returns an EDA_ITEM.
     */
    D_PAD* Duplicate() const
    {
        return (D_PAD*) Clone();
    }

    /**
     * A pad whose hole is the same size as the pad is a NPTH.  However, if the user
     * fails to mark this correctly then the pad will become invisible on the board.
     * This check allows us to special-case this error-condition.
     */
    bool PadShouldBeNPTH() const;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    virtual unsigned int ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    virtual const BOX2I ViewBBox() const override;

    virtual void SwapData( BOARD_ITEM* aImage ) override;

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif


private:
    /**
     * Function boundingRadius
     * returns a calculated radius of a bounding circle for this pad.
     */
    int boundingRadius() const;

    bool buildCustomPadPolygon( SHAPE_POLY_SET* aMergedPolygon, int aError );

private:    // Private variable members:

    // Actually computed and cached on demand by the accessor
    mutable int m_boundingRadius;  ///< radius of the circle containing the pad shape

    wxString    m_name;

    // TODO: Remove m_Pos from Pad or make private.  View positions calculated from m_Pos0
    wxPoint     m_Pos;              ///< pad Position on board

    PAD_SHAPE_T m_padShape;         ///< Shape: PAD_SHAPE_CIRCLE, PAD_SHAPE_RECT,
                                    ///< PAD_SHAPE_OVAL, PAD_SHAPE_TRAPEZOID,
                                    ///< PAD_SHAPE_ROUNDRECT, PAD_SHAPE_POLYGON

    /** for free shape pads: a list of basic shapes,
     * in local coordinates, orient 0, coordinates relative to m_Pos
     * They are expected to define only one copper area.
     */
    std::vector<PAD_CS_PRIMITIVE> m_basicShapes;

    /** for free shape pads: the set of basic shapes, merged as one polygon,
     * in local coordinates, orient 0, coordinates relative to m_Pos
     */
    SHAPE_POLY_SET m_customShapeAsPolygon;

    /**
     * How to build the custom shape in zone, to create the clearance area:
     * CUST_PAD_SHAPE_IN_ZONE_OUTLINE = use pad shape
     * CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL = use the convex hull of the pad shape
     * other values are currently reserved
     */
    CUST_PAD_SHAPE_IN_ZONE  m_customShapeClearanceArea;

    int         m_SubRatsnest;      ///< variable used in rats nest computations
                                    ///< handle subnet (block) number in ratsnest connection

    wxSize      m_Drill;            ///< Drill diam (drill shape = PAD_CIRCLE) or drill size
                                    ///< (shape = OVAL) for drill shape = PAD_CIRCLE, drill
                                    ///< diam = m_Drill.x

    wxSize      m_Size;             ///< X and Y size ( relative to orient 0)

    PAD_DRILL_SHAPE_T m_drillShape; ///< PAD_DRILL_SHAPE_CIRCLE, PAD_DRILL_SHAPE_OBLONG

    double      m_padRoundRectRadiusScale;  ///< scaling factor from smallest m_Size coord
                                            ///< to corner radius, default 0.25
    double      m_padChamferRectScale;      ///< scaling factor from smallest m_Size coord
                                            ///< to chamfer value, default 0.25
    int m_chamferPositions;                 ///< the positions of the chamfered position for a 0 orientation

    PAD_SHAPE_T m_anchorPadShape;         ///< for custom shaped pads: shape of pad anchor,
                                          ///< PAD_SHAPE_RECT, PAD_SHAPE_CIRCLE

    /**
     * m_Offset is useful only for oblong and rect pads (it can be used for other
     * shapes, but without any interest).
     * This is the offset between the pad hole and the pad shape (you must
     * understand here pad shape = copper area around the hole)
     * Most of cases, the hole is the center of the shape (m_Offset = 0).
     * But some board designers use oblong/rect pads with a hole moved to one of the
     * oblong/rect pad shape ends.
     * In all cases the pad position is the pad hole.
     * The physical shape position (used to draw it for instance) is pad
     * position (m_Pos) + m_Offset.
     * D_PAD::ShapePos() returns the physical shape position according to
     * the offset and the pad rotation.
     */
    wxPoint     m_Offset;

    LSET        m_layerMask;        ///< Bitwise layer :1= copper layer, 15= cmp,
                                    ///< 2..14 = internal layers
                                    ///< 16 .. 31 = technical layers

    wxSize      m_DeltaSize;        ///< delta on rectangular shapes

    wxPoint     m_Pos0;             ///< Initial Pad position (i.e. pad position relative to the
                                    ///< module anchor, orientation 0)

    PAD_ATTR_T  m_Attribute;        ///< PAD_ATTRIB_NORMAL, PAD_ATTRIB_SMD,
                                    ///< PAD_ATTRIB_CONN, PAD_ATTRIB_HOLE_NOT_PLATED
    double      m_Orient;           ///< in 1/10 degrees

    int         m_LengthPadToDie;   ///< Length net from pad to die, inside the package

    /// Local clearance. When null, the module default value is used.
    /// when the module default value is null, the netclass value is used
    /// Usually the local clearance is null
    int         m_LocalClearance;

    /// Local mask margins: when 0, the parent footprint design values are used

    int         m_LocalSolderMaskMargin;        ///< Local solder mask margin
    int         m_LocalSolderPasteMargin;       ///< Local solder paste margin absolute value

    double      m_LocalSolderPasteMarginRatio;  ///< Local solder mask margin ratio value of pad size
                                                ///< The final margin is the sum of these 2 values
    /// how the connection to zone is made: no connection, thermal relief ...
    ZoneConnection m_ZoneConnection;

    int         m_ThermalWidth;
    int         m_ThermalGap;
};

#endif  // PAD_H_
