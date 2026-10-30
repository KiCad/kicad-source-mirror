/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <diff_merge/sym_diff_canvas_context.h>
#include <diff_merge/sch_diff_canvas_context.h>
#include <diff_merge/sym_item_diff.h>
#include <diff_merge/diff_scene.h>
#include <dialogs/dialog_kicad_diff.h>

#include <gal/painter.h>
#include <kiid.h>
#include <lib_symbol.h>
#include <sch_item.h>
#include <sch_view.h>
#include <widgets/widget_diff_canvas.h>

#include <map>


namespace KICAD_DIFF
{

void ConfigureSymDiffCanvasContext( WIDGET_DIFF_CANVAS& aCanvas, LIB_SYMBOL* aBefore, LIB_SYMBOL* aAfter )
{
    if( !aBefore && !aAfter )
    {
        aCanvas.SetContextItems( {} );
        return;
    }

    const DIFF_COLOR_THEME theme;

    auto colorFor = [&]( CHANGE_KIND aKind ) -> KIGFX::COLOR4D
    {
        switch( aKind )
        {
        case CHANGE_KIND::ADDED: return theme.added;
        case CHANGE_KIND::REMOVED: return theme.removed;
        default: return theme.modified;
        }
    };

    std::vector<SYM_ELEMENT>       elements = DiffSymbolElements( aBefore, aAfter );
    std::map<KIID, KIGFX::COLOR4D> overrides;

    for( const SYM_ELEMENT& element : elements )
    {
        overrides[element.item->m_Uuid] = colorFor( element.kind );
        const_cast<EDA_ITEM*>( element.item )->SetBrightened();
    }

    std::vector<KIGFX::VIEW_ITEM*> items;

    if( aAfter )
    {
        for( SCH_ITEM& item : aAfter->GetDrawItems() )
            items.push_back( &item );
    }

    for( const SYM_ELEMENT& element : elements )
    {
        if( element.kind == CHANGE_KIND::REMOVED )
            items.push_back( const_cast<EDA_ITEM*>( element.item ) );
    }

    aCanvas.SetWorldUnitLength( SCH_WORLD_UNIT );
    aCanvas.SetContextPainter( MakeSchDiffContextPainter( aCanvas.GetGAL(), nullptr, theme.reference, overrides ) );
    aCanvas.SetContextItems( items );
    aCanvas.ZoomToFit();
}


void ShowSymbolPair( WIDGET_DIFF_CANVAS& aCanvas, const SYM_LIB_DIFFER::SYMBOL_MAP& aBefore,
                     const SYM_LIB_DIFFER::SYMBOL_MAP& aAfter, const wxString& aName,
                     std::vector<std::unique_ptr<LIB_SYMBOL>>& aShown )
{
    auto flatten = [&]( const SYM_LIB_DIFFER::SYMBOL_MAP& aSide ) -> std::unique_ptr<LIB_SYMBOL>
    {
        auto symbol = aSide.find( aName );
        return symbol != aSide.end() ? symbol->second->Flatten() : nullptr;
    };

    std::unique_ptr<LIB_SYMBOL> before = flatten( aBefore );
    std::unique_ptr<LIB_SYMBOL> after = flatten( aAfter );

    ConfigureSymDiffCanvasContext( aCanvas, before.get(), after.get() );

    // Released only after the canvas has let go of the previous pair
    aShown.clear();
    aShown.push_back( std::move( before ) );
    aShown.push_back( std::move( after ) );
}


void ShowSymbolChanges( DIALOG_KICAD_DIFF& aDialog, const DOCUMENT_DIFF& aDiff,
                        const SYM_LIB_DIFFER::SYMBOL_MAP& aBefore, const SYM_LIB_DIFFER::SYMBOL_MAP& aAfter )
{
    std::map<KIID_PATH, wxString> names;

    for( const ITEM_CHANGE& change : aDiff.changes )
    {
        if( change.refdes )
            names[change.id] = *change.refdes;
    }

    auto shown = std::make_shared<std::vector<std::unique_ptr<LIB_SYMBOL>>>();

    aDialog.SetChangeSelectedHandler(
            [&aDialog, &aBefore, &aAfter, names, shown]( const KIID_PATH& aId )
            {
                WIDGET_DIFF_CANVAS* canvas = aDialog.DiffCanvas();

                if( !canvas )
                    return;

                auto name = names.find( aId );

                ShowSymbolPair( *canvas, aBefore, aAfter, name != names.end() ? name->second : wxString(),
                                *shown );
            } );

    aDialog.SelectFirstChange();
}

} // namespace KICAD_DIFF
