/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#include <diff_merge/pcb_diff_canvas_context.h>
#include <diff_merge/pcb_differ.h>

#include <board.h>
#include <board_item.h>
#include <eda_item.h>
#include <footprint.h>
#include <gal/painter.h>
#include <layer_ids.h>
#include <pad.h>
#include <pcb_display_options.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_track.h>
#include <view/view.h>
#include <widgets/widget_diff_canvas.h>
#include <zone.h>

#include <memory>
#include <set>


namespace
{

class PCB_DIFF_PAINTER : public KIGFX::PCB_PAINTER
{
public:
    PCB_DIFF_PAINTER( KIGFX::GAL* aGal, std::map<KIID, KIGFX::COLOR4D> aOverrides ) :
            KIGFX::PCB_PAINTER( aGal, FRAME_PCB_EDITOR ),
            m_overrides( std::move( aOverrides ) ),
            m_dimmed( std::make_shared<std::set<KIID>>() )
    {
    }

    /// Diff IDs of items the canvas has muted. Shared with the dimmer callback so
    /// hiding a change drops its category color back to neutral grey.
    std::shared_ptr<std::set<KIID>> DimmedSet() const { return m_dimmed; }

    bool Draw( const KIGFX::VIEW_ITEM* aItem, int aLayer ) override
    {
        const EDA_ITEM* edaItem = dynamic_cast<const EDA_ITEM*>( aItem );

        if( edaItem )
        {
            KIID key = KICAD_DIFF::PcbDiffItemId( *edaItem );

            if( m_overrides.count( key ) == 0 )
            {
                if( const BOARD_ITEM* boardItem = dynamic_cast<const BOARD_ITEM*>( aItem ) )
                {
                    if( const FOOTPRINT* fp = boardItem->GetParentFootprint() )
                        key = fp->m_Uuid;
                }
            }

            auto it = m_overrides.find( key );

            if( it != m_overrides.end() && m_dimmed->count( key ) == 0 )
            {
                // PCB_PAINTER tints normal items via the layer color, not via
                // LAYER_BRIGHTENED. Swap the colors of the layer we are drawing on
                // and the layer it takes its color from so the override paints the item.
                KIGFX::PCB_RENDER_SETTINGS* settings = GetSettings();
                std::vector<std::pair<int, KIGFX::COLOR4D>> saved;

                for( int layer : colorSourceLayers( aLayer ) )
                {
                    saved.emplace_back( layer, settings->GetLayerColor( layer ) );
                    settings->SetLayerColor( layer, it->second );
                }

                bool result = KIGFX::PCB_PAINTER::Draw( aItem, aLayer );

                for( const auto& [layer, color] : saved )
                    settings->SetLayerColor( layer, color );

                return result;
            }
        }

        return KIGFX::PCB_PAINTER::Draw( aItem, aLayer );
    }

private:
    /// The drawn layer plus the one PCB_RENDER_SETTINGS::GetColor takes its colour from.
    static std::vector<int> colorSourceLayers( int aLayer )
    {
        // Numbers, netnames and drill holes sit on their item, so in its colour they vanish
        if( IsNetnameLayer( aLayer ) || aLayer == LAYER_PAD_PLATEDHOLES || aLayer == LAYER_NON_PLATEDHOLES
            || aLayer == LAYER_VIA_HOLES )
        {
            return {};
        }

        if( IsPadCopperLayer( aLayer ) )
            return { aLayer, aLayer - LAYER_PAD_COPPER_START, LAYER_VIA_HOLES };

        if( IsViaCopperLayer( aLayer ) )
            return { aLayer, aLayer - LAYER_VIA_COPPER_START };

        if( IsClearanceLayer( aLayer ) )
            return { aLayer, aLayer - LAYER_CLEARANCE_START };

        if( IsZoneFillLayer( aLayer ) )
            return { aLayer, aLayer - LAYER_ZONE_START };

        if( aLayer == LAYER_PAD_HOLEWALLS )
            return { aLayer, LAYER_VIA_HOLES };

        return { aLayer };
    }

    std::map<KIID, KIGFX::COLOR4D>  m_overrides;
    std::shared_ptr<std::set<KIID>> m_dimmed;
};

} // namespace


namespace KICAD_DIFF
{

std::vector<KIGFX::VIEW_ITEM*> CollectFootprintDiffContextItems( FOOTPRINT& aFootprint )
{
    std::vector<KIGFX::VIEW_ITEM*> items;

    items.push_back( &aFootprint );

    for( PCB_FIELD* field : aFootprint.GetFields() )
        items.push_back( field );

    for( BOARD_ITEM* item : aFootprint.GraphicalItems() )
        items.push_back( item );

    for( PAD* pad : aFootprint.Pads() )
        items.push_back( pad );

    for( ZONE* zone : aFootprint.Zones() )
        items.push_back( zone );

    return items;
}


std::vector<KIGFX::VIEW_ITEM*> CollectBoardDiffContextItems( BOARD& aBoard )
{
    std::vector<KIGFX::VIEW_ITEM*> items;

    for( PCB_TRACK* track : aBoard.Tracks() )
        items.push_back( track );

    for( ZONE* zone : aBoard.Zones() )
        items.push_back( zone );

    for( BOARD_ITEM* item : aBoard.Drawings() )
        items.push_back( item );

    for( FOOTPRINT* fp : aBoard.Footprints() )
    {
        std::vector<KIGFX::VIEW_ITEM*> fpItems = CollectFootprintDiffContextItems( *fp );
        items.insert( items.end(), fpItems.begin(), fpItems.end() );
    }

    return items;
}


void ConfigurePcbDiffContextRenderSettings( KIGFX::PCB_RENDER_SETTINGS& aSettings,
                                            const KIGFX::COLOR4D& aColor )
{
    const KIGFX::COLOR4D gray( 0.45, 0.45, 0.45, 1.0 );
    const KIGFX::COLOR4D background( 0.97, 0.97, 0.97, 1.0 );

    aSettings.SetAllLayerColors( gray );
    aSettings.SetLayerColor( LAYER_BRIGHTENED, aColor );
    aSettings.SetLayerColor( LAYER_PCB_BACKGROUND, background );
    aSettings.SetLayerColor( LAYER_PAD_PLATEDHOLES, background );

    // Labels sit on grey or diff coloured copper and on light drills, where only dark text reads
    for( int layer = NETNAMES_LAYER_ID_START; layer < NETNAMES_LAYER_ID_END; ++layer )
        aSettings.SetLayerColor( layer, KIGFX::COLOR4D::BLACK );
    aSettings.SetBackgroundColor( background );

    PCB_DISPLAY_OPTIONS displayOptions;
    displayOptions.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;
    displayOptions.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_FILLED;
    displayOptions.m_TrackOpacity = 1.0;
    displayOptions.m_ViaOpacity = 1.0;
    displayOptions.m_PadOpacity = 1.0;
    // Zones fill translucent so footprints and traces under a copper pour
    // stay visible in the compare view.
    displayOptions.m_ZoneOpacity = 0.5;
    displayOptions.m_ImageOpacity = 1.0;
    displayOptions.m_FilledShapeOpacity = 1.0;
    aSettings.LoadDisplayOptions( displayOptions );
    aSettings.SetHighContrast( false );
    aSettings.SetHighlight( false );
}


std::unique_ptr<KIGFX::PAINTER> MakePcbDiffContextPainter( KIGFX::GAL* aGal, const KIGFX::COLOR4D& aColor,
                                                           std::map<KIID, KIGFX::COLOR4D> aOverrides )
{
    auto painter = std::make_unique<PCB_DIFF_PAINTER>( aGal, std::move( aOverrides ) );
    ConfigurePcbDiffContextRenderSettings( *painter->GetSettings(), aColor );

    return painter;
}


void ConfigurePcbDiffCanvasContext( WIDGET_DIFF_CANVAS& aCanvas, BOARD* aReference, BOARD* aComparison,
                                    const KIGFX::COLOR4D& aColor, const std::map<KIID, KIGFX::COLOR4D>& aOverrides,
                                    const std::vector<KIGFX::VIEW_ITEM*>&       aExtraItems,
                                    const std::map<KIID, KICAD_DIFF::CATEGORY>& aCategories )
{
    auto painter = std::make_unique<PCB_DIFF_PAINTER>( aCanvas.GetGAL(), aOverrides );
    ConfigurePcbDiffContextRenderSettings( *painter->GetSettings(), aColor );

    std::shared_ptr<std::set<KIID>> dimmed = painter->DimmedSet();
    aCanvas.SetContextPainter( std::move( painter ) );
    aCanvas.SetItemIdGetter( PcbDiffItemId );

    aCanvas.SetItemDimmer(
            [dimmed]( KIGFX::VIEW_ITEM* aItem, bool aDim )
            {
                EDA_ITEM* eda = dynamic_cast<EDA_ITEM*>( aItem );

                if( !eda )
                    return;

                if( aDim )
                    dimmed->insert( PcbDiffItemId( *eda ) );
                else
                    dimmed->erase( PcbDiffItemId( *eda ) );
            } );

    std::vector<KIGFX::VIEW_ITEM*> items;

    if( aReference )
    {
        std::vector<KIGFX::VIEW_ITEM*> refItems = CollectBoardDiffContextItems( *aReference );
        items.insert( items.end(), refItems.begin(), refItems.end() );
    }

    if( aComparison )
    {
        std::vector<KIGFX::VIEW_ITEM*> compItems = CollectBoardDiffContextItems( *aComparison );
        items.insert( items.end(), compItems.begin(), compItems.end() );
    }

    items.insert( items.end(), aExtraItems.begin(), aExtraItems.end() );

    aCanvas.SetContextItems( items );

    ApplyPcbGalLayerOrder( aCanvas.GetView() );
    ApplyPcbGalLayerDeps( aCanvas.GetView() );

    std::map<KIGFX::VIEW_ITEM*, KICAD_DIFF::CATEGORY> itemCategories;

    for( KIGFX::VIEW_ITEM* viewItem : items )
    {
        EDA_ITEM* edaItem = dynamic_cast<EDA_ITEM*>( viewItem );

        if( !edaItem )
            continue;

        auto it = aCategories.find( PcbDiffItemId( *edaItem ) );

        if( it != aCategories.end() )
            itemCategories[viewItem] = it->second;
    }

    aCanvas.SetItemCategories( std::move( itemCategories ) );
}


void ConfigureFpDiffCanvasContext( WIDGET_DIFF_CANVAS& aCanvas, FOOTPRINT* aBefore, FOOTPRINT* aAfter,
                                   std::shared_ptr<const void> aOwner )
{
    if( !aBefore && !aAfter )
    {
        aCanvas.SetContextItems( {} );
        return;
    }

    const DIFF_COLOR_THEME         theme;
    std::map<KIID, KIGFX::COLOR4D> overrides;
    std::vector<KIGFX::VIEW_ITEM*> items;

    if( !aBefore || !aAfter )
    {
        // A whole footprint added or removed takes its colour from the footprint
        FOOTPRINT* shown = aAfter ? aAfter : aBefore;
        overrides[shown->m_Uuid] = aAfter ? theme.added : theme.removed;
        items = CollectFootprintDiffContextItems( *shown );
    }
    else
    {
        items = CollectFootprintDiffContextItems( *aAfter );

        for( const LIB_ELEMENT& element : DiffFootprintElements( aBefore, aAfter ) )
        {
            overrides[PcbDiffItemId( *element.item )] = theme.ForKind( element.kind );

            // Removed elements exist only on the before side
            if( element.kind == CHANGE_KIND::REMOVED )
                items.push_back( const_cast<EDA_ITEM*>( element.item ) );
        }
    }

    aCanvas.SetContextPainter( MakePcbDiffContextPainter( aCanvas.GetGAL(), theme.reference, std::move( overrides ) ) );
    aCanvas.SetContextItems( items, std::move( aOwner ) );
    ApplyPcbGalLayerOrder( aCanvas.GetView() );
    ApplyPcbGalLayerDeps( aCanvas.GetView() );
    aCanvas.ZoomToFit();
}


void ShowFootprintPair( WIDGET_DIFF_CANVAS& aCanvas, const FP_LIB_DIFFER::FOOTPRINT_MAP& aBefore,
                        const FP_LIB_DIFFER::FOOTPRINT_MAP& aAfter, const wxString& aName )
{
    // Each view takes over the items it draws, so it gets copies parented to a board of their own
    auto holder = std::make_shared<BOARD>();
    holder->SetBoardUse( BOARD_USE::FPHOLDER );

    // Pads only draw on layers their board enables, and a library footprint may use any of them
    holder->SetEnabledLayers( LSET::AllLayersMask() );

    auto copy = [&]( const FP_LIB_DIFFER::FOOTPRINT_MAP& aSide ) -> FOOTPRINT*
    {
        auto footprint = aSide.find( aName );

        if( footprint == aSide.end() || !footprint->second )
            return nullptr;

        FOOTPRINT* clone = static_cast<FOOTPRINT*>( footprint->second->Clone() );
        holder->Add( clone, ADD_MODE::APPEND, true );
        return clone;
    };

    FOOTPRINT* before = copy( aBefore );
    FOOTPRINT* after = copy( aAfter );

    ConfigureFpDiffCanvasContext( aCanvas, before, after, holder );
}

} // namespace KICAD_DIFF
