/**********************************/
/* classes to handle copper zones */
/**********************************/

#include "fctsys.h"
#include "wxstruct.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

/************************/
/* class ZONE_CONTAINER */
/************************/

ZONE_CONTAINER::ZONE_CONTAINER( BOARD* parent ) :
    BOARD_ITEM( parent, TYPEZONE_CONTAINER )
    , CPolyLine( NULL )

{
    m_NetCode = -1;              // Net number for fast comparisons
	m_CornerSelection = -1;
}


ZONE_CONTAINER::~ZONE_CONTAINER()
{
}


/**********************/
/* Class EDGE_ZONE */
/**********************/

/* Constructor */
EDGE_ZONE::EDGE_ZONE( BOARD_ITEM* parent ) :
    DRAWSEGMENT( parent, TYPEEDGEZONE )
{
    m_Width = 2;        // a minimum for visibility, while dragging
}


/* Destructor */
EDGE_ZONE:: ~EDGE_ZONE()
{
}


/****************************************/
bool EDGE_ZONE::Save( FILE* aFile ) const
/****************************************/
{
	return true;
}

/********************************************/
bool ZONE_CONTAINER::Save( FILE* aFile ) const
/********************************************/
{
    if( GetState( DELETED ) )
        return true;

	unsigned item_pos;
	int ret;
	unsigned corners_count = corner.size();
	int outline_hatch;

	fprintf( aFile, "$CZONE_OUTLINE\n");
	// Save the outline main info
	ret = fprintf( aFile, "ZInfo %8.8lX %d \"%s\"\n",
				   m_TimeStamp, m_NetCode,
				   CONV_TO_UTF8(m_Netname) );
	if ( ret < 3 ) return false;
	// Save the ouline layer info
	ret = fprintf( aFile, "ZLayer %d\n",  m_Layer );
	if ( ret < 1 ) return false;
	// Save the ouline aux info
	switch ( m_HatchStyle )
	{
		default:
		case NO_HATCH:
			outline_hatch = 'N';
			break;
		case DIAGONAL_EDGE:
			outline_hatch = 'E';
			break;
		case DIAGONAL_FULL:
			outline_hatch = 'F';
			break;
	}

	ret = fprintf( aFile, "ZAux %d %c\n", corners_count, outline_hatch );
	if ( ret < 2 ) return false;
	// Save the corner list
	for ( item_pos = 0; item_pos < corners_count; item_pos++ )
	{
		ret = fprintf( aFile, "ZCorner %d %d %d \n",
                       corner[item_pos].x, corner[item_pos].y,
                       corner[item_pos].end_contour );
		if ( ret < 3 ) return false;
	}
	fprintf( aFile, "$endCZONE_OUTLINE\n");
	
	return true;
}


/**********************************************************/
int ZONE_CONTAINER::ReadDescr( FILE* aFile, int* aLineNum )
/**********************************************************/
/** Function ReadDescr
 * @param aFile = opened file
 * @param aLineNum = pointer on a line number counter (can be NULL or missing)
 * @return 0 if ok or NULL
 */
{
    char            Line[1024], * text;
	char netname_buffer[1024];
    int             ret;
	int n_corner_item = 0;
	int outline_hatch = NO_HATCH;
	bool error = false, has_corner = false;

	netname_buffer[0] = 0;
    while( GetLine( aFile, Line, aLineNum, sizeof(Line) - 1 ) != NULL )
    {
        if( strnicmp(Line, "ZCorner", 7 ) == 0 )	// new corner found
        {
			int x = 0, y = 0, flag = 0;
			text = Line + 7;
			ret = sscanf( text, "%d %d %d", &x, &y, &flag);
			if (ret < 3 ) error = true;
			else
			{
				if ( ! has_corner )
					Start( m_Layer, 0, 0,
						x, y, 
						outline_hatch );
				else
					AppendCorner( x, y );
				has_corner = true;
				if ( flag ) Close();
			}
		}
        if( strnicmp(Line, "ZInfo", 5 ) == 0 )	// general info found
        {
			int ts = 0, netcode = 0;
			text = Line + 5;
			ret = sscanf( text, "%X %d %s", &ts, &netcode, netname_buffer);
			if (ret < 3 ) error = true;
			else
			{
				m_TimeStamp = ts;
				m_NetCode = netcode;
				ReadDelimitedText( netname_buffer, netname_buffer, 1024 );
				m_Netname = CONV_FROM_UTF8(netname_buffer);
			}
		}
        if( strnicmp(Line, "ZLayer", 6 ) == 0 )	// layer found
        {
			int x = 0;
			text = Line + 6;
			ret = sscanf( text, "%d", &x);
			if (ret < 1 ) error = true;
			else m_Layer = x;
		}
        if( strnicmp(Line, "ZAux", 4 ) == 0 )	// aux info found
        {
			int x = 0;
			char hopt[10];
			text = Line + 4;
			ret = sscanf( text, "%d %c", &x, hopt );
			if (ret < 2 ) error = true;
			else
			{
				n_corner_item = x;
				switch ( hopt[0] )
				{
					case 'n':
					case 'N':
						outline_hatch = NO_HATCH;
						break;
					case 'e':
					case 'E':
						outline_hatch = DIAGONAL_EDGE;
						break;
					case 'f':
					case 'F':
						outline_hatch = DIAGONAL_FULL;
						break;
				}
			}
		}
        if( strnicmp(Line, "$end", 4 ) == 0 )	// end of description
        {
			break;
		}
	}
	
	return error ? 0 : 1;
}


/****************************************************************************************************/
void ZONE_CONTAINER::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int draw_mode )
/****************************************************************************************************/

/** Function Draw
 * @param panel = current Draw Panel
 * @param DC = current Device Context
 * @param offset = Draw offset (usually wxPoint(0,0))
 * @param draw_mode = draw mode: OR, XOR ..
 */
{
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    int color = g_DesignSettings.m_LayerColor[m_Layer];

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
    for( unsigned ic = 0; ic < corner.size(); ic++ )
    {
        int xi = corner[ic].x + offset.x;
        int yi = corner[ic].y + offset.y;
        int xf, yf;
        if( corner[ic].end_contour == FALSE && ic < corner.size() - 1 )
        {
            xf = corner[ic + 1].x + offset.x;
            yf = corner[ic + 1].y + offset.y;
        }
        else
        {
            xf = corner[i_start_contour].x + offset.x;
            yf = corner[i_start_contour].y + offset.y;
            i_start_contour = ic + 1;
        }
        GRLine( &panel->m_ClipBox, DC, xi, yi, xf, yf, 0, color );
    }
	
	// draw hatches
    for( unsigned ic = 0; ic < m_HatchLines.size(); ic++ )
    {
        int xi = m_HatchLines[ic].xi + offset.x;
        int yi = m_HatchLines[ic].yi + offset.y;
        int xf = m_HatchLines[ic].xf + offset.x;
        int yf =m_HatchLines[ic].yf + offset.y;
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
 * "near" means MIN_DIST_IN_PIXELS pixels
 * @return -1 if none, corner index in .corner <vector>
 * @param refPos : A wxPoint to test
 */
int ZONE_CONTAINER::HitTestForCorner( const wxPoint& refPos )
{
	#define MIN_DIST_IN_PIXELS 5
    int dist;
	unsigned item_pos, lim;
	lim = corner.size();

	// Min distance to hit = MIN_DIST_IN_PIXELS pixels :
	WinEDA_BasePcbFrame* frame = ((BOARD*)GetParent())->m_PcbFrame;
    int min_dist = frame ? frame->GetZoom() * MIN_DIST_IN_PIXELS : 3;
	
	for ( item_pos = 0; item_pos < lim; item_pos++ )
	{
		dist = abs( corner[item_pos].x - refPos.x ) + abs( corner[item_pos].y - refPos.y );
		if( dist <= min_dist )
			return item_pos;
	}

    return -1;
}

/**
 * Function HitTestForEdge
 * tests if the given wxPoint near a corner, or near the segment define by 2 corners.
 * "near" means MIN_DIST_IN_PIXELS pixels
 * @return -1 if none,  or index of the starting corner in .corner <vector>
 * @param refPos : A wxPoint to test
 */
int ZONE_CONTAINER::HitTestForEdge( const wxPoint& refPos )
{
	#define MIN_DIST_IN_PIXELS 5
    int dist;
	unsigned item_pos, lim;
	lim = corner.size();

	// Min distance to hit = MIN_DIST_IN_PIXELS pixels :
	WinEDA_BasePcbFrame* frame = ((BOARD*)GetParent())->m_PcbFrame;
    int min_dist = frame ? frame->GetZoom() * MIN_DIST_IN_PIXELS : 3;

    /* Test for an entire segment */
    unsigned first_corner_pos = 0, end_segm;

	for ( item_pos = 0; item_pos < lim; item_pos++ )
	{
		end_segm = item_pos+1;
		/* the last corner of the current outline is tested
		 * the last segment of the current outline starts at current corner, and ends
		 * at the first corner of the outline
		 */
		if( corner[item_pos].end_contour || end_segm >= lim)
		{
			unsigned tmp = first_corner_pos;
			first_corner_pos = end_segm;	// first_corner_pos is now the beginning of the next outline
			end_segm = tmp;	// end_segm is the beginning of the current outline
		}

		/* test the dist between segment and ref point */
		dist = (int) GetPointToLineSegmentDistance( refPos.x,
													refPos.y,
													corner[item_pos].x,
													corner[item_pos].y,
													corner[end_segm].x,
													corner[end_segm].y );
		if( dist <= min_dist )
			return item_pos;
	}

    return -1;
}


/************************************************************/
void ZONE_CONTAINER::Display_Infos( WinEDA_DrawFrame* frame )
/************************************************************/
{
    wxString msg;
    int      text_pos;
	
    frame->MsgPanel->EraseMsgBox();

	msg = _( "Zone Outline" );

    text_pos = 1;
    Affiche_1_Parametre( frame, text_pos, _( "Type" ), msg, DARKCYAN );

    text_pos += 15;
    EQUIPOT* equipot = ( (WinEDA_PcbFrame*) frame )->m_Pcb->FindNet( GetNet() );

	if( equipot )
		msg = equipot->m_Netname;
	else
		msg = wxT( "<noname>" );

	Affiche_1_Parametre( frame, text_pos, _( "NetName" ), msg, RED );

	/* Display net code : (usefull in test or debug) */
	text_pos += 18;
	msg.Printf( wxT( "%d" ), GetNet());
	Affiche_1_Parametre( frame, text_pos, _( "NetCode" ), msg, RED );

    text_pos += 8;
	msg = ReturnPcbLayerName( m_Layer );
    Affiche_1_Parametre( frame, text_pos, _( "Layer" ), msg, BROWN );

    text_pos += 8;
	msg.Printf( wxT( "%d" ), corner.size() );
    Affiche_1_Parametre( frame, text_pos, _( "Corners" ), msg, BLUE );

    text_pos += 8;
	msg.Printf( wxT( "%d" ), m_HatchLines.size() );
    Affiche_1_Parametre( frame, text_pos, _( "Hatch lines" ), msg, BLUE );
}
