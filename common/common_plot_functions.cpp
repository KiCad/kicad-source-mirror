/**
 * @file common_plot_functions.cpp
 * @brief Kicad: Common plotting Routines
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <common.h>
#include <plot_common.h>
#include <worksheet.h>
#include <macros.h>
#include <class_base_screen.h>
#include <drawtxt.h>
#include <class_title_block.h>


wxString GetDefaultPlotExtension( PlotFormat aFormat )
{
    switch( aFormat )
    {
    case PLOT_FORMAT_DXF:
        return DXF_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_POST:
        return PS_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_PDF:
        return PDF_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_HPGL:
        return HPGL_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_GERBER:
        return GERBER_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_SVG:
        return SVG_PLOTTER::GetDefaultFileExtension();

    default:
        wxASSERT( false );
        return wxEmptyString;
    }
}

/* Plot sheet references
 * margin is in mils (1/1000 inch)
 */
void PlotWorkSheet( PLOTTER* plotter, const TITLE_BLOCK& aTitleBlock,
                    const PAGE_INFO& aPageInfo,
                    int aSheetNumber, int aNumberOfSheets,
                    const wxString &aSheetDesc,
                    const wxString &aFilename )
{
    int      iusPerMil = plotter->GetIUsPerDecimil() * 10;
    wxSize   pageSize = aPageInfo.GetSizeMils();  // in mils
    int      xg, yg;

    wxPoint  pos, end, ref;
    wxString msg;
    wxSize   text_size;

    EDA_COLOR_T      plotClr;
    plotClr = plotter->GetColorMode() ? RED : BLACK;
    plotter->SetColor( plotClr );
    plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );

    // Plot edge.
    ref.x = aPageInfo.GetLeftMarginMils() * iusPerMil;
    ref.y = aPageInfo.GetTopMarginMils()  * iusPerMil;

    xg    = ( pageSize.x - aPageInfo.GetRightMarginMils() )  * iusPerMil;
    yg    = ( pageSize.y - aPageInfo.GetBottomMarginMils() ) * iusPerMil;

#if defined(KICAD_GOST)
    int refx, refy;
    int lnMsg, ln;
    text_size.x = SIZETEXT * iusPerMil;
    text_size.y = SIZETEXT * iusPerMil;
    wxSize sz;
    wxSize text_size0_8( SIZETEXT * iusPerMil * 0.8, SIZETEXT * iusPerMil * 1 );
    wxSize text_size1_5( SIZETEXT * iusPerMil * 1.5, SIZETEXT * iusPerMil * 1.5 );
    wxSize text_size2( SIZETEXT * iusPerMil * 2, SIZETEXT * iusPerMil * 2 );
    wxSize text_size3( SIZETEXT * iusPerMil * 3, SIZETEXT * iusPerMil * 3 );
    int lineOsn_widht = plotter->GetCurrentLineWidth() * 2;
    int lineTonk_widht = plotter->GetCurrentLineWidth();

    plotter->SetCurrentLineWidth( lineOsn_widht );
    plotter->MoveTo( ref );
    pos.x = xg;
    pos.y = ref.y;
    plotter->LineTo( pos );
    pos.x = xg;
    pos.y = yg;
    plotter->LineTo( pos );
    pos.x = ref.x;
    pos.y = yg;
    plotter->LineTo( pos );
    plotter->FinishTo( ref );
    plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );

#else
    const int WSTEXTSIZE = 50; // Text size in mils
    int      UpperLimit = VARIABLE_BLOCK_START_POSITION;

    for( unsigned ii = 0; ii < 2; ii++ )
    {
        plotter->MoveTo( ref );

        pos.x = xg;
        pos.y = ref.y;
        plotter->LineTo( pos );

        pos.x = xg;
        pos.y = yg;
        plotter->LineTo( pos );

        pos.x = ref.x;
        pos.y = yg;
        plotter->LineTo( pos );

        plotter->FinishTo( ref );

        ref.x += GRID_REF_W * iusPerMil;
        ref.y += GRID_REF_W * iusPerMil;

        xg    -= GRID_REF_W * iusPerMil;
        yg    -= GRID_REF_W * iusPerMil;
    }

#endif

    // upper left corner in mils
    ref.x = aPageInfo.GetLeftMarginMils();
    ref.y = aPageInfo.GetTopMarginMils();

    // lower right corner in mils
    xg    = ( pageSize.x - aPageInfo.GetRightMarginMils() );
    yg    = ( pageSize.y - aPageInfo.GetBottomMarginMils() );

#if defined(KICAD_GOST)

    // Lower right corner
    refx = xg;
    refy = yg;

    // First page
    if( aSheetNumber == 1 )
    {
        for( Ki_WorkSheetData* WsItem = &WS_Osn1_Line1;
             WsItem != NULL;
             WsItem = WsItem->Pnext )
        {
            pos.x = (refx - WsItem->m_Posx) * iusPerMil;
            pos.y = (refy - WsItem->m_Posy) * iusPerMil;
            end.x = (refx - WsItem->m_Endx) * iusPerMil;
            end.y = (refy - WsItem->m_Endy) * iusPerMil;
            msg = WsItem->m_Legende;
            switch( WsItem->m_Type )
            {
            case WS_OSN:
                plotter->SetCurrentLineWidth( lineOsn_widht );
                plotter->MoveTo( pos );
                plotter->FinishTo( end );
                plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
                break;

            case WS_TONK:
                plotter->SetCurrentLineWidth( lineTonk_widht );
                plotter->MoveTo( pos );
                plotter->FinishTo( end );
                plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
                break;

            case WS_TEXT:
                if( !msg.IsEmpty() )
                {
                    if( WsItem == &WS_Osn1_Text1 )
                        plotter->Text( pos, plotClr,
                                       msg, TEXT_ORIENT_HORIZ, text_size0_8,
                                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );
                    else
                        plotter->Text( pos, plotClr,
                                       msg, TEXT_ORIENT_HORIZ, text_size,
                                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );
                }
                break;

            case WS_TEXTL:
                if( !msg.IsEmpty() )
                    plotter->Text( pos, plotClr,
                                   msg, TEXT_ORIENT_HORIZ, text_size,
                                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                   PLOTTER::DEFAULT_LINE_WIDTH, false, false );
                break;

            }
        }

        // Sheet number
        if( aNumberOfSheets > 1 )
        {
            pos.x = (refx - Mm2mils( 36 )) * iusPerMil;
            pos.y = (refy - Mm2mils( 17.5 )) * iusPerMil;
            msg.Empty();
            msg << aSheetNumber;
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_HORIZ, text_size,
                           GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }

        // Count of sheets
        pos.x = (refx - Mm2mils( 10 )) * iusPerMil;
        pos.y = (refy - Mm2mils( 17.5 )) * iusPerMil;
        msg.Empty();
        msg << aNumberOfSheets;
        plotter->Text( pos, plotClr,
                       msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );

        // Company name
        msg = aTitleBlock.GetCompany();
        if( !msg.IsEmpty() )
        {
            sz = text_size1_5;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
            ln = Mm2mils( 49 );
            if( lnMsg > ln )
                sz.x *= double( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 25 )) * iusPerMil;
            pos.y = (refy - Mm2mils( 7.5 )) * iusPerMil;
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_HORIZ, sz,
                           GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }

        // Title
        msg = aTitleBlock.GetTitle();
        if( !msg.IsEmpty() )
        {
            sz = text_size1_5;
            wxArrayString lines;
            int titleWidth = 0;
            int titleHeight = (sz.y + sz.y * 0.5) / iusPerMil;
            int titleFieldWidth = Mm2mils( 69 );
            int titleFieldHeight = Mm2mils( 24 );
            int index = 0;
            wxString fullMsg = msg;
            do // Reduce the height of wrapped title until the fit
            {
                do // Wrap the title
                {
                    titleWidth = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
                    if( titleWidth > titleFieldWidth )
                    {
                        index = 0;
                        do
                        {
                            msg = msg.Left( msg.Length() - 1 );
                            if( msg.Length() == 0 )
                            {
                                lines.Clear();
                                msg = fullMsg;
                                sz.x -= iusPerMil;
                                break;
                            }
                            else
                            {
                                index++;
                                titleWidth = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;

                                wxString ch = wxString( msg.Last() );
                                if( titleWidth < titleFieldWidth && ch == wxT( " " ) )
                                {
                                    // New sentence on a new line
                                    int dot = msg.Index( wxT( ". " ) );
                                    if( dot != wxNOT_FOUND )
                                    {
                                        index += msg.Length() - dot - 2;
                                        msg = msg.Left( dot + 1 );
                                        lines.Add( msg );
                                        msg = fullMsg.Right( index );
                                        break;
                                    }
                                    else
                                    {
                                        msg = msg.Left( msg.Length() - 1 );
                                        lines.Add( msg );
                                        msg = fullMsg.Right( index );
                                        break;
                                    }
                                }
                            }
                        }while( 1 );
                    }
                    else
                    {
                        // New sentence on a new line
                        int dot = msg.Index( wxT( ". " ) );
                        if( dot != wxNOT_FOUND )
                        {
                            lines.Add( msg.Left( dot + 1 ) );
                            lines.Add( fullMsg.Right( msg.Length() - dot - 2 ) );
                        }
                        else
                            lines.Add( msg );
                        break;
                     }
                }while( 1 );

                if( titleFieldHeight < titleHeight * lines.Count() )
                {
                    sz.y -= iusPerMil;
                    sz.x -= iusPerMil;
                    msg = fullMsg;
                    lines.Clear();
                }
                else
                    break;
            }while( 1 );

            pos.x = (refx - Mm2mils( 85 )) * iusPerMil;
            pos.y = (refy - Mm2mils( 27.5 ) - (titleHeight * (lines.Count() - 1) / 2)) * iusPerMil;

            for( int curLn = 0; curLn < lines.Count(); curLn++ )
            {
                msg = lines[curLn];
                plotter->Text( pos, plotClr,
                               msg, TEXT_ORIENT_HORIZ, sz,
                               GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                               PLOTTER::DEFAULT_LINE_WIDTH, false, false );
                pos.y += titleHeight * iusPerMil;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = text_size3;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
            ln = Mm2mils( 119 );
            if( lnMsg > ln )
                sz.x *= double( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 60 )) * iusPerMil;
            pos.y = (refy - Mm2mils( 47.5 )) * iusPerMil;
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_HORIZ, sz,
                           GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }

        // Developer
        msg = aTitleBlock.GetComment2();
        if( !msg.IsEmpty() )
        {
            sz = text_size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= double( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167.5 )) * iusPerMil;
            pos.y = (refy - Mm2mils( 27.5 )) * iusPerMil;
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_HORIZ, sz,
                           GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }

        // Verifier
        msg = aTitleBlock.GetComment3();
        if( !msg.IsEmpty() )
        {
            sz = text_size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= double( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167.5 )) * iusPerMil;
            pos.y = (refy - Mm2mils( 22.5 )) * iusPerMil;
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_HORIZ, sz,
                           GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }

        // Approver
        msg = aTitleBlock.GetComment4();
        if( !msg.IsEmpty() )
        {
            sz = text_size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= double( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167.5 )) * iusPerMil;
            pos.y = (refy - Mm2mils( 2.5 )) * iusPerMil;
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_HORIZ, sz,
                           GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }
    }
    else // other pages
    {
        for( Ki_WorkSheetData* WsItem = &WS_Osn2a_Line1;
             WsItem != NULL;
             WsItem = WsItem->Pnext )
        {
            pos.x = (refx - WsItem->m_Posx) * iusPerMil;
            pos.y = (refy - WsItem->m_Posy) * iusPerMil;
            end.x = (refx - WsItem->m_Endx) * iusPerMil;
            end.y = (refy - WsItem->m_Endy) * iusPerMil;
            msg = WsItem->m_Legende;
            switch( WsItem->m_Type )
            {
            case WS_OSN:
                plotter->SetCurrentLineWidth( lineOsn_widht );
                plotter->MoveTo( pos );
                plotter->FinishTo( end );
                plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
                break;

            case WS_TONK:
                plotter->SetCurrentLineWidth( lineTonk_widht );
                plotter->MoveTo( pos );
                plotter->FinishTo( end );
                plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
                break;

            case WS_TEXT:
                if( !msg.IsEmpty() )
                {
                    if( WsItem == &WS_Osn2a_Text1 )
                        plotter->Text( pos, plotClr,
                                       msg, TEXT_ORIENT_HORIZ, text_size0_8,
                                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );
                    else
                        plotter->Text( pos, plotClr,
                                       msg, TEXT_ORIENT_HORIZ, text_size,
                                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );
                }
                break;

            case WS_TEXTL:
                if( !msg.IsEmpty() )
                    plotter->Text( pos, plotClr,
                                   msg, TEXT_ORIENT_HORIZ, text_size,
                                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                   PLOTTER::DEFAULT_LINE_WIDTH, false, false );
                break;
            }
        }

        // Sheet number
        pos.x = (refx - Mm2mils( 5 )) * iusPerMil;
        pos.y = (refy - Mm2mils( 4 )) * iusPerMil;
        msg.Empty();
        msg << aSheetNumber;
        plotter->Text( pos, plotClr,
                       msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );

        // Decimal number
        msg = aTitleBlock.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = text_size3;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
            ln = Mm2mils( 109 );
            if( lnMsg > ln )
                sz.x *= double( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 65 )) * iusPerMil;
            pos.y = (refy - Mm2mils( 7.5 )) * iusPerMil;
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_HORIZ, sz,
                           GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }
    }

    // Format
    pos.x = (refx - Mm2mils( 23 )) * iusPerMil;
    pos.y = (refy + Mm2mils( 2.5 )) * iusPerMil;
    msg.Empty();
    msg << aPageInfo.GetType();
    plotter->Text( pos, plotClr,
                   msg, TEXT_ORIENT_HORIZ, text_size,
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                   PLOTTER::DEFAULT_LINE_WIDTH, false, false );

    // Lower left corner
    refx = ref.x;
    refy = yg;
    for( Ki_WorkSheetData* WsItem = &WS_DopLeft_Line1;
         WsItem != NULL;
         WsItem = WsItem->Pnext )
    {
        if( aSheetNumber > 1 && WsItem == &WS_DopLeft_Line9 ) // Some fields for first page only
            break;

        pos.x = (refx - WsItem->m_Posx) * iusPerMil;
        pos.y = (refy - WsItem->m_Posy) * iusPerMil;
        end.x = (refx - WsItem->m_Endx) * iusPerMil;
        end.y = (refy - WsItem->m_Endy) * iusPerMil;
        msg = WsItem->m_Legende;
        switch( WsItem->m_Type )
        {
        case WS_OSN:
            plotter->SetCurrentLineWidth( lineOsn_widht );
            plotter->MoveTo( pos );
            plotter->FinishTo( end );
            plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
            break;

        case WS_TONK:
            plotter->SetCurrentLineWidth( lineTonk_widht );
            plotter->MoveTo( pos );
            plotter->FinishTo( end );
            plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
            break;

        case WS_TEXT:
            if( !msg.IsEmpty() )
                plotter->Text( pos, plotClr,
                               msg, TEXT_ORIENT_VERT, text_size,
                               GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                               PLOTTER::DEFAULT_LINE_WIDTH, false, false );
            break;
        }
    }

    if( aPageInfo.GetType() == PAGE_INFO::A4 || !aPageInfo.IsPortrait() ) // A4 or Landscape
    {
        // Left Top corner
        refx = ref.x;
        refy = ref.y;
        for( Ki_WorkSheetData* WsItem = &WS_DopTop_Line1;
             WsItem != NULL;
             WsItem = WsItem->Pnext )
        {
            if( aSheetNumber > 1 && WsItem == &WS_DopTop_Line3 )// Some fields for first page only
                break;

            pos.x = (refx + WsItem->m_Posx) * iusPerMil;
            pos.y = (refy + WsItem->m_Posy) * iusPerMil;
            end.x = (refx + WsItem->m_Endx) * iusPerMil;
            end.y = (refy + WsItem->m_Endy) * iusPerMil;
            msg = WsItem->m_Legende;
            switch( WsItem->m_Type )
            {
            case WS_OSN:
                plotter->SetCurrentLineWidth( lineOsn_widht );
                plotter->MoveTo( pos );
                plotter->FinishTo( end );
                plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
                break;

            case WS_TONK:
                plotter->SetCurrentLineWidth( lineTonk_widht );
                plotter->MoveTo( pos );
                plotter->FinishTo( end );
                plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
                break;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = text_size2;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
            ln = Mm2mils( 69 );
            if( lnMsg > ln )
                sz.x *= double( ln ) / lnMsg;
            pos.x = (refx + Mm2mils( 35 )) * iusPerMil;
            pos.y = (refy + Mm2mils( 7 )) * iusPerMil;
            plotter->Text( pos, plotClr,
                           msg, 1800, sz,
                           GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }
    }
    else // Portrait
    {
        // Right Top corner
        // Lines are used from the upper left corner by the change of coordinates
        refx = xg;
        refy = ref.y;
        for( Ki_WorkSheetData* WsItem = &WS_DopTop_Line1;
             WsItem != NULL;
             WsItem = WsItem->Pnext )
        {
            if( aSheetNumber > 1 && WsItem == &WS_DopTop_Line3 )// Some fields for first page only
                break;

            pos.x = (refx - WsItem->m_Posy) * iusPerMil;
            pos.y = (refy + WsItem->m_Posx) * iusPerMil;
            end.x = (refx - WsItem->m_Endy) * iusPerMil;
            end.y = (refy + WsItem->m_Endx) * iusPerMil;
            msg = WsItem->m_Legende;
            switch( WsItem->m_Type )
            {
            case WS_OSN:
                plotter->SetCurrentLineWidth( lineOsn_widht );
                plotter->MoveTo( pos );
                plotter->FinishTo( end );
                plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
                break;

            case WS_TONK:
                plotter->SetCurrentLineWidth( lineTonk_widht );
                plotter->MoveTo( pos );
                plotter->FinishTo( end );
                plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
                break;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = text_size2;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / iusPerMil;
            ln = Mm2mils( 69 );
            if( lnMsg > ln )
                sz.x *= double( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 7 )) * iusPerMil;
            pos.y = (refy + Mm2mils( 35 )) * iusPerMil;
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_VERT, sz,
                           GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, false, false );
        }
    }

#else

    text_size.x = WSTEXTSIZE * iusPerMil;
    text_size.y = WSTEXTSIZE * iusPerMil;

    // Plot legend along the X axis.
    int ipas  = ( xg - ref.x ) / PAS_REF;
    int gxpas = ( xg - ref.x ) / ipas;
    for( int ii = ref.x + gxpas, jj = 1; ipas > 0; ii += gxpas, jj++, ipas-- )
    {
        msg.Empty();
        msg << jj;

        if( ii < xg - PAS_REF / 2 )
        {
            pos.x = ii * iusPerMil;
            pos.y = ref.y * iusPerMil;
            plotter->MoveTo( pos );
            pos.x = ii * iusPerMil;
            pos.y = ( ref.y + GRID_REF_W ) * iusPerMil;
            plotter->FinishTo( pos );
        }

        pos.x = ( ii - gxpas / 2 ) * iusPerMil;
        pos.y = ( ref.y + GRID_REF_W / 2 ) * iusPerMil;
        plotter->Text( pos, plotClr,
                       msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );

        if( ii < xg - PAS_REF / 2 )
        {
            pos.x = ii * iusPerMil;
            pos.y = yg * iusPerMil;
            plotter->MoveTo( pos );
            pos.x = ii * iusPerMil;
            pos.y = (yg - GRID_REF_W) * iusPerMil;
            plotter->FinishTo( pos );
        }
        pos.x = ( ii - gxpas / 2 ) * iusPerMil;
        pos.y = ( yg - GRID_REF_W / 2 ) * iusPerMil;
        plotter->Text( pos, plotClr,
                       msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );
    }

    // Plot legend along the Y axis.
    ipas  = ( yg - ref.y ) / PAS_REF;
    int gypas = (  yg - ref.y ) / ipas;
    for( int ii = ref.y + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        if( jj < 26 )
            msg.Printf( wxT( "%c" ), jj + 'A' );
        else    // I hope 52 identifiers are enough...
            msg.Printf( wxT( "%c" ), 'a' + jj - 26 );
        if( ii < yg - PAS_REF / 2 )
        {
            pos.x = ref.x * iusPerMil;
            pos.y = ii * iusPerMil;
            plotter->MoveTo( pos );
            pos.x = ( ref.x + GRID_REF_W ) * iusPerMil;
            pos.y = ii * iusPerMil;
            plotter->FinishTo( pos );
        }
        pos.x = ( ref.x + GRID_REF_W / 2 ) * iusPerMil;
        pos.y = ( ii - gypas / 2 ) * iusPerMil;
        plotter->Text( pos, plotClr,
                       msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );

        if( ii < yg - PAS_REF / 2 )
        {
            pos.x = xg * iusPerMil;
            pos.y = ii * iusPerMil;
            plotter->MoveTo( pos );
            pos.x = ( xg - GRID_REF_W ) * iusPerMil;
            pos.y = ii * iusPerMil;
            plotter->FinishTo( pos );
        }

        pos.x = ( xg - GRID_REF_W / 2 ) * iusPerMil;
        pos.y = ( ii - gypas / 2 ) * iusPerMil;
        plotter->Text( pos, plotClr, msg, TEXT_ORIENT_HORIZ, text_size,
                       GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                       PLOTTER::DEFAULT_LINE_WIDTH, false, false );
    }

    // Plot the worksheet.
    text_size.x = SIZETEXT * iusPerMil;
    text_size.y = SIZETEXT * iusPerMil;

    ref.x = pageSize.x - GRID_REF_W - aPageInfo.GetRightMarginMils();
    ref.y = pageSize.y - GRID_REF_W - aPageInfo.GetBottomMarginMils();

    for( Ki_WorkSheetData* WsItem = &WS_Date;
         WsItem != NULL;
         WsItem = WsItem->Pnext )
    {
        bool bold = false;

        pos.x = ( ref.x - WsItem->m_Posx ) * iusPerMil;
        pos.y = ( ref.y - WsItem->m_Posy ) * iusPerMil;
        if( WsItem->m_Legende )
            msg = WsItem->m_Legende;
        else
            msg.Empty();

        switch( WsItem->m_Type )
        {
        case WS_DATE:
            msg += aTitleBlock.GetDate();
            bold = true;
            break;

        case WS_REV:
            msg += aTitleBlock.GetRevision();
            bold = true;
            break;

        case WS_KICAD_VERSION:
            msg += g_ProductName;
            break;

        case WS_SIZESHEET:
            msg += aPageInfo.GetType();
            break;

        case WS_IDENTSHEET:
            msg << aSheetNumber << wxT( "/" ) << aNumberOfSheets;
            break;

        case WS_FILENAME:
        {
            wxString fname, fext;
            wxFileName::SplitPath( aFilename, NULL, &fname, &fext );
            msg << fname << wxT( "." ) << fext;
        }
        break;

        case WS_FULLSHEETNAME:
            msg += aSheetDesc;
            break;

        case WS_COMPANY_NAME:
            msg += aTitleBlock.GetCompany();
            if( !msg.IsEmpty() )
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            bold = true;
            break;

        case WS_TITLE:
            msg += aTitleBlock.GetTitle();
            bold = true;
            break;

        case WS_COMMENT1:
            msg += aTitleBlock.GetComment1();
            if( !msg.IsEmpty() )
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            break;

        case WS_COMMENT2:
            msg += aTitleBlock.GetComment2();
            if( !msg.IsEmpty() )
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            break;

        case WS_COMMENT3:
            msg += aTitleBlock.GetComment3();
            if( !msg.IsEmpty() )
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            break;

        case WS_COMMENT4:
            msg += aTitleBlock.GetComment4();
            if( !msg.IsEmpty() )
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            break;

        case WS_UPPER_SEGMENT:
            if( UpperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy = WS_MostUpperLine.m_Endy
                = WS_MostLeftLine.m_Posy = UpperLimit;
            pos.y = (ref.y - WsItem->m_Posy) * iusPerMil;

        case WS_SEGMENT:
        {
            wxPoint auxpos;
            auxpos.x = ( ref.x - WsItem->m_Endx ) * iusPerMil;
            auxpos.y = ( ref.y - WsItem->m_Endy ) * iusPerMil;
            plotter->MoveTo( pos );
            plotter->FinishTo( auxpos );
        }
        break;
        }

        if( !msg.IsEmpty() )
        {
            plotter->Text( pos, plotClr,
                           msg, TEXT_ORIENT_HORIZ, text_size,
                           GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                           PLOTTER::DEFAULT_LINE_WIDTH, bold, false );
        }
    }

#endif

}
