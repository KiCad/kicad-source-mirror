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


#define TEST( a, b ) { if( a != b ) return true; }
#define TEST_PADS( a, b ) { if( padsNeedUpdate( a, b ) ) return true; }
#define TEST_SHAPES( a, b ) { if( shapesNeedUpdate( a, b ) ) return true; }
#define TEST_PRIMITIVES( a, b ) { if( primitivesNeedUpdate( a, b ) ) return true; }
#define TEST_ZONES( a, b ) { if( zonesNeedUpdate( a, b ) ) return true; }
#define TEST_MODELS( a, b ) { if( modelsNeedUpdate( a, b ) ) return true; }

#define EPSILON 0.000001
#define TEST_D( a, b ) { if( abs( a - b ) > EPSILON ) return true; }

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
        TEST_D( a->GetArcAngle().AsDegrees(), b->GetArcAngle().AsDegrees() );
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

    TEST( a->GetStroke(), b->GetStroke() );
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

    // Trim layersets to the current board before comparing
    LSET enabledLayers = a->GetBoard()->GetEnabledLayers();
    LSET aLayers = a->GetLayerSet() & enabledLayers;
    LSET bLayers = b->GetLayerSet() & enabledLayers;
    TEST( aLayers, bLayers );

    TEST( a->GetAttribute(), b->GetAttribute() );
    TEST( a->GetProperty(), b->GetProperty() );

    // The pad orientation, for historical reasons is the pad rotation + parent rotation.
    TEST_D(( a->GetOrientation() - a->GetParent()->GetOrientation() ).Normalize().AsDegrees(),
           ( b->GetOrientation() - b->GetParent()->GetOrientation() ).Normalize().AsDegrees() );

    TEST( a->GetSize(), b->GetSize() );
    TEST( a->GetDelta(), b->GetDelta() );
    TEST( a->GetRoundRectCornerRadius(), b->GetRoundRectCornerRadius() );
    TEST_D( a->GetRoundRectRadiusRatio(), b->GetRoundRectRadiusRatio() );
    TEST_D( a->GetChamferRectRatio(), b->GetChamferRectRatio() );
    TEST( a->GetChamferPositions(), b->GetChamferPositions() );
    TEST( a->GetOffset(), b->GetOffset() );

    TEST( a->GetDrillShape(), b->GetDrillShape() );
    TEST( a->GetDrillSize(), b->GetDrillSize() );

    // Clearance and zone connection overrides are as likely to be set at the board level as in
    // the library.
    //
    // If we ignore them and someone *does* change one of them in the library, then stale
    // footprints won't be caught.
    //
    // On the other hand, if we report them then boards that override at the board level are
    // going to be VERY noisy.
#if 0
    TEST( a->GetLocalClearance(), b->GetLocalClearance() );
    TEST( a->GetLocalSolderMaskMargin(), b->GetLocalSolderMaskMargin() );
    TEST( a->GetLocalSolderPasteMargin(), b->GetLocalSolderPasteMargin() );
    TEST_D( a->GetLocalSolderPasteMarginRatio(), b->GetLocalSolderPasteMarginRatio() );

    TEST( a->GetZoneConnection(), b->GetZoneConnection() );
    TEST( a->GetThermalGap(), b->GetThermalGap() );
    TEST( a->GetThermalSpokeWidth(), b->GetThermalSpokeWidth() );
    TEST_D( a->GetThermalSpokeAngle().AsDegrees(), b->GetThermalSpokeAngle().AsDegrees() );
    TEST( a->GetCustomShapeInZoneOpt(), b->GetCustomShapeInZoneOpt() );
#endif

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

        // Arc center is calculated to the nearest 100nm increment and may have round-off errors
        // when parents are differentially rotated.  See CalcArcCenter() function in
        // ./libs/kimath/src/trigo.cpp.
        if( ( std::abs( a->GetCenter0().x - b->GetCenter0().x ) > pcbIUScale.mmToIU( 0.0002 ) ) ||
            ( std::abs( a->GetCenter0().y - b->GetCenter0().y ) > pcbIUScale.mmToIU( 0.0002 ) ) )
            return true;

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

    TEST( a->GetStroke(), b->GetStroke() );
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
    TEST( a->GetAssignedPriority(), b->GetAssignedPriority() );

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

    TEST( a->GetIslandRemovalMode(), b->GetIslandRemovalMode() );
    TEST( a->GetMinIslandArea(), b->GetMinIslandArea() );

    TEST( a->GetFillMode(), b->GetFillMode() );
    TEST( a->GetHatchThickness(), b->GetHatchThickness() );
    TEST( a->GetHatchGap(), b->GetHatchGap() );
    TEST_D( a->GetHatchOrientation().AsDegrees(), b->GetHatchOrientation().AsDegrees() );
    TEST( a->GetHatchSmoothingLevel(), b->GetHatchSmoothingLevel() );
    TEST( a->GetHatchSmoothingValue(), b->GetHatchSmoothingValue() );
    TEST( a->GetHatchHoleMinArea(), b->GetHatchHoleMinArea() );

    // This is just a display property
    // TEST( a->GetHatchBorderAlgorithm(), b->GetHatchBorderAlgorithm() );

    TEST( a->Outline()->TotalVertices(), b->Outline()->TotalVertices() );

    for( int ii = 0; ii < a->Outline()->TotalVertices(); ++ii )
        TEST( a->Outline()->CVertex( ii ), b->Outline()->CVertex( ii ) );

    return false;
}


bool modelsNeedUpdate( const FP_3DMODEL& a, const FP_3DMODEL& b )
{
#define TEST_V3D( a, b ) { TEST_D( a.x, b.x ); TEST_D( a.y, b.y ); TEST_D( a.z, b.z ); }

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
#define TEST_ATTR( a, b, attr ) TEST( ( a & attr ), ( b & attr ) );
    wxASSERT( aLibFootprint );

    if( IsFlipped() )
    {
        std::unique_ptr<FOOTPRINT> temp( static_cast<FOOTPRINT*>( Clone() ) );
        temp->Flip( {0,0}, false );
        temp->SetParentGroup( nullptr );

        bool needsUpdate = temp->FootprintNeedsUpdate( aLibFootprint );

        // This temporary footprint must not have a parent when it goes out of scope because it must
        // not trigger then IncrementTimestamp call in ~FOOTPRINT.
        temp->SetParent( nullptr );
        return needsUpdate;
    }

    TEST( GetDescription(), aLibFootprint->GetDescription() );
    TEST( GetKeywords(), aLibFootprint->GetKeywords() );

    TEST_ATTR( GetAttributes(), aLibFootprint->GetAttributes(), FP_THROUGH_HOLE );
    TEST_ATTR( GetAttributes(), aLibFootprint->GetAttributes(), FP_SMD );
    TEST_ATTR( GetAttributes(), aLibFootprint->GetAttributes(), FP_ALLOW_SOLDERMASK_BRIDGES );
    TEST_ATTR( GetAttributes(), aLibFootprint->GetAttributes(), FP_ALLOW_MISSING_COURTYARD );

    // Clearance and zone connection overrides are as likely to be set at the board level as in
    // the library.
    //
    // If we ignore them and someone *does* change one of them in the library, then stale
    // footprints won't be caught.
    //
    // On the other hand, if we report them then boards that override at the board level are
    // going to be VERY noisy.
#if 0
    TEST( GetLocalClearance(), aLibFootprint->GetLocalClearance() );
    TEST( GetLocalSolderMaskMargin(), aLibFootprint->GetLocalSolderMaskMargin() );
    TEST( GetLocalSolderPasteMargin(), aLibFootprint->GetLocalSolderPasteMargin() );
    TEST_D( GetLocalSolderPasteMarginRatio(), aLibFootprint->GetLocalSolderPasteMarginRatio() );

    TEST( GetZoneConnection(), aLibFootprint->GetZoneConnection() );
#endif

    TEST( GetNetTiePadGroups().size(), aLibFootprint->GetNetTiePadGroups().size() );

    for( size_t ii = 0; ii < GetNetTiePadGroups().size(); ++ii )
        TEST( GetNetTiePadGroups()[ii], aLibFootprint->GetNetTiePadGroups()[ii] );

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

    TEST( aShapes.size(), bShapes.size() );

    for( auto aIt = aShapes.begin(), bIt = bShapes.begin(); aIt != aShapes.end(); aIt++, bIt++ )
    {
        if( ( *aIt )->Type() == PCB_FP_SHAPE_T )
            TEST_SHAPES( static_cast<FP_SHAPE*>( *aIt ), static_cast<FP_SHAPE*>( *bIt ) );
    }

    std::set<PAD*, FOOTPRINT::cmp_pads> aPads( Pads().begin(), Pads().end() );
    std::set<PAD*, FOOTPRINT::cmp_pads> bPads( aLibFootprint->Pads().begin(), aLibFootprint->Pads().end() );

    TEST( aPads.size(), bPads.size() );

    for( auto aIt = aPads.begin(), bIt = bPads.begin(); aIt != aPads.end(); aIt++, bIt++ )
        TEST_PADS( *aIt, *bIt );

    // Rotate/position a copy of libFootprint so that zones sort the same
    std::unique_ptr<FOOTPRINT> libCopy( static_cast<FOOTPRINT*>( aLibFootprint->Clone() ) );

    libCopy->SetOrientation( GetOrientation() );
    libCopy->Move( GetPosition() );

    std::set<FP_ZONE*, FOOTPRINT::cmp_zones> aZones( Zones().begin(), Zones().end() );
    std::set<FP_ZONE*, FOOTPRINT::cmp_zones> bZones( libCopy->Zones().begin(), libCopy->Zones().end() );

    TEST( aZones.size(), bZones.size() );

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

        if( !reportProgress( ii++, board->Footprints().size(), progressDelta ) )
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
