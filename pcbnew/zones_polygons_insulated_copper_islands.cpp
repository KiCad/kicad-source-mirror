/**
 * @file zones_polygons_insulated_copper_islands.cpp
 */

using namespace std;

#include "fctsys.h"
#include "common.h"

#include "class_board.h"
#include "class_module.h"
#include "class_track.h"
#include "class_zone.h"

#include "pcbnew.h"
#include "zones.h"


/**
 * Function Test_For_Copper_Island_And_Remove__Insulated_Islands
 * Remove insulated copper islands found in m_FilledPolysList.
 * @param aPcb = the board to analyse
 */
void ZONE_CONTAINER::Test_For_Copper_Island_And_Remove_Insulated_Islands( BOARD * aPcb )
{
    if( m_FilledPolysList.size() == 0 )
        return;

    // Build a list of points connected to the net:
    // list of coordinates of pads and vias on this layer and on this net.
    std::vector <wxPoint> ListPointsCandidates;

    for( MODULE* module = aPcb->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            if( !pad->IsOnLayer( GetLayer() ) )
                continue;

            if( pad->GetNet() != GetNet() )
                continue;

            ListPointsCandidates.push_back( pad->m_Pos );
        }
    }

    for( TRACK* track = aPcb->m_Track; track; track = track->Next() )
    {
        if( !track->IsOnLayer( GetLayer() ) )
            continue;

        if( track->GetNet() != GetNet() )
            continue;

        ListPointsCandidates.push_back( track->m_Start );

        if( track->Type() != PCB_VIA_T )
            ListPointsCandidates.push_back( track->m_End );
    }

    // test if a point is inside
    unsigned indexstart = 0, indexend;
    bool     connected  = false;

    for( indexend = 0; indexend < m_FilledPolysList.size(); indexend++ )
    {
        if( m_FilledPolysList[indexend].end_contour )    // end of a filled sub-area found
        {
            EDA_RECT bbox = CalculateSubAreaBoundaryBox( indexstart, indexend );

            for( unsigned ic = 0; ic < ListPointsCandidates.size(); ic++ )
            {
                // test if this area is connected to a board item:
                wxPoint pos = ListPointsCandidates[ic];

                if( !bbox.Contains( pos ) )
                    continue;

                if( TestPointInsidePolygon( m_FilledPolysList, indexstart, indexend,
                                            pos.x, pos.y ) )
                {
                    connected = true;
                    break;
                }
            }

            if( connected )                 // this polygon is connected: analyse next polygon
            {
                indexstart = indexend + 1;  // indexstart points the first point of the next polygon
                connected  = false;
            }
            else                             // Not connected: remove this polygon
            {
                m_FilledPolysList.erase( m_FilledPolysList.begin() + indexstart,
                                         m_FilledPolysList.begin() + indexend + 1 );

                indexend = indexstart;   /* indexstart points the first point of the next polygon
                                          * because the current poly is removed */
            }
        }
    }
}


/**
 * Function CalculateSubAreaBoundaryBox
 * Calculates the bounding box of a a filled area ( list of CPolyPt )
 * use m_FilledPolysList as list of CPolyPt (that are the corners of one or more polygons or
 * filled areas )
 * @return an EDA_RECT as bounding box
 * @param aIndexStart = index of the first corner of a polygon (filled area) in m_FilledPolysList
 * @param aIndexEnd = index of the last corner of a polygon in m_FilledPolysList
 */
EDA_RECT ZONE_CONTAINER::CalculateSubAreaBoundaryBox( int aIndexStart, int aIndexEnd )
{
    CPolyPt  start_point, end_point;
    EDA_RECT bbox;

    start_point = m_FilledPolysList[aIndexStart];
    end_point   = start_point;

    for( int ii = aIndexStart; ii <= aIndexEnd; ii++ )
    {
        CPolyPt ptst = m_FilledPolysList[ii];

        if( start_point.x > ptst.x )
            start_point.x = ptst.x;

        if( start_point.y > ptst.y )
            start_point.y = ptst.y;

        if( end_point.x < ptst.x )
            end_point.x = ptst.x;

        if( end_point.y < ptst.y )
            end_point.y = ptst.y;
    }

    bbox.SetOrigin( start_point.x, start_point.y );
    bbox.SetEnd( wxPoint( end_point.x, end_point.y ) );

    return bbox;
}
