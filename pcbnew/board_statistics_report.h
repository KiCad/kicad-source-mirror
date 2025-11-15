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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCBNEW_BOARD_STATISTICS_REPORT_H
#define PCBNEW_BOARD_STATISTICS_REPORT_H

#include <board_statistics.h>
#include <units_provider.h>

#include <vector>

class BOARD;
enum class VIATYPE : int;

struct BOARD_STATISTICS_OPTIONS
{
    bool excludeFootprintsWithoutPads = false;
    bool subtractHolesFromBoardArea = false;
    bool subtractHolesFromCopperAreas = false;
};

struct BOARD_STATISTICS_FP_ENTRY
{
    BOARD_STATISTICS_FP_ENTRY( int aMask, int aValue, const wxString& aTitle ) :
            attributeMask( aMask ),
            attributeValue( aValue ),
            title( aTitle )
    {
    }

    int      attributeMask;
    int      attributeValue;
    wxString title;
    int      frontCount = 0;
    int      backCount = 0;
};

template <typename T>
struct BOARD_STATISTICS_INFO_ENTRY
{
    BOARD_STATISTICS_INFO_ENTRY( T aAttribute, const wxString& aTitle ) :
            attribute( aAttribute ),
            title( aTitle )
    {
    }

    T        attribute;
    wxString title;
    int      quantity = 0;
};

struct BOARD_STATISTICS_DATA
{
    BOARD_STATISTICS_DATA();

    void ResetCounts();

    bool   hasOutline;
    int    boardWidth;
    int    boardHeight;
    double boardArea;
    double frontCopperArea;
    double backCopperArea;
    double frontFootprintCourtyardArea;
    double backFootprintCourtyardArea;
    double frontFootprintDensity;
    double backFootprintDensity;
    int    minClearanceTrackToTrack;
    int    minTrackWidth;
    int    minDrillSize;
    int    boardThickness;

    std::vector<BOARD_STATISTICS_FP_ENTRY>               footprintEntries;
    std::vector<BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>> padEntries;
    std::vector<BOARD_STATISTICS_INFO_ENTRY<PAD_PROP>>   padPropertyEntries;
    std::vector<BOARD_STATISTICS_INFO_ENTRY<VIATYPE>>    viaEntries;
    std::vector<DRILL_LINE_ITEM>                         drillEntries;
};

void InitializeBoardStatisticsData( BOARD_STATISTICS_DATA& aData );

void ComputeBoardStatistics( BOARD* aBoard, const BOARD_STATISTICS_OPTIONS& aOptions, BOARD_STATISTICS_DATA& aData );

wxString FormatBoardStatisticsReport( const BOARD_STATISTICS_DATA& aData, BOARD* aBoard,
                                      const UNITS_PROVIDER& aUnitsProvider, const wxString& aProjectName,
                                      const wxString& aBoardName );

wxString FormatBoardStatisticsJson( const BOARD_STATISTICS_DATA& aData, BOARD* aBoard,
                                    const UNITS_PROVIDER& aUnitsProvider, const wxString& aProjectName,
                                    const wxString& aBoardName );

#endif
