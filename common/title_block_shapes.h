/**
 * @file title_block_shape.h
 * @brief description of graphic items and texts to build a title block
 */

/*
 * This file should be included only in worksheet.cpp
 * This is not an usual .h file, it is more a .cpp file
 * it creates a lot of structures which define the shape of a title block
 * and frame references
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// The coordinates below are relative to the bottom right corner of page and
// will be subtracted from this origin.
#define BLOCK_OX                      4200
#define BLOCK_KICAD_VERSION_X         BLOCK_OX - SIZETEXT
#define BLOCK_KICAD_VERSION_Y         SIZETEXT
#define BLOCK_REV_X                   820
#define BLOCK_REV_Y                   (SIZETEXT * 3)
#define BLOCK_DATE_X                  BLOCK_OX - (SIZETEXT * 15)
#define BLOCK_DATE_Y                  (SIZETEXT * 3)
#define BLOCK_ID_SHEET_X              820
#define BLOCK_ID_SHEET_Y              SIZETEXT
#define BLOCK_SIZE_SHEET_X            BLOCK_OX - SIZETEXT
#define BLOCK_SIZE_SHEET_Y            (SIZETEXT * 3)
#define BLOCK_TITLE_X                 BLOCK_OX - SIZETEXT
#define BLOCK_TITLE_Y                 (SIZETEXT * 5)
#define BLOCK_FULLSHEETNAME_X         BLOCK_OX - SIZETEXT
#define BLOCK_FULLSHEETNAME_Y         (SIZETEXT * 7)
#define BLOCK_FILENAME_X              BLOCK_OX - SIZETEXT
#define BLOCK_FILENAME_Y              (SIZETEXT * 9)
#define BLOCK_COMMENT_X               BLOCK_OX - SIZETEXT
#define BLOCK_COMPANY_Y               (SIZETEXT * 11)
#define BLOCK_COMMENT1_Y              (SIZETEXT * 13)
#define BLOCK_COMMENT2_Y              (SIZETEXT * 15)
#define BLOCK_COMMENT3_Y              (SIZETEXT * 17)
#define BLOCK_COMMENT4_Y              (SIZETEXT * 19)


Ki_WorkSheetData WS_Date =
{
    WS_DATE,
    &WS_Licence,
    BLOCK_DATE_X,   BLOCK_DATE_Y,
    0,              0,
    wxT( "Date: " ),NULL
};

Ki_WorkSheetData WS_Licence =
{
    WS_KICAD_VERSION,
    &WS_Revision,
    BLOCK_KICAD_VERSION_X,BLOCK_KICAD_VERSION_Y,
    0,                    0,
    NULL,                 NULL
};

Ki_WorkSheetData WS_Revision =
{
    WS_REV,
    &WS_SizeSheet,
    BLOCK_REV_X,     BLOCK_REV_Y,
    0,               0,
    wxT( "Rev: " ),  NULL
};

Ki_WorkSheetData WS_SizeSheet =
{
    WS_SIZESHEET,
    &WS_IdentSheet,
    BLOCK_SIZE_SHEET_X, BLOCK_SIZE_SHEET_Y,
    0, 0,
    wxT( "Size: " ), NULL
};

Ki_WorkSheetData WS_IdentSheet =
{
    WS_IDENTSHEET,
    &WS_Title,
    BLOCK_ID_SHEET_X,BLOCK_ID_SHEET_Y,
    0,               0,
    wxT( "Id: " ),   NULL
};

Ki_WorkSheetData WS_Title =
{
    WS_TITLE,
    &WS_SheetFilename,
    BLOCK_TITLE_X,    BLOCK_TITLE_Y,
    0,                0,
    wxT( "Title: " ), NULL
};

Ki_WorkSheetData WS_SheetFilename =
{
    WS_FILENAME,
    &WS_FullSheetName,
    BLOCK_FILENAME_X, BLOCK_FILENAME_Y,
    0,                0,
    wxT( "File: " ),  NULL
};

Ki_WorkSheetData WS_FullSheetName =
{
    WS_FULLSHEETNAME,
    &WS_Company,
    BLOCK_FULLSHEETNAME_X,BLOCK_FULLSHEETNAME_Y,
    0,                    0,
    wxT( "Sheet: " ),     NULL
};

Ki_WorkSheetData WS_Company =
{
    WS_COMPANY_NAME,
    &WS_Comment1,
    BLOCK_COMMENT_X,BLOCK_COMPANY_Y,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment1 =
{
    WS_COMMENT1,
    &WS_Comment2,
    BLOCK_COMMENT_X,BLOCK_COMMENT1_Y,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment2 =
{
    WS_COMMENT2,
    &WS_Comment3,
    BLOCK_COMMENT_X,BLOCK_COMMENT2_Y,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment3 =
{
    WS_COMMENT3,
    &WS_Comment4,
    BLOCK_COMMENT_X,BLOCK_COMMENT3_Y,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment4 =
{
    WS_COMMENT4,
    &WS_MostLeftLine,
    BLOCK_COMMENT_X, BLOCK_COMMENT4_Y,
    0,               0,
    NULL,            NULL
};


/// Left vertical segment
Ki_WorkSheetData WS_MostLeftLine =
{
    WS_LEFT_SEGMENT,
    &WS_SeparatorLine,
    BLOCK_OX,         SIZETEXT * 16,
    BLOCK_OX,         0,
    NULL,             NULL
};


/// horizontal segment between filename and comments
Ki_WorkSheetData WS_SeparatorLine =
{
    WS_SEGMENT,
    &WS_MostUpperLine,
    BLOCK_OX,         VARIABLE_BLOCK_START_POSITION,
    0,                VARIABLE_BLOCK_START_POSITION,
    NULL,             NULL
};


/// superior horizontal segment
Ki_WorkSheetData WS_MostUpperLine =
{
    WS_UPPER_SEGMENT,
    &WS_Segm3,
    BLOCK_OX,        SIZETEXT * 16,
    0,               SIZETEXT * 16,
    NULL,            NULL
};


/// horizontal segment above COMPANY NAME
Ki_WorkSheetData WS_Segm3 =
{
    WS_SEGMENT,
    &WS_Segm4,
    BLOCK_OX,   SIZETEXT * 6,
    0,          SIZETEXT * 6,
    NULL,       NULL
};


/// vertical segment of the left REV and SHEET
Ki_WorkSheetData WS_Segm4 =
{
    WS_SEGMENT,
    &WS_Segm5,
    BLOCK_REV_X + SIZETEXT,SIZETEXT * 4,
    BLOCK_REV_X + SIZETEXT,0,
    NULL,                  NULL
};


Ki_WorkSheetData WS_Segm5 =
{
    WS_SEGMENT,
    &WS_Segm6,
    BLOCK_OX,  SIZETEXT * 2,
    0,         SIZETEXT * 2,
    NULL,      NULL
};


Ki_WorkSheetData WS_Segm6 =
{
    WS_SEGMENT,
    &WS_Segm7,
    BLOCK_OX,   SIZETEXT * 4,
    0,          SIZETEXT * 4,
    NULL,       NULL
};


Ki_WorkSheetData WS_Segm7 =
{
    WS_SEGMENT,
    NULL,
    BLOCK_OX - (SIZETEXT * 11),SIZETEXT * 4,
    BLOCK_OX - (SIZETEXT * 11),SIZETEXT * 2,
    NULL,                      NULL
};

void EDA_DRAW_FRAME::TraceWorkSheet( wxDC* aDC, wxSize& aPageSize,
                                     wxPoint& aLTmargin, wxPoint& aRBmargin,
                                     wxString& aPaperFormat, wxString& aFileName,
                                     TITLE_BLOCK& aTitleBlock,
                                     int aSheetCount, int aSheetNumber,
                                     int aPenWidth, double aScalar,
                                     EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor )
{
    wxPoint pos;
    wxPoint end;
    int refx, refy;
    wxString Line;
    Ki_WorkSheetData* WsItem;
    wxSize size( SIZETEXT * aScalar, SIZETEXT * aScalar );
    wxSize size_ref( SIZETEXT_REF * aScalar, SIZETEXT_REF * aScalar );
    wxString msg;

    GRSetDrawMode( aDC, GR_COPY );

    // Upper left corner
    refx = aLTmargin.x;
    refy = aLTmargin.y;

    // lower right corner
    int xg, yg;
    xg   = aPageSize.x - aRBmargin.x;
    yg   = aPageSize.y - aRBmargin.y;

    // Draw the border.
    int ii, jj, ipas, gxpas, gypas;
    for( ii = 0; ii < 2; ii++ )
    {
        GRRect( m_canvas->GetClipBox(), aDC, refx * aScalar, refy * aScalar,
                xg * aScalar, yg * aScalar, aPenWidth, aLineColor );

        refx += GRID_REF_W; refy += GRID_REF_W;
        xg   -= GRID_REF_W; yg -= GRID_REF_W;
    }

    // Upper left corner
    refx = aLTmargin.x;
    refy = aLTmargin.y;

    // lower right corner
    xg   = aPageSize.x - aRBmargin.x;
    yg   = aPageSize.y - aRBmargin.y;

    ipas  = ( xg - refx ) / PAS_REF;
    gxpas = ( xg - refx ) / ipas;
    for( ii = refx + gxpas, jj = 1; ipas > 0; ii += gxpas, jj++, ipas-- )
    {
        Line.Printf( wxT( "%d" ), jj );

        if( ii < xg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, ii * aScalar, refy * aScalar,
                    ii * aScalar, ( refy + GRID_REF_W ) * aScalar, aPenWidth, aLineColor );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( ii - gxpas / 2 ) * aScalar,
                                  ( refy + GRID_REF_W / 2 ) * aScalar ),
                         aLineColor, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aPenWidth, false, false );

        if( ii < xg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, ii * aScalar, yg * aScalar,
                    ii * aScalar, ( yg - GRID_REF_W ) * aScalar, aPenWidth, aLineColor );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( ii - gxpas / 2 ) * aScalar,
                                  ( yg - GRID_REF_W / 2) * aScalar ),
                         aLineColor, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aPenWidth, false, false );
    }

    ipas  = ( yg - refy ) / PAS_REF;
    gypas = ( yg - refy ) / ipas;

    for( ii = refy + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        if( jj < 26 )
            Line.Printf( wxT( "%c" ), jj + 'A' );
        else    // I hope 52 identifiers are enough...
            Line.Printf( wxT( "%c" ), 'a' + jj - 26 );

        if( ii < yg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, refx * aScalar, ii * aScalar,
                    ( refx + GRID_REF_W ) * aScalar, ii * aScalar, aPenWidth, aLineColor );
        }

        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( refx + GRID_REF_W / 2 ) * aScalar,
                                  ( ii - gypas / 2 ) * aScalar ),
                         aLineColor, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aPenWidth, false, false );

        if( ii < yg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, xg * aScalar, ii * aScalar,
                    ( xg - GRID_REF_W ) * aScalar, ii * aScalar, aPenWidth, aLineColor );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( xg - GRID_REF_W / 2 ) * aScalar,
                                  ( ii - gxpas / 2 ) * aScalar ),
                         aLineColor, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aPenWidth, false, false );
    }

    int UpperLimit = VARIABLE_BLOCK_START_POSITION;
    refx = aPageSize.x - aRBmargin.x  - GRID_REF_W;
    refy = aPageSize.y - aRBmargin.y - GRID_REF_W;

    for( WsItem = &WS_Date; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x = (refx - WsItem->m_Posx) * aScalar;
        pos.y = (refy - WsItem->m_Posy) * aScalar;
        msg.Empty();

        switch( WsItem->m_Type )
        {
        case WS_DATE:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTitleBlock.GetDate();
            DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, true );
            break;

        case WS_REV:
            if( WsItem->m_Legende )
            {
                msg = WsItem->m_Legende;
                DrawGraphicText( m_canvas, aDC, pos, aLineColor, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 GetPenSizeForBold( std::min( size.x, size.y ) ), false, true );
                pos.x += ReturnGraphicTextWidth( msg, size.x, false, false );
             }
            msg = aTitleBlock.GetRevision();
            DrawGraphicText( m_canvas, aDC, pos, aTextColor, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             GetPenSizeForBold( std::min( size.x, size.y ) ), false, true );
            break;

        case WS_KICAD_VERSION:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += g_ProductName + wxGetApp().GetAppName();
            msg += wxT( " " ) + GetBuildVersion();
            DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
            break;

        case WS_SIZESHEET:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aPaperFormat;
            DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
            break;


        case WS_IDENTSHEET:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg << aSheetNumber << wxT( "/" ) << aSheetCount;
            DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
            break;

        case WS_FILENAME:
            {
                wxFileName fn( aFileName );

                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;

                msg << fn.GetFullName();
                DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aPenWidth, false, false );
            }
            break;

        case WS_FULLSHEETNAME:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += GetScreenDesc();
            DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
            break;


        case WS_COMPANY_NAME:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTitleBlock.GetCompany();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 GetPenSizeForBold( std::min( size.x, size.y ) ),
                                 false, true );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_TITLE:
            if( WsItem->m_Legende )
            {
                msg = WsItem->m_Legende;
                DrawGraphicText( m_canvas, aDC, pos, aLineColor, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 GetPenSizeForBold( std::min( size.x, size.y ) ), false, true );
                pos.x += ReturnGraphicTextWidth( msg, size.x, false, false );
            }
            msg = aTitleBlock.GetTitle();
            DrawGraphicText( m_canvas, aDC, pos, aTextColor, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             GetPenSizeForBold( std::min( size.x, size.y ) ), false, true );
            break;

        case WS_COMMENT1:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTitleBlock.GetComment1();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aPenWidth, false, false );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT2:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTitleBlock.GetComment2();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aPenWidth, false, false );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT3:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTitleBlock.GetComment3();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aPenWidth, false, false );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT4:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTitleBlock.GetComment4();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aPenWidth, false, false );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_UPPER_SEGMENT:
            if( UpperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy =
                WS_MostUpperLine.m_Endy    =
                    WS_MostLeftLine.m_Posy = UpperLimit;
            pos.y = (refy - WsItem->m_Posy) * aScalar;

        case WS_SEGMENT:
            xg = aPageSize.x - GRID_REF_W - aRBmargin.x - WsItem->m_Endx;
            yg = aPageSize.y - GRID_REF_W - aRBmargin.y - WsItem->m_Endy;
            GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y,
                    xg * aScalar, yg * aScalar, aPenWidth, aLineColor );
            break;
        }
    }
}
