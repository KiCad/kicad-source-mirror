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
#include <fp_shape.h>
#include <fp_text.h>
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
#define TEST( a, b, msg )                               \
        if( a != b )                                    \
        {                                               \
            diff = true;                                \
                                                        \
            if( aReporter )                             \
                aReporter->Report( msg );               \
        }                                               \
                                                        \
        if( diff && !aReporter )                        \
            return diff;                                \
        /* Prevent binding to else following macro */   \
        else {}

#define EPSILON 0.000001
#define TEST_D( a, b, msg )                             \
        if( abs( a - b ) > EPSILON )                    \
        {                                               \
            diff = true;                                \
                                                        \
            if( aReporter )                             \
                aReporter->Report( msg );               \
        }                                               \
                                                        \
        if( diff && !aReporter )                        \
            return diff;                                \
        /* Prevent binding to else following macro */   \
        else {}

#define TEST_V3D( a, b, msg )                           \
        if( abs( a.x - b.x ) > EPSILON                  \
                || abs( a.y - b.y ) > EPSILON           \
                || abs( a.z - b.z ) > EPSILON )         \
        {                                               \
            diff = true;                                \
                                                        \
            if( aReporter )                             \
                aReporter->Report( msg );               \
        }                                               \
                                                        \
        if( diff && !aReporter )                        \
            return diff;                                \
        /* Prevent binding to else following macro */   \
        else {}

bool primitiveNeedsUpdate( const std::shared_ptr<PCB_SHAPE>& a,
                           const std::shared_ptr<PCB_SHAPE>& b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetShape(), b->GetShape(), wxEmptyString );

    switch( a->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
        TEST( a->GetStart(), b->GetStart(), wxEmptyString );
        TEST( a->GetEnd(), b->GetEnd(), wxEmptyString );
        break;

    case SHAPE_T::ARC:
        TEST( a->GetStart(), b->GetStart(), wxEmptyString );
        TEST( a->GetEnd(), b->GetEnd(), wxEmptyString );
        TEST( a->GetCenter(), b->GetCenter(), wxEmptyString );
        TEST_D( a->GetArcAngle().AsDegrees(), b->GetArcAngle().AsDegrees(), wxEmptyString );
        break;

    case SHAPE_T::BEZIER:
        TEST( a->GetStart(), b->GetStart(), wxEmptyString );
        TEST( a->GetEnd(), b->GetEnd(), wxEmptyString );
        TEST( a->GetBezierC1(), b->GetBezierC1(), wxEmptyString );
        TEST( a->GetBezierC2(), b->GetBezierC2(), wxEmptyString );
        break;

    case SHAPE_T::POLY:
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices(), wxEmptyString);

        for( int ii = 0; ii < a->GetPolyShape().TotalVertices(); ++ii )
            TEST( a->GetPolyShape().CVertex( ii ), b->GetPolyShape().CVertex( ii ), wxEmptyString );

        break;

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    TEST( a->GetStroke(), b->GetStroke(), wxEmptyString );
    TEST( a->IsFilled(), b->IsFilled(), wxEmptyString );

    return diff;
}


bool padNeedsUpdate( const PAD* a, const PAD* b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetPadToDieLength(), b->GetPadToDieLength(), wxEmptyString );
    TEST( a->GetPos0(), b->GetPos0(), wxEmptyString );

    TEST( a->GetNumber(), b->GetNumber(), wxEmptyString );

    // These are assigned from the schematic and not from the library
    // TEST( a->GetPinFunction(), b->GetPinFunction() );
    // TEST( a->GetPinType(), b->GetPinType() );

    TEST( a->GetRemoveUnconnected(), b->GetRemoveUnconnected(), wxEmptyString );

    // NB: KeepTopBottom is undefined if RemoveUnconnected is NOT set.
    if( a->GetRemoveUnconnected() )
    {
        TEST( a->GetKeepTopBottom(), b->GetKeepTopBottom(), wxEmptyString );
    }

    TEST( a->GetShape(), b->GetShape(), wxEmptyString );

    // Trim layersets to the current board before comparing
    LSET enabledLayers = a->GetBoard()->GetEnabledLayers();
    LSET aLayers = a->GetLayerSet() & enabledLayers;
    LSET bLayers = b->GetLayerSet() & enabledLayers;
    TEST( aLayers, bLayers, wxEmptyString );

    TEST( a->GetAttribute(), b->GetAttribute(), wxEmptyString );
    TEST( a->GetProperty(), b->GetProperty(), wxEmptyString );

    // The pad orientation, for historical reasons is the pad rotation + parent rotation.
    TEST_D( ( a->GetOrientation() - a->GetParent()->GetOrientation() ).Normalize().AsDegrees(),
            ( b->GetOrientation() - b->GetParent()->GetOrientation() ).Normalize().AsDegrees(),
            wxEmptyString );

    TEST( a->GetSize(), b->GetSize(), wxEmptyString );
    TEST( a->GetDelta(), b->GetDelta(), wxEmptyString );
    TEST( a->GetRoundRectCornerRadius(), b->GetRoundRectCornerRadius(), wxEmptyString );
    TEST_D( a->GetRoundRectRadiusRatio(), b->GetRoundRectRadiusRatio(), wxEmptyString );
    TEST_D( a->GetChamferRectRatio(), b->GetChamferRectRatio(), wxEmptyString );
    TEST( a->GetChamferPositions(), b->GetChamferPositions(), wxEmptyString );
    TEST( a->GetOffset(), b->GetOffset(), wxEmptyString );

    TEST( a->GetDrillShape(), b->GetDrillShape(), wxEmptyString );
    TEST( a->GetDrillSize(), b->GetDrillSize(), wxEmptyString );

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

    TEST( a->GetPrimitives().size(), b->GetPrimitives().size(), wxEmptyString );

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

    TEST( a->GetLocalClearance(), b->GetLocalClearance(), wxEmptyString );
    TEST( a->GetLocalSolderMaskMargin(), b->GetLocalSolderMaskMargin(), wxEmptyString );
    TEST( a->GetLocalSolderPasteMargin(), b->GetLocalSolderPasteMargin(), wxEmptyString );
    TEST_D( a->GetLocalSolderPasteMarginRatio(), b->GetLocalSolderPasteMarginRatio(), wxEmptyString );

    TEST( a->GetZoneConnection(), b->GetZoneConnection(), wxEmptyString );
    TEST( a->GetThermalGap(), b->GetThermalGap(), wxEmptyString );
    TEST( a->GetThermalSpokeWidth(), b->GetThermalSpokeWidth(), wxEmptyString );
    TEST_D( a->GetThermalSpokeAngle().AsDegrees(), b->GetThermalSpokeAngle().AsDegrees(), wxEmptyString );
    TEST( a->GetCustomShapeInZoneOpt(), b->GetCustomShapeInZoneOpt(), wxEmptyString );

    return diff;
}


bool shapeNeedsUpdate( const FP_SHAPE* a, const FP_SHAPE* b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetShape(), b->GetShape(), wxEmptyString );

    switch( a->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
        TEST( a->GetStart0(), b->GetStart0(), wxEmptyString );
        TEST( a->GetEnd0(), b->GetEnd0(), wxEmptyString );
        break;

    case SHAPE_T::ARC:
        TEST( a->GetStart0(), b->GetStart0(), wxEmptyString );
        TEST( a->GetEnd0(), b->GetEnd0(), wxEmptyString );

        // Arc center is calculated and so may have round-off errors when parents are
        // differentially rotated.
        if( ( a->GetCenter0() - b->GetCenter0() ).EuclideanNorm() > pcbIUScale.mmToIU( 0.0001 ) )
            return true;

        break;

    case SHAPE_T::BEZIER:
        TEST( a->GetStart0(), b->GetStart0(), wxEmptyString );
        TEST( a->GetEnd0(), b->GetEnd0(), wxEmptyString );
        TEST( a->GetBezierC1_0(), b->GetBezierC1_0(), wxEmptyString );
        TEST( a->GetBezierC2_0(), b->GetBezierC2_0(), wxEmptyString );
        break;

    case SHAPE_T::POLY:
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices(), wxEmptyString );

        for( int ii = 0; ii < a->GetPolyShape().TotalVertices(); ++ii )
            TEST( a->GetPolyShape().CVertex( ii ), b->GetPolyShape().CVertex( ii ), wxEmptyString );

        break;

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    TEST( a->GetStroke(), b->GetStroke(), wxEmptyString );
    TEST( a->IsFilled(), b->IsFilled(), wxEmptyString );

    TEST( a->GetLayer(), b->GetLayer(), wxEmptyString );

    return diff;
}


bool textsNeedUpdate( const FP_TEXT* a, const FP_TEXT* b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetLayer(), b->GetLayer(), wxEmptyString );
    TEST( a->IsKeepUpright(), b->IsKeepUpright(), wxEmptyString );

    TEST( a->GetText(), b->GetText(), wxEmptyString );

    TEST( a->GetTextThickness(), b->GetTextThickness(), wxEmptyString );
    TEST( a->GetTextAngle(), b->GetTextAngle(), wxEmptyString );
    TEST( a->IsItalic(), b->IsItalic(), wxEmptyString );
    TEST( a->IsBold(), b->IsBold(), wxEmptyString );
    TEST( a->IsVisible(), b->IsVisible(), wxEmptyString );
    TEST( a->IsMirrored(), b->IsMirrored(), wxEmptyString );

    TEST( a->GetHorizJustify(), b->GetHorizJustify(), wxEmptyString );
    TEST( a->GetVertJustify(), b->GetVertJustify(), wxEmptyString );

    TEST( a->GetTextSize(), b->GetTextSize(), wxEmptyString );
    TEST( a->GetPos0(), b->GetPos0(), wxEmptyString );

    return diff;
}


bool zonesNeedUpdate( const FP_ZONE* a, const FP_ZONE* b )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( a->GetCornerSmoothingType(), b->GetCornerSmoothingType(), wxEmptyString );
    TEST( a->GetCornerRadius(), b->GetCornerRadius(), wxEmptyString );
    TEST( a->GetZoneName(), b->GetZoneName(), wxEmptyString );
    TEST( a->GetAssignedPriority(), b->GetAssignedPriority(), wxEmptyString );

    TEST( a->GetIsRuleArea(), b->GetIsRuleArea(), wxEmptyString );
    TEST( a->GetDoNotAllowCopperPour(), b->GetDoNotAllowCopperPour(), wxEmptyString );
    TEST( a->GetDoNotAllowFootprints(), b->GetDoNotAllowFootprints(), wxEmptyString );
    TEST( a->GetDoNotAllowPads(), b->GetDoNotAllowPads(), wxEmptyString );
    TEST( a->GetDoNotAllowTracks(), b->GetDoNotAllowTracks(), wxEmptyString );
    TEST( a->GetDoNotAllowVias(), b->GetDoNotAllowVias(), wxEmptyString );

    TEST( a->GetLayerSet(), b->GetLayerSet(), wxEmptyString );

    TEST( a->GetPadConnection(), b->GetPadConnection(), wxEmptyString );
    TEST( a->GetLocalClearance(), b->GetLocalClearance(), wxEmptyString );
    TEST( a->GetThermalReliefGap(), b->GetThermalReliefGap(), wxEmptyString );
    TEST( a->GetThermalReliefSpokeWidth(), b->GetThermalReliefSpokeWidth(), wxEmptyString );

    TEST( a->GetMinThickness(), b->GetMinThickness(), wxEmptyString );

    TEST( a->GetIslandRemovalMode(), b->GetIslandRemovalMode(), wxEmptyString );
    TEST( a->GetMinIslandArea(), b->GetMinIslandArea(), wxEmptyString );

    TEST( a->GetFillMode(), b->GetFillMode(), wxEmptyString );
    TEST( a->GetHatchThickness(), b->GetHatchThickness(), wxEmptyString );
    TEST( a->GetHatchGap(), b->GetHatchGap(), wxEmptyString );
    TEST_D( a->GetHatchOrientation().AsDegrees(), b->GetHatchOrientation().AsDegrees(), wxEmptyString );
    TEST( a->GetHatchSmoothingLevel(), b->GetHatchSmoothingLevel(), wxEmptyString );
    TEST( a->GetHatchSmoothingValue(), b->GetHatchSmoothingValue(), wxEmptyString );
    TEST( a->GetHatchHoleMinArea(), b->GetHatchHoleMinArea(), wxEmptyString );

    // This is just a display property
    // TEST( a->GetHatchBorderAlgorithm(), b->GetHatchBorderAlgorithm() );

    TEST( a->Outline()->TotalVertices(), b->Outline()->TotalVertices(), wxEmptyString );

    // The footprint's zone will be in board position, so we must translate & rotate the library
    // footprint's zone to match.
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( a->GetParentFootprint() );
    const SHAPE_POLY_SET& aPoly = *a->Outline();
    SHAPE_POLY_SET        bPoly = b->Outline()->CloneDropTriangulation();

    bPoly.Rotate( parentFootprint->GetOrientation() );
    bPoly.Move( parentFootprint->GetPosition() );

    for( int ii = 0; ii < a->Outline()->TotalVertices(); ++ii )
        TEST( aPoly.CVertex( ii ), bPoly.CVertex( ii ) , wxEmptyString);

    return diff;
}


bool modelsNeedUpdate( const FP_3DMODEL& a, const FP_3DMODEL& b, REPORTER* aReporter )
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

    if( IsFlipped() )
    {
        std::unique_ptr<FOOTPRINT> temp( static_cast<FOOTPRINT*>( Clone() ) );
        temp->Flip( {0,0}, false );
        temp->SetParentGroup( nullptr );

        diff = temp->FootprintNeedsUpdate( aLibFootprint );

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
#define ITEM_DESC( item ) ( item )->GetItemDescription( &unitsProvider )
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
                      return item->Type() == PCB_FP_SHAPE_T;
                  } );
    std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> bShapes;
    std::copy_if( aLibFootprint->GraphicalItems().begin(), aLibFootprint->GraphicalItems().end(),
                  std::inserter( bShapes, bShapes.begin() ),
                  []( BOARD_ITEM* item )
                  {
                      return item->Type() == PCB_FP_SHAPE_T;
                  } );

    if( aShapes.size() != aShapes.size() )
    {
        diff = true;
        REPORT( _( "Graphic item count differs." ) );
    }
    else
    {
        for( auto aIt = aShapes.begin(), bIt = bShapes.begin(); aIt != aShapes.end(); aIt++, bIt++ )
        {
            if( ( *aIt )->Type() == PCB_FP_SHAPE_T )
            {
                if( shapeNeedsUpdate( static_cast<FP_SHAPE*>( *aIt ), static_cast<FP_SHAPE*>( *bIt ) ) )
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

    std::set<FP_ZONE*, FOOTPRINT::cmp_zones> aZones( Zones().begin(), Zones().end() );
    std::set<FP_ZONE*, FOOTPRINT::cmp_zones> bZones( aLibFootprint->Zones().begin(), aLibFootprint->Zones().end() );

    if( aZones.size() != bZones.size() )
    {
        diff = true;
        REPORT( _( "Rule area count differs." ) );
    }
    else
    {
        for( auto aIt = aZones.begin(), bIt = bZones.begin(); aIt != aZones.end(); aIt++, bIt++ )
        {
            if( zonesNeedUpdate( *aIt, *bIt ) )
            {
                diff = true;
                REPORT( wxString::Format( _( "%s differs." ), ITEM_DESC( *aIt ) ) );
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
        {
            if( modelsNeedUpdate( Models()[ii], aLibFootprint->Models()[ii], aReporter ) )
                diff = true;
        }
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
