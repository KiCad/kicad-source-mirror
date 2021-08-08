/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers.
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
        return "library_parity";
    };

    virtual const wxString GetDescription() const override
    {
        return "Performs board footprint vs library integity checks";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;
};


#define TEST( a, b ) { if( a != b ) return true; }
#define TEST_PADS( a, b ) { if( padsNeedUpdate( a, b ) ) return true; }
#define TEST_SHAPES( a, b ) { if( shapesNeedUpdate( a, b ) ) return true; }
#define TEST_PRIMITIVES( a, b ) { if( primitivesNeedUpdate( a, b ) ) return true; }
#define TEST_ZONES( a, b ) { if( zonesNeedUpdate( a, b ) ) return true; }
#define TEST_MODELS( a, b ) { if( modelsNeedUpdate( a, b ) ) return true; }


bool primitivesNeedUpdate( const std::shared_ptr<PCB_SHAPE>& a,
                           const std::shared_ptr<PCB_SHAPE>& b )
{
    TEST( a->GetShape(), b->GetShape() );

    switch( a->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
        TEST( a->GetStart(), b->GetStart() );
        TEST( a->GetEnd(), b->GetEnd() );
        break;

    case SHAPE_T::ARC:
        TEST( a->GetStart(), b->GetStart() );
        TEST( a->GetEnd(), b->GetEnd() );
        TEST( a->GetCenter(), b->GetCenter() );
        TEST( a->GetArcAngle(), b->GetArcAngle() );
        break;

    case SHAPE_T::BEZIER:
        TEST( a->GetStart(), b->GetStart() );
        TEST( a->GetEnd(), b->GetEnd() );
        TEST( a->GetBezierC1(), b->GetBezierC1() );
        TEST( a->GetBezierC2(), b->GetBezierC2() );
        break;

    case SHAPE_T::POLY:
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices() );

        for( int ii = 0; ii < a->GetPolyShape().TotalVertices(); ++ii )
            TEST( a->GetPolyShape().CVertex( ii ), b->GetPolyShape().CVertex( ii ) );

        break;

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    TEST( a->GetWidth(), b->GetWidth() );
    TEST( a->IsFilled(), b->IsFilled() );

    return false;
}


bool padsNeedUpdate( const PAD* a, const PAD* b )
{
    TEST( a->GetPadToDieLength(), b->GetPadToDieLength() );
    TEST( a->GetPos0(), b->GetPos0() );

    TEST( a->GetNumber(), b->GetNumber() );

    // These are assigned from the schematic and not from the library
    // TEST( a->GetPinFunction(), b->GetPinFunction() );
    // TEST( a->GetPinType(), b->GetPinType() );

    TEST( a->GetRemoveUnconnected(), b->GetRemoveUnconnected() );

    // NB: KeepTopBottom is undefined if RemoveUnconnected is NOT set.
    if( a->GetRemoveUnconnected() )
        TEST( a->GetKeepTopBottom(), b->GetKeepTopBottom() );

    TEST( a->GetShape(), b->GetShape() );
    TEST( a->GetLayerSet(), b->GetLayerSet() );
    TEST( a->GetAttribute(), b->GetAttribute() );
    TEST( a->GetProperty(), b->GetProperty() );

    // The pad orientation, for historical reasons is the pad rotation + parent rotation.
    TEST( NormalizeAnglePos( a->GetOrientation() - a->GetParent()->GetOrientation() ),
          NormalizeAnglePos( b->GetOrientation() - b->GetParent()->GetOrientation() ) );

    TEST( a->GetSize(), b->GetSize() );
    TEST( a->GetDelta(), b->GetDelta() );
    TEST( a->GetRoundRectCornerRadius(), b->GetRoundRectCornerRadius() );
    TEST( a->GetRoundRectRadiusRatio(), b->GetRoundRectRadiusRatio() );
    TEST( a->GetChamferRectRatio(), b->GetChamferRectRatio() );
    TEST( a->GetChamferPositions(), b->GetChamferPositions() );
    TEST( a->GetOffset(), b->GetOffset() );

    TEST( a->GetDrillShape(), b->GetDrillShape() );
    TEST( a->GetDrillSize(), b->GetDrillSize() );

    TEST( a->GetLocalClearance(), b->GetLocalClearance() );
    TEST( a->GetLocalSolderMaskMargin(), b->GetLocalSolderMaskMargin() );
    TEST( a->GetLocalSolderPasteMargin(), b->GetLocalSolderPasteMargin() );
    TEST( a->GetLocalSolderPasteMarginRatio(), b->GetLocalSolderPasteMarginRatio() );

    TEST( a->GetZoneConnection(), b->GetZoneConnection() );
    TEST( a->GetThermalGap(), b->GetThermalGap() );
    TEST( a->GetThermalSpokeWidth(), b->GetThermalSpokeWidth() );
    TEST( a->GetThermalSpokeAngle(), b->GetThermalSpokeAngle() );
    TEST( a->GetCustomShapeInZoneOpt(), b->GetCustomShapeInZoneOpt() );

    TEST( a->GetPrimitives().size(), b->GetPrimitives().size() );

    for( size_t ii = 0; ii < a->GetPrimitives().size(); ++ii )
        TEST_PRIMITIVES( a->GetPrimitives()[ii], b->GetPrimitives()[ii] );

    return false;
}


bool shapesNeedUpdate( const FP_SHAPE* a, const FP_SHAPE* b )
{
    TEST( a->GetShape(), b->GetShape() );

    switch( a->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
        TEST( a->GetStart0(), b->GetStart0() );
        TEST( a->GetEnd0(), b->GetEnd0() );
        break;

    case SHAPE_T::ARC:
        TEST( a->GetStart0(), b->GetStart0() );
        TEST( a->GetEnd0(), b->GetEnd0() );
        TEST( a->GetCenter0(), b->GetCenter0() );
        TEST( a->GetArcAngle(), b->GetArcAngle() );
        break;

    case SHAPE_T::BEZIER:
        TEST( a->GetStart0(), b->GetStart0() );
        TEST( a->GetEnd0(), b->GetEnd0() );
        TEST( a->GetBezierC1_0(), b->GetBezierC1_0() );
        TEST( a->GetBezierC2_0(), b->GetBezierC2_0() );
        break;

    case SHAPE_T::POLY:
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices() );

        for( int ii = 0; ii < a->GetPolyShape().TotalVertices(); ++ii )
            TEST( a->GetPolyShape().CVertex( ii ), b->GetPolyShape().CVertex( ii ) );

        break;

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    TEST( a->GetWidth(), b->GetWidth() );
    TEST( a->IsFilled(), b->IsFilled() );

    TEST( a->GetLayer(), b->GetLayer() );

    return false;
}


bool textsNeedUpdate( const FP_TEXT* a, const FP_TEXT* b )
{
    TEST( a->GetLayer(), b->GetLayer() );
    TEST( a->IsKeepUpright(), b->IsKeepUpright() );

    TEST( a->GetText(), b->GetText() );

    TEST( a->GetTextThickness(), b->GetTextThickness() );
    TEST( a->GetTextAngle(), b->GetTextAngle() );
    TEST( a->IsItalic(), b->IsItalic() );
    TEST( a->IsBold(), b->IsBold() );
    TEST( a->IsVisible(), b->IsVisible() );
    TEST( a->IsMirrored(), b->IsMirrored() );

    TEST( a->GetHorizJustify(), b->GetHorizJustify() );
    TEST( a->GetVertJustify(), b->GetVertJustify() );

    TEST( a->GetTextSize(), b->GetTextSize() );
    TEST( a->GetPos0(), b->GetPos0() );

    return false;
}


bool zonesNeedUpdate( const FP_ZONE* a, const FP_ZONE* b )
{
    TEST( a->GetCornerSmoothingType(), b->GetCornerSmoothingType() );
    TEST( a->GetCornerRadius(), b->GetCornerRadius() );
    TEST( a->GetZoneName(), b->GetZoneName() );
    TEST( a->GetPriority(), b->GetPriority() );

    TEST( a->GetIsRuleArea(), b->GetIsRuleArea() );
    TEST( a->GetDoNotAllowCopperPour(), b->GetDoNotAllowCopperPour() );
    TEST( a->GetDoNotAllowFootprints(), b->GetDoNotAllowFootprints() );
    TEST( a->GetDoNotAllowPads(), b->GetDoNotAllowPads() );
    TEST( a->GetDoNotAllowTracks(), b->GetDoNotAllowTracks() );
    TEST( a->GetDoNotAllowVias(), b->GetDoNotAllowVias() );

    TEST( a->GetLayerSet(), b->GetLayerSet() );

    TEST( a->GetPadConnection(), b->GetPadConnection() );
    TEST( a->GetLocalClearance(), b->GetLocalClearance() );
    TEST( a->GetThermalReliefGap(), b->GetThermalReliefGap() );
    TEST( a->GetThermalReliefSpokeWidth(), b->GetThermalReliefSpokeWidth() );

    TEST( a->GetMinThickness(), b->GetMinThickness() );

    TEST( a->GetFillVersion(), b->GetFillVersion() );
    TEST( a->GetIslandRemovalMode(), b->GetIslandRemovalMode() );
    TEST( a->GetMinIslandArea(), b->GetMinIslandArea() );

    TEST( a->GetFillMode(), b->GetFillMode() );
    TEST( a->GetHatchThickness(), b->GetHatchThickness() );
    TEST( a->GetHatchGap(), b->GetHatchGap() );
    TEST( a->GetHatchOrientation(), b->GetHatchOrientation() );
    TEST( a->GetHatchSmoothingLevel(), b->GetHatchSmoothingLevel() );
    TEST( a->GetHatchSmoothingValue(), b->GetHatchSmoothingValue() );
    TEST( a->GetHatchBorderAlgorithm(), b->GetHatchBorderAlgorithm() );
    TEST( a->GetHatchHoleMinArea(), b->GetHatchHoleMinArea() );

    TEST( a->Outline()->TotalVertices(), b->Outline()->TotalVertices() );

    for( int ii = 0; ii < a->Outline()->TotalVertices(); ++ii )
    {
        TEST( a->Outline()->CVertex( ii ) - a->GetParent()->GetPosition(),
              b->Outline()->CVertex( ii ) - b->GetParent()->GetPosition() );
    }

    return false;
}


bool modelsNeedUpdate( const FP_3DMODEL& a, const FP_3DMODEL& b )
{
#define EPSILON 0.000001
#define TEST_V3D( a, b ) { if( abs( a.x - b.x ) > EPSILON    \
                            || abs( a.y - b.y ) > EPSILON    \
                            || abs( a.z - b.z ) > EPSILON )  \
                               return true;                  }

    TEST_V3D( a.m_Scale, b.m_Scale );
    TEST_V3D( a.m_Rotation, b.m_Rotation );
    TEST_V3D( a.m_Offset, b.m_Offset );
    TEST( a.m_Opacity, b.m_Opacity );
    TEST( a.m_Filename, b.m_Filename );
    TEST( a.m_Show, b.m_Show );

    return false;
}


bool FOOTPRINT::FootprintNeedsUpdate( const FOOTPRINT* aLibFootprint )
{
    TEST( GetDescription(), aLibFootprint->GetDescription() );
    TEST( GetKeywords(), aLibFootprint->GetKeywords() );
    TEST( GetAttributes(), aLibFootprint->GetAttributes() );

    TEST( GetPlacementCost90(), aLibFootprint->GetPlacementCost90() );
    TEST( GetPlacementCost180(), aLibFootprint->GetPlacementCost180() );

    TEST( GetLocalClearance(), aLibFootprint->GetLocalClearance() );
    TEST( GetLocalSolderMaskMargin(), aLibFootprint->GetLocalSolderMaskMargin() );
    TEST( GetLocalSolderPasteMargin(), aLibFootprint->GetLocalSolderPasteMargin() );
    TEST( GetLocalSolderPasteMarginRatio(), aLibFootprint->GetLocalSolderPasteMarginRatio() );

    TEST( GetZoneConnection(), aLibFootprint->GetZoneConnection() );

    // Text items are really problematic.  We don't want to test the reference, but after that
    // it gets messy.  What about the value?  Depends on whether or not it's a singleton part.
    // And what about other texts?  They might be added only to instances on the board, or even
    // changed for instances on the board.  Or they might want to be tested for equality.
    // Currently we punt and ignore all the text items.

    // Drawings and pads are also somewhat problematic as there's no gaurantee that they'll be
    // in the same order in the two footprints.  Rather than builds some sophisticated hashing
    // algorithm we use the footprint sorting functions to attempt to sort them in the same order.

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

    std::set<PAD*, FOOTPRINT::cmp_pads> aPads( Pads().begin(), Pads().end() );
    std::set<PAD*, FOOTPRINT::cmp_pads> bPads( aLibFootprint->Pads().begin(), aLibFootprint->Pads().end() );

    std::set<FP_ZONE*, FOOTPRINT::cmp_zones> aZones( Zones().begin(), Zones().end() );
    std::set<FP_ZONE*, FOOTPRINT::cmp_zones> bZones( aLibFootprint->Zones().begin(), aLibFootprint->Zones().end() );

    TEST( aPads.size(), bPads.size() );
    TEST( aZones.size(), bZones.size() );
    TEST( aShapes.size(), bShapes.size() );

    for( auto aIt = aPads.begin(), bIt = bPads.begin(); aIt != aPads.end(); aIt++, bIt++ )
        TEST_PADS( *aIt, *bIt );

    for( auto aIt = aShapes.begin(), bIt = bShapes.begin(); aIt != aShapes.end(); aIt++, bIt++ )
    {
        if( ( *aIt )->Type() == PCB_FP_SHAPE_T )
            TEST_SHAPES( static_cast<FP_SHAPE*>( *aIt ), static_cast<FP_SHAPE*>( *bIt ) );
    }

    for( auto aIt = aZones.begin(), bIt = bZones.begin(); aIt != aZones.end(); aIt++, bIt++ )
        TEST_ZONES( *aIt, *bIt );

    TEST( Models().size(), aLibFootprint->Models().size() );

    for( size_t ii = 0; ii < Models().size(); ++ii )
        TEST_MODELS( Models()[ii], aLibFootprint->Models()[ii] );

    return false;
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
    const int     delta = 50;  // Number of tests between calls to progress bar

    if( !reportPhase( _( "Checking board footprints against library..." ) ) )
        return false;

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_ISSUES ) )
            return true;    // Continue with other tests

        if( !reportProgress( ii++, board->Footprints().size(), delta ) )
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
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
            msg.Printf( _( "The current configuration does not include the library '%s'." ),
                        libName );
            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetCenter() );

            continue;
        }
        else if( !libTable->HasLibrary( libName, true ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
            msg.Printf( _( "The library '%s' is not enabled in the current configuration." ),
                        libName );
            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetCenter() );

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
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
            msg.Printf( "Footprint '%s' not found in library '%s'.",
                        fpName,
                        libName );
            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetCenter() );
        }
        else if( footprint->FootprintNeedsUpdate( libFootprint.get() ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
            msg.Printf( "Footprint '%s' does not match copy in library '%s'.",
                        fpName,
                        libName );
            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetCenter() );
        }
    }

    return true;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_LIBRARY_PARITY::GetConstraintTypes() const
{
    return {};
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_LIBRARY_PARITY> dummy;
}
