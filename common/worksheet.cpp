/**************************************************/
/* WORKSHEET.CPP : routines de trace du cartouche */
/**************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "macros.h"

#include "worksheet.h"

/* Must be defined in main applications: */
extern wxString g_Main_Title;

/*************************************************************************************/
void WinEDA_DrawFrame::TraceWorkSheet( wxDC* DC, BASE_SCREEN* screen, int line_width )
/*************************************************************************************/

/* Draw the sheet references
 */
{
    if( !m_Draw_Sheet_Ref )
        return;

    Ki_PageDescr*     Sheet = screen->m_CurrentSheetDesc;
    int               ii, jj, xg, yg, ipas, gxpas, gypas;
    wxPoint           pos;
    int               refx, refy, Color;
    wxString          Line;
    Ki_WorkSheetData* WsItem;
    int               scale = m_InternalUnits / 1000;
    wxSize            size( SIZETEXT * scale, SIZETEXT * scale );
    wxSize            size_ref( SIZETEXT_REF * scale, SIZETEXT_REF * scale );

    wxString          msg;
    int               UpperLimit = VARIABLE_BLOCK_START_POSITION;
    int               width = line_width;

    Color = RED;
    if( Sheet == NULL )
    {
        DisplayError( this,
            wxT( "WinEDA_DrawFrame::TraceWorkSheet() error: NULL Sheet" ) );
        return;
    }

    // if not printing, draw the page limits:
    if( !g_IsPrinting & g_ShowPageLimits )
    {
        GRSetDrawMode( DC, GR_COPY );
        GRRect( &DrawPanel->m_ClipBox, DC, 0, 0,
            Sheet->m_Size.x * scale, Sheet->m_Size.y * scale, width,
            g_DrawBgColor == WHITE ? LIGHTGRAY : DARKDARKGRAY );
    }

    GRSetDrawMode( DC, GR_COPY );
    /* trace de la bordure */
    refx = Sheet->m_LeftMargin;
    refy = Sheet->m_TopMargin;                      /* Upper left corner */
    xg   = Sheet->m_Size.x - Sheet->m_RightMargin;
    yg   = Sheet->m_Size.y - Sheet->m_BottomMargin; /* lower right corner */

#if defined(KICAD_GOST)
    GRRect( &DrawPanel->m_ClipBox, DC, refx * scale, refy * scale,
	xg * scale, yg * scale, width, Color );

#else
    for( ii = 0; ii < 2; ii++ )
    {
        GRRect( &DrawPanel->m_ClipBox, DC, refx * scale, refy * scale,
            xg * scale, yg * scale, width, Color );

        refx += GRID_REF_W; refy += GRID_REF_W;
        xg   -= GRID_REF_W; yg -= GRID_REF_W;
    }
#endif

    /* trace des reperes */
    refx = Sheet->m_LeftMargin;
#if defined(KICAD_GOST)
    refy = Sheet->m_Size.y - Sheet->m_BottomMargin; /* Lower left corner */
    for( WsItem = &WS_Segm1_LU; WsItem != NULL; WsItem = WsItem->Pnext )
    {
	pos.x = (refx - WsItem->m_Posx)* scale;
	pos.y = (refy - WsItem->m_Posy)* scale;
	msg.Empty();
	switch( WsItem->m_Type )
	{
	    case WS_CADRE:
		break;
	    case WS_PODPIS_LU:
		if(WsItem->m_Legende) msg = WsItem->m_Legende;
		DrawGraphicText(DrawPanel, DC, pos, Color,
				msg, TEXT_ORIENT_VERT, size,
				GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM,width);
		break;
	    case WS_SEGMENT_LU:
		xg = Sheet->m_LeftMargin - WsItem->m_Endx;
		yg = Sheet->m_Size.y - Sheet->m_BottomMargin - WsItem->m_Endy;
		GRLine(&DrawPanel->m_ClipBox, DC, pos.x, pos.y,
			xg * scale, yg * scale, width, Color);
		break;
	}
    }
    refy = Sheet->m_BottomMargin; /* Left Top corner */
    for( WsItem = &WS_Segm1_LT; WsItem != NULL; WsItem = WsItem->Pnext )
    {
	pos.x = (refx + WsItem->m_Posx)* scale;
	pos.y = (refy + WsItem->m_Posy)* scale;
	msg.Empty();
	switch( WsItem->m_Type )
	{
	    case WS_SEGMENT_LT:
		xg = Sheet->m_LeftMargin + WsItem->m_Endx;
		yg = Sheet->m_BottomMargin + WsItem->m_Endy;
		GRLine(&DrawPanel->m_ClipBox, DC, pos.x, pos.y,
			xg * scale, yg * scale, width, Color);
		break;
	}
    }
#else
    refy = Sheet->m_TopMargin;                      /* Upper left corner */
    xg   = Sheet->m_Size.x - Sheet->m_RightMargin;
    yg   = Sheet->m_Size.y - Sheet->m_BottomMargin; /* lower right corner */

    /* Trace des reperes selon l'axe X */
    ipas  = (xg - refx) / PAS_REF;
    gxpas = ( xg - refx) / ipas;
    for( ii = refx + gxpas, jj = 1; ipas > 0; ii += gxpas, jj++, ipas-- )
    {
        Line.Printf( wxT( "%d" ), jj );
        if( ii < xg - PAS_REF / 2 )
        {
            GRLine( &DrawPanel->m_ClipBox, DC, ii * scale, refy * scale,
                ii * scale, (refy + GRID_REF_W) * scale, width, Color );
        }
        DrawGraphicText( DrawPanel, DC,
            wxPoint( (ii - gxpas / 2) * scale, (refy + GRID_REF_W / 2) * scale ),
            Color,
            Line, TEXT_ORIENT_HORIZ, size_ref,
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, width );
        if( ii < xg - PAS_REF / 2 )
        {
            GRLine( &DrawPanel->m_ClipBox, DC, ii * scale, yg * scale,
                ii * scale, (yg - GRID_REF_W) * scale, width, Color );
        }
        DrawGraphicText( DrawPanel, DC,
            wxPoint( (ii - gxpas / 2) * scale, (yg - GRID_REF_W / 2) * scale ),
            Color,
            Line, TEXT_ORIENT_HORIZ, size_ref,
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, width );
    }

    /* Trace des reperes selon l'axe Y */
    ipas  = (yg - refy) / PAS_REF;
    gypas = ( yg - refy) / ipas;
    for( ii = refy + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        Line.Empty();
        if( jj < 26 )
            Line.Printf( wxT( "%c" ), jj + 'A' );
        else
            Line.Printf( wxT( "%c" ), 'a' + jj - 26 );
        if( ii < yg - PAS_REF / 2 )
        {
            GRLine( &DrawPanel->m_ClipBox, DC, refx * scale, ii * scale,
                (refx + GRID_REF_W) * scale, ii * scale, width, Color );
        }
        DrawGraphicText( DrawPanel, DC,
            wxPoint( (refx + GRID_REF_W / 2) * scale, (ii - gypas / 2) * scale ),
            Color,
            Line, TEXT_ORIENT_HORIZ, size_ref,
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, width );
        if( ii < yg - PAS_REF / 2 )
        {
            GRLine( &DrawPanel->m_ClipBox, DC, xg * scale, ii * scale,
                (xg - GRID_REF_W) * scale, ii * scale, width, Color );
        }
        DrawGraphicText( DrawPanel, DC,
            wxPoint( (xg - GRID_REF_W / 2) * scale, (ii - gxpas / 2) * scale ),
            Color,
            Line, TEXT_ORIENT_HORIZ, size_ref,
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, width );
    }
#endif

    /* Trace du cartouche */
#if defined(KICAD_GOST)
    refx = Sheet->m_Size.x - Sheet->m_RightMargin;
    refy = Sheet->m_Size.y - Sheet->m_BottomMargin; /* lower right corner */
    if (screen->m_ScreenNumber == 1)
    {
	for( WsItem = &WS_Date; WsItem != NULL; WsItem = WsItem->Pnext )
	{
	    pos.x = (refx - WsItem->m_Posx)* scale;
	    pos.y = (refy - WsItem->m_Posy)* scale;
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
		    DrawGraphicText(DrawPanel, DC, pos, Color,
				    msg, TEXT_ORIENT_HORIZ, size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width);
		    break;
		case WS_SIZESHEET:
		    break;
		case WS_IDENTSHEET:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    msg << screen->m_ScreenNumber;
		    DrawGraphicText(DrawPanel, DC, pos, Color,
				    msg, TEXT_ORIENT_HORIZ, size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width);
		    break;
		case WS_SHEETS:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    msg << screen->m_NumberOfScreen;
		    DrawGraphicText(DrawPanel, DC, pos, Color,
				    msg, TEXT_ORIENT_HORIZ, size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width);
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
		    WS_MostUpperLine.m_Posy = 
		    WS_MostUpperLine.m_Endy = 
		    WS_MostLeftLine.m_Posy = STAMP_OY;
		    pos.y = (refy - WsItem->m_Posy)* scale;
		case WS_SEGMENT:
		    xg = Sheet->m_Size.x -
		    Sheet->m_RightMargin - WsItem->m_Endx;
		    yg = Sheet->m_Size.y -
		    Sheet->m_BottomMargin - WsItem->m_Endy;
		    GRLine(&DrawPanel->m_ClipBox, DC, pos.x, pos.y,
			    xg * scale, yg * scale, width, Color);
		    break;
	    }
	}
    } else {
	for( WsItem = &WS_CADRE_D; WsItem != NULL; WsItem = WsItem->Pnext )
	{
	    pos.x = (refx - WsItem->m_Posx)* scale;
	    pos.y = (refy - WsItem->m_Posy)* scale;
	    msg.Empty();
	    switch( WsItem->m_Type )
	    {
		case WS_CADRE:
		/* Begin list number > 1 */
		case WS_PODPIS_D:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    DrawGraphicText(DrawPanel, DC, pos, Color,
				    msg, TEXT_ORIENT_HORIZ, size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,width);
		    break;
		case WS_IDENTSHEET_D:
		    if(WsItem->m_Legende) msg = WsItem->m_Legende;
		    msg << screen->m_ScreenNumber;
		    DrawGraphicText(DrawPanel, DC, pos, Color,
				    msg, TEXT_ORIENT_HORIZ, size,
				    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,width);
		    break;
		case WS_LEFT_SEGMENT_D:
		    pos.y = (refy - WsItem->m_Posy)* scale;
		case WS_SEGMENT_D:
		    xg = Sheet->m_Size.x -
			    Sheet->m_RightMargin - WsItem->m_Endx;
		    yg = Sheet->m_Size.y -
			    Sheet->m_BottomMargin - WsItem->m_Endy;
		    GRLine(&DrawPanel->m_ClipBox, DC, pos.x, pos.y,
			    xg * scale, yg * scale, width, Color);
		    break;
	    }
	}
    }
#else
    refx = Sheet->m_Size.x - Sheet->m_RightMargin - GRID_REF_W;
    refy = Sheet->m_Size.y - Sheet->m_BottomMargin - GRID_REF_W; /* lower right corner */

    for( WsItem = &WS_Date; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x = (refx - WsItem->m_Posx) * scale;
        pos.y = (refy - WsItem->m_Posy) * scale;
        msg.Empty();

        switch( WsItem->m_Type )
        {
        case WS_DATE:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += screen->m_Date;
            DrawGraphicText( DrawPanel, DC, pos, Color,
                msg, TEXT_ORIENT_HORIZ, size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
            break;

        case WS_REV:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += screen->m_Revision;
            DrawGraphicText( DrawPanel, DC, pos, Color,
                msg, TEXT_ORIENT_HORIZ, size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
            break;

        case WS_KICAD_VERSION:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += g_ProductName + g_Main_Title;
            msg += wxT( " " ) + GetBuildVersion();
            DrawGraphicText( DrawPanel, DC, pos, Color,
                msg, TEXT_ORIENT_HORIZ, size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
            break;

        case WS_SIZESHEET:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += Sheet->m_Name;
            DrawGraphicText( DrawPanel, DC, pos, Color,
                msg, TEXT_ORIENT_HORIZ, size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
            break;


        case WS_IDENTSHEET:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg << screen->m_ScreenNumber << wxT( "/" ) << screen->m_NumberOfScreen;
            DrawGraphicText( DrawPanel, DC, pos, Color,
                msg, TEXT_ORIENT_HORIZ, size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
            break;

        case WS_FILENAME:
        {
            wxString fname, fext;
            wxFileName::SplitPath( screen->m_FileName, (wxString*) NULL, &fname, &fext );
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg << fname << wxT( "." ) << fext;
            DrawGraphicText( DrawPanel, DC, pos, Color,
                msg, TEXT_ORIENT_HORIZ, size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
        }
            break;

        case WS_FULLSHEETNAME:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += GetScreenDesc();
            DrawGraphicText( DrawPanel, DC, pos, Color,
                msg, TEXT_ORIENT_HORIZ, size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
            break;


        case WS_COMPANY_NAME:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += screen->m_Company;
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( DrawPanel, DC, pos, Color,
                    msg, TEXT_ORIENT_HORIZ, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_TITLE:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += screen->m_Title;
            DrawGraphicText( DrawPanel, DC, pos, Color,
                msg, TEXT_ORIENT_HORIZ, size,
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
            break;

        case WS_COMMENT1:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += screen->m_Commentaire1;
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( DrawPanel, DC, pos, Color,
                    msg, TEXT_ORIENT_HORIZ, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT2:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += screen->m_Commentaire2;
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( DrawPanel, DC, pos, Color,
                    msg, TEXT_ORIENT_HORIZ, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT3:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += screen->m_Commentaire3;
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( DrawPanel, DC, pos, Color,
                    msg, TEXT_ORIENT_HORIZ, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT4:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += screen->m_Commentaire4;
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( DrawPanel, DC, pos, Color,
                    msg, TEXT_ORIENT_HORIZ, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_UPPER_SEGMENT:
            if( UpperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy        =
                WS_MostUpperLine.m_Endy    =
                    WS_MostLeftLine.m_Posy = UpperLimit;
            pos.y = (refy - WsItem->m_Posy) * scale;

        case WS_SEGMENT:
            xg = Sheet->m_Size.x -
                 GRID_REF_W - Sheet->m_RightMargin - WsItem->m_Endx;
            yg = Sheet->m_Size.y -
                 GRID_REF_W - Sheet->m_BottomMargin - WsItem->m_Endy;
            GRLine( &DrawPanel->m_ClipBox, DC, pos.x, pos.y,
                xg * scale, yg * scale, width, Color );
            break;
        }
    }
#endif
}


/************************************************************************************************/
wxString WinEDA_DrawFrame::GetXYSheetReferences( BASE_SCREEN* aScreen, const wxPoint& aPosition )
/************************************************************************************************/

/** Function GetXYSheetReferences
 * Return the X,Y sheet references where the point position is located
 * @param aScreen = screen to use
 * @param aPosition = position to identify by YX ref
 * @return a wxString containing the message locator like A3 or B6 (or ?? if out of page limits)
 */
{
    Ki_PageDescr* Sheet = aScreen->m_CurrentSheetDesc;
    int           ii, xg, yg, ipas, gxpas, gypas;
    int           refx, refy;
    wxString      msg;

    if( Sheet == NULL )
    {
        DisplayError( this,
            wxT( "WinEDA_DrawFrame::GetXYSheetReferences() error: NULL Sheet" ) );
        return msg;
    }

    refx = Sheet->m_LeftMargin;
    refy = Sheet->m_TopMargin;                      /* Upper left corner */
    xg   = Sheet->m_Size.x - Sheet->m_RightMargin;
    yg   = Sheet->m_Size.y - Sheet->m_BottomMargin; /* lower right corner */

    /* Get the Y axis identifier (A symbol A ... Z) */
    if( aPosition.y < refy || aPosition.y > yg )  // Ouside of Y limits
        msg << wxT( "?" );
    else
    {
        ipas  = (yg - refy) / PAS_REF;      // ipas = Y count sections
        gypas = ( yg - refy) / ipas;        // gypas = Y section size
        ii    = (aPosition.y - refy) / gypas;
        msg.Printf( wxT( "%c" ), 'A' + ii );
    }

    /* Get the X axis identifier (A number 1 ... n) */
    if( aPosition.x < refx || aPosition.x > xg )  // Ouside of X limits
        msg << wxT( "?" );
    else
    {
        ipas  = (xg - refx) / PAS_REF;  // ipas = X count sections
        gxpas = ( xg - refx) / ipas;    // gxpas = X section size

        ii = (aPosition.x - refx) / gxpas;
        msg << ii + 1;
    }

    return msg;
}


/*********************************************************************/
wxString WinEDA_DrawFrame::GetScreenDesc()
/*********************************************************************/
{
    wxString msg;

    msg << GetBaseScreen()->m_ScreenNumber << wxT( "/" )
        << GetBaseScreen()->m_NumberOfScreen;
    return msg;
}
