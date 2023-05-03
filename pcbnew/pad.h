/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PAD_H
#define PAD_H

#include <mutex>
#include <array>
#include <zones.h>
#include <board_connected_item.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_compound.h>
#include <pad_shapes.h>
#include <geometry/eda_angle.h>
#include <core/arraydim.h>

class PCB_SHAPE;
class PARAM_CFG;
class SHAPE;
class SHAPE_SEGMENT;

enum CUST_PAD_SHAPE_IN_ZONE
{
    CUST_PAD_SHAPE_IN_ZONE_OUTLINE,
    CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL
};

class LINE_READER;
class EDA_3D_CANVAS;
class FOOTPRINT;

namespace KIGFX
{
    class VIEW;
}

class PAD : public BOARD_CONNECTED_ITEM
{
public:
    PAD( FOOTPRINT* parent );

    // Copy constructor & operator= are needed because the list of basic shapes
    // must be duplicated in copy.
    PAD( const PAD& aPad );
    PAD& operator=( const PAD &aOther );

    /*
     * Default layers used for pads, according to the pad type.
     *
     * This is default values only, they can be changed for a given pad.
     */
    static LSET PTHMask();          ///< layer set for a through hole pad
    static LSET SMDMask();          ///< layer set for a SMD pad on Front layer
    static LSET ConnSMDMask();      ///< layer set for a SMD pad on Front layer
                                    ///< used for edge board connectors
    static LSET UnplatedHoleMask(); ///< layer set for a mechanical unplated through hole pad
    static LSET ApertureMask();     ///< layer set for an aperture pad

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_PAD_T == aItem->Type();
    }

    bool IsType( const std::vector<KICAD_T>& aScanTypes ) const override
    {
        if( BOARD_CONNECTED_ITEM::IsType( aScanTypes ) )
            return true;

        for( KICAD_T scanType : aScanTypes )
        {
            if( HasHole() )
            {
                if( scanType == PCB_LOCATE_HOLE_T )
                    return true;
                else if( scanType == PCB_LOCATE_PTH_T && m_attribute != PAD_ATTRIB::NPTH )
                    return true;
                else if( scanType == PCB_LOCATE_NPTH_T && m_attribute == PAD_ATTRIB::NPTH )
                    return true;
            }
        }

        return false;
    }

    bool HasHole() const override
    {
        return GetDrillSizeX() > 0 && GetDrillSizeY() > 0;
    }

    FOOTPRINT* GetParent() const;

    wxString GetParentAsString() const;

    bool IsLocked() const override;

    /**
     * Import the pad settings from \a aMasterPad.
     *
     * The result is "this" has the same settings (sizes, shapes ... ) as \a aMasterPad.
     *
     * @param aMasterPad the template pad.
     */
    void ImportSettingsFrom( const PAD& aMasterPad );

    /**
     * @return true if the pad has a footprint parent flipped on the back/bottom layer.
     */
    bool IsFlipped() const;

    /**
     * Set the pad number (note that it can be alphanumeric, such as the array reference "AA12").
     */
    void SetNumber( const wxString& aNumber ) { m_number = aNumber; }
    const wxString& GetNumber() const { return m_number; }

    /**
     * Indicates whether or not the pad can have a number.  (NPTH and SMD aperture pads can not.)
     */
    bool CanHaveNumber() const;

    /**
     * Set the pad function (pin name in schematic)
     */
    void SetPinFunction( const wxString& aName ) { m_pinFunction = aName; }
    const wxString& GetPinFunction() const { return m_pinFunction; }

    /**
     * Set the pad electrical type
     */
    void SetPinType( const wxString& aType ) { m_pinType = aType; }
    const wxString& GetPinType() const { return m_pinType; }

    /**
     * Before we had custom pad shapes it was common to have multiple overlapping pads to
     * represent a more complex shape.
     */
    bool SameLogicalPadAs( const PAD* aOther ) const
    {
        // hide tricks behind sensible API
        return GetParentFootprint() == aOther->GetParentFootprint()
            && !m_number.IsEmpty() && m_number == aOther->m_number;
    }

    /**
     * @return true if this and \param aOther represent a net-tie.
     */
    bool SharesNetTieGroup( const PAD* aOther ) const;

    /**
     * @return true if the pad is associated with an "unconnected" pin (or a no-connect symbol)
     * and has no net.
     */
    bool IsNoConnectPad() const;

    /**
     * @return true if the pad is associated with a "free" pin (not-internally-connected) and has
     * not yet been assigned another net (ie: by being routed to).
     */
    bool IsFreePad() const;

    /**
     * Set the new shape of this pad.
     */
    void SetShape( PAD_SHAPE aShape )
    {
        m_padShape = aShape;
        SetDirty();
    }

    /**
     * @return the shape of this pad.
     */
    PAD_SHAPE GetShape() const { return m_padShape; }

    void SetPosition( const VECTOR2I& aPos ) override
    {
        m_pos = aPos;
        SetDirty();
    }

    VECTOR2I GetPosition() const override { return m_pos; }

    /**
     * @return the shape of the anchor pad shape, for custom shaped pads.
     */
    PAD_SHAPE GetAnchorPadShape() const       { return m_anchorPadShape; }

    /**
     * @return the option for the custom pad shape to use as clearance area in copper zones.
     */
    CUST_PAD_SHAPE_IN_ZONE GetCustomShapeInZoneOpt() const
    {
        return m_customShapeClearanceArea;
    }

    /**
     * Set the option for the custom pad shape to use as clearance area in copper zones.
     *
     * @param aOption is the clearance area shape CUST_PAD_SHAPE_IN_ZONE option
     */
    void SetCustomShapeInZoneOpt( CUST_PAD_SHAPE_IN_ZONE aOption )
    {
        m_customShapeClearanceArea = aOption;
    }

    /**
     * Set the shape of the anchor pad for custom shaped pads.
     *
     * @param aShape is the shape of the anchor pad shape( currently, only #PAD_SHAPE::RECT or
     *               #PAD_SHAPE::CIRCLE.
     */
    void SetAnchorPadShape( PAD_SHAPE aShape )
    {
        m_anchorPadShape = ( aShape ==  PAD_SHAPE::RECT ) ? PAD_SHAPE::RECT : PAD_SHAPE::CIRCLE;
        SetDirty();
    }

    /**
     * @return true if the pad is on any copper layer, false otherwise.
     */
    bool IsOnCopperLayer() const override;

    void SetY( int y )                          { m_pos.y = y; SetDirty(); }
    void SetX( int x )                          { m_pos.x = x; SetDirty(); }

    void SetSize( const VECTOR2I& aSize )       { m_size = aSize; SetDirty(); }
    const VECTOR2I& GetSize() const             { return m_size; }
    void SetSizeX( const int aX )               { m_size.x = aX; SetDirty(); }
    int GetSizeX() const                        { return m_size.x; }
    void SetSizeY( const int aY )               { m_size.y = aY; SetDirty(); }
    int GetSizeY() const                        { return m_size.y; }

    void SetDelta( const VECTOR2I& aSize )      { m_deltaSize = aSize; SetDirty(); }
    const VECTOR2I& GetDelta() const            { return m_deltaSize; }

    void SetDrillSize( const VECTOR2I& aSize )  { m_drill = aSize; SetDirty(); }
    const VECTOR2I& GetDrillSize() const        { return m_drill; }
    void SetDrillSizeX( const int aX )          { m_drill.x = aX; SetDirty(); }
    int GetDrillSizeX() const                   { return m_drill.x; }
    void SetDrillSizeY( const int aY )          { m_drill.y = aY; SetDirty(); }
    int GetDrillSizeY() const                   { return m_drill.y; }

    void SetOffset( const VECTOR2I& aOffset )    { m_offset = aOffset; SetDirty(); }
    const VECTOR2I& GetOffset() const            { return m_offset; }

    VECTOR2I GetCenter() const override          { return GetPosition(); }

    /**
     * Has meaning only for custom shape pads.
     * add a free shape to the shape list.
     * the shape can be
     *  - a polygon (outline can have a thickness)
     *  - a thick segment
     *  - a filled circle (thickness == 0) or ring
     *  - a filled rect (thickness == 0) or rectangular outline
     *  - a arc
     *  - a bezier curve
     */
    void AddPrimitivePoly( const SHAPE_POLY_SET& aPoly, int aThickness, bool aFilled );
    void AddPrimitivePoly( const std::vector<VECTOR2I>& aPoly, int aThickness, bool aFilled );
    void AddPrimitiveSegment( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aThickness );
    void AddPrimitiveCircle( const VECTOR2I& aCenter, int aRadius, int aThickness, bool aFilled );
    void AddPrimitiveRect( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aThickness,
                           bool aFilled );
    void AddPrimitiveArc( const VECTOR2I& aCenter, const VECTOR2I& aStart,
                          const EDA_ANGLE& aArcAngle, int aThickness );
    void AddPrimitiveCurve( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCtrl1,
                            const VECTOR2I& aCtrl2, int aThickness );

    /**
     * Has meaning only for custom shape pads.  Allows one to specify the box in which to place
     * the pad number and/or net name (if they are being displayed).
     */
    void AddPrimitiveAnnotationBox( const VECTOR2I& aStart, const VECTOR2I& aEnd );

    bool GetBestAnchorPosition( VECTOR2I& aPos );

    /**
     * Merge all basic shapes to a #SHAPE_POLY_SET.
     *
     * @note The results are relative to the pad position, orientation 0.
     *
     * @param aMergedPolygon will store the final polygon
     * @param aErrorLoc is used when a circle (or arc) is approximated by segments
     *  = ERROR_INSIDE to build a polygon inside the arc/circle (usual shape to raw/plot)
     *  = ERROR_OUIDE to build a polygon outside the arc/circle
     * (for instance when building a clearance area)
     */
    void MergePrimitivesAsPolygon( SHAPE_POLY_SET* aMergedPolygon,
                                   ERROR_LOC aErrorLoc = ERROR_INSIDE ) const;

    /**
     * Clear the basic shapes list.
     */
    void DeletePrimitivesList();

    /**
     * Accessor to the basic shape list for custom-shaped pads.
     */
    const std::vector<std::shared_ptr<PCB_SHAPE>>& GetPrimitives() const
    {
        return m_editPrimitives;
    }

    void Flip( const VECTOR2I& VECTOR2I, bool aFlipLeftRight ) override;

    /**
     * Flip (mirror) the primitives left to right or top to bottom, around the anchor position
     * in custom pads.
     */
    void FlipPrimitives( bool aFlipLeftRight );

    /**
     * Clear the current custom shape primitives list and import a new list.  Copies the input,
     * which is not altered.
     */
    void ReplacePrimitives( const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList );

    /**
     * Import a custom shape primitive list (composed of basic shapes) and add items to the
     * current list.  Copies the input, which is not altered.
     */
    void AppendPrimitives( const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList );

    /**
     * Add item to the custom shape primitives list
     */
    void AddPrimitive( PCB_SHAPE* aPrimitive );

    /**
     * Set the rotation angle of the pad.
     *
     * If \a aAngle is outside of 0 - 360, then it will be normalized.
     */
    void SetOrientation( const EDA_ANGLE& aAngle );
    void SetFPRelativeOrientation( const EDA_ANGLE& aAngle );

    /**
     * Return the rotation angle of the pad.
     */
    EDA_ANGLE GetOrientation() const { return m_orient; }
    EDA_ANGLE GetFPRelativeOrientation();

    // For property system
    void SetOrientationDegrees( double aOrientation )
    {
        SetOrientation( EDA_ANGLE( aOrientation, DEGREES_T ) );
    }
    double GetOrientationDegrees() const
    {
        return m_orient.AsDegrees();
    }

    void SetDrillShape( PAD_DRILL_SHAPE_T aShape ) { m_drillShape = aShape; m_shapesDirty = true; }
    PAD_DRILL_SHAPE_T GetDrillShape() const     { return m_drillShape; }

    bool IsDirty() const
    {
        return m_shapesDirty || m_polyDirty;
    }

    void SetDirty()
    {
        m_shapesDirty = true;
        m_polyDirty = true;
    }

    void SetLayerSet( LSET aLayers ) override   { m_layerMask = aLayers; }
    LSET GetLayerSet() const override           { return m_layerMask; }

    void SetAttribute( PAD_ATTRIB aAttribute );
    PAD_ATTRIB GetAttribute() const             { return m_attribute; }

    void SetProperty( PAD_PROP aProperty );
    PAD_PROP GetProperty() const              { return m_property; }

    // We don't currently have an attribute for APERTURE, and adding one will change the file
    // format, so for now just infer a copper-less pad to be an APERTURE pad.
    bool IsAperturePad() const
    {
        return ( m_layerMask & LSET::AllCuMask() ).none();
    }

    void SetPadToDieLength( int aLength )       { m_lengthPadToDie = aLength; }
    int GetPadToDieLength() const               { return m_lengthPadToDie; }

    int GetLocalSolderMaskMargin() const        { return m_localSolderMaskMargin; }
    void SetLocalSolderMaskMargin( int aMargin ) { m_localSolderMaskMargin = aMargin; }

    int GetLocalClearance( wxString* aSource ) const override;
    int GetLocalClearance() const               { return m_localClearance; }
    void SetLocalClearance( int aClearance )    { m_localClearance = aClearance; }

    int GetLocalSolderPasteMargin() const       { return m_localSolderPasteMargin; }
    void SetLocalSolderPasteMargin( int aMargin ) { m_localSolderPasteMargin = aMargin; }

    double GetLocalSolderPasteMarginRatio() const { return m_localSolderPasteMarginRatio; }
    void SetLocalSolderPasteMarginRatio( double aRatio ) { m_localSolderPasteMarginRatio = aRatio; }

    int GetOwnClearance( PCB_LAYER_ID aLayer, wxString* aSource = nullptr ) const override;

    /**
     * Convert the pad shape to a closed polygon. Circles and arcs are approximated by segments.
     *
     * @param aBuffer a buffer to store the polygon.
     * @param aClearance the clearance around the pad.
     * @param aMaxError maximum error from true when converting arcs.
     * @param aErrorLoc should the approximation error be placed outside or inside the polygon?
     * @param ignoreLineWidth used for edge cuts where the line width is only for visualization.
     */
    void TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                  int aMaxError, ERROR_LOC aErrorLoc,
                                  bool ignoreLineWidth = false ) const override;

    /**
     * Build the corner list of the polygonal drill shape in the board coordinate system.
     *
     * @param aBuffer a buffer to fill.
     * @param aClearance the clearance or margin value.
     * @param aError maximum deviation of an arc from the polygon approximation.
     * @param aErrorLoc = should the approximation error be placed outside or inside the polygon?
     * @return false if the pad has no hole, true otherwise.
     */
    bool TransformHoleToPolygon( SHAPE_POLY_SET& aBuffer, int aClearance, int aError,
                                 ERROR_LOC aErrorLoc ) const;

    /**
     * Some pad shapes can be complex (rounded/chamfered rectangle), even without considering
     * custom shapes.  This routine returns a COMPOUND shape (set of simple shapes which make
     * up the pad for use with routing, collision determination, etc).
     *
     * @note This list can contain a SHAPE_SIMPLE (a simple single-outline non-intersecting
     * polygon), but should never contain a SHAPE_POLY_SET (a complex polygon consisting of
     * multiple outlines and/or holes).
     *
     * @param aLayer optional parameter allowing a caller to specify a particular layer (default
     *               is to return the pad's "natural" shape).
     * @param aFlash optional parameter allowing a caller to force the pad to be flashed (or not
     *               flashed) on the current layer (default is to honour the pad's setting and
     *               the current connections for the given layer).
     */
    virtual std::shared_ptr<SHAPE>
    GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                       FLASHING flashPTHPads = FLASHING::DEFAULT ) const override;

    const std::shared_ptr<SHAPE_POLY_SET>& GetEffectivePolygon() const;

    /**
     * Return a SHAPE_SEGMENT object representing the pad's hole.
     */
    std::shared_ptr<SHAPE_SEGMENT> GetEffectiveHoleShape() const override;

    /**
     * Return the radius of a minimum sized circle which fully encloses this pad.
     *
     * The center is the pad position NOT THE SHAPE POS!
     */
    int GetBoundingRadius() const;

    /**
     * Return any local clearance overrides set in the "classic" (ie: pre-rule) system.
     *
     * @param aSource [out] optionally reports the source as a user-readable string.
     * @return the clearance in internal units.
     */
    int GetLocalClearanceOverrides( wxString* aSource ) const override;

    /**
     * @return the expansion for the solder mask layer
     *
     * Usually > 0 (mask shape bigger than pad).  For pads **not** on copper layers, the value
     * is the local value because there is no default shape to build.  For pads also on copper
     * layers, the value (used to build a default shape) is:
     *  1 the local value
     *  2 if 0, the parent footprint value
     *  3 if 0, the global value
     */
    int GetSolderMaskExpansion() const;

    /**
     * Usually < 0 (mask shape smaller than pad)because the margin can be dependent on the pad
     * size, the margin has a x and a y value.  For pads **not** on copper layers, the value is
     * the local value because there is no default shape to build.  For pads also on copper
     * layers, the value (used to build a default shape) is:
     *  1 the local value
     *  2 if 0, the parent footprint value
     *  3 if 0, the global value
     *
     * @return the margin for the solder mask layer.
    */
    VECTOR2I GetSolderPasteMargin() const;

    void SetZoneConnection( ZONE_CONNECTION aType ) { m_zoneConnection = aType; }
    ZONE_CONNECTION GetZoneConnection() const { return m_zoneConnection; }

    ZONE_CONNECTION GetLocalZoneConnectionOverride( wxString* aSource = nullptr ) const;

    /**
     * Set the width of the thermal spokes connecting the pad to a zone.  If != 0 this will
     * override similar settings in the parent footprint and zone.
     */
    void SetThermalSpokeWidth( int aWidth ) { m_thermalSpokeWidth = aWidth; }
    int GetThermalSpokeWidth() const { return m_thermalSpokeWidth; }

    int GetLocalSpokeWidthOverride( wxString* aSource = nullptr ) const;

    /**
     * The orientation of the thermal spokes.  45° will produce an X (the default for circular
     * pads and circular-anchored custom shaped pads), while 90° will produce a + (the default
     * for all other shapes).
     */
    void SetThermalSpokeAngle( const EDA_ANGLE& aAngle ) { m_thermalSpokeAngle = aAngle; }
    EDA_ANGLE GetThermalSpokeAngle() const { return m_thermalSpokeAngle; }

    // For property system
    void SetThermalSpokeAngleDegrees( double aAngle )
    {
        m_thermalSpokeAngle = EDA_ANGLE( aAngle, DEGREES_T );
    }
    double GetThermalSpokeAngleDegrees() const
    {
        return m_thermalSpokeAngle.AsDegrees();
    }

    void SetThermalGap( int aGap ) { m_thermalGap = aGap; }
    int GetThermalGap() const { return m_thermalGap; }

    int GetLocalThermalGapOverride( wxString* aSource = nullptr ) const;

    /**
     * Has meaning only for rounded rectangle pads.
     *
     * @return The radius of the rounded corners for this pad.
     */
    void SetRoundRectCornerRadius( double aRadius );
    int GetRoundRectCornerRadius() const;

    VECTOR2I ShapePos() const;

    /**
     * Has meaning only for rounded rectangle pads.
     *
     * Set the ratio between the smaller X or Y size and the rounded corner radius.
     * Cannot be > 0.5; the normalized IPC-7351C value is 0.25
     */
    void SetRoundRectRadiusRatio( double aRadiusScale );
    double GetRoundRectRadiusRatio() const { return m_roundedCornerScale; }

    /**
     * Has meaning only for chamfered rectangular pads.
     *
     * Set the ratio between the smaller X or Y size and chamfered corner size.
     * Cannot be < 0.5.
     */
    void SetChamferRectRatio( double aChamferScale );
    double GetChamferRectRatio() const { return m_chamferScale; }

    /**
     * Has meaning only for chamfered rectangular pads.
     *
     * Set the position of the chamfers for orientation 0.
     *
     * @param aPositions a bit-set of #RECT_CHAMFER_POSITIONS.
     */
    void SetChamferPositions( int aPositions ) { m_chamferPositions = aPositions; }
    int GetChamferPositions() const { return m_chamferPositions; }

    /**
     * @return the netcode.
     */
    int GetSubRatsnest() const                  { return m_subRatsnest; }
    void SetSubRatsnest( int aSubRatsnest )     { m_subRatsnest = aSubRatsnest; }

    /**
     * Set the unconnected removal property.
     *
     * If true, the copper is removed on zone fill or when specifically requested when the pad
     * is not connected on a layer.  This requires that there be a through hole.
     */
    void SetRemoveUnconnected( bool aSet )      { m_removeUnconnectedLayer = aSet; }
    bool GetRemoveUnconnected() const           { return m_removeUnconnectedLayer; }

    /**
     * Set whether we keep the top and bottom connections even if they are not connected.
     */
    void SetKeepTopBottom( bool aSet )      { m_keepTopBottomLayer = aSet; }
    bool GetKeepTopBottom() const           { return m_keepTopBottomLayer; }

    bool ConditionallyFlashed( PCB_LAYER_ID aLayer ) const
    {
        if( !m_removeUnconnectedLayer )
            return false;

        if( m_keepTopBottomLayer && ( aLayer == F_Cu || aLayer == B_Cu ) )
            return false;

        return true;
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    bool IsOnLayer( PCB_LAYER_ID aLayer, bool aIncludeCourtyards = false ) const override
    {
        return m_layerMask[aLayer];
    }

    /**
     * Check to see whether the pad should be flashed on the specific layer.
     *
     * @param aLayer Layer to check for connectivity
     * @param aOnlyCheckIfPermitted indicates that the routine should just return whether or not
     *        a flashed connection is permitted on this layer (without checking for a connection)
     * @return true if connected by pad or track (or optionally zone)
     */
    bool FlashLayer( int aLayer, bool aOnlyCheckIfPermitted = false ) const;

    bool CanFlashLayer( int aLayer )
    {
        return FlashLayer( aLayer, true );
    }

    PCB_LAYER_ID GetLayer() const override;

    /**
     * @return the principal copper layer for SMD and CONN pads.
     */
    PCB_LAYER_ID GetPrincipalLayer() const;

    /**
     * Check to see if the pad should be flashed to any of the layers in the set.
     *
     * @param aLayers set of layers to check the via against
     * @return true if connected by pad or track (or optionally zone) on any of the associated
     *         layers
     */
    bool FlashLayer( LSET aLayers ) const;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    wxString GetClass() const override
    {
        return wxT( "PAD" );
    }

    /**
     * The bounding box is cached, so this will be efficient most of the time.
     */
    const BOX2I GetBoundingBox() const override;

    /**
     * Compare two pads and return 0 if they are equal.
     *
     * @return less than 0 if left less than right, 0 if equal, or greater than 0 if left
     *         greater than right.
     */
    static int Compare( const PAD* aPadRef, const PAD* aPadCmp );

    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_pos += aMoveVector;
        SetDirty();
    }

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

    BITMAPS GetMenuImage() const override;

    /**
     * @return the GUI-appropriate name of the shape.
     */
    wxString ShowPadShape() const;

    /**
     * @return the GUI-appropriate description of the pad type (attribute) : Std, SMD ...
     */
    wxString ShowPadAttr() const;

    EDA_ITEM* Clone() const override;

    /**
     * Same as Clone, but returns a PAD item.
     *
     * Useful mainly for python scripts, because Clone returns an EDA_ITEM.
     */
    PAD* ClonePad() const
    {
        return (PAD*) Clone();
    }

    /**
     * Rebuild the effective shape cache (and bounding box and radius) for the pad and clears
     * the dirty bit.
     */
    void BuildEffectiveShapes( PCB_LAYER_ID aLayer ) const;
    void BuildEffectivePolygon() const;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    double ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    virtual const BOX2I ViewBBox() const override;

    void ClearZoneLayerOverrides()
    {
        m_zoneLayerOverrides.fill( ZLO_NONE );
    }

    const ZONE_LAYER_OVERRIDE& GetZoneLayerOverride( PCB_LAYER_ID aLayer ) const
    {
        return m_zoneLayerOverrides.at( aLayer );
    }

    void SetZoneLayerOverride( PCB_LAYER_ID aLayer, ZONE_LAYER_OVERRIDE aOverride )
    {
        std::unique_lock<std::mutex> cacheLock( m_zoneLayerOverridesMutex );
        m_zoneLayerOverrides.at( aLayer ) = aOverride;
    }

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

private:
    void addPadPrimitivesToPolygon( SHAPE_POLY_SET* aMergedPolygon, int aError,
                                    ERROR_LOC aErrorLoc ) const;

private:
    wxString      m_number;             // Pad name (pin number in schematic)
    wxString      m_pinFunction;        // Pin name in schematic
    wxString      m_pinType;            // Pin electrical type in schematic

    VECTOR2I      m_pos; // Pad Position on board

    PAD_SHAPE     m_padShape;           // Shape: PAD_SHAPE::CIRCLE, PAD_SHAPE::RECT,
                                        //   PAD_SHAPE::OVAL, PAD_SHAPE::TRAPEZOID,
                                        //   PAD_SHAPE::ROUNDRECT, PAD_SHAPE_POLYGON
    /*
     * Editing definitions of primitives for custom pad shapes.  In local coordinates relative
     * to m_Pos (NOT shapePos) at orient 0.
     */
    std::vector<std::shared_ptr<PCB_SHAPE>>   m_editPrimitives;

    // Must be set to true to force rebuild shapes to draw (after geometry change for instance)
    mutable bool                              m_shapesDirty;
    mutable std::mutex                        m_shapesBuildingLock;
    mutable BOX2I                             m_effectiveBoundingBox;
    mutable std::shared_ptr<SHAPE_COMPOUND>   m_effectiveShape;
    mutable std::shared_ptr<SHAPE_SEGMENT>    m_effectiveHoleShape;

    mutable bool                              m_polyDirty;
    mutable std::mutex                        m_polyBuildingLock;
    mutable std::shared_ptr<SHAPE_POLY_SET>   m_effectivePolygon;
    mutable int                               m_effectiveBoundingRadius;

    /*
     * How to build the custom shape in zone, to create the clearance area:
     * CUST_PAD_SHAPE_IN_ZONE_OUTLINE = use pad shape
     * CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL = use the convex hull of the pad shape
     */
    CUST_PAD_SHAPE_IN_ZONE  m_customShapeClearanceArea;

    int               m_subRatsnest;        // Variable used to handle subnet (block) number in
                                            //   ratsnest computations

    VECTOR2I          m_drill;              // Drill diameter (x == y) or slot dimensions (x != y)
    VECTOR2I          m_size;               // X and Y size (relative to orient 0)

    PAD_DRILL_SHAPE_T m_drillShape;         // PAD_DRILL_SHAPE_CIRCLE, PAD_DRILL_SHAPE_OBLONG

    double            m_roundedCornerScale; // Scaling factor of min(width, height) to corner
                                            //   radius, default 0.25
    double            m_chamferScale;       // Scaling factor of min(width, height) to chamfer
                                            //   size, default 0.25
    int               m_chamferPositions;   // The positions of the chamfers (at orient 0)

    PAD_SHAPE         m_anchorPadShape;     // For custom shaped pads: shape of pad anchor,
                                            //   PAD_SHAPE::RECT, PAD_SHAPE::CIRCLE

    /*
     * Most of the time the hole is the center of the shape (m_Offset = 0). But some designers
     * use oblong/rect pads with a hole moved to one of the oblong/rect pad shape ends.
     * In all cases the hole is at the pad position.  This offset is from the hole to the center
     * of the pad shape (ie: the copper area around the hole).
     * ShapePos() returns the board shape position according to the offset and the pad rotation.
     */
    VECTOR2I    m_offset;

    LSET        m_layerMask;        // Bitwise layer: 1 = copper layer, 15 = cmp,
                                    // 2..14 = internal layers, 16..31 = technical layers

    VECTOR2I    m_deltaSize;        // Delta for PAD_SHAPE::TRAPEZOID; half the delta squeezes
                                    //   one end and half expands the other.  It is only valid
                                    //   to have a single axis be non-0.

    PAD_ATTRIB  m_attribute;        // PAD_ATTRIB_NORMAL, PAD_ATTRIB::SMD, PAD_ATTRIB::CONN,
                                    //   PAD_ATTRIB::NPTH
    PAD_PROP    m_property;         // Property in fab files (BGA, FIDUCIAL, TESTPOINT, etc.)

    EDA_ANGLE   m_orient;

    int         m_lengthPadToDie;   // Length net from pad to die, inside the package

    ///< If true, the pad copper is removed for layers that are not connected.
    bool        m_removeUnconnectedLayer;

    ///< When removing unconnected pads, keep the top and bottom pads.
    bool        m_keepTopBottomLayer;

    /*
     * Pad clearances, margins, etc. exist in a hierarchy.  If a given level is specified then
     * the remaining levels are NOT consulted.
     *
     * LEVEL 1: (highest priority) local overrides (pad, footprint, etc.)
     * LEVEL 2: Rules
     * LEVEL 3: Accumulated local settings, netclass settings, & board design settings
     *
     * These are the LEVEL 1 settings for a pad.
     */
    int         m_localClearance;
    int         m_localSolderMaskMargin;        // Local solder mask margin
    int         m_localSolderPasteMargin;       // Local solder paste margin absolute value
    double      m_localSolderPasteMarginRatio;  // Local solder mask margin ratio of pad size
                                                // The final margin is the sum of these 2 values

    ZONE_CONNECTION m_zoneConnection;           // No connection, thermal relief, etc.
    int         m_thermalSpokeWidth;            // Thermal spoke width.
    EDA_ANGLE   m_thermalSpokeAngle;            // Rotation of the spokes.  45° will produce an X,
                                                //   while 90° will produce a +.
    int         m_thermalGap;

    std::mutex                                     m_zoneLayerOverridesMutex;
    std::array<ZONE_LAYER_OVERRIDE, MAX_CU_LAYERS> m_zoneLayerOverrides;
};

#endif  // PAD_H
