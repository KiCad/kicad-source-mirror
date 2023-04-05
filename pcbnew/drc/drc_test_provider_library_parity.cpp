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

#define EPSILON 0.000001
#define TEST_D( a, b, msg )                                 \
        do {                                                \
            if( abs( a - b ) > EPSILON )                    \
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

#define TEST_V3D( a, b, msg )                               \
        do {                                                \
            if( abs( a.x - b.x ) > EPSILON                  \
                    || abs( a.y - b.y ) > EPSILON           \
                    || abs( a.z - b.z ) > EPSILON )         \
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

UNITS_PROVIDER g_unitsProvider( pcbIUScale, EDA_UNITS::MILLIMETRES );


bool primitiveNeedsUpdate( const std::shared_ptr<PCB_SHAPE>& a,
                           const std::shared_ptr<PCB_SHAPE>& b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetShape(), b->GetShape(), "" );

    switch( a->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
        TEST( a->GetStart(), b->GetStart(), "" );
        TEST( a->GetEnd(), b->GetEnd(), "" );
        break;

    case SHAPE_T::ARC:
        TEST( a->GetStart(), b->GetStart(), "" );
        TEST( a->GetEnd(), b->GetEnd(), "" );
        TEST( a->GetCenter(), b->GetCenter(), "" );
        TEST_D( a->GetArcAngle().AsDegrees(), b->GetArcAngle().AsDegrees(), "" );
        break;

    case SHAPE_T::BEZIER:
        TEST( a->GetStart(), b->GetStart(), "" );
        TEST( a->GetEnd(), b->GetEnd(), "" );
        TEST( a->GetBezierC1(), b->GetBezierC1(), "" );
        TEST( a->GetBezierC2(), b->GetBezierC2(), "" );
        break;

    case SHAPE_T::POLY:
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices(), "" );

        for( int ii = 0; ii < a->GetPolyShape().TotalVertices(); ++ii )
            TEST( a->GetPolyShape().CVertex( ii ), b->GetPolyShape().CVertex( ii ), "" );

        break;

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    TEST( a->GetStroke(), b->GetStroke(), "" );
    TEST( a->IsFilled(), b->IsFilled(), "" );

    return diff;
}


bool padNeedsUpdate( const PAD* a, const PAD* b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetPadToDieLength(), b->GetPadToDieLength(), "" );
    TEST( a->GetFPRelativePosition(), b->GetFPRelativePosition(), "" );

    TEST( a->GetNumber(), b->GetNumber(), "" );

    // These are assigned from the schematic and not from the library
    // TEST( a->GetPinFunction(), b->GetPinFunction() );
    // TEST( a->GetPinType(), b->GetPinType() );

    TEST( a->GetRemoveUnconnected(), b->GetRemoveUnconnected(), "" );

    // NB: KeepTopBottom is undefined if RemoveUnconnected is NOT set.
    if( a->GetRemoveUnconnected() )
        TEST( a->GetKeepTopBottom(), b->GetKeepTopBottom(), "" );

    TEST( a->GetShape(), b->GetShape(), "" );

    // Trim layersets to the current board before comparing
    LSET enabledLayers = a->GetBoard()->GetEnabledLayers();
    LSET aLayers = a->GetLayerSet() & enabledLayers;
    LSET bLayers = b->GetLayerSet() & enabledLayers;
    TEST( aLayers, bLayers, "" );

    TEST( a->GetAttribute(), b->GetAttribute(), "" );
    TEST( a->GetProperty(), b->GetProperty(), "" );

    // The pad orientation, for historical reasons is the pad rotation + parent rotation.
    TEST_D( ( a->GetOrientation() - a->GetParent()->GetOrientation() ).Normalize().AsDegrees(),
            ( b->GetOrientation() - b->GetParent()->GetOrientation() ).Normalize().AsDegrees(),
            "" );

    TEST( a->GetSize(), b->GetSize(), "" );
    TEST( a->GetDelta(), b->GetDelta(), "" );
    TEST( a->GetRoundRectCornerRadius(), b->GetRoundRectCornerRadius(), "" );
    TEST_D( a->GetRoundRectRadiusRatio(), b->GetRoundRectRadiusRatio(), "" );
    TEST_D( a->GetChamferRectRatio(), b->GetChamferRectRatio(), "" );
    TEST( a->GetChamferPositions(), b->GetChamferPositions(), "" );
    TEST( a->GetOffset(), b->GetOffset(), "" );

    TEST( a->GetDrillShape(), b->GetDrillShape(), "" );
    TEST( a->GetDrillSize(), b->GetDrillSize(), "" );

    // Clearance and zone connection overrides are as likely to be set at the board level as in
    // the library.
    //
    // If we ignore them and someone *does* change one of them in the library, then stale
    // footprints won't be caught.
    //
    // On the other hand, if we report them then boards that override at the board level are
    // going to be VERY noisy.
#if 0
    if( padHasOverrides( a, b )
        return true;
#endif

    TEST( a->GetPrimitives().size(), b->GetPrimitives().size(), "" );

    for( size_t ii = 0; ii < a->GetPrimitives().size(); ++ii )
    {
        if( primitiveNeedsUpdate( a->GetPrimitives()[ii], b->GetPrimitives()[ii] ) )
            return true;
    }

    return diff;
}


bool padHasOverrides( const PAD* a, const PAD* b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetLocalClearance(), b->GetLocalClearance(), "" );
    TEST( a->GetLocalSolderMaskMargin(), b->GetLocalSolderMaskMargin(), "" );
    TEST( a->GetLocalSolderPasteMargin(), b->GetLocalSolderPasteMargin(), "" );
    TEST_D( a->GetLocalSolderPasteMarginRatio(), b->GetLocalSolderPasteMarginRatio(), "" );

    TEST( a->GetZoneConnection(), b->GetZoneConnection(), "" );
    TEST( a->GetThermalGap(), b->GetThermalGap(), "" );
    TEST( a->GetThermalSpokeWidth(), b->GetThermalSpokeWidth(), "" );
    TEST_D( a->GetThermalSpokeAngle().AsDegrees(), b->GetThermalSpokeAngle().AsDegrees(), "" );
    TEST( a->GetCustomShapeInZoneOpt(), b->GetCustomShapeInZoneOpt(), "" );

    return diff;
}


bool shapeNeedsUpdate( const PCB_SHAPE* a, const PCB_SHAPE* b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetShape(), b->GetShape(), "" );

    switch( a->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
        TEST( a->GetStart(), b->GetStart(), "" );
        TEST( a->GetEnd(), b->GetEnd(), "" );
        break;

    case SHAPE_T::ARC:
        TEST( a->GetStart(), b->GetStart(), "" );
        TEST( a->GetEnd(), b->GetEnd(), "" );

        // Arc center is calculated and so may have round-off errors when parents are
        // differentially rotated.
        if( ( a->GetCenter() - b->GetCenter() ).EuclideanNorm() > pcbIUScale.mmToIU( 0.0001 ) )
            return true;

        break;

    case SHAPE_T::BEZIER:
        TEST( a->GetStart(), b->GetStart(), "" );
        TEST( a->GetEnd(), b->GetEnd(), "" );
        TEST( a->GetBezierC1(), b->GetBezierC1(), "" );
        TEST( a->GetBezierC2(), b->GetBezierC2(), "" );
        break;

    case SHAPE_T::POLY:
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices(), "" );

        for( int ii = 0; ii < a->GetPolyShape().TotalVertices(); ++ii )
            TEST( a->GetPolyShape().CVertex( ii ), b->GetPolyShape().CVertex( ii ), "" );

        break;

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    TEST( a->GetStroke(), b->GetStroke(), "" );
    TEST( a->IsFilled(), b->IsFilled(), "" );

    TEST( a->GetLayer(), b->GetLayer(), "" );

    return diff;
}


bool textNeedsUpdate( const PCB_TEXT* a, const PCB_TEXT* b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetLayer(), b->GetLayer(), "" );
    TEST( a->IsKeepUpright(), b->IsKeepUpright(), "" );

    TEST( a->GetText(), b->GetText(), "" );

    TEST( a->GetTextThickness(), b->GetTextThickness(), "" );
    TEST( a->GetTextAngle(), b->GetTextAngle(), "" );
    TEST( a->IsItalic(), b->IsItalic(), "" );
    TEST( a->IsBold(), b->IsBold(), "" );
    TEST( a->IsVisible(), b->IsVisible(), "" );
    TEST( a->IsMirrored(), b->IsMirrored(), "" );

    TEST( a->GetHorizJustify(), b->GetHorizJustify(), "" );
    TEST( a->GetVertJustify(), b->GetVertJustify(), "" );

    TEST( a->GetTextSize(), b->GetTextSize(), "" );
    TEST( a->GetFPRelativePosition(), b->GetFPRelativePosition(), "" );

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


bool modelNeedsUpdate( const FP_3DMODEL& a, const FP_3DMODEL& b, REPORTER* aReporter )
{
    bool diff = false;

    TEST_V3D( a.m_Scale, b.m_Scale, _( "3D model scale doesn't match: " ) + a.m_Filename );
    TEST_V3D( a.m_Rotation, b.m_Rotation, _( "3D model rotation doesn't match: " ) + a.m_Filename );
    TEST_V3D( a.m_Offset, b.m_Offset, _( "3D model offset doesn't match: " ) + a.m_Filename );
    TEST( a.m_Opacity, b.m_Opacity, _( "3D model opacity doesn't match: " ) + a.m_Filename );
    TEST( a.m_Filename, b.m_Filename, _( "3D model doesn't match: " ) + a.m_Filename );
    TEST( a.m_Show, b.m_Show, _( "3D model visibility doesn't match: " ) + a.m_Filename );

    return diff;
}


bool FOOTPRINT::FootprintNeedsUpdate( const FOOTPRINT* aLibFootprint, REPORTER* aReporter )
{
    UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MILLIMETRES );

    wxASSERT( aLibFootprint );
    bool diff = false;

    // To avoid issues when comparing the footprint on board and the footprint in library
    // use a footprint not flipped, not rotated and at position 0,0.
    // Otherwise one can see differences when comparing coordinates of some items
    if( IsFlipped() || GetPosition() != VECTOR2I( 0, 0 )  || GetOrientation() != ANGLE_0 )
    {
        std::unique_ptr<FOOTPRINT> temp( static_cast<FOOTPRINT*>( Clone() ) );
        temp->SetParentGroup( nullptr );

        if( IsFlipped() )
            temp->Flip( {0,0}, false );

        if( GetPosition() != VECTOR2I( 0, 0 ) )
            temp->SetPosition( { 0, 0 } );

        if( GetOrientation() != ANGLE_0 )
            temp->SetOrientation( ANGLE_0 );

        diff = temp->FootprintNeedsUpdate( aLibFootprint, aReporter );

        // This temporary footprint must not have a parent when it goes out of scope because it must
        // not trigger the IncrementTimestamp call in ~FOOTPRINT.
        temp->SetParent( nullptr );
        return diff;
    }

    TEST( GetDescription(), aLibFootprint->GetDescription(),
          _( "Footprint descriptions differ." ) );
    TEST( GetKeywords(), aLibFootprint->GetKeywords(),
          _( "Footprint keywords differ." ) );

#define TEST_ATTR( a, b, attr, msg ) TEST( ( a & attr ), ( b & attr ), msg )

    TEST_ATTR( GetAttributes(), aLibFootprint->GetAttributes(), (FP_THROUGH_HOLE | FP_SMD),
              _( "Footprint types differ." ) );
    TEST_ATTR( GetAttributes(), aLibFootprint->GetAttributes(), FP_ALLOW_SOLDERMASK_BRIDGES,
               _( "Allow bridged solder mask apertures between pads settings differ." ) );
    TEST_ATTR( GetAttributes(), aLibFootprint->GetAttributes(), FP_ALLOW_MISSING_COURTYARD,
               _( "Exempt from courtyard requirement settings differ." ) );

    // Clearance and zone connection overrides are as likely to be set at the board level as in
    // the library.
    //
    // If we ignore them and someone *does* change one of them in the library, then stale
    // footprints won't be caught.
    //
    // On the other hand, if we report them then boards that override at the board level are
    // going to be VERY noisy.
    if( aReporter )
    {
        TEST( GetLocalClearance(), aLibFootprint->GetLocalClearance(),
              _( "Pad clearance overridden." ) );
        TEST( GetLocalSolderMaskMargin(), aLibFootprint->GetLocalSolderMaskMargin(),
              _( "Solder mask expansion overridden." ) );
        TEST( GetLocalSolderPasteMargin(), aLibFootprint->GetLocalSolderPasteMargin(),
              _( "Solder paste absolute clearance overridden." ) );
        TEST_D( GetLocalSolderPasteMarginRatio(), aLibFootprint->GetLocalSolderPasteMarginRatio(),
                _( "Solder paste relative clearance overridden." ) );

        TEST( GetZoneConnection(), aLibFootprint->GetZoneConnection(),
              _( "Zone connection overridden." ) );
    }

    TEST( GetNetTiePadGroups().size(), aLibFootprint->GetNetTiePadGroups().size(),
          _( "Net tie pad groups differ." ) );

    for( size_t ii = 0; ii < GetNetTiePadGroups().size(); ++ii )
    {
        TEST( GetNetTiePadGroups()[ii], aLibFootprint->GetNetTiePadGroups()[ii],
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
    std::copy_if( aLibFootprint->GraphicalItems().begin(), aLibFootprint->GraphicalItems().end(),
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
            if( ( *aIt )->Type() == PCB_SHAPE_T )
            {
                if( shapeNeedsUpdate( static_cast<PCB_SHAPE*>( *aIt ), static_cast<PCB_SHAPE*>( *bIt ) ) )
                {
                    diff = true;
                    REPORT( wxString::Format( _( "%s differs." ), ITEM_DESC( *aIt ) ) );
                }
            }
        }
    }

    CHECKPOINT;

    std::set<PAD*, FOOTPRINT::cmp_pads> aPads( Pads().begin(), Pads().end() );
    std::set<PAD*, FOOTPRINT::cmp_pads> bPads( aLibFootprint->Pads().begin(), aLibFootprint->Pads().end() );

    if( aPads.size() != bPads.size() )
    {
        diff = true;
        REPORT( _( "Pad count differs." ) );
    }
    else
    {
        for( auto aIt = aPads.begin(), bIt = bPads.begin(); aIt != aPads.end(); aIt++, bIt++ )
        {
            if( padNeedsUpdate( *aIt, *bIt ) )
            {
                diff = true;
                REPORT( wxString::Format( _( "Pad %s differs." ), (*aIt)->GetNumber() ) );
            }
            else if( aReporter && padHasOverrides( *aIt, *bIt ) )
            {
                diff = true;
                REPORT( wxString::Format( _( "Pad %s has overrides." ), (*aIt)->GetNumber() ) );
            }
        }
    }

    CHECKPOINT;

    if( Models().size() != aLibFootprint->Models().size() )
    {
        diff = true;
        REPORT( _( "3D model count differs." ) );
    }
    else
    {
        for( size_t ii = 0; ii < Models().size(); ++ii )
            diff |= modelNeedsUpdate( Models()[ii], aLibFootprint->Models()[ii], aReporter );
    }

    CHECKPOINT;

    // Rotate/position a copy of libFootprint so that zones sort the same
    std::unique_ptr<FOOTPRINT> libCopy( static_cast<FOOTPRINT*>( aLibFootprint->Clone() ) );

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

    FP_LIB_TABLE* libTable = project->PcbFootprintLibs();
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
