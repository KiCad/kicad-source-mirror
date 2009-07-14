/******************************************/
/* Kicad: Common plot Postscript Routines */
/******************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "plot_common.h"
#include "worksheet.h"
#include "macros.h"
#include "class_base_screen.h"
#include "drawtxt.h"

/**************************************************************************/
void WinEDA_DrawFrame::PlotWorkSheet( Plotter *plotter, BASE_SCREEN* screen )
/**************************************************************************/

/* Plot sheet references
  * margin is in mils (1/1000 inch)
 */
{
#define WSTEXTSIZE 50   // Text size in mils
    Ki_PageDescr*     Sheet = screen->m_CurrentSheetDesc;
    int               xg, yg, ipas, gxpas, gypas;
    wxSize            PageSize;
    wxPoint           pos, ref;
    EDA_Colors        color;
    int               conv_unit = screen->GetInternalUnits() / 1000; /* Scale to convert dimension in 1/1000 in into internal units
                                                      * (1/1000 inc for EESchema, 1/10000 for pcbnew */
    wxString          msg;
    wxSize            text_size;
    int               UpperLimit = VARIABLE_BLOCK_START_POSITION;
    bool italic = false;
    bool bold = false;
    bool thickness = 0; //@todo : use current pen

    color = BLACK;
    plotter->set_color(color);

    PageSize.x = Sheet->m_Size.x;
    PageSize.y = Sheet->m_Size.y;

    /* trace de la bordure */
    ref.x = Sheet->m_LeftMargin * conv_unit;
    ref.y = Sheet->m_TopMargin * conv_unit;                     /* Upper left corner */
    xg    = (PageSize.x - Sheet->m_RightMargin) * conv_unit;
    yg    = (PageSize.y - Sheet->m_BottomMargin) * conv_unit;   /* lower right corner */

#if defined(KICAD_GOST)
    plotter->move_to( ref );
    pos.x = xg; pos.y = ref.y;
    plotter->line_to( pos );
    pos.x = xg; pos.y = yg;
    plotter->line_to( pos );
    pos.x = ref.x; pos.y = yg;
    plotter->line_to( pos );
    plotter->finish_to( ref );
#else
    for( unsigned ii = 0; ii < 2; ii++ )
    {
        plotter->move_to( ref );
        pos.x = xg; pos.y = ref.y;
        plotter->line_to( pos );
        pos.x = xg; pos.y = yg;
        plotter->line_to( pos );
        pos.x = ref.x; pos.y = yg;
        plotter->line_to( pos );
        plotter->finish_to( ref );
        ref.x += GRID_REF_W * conv_unit; 
	ref.y += GRID_REF_W * conv_unit;
        xg -= GRID_REF_W * conv_unit; 
	yg -= GRID_REF_W * conv_unit;
    }
#endif

    /* trace des reperes */
    text_size.x = WSTEXTSIZE * conv_unit;
    text_size.y = WSTEXTSIZE * conv_unit;

    ref.x = Sheet->m_LeftMargin;
    ref.y = Sheet->m_TopMargin;                     /* Upper left corner in 1/1000 inch */
    xg    = (PageSize.x - Sheet->m_RightMargin);
    yg    = (PageSize.y - Sheet->m_BottomMargin);   /* lower right corner in 1/1000 inch */

#if defined(KICAD_GOST)
    for ( Ki_WorkSheetData* WsItem = &WS_Segm1_LU; 
	    WsItem != NULL; 
	    WsItem = WsItem->Pnext )
    {
	pos.x = (ref.x - WsItem->m_Posx) * conv_unit;
	pos.y = (yg - WsItem->m_Posy) * conv_unit;
	msg.Empty();
	switch( WsItem->m_Type )
	{
	    case WS_CADRE:
		break;
	    case WS_PODPIS_LU:
		if(WsItem->m_Legende) msg = WsItem->m_Legende;
		plotter->text( pos, color,
				msg, TEXT_ORIENT_VERT, text_size,
                        	GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM,
                            thickness, italic, false );
        	break;
    	    case WS_SEGMENT_LU:
    		plotter->move_to(pos);
    		pos.x = (ref.x - WsItem->m_Endx) * conv_unit;
    		pos.y = (yg - WsItem->m_Endy) * conv_unit;
		plotter->finish_to(pos);
    		break;
	}
    }
    for ( Ki_WorkSheetData* WsItem = &WS_Segm1_LT; 
	    WsItem != NULL; 
	    WsItem = WsItem->Pnext )
    {
	pos.x = (ref.x + WsItem->m_Posx) * conv_unit;
	pos.y = (ref.y + WsItem->m_Posy) * conv_unit;
	msg.Empty();
	switch( WsItem->m_Type )
	{
    	    case WS_SEGMENT_LT:
    		plotter->move_to(pos);
        	pos.x = (ref.x + WsItem->m_Endx) * conv_unit;
        	pos.y = (ref.y + WsItem->m_Endy) * conv_unit;
        	plotter->finish_to(pos);
        	break;
	}
    }
#else

    /* Trace des reperes selon l'axe X */
    ipas  = (xg - ref.x) / PAS_REF;
    gxpas = ( xg - ref.x) / ipas;
    for(int ii = ref.x + gxpas, jj = 1; ipas > 0; ii += gxpas, jj++, ipas-- )
    {
        msg.Empty(); msg << jj;
        if( ii < xg - PAS_REF / 2 )
        {
            pos.x = ii * conv_unit; pos.y = ref.y * conv_unit;
            plotter->move_to(pos);
            pos.x = ii * conv_unit; pos.y = (ref.y + GRID_REF_W) * conv_unit;
            plotter->finish_to(pos);
        }
        pos.x = (ii - gxpas / 2) * conv_unit;
        pos.y = (ref.y + GRID_REF_W / 2) * conv_unit;
        plotter->text( pos, color,
            msg, TEXT_ORIENT_HORIZ, text_size,
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
            thickness, italic, false );

        if( ii < xg - PAS_REF / 2 )
        {
            pos.x = ii * conv_unit; pos.y = yg * conv_unit;
            plotter->move_to( pos);
            pos.x = ii * conv_unit; pos.y = (yg - GRID_REF_W) * conv_unit;
            plotter->finish_to(pos);
        }
        pos.x = (ii - gxpas / 2) * conv_unit;
        pos.y = (yg - GRID_REF_W / 2) * conv_unit;
        plotter->text( pos, color,
            msg, TEXT_ORIENT_HORIZ, text_size,
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
            thickness, italic, false );
    }

    /* Trace des reperes selon l'axe Y */
    ipas  = (yg - ref.y) / PAS_REF;
    gypas = ( yg - ref.y) / ipas;
    for( int ii = ref.y + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        if( jj < 26 )
            msg.Printf( wxT( "%c" ), jj + 'A' );
        else    // I hope 52 identifiers are enought...
            msg.Printf( wxT( "%c" ), 'a' + jj - 26 );
        if( ii < yg - PAS_REF / 2 )
        {
            pos.x = ref.x * conv_unit; pos.y = ii * conv_unit;
            plotter->move_to(pos);
            pos.x = (ref.x + GRID_REF_W) * conv_unit; pos.y = ii * conv_unit;
            plotter->finish_to(pos);
        }
        pos.x = (ref.x + GRID_REF_W / 2) * conv_unit;
        pos.y = (ii - gypas / 2) * conv_unit;
        plotter->text( pos, color,
            msg, TEXT_ORIENT_HORIZ, text_size,
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
            thickness, italic, false );

        if( ii < yg - PAS_REF / 2 )
        {
            pos.x = xg * conv_unit; pos.y = ii * conv_unit;
            plotter->move_to(pos);
            pos.x = (xg - GRID_REF_W) * conv_unit; pos.y = ii * conv_unit;
            plotter->finish_to(pos);
        }
        pos.x = (xg - GRID_REF_W / 2) * conv_unit;
        pos.y = (ii - gypas / 2) * conv_unit;
        plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ, text_size,
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
            thickness, italic, false );
    }
#endif

    /* Trace du cartouche */
    text_size.x = SIZETEXT * conv_unit;
    text_size.y = SIZETEXT * conv_unit;
#if defined(KICAD_GOST)
    ref.x = PageSize.x - Sheet->m_RightMargin;
    ref.y = PageSize.y - Sheet->m_BottomMargin;
    if (screen->m_ScreenNumber == 1)
    {
	for(Ki_WorkSheetData* WsItem = &WS_Date; 
		WsItem != NULL; 
		WsItem = WsItem->Pnext )
	{
	    pos.x = (ref.x - WsItem->m_Posx) * conv_unit;
	    pos.y = (ref.y - WsItem->m_Posy) * conv_unit;
	    msg.Empty();
	    switch( WsItem->m_Type )
	    {
		case WS_DATE:
		    break;
		case WS_REV:
		    break;
		case WS_KICAD_VERSION:
		    break;
		case WS_PODPIS:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ,text_size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                    thickness, italic, false );
		    break;
		case WS_SIZESHEET:
		    break;
		case WS_IDENTSHEET:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    msg << screen->m_ScreenNumber;
		    plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ,text_size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                    thickness, italic, false );
		    break;
		case WS_SHEETS:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    msg << screen->m_NumberOfScreen;
		    plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ, text_size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                    thickness, italic, false );
		    break;
		case WS_COMPANY_NAME:
		    break;
		case WS_TITLE:
		    break;
		case WS_COMMENT1:
		    break;
		case WS_COMMENT2:
		    break;
		case WS_COMMENT3:
		    break;
		case WS_COMMENT4:
		    break;
		case WS_UPPER_SEGMENT:
		case WS_LEFT_SEGMENT:
		case WS_SEGMENT:
		    plotter->move_to(pos);
		    pos.x = (ref.x - WsItem->m_Endx) * conv_unit;
		    pos.y = (ref.y - WsItem->m_Endy)  * conv_unit;
		    plotter->finish_to(pos);
		    break;
	    }
	}
    } else {
	for(Ki_WorkSheetData* WsItem = &WS_CADRE_D; 
		WsItem != NULL; 
		WsItem = WsItem->Pnext )
	{
	    pos.x = (ref.x - WsItem->m_Posx) * conv_unit;
	    pos.y = (ref.y - WsItem->m_Posy) * conv_unit;
	    msg.Empty();
	    switch( WsItem->m_Type )
	    {
		case WS_CADRE:
		/* Begin list number > 1 */
		case WS_PODPIS_D:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ, text_size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                    thickness, italic, false );
		    break;
		case WS_IDENTSHEET_D:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    msg << screen->m_ScreenNumber;
		    plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ, text_size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                    thickness, italic, false );
		    break;
		case WS_LEFT_SEGMENT_D:
		case WS_SEGMENT_D:
		    plotter->move_to(pos);
		    pos.x = (ref.x - WsItem->m_Endx) * conv_unit;
		    pos.y = (ref.y - WsItem->m_Endy) * conv_unit;
		    plotter->finish_to(pos);
		    break;
	    }
	}
    }
#else
    ref.x = PageSize.x - GRID_REF_W - Sheet->m_RightMargin;
    ref.y = PageSize.y - GRID_REF_W - Sheet->m_BottomMargin;

    for (Ki_WorkSheetData* WsItem = &WS_Date; 
	    WsItem != NULL; 
	    WsItem = WsItem->Pnext )
    {
        pos.x = (ref.x - WsItem->m_Posx) * conv_unit;
        pos.y = (ref.y - WsItem->m_Posy) * conv_unit;
	 bold = false;
        if( WsItem->m_Legende )
            msg = WsItem->m_Legende;
        else
            msg.Empty();

        switch( WsItem->m_Type )
        {
        case WS_DATE:
            msg += screen->m_Date;
	     bold = true;
            break;

        case WS_REV:
            msg += screen->m_Revision;
	     bold = true;
            break;

        case WS_KICAD_VERSION:
            msg += g_ProductName;
            break;

        case WS_SIZESHEET:
            msg += screen->m_CurrentSheetDesc->m_Name;
            break;

        case WS_IDENTSHEET:
            msg << screen->m_ScreenNumber << wxT( "/" ) << screen->m_NumberOfScreen;
            break;

        case WS_FILENAME:
        {
            wxString fname, fext;
            wxFileName::SplitPath( screen->m_FileName, (wxString*) NULL, &fname, &fext );
            msg << fname << wxT( "." ) << fext;
        }
            break;

        case WS_FULLSHEETNAME:
            msg += GetScreenDesc();
            break;

        case WS_COMPANY_NAME:
            msg += screen->m_Company;
            if( !msg.IsEmpty() )
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
	     bold = true;
            break;

        case WS_TITLE:
            msg += screen->m_Title;
	     bold = true;
            break;

        case WS_COMMENT1:
            msg += screen->m_Commentaire1;
            if( !msg.IsEmpty() )
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            break;

        case WS_COMMENT2:
            msg += screen->m_Commentaire2;
            if( !msg.IsEmpty() )
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            break;

        case WS_COMMENT3:
            msg += screen->m_Commentaire3;
            if( !msg.IsEmpty() )
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            break;

        case WS_COMMENT4:
            msg += screen->m_Commentaire4;
            if( !msg.IsEmpty() )
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            break;

        case WS_UPPER_SEGMENT:
            if( UpperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy        =
                WS_MostUpperLine.m_Endy    =
                    WS_MostLeftLine.m_Posy = UpperLimit;
            pos.y = (ref.y - WsItem->m_Posy) * conv_unit;

        case WS_SEGMENT:
        {
            wxPoint auxpos;
            auxpos.x = (ref.x - WsItem->m_Endx) * conv_unit;;
            auxpos.y = (ref.y - WsItem->m_Endy) * conv_unit;;
		plotter->move_to( pos );
		plotter->finish_to( auxpos );
        }
            break;
        }

        if( !msg.IsEmpty() )
        {
	    plotter->text( pos, color,
                msg.GetData(), TEXT_ORIENT_HORIZ, text_size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                thickness, italic, bold );
        }
    }
#endif
}

/******************************************/
void Plotter::user_to_device_coordinates( wxPoint& pos )
/******************************************/

/* modifie les coord pos.x et pos.y pour le trace selon l'orientation,
  * l'echelle, les offsets de trace */
{
    pos.x = (int) ((pos.x - plot_offset.x) * plot_scale * device_scale);
    
    if (plot_orient_options == PLOT_MIROIR)
	pos.y = (int) ((pos.y - plot_offset.y) * plot_scale * device_scale);
    else
	pos.y = (int) ((paper_size.y - (pos.y - plot_offset.y) * plot_scale) * device_scale);
}

/********************************************************************/
void Plotter::arc( wxPoint centre, int StAngle, int EndAngle, int rayon, 
	FILL_T fill, int width )
/********************************************************************/
/* Generic arc rendered as a polyline */
{
    wxPoint start, end;
    const int delta = 50; 	/* increment (in 0.1 degrees) to draw circles */
    double alpha;

    if (StAngle > EndAngle)
	EXCHG(StAngle, EndAngle);
    
    set_current_line_width(width);
    /* Please NOTE the different sign due to Y-axis flip */
    alpha = StAngle/1800.0*M_PI;
    start.x = centre.x + (int) (rayon * cos(-alpha));
    start.y = centre.y + (int) (rayon * sin(-alpha));
    move_to(start);
    for(int ii = StAngle+delta; ii < EndAngle; ii += delta )
    {
	alpha = ii/1800.0*M_PI;
        end.x = centre.x + (int) (rayon * cos(-alpha));
        end.y = centre.y + (int) (rayon * sin(-alpha));
        line_to(end);
    }

    alpha = EndAngle/1800.0*M_PI;
    end.x = centre.x + (int) (rayon * cos(-alpha));
    end.y = centre.y + (int) (rayon * sin(-alpha));
    finish_to( end );
}

/************************************/
void Plotter::user_to_device_size( wxSize& size )
/************************************/
/* modifie les dimension size.x et size.y pour le trace selon l'echelle */
{
    size.x = (int) (size.x * plot_scale * device_scale);
    size.y = (int) (size.y * plot_scale * device_scale);
}

/************************************/
double Plotter::user_to_device_size( double size )
/************************************/
{
    return size * plot_scale * device_scale;
}

/************************************************************************************/
void Plotter::center_square( const wxPoint& position, int diametre, FILL_T fill)
/************************************************************************************/
{
    int rayon = diametre / 2.8284;
    int coord[10] = {
	position.x+rayon, position.y+rayon,
	position.x+rayon, position.y-rayon,
	position.x-rayon, position.y-rayon,
	position.x-rayon, position.y+rayon,
	position.x+rayon, position.y+rayon
    };
    if (fill) 
    {
	poly(4, coord, fill);
    }
    else
    {
	poly(5, coord, fill);
    }
}

/************************************************************************************/
void Plotter::center_lozenge( const wxPoint& position, int diametre, FILL_T fill)
/************************************************************************************/
{
    int rayon = diametre / 2;
    int coord[10] = {
	position.x, position.y+rayon,
	position.x+rayon, position.y,
	position.x, position.y-rayon,
	position.x-rayon, position.y,
	position.x, position.y+rayon,
    };
    if (fill) 
    {
	poly(4, coord, fill);
    }
    else
    {
	poly(5, coord, fill);
    }
}

/************************************************************************************/
void Plotter::marker( const wxPoint& position, int diametre, int aShapeId)
/************************************************************************************/

/* Trace un motif de numero de forme aShapeId, aux coord x0, y0.
 *  x0, y0 = coordonnees tables
 *  diametre = diametre (coord table) du trou
 *  aShapeId = index ( permet de generer des formes caract )
 */
{
    int  rayon = diametre / 2;

    int  x0, y0;

    x0 = position.x; y0 = position.y;

    switch( aShapeId )
    {
    case 0:     /* vias : forme en X */
        move_to( wxPoint( x0 - rayon, y0 - rayon ) );
	line_to( wxPoint( x0 + rayon, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 - rayon ) );
        finish_to( wxPoint( x0 - rayon, y0 + rayon ) );
        break;

    case 1:     /* Cercle */
	circle(position, diametre, NO_FILL);
        break;

    case 2:     /* forme en + */
        move_to( wxPoint( x0, y0 - rayon ) );
        line_to( wxPoint( x0, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 ) );
        finish_to( wxPoint( x0 - rayon, y0 ) );
        break;

    case 3:     /* forme en X cercle */
	circle(position, diametre, NO_FILL);
        move_to( wxPoint( x0 - rayon, y0 - rayon ) );
        line_to( wxPoint( x0 + rayon, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 - rayon ) );
        finish_to( wxPoint( x0 - rayon, y0 + rayon ) );
        break;

    case 4:     /* forme en cercle barre de - */
	circle(position, diametre, NO_FILL);
        move_to( wxPoint( x0 - rayon, y0 ) );
        finish_to( wxPoint( x0 + rayon, y0 ) );
        break;

    case 5:     /* forme en cercle barre de | */
	circle(position, diametre, NO_FILL);
        move_to( wxPoint( x0, y0 - rayon ) );
        finish_to( wxPoint( x0, y0 + rayon ) );
        break;

    case 6:     /* forme en carre */
	center_square(position, diametre, NO_FILL);
        break;

    case 7:     /* forme en losange */
	center_lozenge(position, diametre, NO_FILL);
        break;

    case 8:     /* forme en carre barre par un X*/
	center_square(position, diametre, NO_FILL);
        move_to( wxPoint( x0 - rayon, y0 - rayon ) );
        line_to( wxPoint( x0 + rayon, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 - rayon ) );
        finish_to( wxPoint( x0 - rayon, y0 + rayon ) );
        break;

    case 9:     /* forme en losange barre par un +*/
	center_lozenge(position, diametre, NO_FILL);
        move_to( wxPoint( x0, y0 - rayon ) );
        line_to( wxPoint( x0, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 ) );
        finish_to( wxPoint( x0 - rayon, y0 ) );
        break;

    case 10:     /* forme en carre barre par un '/' */
	center_square(position, diametre, NO_FILL);
        move_to( wxPoint( x0 - rayon, y0 - rayon ) );
        finish_to( wxPoint( x0 + rayon, y0 + rayon ) );
        break;

    case 11:     /* forme en losange barre par un |*/
	center_lozenge(position, diametre, NO_FILL);
        move_to( wxPoint( x0, y0 - rayon ) );
        finish_to( wxPoint( x0, y0 + rayon ) );
        break;

    case 12:     /* forme en losange barre par un -*/
	center_lozenge(position, diametre, NO_FILL);
        move_to( wxPoint( x0 - rayon, y0 ) );
        finish_to( wxPoint( x0 + rayon, y0 ) );
        break;

    default:
	circle(position, diametre, NO_FILL);
        break;
    }
}

/***************************************************************/
void Plotter::segment_as_oval( wxPoint start, wxPoint end, int width,
	GRTraceMode tracemode)
/***************************************************************/
{
    /* Convert a thick segment and plot it as an oval */
    wxPoint center( (start.x + end.x) / 2, (start.y + end.y) / 2);
    wxSize size( end.x - start.x, end.y - start.y);
    int orient;
    if ( size.y == 0 )
	orient = 0;
    else if ( size.x == 0 )
	orient = 900;
    else orient = - (int) (atan2( (double)size.y, (double)size.x ) * 1800.0 / M_PI);
    size.x = (int) sqrt( ((double)size.x * size.x) + ((double)size.y * size.y) ) + width;
    size.y = width;

    flash_pad_oval( center, size, orient, tracemode );
}

/***************************************************************/
void Plotter::sketch_oval(wxPoint pos, wxSize size, int orient,
	int width)
/***************************************************************/
{
    set_current_line_width(width);
    width = current_pen_width;
    int rayon, deltaxy, cx, cy;
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y ); orient += 900;
        if( orient >= 3600 )
            orient -= 3600;
    }
    deltaxy = size.y - size.x; /* = distance entre centres de l'ovale */
    rayon   = (size.x-width) / 2;
    cx = -rayon; cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    move_to( wxPoint( cx + pos.x, cy + pos.y ) );
    cx = -rayon; cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    finish_to( wxPoint( cx + pos.x, cy + pos.y ) );

    cx = rayon; cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    move_to( wxPoint( cx + pos.x, cy + pos.y ) );
    cx = rayon; cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    finish_to( wxPoint( cx + pos.x, cy + pos.y ) );

    cx = 0; cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    arc( wxPoint( cx + pos.x, cy + pos.y ),
	    orient + 1800 , orient + 3600,
	    rayon, NO_FILL);
    cx = 0; cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    arc( wxPoint( cx + pos.x, cy + pos.y ),
	    orient, orient + 1800,
	    rayon, NO_FILL );
}

/***************************************************************/
void Plotter::thick_segment( wxPoint start, wxPoint end, int width,
	   GRTraceMode tracemode )
/***************************************************************/
/* Plot 1 segment like a track segment
 */
{
    switch (tracemode)
    {
    case FILLED:
    case FILAIRE:
	set_current_line_width(tracemode==FILLED?width:-1);
	move_to(start);
	finish_to(end);
        break;
    case SKETCH:
	set_current_line_width(-1);
	segment_as_oval(start, end, width, tracemode);
	break;
    }
}

void Plotter::thick_arc( wxPoint centre, int StAngle, int EndAngle, int rayon, 
	    int width, GRTraceMode tracemode )
{
    switch (tracemode)
    {
    case FILAIRE:
	set_current_line_width(-1);
	arc(centre, StAngle, EndAngle, rayon, NO_FILL,-1);
	break;
    case FILLED:
	arc(centre, StAngle, EndAngle, rayon,NO_FILL, width);
	break;
    case SKETCH:
	set_current_line_width(-1);
	arc(centre, StAngle, EndAngle, rayon-(width-current_pen_width)/2,NO_FILL, -1);
	arc(centre, StAngle, EndAngle, rayon+(width-current_pen_width)/2,NO_FILL, -1);
	break;
    }
}

void Plotter::thick_rect( wxPoint p1, wxPoint p2, int width,
	    GRTraceMode tracemode)
{
    switch (tracemode)
    {
    case FILAIRE:
	rect(p1, p2,NO_FILL, -1);
	break;
    case FILLED:
	rect(p1, p2,NO_FILL, width);
	break;
    case SKETCH:
	set_current_line_width(-1);
	p1.x -= (width-current_pen_width)/2;
	p1.y -= (width-current_pen_width)/2;
	p2.x += (width-current_pen_width)/2;
	p2.y += (width-current_pen_width)/2;
	rect(p1, p2,NO_FILL, -1);
	p1.x += (width-current_pen_width);
	p1.y += (width-current_pen_width);
	p2.x -= (width-current_pen_width);
	p2.y -= (width-current_pen_width);
	rect(p1, p2,NO_FILL, -1);
        break;
    }
}

void Plotter::thick_circle( wxPoint pos, int diametre, int width,
	    GRTraceMode tracemode)
{
    switch (tracemode)
    {
    case FILAIRE:
	circle(pos, diametre,NO_FILL, -1);
	break;
    case FILLED:
	circle(pos, diametre,NO_FILL, width);
	break;
    case SKETCH:
	set_current_line_width(-1);
	circle(pos, diametre-width+current_pen_width,NO_FILL, -1);
	circle(pos, diametre+width-current_pen_width,NO_FILL, -1);
	break;
    }
}

/*************************************************************************************/
void Plotter::set_paper_size(Ki_PageDescr* asheet)
/*************************************************************************************/
{
    wxASSERT(!output_file);
    sheet = asheet;
    // Sheets are in mils, plotter works with decimils
    paper_size.x = sheet->m_Size.x * 10;
    paper_size.y = sheet->m_Size.y * 10;
}

