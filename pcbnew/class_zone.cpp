/**********************************/
/* classes to handle copper zones */
/**********************************/

#include "fctsys.h"
#include "wxstruct.h"

#include "gr_basic.h"

#include "common.h"
#include "PolyLine.h"
#include "pcbnew.h"
#include "trigo.h"
#include "zones.h"


/************************/
/* class ZONE_CONTAINER */
/************************/

ZONE_CONTAINER::ZONE_CONTAINER( BOARD* parent ) :
    BOARD_ITEM( parent, TYPEZONE_CONTAINER )

{
    m_NetCode = -1;                                                             // Net number for fast comparisons
    m_CornerSelection = -1;
    utility  = 0;                                                               // flags used in polygon calculations
    utility2 = 0;                                                               // flags used in polygon calculations
    m_Poly   = new CPolyLine();                                                 // Outlines
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

    if( m_Parent )
    {
        EQUIPOT* net = ( (BOARD*) m_Parent )->FindNet( anet_code );
        if( net )
            m_Netname = net->m_Netname;
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

    ret = fprintf( aFile, "ZOptions %d %d %c %d %d\n", m_GridFillValue, m_ArcToSegmentsCount,
        m_DrawOptions ? 'S' : 'F', m_ThermalReliefGapValue, m_ThermalReliefCopperBridgeValue );
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
        for( item_pos = 0; item_pos < m_FilledPolysList.size(); item_pos++ )
        {
            const CPolyPt* corner = &m_FilledPolysList[item_pos];
            ret = fprintf( aFile, "%d %d %d\n", corner->x, corner->y, corner->end_contour );
            if( ret < 3 )
                return false;
        }

        fprintf( aFile, "$endPOLYSCORNERS\n" );
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
            int x = 0, y = 0, flag = 0;
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
        if( strnicmp( Line, "ZInfo", 5 ) == 0 )   // general info found
        {
            int ts = 0, netcode = 0;
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
        if( strnicmp( Line, "ZLayer", 6 ) == 0 )  // layer found
        {
            int x = 0;
            text = Line + 6;
            ret  = sscanf( text, "%d", &x );
            if( ret < 1 )
                error = true;
            else
                m_Layer = x;
        }
        if( strnicmp( Line, "ZAux", 4 ) == 0 )    // aux info found
        {
            int  x = 0;
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
        if( strnicmp( Line, "ZOptions", 8 ) == 0 )    // Options info found
        {
            int  gridsize = 50;
            int  arcsegmentcount = 16;
            char drawopt = 'F';
            text = Line + 8;
            ret  = sscanf( text, "%d %d %c %d %d", &gridsize, &arcsegmentcount, &drawopt,
                &m_ThermalReliefGapValue, &m_ThermalReliefCopperBridgeValue );
            if( ret < 1 )  // Must find 1 or more args.
                return false;
            else
                m_GridFillValue = gridsize;

            if( arcsegmentcount >= 32 )
                m_ArcToSegmentsCount = 32;

            if( drawopt == 'S' )  // Sketch mode for filled areas in this zone selected
                m_DrawOptions = 1;
        }
        if( strnicmp( Line, "ZClearance", 10 ) == 0 )    // Clearence and pad options info found
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

        if( strnicmp( Line, "ZMinThickness", 13 ) == 0 )    // Min Thickness info found
        {
            int thickness;
            text = Line + 13;
            ret  = sscanf( text, "%d", &thickness );
            if( ret < 1 )
                error = true;
            else
               m_ZoneMinThickness = thickness;
        }

        if( strnicmp( Line, "$POLYSCORNERS", 13 ) == 0  )  // Read the PolysList (polygons used for fill areas in the zone)
        {
            while( GetLine( aFile, Line, aLineNum, sizeof(Line) - 1 ) != NULL )
            {
                if( strnicmp( Line, "$endPOLYSCORNERS", 4 ) == 0  )
                    break;
                CPolyPt corner;
                int     itmp;
                ret = sscanf( Line, "%d %d %d", &corner.x, &corner.y, &itmp );
                if( ret < 3 )
                    return false;
                corner.end_contour = itmp ? true : false;
                m_FilledPolysList.push_back( corner );
            }
        }

        if( strnicmp( Line, "$end", 4 ) == 0 )    // end of description
        {
            break;
        }
    }

    if( !IsOnCopperLayer() )
    {
        m_GridFillValue = 0;
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
    int     color = g_DesignSettings.m_LayerColor[m_Layer];

    if( ( color & (ITEM_NOT_SHOW | HIGHT_LIGHT_FLAG) ) == ITEM_NOT_SHOW )
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

    // draw the lines
    int i_start_contour = 0;
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
        GRLine( &panel->m_ClipBox, DC, seg_start.x, seg_start.y, seg_end.x, seg_end.y, 0, color );
    }

    // draw hatches
    for( unsigned ic = 0; ic < m_Poly->m_HatchLines.size(); ic++ )
    {
        int xi = m_Poly->m_HatchLines[ic].xi + offset.x;
        int yi = m_Poly->m_HatchLines[ic].yi + offset.y;
        int xf = m_Poly->m_HatchLines[ic].xf + offset.x;
        int yf = m_Poly->m_HatchLines[ic].yf + offset.y;
        GRLine( &panel->m_ClipBox, DC, xi, yi, xf, yf, 0, color );
    }
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
    static int*     CornersBuffer     = NULL;
    static unsigned CornersBufferSize = 0;
    bool            sketch_mode = m_DrawOptions; // false to show filled polys, true to show polygons outlines only (test and debug purposes)

    if( DC == NULL )
        return;

    if( !DisplayOpt.DisplayZones )
        return;

    unsigned imax = m_FilledPolysList.size();

    if( imax == 0 )  // Nothing to draw
        return;

    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    int color = g_DesignSettings.m_LayerColor[m_Layer];

    if( ( color & (ITEM_NOT_SHOW | HIGHT_LIGHT_FLAG) ) == ITEM_NOT_SHOW )
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

    // We need a buffer to store corners coordinates:
    if( CornersBuffer == NULL )
    {
        CornersBufferSize = imax * 4;
        CornersBuffer = (int*) MyMalloc( CornersBufferSize * sizeof(int) );
    }

    if( (imax * 4) > CornersBufferSize )
    {
        CornersBufferSize = imax * 4;
        CornersBuffer = (int*) realloc( CornersBuffer, CornersBufferSize * sizeof(int) );
    }

    // Draw all filled areas
    int corners_count = 0;
    for( unsigned ic = 0, ii = 0; ic < imax; ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        CornersBuffer[ii++] = corner->x + offset.x;
        CornersBuffer[ii++] = corner->y + offset.y;
        corners_count++;
        if( corner->end_contour )
        {   // Draw the current filled area
            if( sketch_mode )
                GRClosedPoly( &panel->m_ClipBox, DC, corners_count, CornersBuffer,
                    false, 0, color, color );
            else
            {
                // Draw outlines:
                if ( m_ZoneMinThickness > 1 )
                {
                    int ilim = corners_count * 2;
                    for (  int is = 0, ie = ilim-2; is < ilim; ie = is, is+=2 )
                    {
                        int x0 = CornersBuffer[is];
                        int y0 = CornersBuffer[is+1];
                        int x1 = CornersBuffer[ie];
                        int y1 = CornersBuffer[ie+1];
                        GRFillCSegm( &panel->m_ClipBox, DC,
                                x0, y0, x1 , y1,
                                m_ZoneMinThickness, color );
                    }
                }
                // Draw areas:
                GRPoly( &panel->m_ClipBox, DC, corners_count, CornersBuffer,
                    true, 0, color, color );
            }
            corners_count = 0;
            ii = 0;
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
    int     color = g_DesignSettings.m_LayerColor[m_Layer & 31] & MASKCOLOR;

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
void ZONE_CONTAINER::Display_Infos( WinEDA_DrawFrame* frame )
/************************************************************/
{
    wxString msg;
    int      text_pos;

    BOARD*   board = (BOARD*) m_Parent;

    wxASSERT( board );

    frame->MsgPanel->EraseMsgBox();

    msg = _( "Zone Outline" );

    int ncont = m_Poly->GetContour( m_CornerSelection );
    if( ncont )
        msg << wxT( " " ) << _( "(Cutout)" );

    text_pos = 1;
    Affiche_1_Parametre( frame, text_pos, _( "Type" ), msg, DARKCYAN );

    text_pos += 15;

    if( IsOnCopperLayer() )
    {
        if( GetNet() >= 0 )
        {
            EQUIPOT* equipot = ( (WinEDA_PcbFrame*) frame )->m_Pcb->FindNet( GetNet() );

            if( equipot )
                msg = equipot->m_Netname;
            else
                msg = wxT( "<noname>" );
        }
        else // a netcode < 0 is an error
        {
            msg = wxT( " [" );
            msg << m_Netname + wxT( "]" );
            msg << wxT( " <" ) << _( "Not Found" ) << wxT( ">" );
        }

        Affiche_1_Parametre( frame, text_pos, _( "NetName" ), msg, RED );
    }
    else
        Affiche_1_Parametre( frame, text_pos, _( "Non Copper Zone" ), wxEmptyString, RED );

    /* Display net code : (usefull in test or debug) */
    text_pos += 18;
    msg.Printf( wxT( "%d" ), GetNet() );
    Affiche_1_Parametre( frame, text_pos, _( "NetCode" ), msg, RED );

    text_pos += 6;
    msg = board->GetLayerName( m_Layer );
    Affiche_1_Parametre( frame, text_pos, _( "Layer" ), msg, BROWN );

    text_pos += 8;
    msg.Printf( wxT( "%d" ), m_Poly->corner.size() );
    Affiche_1_Parametre( frame, text_pos, _( "Corners" ), msg, BLUE );

    text_pos += 6;
    if( m_GridFillValue )
        msg.Printf( wxT( "%d" ), m_GridFillValue );
    else
        msg = _( "No Grid" );
    Affiche_1_Parametre( frame, text_pos, _( "Fill Grid" ), msg, BROWN );

    // Useful for statistics :
    text_pos += 9;
    msg.Printf( wxT( "%d" ), m_Poly->m_HatchLines.size() );
    Affiche_1_Parametre( frame, text_pos, _( "Hatch lines" ), msg, BLUE );

    if( m_FilledPolysList.size() )
    {
        text_pos += 9;
        msg.Printf( wxT( "%d" ), m_FilledPolysList.size() );
        Affiche_1_Parametre( frame, text_pos, _( "Corners in DrawList" ), msg, BLUE );
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
    for( unsigned ii = 0; ii < m_Poly->corner.size(); ii++ )
    {
        SetCornerPosition( ii, GetCornerPosition( ii ) + offset );
    }

    m_Poly->Hatch();
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
    for( unsigned ii = 0; ii < m_Poly->corner.size(); ii++ )
    {
        wxPoint pos;
        pos.x = m_Poly->corner[ii].x;
        pos.y = m_Poly->corner[ii].y;
        RotatePoint( &pos, centre, angle );
        m_Poly->corner[ii].x = pos.x;
        m_Poly->corner[ii].y = pos.y;
    }

    m_Poly->Hatch();
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
        m_Poly->corner[ii].y  = -m_Poly->corner[ii].y;
        m_Poly->corner[ii].y += mirror_ref.y;
    }

    m_Poly->Hatch();
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
    m_Poly->Copy( src->m_Poly );                // copy outlines
    m_CornerSelection = -1;                     // For corner moving, corner index to drag, or -1 if no selection
    m_ZoneClearance   = src->m_ZoneClearance;   // clearance value
    m_GridFillValue   = src->m_GridFillValue;   // Grid used for filling
    m_PadOption = src->m_PadOption;
    m_Poly->SetHatch( src->m_Poly->GetHatchStyle() );
}

/**
 * Function SetNetNameFromNetCode
 * Fin the nat name corresponding to the net code.
 * @param aPcb: the curren board
 * @return bool - true if net found, else false
 */
bool ZONE_CONTAINER::SetNetNameFromNetCode( void )
{
    EQUIPOT* net;
    if ( m_Parent && (net = ((BOARD*)m_Parent)->FindNet( GetNet()) ) )
    {
        m_Netname = net->m_Netname;
        return true;
    }

    return false;
}
