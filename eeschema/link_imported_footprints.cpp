/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "link_imported_footprints.h"

#include <set>

#include <lib_id.h>
#include <reporter.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>


int LinkImportedFootprints( SCHEMATIC& aSchematic, const std::map<wxString, wxString>& aFootprintsByRef,
                            REPORTER& aReporter )
{
    int linked = 0;

    // A reused sheet shares its symbols across instances, so one footprint is only safe when every instance agrees
    std::map<SCH_SYMBOL*, std::set<wxString>> targets;

    for( const SCH_SHEET_PATH& path : aSchematic.Hierarchy() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            const wxString current = symbol->GetField( FIELD_T::FOOTPRINT )->GetText();
            const wxString ref = symbol->GetRef( &path );

            // an unannotated reference names no single part, and still counts against sharing instances
            auto footprint = ref.EndsWith( wxS( "?" ) ) ? aFootprintsByRef.end() : aFootprintsByRef.find( ref );
            wxString target = footprint == aFootprintsByRef.end() ? wxString() : footprint->second;

            if( !current.IsEmpty() )
            {
                // A field in the same library as the board's footprint names an item of the import cache. The
                // cache renames a footprint when two libraries supply different ones of a name, which only the
                // board knows, so the board's name replaces the one the field was given
                LIB_ID currentId;
                LIB_ID targetId;

                // an instance that keeps the field still counts against sharing instances
                if( target.IsEmpty() || currentId.Parse( current ) >= 0 || targetId.Parse( target ) >= 0
                    || currentId.GetLibNickname() != targetId.GetLibNickname() )
                {
                    targets[symbol].insert( wxString() );
                    continue;
                }
            }

            targets[symbol].insert( target );
        }
    }

    for( const auto& [symbol, footprints] : targets )
    {
        if( footprints.size() != 1 || footprints.begin()->IsEmpty() )
            continue;

        if( symbol->GetField( FIELD_T::FOOTPRINT )->GetText() == *footprints.begin() )
            continue;

        symbol->SetFootprintFieldText( *footprints.begin() );
        linked++;
    }

    if( linked > 0 )
    {
        aReporter.Report( wxString::Format( _( "Linked %d symbol(s) to the footprint with the same reference.\n" ),
                                            linked ),
                          RPT_SEVERITY_INFO );
    }

    return linked;
}
