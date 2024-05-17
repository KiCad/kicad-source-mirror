#include "junction_helpers.h"

#include <sch_line.h>

using namespace JUNCTION_HELPERS;

POINT_INFO JUNCTION_HELPERS::AnalyzePoint( const EE_RTREE& aItems, const VECTOR2I& aPosition,
                                           bool aBreakCrossings )
{
    enum layers
    {
        WIRES = 0,
        BUSES
    };

    POINT_INFO info {};
    info.hasBusEntry = false;
    info.hasExplicitJunctionDot = false;
    info.isJunction = false;

    bool                         breakLines[2] = { false };
    std::unordered_set<int>      exitAngles[2];
    std::vector<const SCH_LINE*> midPointLines[2];

    // A pin at 90° still shouldn't match a line at 90° so just give pins unique numbers
    int uniqueAngle = 10000;

    for( const SCH_ITEM* item : aItems.Overlapping( aPosition ) )
    {
        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
            if( item->HitTest( aPosition, -1 ) )
                info.hasExplicitJunctionDot = true;

            break;

        case SCH_LINE_T:
        {
            const SCH_LINE* line = static_cast<const SCH_LINE*>( item );
            int             layer;

            if( line->GetStartPoint() == line->GetEndPoint() )
                break;
            else if( line->GetLayer() == LAYER_WIRE )
                layer = WIRES;
            else if( line->GetLayer() == LAYER_BUS )
                layer = BUSES;
            else
                break;

            if( line->IsConnected( aPosition ) )
            {
                breakLines[layer] = true;
                exitAngles[layer].insert( line->GetAngleFrom( aPosition ) );
            }
            else if( line->HitTest( aPosition, -1 ) )
            {
                if( aBreakCrossings )
                    breakLines[layer] = true;

                // Defer any line midpoints until we know whether or not we're breaking them
                midPointLines[layer].push_back( line );
            }
        }
        break;

        case SCH_BUS_WIRE_ENTRY_T:
            if( item->IsConnected( aPosition ) )
            {
                breakLines[BUSES] = true;
                exitAngles[BUSES].insert( uniqueAngle++ );
                breakLines[WIRES] = true;
                exitAngles[WIRES].insert( uniqueAngle++ );
                info.hasBusEntry = true;
            }

            break;

        case SCH_SYMBOL_T:
        case SCH_SHEET_T:
            if( item->IsConnected( aPosition ) )
            {
                breakLines[WIRES] = true;
                exitAngles[WIRES].insert( uniqueAngle++ );
            }

            break;

        default: break;
        }
    }

    for( int layer : { WIRES, BUSES } )
    {
        if( breakLines[layer] )
        {
            for( const SCH_LINE* line : midPointLines[layer] )
            {
                exitAngles[layer].insert( line->GetAngleFrom( aPosition ) );
                exitAngles[layer].insert( line->GetReverseAngleFrom( aPosition ) );
            }
        }
    }

    info.isJunction = exitAngles[WIRES].size() >= 3 || exitAngles[BUSES].size() >= 3;
    return info;
}