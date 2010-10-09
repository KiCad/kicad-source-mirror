/**********************************/
/* classes to handle copper zones */
/**********************************/

#include "fctsys.h"
#include "wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "class_drawpanel.h"
#include "kicad_string.h"

#include "PolyLine.h"
#include "pcbnew.h"
#include "zones.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"

#include "protos.h"


/************************/
/* class ZONE_CONTAINER */
/************************/

ZONE_CONTAINER::ZONE_CONTAINER( BOARD* parent ) :
    BOARD_CONNECTED_ITEM( parent, TYPE_ZONE_CONTAINER )

{
    m_NetCode = -1;                           // Net number for fast comparisons
    m_CornerSelection = -1;
    m_IsFilled = false;                       // fill status : true when the zone is filled
    utility  = 0;                             // flags used in polygon calculations
    utility2 = 0;                             // flags used in polygon calculations
    m_Poly   = new CPolyLine();               // Outlines
    g_Zone_Default_Setting.ExportSetting(*this);
}


ZONE_CONTAINER::~ZONE_CONTAINER()
{
    delete m_Poly;
    m_Poly = NULL;
}



/** virtual function GetPosition
* @return a wxPoint, position of the first point of the outline
*/
wxPoint& ZONE_CONTAINER::GetPosition()
{
    static wxPoint pos;
    if ( m_Poly )
    {
        pos = GetCornerPosition(0);
    }
    else pos = wxPoint(0,0);
    return pos;
}

/*******************************************/
void ZONE_CONTAINER::SetNet( int anet_code )
/*******************************************/

/**
 * Set the netcode and the netname
 * if netcode >= 0, set the netname
 * if netcode < 0: keep old netname (used to set an necode error flag)
 */
{
    m_NetCode = anet_code;

    if( anet_code < 0 )
        return;

    BOARD* board = GetBoard();
    if( board )
    {
        NETINFO_ITEM* net = board->FindNet( anet_code );
        if( net )
            m_Netname = net->GetNetname();
        else
            m_Netname.Empty();
    }
    else
        m_Netname.Empty();
}


/********************************************/
bool ZONE_CONTAINER::Save( FILE* aFile ) const
/********************************************/
{
    if( GetState( DELETED ) )
        return true;

    unsigned item_pos;
    int      ret;
    unsigned corners_count = m_Poly->corner.size();
    int      outline_hatch;
    char     padoption;

    fprintf( aFile, "$CZONE_OUTLINE\n" );

    // Save the outline main info
    ret = fprintf( aFile, "ZInfo %8.8lX %d \"%s\"\n",
        m_TimeStamp, m_NetCode,
        CONV_TO_UTF8( m_Netname ) );
    if( ret < 3 )
        return false;

    // Save the ouline layer info
    ret = fprintf( aFile, "ZLayer %d\n", m_Layer );
    if( ret < 1 )
        return false;

    // Save the ouline aux info
    switch( m_Poly->GetHatchStyle() )
    {
    default:
    case CPolyLine::NO_HATCH:
        outline_hatch = 'N';
        break;

    case CPolyLine::DIAGONAL_EDGE:
        outline_hatch = 'E';
        break;

    case CPolyLine::DIAGONAL_FULL:
        outline_hatch = 'F';
        break;
    }

    ret = fprintf( aFile, "ZAux %d %c\n", corners_count, outline_hatch );
    if( ret < 2 )
        return false;

    // Save pad option and clearance
    switch( m_PadOption )
    {
    default:
    case PAD_IN_ZONE:
        padoption = 'I';
        break;

    case THERMAL_PAD:
        padoption = 'T';
        break;

    case PAD_NOT_IN_ZONE:
        padoption = 'X';
        break;
    }

    ret = fprintf( aFile, "ZClearance %d %c\n", m_ZoneClearance, padoption );
    if( ret < 2 )
        return false;

    ret = fprintf( aFile, "ZMinThickness %d\n", m_ZoneMinThickness );
    if( ret < 1 )
        return false;

    ret = fprintf( aFile, "ZOptions %d %d %c %d %d\n", m_FillMode, m_ArcToSegmentsCount,
        m_IsFilled ? 'S' : 'F', m_ThermalReliefGapValue, m_ThermalReliefCopperBridgeValue );
    if( ret < 3 )
        return false;


    // Save the corner list
    for( item_pos = 0; item_pos < corners_count; item_pos++ )
    {
        ret = fprintf( aFile, "ZCorner %d %d %d\n",
            m_Poly->corner[item_pos].x, m_Poly->corner[item_pos].y,
            m_Poly->corner[item_pos].end_contour );
        if( ret < 3 )
            return false;
    }

    // Save the PolysList
    if( m_FilledPolysList.size() )
    {
        fprintf( aFile, "$POLYSCORNERS\n" );
        for( unsigned ii = 0; ii < m_FilledPolysList.size(); ii++ )
        {
            const CPolyPt* corner = &m_FilledPolysList[ii];
            ret = fprintf( aFile, "%d %d %d %d\n", corner->x, corner->y, corner->end_contour,  corner->utility );
            if( ret < 4 )
                return false;
        }

        fprintf( aFile, "$endPOLYSCORNERS\n" );
    }

    // Save the filling segments list
    if( m_FillSegmList.size() )
    {
        fprintf( aFile, "$FILLSEGMENTS\n" );
        for( unsigned ii = 0; ii < m_FillSegmList.size(); ii++ )
        {
            ret = fprintf( aFile, "%d %d %d %d\n",
                m_FillSegmList[ii].m_Start.x, m_FillSegmList[ii].m_Start.y,
                m_FillSegmList[ii].m_End.x, m_FillSegmList[ii].m_End.y );
            if( ret < 4 )
                return false;
        }

        fprintf( aFile, "$endFILLSEGMENTS\n" );
    }
    fprintf( aFile, "$endCZONE_OUTLINE\n" );

    return true;
}


/**********************************************************/
int ZONE_CONTAINER::ReadDescr( FILE* aFile, int* aLineNum )
/**********************************************************/

/** Function ReadDescr
 * @param aFile = opened file
 * @param aLineNum = pointer on a line number counter (can be NULL or missing)
 * @return 1 if ok or 0
 */
{
    char Line[1024], * text;
    char netname_buffer[1024];
    int  ret;
    int  n_corner_item = 0;
    int  outline_hatch = CPolyLine::NO_HATCH;
    bool error = false, has_corner = false;

    netname_buffer[0] = 0;
    while( GetLine( aFile, Line, aLineNum, sizeof(Line) - 1 ) != NULL )
    {
        if( strnicmp( Line, "ZCorner", 7 ) == 0 ) // new corner found
        {
            int x;
            int y;
            int flag;

            text = Line + 7;
            ret  = sscanf( text, "%d %d %d", &x, &y, &flag );
            if( ret < 3 )
                error = true;
            else
            {
                if( !has_corner )
                    m_Poly->Start( m_Layer, x, y, outline_hatch );
                else
                    AppendCorner( wxPoint( x, y ) );

                has_corner = true;
                if( flag )
                    m_Poly->Close();
            }
        }
        else if( strnicmp( Line, "ZInfo", 5 ) == 0 )   // general info found
        {
            int ts;
            int netcode;

            text = Line + 5;
            ret  = sscanf( text, "%X %d %s", &ts, &netcode, netname_buffer );
            if( ret < 3 )
                error = true;
            else
            {
                m_TimeStamp = ts;
                m_NetCode   = netcode;
                ReadDelimitedText( netname_buffer, netname_buffer, 1024 );
                m_Netname = CONV_FROM_UTF8( netname_buffer );
            }
        }
        else if( strnicmp( Line, "ZLayer", 6 ) == 0 )  // layer found
        {
            int x;

            text = Line + 6;
            ret  = sscanf( text, "%d", &x );
            if( ret < 1 )
                error = true;
            else
                m_Layer = x;
        }
        else if( strnicmp( Line, "ZAux", 4 ) == 0 )    // aux info found
        {
            int  x;
            char hopt[10];

            text = Line + 4;
            ret  = sscanf( text, "%d %c", &x, hopt );
            if( ret < 2 )
                error = true;
            else
            {
                n_corner_item = x;

                switch( hopt[0] )
                {
                case 'n':
                case 'N':
                    outline_hatch = CPolyLine::NO_HATCH;
                    break;

                case 'e':
                case 'E':
                    outline_hatch = CPolyLine::DIAGONAL_EDGE;
                    break;

                case 'f':
                case 'F':
                    outline_hatch = CPolyLine::DIAGONAL_FULL;
                    break;
                }
            }
            /* Set hatch mode later, after reading outlines corners data */
        }
        else if( strnicmp( Line, "ZOptions", 8 ) == 0 )    // Options info found
        {
            int  fillmode = 1;
            int  arcsegmentcount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;
            char fillstate = 'F';
            text = Line + 8;
            ret  = sscanf( text, "%d %d %c %d %d", &fillmode, &arcsegmentcount, &fillstate,
                &m_ThermalReliefGapValue, &m_ThermalReliefCopperBridgeValue );

            if( ret < 1 )  // Must find 1 or more args.
                return false;
            else
                m_FillMode = fillmode ? 1 : 0;

            if( arcsegmentcount >= ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF )
                m_ArcToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF;

            m_IsFilled = fillstate == 'F' ? true : false;
        }
        else if( strnicmp( Line, "ZClearance", 10 ) == 0 )    // Clearence and pad options info found
        {
            int  clearance = 200;
            char padoption;
            text = Line + 10;
            ret  = sscanf( text, "%d %1c", &clearance, &padoption );
            if( ret < 2 )
                error = true;
            else
            {
                m_ZoneClearance = clearance;

                switch( padoption )
                {
                case 'i':
                case 'I':
                    m_PadOption = PAD_IN_ZONE;
                    break;

                case 't':
                case 'T':
                    m_PadOption = THERMAL_PAD;
                    break;

                case 'x':
                case 'X':
                    m_PadOption = PAD_NOT_IN_ZONE;
                    break;
                }
            }
        }

        else if( strnicmp( Line, "ZMinThickness", 13 ) == 0 )    // Min Thickness info found
        {
            int thickness;
            text = Line + 13;
            ret  = sscanf( text, "%d", &thickness );
            if( ret < 1 )
                error = true;
            else
               m_ZoneMinThickness = thickness;
        }

        else if( strnicmp( Line, "$POLYSCORNERS", 13 ) == 0  )  // Read the PolysList (polygons used for fill areas in the zone)
        {
            while( GetLine( aFile, Line, aLineNum, sizeof(Line) - 1 ) != NULL )
            {
                if( strnicmp( Line, "$endPOLYSCORNERS", 4 ) == 0  )
                    break;
                CPolyPt corner;
                int     end_contour, utility;
                utility = 0;
                ret = sscanf( Line, "%d %d %d %d", &corner.x, &corner.y, &end_contour, &utility );
                if( ret < 4 )
                    return false;
                corner.end_contour = end_contour ? true : false;
                corner.utility = utility;
                m_FilledPolysList.push_back( corner );
            }
        }

        else if( strnicmp( Line, "$FILLSEGMENTS", 13) == 0  )
        {
            SEGMENT segm;
            while( GetLine( aFile, Line, aLineNum, sizeof(Line) - 1 ) != NULL )
            {
                if( strnicmp( Line, "$endFILLSEGMENTS", 4 ) == 0  )
                    break;
                ret = sscanf( Line, "%d %d %d %d", &segm.m_Start.x, &segm.m_Start.y, &segm.m_End.x, &segm.m_End.y );
                if( ret < 4 )
                    return false;
                m_FillSegmList.push_back( segm );
            }
        }
        else if( strnicmp( Line, "$end", 4 ) == 0 )    // end of description
        {
            break;
        }
    }

    if( !IsOnCopperLayer() )
    {
        m_FillMode = 0;
        SetNet( 0 );
    }

    /* Set hatch here, when outlines corners are read */
    m_Poly->SetHatch( outline_hatch );

    return error ? 0 : 1;
}


/****************************************************************************************************/
void ZONE_CONTAINER::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode, const wxPoint& offset )
/****************************************************************************************************/

/** Function Draw
 * @param panel = current Draw Panel
 * @param DC = current Device Context
 * @param offset = Draw offset (usually wxPoint(0,0))
 * @param draw_mode = draw mode: OR, XOR ..
 */
{
    if( DC == NULL )
        return;

    wxPoint seg_start, seg_end;
    int     curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    BOARD * brd =  GetBoard( );
    int     color = brd->GetLayerColor(m_Layer);

    if( brd->IsLayerVisible( m_Layer ) == false &&
        ( color & HIGHT_LIGHT_FLAG ) != HIGHT_LIGHT_FLAG )
        return;

    GRSetDrawMode( DC, draw_mode );

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }

    if( draw_mode & GR_SURBRILL )
    {
        if( draw_mode & GR_AND )
            color &= ~HIGHT_LIGHT_FLAG;
        else
            color |= HIGHT_LIGHT_FLAG;
    }
    if( color & HIGHT_LIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    SetAlpha( &color, 150 );

    // draw the lines
    int i_start_contour = 0;
    wxPoint lines[( GetNumCorners()*2)+2];
    for( int ic = 0; ic < GetNumCorners(); ic++ )
    {
        seg_start = GetCornerPosition( ic ) + offset;
        if( m_Poly->corner[ic].end_contour == FALSE && ic < GetNumCorners() - 1 )
        {
            seg_end = GetCornerPosition( ic + 1 ) + offset;
        }
        else
        {
            seg_end = GetCornerPosition( i_start_contour ) + offset;
            i_start_contour = ic + 1;
        }
        lines[ic*2].x =  seg_start.x;
        lines[ic*2].y =  seg_start.y;
        lines[ic*2+1].x =  seg_start.x;
        lines[ic*2+1].y =  seg_start.y;
    }
   GRLineArray(&panel->m_ClipBox, DC, lines, GetNumCorners(), 0, color);

    // draw hatches
    wxPoint hatches[(m_Poly->m_HatchLines.size() *2)+2];
    for( unsigned ic = 0; ic < m_Poly->m_HatchLines.size(); ic++ )
    {
        hatches[ic*2].x =  m_Poly->m_HatchLines[ic].xi + offset.x;
        hatches[ic*2].y =  m_Poly->m_HatchLines[ic].yi + offset.y;
        hatches[ic*2+1].x =  m_Poly->m_HatchLines[ic].xf + offset.x;
        hatches[ic*2+1].y =  m_Poly->m_HatchLines[ic].yf + offset.y;
    }
    GRLineArray(&panel->m_ClipBox, DC, hatches, m_Poly->m_HatchLines.size(), 0, color );
}


/************************************************************************************/
void ZONE_CONTAINER::DrawFilledArea( WinEDA_DrawPanel* panel,
                                     wxDC* DC, int aDrawMode, const wxPoint& offset )
/************************************************************************************/

/**
 * Function DrawDrawFilledArea
 * Draws the filled areas for this zone (polygon list .m_FilledPolysList)
 * @param panel = current Draw Panel
 * @param DC = current Device Context
 * @param offset = Draw offset (usually wxPoint(0,0))
 * @param aDrawMode = GR_OR, GR_XOR, GR_COPY ..
 */
{
    static vector <char> CornersTypeBuffer;
    static vector <wxPoint> CornersBuffer;

    // outline_mode is false to show filled polys,
    // and true to show polygons outlines only (test and debug purposes)
    bool outline_mode = DisplayOpt.DisplayZonesMode == 2 ? true : false;

    if( DC == NULL )
        return;

    if( DisplayOpt.DisplayZonesMode == 1 )     // Do not show filled areas
        return;

    if( m_FilledPolysList.size() == 0 )  // Nothing to draw
        return;

    BOARD * brd =  GetBoard( );
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    int color = brd->GetLayerColor(m_Layer);

    if( brd->IsLayerVisible( m_Layer ) == false && ( color & HIGHT_LIGHT_FLAG ) != HIGHT_LIGHT_FLAG )
        return;

    GRSetDrawMode( DC, aDrawMode );

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }

    if( aDrawMode & GR_SURBRILL )
    {
        if( aDrawMode & GR_AND )
            color &= ~HIGHT_LIGHT_FLAG;
        else
            color |= HIGHT_LIGHT_FLAG;
    }
    if( color & HIGHT_LIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    SetAlpha( &color, 150 );

    CornersTypeBuffer.clear();
    CornersBuffer.clear();

    // Draw all filled areas
    int imax = m_FilledPolysList.size() - 1;
    for( int ic = 0; ic <= imax; ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];

        wxPoint coord( corner->x + offset.x, corner->y + offset.y );

        CornersBuffer.push_back(coord);

        CornersTypeBuffer.push_back((char) corner->utility);

        if( (corner->end_contour) || (ic == imax) ) // the last corner of a filled area is found: draw it
        {
            /* Draw the current filled area: draw segments ouline first
             * Curiously, draw segments ouline first and after draw filled polygons
             * with oulines thickness = 0 is a faster than
             * just draw filled polygons but with oulines thickness = m_ZoneMinThickness
             * So DO NOT use draw filled polygons with oulines having a thickness  > 0
             * Note: Extra segments ( added by kbool to joint holes with external outline) are not drawn
             */
            {
                // Draw outlines:
                if( (m_ZoneMinThickness > 1) || outline_mode )
                {
                    int ilim = CornersBuffer.size()-1;
                    for(  int is = 0, ie = ilim; is <= ilim; ie = is, is++ )
                    {
                        int x0 = CornersBuffer[is].x;
                        int y0 = CornersBuffer[is].y;
                        int x1 = CornersBuffer[ie].x;
                        int y1 = CornersBuffer[ie].y;

                        if( CornersTypeBuffer[ie] == 0 )   // Draw only basic outlines, not extra segments
                        {
                            if( !DisplayOpt.DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
                                GRCSegm( &panel->m_ClipBox, DC,
                                        x0, y0, x1 , y1,
                                        m_ZoneMinThickness, color );
                            else
                                GRFillCSegm( &panel->m_ClipBox, DC,
                                        x0, y0, x1 , y1,
                                        m_ZoneMinThickness, color );
                        }
                    }
                }

                // Draw areas:
                if( m_FillMode==0  && !outline_mode )
                    GRPoly( &panel->m_ClipBox, DC, CornersBuffer.size(), &CornersBuffer[0],
                        true, 0, color, color );
            }
            CornersTypeBuffer.clear();
            CornersBuffer.clear();
        }
    }

    if( m_FillMode == 1  && !outline_mode )     // filled with segments
    {
        for( unsigned ic = 0; ic < m_FillSegmList.size(); ic++ )
        {
            wxPoint start =  m_FillSegmList[ic].m_Start + offset;
            wxPoint end =  m_FillSegmList[ic].m_End + offset;
            if( !DisplayOpt.DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
                GRCSegm( &panel->m_ClipBox, DC, start.x, start.y, end.x, end.y, m_ZoneMinThickness, color );
            else
                GRFillCSegm( &panel->m_ClipBox, DC, start.x, start.y, end.x, end.y, m_ZoneMinThickness, color );
        }
    }
}


/****************************************/
EDA_Rect ZONE_CONTAINER::GetBoundingBox()
/****************************************/
{
    const int PRELOAD = 0x7FFFFFFF;     // Biggest integer (32 bits)

    int       ymax = -PRELOAD;
    int       ymin = PRELOAD;
    int       xmin = PRELOAD;
    int       xmax = -PRELOAD;

    int       count = GetNumCorners();

    for( int i = 0; i<count;  ++i )
    {
        wxPoint corner = GetCornerPosition( i );

        ymax = MAX( ymax, corner.y );
        xmax = MAX( xmax, corner.x );
        ymin = MIN( ymin, corner.y );
        xmin = MIN( xmin, corner.x );
    }

    EDA_Rect  ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}


/**********************************************************************************************/
void ZONE_CONTAINER::DrawWhileCreateOutline( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode )
/***********************************************************************************************/

/**
 * Function DrawWhileCreateOutline
 * Draws the zone outline when ir is created.
 * The moving edges (last segment and the closing edge segment) are in XOR graphic mode,
 * old segment in OR graphic mode
 * The closing edge has its owm shape
 * @param panel = current Draw Panel
 * @param DC = current Device Context
 * @param draw_mode = draw mode: OR, XOR ..
 */
{
    int     current_gr_mode  = draw_mode;
    bool    is_close_segment = false;
    wxPoint seg_start, seg_end;

    if( DC == NULL )
        return;
    int     curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    BOARD * brd =  GetBoard( );
    int     color = brd->GetLayerColor(m_Layer) & MASKCOLOR;

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }


    // draw the lines
    wxPoint start_contour_pos = GetCornerPosition( 0 );
    int     icmax = GetNumCorners() - 1;
    for( int ic = 0; ic <= icmax; ic++ )
    {
        int xi = GetCornerPosition( ic ).x;
        int yi = GetCornerPosition( ic ).y;
        int xf, yf;
        if( m_Poly->corner[ic].end_contour == FALSE && ic < icmax )
        {
            is_close_segment = false;
            xf = GetCornerPosition( ic + 1 ).x;
            yf = GetCornerPosition( ic + 1 ).y;
            if( (m_Poly->corner[ic + 1].end_contour) || (ic == icmax - 1) )
                current_gr_mode = GR_XOR;
            else
                current_gr_mode = draw_mode;
        }
        else    // Draw the line from last corner to the first corner of the current coutour
        {
            is_close_segment = true;
            current_gr_mode  = GR_XOR;
            xf = start_contour_pos.x;
            yf = start_contour_pos.y;

            // Prepare the next contour for drawing, if exists
            if( ic < icmax )
                start_contour_pos = GetCornerPosition( ic + 1 );
        }
        GRSetDrawMode( DC, current_gr_mode );
        if( is_close_segment )
            GRLine( &panel->m_ClipBox, DC, xi, yi, xf, yf, 0, WHITE );
        else
            GRLine( &panel->m_ClipBox, DC, xi, yi, xf, yf, 0, color );
    }
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 * return true if refPos is near a corner or an edge
 */
bool ZONE_CONTAINER::HitTest( const wxPoint& refPos )
{
    if( HitTestForCorner( refPos ) >= 0 )
        return true;
    if( HitTestForEdge( refPos ) >= 0 )
        return true;

    return false;
}


/**
 * Function HitTestForCorner
 * tests if the given wxPoint near a corner, or near the segment define by 2 corners.
 * Choose the nearest corner
 * "near" means CORNER_MIN_DIST_IN_PIXELS pixels
 * @return -1 if none, corner index in .corner <vector>
 * @param refPos : A wxPoint to test
 */
int ZONE_CONTAINER::HitTestForCorner( const wxPoint& refPos )
{
    #define CORNER_MIN_DIST 500     // distance (in internal units) to detect a corner in a zone outline
    int      dist, min_dist;
    unsigned item_pos, lim;
    lim = m_Poly->corner.size();
    m_CornerSelection = -1;

    min_dist = CORNER_MIN_DIST;
    for( item_pos = 0; item_pos < lim; item_pos++ )
    {
        dist = abs( m_Poly->corner[item_pos].x - refPos.x ) + abs(
            m_Poly->corner[item_pos].y - refPos.y );
        if( dist <= min_dist )
        {
            m_CornerSelection = item_pos;
            min_dist = dist;
        }
    }

    if( m_CornerSelection >= 0 )
        return item_pos;

    return -1;
}


/**
 * Function HitTestForEdge
 * tests if the given wxPoint near a corner, or near the segment define by 2 corners.
 * choose the nearest segment
 * "near" means EDGE_MIN_DIST_IN_PIXELS pixels
 * @return -1 if none,  or index of the starting corner in .corner <vector>
 * @param refPos : A wxPoint to test
 */
int ZONE_CONTAINER::HitTestForEdge( const wxPoint& refPos )
{
    #define EDGE_MIN_DIST 200   // distance (in internal units) to detect a zone outline
    int      dist, min_dist;
    unsigned item_pos, lim;
    lim = m_Poly->corner.size();

    /* Test for an entire segment */
    unsigned first_corner_pos = 0, end_segm;
    m_CornerSelection = -1;
    min_dist = EDGE_MIN_DIST;

    for( item_pos = 0; item_pos < lim; item_pos++ )
    {
        end_segm = item_pos + 1;

        /* the last corner of the current outline is tested
         * the last segment of the current outline starts at current corner, and ends
         * at the first corner of the outline
         */
        if( m_Poly->corner[item_pos].end_contour || end_segm >= lim )
        {
            unsigned tmp = first_corner_pos;
            first_corner_pos = end_segm;    // first_corner_pos is now the beginning of the next outline
            end_segm = tmp;                 // end_segm is the beginning of the current outline
        }

        /* test the dist between segment and ref point */
        dist = (int) GetPointToLineSegmentDistance( refPos.x,
            refPos.y,
            m_Poly->corner[item_pos].x,
            m_Poly->corner[item_pos].y,
            m_Poly->corner[end_segm].x,
            m_Poly->corner[end_segm].y );
        if( dist <= min_dist )
        {
            m_CornerSelection = item_pos;
            min_dist = dist;
        }
    }

    if( m_CornerSelection >= 0 )
        return item_pos;

    return -1;
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_Rect contains the bounds of this object.
 * @param refArea : the given EDA_Rect
 * @return bool - true if a hit, else false
 */
bool ZONE_CONTAINER::HitTest( EDA_Rect& refArea )
{
    bool  is_out_of_box = false;

    CRect rect = m_Poly->GetCornerBounds();

    if( rect.left < refArea.GetX() )
        is_out_of_box = true;
    if( rect.top < refArea.GetY() )
        is_out_of_box = true;
    if( rect.right > refArea.GetRight() )
        is_out_of_box = true;
    if( rect.bottom > refArea.GetBottom() )
        is_out_of_box = true;

    return is_out_of_box ? false : true;
}

/**
 * Function HitTestFilledArea
 * tests if the given wxPoint is within the bounds of a filled area of this zone.
 * @param aRefPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool ZONE_CONTAINER::HitTestFilledArea( const wxPoint& aRefPos )
{
    unsigned indexstart = 0, indexend;
    bool     inside  = false;
    for( indexend = 0; indexend < m_FilledPolysList.size(); indexend++ )
    {
        if( m_FilledPolysList[indexend].end_contour )       // end of a filled sub-area found
        {
            if( TestPointInsidePolygon( m_FilledPolysList, indexstart, indexend, aRefPos.x, aRefPos.y ) )
            {
                inside = true;
                break;
            }
        }
    }
    return inside;
}



/************************************************************/
void ZONE_CONTAINER::DisplayInfo( WinEDA_DrawFrame* frame )
/************************************************************/
{
    wxString msg;

    BOARD*   board = (BOARD*) m_Parent;

    wxASSERT( board );

    frame->ClearMsgPanel();

    msg = _( "Zone Outline" );

    int ncont = m_Poly->GetContour( m_CornerSelection );
    if( ncont )
        msg << wxT( " " ) << _( "(Cutout)" );

    frame->AppendMsgPanel( _( "Type" ), msg, DARKCYAN );

    if( IsOnCopperLayer() )
    {
        if( GetNet() >= 0 )
        {
            NETINFO_ITEM* equipot = ( (WinEDA_BasePcbFrame*) frame )->GetBoard()->FindNet( GetNet() );

            if( equipot )
                msg = equipot->GetNetname();
            else
                msg = wxT( "<noname>" );
        }
        else // a netcode < 0 is an error
        {
            msg = wxT( " [" );
            msg << m_Netname + wxT( "]" );
            msg << wxT( " <" ) << _( "Not Found" ) << wxT( ">" );
        }

        frame->AppendMsgPanel( _( "NetName" ), msg, RED );
    }
    else
        frame->AppendMsgPanel( _( "Non Copper Zone" ), wxEmptyString, RED );

    /* Display net code : (usefull in test or debug) */
    msg.Printf( wxT( "%d" ), GetNet() );
    frame->AppendMsgPanel( _( "NetCode" ), msg, RED );

    msg = board->GetLayerName( m_Layer );
    frame->AppendMsgPanel( _( "Layer" ), msg, BROWN );

    msg.Printf( wxT( "%d" ), m_Poly->corner.size() );
    frame->AppendMsgPanel( _( "Corners" ), msg, BLUE );

    if( m_FillMode )
        msg.Printf( _( "Segments" ), m_FillMode );
    else
        msg = _( "Polygons" );
    frame->AppendMsgPanel( _( "Fill mode" ), msg, BROWN );

    // Useful for statistics :
    msg.Printf( wxT( "%d" ), m_Poly->m_HatchLines.size() );
    frame->AppendMsgPanel( _( "Hatch lines" ), msg, BLUE );

    if( m_FilledPolysList.size() )
    {
        msg.Printf( wxT( "%d" ), m_FilledPolysList.size() );
        frame->AppendMsgPanel( _( "Corners in DrawList" ), msg, BLUE );
    }
}


/* Geometric transforms: */

/**
 * Function Move
 * Move the outlines
 * @param offset = moving vector
 */
void ZONE_CONTAINER::Move( const wxPoint& offset )
{
    /* move outlines */
    for( unsigned ii = 0; ii < m_Poly->corner.size(); ii++ )
    {
        SetCornerPosition( ii, GetCornerPosition( ii ) + offset );
    }

    m_Poly->Hatch();

    /* move filled areas: */
    for( unsigned ic = 0; ic < m_FilledPolysList.size(); ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        corner->x += offset.x;
        corner->y += offset.y;
    }
    for( unsigned ic = 0; ic < m_FillSegmList.size(); ic++ )
    {
        m_FillSegmList[ic].m_Start += offset;
        m_FillSegmList[ic].m_End += offset;
    }
}


/**
 * Function MoveEdge
 * Move the outline Edge. m_CornerSelection is the start point of the outline edge
 * @param offset = moving vector
 */
void ZONE_CONTAINER::MoveEdge( const wxPoint& offset )
{
    int ii = m_CornerSelection;

    // Move the start point of the selected edge:
    SetCornerPosition( ii, GetCornerPosition( ii ) + offset );

    // Move the end point of the selected edge:
    if( m_Poly->corner[ii].end_contour || ii == GetNumCorners() - 1 )
    {
        int icont = m_Poly->GetContour( ii );
        ii = m_Poly->GetContourStart( icont );
    }
    else
        ii++;
    SetCornerPosition( ii, GetCornerPosition( ii ) + offset );

    m_Poly->Hatch();
}


/**
 * Function Rotate
 * Move the outlines
 * @param centre = rot centre
 * @param angle = in 0.1 degree
 */
void ZONE_CONTAINER::Rotate( const wxPoint& centre, int angle )
{
    wxPoint pos;
    for( unsigned ii = 0; ii < m_Poly->corner.size(); ii++ )
    {
        pos.x = m_Poly->corner[ii].x;
        pos.y = m_Poly->corner[ii].y;
        RotatePoint( &pos, centre, angle );
        m_Poly->corner[ii].x = pos.x;
        m_Poly->corner[ii].y = pos.y;
    }

    m_Poly->Hatch();

    /* rotate filled areas: */
    for( unsigned ic = 0; ic < m_FilledPolysList.size(); ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        pos.x = corner->x;
        pos.y = corner->y;
        RotatePoint( &pos, centre, angle );
        corner->x = pos.x;
        corner->y = pos.y;
    }
    for( unsigned ic = 0; ic < m_FillSegmList.size(); ic++ )
    {
        RotatePoint( &m_FillSegmList[ic].m_Start, centre, angle );
        RotatePoint( &m_FillSegmList[ic].m_End, centre, angle );
    }
}

/**
 * Function Flip
 * Flip this object, i.e. change the board side for this object
 * (like Mirror() but changes layer)
 * @param const wxPoint& aCentre - the rotation point.
 */
void ZONE_CONTAINER::Flip(const wxPoint& aCentre )
{
    Mirror( aCentre );
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
}

/**
 * Function Mirror
 * flip the outlines , relative to a given horizontal axis
 * @param mirror_ref = vertical axis position
 */
void ZONE_CONTAINER::Mirror( const wxPoint& mirror_ref )
{
    for( unsigned ii = 0; ii < m_Poly->corner.size(); ii++ )
    {
        m_Poly->corner[ii].y -= mirror_ref.y;
        NEGATE(m_Poly->corner[ii].y);
        m_Poly->corner[ii].y += mirror_ref.y;
    }

    m_Poly->Hatch();

    /* mirror filled areas: */
    for( unsigned ic = 0; ic < m_FilledPolysList.size(); ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        corner->y -= mirror_ref.y;
        NEGATE(corner->y);
        corner->y += mirror_ref.y;
    }
    for( unsigned ic = 0; ic < m_FillSegmList.size(); ic++ )
    {
        m_FillSegmList[ic].m_Start.y -= mirror_ref.y;
        NEGATE(m_FillSegmList[ic].m_Start.y);
        m_FillSegmList[ic].m_Start.y += mirror_ref.y;
        m_FillSegmList[ic].m_End.y -= mirror_ref.y;
        NEGATE(m_FillSegmList[ic].m_End.y);
        m_FillSegmList[ic].m_End.y += mirror_ref.y;
    }
}


/** Function copy
 * copy usefull data from the source.
 * flags and linked list pointers are NOT copied
 */
void ZONE_CONTAINER::Copy( ZONE_CONTAINER* src )
{
    m_Parent = src->m_Parent;
    m_Layer  = src->m_Layer;
    SetNet( src->GetNet() );
    m_TimeStamp = src->m_TimeStamp;
    m_Poly->RemoveAllContours();
    m_Poly->Copy( src->m_Poly );                // copy outlines
    m_CornerSelection = -1;                     // For corner moving, corner index to drag, or -1 if no selection
    m_ZoneClearance   = src->m_ZoneClearance;   // clearance value
    m_FillMode   = src->m_FillMode;             // Filling mode (segments/polygons)
    m_ArcToSegmentsCount   = src->m_ArcToSegmentsCount;
    m_PadOption = src->m_PadOption;
    m_ThermalReliefGapValue   = src->m_ThermalReliefGapValue;
    m_ThermalReliefCopperBridgeValue   = src->m_ThermalReliefCopperBridgeValue;
    m_Poly->m_HatchStyle = src->m_Poly->GetHatchStyle();
    m_Poly->m_HatchLines = src->m_Poly->m_HatchLines;   // Copy vector <CSegment>
    m_FilledPolysList.clear();
    m_FilledPolysList = src->m_FilledPolysList;
    m_FillSegmList.clear();
    m_FillSegmList = src->m_FillSegmList;
}

/**
 * Function SetNetNameFromNetCode
 * Find the net name corresponding to the net code.
 * @param aPcb: the current board
 * @return bool - true if net found, else false
 */
bool ZONE_CONTAINER::SetNetNameFromNetCode( void )
{
    NETINFO_ITEM* net;
    if ( m_Parent && (net = ((BOARD*)m_Parent)->FindNet( GetNet()) ) )
    {
        m_Netname = net->GetNetname();
        return true;
    }

    return false;
}
