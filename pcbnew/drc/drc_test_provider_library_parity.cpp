/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021-2023 KiCad Developers.
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

#include <kiway.h>
#include <macros.h>
#include <netlist_reader/pcb_netlist.h>
#include <fp_lib_table.h>
#include <board.h>
#include <pcb_shape.h>
#include <zone.h>
#include <footprint.h>
#include <pad.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <project_pcb.h>

/*
    Library parity test.

    Errors generated:
    - DRCE_LIB_FOOTPRINT_ISSUES
    - DRCE_LIB_FOOTPRINT_MISMATCH
*/

class DRC_TEST_PROVIDER_LIBRARY_PARITY : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_LIBRARY_PARITY()
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_LIBRARY_PARITY()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "library_parity" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Performs board footprint vs library integity checks" );
    }
};


//
// The TEST*() macros have two modes:
// In "Report" mode (aReporter != nullptr) all properties are checked and reported on.
// In "DRC" mode (aReporter == nulltpr) properties are only checked until a difference is found.
//
#define TEST( a, b, msg )                                   \
        do {                                                \
            if( a != b )                                    \
            {                                               \
                diff = true;                                \
                                                            \
                if( aReporter && wxString( msg ).length() ) \
                    aReporter->Report( msg );               \
            }                                               \
                                                            \
            if( diff && !aReporter )                        \
                return diff;                                \
        } while (0)

#define EPSILON 1
#define TEST_PT( a, b, msg )                                \
        do {                                                \
            if( abs( a.x - b.x ) > EPSILON                  \
                    || abs( a.y - b.y ) > EPSILON )         \
            {                                               \
                diff = true;                                \
                                                            \
                if( aReporter && wxString( msg ).length() ) \
                    aReporter->Report( msg );               \
            }                                               \
                                                            \
            if( diff && !aReporter )                        \
                return diff;                                \
        } while (0)

#define EPSILON_D 0.000001
#define TEST_D( a, b, msg )                                 \
        do {                                                \
            if( abs( a - b ) > EPSILON_D )                  \
            {                                               \
                diff = true;                                \
                                                            \
                if( aReporter && wxString( msg ).length() ) \
                    aReporter->Report( msg );               \
            }                                               \
                                                            \
            if( diff && !aReporter )                        \
                return diff;                                \
        } while (0)

#define ITEM_DESC( item ) ( item )->GetItemDescription( &g_unitsProvider )
#define PAD_DESC( pad ) wxString::Format( _( "Pad %s" ), ( pad )->GetNumber() )


UNITS_PROVIDER g_unitsProvider( pcbIUScale, EDA_UNITS::MILLIMETRES );


bool primitiveNeedsUpdate( const std::shared_ptr<PCB_SHAPE>& a,
                           const std::shared_ptr<PCB_SHAPE>& b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetShape(), b->GetShape(), "" );

    switch( a->GetShape() )
    {
    case SHAPE_T::RECTANGLE:
    {
        BOX2I aRect( a->GetStart(), a->GetEnd() - a->GetStart() );
        BOX2I bRect( b->GetStart(), b->GetEnd() - b->GetStart() );

        aRect.Normalize();
        bRect.Normalize();

        TEST_PT( aRect.GetOrigin(), bRect.GetOrigin(), "" );
        TEST_PT( aRect.GetEnd(), bRect.GetEnd(), "" );
        break;
    }

    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
        TEST_PT( a->GetStart(), b->GetStart(), "" );
        TEST_PT( a->GetEnd(), b->GetEnd(), "" );
        break;

    case SHAPE_T::ARC:
        TEST_PT( a->GetStart(), b->GetStart(), "" );
        TEST_PT( a->GetEnd(), b->GetEnd(), "" );

        // Arc center is calculated and so may have round-off errors when parents are
        // differentially rotated.
        if( ( a->GetCenter() - b->GetCenter() ).EuclideanNorm() > pcbIUScale.mmToIU( 0.0005 ) )
            return true;

        break;

    case SHAPE_T::BEZIER:
        TEST_PT( a->GetStart(), b->GetStart(), "" );
        TEST_PT( a->GetEnd(), b->GetEnd(), "" );
        TEST_PT( a->GetBezierC1(), b->GetBezierC1(), "" );
        TEST_PT( a->GetBezierC2(), b->GetBezierC2(), "" );
        break;

    case SHAPE_T::POLY:
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices(), "" );

        for( int ii = 0; ii < a->GetPolyShape().TotalVertices(); ++ii )
            TEST_PT( a->GetPolyShape().CVertex( ii ), b->GetPolyShape().CVertex( ii ), "" );

        break;

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    TEST( a->GetStroke(), b->GetStroke(), "" );
    TEST( a->IsFilled(), b->IsFilled(), "" );

    return diff;
}


bool padHasOverrides( const PAD* a, const PAD* b, REPORTER* aReporter )
{
    bool diff = false;

    TEST( a->GetLocalClearance(), b->GetLocalClearance(),
          wxString::Format( _( "%s has clearance override." ), PAD_DESC( a ) ) );
    TEST( a->GetLocalSolderMaskMargin(), b->GetLocalSolderMaskMargin(),
          wxString::Format( _( "%s has solder mask expansion override." ), PAD_DESC( a ) ) );
    TEST( a->GetLocalSolderPasteMargin(), b->GetLocalSolderPasteMargin(),
          wxString::Format( _( "%s has solder paste clearance override." ), PAD_DESC( a ) ) );
    TEST_D( a->GetLocalSolderPasteMarginRatio(), b->GetLocalSolderPasteMarginRatio(),
          wxString::Format( _( "%s has solder paste clearance override." ), PAD_DESC( a ) ) );

    TEST( a->GetZoneConnection(), b->GetZoneConnection(),
          wxString::Format( _( "%s has zone connection override." ), PAD_DESC( a ) ) );
    TEST( a->GetThermalGap(), b->GetThermalGap(),
          wxString::Format( _( "%s has thermal relief gap override." ), PAD_DESC( a ) ) );
    TEST( a->GetThermalSpokeWidth(), b->GetThermalSpokeWidth(),
          wxString::Format( _( "%s has thermal relief spoke width override." ), PAD_DESC( a ) ) );
    TEST_D( a->GetThermalSpokeAngle().AsDegrees(), b->GetThermalSpokeAngle().AsDegrees(),
            wxString::Format( _( "%s has thermal relief spoke angle override." ), PAD_DESC( a ) ) );
    TEST( a->GetCustomShapeInZoneOpt(), b->GetCustomShapeInZoneOpt(),
          wxString::Format( _( "%s has zone knockout setting override." ), PAD_DESC( a ) ) );

    return diff;
}


bool padNeedsUpdate( const PAD* a, const PAD* b, REPORTER* aReporter )
{
    bool      diff = false;

    TEST( a->GetPadToDieLength(), b->GetPadToDieLength(),
          wxString::Format( _( "%s pad to die length differs." ), PAD_DESC( a ) ) );
    TEST_PT( a->GetFPRelativePosition(), b->GetFPRelativePosition(),
             wxString::Format( _( "%s position differs." ), PAD_DESC( a ) ) );

    TEST( a->GetNumber(), b->GetNumber(),
          wxString::Format( _( "%s has different numbers." ), PAD_DESC( a ) ) );

    // These are assigned from the schematic and not from the library
    // TEST( a->GetPinFunction(), b->GetPinFunction() );
    // TEST( a->GetPinType(), b->GetPinType() );

    bool layerSettingsDiffer = a->GetRemoveUnconnected() != b->GetRemoveUnconnected();

    // NB: KeepTopBottom is undefined if RemoveUnconnected is NOT set.
    if( a->GetRemoveUnconnected() )
        layerSettingsDiffer |= a->GetKeepTopBottom() != b->GetKeepTopBottom();

    // Trim layersets to the current board before comparing
    LSET enabledLayers = a->GetBoard()->GetEnabledLayers();
    LSET aLayers = a->GetLayerSet() & enabledLayers;
    LSET bLayers = b->GetLayerSet() & enabledLayers;

    if( layerSettingsDiffer || aLayers != bLayers )
    {
        diff = true;

        if( aReporter )
            aReporter->Report( wxString::Format( _( "%s layers differ." ), PAD_DESC( a ) ) );
        else
            return true;
    }

    TEST( a->GetShape(), b->GetShape(),
          wxString::Format( _( "%s pad shape type differs." ), PAD_DESC( a ) ) );

    TEST( a->GetAttribute(), b->GetAttribute(),
          wxString::Format( _( "%s pad type differs." ), PAD_DESC( a ) ) );
    TEST( a->GetProperty(), b->GetProperty(),
          wxString::Format( _( "%s fabrication property differs." ), PAD_DESC( a ) ) );

    // The pad orientation, for historical reasons is the pad rotation + parent rotation.
    TEST_D( ( a->GetOrientation() - a->GetParentFootprint()->GetOrientation() ).Normalize().AsDegrees(),
            ( b->GetOrientation() - b->GetParentFootprint()->GetOrientation() ).Normalize().AsDegrees(),
            wxString::Format( _( "%s orientation differs." ), PAD_DESC( a ) ) );

    TEST( a->GetSize(), b->GetSize(),
          wxString::Format( _( "%s size differs." ), PAD_DESC( a ) ) );
    TEST( a->GetDelta(), b->GetDelta(),
          wxString::Format( _( "%s trapezoid delta differs." ), PAD_DESC( a ) ) );

    if( a->GetRoundRectCornerRadius() != b->GetRoundRectCornerRadius()
        || a->GetRoundRectRadiusRatio() != b->GetRoundRectRadiusRatio() )
    {
        diff = true;

        if( aReporter )
            aReporter->Report( wxString::Format( _( "%s rounded corners differ." ), PAD_DESC( a ) ) );
        else
            return true;
    }

    if( a->GetChamferRectRatio() != b->GetChamferRectRatio()
        || a->GetChamferPositions() != b->GetChamferPositions() )
    {
        diff = true;

        if( aReporter )
            aReporter->Report( wxString::Format( _( "%s chamfered corners differ." ), PAD_DESC( a ) ) );
        else
            return true;
    }

    TEST_PT( a->GetOffset(), b->GetOffset(),
             wxString::Format( _( "%s shape offset from hole differs." ), PAD_DESC( a ) ) );

    TEST( a->GetDrillShape(), b->GetDrillShape(),
          wxString::Format( _( "%s drill shape differs." ), PAD_DESC( a ) ) );
    TEST( a->GetDrillSize(), b->GetDrillSize(),
          wxString::Format( _( "%s drill size differs." ), PAD_DESC( a ) ) );

    // Clearance and zone connection overrides are as likely to be set at the board level as in
    // the library.
    //
    // If we ignore them and someone *does* change one of them in the library, then stale
    // footprints won't be caught.
    //
    // On the other hand, if we report them then boards that override at the board level are
    // going to be VERY noisy.
    //
    // So we just do it when we have a reporter.
    if( aReporter && padHasOverrides( a, b, aReporter ) )
        diff = true;

    bool primitivesDiffer = false;

    if( a->GetPrimitives().size() != b->GetPrimitives().size() )
    {
        primitivesDiffer = true;
    }
    else
    {
        for( size_t ii = 0; ii < a->GetPrimitives().size(); ++ii )
        {
            if( primitiveNeedsUpdate( a->GetPrimitives()[ii], b->GetPrimitives()[ii] ) )
            {
                primitivesDiffer = true;
                break;
            }
        }
    }

    if( primitivesDiffer )
    {
        diff = true;

        if( aReporter )
            aReporter->Report( wxString::Format( _( "%s shape primitives differ." ), PAD_DESC( a ) ) );
        else
            return true;
    }

    return diff;
}


bool shapeNeedsUpdate( const PCB_SHAPE* a, const PCB_SHAPE* b )
{
    // Preliminary test: if a shape is a rectangle and the other is a polygon,
    // try to convert the polygon to a rectangle for comparison, because some transforms
    // ( and especially PCB_SHAPE::Normalize() ) can convert a polygon to a rectangle
    // So a poly and a rectangle can be in fact the same shape
    if( a->GetShape() == SHAPE_T::POLY && b->GetShape() == SHAPE_T::RECTANGLE )
    {
        PCB_SHAPE rect_test( *a );
        rect_test.Normalize();

        if( rect_test.GetShape() == SHAPE_T::RECTANGLE )
            return shapeNeedsUpdate( &rect_test, b );
    }
    else if( a->GetShape() == SHAPE_T::RECTANGLE && b->GetShape() == SHAPE_T::POLY )
    {
        PCB_SHAPE rect_test( *b );
        rect_test.Normalize();

        if( rect_test.GetShape() == SHAPE_T::RECTANGLE )
            return shapeNeedsUpdate( a, &rect_test );
    }

    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetShape(), b->GetShape(), "" );

    switch( a->GetShape() )
    {
    case SHAPE_T::RECTANGLE:
    {
        BOX2I aRect( a->GetStart(), a->GetEnd() - a->GetStart() );
        BOX2I bRect( b->GetStart(), b->GetEnd() - b->GetStart() );

        aRect.Normalize();
        bRect.Normalize();

        TEST_PT( aRect.GetOrigin(), bRect.GetOrigin(), "" );
        TEST_PT( aRect.GetEnd(), bRect.GetEnd(), "" );
        break;
    }

    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
        TEST_PT( a->GetStart(), b->GetStart(), "" );
        TEST_PT( a->GetEnd(), b->GetEnd(), "" );
        break;

    case SHAPE_T::ARC:
        TEST_PT( a->GetStart(), b->GetStart(), "" );
        TEST_PT( a->GetEnd(), b->GetEnd(), "" );

        // Arc center is calculated and so may have round-off errors when parents are
        // differentially rotated.
        if( ( a->GetCenter() - b->GetCenter() ).EuclideanNorm() > pcbIUScale.mmToIU( 0.0005 ) )
            return true;

        break;

    case SHAPE_T::BEZIER:
        TEST_PT( a->GetStart(), b->GetStart(), "" );
        TEST_PT( a->GetEnd(), b->GetEnd(), "" );
        TEST_PT( a->GetBezierC1(), b->GetBezierC1(), "" );
        TEST_PT( a->GetBezierC2(), b->GetBezierC2(), "" );
        break;

    case SHAPE_T::POLY:
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices(), "" );

        for( int ii = 0; ii < a->GetPolyShape().TotalVertices(); ++ii )
            TEST_PT( a->GetPolyShape().CVertex( ii ), b->GetPolyShape().CVertex( ii ), "" );

        break;

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    if( a->IsOnCopperLayer() )
        TEST( a->GetStroke(), b->GetStroke(), "" );

    TEST( a->IsFilled(), b->IsFilled(), "" );

    TEST( a->GetLayer(), b->GetLayer(), "" );

    return diff;
}


bool zoneNeedsUpdate( const ZONE* a, const ZONE* b, REPORTER* aReporter )
{
    bool diff = false;

    TEST( a->GetCornerSmoothingType(), b->GetCornerSmoothingType(),
          wxString::Format( _( "%s corner smoothing setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetCornerRadius(), b->GetCornerRadius(),
          wxString::Format( _( "%s corner smoothing radius differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetZoneName(), b->GetZoneName(),
          wxString::Format( _( "%s name differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetAssignedPriority(), b->GetAssignedPriority(),
          wxString::Format( _( "%s priority differs." ), ITEM_DESC( a ) ) );

    TEST( a->GetIsRuleArea(), b->GetIsRuleArea(),
          wxString::Format( _( "%s keep-out property differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowCopperPour(), b->GetDoNotAllowCopperPour(),
          wxString::Format( _( "%s keep out copper fill setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowFootprints(), b->GetDoNotAllowFootprints(),
          wxString::Format( _( "%s keep out footprints setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowPads(), b->GetDoNotAllowPads(),
          wxString::Format( _( "%s keep out pads setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowTracks(), b->GetDoNotAllowTracks(),
          wxString::Format( _( "%s keep out tracks setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowVias(), b->GetDoNotAllowVias(),
          wxString::Format( _( "%s keep out vias setting differs." ), ITEM_DESC( a ) ) );

    TEST( a->GetLayerSet(), b->GetLayerSet(),
          wxString::Format( _( "%s layers differ." ), ITEM_DESC( a ) ) );

    TEST( a->GetPadConnection(), b->GetPadConnection(),
          wxString::Format( _( "%s pad connection property differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetLocalClearance(), b->GetLocalClearance(),
          wxString::Format( _( "%s local clearance differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetThermalReliefGap(), b->GetThermalReliefGap(),
          wxString::Format( _( "%s thermal relief gap differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetThermalReliefSpokeWidth(), b->GetThermalReliefSpokeWidth(),
          wxString::Format( _( "%s thermal relief spoke width differs." ), ITEM_DESC( a ) ) );

    TEST( a->GetMinThickness(), b->GetMinThickness(),
          wxString::Format( _( "%s min thickness differs." ), ITEM_DESC( a ) ) );

    TEST( a->GetIslandRemovalMode(), b->GetIslandRemovalMode(),
          wxString::Format( _( "%s remove islands setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetMinIslandArea(), b->GetMinIslandArea(),
              wxString::Format( _( "%s minimum island size setting differs." ), ITEM_DESC( a ) ) );

    TEST( a->GetFillMode(), b->GetFillMode(),
          wxString::Format( _( "%s fill type differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetHatchThickness(), b->GetHatchThickness(),
          wxString::Format( _( "%s hatch width differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetHatchGap(), b->GetHatchGap(),
          wxString::Format( _( "%s hatch gap differs." ), ITEM_DESC( a ) ) );
    TEST_D( a->GetHatchOrientation().AsDegrees(), b->GetHatchOrientation().AsDegrees(),
          wxString::Format( _( "%s hatch orientation differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetHatchSmoothingLevel(), b->GetHatchSmoothingLevel(),
          wxString::Format( _( "%s hatch smoothing level differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetHatchSmoothingValue(), b->GetHatchSmoothingValue(),
          wxString::Format( _( "%s hatch smoothing amount differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetHatchHoleMinArea(), b->GetHatchHoleMinArea(),
          wxString::Format( _( "%s minimum hatch hole setting differs." ), ITEM_DESC( a ) ) );

    // This is just a display property
    // TEST( a->GetHatchBorderAlgorithm(), b->GetHatchBorderAlgorithm() );

    TEST( a->Outline()->TotalVertices(), b->Outline()->TotalVertices(),
          wxString::Format( _( "%s outline corner count differs." ), ITEM_DESC( a ) ) );

    bool cornersDiffer = false;

    for( int ii = 0; ii < a->Outline()->TotalVertices(); ++ii )
    {
        if( a->Outline()->CVertex( ii ) != b->Outline()->CVertex( ii ) )
        {
            diff = true;
            cornersDiffer = true;
            break;
        }
    }

    if( cornersDiffer && aReporter )
        aReporter->Report( wxString::Format( _( "%s corners differ." ), ITEM_DESC( a ) ) );

    return diff;
}


bool FOOTPRINT::FootprintNeedsUpdate( const FOOTPRINT* aLibFP, REPORTER* aReporter )
{
    UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MILLIMETRES );

    wxASSERT( aLibFP );
    bool diff = false;

    // To avoid issues when comparing the footprint on board and the footprint in library
    // use a footprint not flipped, not rotated and at position 0,0.
    // Otherwise one can see differences when comparing coordinates of some items
    if( IsFlipped() || GetPosition() != VECTOR2I( 0, 0 ) || GetOrientation() != ANGLE_0 )
    {
        std::unique_ptr<FOOTPRINT> temp( static_cast<FOOTPRINT*>( Clone() ) );
        temp->SetParentGroup( nullptr );

        if( IsFlipped() )
            temp->Flip( {0,0}, false );

        if( GetPosition() != VECTOR2I( 0, 0 ) )
            temp->SetPosition( { 0, 0 } );

        if( GetOrientation() != ANGLE_0 )
            temp->SetOrientation( ANGLE_0 );

        for( BOARD_ITEM* item : temp->GraphicalItems() )
            item->Normalize();

        diff = temp->FootprintNeedsUpdate( aLibFP, aReporter );

        // This temporary footprint must not have a parent when it goes out of scope because it
        // must not trigger the IncrementTimestamp call in ~FOOTPRINT.
        temp->SetParent( nullptr );
        return diff;
    }

    TEST( GetLibDescription(), aLibFP->GetLibDescription(), _( "Footprint descriptions differ." ) );
    TEST( GetKeywords(), aLibFP->GetKeywords(), _( "Footprint keywords differ." ) );

#define TEST_ATTR( a, b, attr, msg ) TEST( ( a & attr ), ( b & attr ), msg )

    TEST_ATTR( GetAttributes(), aLibFP->GetAttributes(), (FP_THROUGH_HOLE | FP_SMD),
               _( "Footprint types differ." ) );
    TEST_ATTR( GetAttributes(), aLibFP->GetAttributes(), FP_ALLOW_SOLDERMASK_BRIDGES,
               _( "Allow bridged solder mask apertures between pads settings differ." ) );
    TEST_ATTR( GetAttributes(), aLibFP->GetAttributes(), FP_ALLOW_MISSING_COURTYARD,
               _( "Exempt from courtyard requirement settings differ." ) );

    // Clearance and zone connection overrides are as likely to be set at the board level as in
    // the library.
    //
    // If we ignore them and someone *does* change one of them in the library, then stale
    // footprints won't be caught.
    //
    // On the other hand, if we report them then boards that override at the board level are
    // going to be VERY noisy.
    //
    // For now we report them if there's a reporter, but we DON'T generate DRC errors on them.
    if( aReporter )
    {
        TEST( GetLocalClearance(), aLibFP->GetLocalClearance(),
              _( "Pad clearance overridden." ) );
        TEST( GetLocalSolderMaskMargin(), aLibFP->GetLocalSolderMaskMargin(),
              _( "Solder mask expansion overridden." ) );
        TEST( GetLocalSolderPasteMargin(), aLibFP->GetLocalSolderPasteMargin(),
              _( "Solder paste absolute clearance overridden." ) );
        TEST_D( GetLocalSolderPasteMarginRatio(), aLibFP->GetLocalSolderPasteMarginRatio(),
                _( "Solder paste relative clearance overridden." ) );

        TEST( GetZoneConnection(), aLibFP->GetZoneConnection(),
              _( "Zone connection overridden." ) );
    }

    TEST( GetNetTiePadGroups().size(), aLibFP->GetNetTiePadGroups().size(),
          _( "Net tie pad groups differ." ) );

    for( size_t ii = 0; ii < GetNetTiePadGroups().size(); ++ii )
    {
        TEST( GetNetTiePadGroups()[ii], aLibFP->GetNetTiePadGroups()[ii],
              _( "Net tie pad groups differ." ) );
    }

#define REPORT( msg ) { if( aReporter ) aReporter->Report( msg ); }
#define CHECKPOINT { if( diff && !aReporter ) return diff; }

    // Text items are really problematic.  We don't want to test the reference, but after that
    // it gets messy.
    //
    // What about the value?  Depends on whether or not it's a singleton part.
    //
    // And what about other texts?  They might be added only to instances on the board, or even
    // changed for instances on the board.  Or they might want to be tested for equality.
    //
    // Currently we punt and ignore all the text items.

    // Drawings and pads are also somewhat problematic as there's no guarantee that they'll be
    // in the same order in the two footprints.  Rather than building some sophisticated hashing
    // algorithm we use the footprint sorting functions to attempt to sort them in the same
    // order.

    std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> aShapes;
    std::copy_if( GraphicalItems().begin(), GraphicalItems().end(),
                  std::inserter( aShapes, aShapes.begin() ),
                  []( BOARD_ITEM* item )
                  {
                      return item->Type() == PCB_SHAPE_T;
                  } );
    std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> bShapes;
    std::copy_if( aLibFP->GraphicalItems().begin(), aLibFP->GraphicalItems().end(),
                  std::inserter( bShapes, bShapes.begin() ),
                  []( BOARD_ITEM* item )
                  {
                      return item->Type() == PCB_SHAPE_T;
                  } );

    if( aShapes.size() != bShapes.size() )
    {
        diff = true;
        REPORT( _( "Graphic item count differs." ) );
    }
    else
    {
        for( auto aIt = aShapes.begin(), bIt = bShapes.begin(); aIt != aShapes.end(); aIt++, bIt++ )
        {
            if( shapeNeedsUpdate( static_cast<PCB_SHAPE*>( *aIt ), static_cast<PCB_SHAPE*>( *bIt ) ) )
            {
                diff = true;
                REPORT( wxString::Format( _( "%s differs." ), ITEM_DESC( *aIt ) ) );
            }
        }
    }

    CHECKPOINT;

    std::set<PAD*, FOOTPRINT::cmp_pads> aPads( Pads().begin(), Pads().end() );
    std::set<PAD*, FOOTPRINT::cmp_pads> bPads( aLibFP->Pads().begin(), aLibFP->Pads().end() );

    if( aPads.size() != bPads.size() )
    {
        diff = true;
        REPORT( _( "Pad count differs." ) );
    }
    else
    {
        for( auto aIt = aPads.begin(), bIt = bPads.begin(); aIt != aPads.end(); aIt++, bIt++ )
        {
            if( padNeedsUpdate( *aIt, *bIt, aReporter ) )
                diff = true;
            else if( aReporter && padHasOverrides( *aIt, *bIt, aReporter ) )
                diff = true;
        }
    }

    CHECKPOINT;

    // Rotate/position a copy of libFootprint so that zones sort the same
    std::unique_ptr<FOOTPRINT> libCopy( static_cast<FOOTPRINT*>( aLibFP->Clone() ) );

    libCopy->SetOrientation( GetOrientation() );
    libCopy->Move( GetPosition() );

    std::set<ZONE*, FOOTPRINT::cmp_zones> aZones( Zones().begin(), Zones().end() );
    std::set<ZONE*, FOOTPRINT::cmp_zones> bZones( libCopy->Zones().begin(), libCopy->Zones().end() );

    if( aZones.size() != bZones.size() )
    {
        diff = true;
        REPORT( _( "Rule area count differs." ) );
    }
    else
    {
        for( auto aIt = aZones.begin(), bIt = bZones.begin(); aIt != aZones.end(); aIt++, bIt++ )
            diff |= zoneNeedsUpdate( *aIt, *bIt, aReporter );
    }

    return diff;
}


bool DRC_TEST_PROVIDER_LIBRARY_PARITY::Run()
{
    BOARD*   board = m_drcEngine->GetBoard();
    PROJECT* project = board->GetProject();

    if( !project )
    {
        reportAux( _( "No project loaded, skipping library parity tests." ) );
        return true;    // Continue with other tests
    }

    if( !reportPhase( _( "Loading footprint library table..." ) ) )
        return false;   // DRC cancelled

    std::map<LIB_ID, std::shared_ptr<FOOTPRINT>> libFootprintCache;

    FP_LIB_TABLE* libTable = PROJECT_PCB::PcbFootprintLibs( project );
    wxString      msg;
    int           ii = 0;
    const int     progressDelta = 250;

    if( !reportPhase( _( "Checking board footprints against library..." ) ) )
        return false;

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_ISSUES )
                && m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_MISMATCH ) )
        {
            return true;    // Continue with other tests
        }

        if( !reportProgress( ii++, (int) board->Footprints().size(), progressDelta ) )
            return false;   // DRC cancelled

        LIB_ID               fpID = footprint->GetFPID();
        wxString             libName = fpID.GetLibNickname();
        wxString             fpName = fpID.GetLibItemName();
        const LIB_TABLE_ROW* libTableRow = nullptr;

        try
        {
            libTableRow = libTable->FindRow( libName );
        }
        catch( const IO_ERROR& )
        {
        }

        if( !libTableRow )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_ISSUES ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
                msg.Printf( _( "The current configuration does not include the library '%s'." ),
                            libName );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetCenter(), UNDEFINED_LAYER );
            }

            continue;
        }
        else if( !libTable->HasLibrary( libName, true ) )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_ISSUES ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
                msg.Printf( _( "The library '%s' is not enabled in the current configuration." ),
                            libName );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetCenter(), UNDEFINED_LAYER );
            }

            continue;
        }

        auto                       cacheIt = libFootprintCache.find( fpID );
        std::shared_ptr<FOOTPRINT> libFootprint;

        if( cacheIt != libFootprintCache.end() )
        {
            libFootprint = cacheIt->second;
        }
        else
        {
            try
            {
                libFootprint.reset( libTable->FootprintLoad( libName, fpName, true ) );

                if( libFootprint )
                    libFootprintCache[ fpID ] = libFootprint;
            }
            catch( const IO_ERROR& )
            {
            }
        }

        if( !libFootprint )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_ISSUES ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
                msg.Printf( _( "Footprint '%s' not found in library '%s'." ),
                            fpName,
                            libName );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetCenter(), UNDEFINED_LAYER );
            }
        }
        else if( footprint->FootprintNeedsUpdate( libFootprint.get() ) )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_MISMATCH ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_MISMATCH );
                msg.Printf( _( "Footprint '%s' does not match copy in library '%s'." ),
                            fpName,
                            libName );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetCenter(), UNDEFINED_LAYER );
            }
        }
    }

    return true;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_LIBRARY_PARITY> dummy;
}
