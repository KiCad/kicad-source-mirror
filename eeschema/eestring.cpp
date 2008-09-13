/**************************************/
/*	Module to handle screen printing. */
/**************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "plot_common.h"

#include "protos.h"

extern void Move_Plume( wxPoint pos, int plume ); // see plot.cpp


/*****************************************************************************
* Plot pin number and pin text info, given the pin line coordinates.	  *
* Same as DrawPinTexts((), but output is the plotter
* The line must be vertical or horizontal.						  *
* If PinNext == NULL nothing is printed.									*
* Current Zoom factor is taken into account.					 *
* If TextInside then the text is been put inside (moving from x1, y1 in		 *
* the opposite direction to x2,y2), otherwise all is drawn outside.		 *
*****************************************************************************/
void LibDrawPin::PlotPinTexts( wxPoint& pin_pos, int orient,
                               int TextInside, bool DrawPinNum, bool DrawPinName )
{
    int      dx, len, start;
    int      ii, x, y, x1, y1, cte;
    wxString StringPinNum;
    wxString PinText;
    int      PinTextBarPos[256];
    int      PinTextBarCount;
    int      NameColor, NumColor;
    int      PinTxtLen   = 0;
    wxSize   PinNameSize = wxSize( m_PinNameSize, m_PinNameSize );
    wxSize   PinNumSize  = wxSize( m_PinNumSize, m_PinNumSize );
    bool     plot_color  = (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt;

    /* Get the num and name colors */
    NameColor = plot_color ? ReturnLayerColor( LAYER_PINNAM ) : -1;
    NumColor  = plot_color ? ReturnLayerColor( LAYER_PINNUM ) : -1;

    /* Create the pin num string */
    ReturnPinStringNum( StringPinNum );
    x1 = pin_pos.x; y1 = pin_pos.y;

    switch( orient )
    {
    case PIN_UP:
        y1 -= m_PinLen; break;

    case PIN_DOWN:
        y1 += m_PinLen; break;

    case PIN_LEFT:
        x1 -= m_PinLen; break;

    case PIN_RIGHT:
        x1 += m_PinLen; break;
    }

    const wxChar* textsrc = m_PinName.GetData();
    float         fPinTextPitch = PinNameSize.x * 1.1;
    /* Do we need to invert the string? Is this string has only "~"? */
    PinTextBarCount = 0; PinTxtLen = 0;
    ii = 0;
    while( *textsrc )
    {
        if( *textsrc == '~' )
        {
            PinTextBarPos[PinTextBarCount++] = (int) (fPinTextPitch * PinTxtLen);
        }
        else
        {
            PinText.Append( *textsrc );
            PinTxtLen++;
        }

        textsrc++;
    }

    PinTxtLen = (int) (fPinTextPitch * PinTxtLen);
    PinTextBarPos[PinTextBarCount] = PinTxtLen; // Needed if no end '~'

    if( PinText[0] == 0 )
        DrawPinName = FALSE;

    if( TextInside )  /* Draw the text inside, but the pin numbers outside. */
    {
        if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
        {     /* Its an horizontal line. */
            if( PinText && DrawPinName )
            {
                if( orient == PIN_RIGHT )
                {
                    x = x1 + TextInside;
                    PlotGraphicText( g_PlotFormat, wxPoint( x, y1 ), NameColor, PinText,
                        TEXT_ORIENT_HORIZ,
                        PinNameSize,
                        GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER );

                    for( ii = 0; ii < PinTextBarCount; )
                    {
                        cte = y1 - PinNameSize.y / 2 - TXTMARGE;
                        dx  = PinTextBarPos[ii++];  // Get the line pos
                        Move_Plume( wxPoint( x + dx, cte ), 'U' );
                        len = PinTextBarPos[ii++];  // Get the line end
                        Move_Plume( wxPoint( x + len, cte ), 'D' );
                    }
                }
                else    // orient == PIN_LEFT
                {
                    x = x1 - TextInside;
                    PlotGraphicText( g_PlotFormat, wxPoint( x, y1 ),
                        NameColor, PinText, TEXT_ORIENT_HORIZ,
                        PinNameSize,
                        GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER );

                    for( ii = 0; ii < PinTextBarCount; )
                    {
                        cte = y1 - PinNameSize.y / 2 - TXTMARGE;
                        dx  = PinTextBarPos[ii++];  // Get the line pos
                        Move_Plume( wxPoint( x + dx - PinTxtLen, cte ), 'U' );
                        len = PinTextBarPos[ii++];  // Get the line end
                        Move_Plume( wxPoint( x + len - PinTxtLen, cte ), 'D' );
                    }
                }
            }

            if( DrawPinNum )
            {
                PlotGraphicText( g_PlotFormat, wxPoint( (x1 + pin_pos.x) / 2, y1 - TXTMARGE ),
                    NumColor, StringPinNum,
                    TEXT_ORIENT_HORIZ, PinNumSize,
                    GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM );
            }
        }
        else         /* Its a vertical line. */
        {
            if( PinText && DrawPinName )
            {
                if( orient == PIN_DOWN )
                {
                    y = y1 + TextInside;

                    PlotGraphicText( g_PlotFormat, wxPoint( x1, y ), NameColor, PinText,
                        TEXT_ORIENT_VERT, PinNameSize,
                        GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP );

                    for( ii = 0; ii < PinTextBarCount; )
                    {
                        cte = x1 - PinNameSize.y / 2 - TXTMARGE;
                        dx  = PinTextBarPos[ii++];  // Get the line pos
                        Move_Plume( wxPoint( cte, y + PinTxtLen - dx ), 'U' );
                        len = PinTextBarPos[ii++];  // Get the line end
                        Move_Plume( wxPoint( cte, y + PinTxtLen - len ), 'D' );
                    }
                }
                else    /* PIN_UP */
                {
                    y = y1 - TextInside;

                    PlotGraphicText( g_PlotFormat, wxPoint( x1, y ), NameColor, PinText,
                        TEXT_ORIENT_VERT, PinNameSize,
                        GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM );

                    for( ii = 0; ii < PinTextBarCount; )
                    {
                        cte = x1 - PinNameSize.y / 2 - TXTMARGE;
                        dx  = PinTextBarPos[ii++];  // Get the line pos
                        Move_Plume( wxPoint( cte, y - dx ), 'U' );
                        len = PinTextBarPos[ii++];  // Get the line end
                        Move_Plume( wxPoint( cte, y - len ), 'D' );
                    }
                }
            }

            if( DrawPinNum )
            {
                PlotGraphicText( g_PlotFormat, wxPoint( x1 - TXTMARGE, (y1 + pin_pos.y) / 2 ),
                    NumColor, StringPinNum,
                    TEXT_ORIENT_VERT, PinNumSize,
                    GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER );
            }
        }
    }
    else     /* Draw num & text pin outside */
    {
        if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
        /* Its an horizontal line. */
        {
            if( PinText && DrawPinName )
            {
                x = (x1 + pin_pos.x) / 2;
                PlotGraphicText( g_PlotFormat, wxPoint( x, y1 - TXTMARGE ),
                    NameColor, PinText,
                    TEXT_ORIENT_HORIZ, PinNameSize,
                    GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM );

                for( ii = 0; ii < PinTextBarCount; )
                {
                    cte   = y1 - PinNameSize.y - TXTMARGE * 2;
                    start = x - (PinTxtLen / 2);
                    dx    = PinTextBarPos[ii++];    // Get the line pos
                    Move_Plume( wxPoint( start + dx, cte ), 'U' );
                    len = PinTextBarPos[ii++];      // Get the line end
                    Move_Plume( wxPoint( start + len, cte ), 'D' );
                }
            }
            if( DrawPinNum )
            {
                x = (x1 + pin_pos.x) / 2;
                PlotGraphicText( g_PlotFormat, wxPoint( x, y1 + TXTMARGE ),
                    NumColor, StringPinNum,
                    TEXT_ORIENT_HORIZ, PinNumSize,
                    GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP );
            }
        }
        else     /* Its a vertical line. */
        {
            if( PinText && DrawPinName )
            {
                y = (y1 + pin_pos.y) / 2;
                PlotGraphicText( g_PlotFormat, wxPoint( x1 - TXTMARGE, y ),
                    NameColor, PinText,
                    TEXT_ORIENT_VERT, PinNameSize,
                    GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER );

                for( ii = 0; ii < PinTextBarCount; )
                {
                    cte   = x1 - PinNameSize.y - TXTMARGE * 2;
                    start = y + (PinTxtLen / 2);
                    dx    = PinTextBarPos[ii++];    // Get the line pos
                    Move_Plume( wxPoint( cte, start - dx ), 'U' );
                    len = PinTextBarPos[ii++];      // Get the line end
                    Move_Plume( wxPoint( cte, start - len ), 'D' );
                }
            }

            if( DrawPinNum )
            {
                PlotGraphicText( g_PlotFormat, wxPoint( x1 + TXTMARGE, (y1 + pin_pos.y) / 2 ),
                    NumColor, StringPinNum,
                    TEXT_ORIENT_VERT, PinNumSize,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER );
            }
        }
    }
}
