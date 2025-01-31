/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
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

/**
 * @file spread_footprints.cpp
 * @brief functions to spread footprints on free areas outside a board.
 * this is useful after reading a netlist, when new footprints are loaded
 * and stacked at 0,0 coordinate.
 * Often, spread them on a free area near the board being edited make more easy
 * their selection.
 */

#include <spread_footprints.h>
#include <optional>
#include <algorithm>
#include <refdes_utils.h>
#include <string_utils.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <rectpack2d/finders_interface.h>


constexpr bool allow_flip = true;

using spaces_type = rectpack2D::empty_spaces<allow_flip, rectpack2D::default_empty_spaces>;
using rect_type = rectpack2D::output_rect_t<spaces_type>;
using rect_ptr = rect_type*;
using rect_vector = std::vector<rect_type>;

// Use 0.01 mm units to calculate placement, to avoid long calculation time
const int scale = (int) ( 0.01 * pcbIUScale.IU_PER_MM );


static bool compareFootprintsbyRef( FOOTPRINT* ref, FOOTPRINT* compare )
{
    const wxString& refPrefix = UTIL::GetRefDesPrefix( ref->GetReference() );
    const wxString& cmpPrefix = UTIL::GetRefDesPrefix( compare->GetReference() );

    if( refPrefix != cmpPrefix )
    {
        return refPrefix < cmpPrefix;
    }
    else
    {
        const int refInt = GetTrailingInt( ref->GetReference() );
        const int cmpInt = GetTrailingInt( compare->GetReference() );

        return refInt < cmpInt;
    }

    return false;
}


// Spread a list of rectangles inside a placement area
std::optional<rectpack2D::rect_wh> spreadRectangles( rect_vector& vecSubRects, int areaSizeX,
                                                     int areaSizeY )
{
    areaSizeX /= scale;
    areaSizeY /= scale;

    std::optional<rectpack2D::rect_wh> result;

    int max_side = std::max( areaSizeX, areaSizeY );

    for( int i = 0; i < 2000; i++ )
    {
        bool      anyUnsuccessful = false;
        const int discard_step = 1;

        auto report_successful = [&]( rect_type& )
        {
            return rectpack2D::callback_result::CONTINUE_PACKING;
        };

        auto report_unsuccessful = [&]( rect_type& r )
        {
            anyUnsuccessful = true;
            return rectpack2D::callback_result::ABORT_PACKING;
        };

        result = rectpack2D::find_best_packing<spaces_type>(
                vecSubRects,
                make_finder_input( max_side, discard_step, report_successful, report_unsuccessful,
                                   rectpack2D::flipping_option::DISABLED ) );

        if( !result || anyUnsuccessful )
        {
            max_side = (int) ( max_side * 1.2 );
            continue;
        }

        break;
    }

    return result;
}


void SpreadFootprints( std::vector<FOOTPRINT*>* aFootprints, VECTOR2I aTargetBoxPosition,
                       bool aGroupBySheet, int aComponentGap, int aGroupGap )
{
    using FpBBoxToFootprintsPair = std::pair<BOX2I, std::vector<FOOTPRINT*>>;
    using SheetBBoxToFootprintsMapPair =
            std::pair<BOX2I, std::map<VECTOR2I, FpBBoxToFootprintsPair>>;

    std::map<wxString, SheetBBoxToFootprintsMapPair> sheetsMap;

    // Fill in the maps
    for( FOOTPRINT* footprint : *aFootprints )
    {
        wxString path =
                aGroupBySheet ? footprint->GetPath().AsString().BeforeLast( '/' ) : wxString( wxS( "" ) );

        VECTOR2I size = footprint->GetBoundingBox( false ).GetSize();
        size.x += aComponentGap;
        size.y += aComponentGap;

        sheetsMap[path].second[size].second.push_back( footprint );
    }

    for( auto& [sheetPath, sheetPair] : sheetsMap )
    {
        auto& [sheet_bbox, sizeToFpMap] = sheetPair;

        for( auto& [fpSize, fpPair] : sizeToFpMap )
        {
            auto& [block_bbox, footprints] = fpPair;

            // Find optimal arrangement of same-size footprints

            double blockEstimateArea = (double) fpSize.x * fpSize.y * footprints.size();
            double initialSide = std::sqrt( blockEstimateArea );
            bool   vertical = fpSize.x >= fpSize.y;

            int initialCountPerLine = footprints.size();

            const int singleLineRatio = 5;

            // Wrap the line if the ratio is not satisfied
            if( vertical )
            {
                if( ( fpSize.y * footprints.size() / fpSize.x ) > singleLineRatio )
                    initialCountPerLine = initialSide / fpSize.y;
            }
            else
            {
                if( ( fpSize.x * footprints.size() / fpSize.y ) > singleLineRatio )
                    initialCountPerLine = initialSide / fpSize.x;
            }

            int optimalCountPerLine = initialCountPerLine;
            int optimalRemainder = footprints.size() % optimalCountPerLine;

            if( optimalRemainder != 0 )
            {
                for( int i = std::max( 2, initialCountPerLine - 2 );
                     i <= std::min( (int) footprints.size() - 2, initialCountPerLine + 2 ); i++ )
                {
                    int r = footprints.size() % i;

                    if( r == 0 || r >= optimalRemainder )
                    {
                        optimalCountPerLine = i;
                        optimalRemainder = r;
                    }
                }
            }

            std::sort( footprints.begin(), footprints.end(), compareFootprintsbyRef );

            // Arrange footprints in rows or columns (blocks)
            for( unsigned i = 0; i < footprints.size(); i++ )
            {
                FOOTPRINT* footprint = footprints[i];

                VECTOR2I position = fpSize / 2;

                if( vertical )
                {
                    position.x += fpSize.x * ( i / optimalCountPerLine );
                    position.y += fpSize.y * ( i % optimalCountPerLine );
                }
                else
                {
                    position.x += fpSize.x * ( i % optimalCountPerLine );
                    position.y += fpSize.y * ( i / optimalCountPerLine );
                }

                BOX2I old_fp_bbox = footprint->GetBoundingBox( false );
                footprint->Move( position - old_fp_bbox.GetOrigin() );

                BOX2I new_fp_bbox = footprint->GetBoundingBox( false );
                new_fp_bbox.Inflate( aComponentGap / 2 );
                block_bbox.Merge( new_fp_bbox );
            }
        }

        rect_vector vecSubRects;
        long long   blocksArea = 0;

        // Fill in arrays for packing of blocks
        for( auto& [fpSize, fpPair] : sizeToFpMap )
        {
            auto& [block_bbox, footprints] = fpPair;

            vecSubRects.emplace_back( 0, 0, block_bbox.GetWidth() / scale,
                                      block_bbox.GetHeight() / scale, false );

            blocksArea += block_bbox.GetArea();
        }

        // Pack the blocks
        int areaSide = std::sqrt( blocksArea );
        spreadRectangles( vecSubRects, areaSide, areaSide );

        unsigned block_i = 0;

        // Move footprints to the new block locations
        for( auto& [fpSize, pair] : sizeToFpMap )
        {
            auto& [src_bbox, footprints] = pair;

            rect_type srect = vecSubRects[block_i];

            VECTOR2I target_pos( srect.x * scale, srect.y * scale );
            VECTOR2I target_size( srect.w * scale, srect.h * scale );

            // Avoid too large coordinates: Overlapping components
            // are better than out of screen components
            if( (uint64_t) target_pos.x + (uint64_t) target_size.x > INT_MAX / 2 )
                target_pos.x -= INT_MAX / 2;

            if( (uint64_t) target_pos.y + (uint64_t) target_size.y > INT_MAX / 2 )
                target_pos.y -= INT_MAX / 2;

            for( FOOTPRINT* footprint : footprints )
            {
                footprint->Move( target_pos - src_bbox.GetPosition() );
                sheet_bbox.Merge( footprint->GetBoundingBox( false ) );
            }

            block_i++;
        }
    }

    rect_vector vecSubRects;
    long long   sheetsArea = 0;

    // Fill in arrays for packing of hierarchical sheet groups
    for( auto& [sheetPath, sheetPair] : sheetsMap )
    {
        auto& [sheet_bbox, sizeToFpMap] = sheetPair;
        BOX2I rect = sheet_bbox;

        // Add a margin around the sheet placement area:
        rect.Inflate( aGroupGap );

        vecSubRects.emplace_back( 0, 0, rect.GetWidth() / scale, rect.GetHeight() / scale, false );

        sheetsArea += sheet_bbox.GetArea();
    }

    // Pack the hierarchical sheet groups
    int areaSide = std::sqrt( sheetsArea );
    spreadRectangles( vecSubRects, areaSide, areaSide );

    unsigned srect_i = 0;

    // Move footprints to the new hierarchical sheet group locations
    for( auto& [sheetPath, sheetPair] : sheetsMap )
    {
        auto& [src_bbox, sizeToFpMap] = sheetPair;

        rect_type srect = vecSubRects[srect_i];

        VECTOR2I target_pos( srect.x * scale + aTargetBoxPosition.x,
                             srect.y * scale + aTargetBoxPosition.y );
        VECTOR2I target_size( srect.w * scale, srect.h * scale );

        // Avoid too large coordinates: Overlapping components
        // are better than out of screen components
        if( (int64_t) target_pos.x + (int64_t) target_size.x > INT_MAX / 2 )
            target_pos.x -= INT_MAX / 2;

        if( (int64_t) target_pos.y + (int64_t) target_size.y > INT_MAX / 2 )
            target_pos.y -= INT_MAX / 2;

        for( auto& [fpSize, fpPair] : sizeToFpMap )
        {
            auto& [block_bbox, footprints] = fpPair;
            for( FOOTPRINT* footprint : footprints )
            {
                footprint->Move( target_pos - src_bbox.GetPosition() );
            }
        }

        srect_i++;
    }
}
