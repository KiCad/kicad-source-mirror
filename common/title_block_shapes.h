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


#define GRID_REF_W                      70      // height of the band reference grid
#define SIZETEXT                        60      // worksheet text size
#define SIZETEXT_REF                    50      // worksheet frame reference text size
#define PAS_REF                         2000    // reference markings on worksheet frame
#define VARIABLE_BLOCK_START_POSITION   (SIZETEXT * 10)

// The coordinates below are relative to the bottom right corner of page and
// will be subtracted from this origin.
#define BLOCK_OX                4200
#define BLOCK_KICAD_VERSION_X   BLOCK_OX - SIZETEXT
#define BLOCK_KICAD_VERSION_Y   SIZETEXT
#define BLOCK_REV_X             820
#define BLOCK_REV_Y             (SIZETEXT * 3)
#define BLOCK_DATE_X            BLOCK_OX - (SIZETEXT * 15)
#define BLOCK_DATE_Y            (SIZETEXT * 3)
#define BLOCK_ID_SHEET_X        820
#define BLOCK_ID_SHEET_Y        SIZETEXT
#define BLOCK_SIZE_SHEET_X      BLOCK_OX - SIZETEXT
#define BLOCK_SIZE_SHEET_Y      (SIZETEXT * 3)
#define BLOCK_TITLE_X           BLOCK_OX - SIZETEXT
#define BLOCK_TITLE_Y           (SIZETEXT * 5)
#define BLOCK_FULLSHEETNAME_X   BLOCK_OX - SIZETEXT
#define BLOCK_FULLSHEETNAME_Y   (SIZETEXT * 7)
#define BLOCK_FILENAME_X        BLOCK_OX - SIZETEXT
#define BLOCK_FILENAME_Y        (SIZETEXT * 9)
#define BLOCK_COMMENT_X         BLOCK_OX - SIZETEXT
#define BLOCK_COMPANY_Y         (SIZETEXT * 11)
#define BLOCK_COMMENT1_Y        (SIZETEXT * 13)
#define BLOCK_COMMENT2_Y        (SIZETEXT * 15)
#define BLOCK_COMMENT3_Y        (SIZETEXT * 17)
#define BLOCK_COMMENT4_Y        (SIZETEXT * 19)

// Work sheet structure type definitions.
enum TypeKi_WorkSheetData {
    WS_DATE,
    WS_REV,
    WS_KICAD_VERSION,
    WS_SIZESHEET,
    WS_IDENTSHEET,
    WS_TITLE,
    WS_FILENAME,
    WS_FULLSHEETNAME,
    WS_COMPANY_NAME,
    WS_COMMENT1,
    WS_COMMENT2,
    WS_COMMENT3,
    WS_COMMENT4,
    WS_SEGMENT,
    WS_UPPER_SEGMENT,
    WS_LEFT_SEGMENT,
    WS_CADRE
};

extern Ki_WorkSheetData WS_Date;
extern Ki_WorkSheetData WS_Revision;
extern Ki_WorkSheetData WS_Licence;
extern Ki_WorkSheetData WS_SizeSheet;
extern Ki_WorkSheetData WS_IdentSheet;
extern Ki_WorkSheetData WS_FullSheetName;
extern Ki_WorkSheetData WS_SheetFilename;
extern Ki_WorkSheetData WS_Title;
extern Ki_WorkSheetData WS_Company;
extern Ki_WorkSheetData WS_Comment1;
extern Ki_WorkSheetData WS_Comment2;
extern Ki_WorkSheetData WS_Comment3;
extern Ki_WorkSheetData WS_Comment4;
extern Ki_WorkSheetData WS_SeparatorLine;
extern Ki_WorkSheetData WS_MostLeftLine;
extern Ki_WorkSheetData WS_MostUpperLine;
extern Ki_WorkSheetData WS_Segm3;
extern Ki_WorkSheetData WS_Segm4;
extern Ki_WorkSheetData WS_Segm5;
extern Ki_WorkSheetData WS_Segm6;
extern Ki_WorkSheetData WS_Segm7;

Ki_WorkSheetData        WS_Date =
{
    WS_DATE,
    &WS_Licence,
    BLOCK_DATE_X,   BLOCK_DATE_Y,
    0,                          0,
    wxT( "Date: " ),NULL
};

Ki_WorkSheetData        WS_Licence =
{
    WS_KICAD_VERSION,
    &WS_Revision,
    BLOCK_KICAD_VERSION_X,BLOCK_KICAD_VERSION_Y,
    0,
    0,
    NULL,                 NULL
};

Ki_WorkSheetData        WS_Revision =
{
    WS_REV,
    &WS_SizeSheet,
    BLOCK_REV_X,   BLOCK_REV_Y,
    0,                      0,
    wxT( "Rev: " ),NULL
};

Ki_WorkSheetData        WS_SizeSheet =
{
    WS_SIZESHEET,
    &WS_IdentSheet,
    BLOCK_SIZE_SHEET_X,BLOCK_SIZE_SHEET_Y,
    0,                                   0,
    wxT( "Size: " ),   NULL
};

Ki_WorkSheetData        WS_IdentSheet =
{
    WS_IDENTSHEET,
    &WS_Title,
    BLOCK_ID_SHEET_X,BLOCK_ID_SHEET_Y,
    0,                               0,
    wxT( "Id: " ),   NULL
};

Ki_WorkSheetData        WS_Title =
{
    WS_TITLE,
    &WS_SheetFilename,
    BLOCK_TITLE_X,    BLOCK_TITLE_Y,
    0,                         0,
    wxT( "Title: " ), NULL
};

Ki_WorkSheetData        WS_SheetFilename =
{
    WS_FILENAME,
    &WS_FullSheetName,
    BLOCK_FILENAME_X, BLOCK_FILENAME_Y,
    0,                               0,
    wxT( "File: " ),  NULL
};

Ki_WorkSheetData        WS_FullSheetName =
{
    WS_FULLSHEETNAME,
    &WS_Company,
    BLOCK_FULLSHEETNAME_X,BLOCK_FULLSHEETNAME_Y,
    0,
    0,
    wxT( "Sheet: " ),     NULL
};

Ki_WorkSheetData        WS_Company =
{
    WS_COMPANY_NAME,
    &WS_Comment1,
    BLOCK_COMMENT_X,BLOCK_COMPANY_Y,
    0,                             0,
    NULL,           NULL
};

Ki_WorkSheetData        WS_Comment1 =
{
    WS_COMMENT1,
    &WS_Comment2,
    BLOCK_COMMENT_X,BLOCK_COMMENT1_Y,
    0,                              0,
    NULL,           NULL
};

Ki_WorkSheetData        WS_Comment2 =
{
    WS_COMMENT2,
    &WS_Comment3,
    BLOCK_COMMENT_X,BLOCK_COMMENT2_Y,
    0,                              0,
    NULL,           NULL
};

Ki_WorkSheetData        WS_Comment3 =
{
    WS_COMMENT3,
    &WS_Comment4,
    BLOCK_COMMENT_X,BLOCK_COMMENT3_Y,
    0,                              0,
    NULL,           NULL
};

Ki_WorkSheetData        WS_Comment4 =
{
    WS_COMMENT4,
    &WS_MostLeftLine,
    BLOCK_COMMENT_X, BLOCK_COMMENT4_Y,
    0,                              0,
    NULL,            NULL
};


// Left vertical segment
Ki_WorkSheetData WS_MostLeftLine =
{
    WS_LEFT_SEGMENT,
    &WS_SeparatorLine,
    BLOCK_OX,         SIZETEXT * 16,
    BLOCK_OX,             0,
    NULL,             NULL
};


// horizontal segment between filename and comments
Ki_WorkSheetData WS_SeparatorLine =
{
    WS_SEGMENT,
    &WS_MostUpperLine,
    BLOCK_OX,         VARIABLE_BLOCK_START_POSITION,
    0,                VARIABLE_BLOCK_START_POSITION,
    NULL,             NULL
};


// superior horizontal segment
Ki_WorkSheetData WS_MostUpperLine =
{
    WS_UPPER_SEGMENT,
    &WS_Segm3,
    BLOCK_OX,        SIZETEXT * 16,
    0,               SIZETEXT * 16,
    NULL,            NULL
};


// horizontal segment above COMPANY NAME
Ki_WorkSheetData WS_Segm3 =
{
    WS_SEGMENT,
    &WS_Segm4,
    BLOCK_OX,  SIZETEXT * 6,
    0,         SIZETEXT * 6,
    NULL,      NULL
};


// vertical segment of the left REV and SHEET
Ki_WorkSheetData WS_Segm4 =
{
    WS_SEGMENT,
    &WS_Segm5,
    BLOCK_REV_X + SIZETEXT,SIZETEXT * 4,
    BLOCK_REV_X + SIZETEXT,            0,
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
    BLOCK_OX,  SIZETEXT * 4,
    0,         SIZETEXT * 4,
    NULL,      NULL
};


Ki_WorkSheetData WS_Segm7 =
{
    WS_SEGMENT,
    NULL,
    BLOCK_OX - (SIZETEXT * 11),SIZETEXT * 4,
    BLOCK_OX - (SIZETEXT * 11),SIZETEXT * 2,
    NULL,                      NULL
};

#include <worksheet_shape_builder.h>

void WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList(
                       const wxString& aPaperFormat,
                       const wxString& aFileName,
                       const wxString& aSheetPathHumanReadable,
                       const TITLE_BLOCK& aTitleBlock,
                       int aSheetCount, int aSheetNumber,
                       EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor )
{
    wxPoint             pos;
    wxPoint             end;
    int                 refx, refy;
    wxString            Line;
    Ki_WorkSheetData*   WsItem;
    wxSize              size( SIZETEXT * m_milsToIu, SIZETEXT * m_milsToIu );
    wxSize              size_ref( SIZETEXT_REF * m_milsToIu, SIZETEXT_REF * m_milsToIu );
    wxString            msg;

    // Upper left corner
    refx    = m_LTmargin.x;
    refy    = m_LTmargin.y;

    // lower right corner
    wxPoint currpos;
    currpos.x  = m_pageSize.x - m_RBmargin.x;
    currpos.y  = m_pageSize.y - m_RBmargin.y;

    // Draw the border.
    int ii, jj, ipas, gxpas, gypas;

    for( ii = 0; ii < 2; ii++ )
    {
        Append( new WS_DRAW_ITEM_RECT(
                    wxPoint( refx * m_milsToIu, refy * m_milsToIu ),
                    wxPoint( currpos.x * m_milsToIu, currpos.y * m_milsToIu ),
                    m_penSize, aLineColor ) );

        refx += GRID_REF_W;
        refy += GRID_REF_W;
        currpos.x -= GRID_REF_W;
        currpos.y -= GRID_REF_W;
    }

    // Upper left corner
    refx    = m_LTmargin.x;
    refy    = m_LTmargin.y;

    // lower right corner
    currpos.x  = m_pageSize.x - m_RBmargin.x;
    currpos.y  = m_pageSize.y - m_RBmargin.y;

    ipas    = ( currpos.x - refx ) / PAS_REF;
    gxpas   = ( currpos.x - refx ) / ipas;

    for( ii = refx + gxpas, jj = 1; ipas > 0; ii += gxpas, jj++, ipas-- )
    {
        Line.Printf( wxT( "%d" ), jj );

        if( ii < currpos.x - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( ii * m_milsToIu, refy * m_milsToIu ),
                        wxPoint( ii * m_milsToIu, ( refy + GRID_REF_W ) * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( Line,
                                       wxPoint( ( ii - gxpas / 2 ) * m_milsToIu,
                                                ( refy + GRID_REF_W / 2 ) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );

        if( ii < currpos.x - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( ii * m_milsToIu, currpos.y * m_milsToIu ),
                        wxPoint( ii * m_milsToIu, (currpos.y - GRID_REF_W ) * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( Line,
                                       wxPoint( ( ii - gxpas / 2 ) * m_milsToIu,
                                                ( currpos.y - GRID_REF_W / 2) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );
    }

    ipas    = ( currpos.y - refy ) / PAS_REF;
    gypas   = ( currpos.y - refy ) / ipas;

    for( ii = refy + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        if( jj < 26 )
            Line.Printf( wxT( "%c" ), jj + 'A' );
        else // I hope 52 identifiers are enough...
            Line.Printf( wxT( "%c" ), 'a' + jj - 26 );

        if( ii < currpos.y - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( refx * m_milsToIu, ii * m_milsToIu ),
                        wxPoint( ( refx + GRID_REF_W ) * m_milsToIu, ii * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( Line,
                                       wxPoint( ( refx + GRID_REF_W / 2 ) * m_milsToIu,
                                                ( ii - gypas / 2 ) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );

        if( ii < currpos.y - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( currpos.x * m_milsToIu, ii * m_milsToIu ),
                        wxPoint( ( currpos.x - GRID_REF_W ) * m_milsToIu, ii * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( Line,
                                       wxPoint( ( currpos.x - GRID_REF_W / 2 ) * m_milsToIu,
                                                ( ii - gxpas / 2 ) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );
    }

    int UpperLimit = VARIABLE_BLOCK_START_POSITION;
    refx    = m_pageSize.x - m_RBmargin.x - GRID_REF_W;
    refy    = m_pageSize.y - m_RBmargin.y - GRID_REF_W;

    WS_DRAW_ITEM_TEXT* gtext;

    for( WsItem = &WS_Date; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x   = (refx - WsItem->m_Posx) * m_milsToIu;
        pos.y   = (refy - WsItem->m_Posy) * m_milsToIu;
        msg.Empty();

        switch( WsItem->m_Type )
        {
        case WS_DATE:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += aTitleBlock.GetDate();
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                   size, m_penSize, aLineColor, false,
                                                   true ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case WS_REV:

            if( WsItem->m_Legende )
            {
                msg = WsItem->m_Legende;
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                       GetPenSizeForBold( std::min( size.
                                                                                    x,
                                                                                    size.
                                                                                    y ) ),
                                                       aLineColor, false, true ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                pos.x += ReturnGraphicTextWidth( msg, size.x, false, false );
            }

            msg = aTitleBlock.GetRevision();
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                   GetPenSizeForBold( std::min( size.x,
                                                                                size.y ) ),
                                                   aTextColor, false, true ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case WS_KICAD_VERSION:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += g_ProductName + wxGetApp().GetAppName();
            msg += wxT( " " ) + GetBuildVersion();
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                   m_penSize, aLineColor ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case WS_SIZESHEET:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += aPaperFormat;
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                   m_penSize, aLineColor ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;


        case WS_IDENTSHEET:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg << aSheetNumber << wxT( "/" ) << aSheetCount;
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                   m_penSize, aLineColor ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case WS_FILENAME:
            {
                wxFileName fn( aFileName );

                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;

                msg << fn.GetFullName();
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                       m_penSize, aLineColor ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            }
            break;

        case WS_FULLSHEETNAME:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += aSheetPathHumanReadable;
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                   m_penSize, aLineColor ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;


        case WS_COMPANY_NAME:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += aTitleBlock.GetCompany();

            if( !msg.IsEmpty() )
            {
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                       GetPenSizeForBold( std::min( size.
                                                                                    x,
                                                                                    size.
                                                                                    y ) ),
                                                       aTextColor, false, true ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }

            break;

        case WS_TITLE:

            if( WsItem->m_Legende )
            {
                msg = WsItem->m_Legende;
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                       GetPenSizeForBold( std::min( size.
                                                                                    x,
                                                                                    size.
                                                                                    y ) ),
                                                       aLineColor, false, true ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                pos.x += ReturnGraphicTextWidth( msg, size.x, false, false );
            }

            msg = aTitleBlock.GetTitle();
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                   GetPenSizeForBold( std::min( size.x,
                                                                                size.y ) ),
                                                   aTextColor, false, true ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case WS_COMMENT1:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += aTitleBlock.GetComment1();

            if( !msg.IsEmpty() )
            {
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                       m_penSize, aTextColor ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }

            break;

        case WS_COMMENT2:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += aTitleBlock.GetComment2();

            if( !msg.IsEmpty() )
            {
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                       m_penSize, aTextColor ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }

            break;

        case WS_COMMENT3:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += aTitleBlock.GetComment3();

            if( !msg.IsEmpty() )
            {
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                       m_penSize, aTextColor ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }

            break;

        case WS_COMMENT4:

            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;

            msg += aTitleBlock.GetComment4();

            if( !msg.IsEmpty() )
            {
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, size,
                                                       m_penSize, aTextColor ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }

            break;

        case WS_UPPER_SEGMENT:

            if( UpperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy         =
                WS_MostUpperLine.m_Endy     =
                    WS_MostLeftLine.m_Posy  = UpperLimit;
            pos.y = (refy - WsItem->m_Posy) * m_milsToIu;

        case WS_SEGMENT:
            currpos.x  = m_pageSize.x - GRID_REF_W - m_RBmargin.x - WsItem->m_Endx;
            currpos.y  = m_pageSize.y - GRID_REF_W - m_RBmargin.y - WsItem->m_Endy;
            Append( new WS_DRAW_ITEM_LINE( pos,
                                           wxPoint( currpos.x * m_milsToIu, currpos.y * m_milsToIu ),
                                           m_penSize, aLineColor ) );
            break;
        }
    }
}
