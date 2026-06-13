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
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <diff_merge/sch_geometry_extractor.h>
#include "sch_diff_utils.h"

#include <lib_symbol.h>
#include <schematic.h>
#include <sch_item.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>

#include <algorithm>
#include <set>


namespace KICAD_DIFF
{

namespace
{

    void addBBoxAsPolygon( const BOX2I& aBBox, const KIGFX::COLOR4D& aColor, bool aFilled, DOCUMENT_GEOMETRY& aOut )
    {
        DOCUMENT_POLYGON poly = MakeBBoxOutline( aBBox, aColor );
        poly.filled = aFilled;

        if( !poly.outline.empty() )
            aOut.polygons.push_back( std::move( poly ) );
    }


    void extractScreen( const SCHEMATIC* aSchematic, const SCH_SHEET_PATH& aPath, const SCH_SCREEN& aScreen,
                        const KIGFX::COLOR4D& aColor, const std::map<KIID, KIGFX::COLOR4D>& aOverrides,
                        bool aOnlyOverrides, DOCUMENT_GEOMETRY& aOut )
    {
        SHEET_SCOPE scope( aSchematic, &aPath );

        auto colorFor = [&]( const SCH_ITEM* aItem ) -> KIGFX::COLOR4D
        {
            auto it = aOverrides.find( aItem->m_Uuid );
            return it != aOverrides.end() ? it->second : aColor;
        };

        for( const SCH_ITEM* item : aScreen.Items() )
        {
            if( !item )
                continue;

            if( aOnlyOverrides && !aOverrides.count( item->m_Uuid ) )
                continue;

            const KIGFX::COLOR4D color = colorFor( item );

            switch( item->Type() )
            {
            case SCH_LINE_T:
            {
                const SCH_LINE& line = static_cast<const SCH_LINE&>( *item );

                DOCUMENT_SEGMENT seg;
                seg.start = line.GetStartPoint();
                seg.end = line.GetEndPoint();
                seg.width = line.GetLineWidth();
                seg.color = color;
                aOut.segments.push_back( seg );
                break;
            }

            case SCH_JUNCTION_T:
            {
                const SCH_JUNCTION& junc = static_cast<const SCH_JUNCTION&>( *item );

                DOCUMENT_CIRCLE c;
                c.center = junc.GetPosition();
                c.radius = std::max( 1, junc.GetDiameter() / 2 );
                c.filled = true;
                c.lineWidth = 0;
                c.color = color;
                aOut.circles.push_back( c );
                break;
            }

            case SCH_SHEET_T:
            case SCH_SYMBOL_T:
            case SCH_NO_CONNECT_T:
            case SCH_TEXT_T:
            case SCH_TEXTBOX_T:
            case SCH_TABLE_T:
            case SCH_BITMAP_T:
            case SCH_RULE_AREA_T:
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
            case SCH_DIRECTIVE_LABEL_T:
            case SCH_SHAPE_T:
            case SCH_BUS_WIRE_ENTRY_T:
            case SCH_BUS_BUS_ENTRY_T:
            {
                const bool overridden = aOverrides.count( item->m_Uuid ) > 0;
                addBBoxAsPolygon( item->GetBoundingBox(), color, overridden, aOut );
                break;
            }

            default: break;
            }
        }
    }

} // namespace


DOCUMENT_GEOMETRY ExtractSchematicGeometry( const SCHEMATIC& aSchematic, const KIGFX::COLOR4D& aColor,
                                            const std::map<KIID, KIGFX::COLOR4D>& aOverrides, bool aOnlyOverrides )
{
    DOCUMENT_GEOMETRY out;

    std::set<const SCH_SCREEN*> visited;

    for( const SCH_SHEET_PATH& sheet : aSchematic.Hierarchy() )
    {
        const SCH_SCREEN* screen = sheet.LastScreen();

        if( !screen || !visited.insert( screen ).second )
            continue;

        extractScreen( &aSchematic, sheet, *screen, aColor, aOverrides, aOnlyOverrides, out );
    }

    return out;
}


DOCUMENT_GEOMETRY ExtractSymbolGeometry( const LIB_SYMBOL& aSymbol, const KIGFX::COLOR4D& aColor, int aUnit,
                                         int aBodyStyle )
{
    DOCUMENT_GEOMETRY out;

    for( const SCH_ITEM& item : aSymbol.GetDrawItems() )
    {
        if( item.GetUnit() > 0 && aUnit > 0 && item.GetUnit() != aUnit )
            continue;

        if( item.GetBodyStyle() > 0 && aBodyStyle > 0 && item.GetBodyStyle() != aBodyStyle )
            continue;

        addBBoxAsPolygon( item.GetBoundingBox(), aColor, false, out );
    }

    if( out.Empty() )
        addBBoxAsPolygon( aSymbol.GetUnitBoundingBox( aUnit, aBodyStyle ), aColor, false, out );

    return out;
}

} // namespace KICAD_DIFF
