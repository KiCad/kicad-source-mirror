/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <layer_range.h>
#include <layer_utils.h>
#include <kiway.h>
#include <macros.h>
#include <netlist_reader/pcb_netlist.h>
#include <footprint_library_adapter.h>
#include <board.h>
#include <pcb_shape.h>
#include <pcb_barcode.h>
#include <zone.h>
#include <footprint.h>
#include <pad.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <project_pcb.h>
#include <string_utils.h>


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

    virtual ~DRC_TEST_PROVIDER_LIBRARY_PARITY() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "library_parity" ); };
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

#define EPSILON 10
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

#define EPSILON_D 0.000010
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

#define ITEM_DESC( item ) ( item )->GetItemDescription( &g_unitsProvider, true )
#define PAD_DESC( pad ) wxString::Format( _( "Pad %s" ), ( pad )->GetNumber() )


UNITS_PROVIDER g_unitsProvider( pcbIUScale, EDA_UNITS::MM );


LSET getBoardNormalizedLayerSet( const BOARD_ITEM* aLibItem, const BOARD* aBoard )
{
    LSET lset = aLibItem->GetLayerSet();

    if( aBoard )
        lset &= aBoard->GetEnabledLayers();

    return lset;
}


static bool boardLayersMatchWithInnerLayerExpansion( const LSET& aItem, const LSET& bLib,
                                                     bool aAllowCuExpansion )
{
    if( !aAllowCuExpansion )
    {
        return aItem == bLib;
    }

    // first test non copper layers, these should exact match
    const LSET nonCuMask = LSET::AllNonCuMask();
    const LSET aNonCu = aItem & nonCuMask;
    const LSET bNonCu = bLib & nonCuMask;

    if( aNonCu != bNonCu )
        return false;

    // top and bottom copper must match exactly
    if( ( aItem & LSET::ExternalCuMask() ) != ( bLib & LSET::ExternalCuMask() ) )
        return false;

    // technically we can ignore the inner layers entirely due to aAllowCuExpansion at this point
    // since its assumed the layers got expanded elsewhere
    // but it feels weird not to sanity this

    // extract the inner layers to compare prescence
    const LSET aInner = ( aItem & LSET::AllCuMask() ) & ~( LSET::ExternalCuMask() );
    const LSET bInner = ( bLib & LSET::AllCuMask() ) & ~( LSET::ExternalCuMask() );

    const LSET missingInnerInA = bInner & ~aInner;
    if( missingInnerInA.count() )
        return false;

    return true;
}


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
        if( ( a->GetArcMid() - b->GetArcMid() ).EuclideanNorm() > pcbIUScale.mmToIU( 0.0005 ) )
            return true;

        break;

    case SHAPE_T::BEZIER:
        TEST_PT( a->GetStart(), b->GetStart(), "" );
        TEST_PT( a->GetEnd(), b->GetEnd(), "" );
        TEST_PT( a->GetBezierC1(), b->GetBezierC1(), "" );
        TEST_PT( a->GetBezierC2(), b->GetBezierC2(), "" );
        break;

    case SHAPE_T::POLY:
    {
        TEST( a->GetPolyShape().TotalVertices(), b->GetPolyShape().TotalVertices(), "" );

        for( int poly = 0; poly < static_cast<int>( a->GetPolyShape().CPolygons().size() ); poly++ )
        {
            const SHAPE_POLY_SET::POLYGON aPolygon = a->GetPolyShape().CPolygon( poly );
            const SHAPE_POLY_SET::POLYGON  bPolygon = b->GetPolyShape().CPolygon( poly );

            if( aPolygon.size() == 0 || bPolygon.size() == 0
                || !aPolygon[0].CompareGeometry( bPolygon[0], true, EPSILON ) )
            {
                diff = true;
                return diff;
            }
        }

        break;
    }

    default:
        UNIMPLEMENTED_FOR( a->SHAPE_T_asString() );
    }

    TEST( a->GetStroke(), b->GetStroke(), "" );
    TEST( a->GetFillMode(), b->GetFillMode(), "" );

    return diff;
}


bool padHasOverrides( const PAD* a, const PAD* b, REPORTER& aReporter )
{
    bool diff = false;

#define REPORT_MSG( s, p ) aReporter.Report( wxString::Format( s, p ) )

    if( a->GetLocalClearance().has_value() && a->GetLocalClearance() != b->GetLocalClearance() )
    {
        diff = true;
        REPORT_MSG( _( "%s has clearance override." ), PAD_DESC( a ) );
    }

    if( a->GetLocalSolderMaskMargin().has_value()
            && a->GetLocalSolderMaskMargin() != b->GetLocalSolderMaskMargin() )
    {
        diff = true;
        REPORT_MSG( _( "%s has solder mask expansion override." ), PAD_DESC( a ) );
    }


    if( a->GetLocalSolderPasteMargin().has_value()
            && a->GetLocalSolderPasteMargin() != b->GetLocalSolderPasteMargin() )
    {
        diff = true;
        REPORT_MSG( _( "%s has solder paste clearance override." ), PAD_DESC( a ) );
    }

    if( a->GetLocalSolderPasteMarginRatio()
            && a->GetLocalSolderPasteMarginRatio() != b->GetLocalSolderPasteMarginRatio() )
    {
        diff = true;
        REPORT_MSG( _( "%s has solder paste clearance override." ), PAD_DESC( a ) );
    }

    if( a->GetLocalZoneConnection() != ZONE_CONNECTION::INHERITED
            && a->GetLocalZoneConnection() != b->GetLocalZoneConnection() )
    {
        diff = true;
        REPORT_MSG( _( "%s has zone connection override." ), PAD_DESC( a ) );
    }

    if( a->GetLocalThermalGapOverride().has_value()
            && a->GetThermalGap() != b->GetThermalGap() )
    {
        diff = true;
        REPORT_MSG( _( "%s has thermal relief gap override." ), PAD_DESC( a ) );
    }

    if( a->GetLocalThermalSpokeWidthOverride().has_value()
            && a->GetLocalThermalSpokeWidthOverride() != b->GetLocalThermalSpokeWidthOverride() )
    {
        diff = true;
        REPORT_MSG( _( "%s has thermal relief spoke width override." ), PAD_DESC( a ) );
    }

    if( a->GetThermalSpokeAngle() != b->GetThermalSpokeAngle() )
    {
        diff = true;
        REPORT_MSG( _( "%s has thermal relief spoke angle override." ), PAD_DESC( a ) );
    }

    if( a->GetCustomShapeInZoneOpt() != b->GetCustomShapeInZoneOpt() )
    {
        diff = true;
        REPORT_MSG( _( "%s has zone knockout setting override." ), PAD_DESC( a ) );
    }

    return diff;
}


bool padNeedsUpdate( const PAD* a, const PAD* bLib, REPORTER* aReporter )
{
    bool      diff = false;

    TEST( a->GetPadToDieLength(), bLib->GetPadToDieLength(),
          wxString::Format( _( "%s pad to die length differs." ), PAD_DESC( a ) ) );
    TEST_PT( a->GetFPRelativePosition(), bLib->GetFPRelativePosition(),
             wxString::Format( _( "%s position differs." ), PAD_DESC( a ) ) );

    TEST( a->GetNumber(), bLib->GetNumber(),
          wxString::Format( _( "%s has different numbers." ), PAD_DESC( a ) ) );

    // These are assigned from the schematic and not from the library
    // TEST( a->GetPinFunction(), b->GetPinFunction() );
    // TEST( a->GetPinType(), b->GetPinType() );

    bool layerSettingsDiffer = a->GetRemoveUnconnected() != bLib->GetRemoveUnconnected();

    // NB: KeepTopBottom is undefined if RemoveUnconnected is NOT set.
    if( a->GetRemoveUnconnected() )
        layerSettingsDiffer |= a->GetKeepTopBottom() != bLib->GetKeepTopBottom();

    bool allowExpansion = false;
    if( const FOOTPRINT* fp = a->GetParentFootprint() )
        allowExpansion = ( fp->GetStackupMode() == FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS );

    if( layerSettingsDiffer
           || !boardLayersMatchWithInnerLayerExpansion( getBoardNormalizedLayerSet( a, a->GetBoard() ),
                                                       getBoardNormalizedLayerSet( bLib, a->GetBoard() ),
                                                       allowExpansion ) )
    {
        diff = true;

        if( aReporter )
            aReporter->Report( wxString::Format( _( "%s layers differ." ), PAD_DESC( a ) ) );
        else
            return true;
    }

    TEST( a->GetAttribute(), bLib->GetAttribute(),
          wxString::Format( _( "%s pad type differs." ), PAD_DESC( a ) ) );
    TEST( a->GetProperty(), bLib->GetProperty(),
          wxString::Format( _( "%s fabrication property differs." ), PAD_DESC( a ) ) );

    // The pad orientation, for historical reasons is the pad rotation + parent rotation.
    TEST_D( a->GetFPRelativeOrientation().Normalize().AsDegrees(),
            bLib->GetFPRelativeOrientation().Normalize().AsDegrees(),
            wxString::Format( _( "%s orientation differs." ), PAD_DESC( a ) ) );

    std::vector<PCB_LAYER_ID> layers = a->Padstack().UniqueLayers();
    const BOARD* board = a->GetBoard();
    wxString layerName;

    for( PCB_LAYER_ID layer : layers )
    {
        layerName = board ? board->GetLayerName( layer ) : LayerName( layer );

        TEST( a->GetShape( layer ), bLib->GetShape( layer ),
              wxString::Format( _( "%s pad shape type differs on layer %s." ),
                                PAD_DESC( a ),
                                layerName ) );

        TEST( a->GetSize( layer ), bLib->GetSize( layer ),
              wxString::Format( _( "%s size differs on layer %s." ),
                                PAD_DESC( a ),
                                layerName ) );

        TEST( a->GetDelta( layer ), bLib->GetDelta( layer ),
              wxString::Format( _( "%s trapezoid delta differs on layer %s." ),
                                PAD_DESC( a ),
                                layerName ) );

        if( a->GetShape( layer ) == PAD_SHAPE::ROUNDRECT || a->GetShape( layer ) == PAD_SHAPE::CHAMFERED_RECT)
        {
            TEST_D( a->GetRoundRectRadiusRatio( layer ),
                    bLib->GetRoundRectRadiusRatio( layer ),
                    wxString::Format( _( "%s rounded corners differ on layer %s." ),
                                      PAD_DESC( a ),
                                      layerName ) );
        }

        if( a->GetShape( layer ) == PAD_SHAPE::CHAMFERED_RECT)
        {
            TEST_D( a->GetChamferRectRatio( layer ),
                    bLib->GetChamferRectRatio( layer ),
                    wxString::Format( _( "%s chamfered corner sizes differ on layer %s." ),
                                      PAD_DESC( a ),
                                      layerName ) );

            TEST( a->GetChamferPositions( layer ),
                  bLib->GetChamferPositions( layer ),
                  wxString::Format( _( "%s chamfered corners differ on layer %s." ),
                                    PAD_DESC( a ),
                                    layerName ) );
        }

        TEST_PT( a->GetOffset( layer ), bLib->GetOffset( layer ),
                 wxString::Format( _( "%s shape offset from hole differs on layer %s." ),
                                   PAD_DESC( a ),
                                   layerName ) );
    }

    TEST( a->GetDrillShape(), bLib->GetDrillShape(),
          wxString::Format( _( "%s drill shape differs." ), PAD_DESC( a ) ) );
    TEST( a->GetDrillSize(), bLib->GetDrillSize(),
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
    if( aReporter && padHasOverrides( a, bLib, *aReporter ) )
        diff = true;

    bool primitivesDiffer = false;
    PCB_LAYER_ID firstDifferingLayer = UNDEFINED_LAYER;

    a->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                if( a->GetPrimitives( aLayer ).size() != bLib->GetPrimitives( aLayer ).size() )
                {
                    primitivesDiffer = true;
                }
                else
                {
                    for( size_t ii = 0; ii < a->GetPrimitives( aLayer ).size(); ++ii )
                    {
                        if( primitiveNeedsUpdate( a->GetPrimitives( aLayer )[ii],
                                                  bLib->GetPrimitives( aLayer )[ii] ) )
                        {
                            primitivesDiffer = true;
                            break;
                        }
                    }
                }

                if( primitivesDiffer && firstDifferingLayer == UNDEFINED_LAYER )
                    firstDifferingLayer = aLayer;
            } );


    if( primitivesDiffer )
    {
        diff = true;
        layerName = board ? board->GetLayerName( firstDifferingLayer )
                          : LayerName( firstDifferingLayer );

        if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "%s shape primitives differ on layer %s." ),
                                                 PAD_DESC( a ),
                                                 layerName ) );
        }
        else
        {
            return true;
        }
    }

    return diff;
}


bool barcodeNeedsUpdate( const PCB_BARCODE& curr_barcode, const PCB_BARCODE& ref_barcode )
{
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( curr_barcode.GetText(), ref_barcode.GetText(),
          wxString::Format( _( "%s text differs." ), ITEM_DESC( &curr_barcode ) ) );

    TEST_PT( curr_barcode.GetPosition(), ref_barcode.GetPosition(),
             wxString::Format( _( "%s position differs." ), ITEM_DESC( &curr_barcode ) ) );

    TEST( curr_barcode.GetWidth(), ref_barcode.GetWidth(),
          wxString::Format( _( "%s width differs." ), ITEM_DESC( &curr_barcode ) ) );
    TEST( curr_barcode.GetHeight(), ref_barcode.GetHeight(),
          wxString::Format( _( "%s height differs." ), ITEM_DESC( &curr_barcode ) ) );

    TEST( curr_barcode.GetTextSize(), ref_barcode.GetTextSize(),
          wxString::Format( _( "%s text size differs." ), ITEM_DESC( &curr_barcode ) ) );

    TEST( (int) curr_barcode.GetKind(), (int) ref_barcode.GetKind(),
          wxString::Format( _( "%s code differs." ), ITEM_DESC( &curr_barcode ) ) );
    TEST( (int) curr_barcode.GetErrorCorrection(), (int) ref_barcode.GetErrorCorrection(),
          wxString::Format( _( "%s error correction level differs." ), ITEM_DESC( &curr_barcode ) ) );

    return diff;
}


bool shapeNeedsUpdate( const PCB_SHAPE& curr_shape, const PCB_SHAPE& ref_shape )
{
    // curr_shape and ref_shape are expected to be normalized, for a more reliable test.
    REPORTER* aReporter = nullptr;
    bool      diff = false;

    TEST( curr_shape.GetShape(), ref_shape.GetShape(), "" );

    switch( curr_shape.GetShape() )
    {
    case SHAPE_T::RECTANGLE:
    {
        BOX2I aRect( curr_shape.GetStart(), curr_shape.GetEnd() - curr_shape.GetStart() );
        BOX2I bRect( ref_shape.GetStart(), ref_shape.GetEnd() - ref_shape.GetStart() );

        aRect.Normalize();
        bRect.Normalize();

        TEST_PT( aRect.GetOrigin(), bRect.GetOrigin(), "" );
        TEST_PT( aRect.GetEnd(), bRect.GetEnd(), "" );
        break;
    }

    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
        TEST_PT( curr_shape.GetStart(), ref_shape.GetStart(), "" );
        TEST_PT( curr_shape.GetEnd(), ref_shape.GetEnd(), "" );
        break;

    case SHAPE_T::ARC:
        TEST_PT( curr_shape.GetStart(), ref_shape.GetStart(), "" );
        TEST_PT( curr_shape.GetEnd(), ref_shape.GetEnd(), "" );

        // Arc center is calculated and so may have round-off errors when parents are
        // differentially rotated.
        if( ( curr_shape.GetArcMid() - ref_shape.GetArcMid() ).EuclideanNorm() > pcbIUScale.mmToIU( 0.0005 ) )
            return true;

        break;

    case SHAPE_T::BEZIER:
        TEST_PT( curr_shape.GetStart(), ref_shape.GetStart(), "" );
        TEST_PT( curr_shape.GetEnd(), ref_shape.GetEnd(), "" );
        TEST_PT( curr_shape.GetBezierC1(), ref_shape.GetBezierC1(), "" );
        TEST_PT( curr_shape.GetBezierC2(), ref_shape.GetBezierC2(), "" );
        break;

    case SHAPE_T::POLY:
    {
        TEST( curr_shape.GetPolyShape().TotalVertices(), ref_shape.GetPolyShape().TotalVertices(), "" );

        for( int poly = 0; poly < static_cast<int>( curr_shape.GetPolyShape().CPolygons().size() ); poly++ )
        {
            const SHAPE_POLY_SET::POLYGON curr_polygon = curr_shape.GetPolyShape().CPolygon( poly );
            const SHAPE_POLY_SET::POLYGON ref_polygon  = ref_shape.GetPolyShape().CPolygon( poly );
            if( curr_polygon.size() == 0 || ref_polygon.size() == 0
                || !curr_polygon[0].CompareGeometry( ref_polygon[0], true, EPSILON ) )
            {
                diff = true;
                return diff;
            }
        }
        break;
    }

    default:
        UNIMPLEMENTED_FOR( curr_shape.SHAPE_T_asString() );
    }

    if( curr_shape.IsOnCopperLayer() )
        TEST( curr_shape.GetStroke(), ref_shape.GetStroke(), "" );

    TEST( curr_shape.GetFillMode(), ref_shape.GetFillMode(), "" );

    TEST( curr_shape.GetLayer(), ref_shape.GetLayer(), "" );

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
    TEST( a->GetDoNotAllowZoneFills(), b->GetDoNotAllowZoneFills(),
          wxString::Format( _( "%s keep out zone fill setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowFootprints(), b->GetDoNotAllowFootprints(),
          wxString::Format( _( "%s keep out footprints setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowPads(), b->GetDoNotAllowPads(),
          wxString::Format( _( "%s keep out pads setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowTracks(), b->GetDoNotAllowTracks(),
          wxString::Format( _( "%s keep out tracks setting differs." ), ITEM_DESC( a ) ) );
    TEST( a->GetDoNotAllowVias(), b->GetDoNotAllowVias(),
          wxString::Format( _( "%s keep out vias setting differs." ), ITEM_DESC( a ) ) );

    // In1_Cu is used to indicate whether inner layer expansion is allowed on rulesets
    // Kind of annoying footprint pads use both in1_cu and have an stackup mode setting
    bool innerLayerExpansionAllowed = b->GetLayerSet().Contains( In1_Cu );

    if( !boardLayersMatchWithInnerLayerExpansion( a->GetLayerSet(), 
                                                 getBoardNormalizedLayerSet( b, a->GetBoard() ),
                                                 innerLayerExpansionAllowed ) )
    {
        diff = true;

        if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "%s layers differ." ), ITEM_DESC( a ) ) );
        }
        else
        {
            return true;
        }
    }

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

    for( int poly = 0; poly < static_cast<int>( a->Outline()->CPolygons().size() ); poly++ )
    {
        const SHAPE_POLY_SET::POLYGON aPolygon = a->Outline()->CPolygon( poly );
        const SHAPE_POLY_SET::POLYGON bPolygon = b->Outline()->CPolygon( poly );

        if( aPolygon.size() == 0 || bPolygon.size() == 0
            || !aPolygon[0].CompareGeometry( bPolygon[0], true, EPSILON ) )
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


/**
 * Compare the stackup related settings of two footprints.
 *
 * Returns true if they differ.
 */
bool stackupNeedsUpdate( const FOOTPRINT& a, const FOOTPRINT& b, REPORTER* aReporter )
{
    bool diff = false;

    TEST( a.GetStackupMode(), b.GetStackupMode(),
          wxString::Format( _( "Footprint stackup mode differs." ) ) );

    const LSET& aLayers = a.GetLayerSet();
    const LSET& bLayers = b.GetLayerSet();

    TEST( aLayers, bLayers,
          wxString::Format( _( "Footprint layers differ." ) ) );

    return diff;
}


/**
 * Report board->footprint stackup differences.
 *
 * This is not necessarily a comparison failure, but may be useful information
 * for the user to see.
 *
 * @return true if there is
 */
bool footprintVsBoardStackup( const FOOTPRINT& aFp, const BOARD& aBoard, REPORTER* aReporter )
{
    if( aFp.GetStackupMode() == FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS )
        return false;

    // Filter only layers that can differ between footprint and board
    const LSET& fpLayers = aFp.GetStackupLayers();
    const LSET& brdLayers = aBoard.GetEnabledLayers() & ( LSET::AllCuMask() | LSET::UserDefinedLayersMask() );

    bool mismatch = false;

    // Any layer in the FP and not on the board is flagged
    const LSET onlyInFp = fpLayers & ~brdLayers;

    if( onlyInFp.count() )
    {
        mismatch = true;
        if( aReporter )
            aReporter->Report( wxString::Format( _( "Footprint has %lu layers not on board: %s" ), onlyInFp.count(),
                                                 LAYER_UTILS::AccumulateNames( onlyInFp, &aBoard ) ) );
    }

    // Only look at copper layers here: user layers on the board and not in the FP is normal
    const LSET cuOnlyInBoard = ( brdLayers & ~fpLayers ) & LSET::AllCuMask();

    if( cuOnlyInBoard.count() )
    {
        mismatch = true;
        if( aReporter )
            aReporter->Report( wxString::Format( _( "Board has %lu copper layers not in footprint: %s" ),
                                                 cuOnlyInBoard.count(),
                                                 LAYER_UTILS::AccumulateNames( cuOnlyInBoard, &aBoard ) ) );
    }

    return mismatch;
}


bool FOOTPRINT::FootprintNeedsUpdate( const FOOTPRINT* aLibFP, int aCompareFlags,
                                      REPORTER* aReporter )
{
    wxASSERT( aLibFP );
    bool diff = false;

    // To avoid issues when comparing the footprint on board and the footprint in library
    // use the footprint from lib flipped, rotated and at same position as this.
    // And using the footprint from lib with same changes as this minimize the issues
    // due to rounding and shape modifications

    std::unique_ptr<FOOTPRINT> temp( static_cast<FOOTPRINT*>( aLibFP->Clone() ) );

    temp->SetParent( GetBoard() );  // Needed to know the copper layer count;

    if( !( aCompareFlags & COMPARE_FLAGS::INSTANCE_TO_INSTANCE ) )
    {
        if( IsFlipped() != temp->IsFlipped() )
            temp->Flip( { 0, 0 }, FLIP_DIRECTION::TOP_BOTTOM );

        // Set position first before rotating to minimize rounding errors.
        if( GetPosition() != temp->GetPosition() )
            temp->SetPosition( GetPosition() );

        if( GetOrientation() != temp->GetOrientation() )
            temp->SetOrientation( GetOrientation() );
    }

    for( BOARD_ITEM* item : temp->GraphicalItems() )
        item->NormalizeForCompare();

    // This temporary footprint must not have a parent when it goes out of scope because it
    // must not trigger the IncrementTimestamp call in ~FOOTPRINT.
    temp->SetParent( nullptr );

    aLibFP = temp.get();

    // These checks don't set off errors, they're just informational
    footprintVsBoardStackup( *this, *GetBoard(), aReporter );

#define TEST_ATTR( a, b, attr, msg ) TEST( ( a & attr ), ( b & attr ), msg )

    TEST_ATTR( GetAttributes(), aLibFP->GetAttributes(), (FP_THROUGH_HOLE | FP_SMD),
               _( "Footprint types differ." ) );

    TEST( AllowSolderMaskBridges(), aLibFP->AllowSolderMaskBridges(),
          wxString::Format( _( "'%s' settings differ." ),
                            _( "Allow bridged solder mask apertures between pads" ) ) );

    if( !( aCompareFlags & COMPARE_FLAGS::DRC ) )
    {
        // These tests are skipped for DRC: they are presumed to relate to a given design.
        TEST_ATTR( GetAttributes(), aLibFP->GetAttributes(), FP_BOARD_ONLY,
                   wxString::Format( _( "'%s' settings differ." ),
                                     _( "Not in schematic" ) ) );

        TEST_ATTR( GetAttributes(), aLibFP->GetAttributes(), FP_EXCLUDE_FROM_POS_FILES,
                   wxString::Format( _( "'%s' settings differ." ),
                                     _( "Exclude from position files" ) ) );

        TEST_ATTR( GetAttributes(), aLibFP->GetAttributes(), FP_EXCLUDE_FROM_BOM,
                   wxString::Format( _( "'%s' settings differ." ),
                                     _( "Exclude from bill of materials" ) ) );

        TEST_ATTR( GetAttributes(), aLibFP->GetAttributes(), FP_DNP,
                   wxString::Format( _( "'%s' settings differ." ),
                                     _( "Do not populate" ) ) );
    }

#define REPORT( msg ) { if( aReporter ) aReporter->Report( msg ); }
#define CHECKPOINT { if( diff && !aReporter ) return diff; }

    if( stackupNeedsUpdate( *this, *aLibFP, aReporter ) )
    {
        diff = true;
        REPORT( _( "Footprint stackup differs." ) );
    }

    // Clearance and zone connection overrides are as likely to be set at the board level as in
    // the library.
    //
    // If we ignore them and someone *does* change one of them in the library, then stale
    // footprints won't be caught.
    //
    // On the other hand, if we report them then boards that override at the board level are
    // going to be VERY noisy.
    //
    // For report them as different, but we DON'T generate DRC errors on them.
    if( !( aCompareFlags & COMPARE_FLAGS::DRC ) )
    {
        if( GetLocalClearance().has_value() && GetLocalClearance() != aLibFP->GetLocalClearance() )
        {
            diff = true;
            REPORT( _( "Pad clearance overridden." ) );
        }

        if( GetLocalSolderMaskMargin().has_value()
                && GetLocalSolderMaskMargin() != aLibFP->GetLocalSolderMaskMargin() )
        {
            diff = true;
            REPORT( _( "Solder mask expansion overridden." ) );
        }


        if( GetLocalSolderPasteMargin().has_value()
                && GetLocalSolderPasteMargin() != aLibFP->GetLocalSolderPasteMargin() )
        {
            diff = true;
            REPORT( _( "Solder paste absolute clearance overridden." ) );
        }

        if( GetLocalSolderPasteMarginRatio()
                && GetLocalSolderPasteMarginRatio() != aLibFP->GetLocalSolderPasteMarginRatio() )
        {
            diff = true;
            REPORT( _( "Solder paste relative clearance overridden." ) );
        }

        if( GetLocalZoneConnection() != ZONE_CONNECTION::INHERITED
                && GetLocalZoneConnection() != aLibFP->GetLocalZoneConnection() )
        {
            diff = true;
            REPORT( _( "Zone connection overridden." ) );
        }
    }

    TEST( GetNetTiePadGroups().size(), aLibFP->GetNetTiePadGroups().size(),
          _( "Net tie pad groups differ." ) );

    for( size_t ii = 0; ii < GetNetTiePadGroups().size(); ++ii )
    {
        TEST( GetNetTiePadGroups()[ii], aLibFP->GetNetTiePadGroups()[ii],
              _( "Net tie pad groups differ." ) );
    }

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

    // However FOOTPRINT::cmp_drawings uses PCB_SHAPE coordinates and other infos, so we have
    // already normalized graphic items in model footprint from library, so we need to normalize
    // graphic items in the footprint to test (*this). So normalize them using a copy of this
    FOOTPRINT dummy( *this );
    dummy.SetParentGroup( nullptr );
    dummy.SetParent( nullptr );

    for( BOARD_ITEM* item : dummy.GraphicalItems() )
        item->NormalizeForCompare();

    std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> aShapes;
    std::copy_if( dummy.GraphicalItems().begin(), dummy.GraphicalItems().end(),
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
            // aShapes and bShapes are the tested footprint PCB_SHAPE and the model PCB_SHAPE.
            // These shapes are already normalized.
            PCB_SHAPE* curr_shape = static_cast<PCB_SHAPE*>( *aIt );
            PCB_SHAPE* test_shape = static_cast<PCB_SHAPE*>( *bIt );

            if( shapeNeedsUpdate( *curr_shape, *test_shape ) )
            {
                diff = true;
                REPORT( wxString::Format( _( "%s differs." ), ITEM_DESC( *aIt ) ) );
            }
        }
    }

    CHECKPOINT;

    std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> aBarcodes;
    std::copy_if( dummy.GraphicalItems().begin(), dummy.GraphicalItems().end(),
                  std::inserter( aBarcodes, aBarcodes.begin() ),
                  []( BOARD_ITEM* item )
                  {
                      return item->Type() == PCB_BARCODE_T;
                  } );

    std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> bBarcodes;
    std::copy_if( aLibFP->GraphicalItems().begin(), aLibFP->GraphicalItems().end(),
                  std::inserter( bBarcodes, bBarcodes.begin() ),
                  []( BOARD_ITEM* item )
                  {
                      return item->Type() == PCB_BARCODE_T;
                  } );

    if( aBarcodes.size() != bBarcodes.size() )
    {
        diff = true;
        REPORT( _( "Barcode count differs." ) );
    }
    else
    {
        for( auto aIt = aBarcodes.begin(), bIt = bBarcodes.begin(); aIt != aBarcodes.end(); aIt++, bIt++ )
        {
            // aBarcodes and bBarcodes are the tested footprint PCB_BARCODE and the model PCB_BARCODE.
            // These shapes are already normalized.
            PCB_BARCODE* curr_barcode = static_cast<PCB_BARCODE*>( *aIt );
            PCB_BARCODE* test_barcode = static_cast<PCB_BARCODE*>( *bIt );

            if( barcodeNeedsUpdate( *curr_barcode, *test_barcode ) )
            {
                diff = true;
                REPORT( wxString::Format( _( "%s differs." ), ITEM_DESC( *aIt ) ) );
            }
        }
    }

    CHECKPOINT;

    std::set<PAD*, FOOTPRINT::cmp_pads> aPads( Pads().begin(), Pads().end() );
    std::set<PAD*, FOOTPRINT::cmp_pads> bLibPads( aLibFP->Pads().begin(), aLibFP->Pads().end() );

    if( aPads.size() != bLibPads.size() )
    {
        diff = true;
        REPORT( _( "Pad count differs." ) );
    }
    else
    {
        for( auto aIt = aPads.begin(), bLibIt = bLibPads.begin(); aIt != aPads.end(); aIt++, bLibIt++ )
        {
            if( padNeedsUpdate( *aIt, *bLibIt, aReporter ) )
                diff = true;
            else if( aReporter && padHasOverrides( *aIt, *bLibIt, *aReporter ) )
                diff = true;
        }
    }

    CHECKPOINT;

    std::set<ZONE*, FOOTPRINT::cmp_zones> aZones( Zones().begin(), Zones().end() );
    std::set<ZONE*, FOOTPRINT::cmp_zones> bZones( aLibFP->Zones().begin(), aLibFP->Zones().end() );

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
        REPORT_AUX( _( "No project loaded, skipping library parity tests." ) );
        return true;    // Continue with other tests
    }

    if( !reportPhase( _( "Loading footprint library table..." ) ) )
        return false;   // DRC cancelled

    std::map<LIB_ID, std::shared_ptr<FOOTPRINT>> libFootprintCache;

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( project );
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
        LIBRARY_TABLE_ROW*   libTableRow = nullptr;

        if( libName.IsEmpty() )
        {
            // Not much we can do here
            continue;
        }

        if( std::optional<LIBRARY_TABLE_ROW*> optRow = adapter->GetRow( libName ); optRow )
            libTableRow = *optRow;

        if( !libTableRow )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_ISSUES ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
                msg.Printf( _( "The current configuration does not include the footprint library '%s'" ),
                            UnescapeString( libName ) );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetCenter(), UNDEFINED_LAYER );
            }

            continue;
        }
        else if( !adapter->HasLibrary( libName, true ) )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_ISSUES ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
                msg.Printf( _( "The footprint library '%s' is not enabled in the current configuration" ),
                            UnescapeString( libName ) );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetCenter(), UNDEFINED_LAYER );
            }

            continue;
        }
        else if( !adapter->IsLibraryLoaded( libName ) )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_ISSUES ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_ISSUES );
                msg.Printf( _( "The footprint library '%s' was not found at '%s'" ),
                            UnescapeString( libName ),
                            LIBRARY_MANAGER::GetFullURI( libTableRow, true ) );
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
                libFootprint.reset( adapter->LoadFootprint( libName, fpName, true ) );

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
                msg.Printf( _( "Footprint '%s' not found in library '%s'" ),
                            fpName,
                            libName );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetCenter(), UNDEFINED_LAYER );
            }
        }
        else if( footprint->FootprintNeedsUpdate( libFootprint.get(), BOARD_ITEM::COMPARE_FLAGS::DRC ) )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_LIB_FOOTPRINT_MISMATCH ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LIB_FOOTPRINT_MISMATCH );
                msg.Printf( _( "Footprint '%s' does not match copy in library '%s'" ),
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
