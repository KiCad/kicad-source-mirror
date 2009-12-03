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


/* Plot sheet references
 * margin is in mils (1/1000 inch)
 */
void WinEDA_DrawFrame::PlotWorkSheet( PLOTTER* plotter, BASE_SCREEN* screen )
{
#define WSTEXTSIZE 50   // Text size in mils
    Ki_PageDescr* Sheet = screen->m_CurrentSheetDesc;
    int           xg, yg, ipas, gxpas, gypas;
    wxSize        PageSize;
    wxPoint       pos, ref;
    EDA_Colors    color;

    /* Scale to convert dimension in 1/1000 in into internal units
     * (1/1000 inc for EESchema, 1/10000 for pcbnew. */
    int      conv_unit = screen->GetInternalUnits() / 1000;
    wxString msg;
    wxSize   text_size;
#if defined(KICAD_GOST)
    wxSize   text_size2;
    wxSize   text_size3;
    wxSize   text_size1_5;
#endif
    int      UpperLimit = VARIABLE_BLOCK_START_POSITION;
    bool     italic     = false;
    bool     bold = false;
    bool     thickness = 0;      //@todo : use current pen

    color = BLACK;
    plotter->set_color( color );

    PageSize.x = Sheet->m_Size.x;
    PageSize.y = Sheet->m_Size.y;

    /* Plot edge. */
    ref.x = Sheet->m_LeftMargin * conv_unit;
    ref.y = Sheet->m_TopMargin * conv_unit;
    xg    = ( PageSize.x - Sheet->m_RightMargin ) * conv_unit;
    yg    = ( PageSize.y - Sheet->m_BottomMargin ) * conv_unit;

#if defined(KICAD_GOST)
    plotter->move_to( ref );
    pos.x = xg;
    pos.y = ref.y;
    plotter->line_to( pos );
    pos.x = xg;
    pos.y = yg;
    plotter->line_to( pos );
    pos.x = ref.x;
    pos.y = yg;
    plotter->line_to( pos );
    plotter->finish_to( ref );
#else
    for( unsigned ii = 0; ii < 2; ii++ )
    {
        plotter->move_to( ref );
        pos.x = xg;
        pos.y = ref.y;
        plotter->line_to( pos );
        pos.x = xg;
        pos.y = yg;
        plotter->line_to( pos );
        pos.x = ref.x;
        pos.y = yg;
        plotter->line_to( pos );
        plotter->finish_to( ref );
        ref.x += GRID_REF_W * conv_unit;
        ref.y += GRID_REF_W * conv_unit;
        xg    -= GRID_REF_W * conv_unit;
        yg    -= GRID_REF_W * conv_unit;
    }

#endif

    text_size.x = WSTEXTSIZE * conv_unit;
    text_size.y = WSTEXTSIZE * conv_unit;

    ref.x = Sheet->m_LeftMargin;
    ref.y = Sheet->m_TopMargin;                         /* Upper left corner in
                                                         * 1/1000 inch */
    xg    = ( PageSize.x - Sheet->m_RightMargin );
    yg    = ( PageSize.y - Sheet->m_BottomMargin );     /* lower right corner
                                                         * in 1/1000 inch */

#if defined(KICAD_GOST)
    for( Ki_WorkSheetData* WsItem = &WS_Segm1_LU;
         WsItem != NULL;
         WsItem = WsItem->Pnext )
    {
        pos.x = ( ref.x - WsItem->m_Posx ) * conv_unit;
        pos.y = ( yg - WsItem->m_Posy ) * conv_unit;
        msg.Empty();
        switch( WsItem->m_Type )
        {
        case WS_CADRE:
            break;

        case WS_PODPIS_LU:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            plotter->text( pos, color,
                           msg, TEXT_ORIENT_VERT, text_size,
                           GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM,
                           thickness, italic, false );
            break;

        case WS_SEGMENT_LU:
            plotter->move_to( pos );
            pos.x = ( ref.x - WsItem->m_Endx ) * conv_unit;
            pos.y = ( yg - WsItem->m_Endy ) * conv_unit;
            plotter->finish_to( pos );
            break;
        }
    }

    for( Ki_WorkSheetData* WsItem = &WS_Segm1_LT;
         WsItem != NULL;
         WsItem = WsItem->Pnext )
    {
        pos.x = ( ref.x + WsItem->m_Posx ) * conv_unit;
        pos.y = ( ref.y + WsItem->m_Posy ) * conv_unit;
        msg.Empty();
        switch( WsItem->m_Type )
        {
        case WS_SEGMENT_LT:
            plotter->move_to( pos );
            pos.x = ( ref.x + WsItem->m_Endx ) * conv_unit;
            pos.y = ( ref.y + WsItem->m_Endy ) * conv_unit;
            plotter->finish_to( pos );
            break;
        }
    }

#else

    /* Plot legend along the X axis. */
    ipas  = ( xg - ref.x ) / PAS_REF;
    gxpas = ( xg - ref.x ) / ipas;
    for( int ii = ref.x + gxpas, jj = 1; ipas > 0; ii += gxpas, jj++, ipas-- )
    {
        msg.Empty();
        msg << jj;

        if( ii < xg - PAS_REF / 2 )
        {
            pos.x = ii * conv_unit;
            pos.y = ref.y * conv_unit;
            plotter->move_to( pos );
            pos.x = ii * conv_unit;
            pos.y = ( ref.y + GRID_REF_W ) * conv_unit;
            plotter->finish_to( pos );
        }

        pos.x = ( ii - gxpas / 2 ) * conv_unit;
        pos.y = ( ref.y + GRID_REF_W / 2 ) * conv_unit;
        plotter->text( pos, color,
                       msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       thickness, italic, false );

        if( ii < xg - PAS_REF / 2 )
        {
            pos.x = ii * conv_unit;
            pos.y = yg * conv_unit;
            plotter->move_to( pos );
            pos.x = ii * conv_unit;
            pos.y = (yg - GRID_REF_W) * conv_unit;
            plotter->finish_to( pos );
        }
        pos.x = ( ii - gxpas / 2 ) * conv_unit;
        pos.y = ( yg - GRID_REF_W / 2 ) * conv_unit;
        plotter->text( pos, color,
                       msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       thickness, italic, false );
    }

    /* Plot legend along the Y axis. */
    ipas  = ( yg - ref.y ) / PAS_REF;
    gypas = (  yg - ref.y ) / ipas;
    for( int ii = ref.y + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        if( jj < 26 )
            msg.Printf( wxT( "%c" ), jj + 'A' );
        else    // I hope 52 identifiers are enough...
            msg.Printf( wxT( "%c" ), 'a' + jj - 26 );
        if( ii < yg - PAS_REF / 2 )
        {
            pos.x = ref.x * conv_unit;
            pos.y = ii * conv_unit;
            plotter->move_to( pos );
            pos.x = ( ref.x + GRID_REF_W ) * conv_unit;
            pos.y = ii * conv_unit;
            plotter->finish_to( pos );
        }
        pos.x = ( ref.x + GRID_REF_W / 2 ) * conv_unit;
        pos.y = ( ii - gypas / 2 ) * conv_unit;
        plotter->text( pos, color,
                       msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       thickness, italic, false );

        if( ii < yg - PAS_REF / 2 )
        {
            pos.x = xg * conv_unit;
            pos.y = ii * conv_unit;
            plotter->move_to( pos );
            pos.x = ( xg - GRID_REF_W ) * conv_unit;
            pos.y = ii * conv_unit;
            plotter->finish_to( pos );
        }

        pos.x = ( xg - GRID_REF_W / 2 ) * conv_unit;
        pos.y = ( ii - gypas / 2 ) * conv_unit;
        plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       thickness, italic, false );
    }

#endif

    /* Plot the worksheet. */
    text_size.x = SIZETEXT * conv_unit;
    text_size.y = SIZETEXT * conv_unit;
#if defined(KICAD_GOST)
    text_size2.x = SIZETEXT * conv_unit * 2;
    text_size2.y = SIZETEXT * conv_unit * 2;
    text_size3.x = SIZETEXT * conv_unit * 3;
    text_size3.y = SIZETEXT * conv_unit * 3;
    text_size1_5.x = SIZETEXT * conv_unit * 1.5;
    text_size1_5.y = SIZETEXT * conv_unit * 1.5;
    ref.x = PageSize.x - Sheet->m_RightMargin;
    ref.y = PageSize.y - Sheet->m_BottomMargin;

    if( screen->m_ScreenNumber == 1 )
    {
        for( Ki_WorkSheetData* WsItem = &WS_Date;
             WsItem != NULL;
             WsItem = WsItem->Pnext )
        {
            pos.x = ( ref.x - WsItem->m_Posx ) * conv_unit;
            pos.y = ( ref.y - WsItem->m_Posy ) * conv_unit;
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
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                plotter->text( pos, color,
                               msg, TEXT_ORIENT_HORIZ, text_size,
                               GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                               thickness, italic, false );
                break;

            case WS_SIZESHEET:
                break;

            case WS_IDENTSHEET:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                msg << screen->m_ScreenNumber;
                plotter->text( pos, color,
                               msg, TEXT_ORIENT_HORIZ, text_size,
                               GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                               thickness, italic, false );
                break;

            case WS_SHEETS:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                msg << screen->m_NumberOfScreen;
                plotter->text( pos, color,
                               msg, TEXT_ORIENT_HORIZ, text_size,
                               GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                               thickness, italic, false );
                break;

            case WS_COMPANY_NAME:
                msg = screen->m_Company;
                if( !msg.IsEmpty() )
                {
                    plotter->text( pos, color,
                                   msg, TEXT_ORIENT_HORIZ, text_size1_5,
                                   GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                }
                break;

            case WS_TITLE:
                msg = screen->m_Title;
                if( !msg.IsEmpty() )
                {
                    plotter->text( pos, color,
                                   msg, TEXT_ORIENT_HORIZ, text_size1_5,
                                   GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                }
                break;

            case WS_COMMENT1:
                msg = screen->m_Commentaire1;
                if( !msg.IsEmpty() )
                {
                    plotter->text( pos, color,
                                   msg, TEXT_ORIENT_HORIZ, text_size3,
                                   GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                    pos.x = (Sheet->m_LeftMargin + 1260) * conv_unit;
                    pos.y = (Sheet->m_TopMargin + 270) * conv_unit;
                    plotter->text( pos, color,
                                   msg.GetData(), 1800, text_size2,
                                   GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                }
                break;

            case WS_COMMENT2:
                msg = screen->m_Commentaire2;
                if( !msg.IsEmpty() )
                {
                    plotter->text( pos, color,
                                   msg, TEXT_ORIENT_HORIZ, text_size,
                                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                }
                break;

            case WS_COMMENT3:
                msg = screen->m_Commentaire3;
                if( !msg.IsEmpty() )
                {
                    plotter->text( pos, color,
                                   msg, TEXT_ORIENT_HORIZ, text_size,
                                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                }
                break;

            case WS_COMMENT4:
                msg = screen->m_Commentaire4;
                if( !msg.IsEmpty() )
                {
                    plotter->text( pos, color,
                                   msg, TEXT_ORIENT_HORIZ, text_size,
                                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                }
                break;

            case WS_UPPER_SEGMENT:
            case WS_LEFT_SEGMENT:
            case WS_SEGMENT:
                plotter->move_to( pos );
                pos.x = ( ref.x - WsItem->m_Endx ) * conv_unit;
                pos.y = ( ref.y - WsItem->m_Endy ) * conv_unit;
                plotter->finish_to( pos );
                break;
            }
        }
    }
    else
    {
        for( Ki_WorkSheetData* WsItem = &WS_CADRE_D;
             WsItem != NULL;
             WsItem = WsItem->Pnext )
        {
            pos.x = ( ref.x - WsItem->m_Posx ) * conv_unit;
            pos.y = ( ref.y - WsItem->m_Posy ) * conv_unit;
            msg.Empty();

            switch( WsItem->m_Type )
            {
            case WS_CADRE:
            /* Begin list number > 1 */
                msg = screen->m_Commentaire1;
                if( !msg.IsEmpty() )
                {
                    plotter->text( pos, color,
                                   msg, TEXT_ORIENT_HORIZ, text_size3,
                                   GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                    pos.x = (Sheet->m_LeftMargin + 1260) * conv_unit;
                    pos.y = (Sheet->m_TopMargin + 270) * conv_unit;
                    plotter->text( pos, color,
                                   msg, 1800, text_size2,
                                   GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                   thickness, italic, false );
                }
                break;

            case WS_PODPIS_D:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ, text_size,
                               GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                               thickness, italic, false );
                break;

            case WS_IDENTSHEET_D:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                msg << screen->m_ScreenNumber;
                plotter->text( pos, color, msg, TEXT_ORIENT_HORIZ, text_size,
                               GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                               thickness, italic, false );
                break;

            case WS_LEFT_SEGMENT_D:
            case WS_SEGMENT_D:
                plotter->move_to( pos );
                pos.x = ( ref.x - WsItem->m_Endx ) * conv_unit;
                pos.y = ( ref.y - WsItem->m_Endy ) * conv_unit;
                plotter->finish_to( pos );
                break;
            }
        }
    }
#else
    ref.x = PageSize.x - GRID_REF_W - Sheet->m_RightMargin;
    ref.y = PageSize.y - GRID_REF_W - Sheet->m_BottomMargin;

    for( Ki_WorkSheetData* WsItem = &WS_Date;
         WsItem != NULL;
         WsItem = WsItem->Pnext )
    {
        pos.x = ( ref.x - WsItem->m_Posx ) * conv_unit;
        pos.y = ( ref.y - WsItem->m_Posy ) * conv_unit;
        bold  = false;
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
            msg << screen->m_ScreenNumber << wxT( "/" ) <<
                screen->m_NumberOfScreen;
            break;

        case WS_FILENAME:
        {
            wxString fname, fext;
            wxFileName::SplitPath( screen->m_FileName,
                                   (wxString*) NULL,
                                   &fname,
                                   &fext );
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
            WS_MostUpperLine.m_Posy = WS_MostUpperLine.m_Endy
                = WS_MostLeftLine.m_Posy = UpperLimit;
            pos.y = (ref.y - WsItem->m_Posy) * conv_unit;

        case WS_SEGMENT:
        {
            wxPoint auxpos;
            auxpos.x = ( ref.x - WsItem->m_Endx ) * conv_unit;;
            auxpos.y = ( ref.y - WsItem->m_Endy ) * conv_unit;;
            plotter->move_to( pos );
            plotter->finish_to( auxpos );
        }
        break;
        }

        if( !msg.IsEmpty() )
        {
            plotter->text( pos, color,
                           msg, TEXT_ORIENT_HORIZ, text_size,
                           GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                           thickness, italic, bold );
        }
    }

#endif
}
