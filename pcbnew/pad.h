/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <mutex>
#include <array>

#include <board_connected_item.h>
#include <core/arraydim.h>
#include <core/mirror.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_compound.h>
#include <lset.h>
#include <padstack.h>
#include <zones.h>

class PCB_SHAPE;
class SHAPE;
class SHAPE_SEGMENT;

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

    void CopyFrom( const BOARD_ITEM* aOther ) override;

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

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

    bool HasDrilledHole() const override
    {
        return HasHole() && GetDrillSizeX() == GetDrillSizeY();
    }

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
    void SetShape( PCB_LAYER_ID aLayer, PAD_SHAPE aShape )
    {
        m_padStack.SetShape( aShape, aLayer );
        SetDirty();
    }

    /**
     * @return the shape of this pad.
     */
    PAD_SHAPE GetShape( PCB_LAYER_ID aLayer ) const { return m_padStack.Shape( aLayer ); }

    // Used for the properties panel, which does not support padstacks at the moment
    void SetFrontShape( PAD_SHAPE aShape );

    PAD_SHAPE GetFrontShape() const { return m_padStack.Shape( F_Cu ); }

    void SetPosition( const VECTOR2I& aPos ) override
    {
        m_pos = aPos;
        SetDirty();
    }

    VECTOR2I GetPosition() const override { return m_pos; }

    /**
     * @return the shape of the anchor pad shape, for custom shaped pads.
     */
    PAD_SHAPE GetAnchorPadShape( PCB_LAYER_ID aLayer ) const
    {
        return m_padStack.AnchorShape( aLayer );
    }

    /**
     * @return the option for the custom pad shape to use as clearance area in copper zones.
     */
    PADSTACK::CUSTOM_SHAPE_ZONE_MODE GetCustomShapeInZoneOpt() const
    {
        return m_padStack.CustomShapeInZoneMode();
    }

    /**
     * Set the option for the custom pad shape to use as clearance area in copper zones.
     *
     * @param aOption is the clearance area shape CUST_PAD_SHAPE_IN_ZONE option
     */
    void SetCustomShapeInZoneOpt( PADSTACK::CUSTOM_SHAPE_ZONE_MODE aOption )
    {
        m_padStack.SetCustomShapeInZoneMode( aOption );
    }

    /**
     * Set the shape of the anchor pad for custom shaped pads.
     *
     * @param aShape is the shape of the anchor pad shape( currently, only #PAD_SHAPE::RECTANGLE or
     *               #PAD_SHAPE::CIRCLE.
     */
    void SetAnchorPadShape( PCB_LAYER_ID aLayer, PAD_SHAPE aShape )
    {
        m_padStack.SetAnchorShape( aShape == PAD_SHAPE::RECTANGLE
                                   ? PAD_SHAPE::RECTANGLE
                                   : PAD_SHAPE::CIRCLE,
                                   aLayer );
        SetDirty();
    }

    /**
     * @return true if the pad is on any copper layer, false otherwise.
     */
    bool IsOnCopperLayer() const override;

    void SetY( int y )                          { m_pos.y = y; SetDirty(); }
    void SetX( int x )                          { m_pos.x = x; SetDirty(); }

    void SetSize( PCB_LAYER_ID aLayer, const VECTOR2I& aSize )
    {
        m_padStack.SetSize( aSize, aLayer );
        SetDirty();
    }
    const VECTOR2I& GetSize( PCB_LAYER_ID aLayer ) const { return m_padStack.Size( aLayer ); }

    // These accessors are for the properties panel, which does not have the ability to deal with
    // custom padstacks where the properties can vary by layer.  The properties should be disabled
    // in the GUI when the padstack mode is set to anything other than NORMAL, but so that the code
    // compiles, these are set up to work with the front layer (in other words, assume the mode is
    // NORMAL, where F_Cu stores the whole padstack data)
    void SetSizeX( const int aX )
    {
        if( aX > 0 )
        {
            m_padStack.SetSize( { aX, m_padStack.Size( PADSTACK::ALL_LAYERS ).y }, PADSTACK::ALL_LAYERS );
            SetDirty();
        }
    }

    int GetSizeX() const { return m_padStack.Size( PADSTACK::ALL_LAYERS ).x; }

    void SetSizeY( const int aY )
    {
        if( aY > 0 )
        {
            m_padStack.SetSize( { m_padStack.Size( PADSTACK::ALL_LAYERS ).x, aY }, PADSTACK::ALL_LAYERS );
            SetDirty();
        }
    }

    int GetSizeY() const { return m_padStack.Size( PADSTACK::ALL_LAYERS ).y; }

    void SetDelta( PCB_LAYER_ID aLayer, const VECTOR2I& aSize )
    {
        m_padStack.TrapezoidDeltaSize( aLayer ) = aSize;
        SetDirty();
    }

    const VECTOR2I& GetDelta( PCB_LAYER_ID aLayer ) const
    {
        return m_padStack.TrapezoidDeltaSize( aLayer );
    }

    void SetDrillSize( const VECTOR2I& aSize )  { m_padStack.Drill().size = aSize; SetDirty(); }
    const VECTOR2I& GetDrillSize() const        { return m_padStack.Drill().size; }
    void SetDrillSizeX( const int aX );
    int GetDrillSizeX() const                   { return m_padStack.Drill().size.x; }
    void SetDrillSizeY( const int aY )          { m_padStack.Drill().size.y = aY; SetDirty(); }
    int GetDrillSizeY() const                   { return m_padStack.Drill().size.y; }

    void SetOffset( PCB_LAYER_ID aLayer, const VECTOR2I& aOffset )
    {
        m_padStack.Offset( aLayer ) = aOffset;
        SetDirty();
    }

    const VECTOR2I& GetOffset( PCB_LAYER_ID aLayer ) const { return m_padStack.Offset( aLayer ); }

    VECTOR2I GetCenter() const override          { return GetPosition(); }

    const PADSTACK& Padstack() const              { return m_padStack; }
    PADSTACK& Padstack()                          { return m_padStack; }
    void SetPadstack( const PADSTACK& aPadstack ) { m_padStack = aPadstack; }

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
    void AddPrimitivePoly( PCB_LAYER_ID aLayer, const SHAPE_POLY_SET& aPoly, int aThickness,
                           bool aFilled );
    void AddPrimitivePoly( PCB_LAYER_ID aLayer, const std::vector<VECTOR2I>& aPoly, int aThickness,
                           bool aFilled );

    /**
     * Merge all basic shapes to a #SHAPE_POLY_SET.
     *
     * @note The results are relative to the pad position, orientation 0.
     *
     * @param aLayer is the copper layer to merge shapes for
     * @param aMergedPolygon will store the final polygon
     * @param aErrorLoc is used when a circle (or arc) is approximated by segments
     *  = ERROR_INSIDE to build a polygon inside the arc/circle (usual shape to raw/plot)
     *  = ERROR_OUIDE to build a polygon outside the arc/circle
     * (for instance when building a clearance area)
     */
    void MergePrimitivesAsPolygon( PCB_LAYER_ID aLayer, SHAPE_POLY_SET* aMergedPolygon,
                                   ERROR_LOC aErrorLoc = ERROR_INSIDE ) const;

    /**
     * Clear the basic shapes list.
     * @param aLayer is the layer to clear, or UNDEFINED_LAYER to clear all layers
     */
    void DeletePrimitivesList( PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

    /**
     * Accessor to the basic shape list for custom-shaped pads.
     */
    const std::vector<std::shared_ptr<PCB_SHAPE>>& GetPrimitives( PCB_LAYER_ID aLayer ) const
    {
        return m_padStack.Primitives( aLayer );
    }

    void Flip( const VECTOR2I& VECTOR2I, FLIP_DIRECTION aFlipDirection ) override;

    /**
     * Flip (mirror) the primitives left to right or top to bottom, around the anchor position
     * in custom pads.
     */
    void FlipPrimitives( FLIP_DIRECTION aFlipDirection );

    /**
     * Clear the current custom shape primitives list and import a new list.  Copies the input,
     * which is not altered.
     */
    void ReplacePrimitives( PCB_LAYER_ID aLayer,
                            const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList );

    /**
     * Import a custom shape primitive list (composed of basic shapes) and add items to the
     * current list.  Copies the input, which is not altered.
     */
    void AppendPrimitives( PCB_LAYER_ID aLayer,
                           const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList );

    /**
     * Add item to the custom shape primitives list
     */
    void AddPrimitive( PCB_LAYER_ID aLayer, PCB_SHAPE* aPrimitive );

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
    EDA_ANGLE GetOrientation() const { return m_padStack.GetOrientation(); }
    EDA_ANGLE GetFPRelativeOrientation() const;

    // For property system
    void SetOrientationDegrees( double aOrientation )
    {
        SetOrientation( EDA_ANGLE( aOrientation, DEGREES_T ) );
    }
    double GetOrientationDegrees() const
    {
        return m_padStack.GetOrientation().AsDegrees();
    }

    void SetDrillShape( PAD_DRILL_SHAPE aShape );
    PAD_DRILL_SHAPE GetDrillShape() const { return m_padStack.Drill().shape; }

    bool IsDirty() const
    {
        return m_shapesDirty || m_polyDirty[ERROR_INSIDE] || m_polyDirty[ERROR_OUTSIDE];
    }

    void SetDirty()
    {
        m_shapesDirty = true;
        m_polyDirty[ERROR_INSIDE] = true;
        m_polyDirty[ERROR_OUTSIDE] = true;
    }

    void SetLayerSet( const LSET& aLayers ) override   { m_padStack.SetLayerSet( aLayers ); SetDirty(); }
    LSET GetLayerSet() const override           { return m_padStack.LayerSet(); }

    void SetAttribute( PAD_ATTRIB aAttribute );
    PAD_ATTRIB GetAttribute() const             { return m_attribute; }

    void SetProperty( PAD_PROP aProperty );
    PAD_PROP GetProperty() const                { return m_property; }

    // We don't currently have an attribute for APERTURE, and adding one will change the file
    // format, so for now just infer a copper-less pad to be an APERTURE pad.
    bool IsAperturePad() const
    {
        return ( m_padStack.LayerSet() & LSET::AllCuMask() ).none();
    }

    void SetPadToDieLength( int aLength )       { m_lengthPadToDie = aLength; }
    int GetPadToDieLength() const               { return m_lengthPadToDie; }

    void SetPadToDieDelay( int aDelay ) { m_delayPadToDie = aDelay; }
    int  GetPadToDieDelay() const { return m_delayPadToDie; }

    std::optional<int> GetLocalClearance() const override   { return m_padStack.Clearance(); }
    void SetLocalClearance( std::optional<int> aClearance ) { m_padStack.Clearance() = aClearance; }

    std::optional<int> GetLocalSolderMaskMargin() const { return m_padStack.SolderMaskMargin(); }
    void               SetLocalSolderMaskMargin( std::optional<int> aMargin )
    {
        m_padStack.SolderMaskMargin( F_Mask ) = aMargin;
        m_padStack.SolderMaskMargin( B_Mask ) = aMargin;
    }

    std::optional<int> GetLocalSolderPasteMargin() const { return m_padStack.SolderPasteMargin(); }
    void               SetLocalSolderPasteMargin( std::optional<int> aMargin )
    {
        m_padStack.SolderPasteMargin( F_Paste ) = aMargin;
        m_padStack.SolderPasteMargin( B_Paste ) = aMargin;
    }

    std::optional<double> GetLocalSolderPasteMarginRatio() const
    {
        return m_padStack.SolderPasteMarginRatio();
    }
    void SetLocalSolderPasteMarginRatio( std::optional<double> aRatio )
    {
        m_padStack.SolderPasteMarginRatio( F_Paste ) = aRatio;
        m_padStack.SolderPasteMarginRatio( B_Paste ) = aRatio;
    }

    void SetLocalZoneConnection( ZONE_CONNECTION aType ) { m_padStack.ZoneConnection() = aType; }
    ZONE_CONNECTION GetLocalZoneConnection() const
    {
        return m_padStack.ZoneConnection().value_or( ZONE_CONNECTION::INHERITED );
    }

    /**
     * Return the pad's "own" clearance in internal units.
     *
     * @param aLayer the layer in question.
     * @param aSource [out] optionally reports the source as a user-readable string.
     * @return the clearance in internal units.
     */
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
                                  int aMaxError, ERROR_LOC aErrorLoc = ERROR_INSIDE,
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
                                 ERROR_LOC aErrorLoc = ERROR_INSIDE ) const;

    /**
     * Some pad shapes can be complex (rounded/chamfered rectangle), even without considering
     * custom shapes.  This routine returns a COMPOUND shape (set of simple shapes which make
     * up the pad for use with routing, collision determination, etc).
     *
     * @note This list can contain a SHAPE_SIMPLE (a simple single-outline non-intersecting
     * polygon), but should never contain a SHAPE_POLY_SET (a complex polygon consisting of
     * multiple outlines and/or holes).
     *
     * @param aLayer determines which layer to query for shape
     * @param aFlash optional parameter allowing a caller to force the pad to be flashed (or not
     *               flashed) on the current layer (default is to honour the pad's setting and
     *               the current connections for the given layer).
     */
    virtual std::shared_ptr<SHAPE>
    GetEffectiveShape( PCB_LAYER_ID aLayer,
                       FLASHING flashPTHPads = FLASHING::DEFAULT ) const override;

    const std::shared_ptr<SHAPE_POLY_SET>& GetEffectivePolygon( PCB_LAYER_ID aLayer,
                                                                ERROR_LOC aErrorLoc = ERROR_INSIDE ) const;

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
    std::optional<int> GetLocalClearance( wxString* aSource ) const override;

    /**
     * Return any clearance overrides set in the "classic" (ie: pre-rule) system.
     *
     * @param aSource [out] optionally reports the source as a user-readable string.
     * @return the clearance in internal units.
     */
    std::optional<int> GetClearanceOverrides( wxString* aSource ) const override;

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
    int GetSolderMaskExpansion( PCB_LAYER_ID aLayer ) const;

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
    VECTOR2I GetSolderPasteMargin( PCB_LAYER_ID aLayer ) const;

    ZONE_CONNECTION GetZoneConnectionOverrides( wxString* aSource = nullptr ) const;

    /**
     * Set the width of the thermal spokes connecting the pad to a zone.  If != 0 this will
     * override similar settings in the parent footprint and zone.
     */
    void SetLocalThermalSpokeWidthOverride( std::optional<int> aWidth )
    {
        m_padStack.ThermalSpokeWidth() = aWidth;
    }
    std::optional<int> GetLocalThermalSpokeWidthOverride() const
    {
        return m_padStack.ThermalSpokeWidth();
    }

    int GetLocalSpokeWidthOverride( wxString* aSource = nullptr ) const;

    /**
     * The orientation of the thermal spokes.  45° will produce an X (the default for circular
     * pads and circular-anchored custom shaped pads), while 90° will produce a + (the default
     * for all other shapes).
     */
    void SetThermalSpokeAngle( const EDA_ANGLE& aAngle )
    {
        m_padStack.SetThermalSpokeAngle( aAngle );
    }
    EDA_ANGLE GetThermalSpokeAngle() const
    {
        return m_padStack.ThermalSpokeAngle();
    }

    // For property system
    void SetThermalSpokeAngleDegrees( double aAngle )
    {
        m_padStack.SetThermalSpokeAngle( EDA_ANGLE( aAngle, DEGREES_T ) );
    }
    double GetThermalSpokeAngleDegrees() const
    {
        return m_padStack.ThermalSpokeAngle().AsDegrees();
    }

    void SetThermalGap( int aGap ) { m_padStack.ThermalGap() = aGap; }
    int GetThermalGap() const { return m_padStack.ThermalGap().value_or( 0 ); }

    int GetLocalThermalGapOverride( wxString* aSource ) const;

    std::optional<int> GetLocalThermalGapOverride() const
    {
        return m_padStack.ThermalGap();
    }
    void SetLocalThermalGapOverride( const std::optional<int>& aOverride )
    {
        m_padStack.ThermalGap() = aOverride;
    }

    /**
     * Has meaning only for rounded rectangle pads.
     *
     * @return The radius of the rounded corners for this pad.
     */
    void SetRoundRectCornerRadius( PCB_LAYER_ID aLayer, double aRadius );
    int GetRoundRectCornerRadius( PCB_LAYER_ID aLayer ) const;

    VECTOR2I ShapePos( PCB_LAYER_ID aLayer ) const;

    /**
     * Has meaning only for rounded rectangle pads.
     *
     * Set the ratio between the smaller X or Y size and the rounded corner radius.
     * Cannot be > 0.5; the normalized IPC-7351C value is 0.25
     */
    void SetRoundRectRadiusRatio( PCB_LAYER_ID aLayer, double aRadiusScale );
    double GetRoundRectRadiusRatio( PCB_LAYER_ID aLayer ) const
    {
        return m_padStack.RoundRectRadiusRatio( aLayer );
    }

    // For properties panel, which only supports normal padstacks
    void SetFrontRoundRectRadiusRatio( double aRadiusScale );
    double GetFrontRoundRectRadiusRatio() const
    {
       return m_padStack.RoundRectRadiusRatio( F_Cu );
    }

    // For properties panel, which only supports normal padstacks
    void SetFrontRoundRectRadiusSize( int aRadius );
    int  GetFrontRoundRectRadiusSize() const;

    /**
     * Has meaning only for chamfered rectangular pads.
     *
     * Set the ratio between the smaller X or Y size and chamfered corner size.
     * Cannot be < 0.5.
     */
    void SetChamferRectRatio( PCB_LAYER_ID aLayer, double aChamferScale );
    double GetChamferRectRatio( PCB_LAYER_ID aLayer ) const
    {
        return m_padStack.ChamferRatio( aLayer );
    }

    /**
     * Has meaning only for chamfered rectangular pads.
     *
     * Set the position of the chamfers for orientation 0.
     *
     * @param aPositions a bit-set of #RECT_CHAMFER_POSITIONS.
     */
    void SetChamferPositions( PCB_LAYER_ID aLayer, int aPositions )
    {
        m_padStack.SetChamferPositions( aPositions, aLayer );
    }

    int GetChamferPositions( PCB_LAYER_ID aLayer ) const
    {
        return m_padStack.ChamferPositions( aLayer );
    }

    /**
     * @return the netcode.
     */
    int GetSubRatsnest() const                  { return m_subRatsnest; }
    void SetSubRatsnest( int aSubRatsnest )     { m_subRatsnest = aSubRatsnest; }

    /**
     * @deprecated - use Padstack().SetUnconnectedLayerMode()
     * Sets the unconnected removal property.  If true, the copper is removed on zone fill
     * or when specifically requested when the via is not connected on a layer.
     */
    void SetRemoveUnconnected( bool aSet )
    {
        m_padStack.SetUnconnectedLayerMode( aSet
                ? PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL
                : PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL );
    }

    bool GetRemoveUnconnected() const
    {
        return m_padStack.UnconnectedLayerMode() != PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL;
    }

    /**
     * @deprecated - use Padstack().SetUnconnectedLayerMode()
     * Sets whether we keep the start and end annular rings even if they are not connected
     */
    void SetKeepTopBottom( bool aSet )
    {
        m_padStack.SetUnconnectedLayerMode( aSet
                ? PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END
                : PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL );
    }

    bool GetKeepTopBottom() const
    {
        return m_padStack.UnconnectedLayerMode()
               == PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END;
    }

    void SetUnconnectedLayerMode( PADSTACK::UNCONNECTED_LAYER_MODE aMode )
    {
        m_padStack.SetUnconnectedLayerMode( aMode );
    }

    PADSTACK::UNCONNECTED_LAYER_MODE GetUnconnectedLayerMode() const
    {
        return m_padStack.UnconnectedLayerMode();
    }

    bool ConditionallyFlashed( PCB_LAYER_ID aLayer ) const
    {
        switch( m_padStack.UnconnectedLayerMode() )
        {
        case PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL:
            return false;

        case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL:
            return true;

        case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END:
        case PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY:
            return aLayer != m_padStack.Drill().start && aLayer != m_padStack.Drill().end;
        }

        return true;
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    bool IsOnLayer( PCB_LAYER_ID aLayer ) const override
    {
        return m_padStack.LayerSet().test( aLayer );
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
    bool FlashLayer( const LSET& aLayers ) const;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    /**
     * Recombines the pad with other graphical shapes in the footprint
     *
     * @param aIsDryRun if true, the pad will not be recombined but the operation will still be logged
     * @param aMaxError the maximum error to allow for converting arcs to polygons
     * @return a list of shapes that were recombined
    */
    std::vector<PCB_SHAPE*> Recombine( bool aIsDryRun, int aMaxError );

    wxString GetClass() const override
    {
        return wxT( "PAD" );
    }

    /**
     * The bounding box is cached, so this will be efficient most of the time.
     */
    const BOX2I GetBoundingBox() const override;
    const BOX2I GetBoundingBox( PCB_LAYER_ID aLayer ) const;

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

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    /**
     * @return the GUI-appropriate name of the shape.
     */
    wxString ShowPadShape( PCB_LAYER_ID aLayer ) const;

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
    void BuildEffectiveShapes() const;
    void BuildEffectivePolygon( ERROR_LOC aErrorLoc = ERROR_INSIDE ) const;

    virtual std::vector<int> ViewGetLayers() const override;

    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

    virtual const BOX2I ViewBBox() const override;

    void ClearZoneLayerOverrides();

    const ZONE_LAYER_OVERRIDE& GetZoneLayerOverride( PCB_LAYER_ID aLayer ) const;

    void SetZoneLayerOverride( PCB_LAYER_ID aLayer, ZONE_LAYER_OVERRIDE aOverride );

    void CheckPad( UNITS_PROVIDER* aUnitsProvider, bool aForPadProperties,
                   const std::function<void( int aErrorCode,
                                             const wxString& aMsg )>& aErrorHandler ) const;

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const PAD& aOther ) const;
    bool operator==( const BOARD_ITEM& aBoardItem ) const override;

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

private:
    const SHAPE_COMPOUND& buildEffectiveShape( PCB_LAYER_ID aLayer ) const;

    void doCheckPad( PCB_LAYER_ID aLayer, UNITS_PROVIDER* aUnitsProvider, bool aForPadProperties,
                     const std::function<void( int aErrorCode,
                                               const wxString& aMsg )>& aErrorHandler ) const;

private:
    wxString      m_number;             // Pad name (pin number in schematic)
    wxString      m_pinFunction;        // Pin name in schematic
    wxString      m_pinType;            // Pin electrical type in schematic

    VECTOR2I      m_pos; // Pad Position on board

    PADSTACK      m_padStack;

    // Must be set to true to force rebuild shapes to draw (after geometry change for instance)
    typedef std::map<PCB_LAYER_ID, std::shared_ptr<SHAPE_COMPOUND>> LAYER_SHAPE_MAP;
    mutable bool                              m_shapesDirty;
    mutable std::mutex                        m_shapesBuildingLock;
    mutable BOX2I                             m_effectiveBoundingBox;
    mutable LAYER_SHAPE_MAP                   m_effectiveShapes;
    mutable std::shared_ptr<SHAPE_SEGMENT>    m_effectiveHoleShape;

    typedef std::map<PCB_LAYER_ID, std::array<std::shared_ptr<SHAPE_POLY_SET>, 2>> LAYER_POLYGON_MAP;
    mutable bool                              m_polyDirty[2];
    mutable std::mutex                        m_polyBuildingLock;
    mutable LAYER_POLYGON_MAP                 m_effectivePolygons;
    mutable int                               m_effectiveBoundingRadius;
    // Last zoom level used to draw the pad: the LAYER_PAD_HOLEWALLS layer shape
    // depend on the zoom level. So keep trace on the last used zoom level
    mutable double                            m_lastGalZoomLevel;

    int               m_subRatsnest;        // Variable used to handle subnet (block) number in
                                            //   ratsnest computations

    PAD_ATTRIB  m_attribute = PAD_ATTRIB::PTH;

    PAD_PROP    m_property;         // Property in fab files (BGA, FIDUCIAL, TESTPOINT, etc.)

    int m_lengthPadToDie; // Length net from pad to die, inside the package
    int m_delayPadToDie;  // Propagation delay from pad to die

    mutable std::mutex                          m_zoneLayerOverridesMutex;
    std::map<PCB_LAYER_ID, ZONE_LAYER_OVERRIDE> m_zoneLayerOverrides;
};
