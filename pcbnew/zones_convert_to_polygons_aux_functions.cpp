/**
 * @file zones_convert_to_polygons_aux_functions.cpp
 */

#include <fctsys.h>
#include <polygons_defs.h>
#include <PolyLine.h>
#include <wxPcbStruct.h>
#include <trigo.h>

#include <class_board.h>
#include <class_module.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>


/**
 * Function BuildUnconnectedThermalStubsPolygonList
 * Creates a set of polygons corresponding to stubs created by thermal shapes on pads
 * which are not connected to a zone (dangling bridges)
 * @param aCornerBuffer = a std::vector<CPolyPt> where to store polygons
 * @param aPcb = the board.
 * @param aZone = a pointer to the ZONE_CONTAINER  to examine.
 * @param aArcCorrection = a pointer to the ZONE_CONTAINER  to examine.
 * @param aRoundPadThermalRotation = the rotation in 1.0 degree for thermal stubs in round pads
 */

void BuildUnconnectedThermalStubsPolygonList( std::vector<CPolyPt>& aCornerBuffer,
                                              BOARD*                aPcb,
                                              ZONE_CONTAINER*       aZone,
                                              double                aArcCorrection,
                                              int                   aRoundPadThermalRotation )
{
    std::vector<wxPoint> corners_buffer;    // a local polygon buffer to store one stub
    corners_buffer.reserve( 4 );
    wxPoint  ptTest[4];

    int      zone_clearance = aZone->m_ZoneClearance;

    EDA_RECT item_boundingbox;
    EDA_RECT zone_boundingbox  = aZone->GetBoundingBox();
    int      biggest_clearance = aPcb->GetBiggestClearanceValue();
    biggest_clearance = max( biggest_clearance, zone_clearance );
    zone_boundingbox.Inflate( biggest_clearance );

    // half size of the pen used to draw/plot zones outlines
    int pen_radius = aZone->m_ZoneMinThickness / 2;

    // Calculate thermal bridge half width
    int thermbridgeWidth = aZone->m_ThermalReliefCopperBridge / 2;
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            if( aZone->GetPadConnection( pad ) != THERMAL_PAD )
                continue;

            // check
            if( !pad->IsOnLayer( aZone->GetLayer() ) )
                continue;

            if( pad->GetNet() != aZone->GetNet() )
                continue;

            item_boundingbox = pad->GetBoundingBox();
            item_boundingbox.Inflate( aZone->m_ThermalReliefGap );
            if( !( item_boundingbox.Intersects( zone_boundingbox ) ) )
                continue;

            // Thermal bridges are like a segment from a starting point inside the pad
            // to an ending point outside the pad
            wxPoint startpoint, endpoint;
            endpoint.x = ( pad->GetSize().x / 2 ) + aZone->m_ThermalReliefGap;
            endpoint.y = ( pad->GetSize().y / 2 ) + aZone->m_ThermalReliefGap;

            int copperThickness = aZone->m_ThermalReliefCopperBridge - aZone->m_ZoneMinThickness;
            if( copperThickness < 0 )
                copperThickness = 0;

            startpoint.x = min( pad->GetSize().x, copperThickness );
            startpoint.y = min( pad->GetSize().y, copperThickness );
            startpoint.x /= 2;
            startpoint.y /= 2;

            // This is CIRCLE pad tweak (for circle pads the thermal stubs are at 45 deg)
            int fAngle = pad->GetOrientation();
            if( pad->GetShape() == PAD_CIRCLE )
            {
                endpoint.x     = (int) ( endpoint.x * aArcCorrection );
                endpoint.y     = endpoint.x;
                fAngle = aRoundPadThermalRotation;
            }

            // contour line width has to be taken into calculation to avoid "thermal stub bleed"
            endpoint.x += pen_radius;
            endpoint.y += pen_radius;
            // compute north, south, west and east points for zone connection.
            ptTest[0] = wxPoint( 0, endpoint.y );       // lower point
            ptTest[1] = wxPoint( 0, -endpoint.y );      // upper point
            ptTest[2] = wxPoint( endpoint.x, 0 );       // right point
            ptTest[3] = wxPoint( -endpoint.x, 0 );      // left point

            // Test all sides
            for( int i = 0; i < 4; i++ )
            {
                // rotate point
                RotatePoint( &ptTest[i], fAngle );

                // translate point
                ptTest[i] += pad->ReturnShapePos();
                if( aZone->HitTestFilledArea( ptTest[i] ) )
                    continue;

                corners_buffer.clear();

                // polygons are rectangles with width of copper bridge value
                switch( i )
                {
                case 0:       // lower stub
                    corners_buffer.push_back( wxPoint( -thermbridgeWidth, endpoint.y ) );
                    corners_buffer.push_back( wxPoint( +thermbridgeWidth, endpoint.y ) );
                    corners_buffer.push_back( wxPoint( +thermbridgeWidth, startpoint.y ) );
                    corners_buffer.push_back( wxPoint( -thermbridgeWidth, startpoint.y ) );
                    break;

                case 1:       // upper stub
                    corners_buffer.push_back( wxPoint( -thermbridgeWidth, -endpoint.y ) );
                    corners_buffer.push_back( wxPoint( +thermbridgeWidth, -endpoint.y ) );
                    corners_buffer.push_back( wxPoint( +thermbridgeWidth, -startpoint.y ) );
                    corners_buffer.push_back( wxPoint( -thermbridgeWidth, -startpoint.y ) );
                    break;

                case 2:       // right stub
                    corners_buffer.push_back( wxPoint( endpoint.x, -thermbridgeWidth ) );
                    corners_buffer.push_back( wxPoint( endpoint.x, thermbridgeWidth ) );
                    corners_buffer.push_back( wxPoint( +startpoint.x, thermbridgeWidth ) );
                    corners_buffer.push_back( wxPoint( +startpoint.x, -thermbridgeWidth ) );
                    break;

                case 3:       // left stub
                    corners_buffer.push_back( wxPoint( -endpoint.x, -thermbridgeWidth ) );
                    corners_buffer.push_back( wxPoint( -endpoint.x, thermbridgeWidth ) );
                    corners_buffer.push_back( wxPoint( -startpoint.x, thermbridgeWidth ) );
                    corners_buffer.push_back( wxPoint( -startpoint.x, -thermbridgeWidth ) );
                    break;
                }


                // add computed polygon to list
                for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
                {
                    wxPoint cpos = corners_buffer[ic];
                    RotatePoint( &cpos, fAngle );                               // Rotate according to module orientation
                    cpos += pad->ReturnShapePos();                              // Shift origin to position
                    CPolyPt corner;
                    corner.x = cpos.x;
                    corner.y = cpos.y;
                    corner.end_contour = ( ic < (corners_buffer.size() - 1) ) ? 0 : 1;
                    aCornerBuffer.push_back( corner );
                }
            }
        }
    }
}
