/////////////////////////////////////////////////////////////////////////////

// Name:        zones_polygons_insulated_copper_islands.cpp
// Licence:     GPL License
/////////////////////////////////////////////////////////////////////////////

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

using namespace std;

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "PolyLine.h"

#include "zones.h"


static void CalculateSubAreaBoundaryBox( EDA_Rect&             aBbox,
                                         std::vector <CPolyPt> aPolysList,
                                         int                   aIndexStart,
                                         int                   aIndexEnd );

/* Local variables */
std::vector <wxPoint> s_ListPoints;  // list of coordinates of pads and vias on this layer and on this net.


/***************************************************************************************/
void ZONE_CONTAINER::Test_For_Copper_Island_And_Remove_Insulated_Islands( BOARD* aPcb )
/***************************************************************************************/

/**
 * Function Test_For_Copper_Island_And_Remove__Insulated_Islands
 * Remove insulated copper islands found in m_FilledPolysList.
 * @param aPcb = the board to analyse
 */
{
    if( m_FilledPolysList.size() == 0 )
        return;

    // Build the list:
    s_ListPoints.clear();
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            if( !pad->IsOnLayer( GetLayer() ) )
                continue;

            if( pad->GetNet() != GetNet() )
                continue;

            s_ListPoints.push_back( pad->m_Pos );
        }
    }

    for( TRACK* track = aPcb->m_Track;  track;  track = track->Next() )
    {
        if( !track->IsOnLayer( GetLayer() ) )
            continue;
        if( track->GetNet() != GetNet() )
            continue;
        s_ListPoints.push_back( track->m_Start );
        if( track->Type() != TYPEVIA )
            s_ListPoints.push_back( track->m_End );
    }

    // test if a point is inside
    unsigned indexstart = 0, indexend;
    bool     connected  = false;
    for( indexend = 0; indexend < m_FilledPolysList.size(); indexend++ )
    {
        if( m_FilledPolysList[indexend].end_contour )   // end of area found
        {
            EDA_Rect bbox;
            CalculateSubAreaBoundaryBox( bbox, m_FilledPolysList, indexstart, indexend );
            for( unsigned ic = 0; ic < s_ListPoints.size(); ic++ )
            {
                wxPoint pos = s_ListPoints[ic];
                if( !bbox.Inside( pos ) )
                    continue;
                if( TestPointInsidePolygon( m_FilledPolysList, indexstart, indexend, pos.x, pos.y ) )
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
            else // Not connected: remove this polygon
            {
                m_FilledPolysList.erase(
                    m_FilledPolysList.begin() + indexstart,
                    m_FilledPolysList.begin() + indexend + 1 );
                indexend = indexstart;  /* indexstart points the first point of the next polygon
                                         * because the current poly is removed */
            }
        }
    }
}


/******************************************************************/
void CalculateSubAreaBoundaryBox( EDA_Rect&             aBbox,
                                  std::vector <CPolyPt> aPolysList,
                                  int                   aIndexStart,
                                  int                   aIndexEnd )
/******************************************************************/

/** function CalculateSubAreaBoundaryBox
  * Calculates the bounding box of a polygon stored in a vector <CPolyPt>
  * @param aBbox = EDA_Rect to init as bounding box
  * @param aPolysList =  set of CPolyPt that are the corners of one or more polygons
  * @param aIndexStart = index of the first corner of a polygon in aPolysList
  * @param aIndexEnd = index of the last corner of a polygon in aPolysList
 */
{
    CPolyPt start_point, end_point;

    start_point = aPolysList[aIndexStart];
    end_point   = start_point;
    for( int ii = aIndexStart; ii <= aIndexEnd; ii++ )
    {
        CPolyPt ptst = aPolysList[ii];
        if( start_point.x > ptst.x )
            start_point.x = ptst.x;
        if( start_point.y > ptst.y )
            start_point.y = ptst.y;
        if( end_point.x < ptst.x )
            end_point.x = ptst.x;
        if( end_point.y < ptst.y )
            end_point.y = ptst.y;
    }

    aBbox.SetOrigin( start_point.x, start_point.y );
    aBbox.SetEnd( wxPoint( end_point.x, end_point.y ) );
}
