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

#include <diff_merge/sch_diff_canvas_context.h>

#include <eda_item.h>
#include <layer_ids.h>
#include <sch_field.h>
#include <sch_item.h>
#include <sch_painter.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <set>
#include <widgets/widget_diff_canvas.h>


namespace
{

class SCH_DIFF_PAINTER : public KIGFX::SCH_PAINTER
{
public:
    SCH_DIFF_PAINTER( KIGFX::GAL* aGal, std::map<KIID, KIGFX::COLOR4D> aOverrides ) :
            KIGFX::SCH_PAINTER( aGal ),
            m_overrides( std::move( aOverrides ) )
    {
    }

    bool Draw( const KIGFX::VIEW_ITEM* aItem, int aLayer ) override
    {
        const EDA_ITEM* edaItem = dynamic_cast<const EDA_ITEM*>( aItem );

        if( edaItem )
        {
            auto it = m_overrides.find( edaItem->m_Uuid );

            if( it != m_overrides.end() )
            {
                SCH_RENDER_SETTINGS* settings = GetSettings();
                KIGFX::COLOR4D       saved = settings->GetLayerColor( LAYER_BRIGHTENED );
                settings->SetLayerColor( LAYER_BRIGHTENED, it->second );

                bool result = KIGFX::SCH_PAINTER::Draw( aItem, aLayer );

                settings->SetLayerColor( LAYER_BRIGHTENED, saved );
                return result;
            }
        }

        return KIGFX::SCH_PAINTER::Draw( aItem, aLayer );
    }

private:
    std::map<KIID, KIGFX::COLOR4D> m_overrides;
};

} // namespace


namespace KICAD_DIFF
{

std::vector<KIGFX::VIEW_ITEM*> CollectSchematicDiffContextItems( SCHEMATIC& aSchematic, SCH_SCREEN* aScreen )
{
    std::vector<KIGFX::VIEW_ITEM*> items;
    SCH_SCREEN*                    screen = aScreen ? aScreen : aSchematic.RootScreen();

    if( !screen )
        return items;

    for( SCH_ITEM* item : screen->Items() )
        items.push_back( item );

    return items;
}


void ConfigureSchDiffContextRenderSettings( SCH_RENDER_SETTINGS& aSettings,
                                            const KIGFX::COLOR4D& aColor )
{
    const KIGFX::COLOR4D gray( 0.5, 0.5, 0.5, 1.0 );

    for( int layer = 0; layer < LAYER_ID_COUNT; ++layer )
        aSettings.SetLayerColor( layer, gray );

    aSettings.SetLayerColor( LAYER_BRIGHTENED, aColor );

    aSettings.SetBackgroundColor( KIGFX::COLOR4D( 0.97, 0.97, 0.97, 1.0 ) );
    aSettings.SetHighContrast( false );
    aSettings.SetHighlight( false );
    aSettings.m_OverrideItemColors = true;
    aSettings.m_ShowPinsElectricalType = false;
    aSettings.m_ShowHiddenPins = true;
    aSettings.m_ShowHiddenFields = true;
    aSettings.m_ShowVisibleFields = true;
}


std::unique_ptr<KIGFX::PAINTER> MakeSchDiffContextPainter( KIGFX::GAL* aGal, SCHEMATIC* aSchematic,
                                                           const KIGFX::COLOR4D&          aColor,
                                                           std::map<KIID, KIGFX::COLOR4D> aOverrides )
{
    auto painter = std::make_unique<SCH_DIFF_PAINTER>( aGal, std::move( aOverrides ) );
    painter->SetSchematic( aSchematic );
    ConfigureSchDiffContextRenderSettings( *painter->GetSettings(), aColor );

    return painter;
}


void ConfigureSchDiffCanvasContext( WIDGET_DIFF_CANVAS& aCanvas, SCHEMATIC* aReference, SCHEMATIC* aComparison,
                                    const KIGFX::COLOR4D& aColor, const std::map<KIID, KIGFX::COLOR4D>& aOverrides,
                                    const std::vector<KIGFX::VIEW_ITEM*>&       aExtraItems,
                                    const std::map<KIID, KICAD_DIFF::CATEGORY>& aCategories,
                                    SCH_SCREEN* aReferenceScreen, SCH_SCREEN* aComparisonScreen )
{
    aCanvas.SetContextPainter(
            MakeSchDiffContextPainter( aCanvas.GetGAL(), aReference ? aReference : aComparison, aColor, aOverrides ) );

    std::vector<KIGFX::VIEW_ITEM*> items;

    if( aReference )
    {
        std::vector<KIGFX::VIEW_ITEM*> refItems = CollectSchematicDiffContextItems( *aReference, aReferenceScreen );
        items.insert( items.end(), refItems.begin(), refItems.end() );
    }

    if( aComparison )
    {
        std::vector<KIGFX::VIEW_ITEM*> compItems = CollectSchematicDiffContextItems( *aComparison, aComparisonScreen );
        items.insert( items.end(), compItems.begin(), compItems.end() );
    }

    items.insert( items.end(), aExtraItems.begin(), aExtraItems.end() );

    for( KIGFX::VIEW_ITEM* viewItem : items )
    {
        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( viewItem );

        if( !schItem )
            continue;

        const bool brighten = aOverrides.count( schItem->m_Uuid ) > 0;

        if( brighten )
            schItem->SetBrightened();
        else
            schItem->ClearBrightened();

        // Mirror brightening onto pins and fields. The painter checks each
        // child item's IsBrightened individually.
        if( SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( schItem ) )
        {
            for( SCH_PIN* pin : sym->GetPins() )
            {
                if( !pin )
                    continue;

                if( brighten )
                    pin->SetBrightened();
                else
                    pin->ClearBrightened();
            }

            for( SCH_FIELD& field : sym->GetFields() )
            {
                if( brighten )
                    field.SetBrightened();
                else
                    field.ClearBrightened();
            }
        }
    }

    aCanvas.SetContextItems( items );

    aCanvas.SetItemDimmer(
            [overrides = aOverrides]( KIGFX::VIEW_ITEM* aItem, bool aDim )
            {
                SCH_ITEM* sch = dynamic_cast<SCH_ITEM*>( aItem );

                if( !sch )
                    return;

                const bool inOverride = overrides.count( sch->m_Uuid ) > 0;
                const bool brighten = !aDim && inOverride;

                auto apply = [brighten]( EDA_ITEM* aEdaItem )
                {
                    if( brighten )
                        aEdaItem->SetBrightened();
                    else
                        aEdaItem->ClearBrightened();
                };

                apply( sch );

                if( SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( sch ) )
                {
                    for( SCH_PIN* pin : sym->GetPins() )
                    {
                        if( pin )
                            apply( pin );
                    }

                    for( SCH_FIELD& field : sym->GetFields() )
                        apply( &field );
                }
            } );

    std::map<KIGFX::VIEW_ITEM*, KICAD_DIFF::CATEGORY> itemCategories;

    for( KIGFX::VIEW_ITEM* viewItem : items )
    {
        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( viewItem );

        if( !schItem )
            continue;

        auto it = aCategories.find( schItem->m_Uuid );

        if( it != aCategories.end() )
            itemCategories[viewItem] = it->second;
    }

    aCanvas.SetItemCategories( std::move( itemCategories ) );
}

} // namespace KICAD_DIFF
