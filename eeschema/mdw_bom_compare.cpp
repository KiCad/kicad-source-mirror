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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "mdw_bom_compare.h"

#include <sch_reference_list.h>
#include <sch_symbol.h>
#include <symbol_fields_data_model.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <tuple>


namespace
{

/// One part may arrive with an empty source here and its owner's locator there
const wxString& sourceOf( const MDW_ITEM_REF& aRef, const wxString& aBomSource )
{
    return aRef.source.IsEmpty() && !aRef.item_id.IsEmpty() ? aBomSource : aRef.source;
}


bool samePart( const MDW_ITEM_REF& aLhs, const MDW_ITEM_REF& aRhs, const wxString& aBomSource )
{
    return !aLhs.item_id.IsEmpty() && aLhs.item_id == aRhs.item_id
           && sourceOf( aLhs, aBomSource ) == sourceOf( aRhs, aBomSource );
}


MDW_BOM_STATUS partStatus( const MDW_DESIGN_SYMBOL& aSymbol, const MDW_BOM_LINE& aLine, const wxString& aBomSource )
{
    if( aLine.part.item_id.IsEmpty() )
        return MDW_BOM_STATUS::PART_UNKNOWN;

    if( aSymbol.part )
    {
        return samePart( *aSymbol.part, aLine.part, aBomSource ) ? MDW_BOM_STATUS::MATCHES
                                                                 : MDW_BOM_STATUS::DIFFERENT_PART;
    }

    if( aSymbol.candidates.empty() )
        return MDW_BOM_STATUS::PART_UNKNOWN;

    // A generic symbol serves several parts, so any candidate agreeing with the line is a match
    bool anyMatch = std::any_of( aSymbol.candidates.begin(), aSymbol.candidates.end(),
                                 [&]( const MDW_ITEM_REF& aCandidate )
                                 {
                                     return samePart( aCandidate, aLine.part, aBomSource );
                                 } );

    return anyMatch ? MDW_BOM_STATUS::MATCHES : MDW_BOM_STATUS::DIFFERENT_PART;
}


struct OUTCOME
{
    MDW_BOM_STATUS status = MDW_BOM_STATUS::NOT_IN_WORKSPACE;
    int            line = -1;
    bool           repeat = false; ///< Another unit of a reference already counted
};

} // namespace


std::vector<MDW_BOM_ROW> MdwCompareBom( const std::vector<std::vector<MDW_DESIGN_SYMBOL>>& aDesign,
                                        const MDW_BOM_DATA& aBom, const wxString& aBomSource )
{
    const std::vector<std::vector<MDW_DESIGN_SYMBOL>>& design = aDesign;
    const MDW_BOM_DATA&                                bom = aBom;

    std::map<wxString, int> lineOfRef;

    for( size_t ii = 0; ii < bom.lines.size(); ++ii )
    {
        for( const wxString& ref : bom.lines[ii].references )
            lineOfRef.emplace( ref, static_cast<int>( ii ) );
    }

    std::set<wxString>                claimed;
    std::set<wxString>                seen;
    std::vector<std::vector<OUTCOME>> outcomes( design.size() );

    for( size_t gg = 0; gg < design.size(); ++gg )
    {
        for( const MDW_DESIGN_SYMBOL& symbol : design[gg] )
        {
            OUTCOME& outcome = outcomes[gg].emplace_back();
            auto     it = lineOfRef.find( symbol.reference );

            // Unannotated symbols share "R?" without being units of one another
            if( !symbol.reference.EndsWith( wxS( "?" ) ) && !seen.insert( symbol.reference ).second )
                outcome.repeat = true;
            else if( it != lineOfRef.end() )
            {
                outcome = { partStatus( symbol, bom.lines[it->second], aBomSource ), it->second };
                claimed.insert( symbol.reference );
            }
        }
    }

    // Lines without designators for one part and unit pool their quantities under the first of them
    std::vector<size_t>                                        refLessLead( bom.lines.size() );
    std::vector<double>                                        refLessTotal( bom.lines.size(), 0.0 );
    std::map<std::tuple<wxString, wxString, wxString>, size_t> leadOfPart;

    for( size_t ll = 0; ll < bom.lines.size(); ++ll )
    {
        const MDW_BOM_LINE& line = bom.lines[ll];

        if( !line.references.empty() )
            continue;

        auto key = std::make_tuple( sourceOf( line.part, aBomSource ), line.part.item_id, line.unit );
        refLessLead[ll] = leadOfPart.emplace( key, ll ).first->second;
        refLessTotal[refLessLead[ll]] += line.quantity;
    }

    // A generic symbol may fit several lines, so fill them by matching rather than first come
    std::vector<OUTCOME*>            placing;
    std::vector<std::vector<size_t>> options;
    std::vector<size_t>              capacity( bom.lines.size(), 0 );
    std::vector<std::vector<size_t>> holders( bom.lines.size() );

    for( size_t gg = 0; gg < design.size(); ++gg )
    {
        for( size_t ss = 0; ss < design[gg].size(); ++ss )
        {
            if( outcomes[gg][ss].repeat || outcomes[gg][ss].line >= 0 )
                continue;

            std::vector<size_t> fits;

            for( size_t ll = 0; ll < bom.lines.size(); ++ll )
            {
                if( bom.lines[ll].references.empty() && refLessLead[ll] == ll
                    && partStatus( design[gg][ss], bom.lines[ll], aBomSource ) == MDW_BOM_STATUS::MATCHES )
                {
                    fits.push_back( ll );
                }
            }

            if( !fits.empty() )
            {
                placing.push_back( &outcomes[gg][ss] );
                options.push_back( std::move( fits ) );
            }
        }
    }

    std::vector<bool>             visited;
    std::function<bool( size_t )> place = [&]( size_t aSymbol ) -> bool
    {
        for( size_t ll : options[aSymbol] )
        {
            if( visited[ll] )
                continue;

            visited[ll] = true;

            if( holders[ll].size() < capacity[ll] )
            {
                holders[ll].push_back( aSymbol );
                return true;
            }

            for( size_t& holder : holders[ll] )
            {
                if( place( holder ) )
                {
                    holder = aSymbol;
                    return true;
                }
            }
        }

        return false;
    };

    // A measured line takes one symbol until every piece line has had its chance
    for( size_t ll = 0; ll < bom.lines.size(); ++ll )
    {
        if( !bom.lines[ll].references.empty() || refLessLead[ll] != ll )
            continue;

        double pieces = std::max( 0.0, std::round( refLessTotal[ll] ) );

        if( !bom.lines[ll].unit.IsEmpty() )
            capacity[ll] = 1;
        else if( pieces < static_cast<double>( placing.size() ) )
            capacity[ll] = static_cast<size_t>( pieces );
        else
            capacity[ll] = placing.size();
    }

    std::vector<size_t> unplaced;

    for( size_t ii = 0; ii < placing.size(); ++ii )
    {
        visited.assign( bom.lines.size(), false );

        if( !place( ii ) )
            unplaced.push_back( ii );
    }

    for( size_t ll = 0; ll < bom.lines.size(); ++ll )
    {
        if( bom.lines[ll].references.empty() && refLessLead[ll] == ll && !bom.lines[ll].unit.IsEmpty() )
            capacity[ll] = placing.size();
    }

    for( size_t ii : unplaced )
    {
        visited.assign( bom.lines.size(), false );

        // What no line has room for still counts against its first line, which then reads as differing
        if( !place( ii ) )
            holders[options[ii].front()].push_back( ii );
    }

    std::vector<size_t> refLessCount( bom.lines.size(), 0 );

    for( size_t ll = 0; ll < bom.lines.size(); ++ll )
    {
        refLessCount[ll] = holders[ll].size();

        for( size_t symbol : holders[ll] )
            placing[symbol]->line = static_cast<int>( ll );
    }

    std::vector<MDW_BOM_ROW>   rows;
    std::map<int, size_t>      refLessRow;
    std::map<wxString, size_t> rowOfRef;

    for( size_t gg = 0; gg < design.size(); ++gg )
    {
        const size_t groupStart = rows.size();

        for( size_t ss = 0; ss < design[gg].size(); ++ss )
        {
            const wxString& ref = design[gg][ss].reference;
            const wxString& path = design[gg][ss].path;
            const OUTCOME&  outcome = outcomes[gg][ss];

            if( outcome.repeat )
            {
                rows[rowOfRef[ref]].design_paths.push_back( path );
                continue;
            }

            if( outcome.line >= 0 && bom.lines[outcome.line].references.empty() )
            {
                auto [it, added] = refLessRow.emplace( outcome.line, rows.size() );

                if( added )
                {
                    const MDW_BOM_LINE& line = bom.lines[outcome.line];
                    MDW_BOM_ROW&        row = rows.emplace_back();

                    double total = refLessTotal[outcome.line];

                    // Only a count of pieces can be checked against the number of symbols
                    bool countAgrees = !line.unit.IsEmpty() || std::abs( total - refLessCount[outcome.line] ) < 1e-9;

                    row.status = countAgrees ? MDW_BOM_STATUS::MATCHES : MDW_BOM_STATUS::QUANTITY_DIFFERS;
                    row.line = outcome.line;
                    row.workspace_quantity = total;
                }

                rowOfRef[ref] = it->second;
                rows[it->second].design_refs.push_back( ref );
                rows[it->second].design_paths.push_back( path );
                continue;
            }

            auto match = std::find_if( rows.begin() + groupStart, rows.end(),
                                       [&]( const MDW_BOM_ROW& aRow )
                                       {
                                           return aRow.status == outcome.status && aRow.line == outcome.line
                                                  && ( aRow.line < 0 || !bom.lines[aRow.line].references.empty() );
                                       } );

            rowOfRef[ref] = match != rows.end() ? match - rows.begin() : rows.size();

            MDW_BOM_ROW& row = match != rows.end() ? *match : rows.emplace_back();
            row.status = outcome.status;
            row.line = outcome.line;
            row.design_refs.push_back( ref );
            row.design_paths.push_back( path );

            if( outcome.line >= 0 )
                row.workspace_refs.push_back( ref );
        }
    }

    for( MDW_BOM_ROW& row : rows )
    {
        if( row.line < 0 || row.workspace_refs.empty() )
            continue;

        const MDW_BOM_LINE& line = bom.lines[row.line];
        row.workspace_quantity = line.quantity * row.workspace_refs.size() / line.references.size();

        bool partAgrees = row.status == MDW_BOM_STATUS::MATCHES || row.status == MDW_BOM_STATUS::PART_UNKNOWN;

        // A count of pieces must agree with the line's own designators, known part or not
        if( partAgrees && line.unit.IsEmpty() && std::abs( line.quantity - line.references.size() ) >= 1e-9 )
        {
            row.status = MDW_BOM_STATUS::QUANTITY_DIFFERS;
        }
    }

    for( size_t ll = 0; ll < bom.lines.size(); ++ll )
    {
        const MDW_BOM_LINE& line = bom.lines[ll];
        MDW_BOM_ROW         row;

        row.status = MDW_BOM_STATUS::NOT_IN_DESIGN;
        row.line = static_cast<int>( ll );

        if( line.references.empty() )
        {
            if( refLessCount[refLessLead[ll]] > 0 )
                continue;

            row.workspace_quantity = line.quantity;
            rows.push_back( row );
            continue;
        }

        for( const wxString& ref : line.references )
        {
            if( !claimed.count( ref ) && lineOfRef[ref] == row.line )
                row.workspace_refs.push_back( ref );
        }

        if( row.workspace_refs.empty() )
            continue;

        row.workspace_quantity = line.quantity * row.workspace_refs.size() / line.references.size();
        rows.push_back( row );
    }

    return rows;
}


MDW_BOM_VIEW MdwBomView( const std::vector<MDW_BOM_ROW>& aRows, bool aDifferencesOnly )
{
    MDW_BOM_VIEW view;

    for( size_t ii = 0; ii < aRows.size(); ++ii )
    {
        const MDW_BOM_STATUS status = aRows[ii].status;
        const bool           known = status != MDW_BOM_STATUS::PART_UNKNOWN;
        const bool           differs = known && status != MDW_BOM_STATUS::MATCHES;

        view.differences += differs;
        view.unknown += !known;
        view.references += static_cast<int>( aRows[ii].design_refs.size() );

        if( differs || !aDifferencesOnly )
            view.shown.push_back( ii );
    }

    return view;
}


std::vector<std::vector<MDW_DESIGN_SYMBOL>> MdwDesignGroups( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL& aModel,
                                                             const MDW_SYMBOL_IDENTITY&            aIdentify )
{
    std::vector<std::vector<MDW_DESIGN_SYMBOL>> groups;

    for( int row = 0; row < aModel.GetNumberRows(); ++row )
    {
        // An expanded group's members are already in its own row
        if( aModel.GetRowState( row ) == ROW_STATE::EXPANDED_CHILD )
            continue;

        std::vector<MDW_DESIGN_SYMBOL>& group = groups.emplace_back();

        for( const SCH_REFERENCE& ref : aModel.GetRowGroupItems( row ) )
        {
            MDW_DESIGN_SYMBOL& symbol = group.emplace_back();
            symbol.reference = ref.GetSymbol()->GetRef( &ref.GetSheetPath() );
            symbol.path = ref.GetFullPath();
            symbol.lib_id = ref.GetSymbol()->GetLibId();

            std::optional<SCH_SYMBOL_VARIANT> variant =
                    ref.GetSymbol()->GetVariant( ref.GetSheetPath(), aModel.GetCurrentVariant() );

            if( variant && variant->m_SymbolOverride )
                symbol.lib_id = *variant->m_SymbolOverride;

            if( aIdentify )
                aIdentify( ref, symbol );
        }
    }

    return groups;
}
