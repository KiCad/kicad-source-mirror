/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <downgrade/board_downgrade.h>

#include <downgrade_scan.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <constraints/pcb_constraint.h>
#include <pad.h>
#include <pcb_barcode.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <pcb_point.h>
#include <pcb_reference_image.h>
#include <zone.h>
#include <zone_settings.h>
#include <netinfo.h>
#include <eda_group.h>
#include <eda_shape.h>
#include <pcb_group.h>
#include <pcb_dimension.h>
#include <pcb_generator.h>
#include <pcb_text.h>
#include <project.h>
#include <common.h>
#include <text_eval/text_eval_wrapper.h>
#include <board_stackup_manager/board_stackup.h>
#include <layer_ids.h>
#include <lset.h>
#include <geometry/shape_ellipse.h>
#include <geometry/shape_poly_set.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


// Board format date each feature landed. A rule fires only when the target predates this.
static constexpr int VER_USER_LAYERS = 20241229;
static constexpr int VER_TEXTBOX_KNOCKOUT = 20250210;
static constexpr int VER_SHAPE_HATCH = 20250222;
static constexpr int VER_VIA_PROTECTION = 20250228;
static constexpr int VER_ZONE_DEFAULTS = 20250302;
static constexpr int VER_JUMPER_PADS = 20250324;
static constexpr int VER_VIA_SPLIT = 20250926;
static constexpr int VER_SKIP_VIA = 20250801;
static constexpr int VER_PRESSFIT = 20250811;
static constexpr int VER_POST_MACHINING = 20251101;
static constexpr int VER_PCB_POINTS = 20250901;
static constexpr int VER_ROUNDED_RECT = 20250829;
static constexpr int VER_FP_UNITS = 20250909;
static constexpr int VER_BARCODE = 20250914;
static constexpr int VER_PAD_DIE_DELAY = 20250401;
static constexpr int VER_CELL_KNOCKOUT = 20260603;
static constexpr int VER_BACKDRILL = 20251101;
static constexpr int VER_EXTRUDED_BODY = 20260410;
static constexpr int VER_ELLIPSE = 20260508;
static constexpr int VER_THIEVING = 20260513;
static constexpr int VER_NET_CHAINS = 20260512;
static constexpr int VER_PAD_SIM = 20260521;
static constexpr int VER_DIELECTRIC = 20260511;
static constexpr int VER_GROUP_LIB_ID = 20250513;
static constexpr int VER_IMAGE_PPI = 20260623;
static constexpr int VER_CONSTRAINTS = 20260624;
static constexpr int VER_GRIDS = 20260728;
static constexpr int VER_VIA_STITCH = 20260816;
static constexpr int VER_PROPERTY_VARIABLES = 20260817;
static constexpr int VER_LINE_ENDINGS = 20260818;
static constexpr int VER_SIM_EXCLUSION = 20260828;
static constexpr int VER_VIA_STACK = 20260830;
static constexpr int VER_CUSTOM_PROPERTIES = 20260831;
static constexpr int VER_DRILL_DRAWINGS = 20260901;

// The board format this feature was built for. If SEXPR_BOARD_FILE_VERSION moves past this, a new
// feature landed and the rules or denylist may be stale. Review pcb.keywords, then update this.
// Bumps whose data lives in the project file, not the board file, need no rule here. That covers
// 20250309 component class assignment rules and the tuning profile part of 20250401.
static constexpr int BOARD_DOWNGRADE_COVERED = 20260901;


static bool isEllipse( const BOARD_ITEM* aItem )
{
    if( aItem->Type() != PCB_SHAPE_T )
        return false;

    SHAPE_T shape = static_cast<const PCB_SHAPE*>( aItem )->GetShape();
    return shape == SHAPE_T::ELLIPSE || shape == SHAPE_T::ELLIPSE_ARC;
}


static bool isHatchedShape( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_SHAPE_T && static_cast<const PCB_SHAPE*>( aItem )->IsHatchedFill();
}


// A hatched fill on copper is copper, so losing it changes the board electrically. Elsewhere it is
// ink. The two cases are reported and transformed separately.
static bool isHatchedCopperShape( const BOARD_ITEM* aItem )
{
    return isHatchedShape( aItem ) && IsCopperLayer( aItem->GetLayer() );
}


static bool isHatchedInkShape( const BOARD_ITEM* aItem )
{
    return isHatchedShape( aItem ) && !IsCopperLayer( aItem->GetLayer() );
}


// FOOTPRINT::Remove does not detach group membership the way BOARD::Remove does, so items
// deleted from a footprint must leave their group first or the writer walks a dangling pointer.
static void detachFromGroup( BOARD_ITEM* aItem )
{
    if( EDA_GROUP* group = aItem->GetParentGroup() )
        group->RemoveItem( aItem );
}


static void ellipseToPolygon( PCB_SHAPE* aShape, int aMaxError )
{
    // A board polygon is always closed, so an elliptical arc becomes a filled polygon of
    // its stroked ink instead of an open outline.
    if( aShape->GetShape() == SHAPE_T::ELLIPSE_ARC )
    {
        aShape->SetWidth( std::max( aShape->GetWidth(), 2 * aMaxError ) );

        SHAPE_POLY_SET poly;
        aShape->EDA_SHAPE::TransformShapeToPolygon( poly, 0, aMaxError, ERROR_INSIDE, false );
        poly.Simplify();

        // The writers save one outline per shape, so holes must become cuts.
        poly.Fracture();

        aShape->SetShape( SHAPE_T::POLY );
        aShape->SetPolyShape( poly );
        aShape->SetFillMode( FILL_T::FILLED_SHAPE );
        aShape->SetWidth( 0 );
        return;
    }

    SHAPE_ELLIPSE ellipse( aShape->GetEllipseCenter(), aShape->GetEllipseMajorRadius(), aShape->GetEllipseMinorRadius(),
                           aShape->GetEllipseRotation() );

    SHAPE_LINE_CHAIN chain = ellipse.ConvertToPolyline( aMaxError );
    chain.SetClosed( true );

    SHAPE_POLY_SET poly;
    poly.AddOutline( chain );

    aShape->SetShape( SHAPE_T::POLY );
    aShape->SetPolyShape( poly );
}


// Per-footprint rule scopes. The board apply functions delegate to these, and
// DowngradeFootprintInPlace runs them from the same rule table, so the standalone
// .kicad_mod path cannot drift from the board path.
static int footprintMaxError( const FOOTPRINT* aFootprint )
{
    return aFootprint->GetBoard() ? aFootprint->GetBoard()->GetDesignSettings().m_MaxError : pcbIUScale.mmToIU( 0.005 );
}


static void lowerEllipsesFp( FOOTPRINT* aFootprint )
{
    int maxError = footprintMaxError( aFootprint );

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( isEllipse( item ) )
            ellipseToPolygon( static_cast<PCB_SHAPE*>( item ), maxError );
    }
}


static void setHatchedFillsFp( FOOTPRINT* aFootprint, const std::function<bool( const BOARD_ITEM* )>& aMatches,
                               FILL_T aFill )
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( aMatches( item ) )
            static_cast<PCB_SHAPE*>( item )->SetFillMode( aFill );
    }
}


// The sexpr writer only saves outline 0 of a polygon shape, so a barcode's ink must be
// split into one shape per outline or all but one module would be lost on save.
static std::vector<PCB_SHAPE*> barcodeToShapes( PCB_BARCODE* aBarcode, BOARD_ITEM_CONTAINER* aParent )
{
    SHAPE_POLY_SET poly;
    aBarcode->TransformShapeToPolygon( poly, aBarcode->GetLayer(), 0, 0, ERROR_INSIDE );
    poly.Simplify();

    // Each shape below carries a single outline, so holes must become cuts first or a
    // finder ring would print as a solid square.
    poly.Fracture();

    std::vector<PCB_SHAPE*> shapes;

    for( int ii = 0; ii < poly.OutlineCount(); ++ii )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( aParent, SHAPE_T::POLY );

        SHAPE_POLY_SET single;
        single.AddOutline( poly.Outline( ii ) );

        shape->SetPolyShape( single );
        shape->SetFillMode( FILL_T::FILLED_SHAPE );
        shape->SetWidth( 0 );
        shape->SetLayer( aBarcode->GetLayer() );
        shape->SetLocked( aBarcode->IsLocked() );
        shapes.push_back( shape );
    }

    return shapes;
}


static void lowerBarcodesFp( FOOTPRINT* aFootprint )
{
    std::vector<PCB_BARCODE*> barcodes;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_BARCODE_T )
            barcodes.push_back( static_cast<PCB_BARCODE*>( item ) );
    }

    for( PCB_BARCODE* barcode : barcodes )
    {
        EDA_GROUP* group = barcode->GetParentGroup();

        for( PCB_SHAPE* shape : barcodeToShapes( barcode, aFootprint ) )
        {
            aFootprint->Add( shape );

            if( group )
                group->AddItem( shape );
        }

        detachFromGroup( barcode );
        aFootprint->Remove( barcode );
        delete barcode;
    }
}


static void dropPcbPointsFp( FOOTPRINT* aFootprint )
{
    for( PCB_POINT* point : aFootprint->Points() )
    {
        detachFromGroup( point );
        delete point;
    }

    aFootprint->Points().clear();
}


static void dropExtrudedBodiesFp( FOOTPRINT* aFootprint )
{
    aFootprint->ClearExtrudedBody();
}


static void dropPadSimTypesFp( FOOTPRINT* aFootprint )
{
    for( PAD* pad : aFootprint->Pads() )
        pad->SetSimElectricalType( PAD_SIM_ELECTRICAL_TYPE::NONE );
}


static void dropUnitInfoFp( FOOTPRINT* aFootprint )
{
    aFootprint->SetUnitInfo( {} );
}


static void dropJumperPadsFp( FOOTPRINT* aFootprint )
{
    aFootprint->JumperPadGroups().Clear();
    aFootprint->SetDuplicatePadNumbersAreJumpers( false );
}


static void dropPcbVariantsFp( FOOTPRINT* aFootprint )
{
    std::vector<wxString> names;

    for( const auto& [name, variant] : aFootprint->GetVariants() )
        names.push_back( name );

    for( const wxString& name : names )
        aFootprint->DeleteVariant( name );
}


static void dropPostMachiningFp( FOOTPRINT* aFootprint )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        pad->Padstack().FrontPostMachining().mode.reset();
        pad->Padstack().BackPostMachining().mode.reset();
    }
}


static void dropPressFitPadsFp( FOOTPRINT* aFootprint )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        if( pad->GetProperty() == PAD_PROP::PRESSFIT )
            pad->SetProperty( PAD_PROP::NONE );
    }
}


static void dropDieDelaysFp( FOOTPRINT* aFootprint )
{
    for( PAD* pad : aFootprint->Pads() )
        pad->SetPadToDieDelay( 0 );
}


static void dropGroupDesignBlocksFp( FOOTPRINT* aFootprint )
{
    for( PCB_GROUP* group : aFootprint->Groups() )
        group->SetDesignBlockLibId( LIB_ID() );
}


static int countNetChains( const BOARD* aBoard )
{
    int count = 0;

    // Match what the writer emits: a named chain, or a net with terminal pads but no chain name.
    for( NETINFO_ITEM* net : aBoard->GetNetInfo() )
    {
        if( !net->GetNetChain().IsEmpty() || net->GetTerminalPad( 0 ) || net->GetTerminalPad( 1 ) )
            count++;
    }

    return count;
}


static void dropNetChains( BOARD* aBoard )
{
    for( NETINFO_ITEM* net : aBoard->GetNetInfo() )
    {
        net->SetNetChain( wxEmptyString );
        net->ClearTerminalPad( 0 ); // terminal_pad is part of the net-chain feature
        net->ClearTerminalPad( 1 );
    }

    std::vector<wxString> chains;

    for( const auto& [chain, color] : aBoard->GetNetChainColors() )
        chains.push_back( chain );

    for( const wxString& chain : chains )
        aBoard->SetNetChainColor( chain, KIGFX::COLOR4D::UNSPECIFIED );
}


static int countEllipses( const BOARD* aBoard )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( isEllipse( item ) )
            count++;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( isEllipse( item ) )
                count++;
        }
    }

    return count;
}


static void lowerEllipses( BOARD* aBoard )
{
    int maxError = aBoard->GetDesignSettings().m_MaxError;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( isEllipse( item ) )
            ellipseToPolygon( static_cast<PCB_SHAPE*>( item ), maxError );
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        lowerEllipsesFp( fp );
}


static int countExtrudedBodies( const BOARD* aBoard )
{
    int count = 0;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->HasExtrudedBody() )
            count++;
    }

    return count;
}


static void dropExtrudedBodies( BOARD* aBoard )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropExtrudedBodiesFp( fp );
}


static int countThievingZones( const BOARD* aBoard )
{
    int count = 0;

    for( ZONE* zone : aBoard->Zones() )
    {
        if( zone->IsCopperThieving() )
            count++;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( ZONE* zone : fp->Zones() )
        {
            if( zone->IsCopperThieving() )
                count++;
        }
    }

    return count;
}


static int countPadSimTypes( const BOARD* aBoard )
{
    int count = 0;

    for( PAD* pad : aBoard->GetPads() )
    {
        if( pad->GetSimElectricalType() != PAD_SIM_ELECTRICAL_TYPE::NONE )
            count++;
    }

    return count;
}


static void dropPadSimTypes( BOARD* aBoard )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropPadSimTypesFp( fp );
}


static int countUnitInfo( const BOARD* aBoard )
{
    int count = 0;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        // Every footprint carries a default single unit. Only more than one unit holds real
        // gate-swap information, so only that is a reportable loss.
        if( fp->GetUnitInfo().size() > 1 )
            count++;
    }

    return count;
}


static void dropUnitInfo( BOARD* aBoard )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropUnitInfoFp( fp );
}


static int countBarcodes( const BOARD* aBoard )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( item->Type() == PCB_BARCODE_T )
            count++;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_BARCODE_T )
                count++;
        }
    }

    return count;
}


static void lowerBarcodes( BOARD* aBoard )
{
    std::vector<PCB_BARCODE*> barcodes;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( item->Type() == PCB_BARCODE_T )
            barcodes.push_back( static_cast<PCB_BARCODE*>( item ) );
    }

    for( PCB_BARCODE* barcode : barcodes )
    {
        EDA_GROUP* group = barcode->GetParentGroup();

        for( PCB_SHAPE* shape : barcodeToShapes( barcode, aBoard ) )
        {
            aBoard->Add( shape );

            if( group )
                group->AddItem( shape );
        }

        aBoard->Remove( barcode );
        delete barcode;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        lowerBarcodesFp( fp );
}


static int countPcbVariants( const BOARD* aBoard )
{
    int count = aBoard->GetVariantNames().empty() ? 0 : 1;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( !fp->GetVariants().empty() )
            count++;
    }

    return count;
}


static void dropPcbVariants( BOARD* aBoard )
{
    aBoard->SetVariantNames( {} );
    aBoard->SetCurrentVariant( wxEmptyString );

    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropPcbVariantsFp( fp );
}


static int countJumperPads( const BOARD* aBoard )
{
    int count = 0;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( !fp->JumperPadGroups().IsEmpty() || fp->GetDuplicatePadNumbersAreJumpers() )
            count++;
    }

    return count;
}


static void dropJumperPads( BOARD* aBoard )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropJumperPadsFp( fp );
}


static bool hasBackdrill( const PADSTACK& aPadstack )
{
    return aPadstack.SecondaryDrill().size.x > 0 || aPadstack.TertiaryDrill().size.x > 0;
}


static int countBackdrills( const BOARD* aBoard )
{
    int count = 0;

    for( PAD* pad : aBoard->GetPads() )
    {
        if( hasBackdrill( pad->Padstack() ) )
            count++;
    }

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T && hasBackdrill( static_cast<PCB_VIA*>( track )->Padstack() ) )
            count++;
    }

    return count;
}


static bool hasPostMachining( const PADSTACK& aPadstack )
{
    auto set = []( const PADSTACK::POST_MACHINING_PROPS& aProps )
    {
        return aProps.mode.has_value() && aProps.mode != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED;
    };

    return set( aPadstack.FrontPostMachining() ) || set( aPadstack.BackPostMachining() );
}


static int countPostMachining( const BOARD* aBoard )
{
    int count = 0;

    for( PAD* pad : aBoard->GetPads() )
    {
        if( hasPostMachining( pad->Padstack() ) )
            count++;
    }

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T && hasPostMachining( static_cast<PCB_VIA*>( track )->Padstack() ) )
            count++;
    }

    return count;
}


static void dropPostMachining( BOARD* aBoard )
{
    auto clear = []( PADSTACK& aPadstack )
    {
        aPadstack.FrontPostMachining().mode.reset();
        aPadstack.BackPostMachining().mode.reset();
    };

    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropPostMachiningFp( fp );

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            clear( static_cast<PCB_VIA*>( track )->Padstack() );
    }
}


static int countPressFitPads( const BOARD* aBoard )
{
    int count = 0;

    for( PAD* pad : aBoard->GetPads() )
    {
        if( pad->GetProperty() == PAD_PROP::PRESSFIT )
            count++;
    }

    return count;
}


static void dropPressFitPads( BOARD* aBoard )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropPressFitPadsFp( fp );
}


static int countBuriedVias( const BOARD* aBoard )
{
    int count = 0;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T && static_cast<PCB_VIA*>( track )->GetViaType() == VIATYPE::BURIED )
            count++;
    }

    return count;
}


// The old format had one blind type covering both, so a buried via becomes blind.
static void lowerBuriedVias( BOARD* aBoard )
{
    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            if( via->GetViaType() == VIATYPE::BURIED )
                via->SetViaType( VIATYPE::BLIND );
        }
    }
}


static int countZoneDefaults( const BOARD* aBoard )
{
    return aBoard->GetDesignSettings().m_ZoneLayerProperties.empty() ? 0 : 1;
}


static void dropZoneDefaults( BOARD* aBoard )
{
    aBoard->GetDesignSettings().m_ZoneLayerProperties.clear();
}


static int countSkipVias( const BOARD* aBoard )
{
    int count = 0;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T
            && static_cast<PCB_VIA*>( track )->Padstack().UnconnectedLayerMode()
                       == UNCONNECTED_LAYER_MODE::START_END_ONLY )
        {
            count++;
        }
    }

    return count;
}


// Skip vias keep copper only on the start and end layers. The nearest older mode keeps the outer
// rings and drops the inner ones, so use that.
static void lowerSkipVias( BOARD* aBoard )
{
    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PADSTACK& padstack = static_cast<PCB_VIA*>( track )->Padstack();

        if( padstack.UnconnectedLayerMode() == UNCONNECTED_LAYER_MODE::START_END_ONLY )
            padstack.SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
    }
}


static int countDieDelays( const BOARD* aBoard )
{
    int count = 0;

    for( PAD* pad : aBoard->GetPads() )
    {
        if( pad->GetPadToDieDelay() != 0 )
            count++;
    }

    return count;
}


static void dropDieDelays( BOARD* aBoard )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropDieDelaysFp( fp );
}


static int countHatchedFills( const BOARD* aBoard, const std::function<bool( const BOARD_ITEM* )>& aMatches )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( aMatches( item ) )
            count++;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( aMatches( item ) )
                count++;
        }
    }

    return count;
}


static void setHatchedFills( BOARD* aBoard, const std::function<bool( const BOARD_ITEM* )>& aMatches, FILL_T aFill )
{
    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( aMatches( item ) )
            static_cast<PCB_SHAPE*>( item )->SetFillMode( aFill );
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        setHatchedFillsFp( fp, aMatches, aFill );
}


static int countPcbPoints( const BOARD* aBoard )
{
    int count = static_cast<int>( aBoard->Points().size() );

    for( FOOTPRINT* fp : aBoard->Footprints() )
        count += static_cast<int>( fp->Points().size() );

    return count;
}


static void dropPcbPoints( BOARD* aBoard )
{
    std::vector<BOARD_ITEM*> points( aBoard->Points().begin(), aBoard->Points().end() );

    for( BOARD_ITEM* item : points )
    {
        aBoard->Remove( item );
        delete item;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropPcbPointsFp( fp );
}


static int countHighUserLayers( const BOARD* aBoard )
{
    const LSET& enabled = aBoard->GetEnabledLayers();
    int         count = 0;

    // User_10 and up only exist since 20241229. Odd ids in this range are the user layers.
    for( int layer = User_10; layer <= User_45; layer += 2 )
    {
        if( enabled.Contains( PCB_LAYER_ID( layer ) ) )
            count++;
    }

    return count;
}


static bool hasViaProtection( const PCB_VIA* aVia )
{
    const PADSTACK& padstack = aVia->Padstack();

    return padstack.Drill().is_filled.has_value() || padstack.Drill().is_capped.has_value()
           || padstack.FrontOuterLayers().has_covering.has_value()
           || padstack.BackOuterLayers().has_covering.has_value()
           || padstack.FrontOuterLayers().has_plugging.has_value()
           || padstack.BackOuterLayers().has_plugging.has_value();
}


static int countViaProtection( const BOARD* aBoard )
{
    int count = 0;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T && hasViaProtection( static_cast<PCB_VIA*>( track ) ) )
            count++;
    }

    const BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    if( bds.m_CoverViasFront || bds.m_CoverViasBack || bds.m_PlugViasFront || bds.m_PlugViasBack || bds.m_CapVias
        || bds.m_FillVias )
    {
        count++;
    }

    return count;
}


static void dropViaProtection( BOARD* aBoard )
{
    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PADSTACK& padstack = static_cast<PCB_VIA*>( track )->Padstack();

        padstack.Drill().is_filled.reset();
        padstack.Drill().is_capped.reset();
        padstack.FrontOuterLayers().has_covering.reset();
        padstack.BackOuterLayers().has_covering.reset();
        padstack.FrontOuterLayers().has_plugging.reset();
        padstack.BackOuterLayers().has_plugging.reset();
    }

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    bds.m_CoverViasFront = false;
    bds.m_CoverViasBack = false;
    bds.m_PlugViasFront = false;
    bds.m_PlugViasBack = false;
    bds.m_CapVias = false;
    bds.m_FillVias = false;
}


static int countDielectricModels( const BOARD* aBoard )
{
    const BOARD_STACKUP& stackup = aBoard->GetDesignSettings().GetStackupDescriptor();

    int count = 0;

    for( BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        if( item->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            continue;

        for( int idx = 0; idx < item->GetSublayersCount(); idx++ )
        {
            if( item->GetDielectricModel( idx ) != DIELECTRIC_MODEL::CONSTANT )
                count++;
        }
    }

    return count;
}


static void dropDielectricModels( BOARD* aBoard )
{
    BOARD_STACKUP& stackup = aBoard->GetDesignSettings().GetStackupDescriptor();

    for( BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        if( item->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            continue;

        for( int idx = 0; idx < item->GetSublayersCount(); idx++ )
            item->SetDielectricModel( DIELECTRIC_MODEL::CONSTANT, idx );
    }
}


static int countGroupDesignBlocks( const BOARD* aBoard )
{
    int count = 0;

    for( PCB_GROUP* group : aBoard->Groups() )
    {
        if( group->HasDesignBlockLink() )
            count++;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( PCB_GROUP* group : fp->Groups() )
        {
            if( group->HasDesignBlockLink() )
                count++;
        }
    }

    return count;
}


static void dropGroupDesignBlocks( BOARD* aBoard )
{
    for( PCB_GROUP* group : aBoard->Groups() )
        group->SetDesignBlockLibId( LIB_ID() );

    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropGroupDesignBlocksFp( fp );
}


static int countScaledFootprints( const BOARD* aBoard )
{
    int count = 0;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->GetTransform().GetScaleX() != 1.0 || fp->GetTransform().GetScaleY() != 1.0 )
            count++;
    }

    return count;
}


static bool isRoundedRect( const BOARD_ITEM* aItem )
{
    if( aItem->Type() != PCB_SHAPE_T )
        return false;

    const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );
    return shape->GetShape() == SHAPE_T::RECTANGLE && shape->GetCornerRadius() > 0;
}


static void roundedRectToPolygon( PCB_SHAPE* aShape, int aMaxError )
{
    int radius = aShape->GetCornerRadius();

    SHAPE_POLY_SET poly;
    poly.NewOutline();

    for( const VECTOR2I& corner : aShape->GetRectCorners() )
        poly.Append( corner );

    poly = poly.Fillet( radius, aMaxError );

    aShape->SetCornerRadius( 0 );
    aShape->SetShape( SHAPE_T::POLY );
    aShape->SetPolyShape( poly );
}


// Rounded rectangles can also live in custom pad primitives.
static void forEachPadPrimitive( FOOTPRINT* aFootprint, const std::function<void( PCB_SHAPE* )>& aFunc )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        pad->Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID aLayer )
                {
                    for( const std::shared_ptr<PCB_SHAPE>& primitive : pad->GetPrimitives( aLayer ) )
                        aFunc( primitive.get() );
                } );
    }
}


static int countRoundedRectsFp( const FOOTPRINT* aFootprint )
{
    int count = 0;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( isRoundedRect( item ) )
            count++;
    }

    forEachPadPrimitive( const_cast<FOOTPRINT*>( aFootprint ),
                         [&]( PCB_SHAPE* aPrimitive )
                         {
                             if( isRoundedRect( aPrimitive ) )
                                 count++;
                         } );

    return count;
}


static void lowerRoundedRectsFp( FOOTPRINT* aFootprint )
{
    int maxError = footprintMaxError( aFootprint );

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( isRoundedRect( item ) )
            roundedRectToPolygon( static_cast<PCB_SHAPE*>( item ), maxError );
    }

    forEachPadPrimitive( aFootprint,
                         [&]( PCB_SHAPE* aPrimitive )
                         {
                             if( isRoundedRect( aPrimitive ) )
                                 roundedRectToPolygon( aPrimitive, maxError );
                         } );
}


static int countRoundedRects( const BOARD* aBoard )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( isRoundedRect( item ) )
            count++;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        count += countRoundedRectsFp( fp );

    return count;
}


static void lowerRoundedRects( BOARD* aBoard )
{
    int maxError = aBoard->GetDesignSettings().m_MaxError;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( isRoundedRect( item ) )
            roundedRectToPolygon( static_cast<PCB_SHAPE*>( item ), maxError );
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        lowerRoundedRectsFp( fp );
}


static int countKnockoutTextBoxesFp( const FOOTPRINT* aFootprint )
{
    int count = 0;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_TEXTBOX_T && item->IsKnockout() )
            count++;
    }

    return count;
}


static void dropKnockoutTextBoxesFp( FOOTPRINT* aFootprint )
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_TEXTBOX_T && item->IsKnockout() )
            item->SetIsKnockout( false );
    }
}


static int countKnockoutTextBoxes( const BOARD* aBoard )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( item->Type() == PCB_TEXTBOX_T && item->IsKnockout() )
            count++;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        count += countKnockoutTextBoxesFp( fp );

    return count;
}


static void dropKnockoutTextBoxes( BOARD* aBoard )
{
    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( item->Type() == PCB_TEXTBOX_T && item->IsKnockout() )
            item->SetIsKnockout( false );
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropKnockoutTextBoxesFp( fp );
}


static int countKnockoutCellsIn( const BOARD_ITEM* aItem )
{
    if( aItem->Type() != PCB_TABLE_T )
        return 0;

    int count = 0;

    for( PCB_TABLECELL* cell : static_cast<const PCB_TABLE*>( aItem )->GetCells() )
    {
        if( cell->IsKnockout() )
            count++;
    }

    return count;
}


static int countKnockoutTableCells( const BOARD* aBoard )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
        count += countKnockoutCellsIn( item );

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( BOARD_ITEM* item : fp->GraphicalItems() )
            count += countKnockoutCellsIn( item );
    }

    return count;
}


static void dropKnockoutCellsIn( BOARD_ITEM* aItem )
{
    if( aItem->Type() != PCB_TABLE_T )
        return;

    for( PCB_TABLECELL* cell : static_cast<PCB_TABLE*>( aItem )->GetCells() )
    {
        if( cell->IsKnockout() )
            cell->SetIsKnockout( false );
    }
}


static void dropKnockoutTableCellsFp( FOOTPRINT* aFootprint )
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
        dropKnockoutCellsIn( item );
}


static void dropKnockoutTableCells( BOARD* aBoard )
{
    for( BOARD_ITEM* item : aBoard->Drawings() )
        dropKnockoutCellsIn( item );

    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropKnockoutTableCellsFp( fp );
}


static int countZoneLayerProperties( const BOARD* aBoard )
{
    int count = 0;

    for( ZONE* zone : aBoard->Zones() )
    {
        if( !zone->LayerProperties().empty() )
            count++;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( ZONE* zone : fp->Zones() )
        {
            if( !zone->LayerProperties().empty() )
                count++;
        }
    }

    return count;
}


static void dropZoneLayerPropertiesFp( FOOTPRINT* aFootprint )
{
    for( ZONE* zone : aFootprint->Zones() )
        zone->SetLayerProperties( {} );
}


static void dropZoneLayerProperties( BOARD* aBoard )
{
    for( ZONE* zone : aBoard->Zones() )
        zone->SetLayerProperties( {} );

    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropZoneLayerPropertiesFp( fp );
}


static int countCustomFpStackups( const BOARD* aBoard )
{
    int count = 0;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->GetStackupMode() != FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS )
            count++;
    }

    return count;
}


// Omission is distinct from lowering: do not leave replacement polygons, plain knockout
// text, or electrically different vias behind when the user declines approximations.
static void omitGraphicsFp( FOOTPRINT* aFootprint, const std::function<bool( const BOARD_ITEM* )>& aMatches )
{
    std::vector<BOARD_ITEM*> remove;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( aMatches( item ) )
            remove.push_back( item );
    }

    for( BOARD_ITEM* item : remove )
    {
        detachFromGroup( item );
        aFootprint->Remove( item );
        delete item;
    }
}


static void omitGraphics( BOARD* aBoard, const std::function<bool( const BOARD_ITEM* )>& aMatches )
{
    std::vector<BOARD_ITEM*> remove;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( aMatches( item ) )
            remove.push_back( item );
    }

    for( BOARD_ITEM* item : remove )
    {
        aBoard->Remove( item );
        delete item;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        omitGraphicsFp( fp, aMatches );
}


static void omitEllipsesFp( FOOTPRINT* aFootprint )
{
    omitGraphicsFp( aFootprint, isEllipse );
}
static void omitEllipses( BOARD* aBoard )
{
    omitGraphics( aBoard, isEllipse );
}

static bool isBarcode( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_BARCODE_T;
}
static void omitBarcodesFp( FOOTPRINT* aFootprint )
{
    omitGraphicsFp( aFootprint, isBarcode );
}
static void omitBarcodes( BOARD* aBoard )
{
    omitGraphics( aBoard, isBarcode );
}

static bool isKnockoutTextBox( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_TEXTBOX_T && aItem->IsKnockout();
}

static void omitKnockoutTextBoxesFp( FOOTPRINT* aFootprint )
{
    omitGraphicsFp( aFootprint, isKnockoutTextBox );
}
static void omitKnockoutTextBoxes( BOARD* aBoard )
{
    omitGraphics( aBoard, isKnockoutTextBox );
}


static void omitRoundedPadPrimitives( FOOTPRINT* aFootprint )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        pad->Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID aLayer )
                {
                    auto& primitives = pad->Padstack().Primitives( aLayer );
                    primitives.erase( std::remove_if( primitives.begin(), primitives.end(),
                                                      []( const std::shared_ptr<PCB_SHAPE>& aPrimitive )
                                                      {
                                                          return isRoundedRect( aPrimitive.get() );
                                                      } ),
                                      primitives.end() );
                } );
        pad->SetDirty();
    }
}


static void omitRoundedRectsFp( FOOTPRINT* aFootprint )
{
    omitGraphicsFp( aFootprint, isRoundedRect );
    omitRoundedPadPrimitives( aFootprint );
}


static void omitRoundedRects( BOARD* aBoard )
{
    omitGraphics( aBoard, isRoundedRect );

    for( FOOTPRINT* fp : aBoard->Footprints() )
        omitRoundedPadPrimitives( fp );
}


static void omitKnockoutCellContents( BOARD_ITEM* aItem )
{
    if( aItem->Type() != PCB_TABLE_T )
        return;

    for( PCB_TABLECELL* cell : static_cast<PCB_TABLE*>( aItem )->GetCells() )
    {
        if( cell->IsKnockout() )
        {
            // Keep the supported grid structure, but not a plain-text approximation of the ink.
            cell->SetText( wxEmptyString );
            cell->SetIsKnockout( false );
        }
    }
}


static void omitKnockoutTableCellsFp( FOOTPRINT* aFootprint )
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
        omitKnockoutCellContents( item );
}


static void omitKnockoutTableCells( BOARD* aBoard )
{
    for( BOARD_ITEM* item : aBoard->Drawings() )
        omitKnockoutCellContents( item );

    for( FOOTPRINT* fp : aBoard->Footprints() )
        omitKnockoutTableCellsFp( fp );
}


static int countConstraints( const BOARD* aBoard )
{
    int count = aBoard->Constraints().size();

    for( const FOOTPRINT* fp : aBoard->Footprints() )
        count += fp->Constraints().size();

    return count;
}


static void dropConstraintsFp( FOOTPRINT* aFootprint )
{
    const auto constraints = aFootprint->Constraints();

    for( PCB_CONSTRAINT* constraint : constraints )
    {
        detachFromGroup( constraint );
        aFootprint->Remove( constraint );
        delete constraint;
    }
}


static void dropConstraints( BOARD* aBoard )
{
    const auto constraints = aBoard->Constraints();

    for( PCB_CONSTRAINT* constraint : constraints )
    {
        aBoard->Remove( constraint );
        delete constraint;
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropConstraintsFp( fp );
}


static bool isGrid( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_GRID_ITEM_T;
}


static bool isDrillDrawing( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_DRILL_CHART_T || aItem->Type() == PCB_DRILL_MAP_T;
}


static int countDrawings( const BOARD* aBoard, const std::function<bool( const BOARD_ITEM* )>& aMatches )
{
    return std::count_if( aBoard->Drawings().begin(), aBoard->Drawings().end(), aMatches );
}


static int countGrids( const BOARD* aBoard )
{
    return countDrawings( aBoard, isGrid );
}


static void dropGrids( BOARD* aBoard )
{
    omitGraphics( aBoard, isGrid );
}


static int countDrillDrawings( const BOARD* aBoard )
{
    return countDrawings( aBoard, isDrillDrawing );
}


static void dropDrillDrawings( BOARD* aBoard )
{
    omitGraphics( aBoard, isDrillDrawing );
}


static int countDrillSymbolConfiguration( const BOARD* aBoard )
{
    return aBoard->GetDesignSettings().GetDrillSymbolProfile() == DRILL_SYMBOL_PROFILE() ? 0 : 1;
}


static void dropDrillSymbolConfiguration( BOARD* aBoard )
{
    aBoard->GetDesignSettings().GetDrillSymbolProfile() = DRILL_SYMBOL_PROFILE();
}


static int countGenerators( const BOARD* aBoard, const wxString& aType )
{
    return std::count_if( aBoard->Generators().begin(), aBoard->Generators().end(),
                          [&]( const PCB_GENERATOR* aGenerator )
                          {
                              return aGenerator->GetGeneratorType() == aType;
                          } );
}


static void dropGenerators( BOARD* aBoard, const wxString& aType )
{
    const auto generators = aBoard->Generators();

    for( PCB_GENERATOR* generator : generators )
    {
        if( generator->GetGeneratorType() != aType )
            continue;

        EDA_GROUP* outer = generator->GetParentGroup();
        const auto members = generator->GetItems();
        generator->RemoveAll();

        if( outer )
        {
            for( EDA_ITEM* member : members )
                outer->AddItem( member );
        }

        aBoard->Remove( generator );
        delete generator;
    }
}


static int countUnknownGenerators( const BOARD* aBoard )
{
    int count = 0;

    for( PCB_GENERATOR* generator : aBoard->Generators() )
    {
        const wxString type = generator->GetGeneratorType();

        if( type != wxT( "tuning_pattern" ) && type != wxT( "via_stitch" ) && type != wxT( "via_stack" ) )
            ++count;

        for( EDA_ITEM* member : generator->GetItems() )
        {
            // Detached children would disappear when ownership metadata is removed.
            if( !dynamic_cast<BOARD_ITEM*>( member )
                || !aBoard->IsItemIndexedById( static_cast<BOARD_ITEM*>( member ) ) )
            {
                ++count;
                break;
            }
        }
    }

    return count;
}


static bool isUnreviewedDrawing( const BOARD_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_SHAPE_T:
    case PCB_TEXT_T:
    case PCB_FIELD_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_TARGET_T:
    case PCB_BARCODE_T:
    case PCB_GRID_ITEM_T:
    case PCB_DRILL_CHART_T:
    case PCB_DRILL_MAP_T: return false;

    default: return true;
    }
}


static int countUnreviewedDrawings( const BOARD* aBoard )
{
    int count = countDrawings( aBoard, isUnreviewedDrawing );

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        count += std::count_if( fp->GraphicalItems().begin(), fp->GraphicalItems().end(), isUnreviewedDrawing );
    }

    return count;
}


static void visitFootprintItems( const FOOTPRINT* aFootprint, const std::function<void( BOARD_ITEM* )>& aVisit )
{
    aVisit( const_cast<FOOTPRINT*>( aFootprint ) );
    aFootprint->RunOnChildren( aVisit, RECURSE_MODE::RECURSE );
}


static void visitBoardItems( const BOARD* aBoard, const std::function<void( BOARD_ITEM* )>& aVisit )
{
    aBoard->RunOnChildren( aVisit, RECURSE_MODE::RECURSE );

    // Generators are omitted by BOARD::RunOnChildren.
    for( PCB_GENERATOR* generator : aBoard->Generators() )
    {
        aVisit( generator );

        for( const auto& [name, item] : generator->GetTemplateItems() )
        {
            if( item )
                aVisit( const_cast<BOARD_ITEM*>( item ) );
        }
    }
}


static int countCustomProperties( const BOARD* aBoard )
{
    int count = 0;
    visitBoardItems( aBoard,
                     [&]( BOARD_ITEM* aItem )
                     {
                         if( aItem && aItem->HasCustomProperties() )
                             ++count;
                     } );
    return count;
}


static void dropCustomPropertiesFp( FOOTPRINT* aFootprint )
{
    visitFootprintItems( aFootprint,
                         []( BOARD_ITEM* aItem )
                         {
                             if( aItem )
                                 aItem->ClearCustomProperties();
                         } );
}


static void dropCustomProperties( BOARD* aBoard )
{
    visitBoardItems( aBoard,
                     []( BOARD_ITEM* aItem )
                     {
                         if( aItem )
                             aItem->ClearCustomProperties();
                     } );
}


static int countSimulationExclusions( const BOARD* aBoard )
{
    int count = 0;

    for( const FOOTPRINT* fp : aBoard->Footprints() )
    {
        bool excluded = fp->IsExcludedFromSim();

        for( const auto& [name, variant] : fp->GetVariants() )
            excluded = excluded || variant.GetExcludedFromSim();

        if( excluded )
            ++count;
    }

    return count;
}


static void dropSimulationExclusionsFp( FOOTPRINT* aFootprint )
{
    aFootprint->SetExcludedFromSim( false );

    for( const auto& [name, variant] : aFootprint->GetVariants() )
        aFootprint->GetVariant( name )->SetExcludedFromSim( false );
}


static void dropSimulationExclusions( BOARD* aBoard )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
        dropSimulationExclusionsFp( fp );
}


static bool hasLineEndings( const BOARD_ITEM* aItem )
{
    if( !aItem || aItem->Type() != PCB_SHAPE_T )
        return false;

    const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );
    return shape->GetStartEndingStyle() != LINE_ENDING_STYLE::NONE
           || shape->GetEndEndingStyle() != LINE_ENDING_STYLE::NONE;
}


static bool unsafeLineEndings( const PCB_SHAPE* aShape )
{
    VECTOR2I start, end;

    if( !aShape->GetLineEndingEndpoints( start, end ) )
        return false;

    const auto unsafeStroke = []( const STROKE_PARAMS& aStroke )
    {
        return ( aStroke.GetLineStyle() != LINE_STYLE::DEFAULT && aStroke.GetLineStyle() != LINE_STYLE::SOLID )
               || aStroke.GetColor() != KIGFX::COLOR4D::UNSPECIFIED;
    };

    // The native polygon helper does not preserve dash patterns or ending colors.
    return aShape->GetEffectiveWidth() <= 0
           || ( aShape->GetLineStyle() != LINE_STYLE::DEFAULT && aShape->GetLineStyle() != LINE_STYLE::SOLID )
           || unsafeStroke( aShape->GetStartEnding().GetStroke() )
           || unsafeStroke( aShape->GetEndEnding().GetStroke() );
}


static int countLineEndings( const BOARD* aBoard, bool aUnsafeOnly = false )
{
    int count = 0;
    visitBoardItems( aBoard,
                     [&]( BOARD_ITEM* aItem )
                     {
                         if( hasLineEndings( aItem )
                             && ( !aUnsafeOnly || unsafeLineEndings( static_cast<PCB_SHAPE*>( aItem ) ) ) )
                             ++count;
                     } );
    return count;
}


static void clearLineEndings( PCB_SHAPE* aShape )
{
    aShape->SetStartEnding( LINE_ENDING() );
    aShape->SetEndEnding( LINE_ENDING() );
}


static void lowerLineEndingShape( PCB_SHAPE* aShape, int aMaxError )
{
    VECTOR2I start, end;

    if( !aShape->GetLineEndingEndpoints( start, end ) )
    {
        clearLineEndings( aShape );
        return;
    }

    SHAPE_POLY_SET ink;
    aShape->TransformWithLineEndingsToPolygon( ink, 0, aMaxError, ERROR_INSIDE );
    ink.Simplify();
    ink.Fracture();
    EDA_GROUP*            group = aShape->GetParentGroup();
    BOARD_ITEM_CONTAINER* parent = aShape->GetParent();

    for( int outline = 0; outline < ink.OutlineCount(); ++outline )
    {
        PCB_SHAPE*     polygon = outline == 0 ? aShape : new PCB_SHAPE( *aShape );
        SHAPE_POLY_SET poly;
        poly.AddOutline( ink.COutline( outline ) );
        polygon->SetShape( SHAPE_T::POLY );
        polygon->SetPolyShape( poly );
        polygon->SetWidth( 0 );
        polygon->SetFillMode( FILL_T::FILLED_SHAPE );
        polygon->SetLineStyle( LINE_STYLE::SOLID );
        clearLineEndings( polygon );

        if( outline > 0 )
        {
            const_cast<KIID&>( polygon->m_Uuid ) = KIID();
            polygon->SetParentGroup( nullptr );
            parent->Add( polygon );

            if( group )
                group->AddItem( polygon );
        }
    }

    if( ink.OutlineCount() == 0 )
        clearLineEndings( aShape );
}


static void transformLineEndingsFp( FOOTPRINT* aFootprint, bool aOmit )
{
    std::vector<PCB_SHAPE*> shapes;
    visitFootprintItems( aFootprint,
                         [&]( BOARD_ITEM* aItem )
                         {
                             if( hasLineEndings( aItem ) )
                                 shapes.push_back( static_cast<PCB_SHAPE*>( aItem ) );
                         } );

    for( PCB_SHAPE* shape : shapes )
    {
        if( aOmit )
            clearLineEndings( shape );
        else if( !unsafeLineEndings( shape ) )
            lowerLineEndingShape( shape, footprintMaxError( aFootprint ) );
    }
}


static void transformLineEndings( BOARD* aBoard, bool aOmit )
{
    std::vector<PCB_SHAPE*> shapes;
    visitBoardItems( aBoard,
                     [&]( BOARD_ITEM* aItem )
                     {
                         if( hasLineEndings( aItem ) )
                             shapes.push_back( static_cast<PCB_SHAPE*>( aItem ) );
                     } );

    for( PCB_SHAPE* shape : shapes )
    {
        if( aOmit )
            clearLineEndings( shape );
        else if( !unsafeLineEndings( shape ) )
            lowerLineEndingShape( shape, aBoard->GetDesignSettings().m_MaxError );
    }
}


static int titleFieldIndex( const wxString& aName )
{
    if( aName == wxT( "TITLE" ) )
        return 0;
    if( aName == wxT( "ISSUE_DATE" ) )
        return 1;
    if( aName == wxT( "REVISION" ) )
        return 2;
    if( aName == wxT( "COMPANY" ) )
        return 3;
    if( aName.length() == 8 && aName.StartsWith( wxT( "COMMENT" ) ) && aName[7] >= '1' && aName[7] <= '9' )
        return 4 + aName[7].GetValue() - '1';
    return -1;
}


static const wxString& titleFieldText( const TITLE_BLOCK& aTitle, int aIndex )
{
    switch( aIndex )
    {
    case 0: return aTitle.GetTitle();
    case 1: return aTitle.GetDate();
    case 2: return aTitle.GetRevision();
    case 3: return aTitle.GetCompany();
    default: return aTitle.GetComment( aIndex - 4 );
    }
}


static bool postTargetText( const wxString& aText, const BOARD* aBoard, int aTargetVersion,
                            std::set<wxString>& aDependencies, int aDepth = 0 )
{
    if( aDepth > 32 )
        return true;

    for( size_t pos = 0; pos + 1 < aText.length(); ++pos )
    {
        if( aText[pos] == '\\' && pos + 2 < aText.length() && ( aText[pos + 1] == '$' || aText[pos + 1] == '@' )
            && aText[pos + 2] == '{' )
        {
            if( aTargetVersion <= PCB_WRITER_V9::FORMAT_VERSION )
                return true;

            int braces = 1;
            pos += 3;

            for( ; pos < aText.length() && braces; ++pos )
                braces += aText[pos] == '{' ? 1 : ( aText[pos] == '}' ? -1 : 0 );

            --pos;
            continue;
        }

        if( aText[pos] == '@' && aText[pos + 1] == '{'
            && ( aTargetVersion <= PCB_WRITER_V9::FORMAT_VERSION || aText.Mid( pos ).Contains( wxT( "mils" ) ) ) )
            return true;

        if( aText[pos] == '$' && aText[pos + 1] == '{'
            && aText.Mid( pos + 2 ).BeforeFirst( '}' ).AfterFirst( ':' ).StartsWith( wxT( "PROPERTY." ) ) )
            return true;
    }

    for( const auto& reference : ExtractTextVarReferences( aText ) )
    {
        const wxString& name = reference.primary;

        if( name.StartsWith( wxT( "PROPERTY." ) ) || reference.secondary.StartsWith( wxT( "PROPERTY." ) )
            || name == wxT( "DRILL_OPERATIONS" ) || name == wxT( "DRILL_SITES" ) || name == wxT( "DRILL_GROUPS" ) )
            return true;

        if( aBoard && titleFieldIndex( name ) >= 0 )
        {
            wxString resolved = name;

            if( aBoard->GetTitleBlock().TextVarResolver( &resolved, aBoard->GetProject(), INTERNAL ) )
            {
                FinalizeTextVarExpansion( resolved, INTERNAL );

                if( aDependencies.insert( name ).second
                    && postTargetText( resolved, aBoard, aTargetVersion, aDependencies, aDepth + 1 ) )
                    return true;

                continue;
            }
        }

        if( aBoard )
        {
            const wxString* value = nullptr;

            if( aBoard->GetProject() )
            {
                const auto& variables = aBoard->GetProject()->GetTextVars();
                auto        variable = variables.find( name );

                if( variable != variables.end() )
                    value = &variable->second;
            }

            if( !value )
            {
                const auto& properties = aBoard->GetProperties();
                auto        property = properties.find( name );

                if( property != properties.end() )
                    value = &property->second;
            }

            if( value && aDependencies.insert( name ).second
                && postTargetText( *value, aBoard, aTargetVersion, aDependencies, aDepth + 1 ) )
                return true;
        }
    }

    return false;
}


static bool postTargetText( const wxString& aText, const BOARD* aBoard, int aTargetVersion )
{
    std::set<wxString> dependencies;
    return postTargetText( aText, aBoard, aTargetVersion, dependencies );
}


struct TEXT_DOWNGRADE_PLAN
{
    std::vector<std::pair<BOARD_ITEM*, wxString>> m_replacements;
    std::vector<std::pair<int, wxString>>         m_titleReplacements;
    BOARD*                                        m_board = nullptr;
    int                                           m_blocked = 0;
};


static bool hasActiveExpression( const wxString& aText )
{
    for( size_t pos = 0; pos + 1 < aText.length(); ++pos )
    {
        if( aText[pos] == '\\' && pos + 2 < aText.length() && ( aText[pos + 1] == '$' || aText[pos + 1] == '@' )
            && aText[pos + 2] == '{' )
        {
            int braces = 1;
            pos += 3;

            for( ; pos < aText.length() && braces; ++pos )
                braces += aText[pos] == '{' ? 1 : ( aText[pos] == '}' ? -1 : 0 );

            --pos;
        }
        else if( aText[pos] == '@' && aText[pos + 1] == '{' )
        {
            return true;
        }
    }

    return false;
}


static void planTextItem( BOARD_ITEM* aItem, int aTargetVersion, TEXT_DOWNGRADE_PLAN& aPlan,
                          const std::function<bool( wxString* )>* aResolver = nullptr )
{
    if( !aItem )
        return;

    EDA_TEXT*           text = dynamic_cast<EDA_TEXT*>( aItem );
    PCB_BARCODE*        barcode = dynamic_cast<PCB_BARCODE*>( aItem );
    PCB_DIMENSION_BASE* dimension = dynamic_cast<PCB_DIMENSION_BASE*>( aItem );

    if( !text && !barcode )
        return;

    if( dimension && !dimension->GetOverrideTextEnabled() )
        return;

    const wxString raw = dimension ? dimension->GetOverrideText() : ( text ? text->GetText() : barcode->GetText() );
    BOARD*         board = aItem->GetBoard();

    if( !postTargetText( raw, board, aTargetVersion ) )
        return;

    std::function<bool( wxString* )> resolver = [&]( wxString* aToken )
    {
        if( aResolver )
            return ( *aResolver )( aToken );

        if( *aToken == wxT( "LAYER" ) && board )
        {
            *aToken = board->GetLayerName( aItem->GetLayer() );
            return true;
        }

        FOOTPRINT* fp = aItem->GetParentFootprint();
        return ( fp && fp->ResolveTextVar( aToken, 0 ) ) || ( board && board->ResolveTextVar( aToken, 0 ) );
    };

    wxString result;
    bool     blocked = false;

    for( size_t pos = 0; pos < raw.length(); )
    {
        size_t start = pos;
        bool   escaped = raw[pos] == '\\' && pos + 2 < raw.length() && ( raw[pos + 1] == '$' || raw[pos + 1] == '@' )
                         && raw[pos + 2] == '{';
        size_t marker = escaped ? pos + 1 : pos;

        if( marker + 1 >= raw.length() || ( raw[marker] != '$' && raw[marker] != '@' ) || raw[marker + 1] != '{' )
        {
            result += raw[pos++];
            continue;
        }

        int braces = 1;
        pos = marker + 2;

        while( pos < raw.length() && braces )
        {
            braces += raw[pos] == '{' ? 1 : ( raw[pos] == '}' ? -1 : 0 );
            ++pos;
        }

        wxString block = raw.Mid( start, pos - start );

        if( escaped || !postTargetText( block, board, aTargetVersion ) )
        {
            result += block;
            continue;
        }

        if( braces )
        {
            blocked = true;
            break;
        }

        if( !aResolver )
        {
            // Resolve only the affected token using the native item-context semantics.
            if( auto cell = dynamic_cast<PCB_TABLECELL*>( aItem ) )
            {
                if( cell->GetRow() < 0 || cell->GetColumn() < 0 )
                {
                    blocked = true;
                    break;
                }

                PCB_TABLE      table( *static_cast<PCB_TABLE*>( cell->GetParent() ) );
                PCB_TABLECELL* temporary = table.GetCell( cell->GetRow(), cell->GetColumn() );

                if( !temporary )
                {
                    blocked = true;
                    break;
                }

                temporary->SetText( block );
                block = temporary->GetUnwrappedShownText( INTERNAL );
            }
            else
            {
                PCB_TEXT temporary( aItem->GetParent() );
                temporary.SetLayer( aItem->GetLayer() );
                temporary.SetText( block );
                block = temporary.GetShownText( INTERNAL );
            }

            if( hasActiveExpression( block ) )
                blocked = true;
        }
        else
        {
            for( int depth = 0; depth < 32; ++depth )
            {
                wxString expanded = ExpandTextVars( block, &resolver, INTERNAL );

                if( hasActiveExpression( expanded ) )
                {
                    EXPRESSION_EVALUATOR evaluator;
                    expanded = evaluator.Evaluate( expanded );

                    if( evaluator.HasErrors() )
                    {
                        blocked = true;
                        break;
                    }
                }

                if( expanded == block )
                    break;

                block = expanded;
            }

            if( hasActiveExpression( block ) )
                blocked = true;
        }

        FinalizeTextVarExpansion( block, INTERNAL );

        if( postTargetText( block, board, aTargetVersion ) || block.Contains( wxT( "<Unresolved:" ) )
            || block.Contains( wxT( "<Unknown reference:" ) ) )
            blocked = true;

        result += block;
    }

    if( aTargetVersion <= PCB_WRITER_V9::FORMAT_VERSION
        && ( result.Contains( wxT( "\\${" ) ) || result.Contains( wxT( "\\@{" ) ) ) )
        blocked = true;

    if( blocked )
        ++aPlan.m_blocked;
    else if( result != raw )
        aPlan.m_replacements.emplace_back( aItem, result );
}


static TEXT_DOWNGRADE_PLAN planBoardText( const BOARD* aBoard, int aTargetVersion )
{
    TEXT_DOWNGRADE_PLAN plan;
    plan.m_board = const_cast<BOARD*>( aBoard );
    visitBoardItems( aBoard,
                     [&]( BOARD_ITEM* aItem )
                     {
                         planTextItem( aItem, aTargetVersion, plan );
                     } );

    std::function<bool( wxString* )> resolver = [&]( wxString* aToken )
    {
        if( *aToken == wxT( "FILENAME" ) || *aToken == wxT( "FILEPATH" ) || *aToken == wxT( "VARIANT" )
            || *aToken == wxT( "VARIANT_DESC" ) )
            return aBoard->ResolveTextVar( aToken, 0 );

        return aBoard->GetTitleBlock().TextVarResolver( aToken, aBoard->GetProject(), INTERNAL )
               || ( aBoard->GetProject() && aBoard->GetProject()->TextVarResolver( aToken ) );
    };

    // Worksheet metadata has no footprint context or fixed plotted layer.
    for( int field = 0; field < 13; ++field )
    {
        PCB_TEXT temporary( const_cast<BOARD*>( aBoard ) );
        temporary.SetText( titleFieldText( aBoard->GetTitleBlock(), field ) );
        TEXT_DOWNGRADE_PLAN title;
        planTextItem( &temporary, aTargetVersion, title, &resolver );
        plan.m_blocked += title.m_blocked;

        if( !title.m_replacements.empty() )
            plan.m_titleReplacements.emplace_back( field, title.m_replacements.front().second );
    }

    return plan;
}


static void applyTextPlan( const TEXT_DOWNGRADE_PLAN& aPlan )
{
    for( const auto& [item, value] : aPlan.m_replacements )
    {
        if( auto dimension = dynamic_cast<PCB_DIMENSION_BASE*>( item ) )
            dimension->ChangeOverrideText( value );
        else if( auto text = dynamic_cast<EDA_TEXT*>( item ) )
            text->SetText( value );
        else if( auto barcode = dynamic_cast<PCB_BARCODE*>( item ) )
            barcode->SetBarcodeText( value );
    }

    for( const auto& [field, value] : aPlan.m_titleReplacements )
    {
        TITLE_BLOCK& title = aPlan.m_board->GetTitleBlock();

        switch( field )
        {
        case 0: title.SetTitle( value ); break;
        case 1: title.SetDate( value ); break;
        case 2: title.SetRevision( value ); break;
        case 3: title.SetCompany( value ); break;
        default: title.SetComment( field - 4, value ); break;
        }
    }
}


// One row per feature. The report and the transform both walk this list, so they cannot drift.
// To add a feature, add a row. A null apply means the feature blocks the export.
struct BOARD_RULE
{
    int                                m_introducedIn;
    DOWNGRADE_BUCKET                   m_bucket;
    wxString                           m_feature;
    wxString                           m_detail;
    std::function<int( const BOARD* )> m_count;
    std::function<void( BOARD* )>      m_apply;
    std::function<void( FOOTPRINT* )>  m_applyFp = {}; ///< Footprint-scoped part, for .kicad_mod files
    std::function<void( BOARD* )>      m_omit = {};
    std::function<void( FOOTPRINT* )>  m_omitFp = {};
    wxString                           m_omitDetail = {};

    /// An approximation the omission preference must not remove, because removing it would change
    /// the board electrically. The preference is a drawing choice, not an electrical override.
    bool m_keepUnderOmitPolicy = false;
};


static std::vector<BOARD_RULE> boardRules()
{
    return {
        { VER_CONSTRAINTS, DOWNGRADE_BUCKET::BLOCK, _( "Unreviewed generated objects" ),
          _( "Unknown generator kinds or detached generated children cannot be safely exported." ),
          countUnknownGenerators, nullptr },
        { VER_CONSTRAINTS, DOWNGRADE_BUCKET::BLOCK, _( "Unreviewed board graphics" ),
          _( "An unrecognized graphic item cannot be safely represented by the target writer." ),
          countUnreviewedDrawings, nullptr },
        { VER_CONSTRAINTS, DOWNGRADE_BUCKET::DROP, _( "Geometric constraints" ),
          _( "Solver relationships are removed. Existing geometry is kept in its current position." ), countConstraints,
          dropConstraints, dropConstraintsFp },
        { VER_GRIDS, DOWNGRADE_BUCKET::DROP, _( "Local grids" ),
          _( "Placement, snapping, and routing grid configuration is removed. Copper is unchanged." ), countGrids,
          dropGrids },
        { VER_VIA_STITCH, DOWNGRADE_BUCKET::DROP, _( "Via stitching and guarding" ),
          _( "Generator editability is removed. Existing generated copper and group membership are kept." ),
          []( const BOARD* b )
          {
              return countGenerators( b, wxT( "via_stitch" ) );
          },
          []( BOARD* b )
          {
              dropGenerators( b, wxT( "via_stitch" ) );
          } },
        { VER_VIA_STACK, DOWNGRADE_BUCKET::DROP, _( "Microvia stack generators" ),
          _( "Generator editability is removed. Existing generated copper and group membership are kept." ),
          []( const BOARD* b )
          {
              return countGenerators( b, wxT( "via_stack" ) );
          },
          []( BOARD* b )
          {
              dropGenerators( b, wxT( "via_stack" ) );
          } },
        { VER_LINE_ENDINGS, DOWNGRADE_BUCKET::LOWER, _( "Graphic line endings" ),
          _( "Converted to printed polygons with the shortened line body. Ending editability is lost." ),
          []( const BOARD* b )
          {
              return countLineEndings( b );
          },
          []( BOARD* b )
          {
              transformLineEndings( b, false );
          },
          []( FOOTPRINT* f )
          {
              transformLineEndingsFp( f, false );
          },
          []( BOARD* b )
          {
              transformLineEndings( b, true );
          },
          []( FOOTPRINT* f )
          {
              transformLineEndingsFp( f, true );
          },
          _( "Endpoint embellishments are removed. The supported line body and its style are kept." ) },
        { VER_SIM_EXCLUSION, DOWNGRADE_BUCKET::DROP, _( "Footprint simulation exclusions" ),
          _( "Simulation exclusions are removed. The target may include these footprints in simulation." ),
          countSimulationExclusions, dropSimulationExclusions, dropSimulationExclusionsFp },
        { VER_CUSTOM_PROPERTIES, DOWNGRADE_BUCKET::DROP, _( "Custom properties" ),
          _( "Custom item metadata is removed after affected displayed text has been resolved." ),
          countCustomProperties, dropCustomProperties, dropCustomPropertiesFp },
        { VER_DRILL_DRAWINGS, DOWNGRADE_BUCKET::DROP, _( "Drill charts and maps" ),
          _( "Drill chart tables and drill map symbols are removed from the fabrication drawings. Holes are kept." ),
          countDrillDrawings, dropDrillDrawings },
        { VER_DRILL_DRAWINGS, DOWNGRADE_BUCKET::DROP, _( "Drill symbol configuration" ),
          _( "Drill symbol assignments and display settings are removed. Holes are unchanged." ),
          countDrillSymbolConfiguration, dropDrillSymbolConfiguration },
        { VER_NET_CHAINS, DOWNGRADE_BUCKET::DROP, _( "Net chains" ),
          _( "No equivalent in the target. Connectivity will change." ), countNetChains, dropNetChains },
        { VER_ELLIPSE, DOWNGRADE_BUCKET::LOWER, _( "Ellipse graphics" ), _( "Approximated by a polygon." ),
          countEllipses, lowerEllipses, lowerEllipsesFp, omitEllipses, omitEllipsesFp,
          _( "Ellipse graphics are omitted instead of approximated." ) },
        { VER_EXTRUDED_BODY, DOWNGRADE_BUCKET::DROP, _( "Extruded 3D bodies" ),
          _( "No equivalent in the target. The 3D body is left out." ), countExtrudedBodies, dropExtrudedBodies,
          dropExtrudedBodiesFp },
        { VER_THIEVING, DOWNGRADE_BUCKET::BLOCK, _( "Copper-thieving zones" ),
          _( "A refill in the target would turn the pattern into solid copper." ), countThievingZones, nullptr },
        { VER_PAD_SIM, DOWNGRADE_BUCKET::DROP, _( "Pad simulation types" ),
          _( "The simulation electrical type is left out." ), countPadSimTypes, dropPadSimTypes, dropPadSimTypesFp },
        { VER_FP_UNITS, DOWNGRADE_BUCKET::DROP, _( "Footprint unit metadata" ),
          _( "Pin-to-unit assignments used for gate swapping are removed." ), countUnitInfo, dropUnitInfo,
          dropUnitInfoFp },
        { VER_BARCODE, DOWNGRADE_BUCKET::LOWER, _( "Barcodes" ),
          _( "Converted to the printed polygons. No longer editable as a barcode." ), countBarcodes, lowerBarcodes,
          lowerBarcodesFp, omitBarcodes, omitBarcodesFp,
          _( "Barcodes are omitted instead of converted to polygons." ) },
        { FIRST_PCB_VARIANTS, DOWNGRADE_BUCKET::DROP, _( "PCB variants" ),
          _( "Variant registry and per-footprint overrides are removed." ), countPcbVariants, dropPcbVariants,
          dropPcbVariantsFp },
        { VER_JUMPER_PADS, DOWNGRADE_BUCKET::DROP, _( "Jumper pad groups" ), _( "Jumper pad groupings are removed." ),
          countJumperPads, dropJumperPads, dropJumperPadsFp },
        { VER_BACKDRILL, DOWNGRADE_BUCKET::BLOCK, _( "Backdrill / tertiary drill" ),
          _( "Backdrilling changes copper connectivity and cannot be represented." ), countBackdrills, nullptr },
        { VER_POST_MACHINING, DOWNGRADE_BUCKET::DROP, _( "Counterbore / countersink" ),
          _( "Drill post-machining is removed." ), countPostMachining, dropPostMachining, dropPostMachiningFp },
        { VER_PRESSFIT, DOWNGRADE_BUCKET::DROP, _( "Press-fit pads" ),
          _( "The press-fit fabrication property is removed." ), countPressFitPads, dropPressFitPads,
          dropPressFitPadsFp },
        { VER_SKIP_VIA,
          DOWNGRADE_BUCKET::LOWER,
          _( "Skip vias" ),
          _( "Approximated by keeping only the start and end layers." ),
          countSkipVias,
          lowerSkipVias,
          {},
          {},
          {},
          {},
          true },
        { VER_VIA_SPLIT,
          DOWNGRADE_BUCKET::LOWER,
          _( "Buried vias" ),
          _( "Written with the older combined blind type." ),
          countBuriedVias,
          lowerBuriedVias,
          {},
          {},
          {},
          {},
          true },
        { VER_ZONE_DEFAULTS, DOWNGRADE_BUCKET::DROP, _( "Zone layer defaults" ),
          _( "Per-layer zone defaults are removed." ), countZoneDefaults, dropZoneDefaults },
        { VER_PAD_DIE_DELAY, DOWNGRADE_BUCKET::DROP, _( "Pad-to-die delays" ), _( "Pad-to-die delay is removed." ),
          countDieDelays, dropDieDelays, dropDieDelaysFp },
        { VER_SHAPE_HATCH, DOWNGRADE_BUCKET::LOWER, _( "Hatched shape fills" ), _( "Converted to a solid fill." ),
          []( const BOARD* b )
          {
              return countHatchedFills( b, isHatchedInkShape );
          },
          []( BOARD* b )
          {
              setHatchedFills( b, isHatchedInkShape, FILL_T::FILLED_SHAPE );
          },
          []( FOOTPRINT* f )
          {
              setHatchedFillsFp( f, isHatchedInkShape, FILL_T::FILLED_SHAPE );
          },
          []( BOARD* b )
          {
              setHatchedFills( b, isHatchedInkShape, FILL_T::NO_FILL );
          },
          []( FOOTPRINT* f )
          {
              setHatchedFillsFp( f, isHatchedInkShape, FILL_T::NO_FILL );
          },
          _( "The hatch fill is omitted. The supported outline is kept." ) },
        { VER_SHAPE_HATCH,
          DOWNGRADE_BUCKET::LOWER,
          _( "Hatched copper fills" ),
          _( "Converted to a solid fill. Copper geometry may change." ),
          []( const BOARD* b )
          {
              return countHatchedFills( b, isHatchedCopperShape );
          },
          []( BOARD* b )
          {
              setHatchedFills( b, isHatchedCopperShape, FILL_T::FILLED_SHAPE );
          },
          []( FOOTPRINT* f )
          {
              setHatchedFillsFp( f, isHatchedCopperShape, FILL_T::FILLED_SHAPE );
          },
          {},
          {},
          {},
          true },
        { VER_PCB_POINTS, DOWNGRADE_BUCKET::DROP, _( "PCB points" ),
          _( "No equivalent in the target. Points are removed." ), countPcbPoints, dropPcbPoints, dropPcbPointsFp },
        { VER_VIA_PROTECTION, DOWNGRADE_BUCKET::DROP, _( "Via protection" ),
          _( "IPC-4761 covering, plugging, capping, and filling are removed." ), countViaProtection,
          dropViaProtection },
        { VER_DIELECTRIC, DOWNGRADE_BUCKET::DROP, _( "Dielectric frequency models" ),
          _( "The frequency-dependent dielectric model is removed." ), countDielectricModels, dropDielectricModels },
        { VER_GROUP_LIB_ID, DOWNGRADE_BUCKET::DROP, _( "Group design block links" ),
          _( "The link from a group to its design block is removed." ), countGroupDesignBlocks, dropGroupDesignBlocks,
          dropGroupDesignBlocksFp },
        { VER_TEXTBOX_KNOCKOUT, DOWNGRADE_BUCKET::LOWER, _( "Knockout text boxes" ), _( "Drawn as plain text boxes." ),
          countKnockoutTextBoxes, dropKnockoutTextBoxes, dropKnockoutTextBoxesFp, omitKnockoutTextBoxes,
          omitKnockoutTextBoxesFp, _( "Knockout text boxes are omitted instead of drawn as plain text." ) },
        { VER_ROUNDED_RECT, DOWNGRADE_BUCKET::LOWER, _( "Rounded rectangles" ), _( "Approximated by a polygon." ),
          countRoundedRects, lowerRoundedRects, lowerRoundedRectsFp, omitRoundedRects, omitRoundedRectsFp,
          _( "Rounded rectangles and rounded custom pad primitives are omitted. Copper geometry may change." ) },
        { VER_CELL_KNOCKOUT, DOWNGRADE_BUCKET::LOWER, _( "Knockout table cells" ), _( "Drawn as plain cells." ),
          countKnockoutTableCells, dropKnockoutTableCells, dropKnockoutTableCellsFp, omitKnockoutTableCells,
          omitKnockoutTableCellsFp,
          _( "Knockout cell contents are omitted; empty cells and the table grid are kept." ) },
        { VER_ZONE_DEFAULTS, DOWNGRADE_BUCKET::DROP, _( "Per-zone layer properties" ),
          _( "The per-layer overrides are removed." ), countZoneLayerProperties, dropZoneLayerProperties,
          dropZoneLayerPropertiesFp },
        { VER_USER_LAYERS, DOWNGRADE_BUCKET::BLOCK, _( "User layers past User_9" ),
          _( "The target supports only 9 user layers." ), countHighUserLayers, nullptr },
        { FIRST_FP_CUSTOM_STACKUP, DOWNGRADE_BUCKET::BLOCK, _( "Custom footprint stackups" ),
          _( "The target would expand the footprint's inner layers and change its connectivity." ),
          countCustomFpStackups, nullptr },
        { FIRST_FP_AFFINE_TRANSFORM, DOWNGRADE_BUCKET::BLOCK, _( "Scaled footprints" ),
          _( "The target cannot represent a scaled footprint." ), countScaledFootprints, nullptr },
    };
}


COMPATIBILITY_REPORT ClassifyBoardForDowngrade( const BOARD* aBoard, const DOWNGRADE_TARGET& aTarget,
                                                bool aDropInsteadOfApproximate )
{
    // The rules were written against a specific format. Warn developers when the format has
    // moved on, since a frozen writer silently omits features that have no rule yet.
    wxASSERT_MSG( SEXPR_BOARD_FILE_VERSION == BOARD_DOWNGRADE_COVERED,
                  wxT( "Board format changed. Review the downgrade rules and denylist, then update "
                       "BOARD_DOWNGRADE_COVERED." ) );

    COMPATIBILITY_REPORT report;

    if( SEXPR_BOARD_FILE_VERSION != BOARD_DOWNGRADE_COVERED )
        report.Add( DOWNGRADE_BUCKET::BLOCK, _( "Unreviewed board format" ),
                    _( "The current board format has changed since the downgrade rules were reviewed." ), 1 );

    if( aBoard->GetPlotOptions().GetFormat() == PLOT_FORMAT::PNG )
        report.Add( DOWNGRADE_BUCKET::DROP, _( "PNG plot selection" ),
                    _( "The unsupported plot format is reset to Gerber." ), 1 );

    if( aTarget.m_boardVersion < VER_PROPERTY_VARIABLES )
    {
        const TEXT_DOWNGRADE_PLAN text = planBoardText( aBoard, aTarget.m_boardVersion );

        if( text.m_blocked )
            report.Add( DOWNGRADE_BUCKET::BLOCK, _( "Unresolved post-target text" ),
                        _( "Affected property variables or expressions could not be safely resolved." ),
                        text.m_blocked );

        if( !text.m_replacements.empty() || !text.m_titleReplacements.empty() )
            report.Add( DOWNGRADE_BUCKET::DROP, _( "Post-target text variables" ),
                        _( "Affected displayed text is resolved to its current value and loses dynamic evaluation." ),
                        text.m_replacements.size() + text.m_titleReplacements.size() );
    }

    if( aTarget.m_boardVersion < VER_LINE_ENDINGS && !aDropInsteadOfApproximate )
    {
        if( int unsafe = countLineEndings( aBoard, true ); unsafe > 0 )
            report.Add( DOWNGRADE_BUCKET::BLOCK, _( "Styled graphic line endings" ),
                        _( "Dashed, colored, or unresolved-width line endings cannot be faithfully converted. "
                           "The omission policy can remove just the endpoint embellishments." ),
                        unsafe );
    }

    for( const BOARD_RULE& rule : boardRules() )
    {
        if( aTarget.m_boardVersion >= rule.m_introducedIn )
            continue;

        if( int count = rule.m_count( aBoard ); count > 0 )
        {
            bool omit = aDropInsteadOfApproximate && rule.m_bucket == DOWNGRADE_BUCKET::LOWER
                        && !rule.m_keepUnderOmitPolicy;
            report.Add( omit ? DOWNGRADE_BUCKET::DROP : rule.m_bucket, rule.m_feature,
                        omit ? rule.m_omitDetail : rule.m_detail, count );
        }
    }

    return report;
}


// The stored scale compensates for the truncated pixels/cm PPI that targets before 20260623
// compute, so it must be rewound. The catalog keeps images with a silent correction, so this
// is not a table rule and adds no report row.
static void compensateImageScale( PCB_REFERENCE_IMAGE* aImage )
{
    REFERENCE_IMAGE& ref = aImage->GetReferenceImage();
    int              legacyPPI = ref.GetImage().GetLegacyPPI();

    if( legacyPPI > 0 && ref.GetImage().GetPPI() != legacyPPI )
        ref.SetImageScale( ref.GetImageScale() * legacyPPI / ref.GetImage().GetPPI() );
}


static void compensateReferenceImageScalesFp( FOOTPRINT* aFootprint )
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_REFERENCE_IMAGE_T )
            compensateImageScale( static_cast<PCB_REFERENCE_IMAGE*>( item ) );
    }
}


static void compensateReferenceImageScales( BOARD* aBoard )
{
    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( item->Type() == PCB_REFERENCE_IMAGE_T )
            compensateImageScale( static_cast<PCB_REFERENCE_IMAGE*>( item ) );
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
        compensateReferenceImageScalesFp( fp );
}


void DowngradeBoardInPlace( BOARD* aBoard, const DOWNGRADE_TARGET& aTarget, bool aDropInsteadOfApproximate )
{
    if( aBoard->GetPlotOptions().GetFormat() == PLOT_FORMAT::PNG )
    {
        PCB_PLOT_PARAMS options = aBoard->GetPlotOptions();
        options.SetFormat( PLOT_FORMAT::GERBER );
        aBoard->SetPlotOptions( options );
    }

    if( aTarget.m_boardVersion < VER_PROPERTY_VARIABLES )
        applyTextPlan( planBoardText( aBoard, aTarget.m_boardVersion ) );

    if( aTarget.m_boardVersion < VER_IMAGE_PPI )
        compensateReferenceImageScales( aBoard );

    for( const BOARD_RULE& rule : boardRules() )
    {
        if( aTarget.m_boardVersion >= rule.m_introducedIn )
            continue;

        bool        omit = aDropInsteadOfApproximate && rule.m_bucket == DOWNGRADE_BUCKET::LOWER
                           && !rule.m_keepUnderOmitPolicy;
        const auto& apply = omit ? rule.m_omit : rule.m_apply;
        wxASSERT_MSG( !omit || apply, wxT( "A LOWER rule needs an omission transform." ) );

        if( apply )
            apply( aBoard );
    }
}


// A library footprint carries the same post-target features as one on a board, but it lives in its
// own .kicad_mod, so the board rules never see it. This applies the footprint-scoped subset.
void DowngradeFootprintInPlace( FOOTPRINT* aFootprint, const DOWNGRADE_TARGET& aTarget, bool aDropInsteadOfApproximate )
{
    if( aTarget.m_boardVersion < VER_PROPERTY_VARIABLES )
    {
        TEXT_DOWNGRADE_PLAN text;
        visitFootprintItems( aFootprint,
                             [&]( BOARD_ITEM* item )
                             {
                                 planTextItem( item, aTarget.m_boardVersion, text );
                             } );
        applyTextPlan( text );
    }

    if( aTarget.m_boardVersion < VER_IMAGE_PPI )
        compensateReferenceImageScalesFp( aFootprint );

    for( const BOARD_RULE& rule : boardRules() )
    {
        if( aTarget.m_boardVersion >= rule.m_introducedIn )
            continue;

        bool        omit = aDropInsteadOfApproximate && rule.m_bucket == DOWNGRADE_BUCKET::LOWER
                           && !rule.m_keepUnderOmitPolicy;
        const auto& apply = omit ? rule.m_omitFp : rule.m_applyFp;
        wxASSERT_MSG( !omit || !rule.m_applyFp || apply, wxT( "A footprint LOWER rule needs an omission transform." ) );

        if( apply )
            apply( aFootprint );
    }
}


// Node heads dated by the format that introduced them. The versioned writers cannot emit these,
// so this gate is a safety net against a wrong or miswired writer. To add a token, add a row
// with its format version.
static constexpr DATED_TOKEN BOARD_TOKENS[] = {
    { "constraint", VER_CONSTRAINTS },
    { "grid_item", VER_GRIDS },
    { "start_shape", VER_LINE_ENDINGS },
    { "end_shape", VER_LINE_ENDINGS },
    { "custom_property", VER_CUSTOM_PROPERTIES },
    { "exclude_from_sim", VER_SIM_EXCLUSION },
    { "drill_chart", VER_DRILL_DRAWINGS },
    { "drill_map", VER_DRILL_DRAWINGS },
    { "drill_symbol_profile", VER_DRILL_DRAWINGS },
    { "templates", VER_VIA_STITCH },
    { "hatch_position", VER_ZONE_DEFAULTS },
    { "covering", VER_VIA_PROTECTION },
    { "plugging", VER_VIA_PROTECTION },
    { "capping", VER_VIA_PROTECTION },
    { "filling", VER_VIA_PROTECTION },
    { "depth", VER_VIA_PROTECTION },
    { "zone_defaults", VER_ZONE_DEFAULTS },
    { "jumper_pad_groups", VER_JUMPER_PADS },
    { "lib_id", VER_GROUP_LIB_ID },
    { "start_end_only", VER_SKIP_VIA },
    { "point", VER_PCB_POINTS },
    { "unit", VER_FP_UNITS },
    { "pins", VER_FP_UNITS },
    { "barcode", VER_BARCODE },
    { "ecc_level", VER_BARCODE },
    { "text", VER_BARCODE },
    { "text_height", VER_BARCODE },
    { "die_delay", VER_PAD_DIE_DELAY },
    { "backdrill", VER_BACKDRILL },
    { "tertiary_drill", VER_BACKDRILL },
    { "front_post_machining", VER_POST_MACHINING },
    { "back_post_machining", VER_POST_MACHINING },
    { "variant", FIRST_PCB_VARIANTS },
    { "variants", FIRST_PCB_VARIANTS },
    { "field", FIRST_PCB_VARIANTS },
    { "description", FIRST_PCB_VARIANTS },
    { "body_pcb_gap", VER_EXTRUDED_BODY },
    { "overall_height", VER_EXTRUDED_BODY },
    { "gr_ellipse", VER_ELLIPSE },
    { "gr_ellipse_arc", VER_ELLIPSE },
    { "fp_ellipse", VER_ELLIPSE },
    { "fp_ellipse_arc", VER_ELLIPSE },
    { "dielectric_model", VER_DIELECTRIC },
    { "spec_frequency", VER_DIELECTRIC },
    { "constant", VER_DIELECTRIC },
    { "net_chain", VER_NET_CHAINS },
    { "net_chains", VER_NET_CHAINS },
    { "terminal_pad", VER_NET_CHAINS },
    { "thieving", VER_THIEVING },
    { "rotation_angle", VER_ELLIPSE },
    { "stagger", VER_THIEVING },
    { "sim_electrical_type", VER_PAD_SIM },
    { "transform", FIRST_FP_AFFINE_TRANSFORM },
};

// Bareword values the target cannot parse. The node scan cannot see these.
// Hatch fill values cannot be listed here, unlike the schematic gate. Zones legitimately emit
// hatch tokens in every target format, so a hatched shape fill leak is invisible to this gate.
static constexpr DATED_TOKEN BOARD_VALUES[] = {
    { "exclude_from_sim", VER_SIM_EXCLUSION }, { "via_stitch", VER_VIA_STITCH }, { "via_stack", VER_VIA_STACK },
    { "pad_prop_pressfit", VER_PRESSFIT },     { "buried", VER_VIA_SPLIT },
};


int BoardDowngradeCoveredVersion()
{
    return BOARD_DOWNGRADE_COVERED;
}


wxString FindUnsupportedBoardToken( const wxString& aSerialized, const DOWNGRADE_TARGET& aTarget )
{
    if( aTarget.m_boardVersion >= SEXPR_BOARD_FILE_VERSION )
        return wxEmptyString;

    return FindUnsupportedToken( aSerialized, BOARD_TOKENS, std::size( BOARD_TOKENS ), BOARD_VALUES,
                                 std::size( BOARD_VALUES ), aTarget.m_boardVersion );
}


void FlattenBoardVariant( BOARD* aBoard, const wxString& aVariantName )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        fp->SetDNP( fp->GetDNPForVariant( aVariantName ) );
        fp->SetExcludedFromBOM( fp->GetExcludedFromBOMForVariant( aVariantName ) );
        fp->SetExcludedFromPosFiles( fp->GetExcludedFromPosFilesForVariant( aVariantName ) );
        fp->SetExcludedFromSim( fp->GetExcludedFromSimForVariant( aVariantName ) );

        if( const FOOTPRINT_VARIANT* variant = fp->GetVariant( aVariantName ) )
        {
            for( const auto& [name, value] : variant->GetFields() )
            {
                if( PCB_FIELD* field = fp->GetField( name ) )
                    field->SetText( value );
            }
        }
    }
}

