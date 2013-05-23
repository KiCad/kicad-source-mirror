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
#define TEXTSIZE                        60      // worksheet text size
#define FRMREF_TXTSIZE                  50      // worksheet frame reference text size
#define VARIABLE_BLOCK_START_POSITION   (TEXTSIZE * 10)

// The coordinates below are relative to the bottom right corner of page and
// will be subtracted from this origin.
#define BLOCK_OX                4200
#define BLOCK_KICAD_VERSION_X   BLOCK_OX - TEXTSIZE
#define BLOCK_KICAD_VERSION_Y   TEXTSIZE
#define BLOCK_REV_X             820
#define BLOCK_REV_Y             (TEXTSIZE * 3)
#define BLOCK_DATE_X            BLOCK_OX - (TEXTSIZE * 15)
#define BLOCK_DATE_Y            (TEXTSIZE * 3)
#define BLOCK_ID_SHEET_X        820
#define BLOCK_ID_SHEET_Y        TEXTSIZE
#define BLOCK_SIZE_SHEET_X      BLOCK_OX - TEXTSIZE
#define BLOCK_SIZE_SHEET_Y      (TEXTSIZE * 3)
#define BLOCK_TITLE_X           BLOCK_OX - TEXTSIZE
#define BLOCK_TITLE_Y           (TEXTSIZE * 5)
#define BLOCK_FULLSHEETNAME_X   BLOCK_OX - TEXTSIZE
#define BLOCK_FULLSHEETNAME_Y   (TEXTSIZE * 7)
#define BLOCK_FILENAME_X        BLOCK_OX - TEXTSIZE
#define BLOCK_FILENAME_Y        (TEXTSIZE * 9)
#define BLOCK_COMMENT_X         BLOCK_OX - TEXTSIZE
#define BLOCK_COMPANY_Y         (TEXTSIZE * 11)
#define BLOCK_COMMENT1_Y        (TEXTSIZE * 13)
#define BLOCK_COMMENT2_Y        (TEXTSIZE * 15)
#define BLOCK_COMMENT3_Y        (TEXTSIZE * 17)
#define BLOCK_COMMENT4_Y        (TEXTSIZE * 19)

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


// superior horizontal segment: should be after comments
// to know the exact position
Ki_WorkSheetData WS_MostUpperLine =
{
    WS_UPPER_SEGMENT,
    NULL,
    BLOCK_OX,        TEXTSIZE * 16,
    0,               TEXTSIZE * 16,
    NULL,            NULL
};

// Left vertical segment: should be after comments
// to know the exact position
Ki_WorkSheetData WS_MostLeftLine =
{
    WS_LEFT_SEGMENT,
    &WS_MostUpperLine,
    BLOCK_OX,         TEXTSIZE * 16,
    BLOCK_OX,             0,
    NULL,             NULL
};

// horizontal segment between filename and comments
Ki_WorkSheetData WS_SeparatorLine =
{
    WS_SEGMENT,
    &WS_MostLeftLine,
    BLOCK_OX,         VARIABLE_BLOCK_START_POSITION,
    0,                VARIABLE_BLOCK_START_POSITION,
    NULL,             NULL
};

Ki_WorkSheetData        WS_Date =
{
    WS_DATE,
    &WS_SeparatorLine,
    BLOCK_DATE_X,   BLOCK_DATE_Y,
    0,                          0,
    wxT( "Date: " ),NULL
};

Ki_WorkSheetData        WS_Licence =
{
    WS_KICAD_VERSION,
    &WS_Date,
    BLOCK_KICAD_VERSION_X,BLOCK_KICAD_VERSION_Y,
    0,
    0,
    NULL,                 NULL
};

Ki_WorkSheetData        WS_Revision =
{
    WS_REV,
    &WS_Licence,
    BLOCK_REV_X,   BLOCK_REV_Y,
    0,                      0,
    wxT( "Rev: " ),NULL
};

Ki_WorkSheetData        WS_SizeSheet =
{
    WS_SIZESHEET,
    &WS_Revision,
    BLOCK_SIZE_SHEET_X,BLOCK_SIZE_SHEET_Y,
    0,                                   0,
    wxT( "Size: " ),   NULL
};

Ki_WorkSheetData        WS_IdentSheet =
{
    WS_IDENTSHEET,
    &WS_SizeSheet,
    BLOCK_ID_SHEET_X,BLOCK_ID_SHEET_Y,
    0,                               0,
    wxT( "Id: " ),   NULL
};

Ki_WorkSheetData        WS_Title =
{
    WS_TITLE,
    &WS_IdentSheet,
    BLOCK_TITLE_X,    BLOCK_TITLE_Y,
    0,                         0,
    wxT( "Title: " ), NULL
};

Ki_WorkSheetData        WS_SheetFilename =
{
    WS_FILENAME,
    &WS_Title,
    BLOCK_FILENAME_X, BLOCK_FILENAME_Y,
    0,                               0,
    wxT( "File: " ),  NULL
};

Ki_WorkSheetData        WS_FullSheetName =
{
    WS_FULLSHEETNAME,
    &WS_SheetFilename,
    BLOCK_FULLSHEETNAME_X,BLOCK_FULLSHEETNAME_Y,
    0,
    0,
    wxT( "Sheet: " ),     NULL
};

Ki_WorkSheetData        WS_Company =
{
    WS_COMPANY_NAME,
    &WS_FullSheetName,
    BLOCK_COMMENT_X,BLOCK_COMPANY_Y,
    0,                             0,
    NULL,           NULL
};

Ki_WorkSheetData        WS_Comment1 =
{
    WS_COMMENT1,
    &WS_Company,
    BLOCK_COMMENT_X,BLOCK_COMMENT1_Y,
    0,                              0,
    NULL,           NULL
};

Ki_WorkSheetData        WS_Comment2 =
{
    WS_COMMENT2,
    &WS_Comment1,
    BLOCK_COMMENT_X,BLOCK_COMMENT2_Y,
    0,                              0,
    NULL,           NULL
};

Ki_WorkSheetData        WS_Comment3 =
{
    WS_COMMENT3,
    &WS_Comment2,
    BLOCK_COMMENT_X,BLOCK_COMMENT3_Y,
    0,                              0,
    NULL,           NULL
};

Ki_WorkSheetData        WS_Comment4 =
{
    WS_COMMENT4,
    &WS_Comment3,
    BLOCK_COMMENT_X, BLOCK_COMMENT4_Y,
    0,                              0,
    NULL,            NULL
};


// horizontal segment above COMPANY NAME
Ki_WorkSheetData WS_Segm3 =
{
    WS_SEGMENT,
    &WS_Comment4,
    BLOCK_OX,  TEXTSIZE * 6,
    0,         TEXTSIZE * 6,
    NULL,      NULL
};


// vertical segment of the left REV and SHEET
Ki_WorkSheetData WS_Segm4 =
{
    WS_SEGMENT,
    &WS_Segm3,
    BLOCK_REV_X + TEXTSIZE,TEXTSIZE * 4,
    BLOCK_REV_X + TEXTSIZE,            0,
    NULL,                  NULL
};


Ki_WorkSheetData WS_Segm5 =
{
    WS_SEGMENT,
    &WS_Segm4,
    BLOCK_OX,  TEXTSIZE * 2,
    0,         TEXTSIZE * 2,
    NULL,      NULL
};


Ki_WorkSheetData WS_Segm6 =
{
    WS_SEGMENT,
    &WS_Segm5,
    BLOCK_OX,  TEXTSIZE * 4,
    0,         TEXTSIZE * 4,
    NULL,      NULL
};


Ki_WorkSheetData WS_Segm7 =
{
    WS_SEGMENT,
    &WS_Segm6,
    BLOCK_OX - (TEXTSIZE * 11),TEXTSIZE * 4,
    BLOCK_OX - (TEXTSIZE * 11),TEXTSIZE * 2,
    NULL,                      NULL
};

#include <worksheet_shape_builder.h>

// Helper function which returns the text corresponding to the aIdent identifier
static wxString FindUserText( int aIdent, const TITLE_BLOCK& aTitleBlock,
                          const wxString& aPaperFormat,
                          const wxString& aFileName,
                          const wxString& aSheetPathHumanReadable,
                          int aSheetCount, int aSheetNumber );


void WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList(
                       const wxString& aPaperFormat,
                       const wxString& aFileName,
                       const wxString& aSheetPathHumanReadable,
                       const TITLE_BLOCK& aTitleBlock,
                       int aSheetCount, int aSheetNumber,
                       EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor )
{
    wxPoint             pos;
    wxSize              textsize( TEXTSIZE * m_milsToIu, TEXTSIZE * m_milsToIu );
    wxSize              size_ref( FRMREF_TXTSIZE * m_milsToIu,
                                  FRMREF_TXTSIZE * m_milsToIu );
    wxString            msg;

    // Upper left corner
    int refx    = m_LTmargin.x;
    int refy    = m_LTmargin.y;

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
        msg.Printf( wxT( "%d" ), jj );

        if( ii < currpos.x - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( ii * m_milsToIu, refy * m_milsToIu ),
                        wxPoint( ii * m_milsToIu, ( refy + GRID_REF_W ) * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( msg,
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

        Append( new WS_DRAW_ITEM_TEXT( msg,
                                       wxPoint( ( ii - gxpas / 2 ) * m_milsToIu,
                                                ( currpos.y - GRID_REF_W / 2) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );
    }

    ipas    = ( currpos.y - refy ) / PAS_REF;
    gypas   = ( currpos.y - refy ) / ipas;

    for( ii = refy + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        if( jj < 26 )
            msg.Printf( wxT( "%c" ), jj + 'A' );
        else // I hope 52 identifiers are enough...
            msg.Printf( wxT( "%c" ), 'a' + jj - 26 );

        if( ii < currpos.y - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( refx * m_milsToIu, ii * m_milsToIu ),
                        wxPoint( ( refx + GRID_REF_W ) * m_milsToIu, ii * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( msg,
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

        Append( new WS_DRAW_ITEM_TEXT( msg,
                                       wxPoint( ( currpos.x - GRID_REF_W / 2 ) * m_milsToIu,
                                                ( ii - gxpas / 2 ) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );
    }

    int UpperLimit = VARIABLE_BLOCK_START_POSITION;
    refx    = m_pageSize.x - m_RBmargin.x - GRID_REF_W;
    refy    = m_pageSize.y - m_RBmargin.y - GRID_REF_W;

    WS_DRAW_ITEM_TEXT* gtext;
    Ki_WorkSheetData*  WsItem;
    int boldPenSize;

    for( WsItem = &WS_Segm7; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x   = (refx - WsItem->m_Posx) * m_milsToIu;
        pos.y   = (refy - WsItem->m_Posy) * m_milsToIu;

        msg.Empty();

        if( WsItem->m_Legende )
            msg = WsItem->m_Legende;
        msg += FindUserText( WsItem->m_Type, aTitleBlock, aPaperFormat, aFileName,
                             aSheetPathHumanReadable, aSheetCount, aSheetNumber );

        switch( WsItem->m_Type )
        {
        case WS_REV:
        case WS_TITLE:
            boldPenSize = GetPenSizeForBold( std::min( textsize.x, textsize.y ) );
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, textsize, boldPenSize,
                                                   aTextColor, false, true ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case WS_KICAD_VERSION:
        case WS_SIZESHEET:
        case WS_IDENTSHEET:
        case WS_FILENAME:
        case WS_FULLSHEETNAME:
        case WS_DATE:
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, textsize,
                                                   m_penSize, aLineColor ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case WS_COMPANY_NAME:
            if( !msg.IsEmpty() )
            {
                boldPenSize = GetPenSizeForBold( std::min( textsize.x, textsize.y ) );
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, textsize ,boldPenSize,
                                                       aTextColor, false, true ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + TEXTSIZE );
            }

            break;

        case WS_COMMENT1:
        case WS_COMMENT2:
        case WS_COMMENT3:
        case WS_COMMENT4:
            if( !msg.IsEmpty() )
            {
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, textsize,
                                                       m_penSize, aTextColor ) );
                gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + TEXTSIZE );
            }

            break;

        case WS_UPPER_SEGMENT:

            if( UpperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy = WS_MostUpperLine.m_Endy =
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

// returns the text corresponding to the aIdent identifier
wxString FindUserText( int aIdent, const TITLE_BLOCK& aTitleBlock,
                   const wxString& aPaperFormat,
                   const wxString& aFileName,
                   const wxString& aSheetPathHumanReadable,
                   int aSheetCount, int aSheetNumber )
{
    wxString msg;

    switch( aIdent )
    {
        case WS_DATE:
            msg = aTitleBlock.GetDate();
            break;

        case WS_REV:
            msg = aTitleBlock.GetRevision();
            break;

        case WS_KICAD_VERSION:
            msg = g_ProductName + wxGetApp().GetAppName();
            msg += wxT( " " ) + GetBuildVersion();
            break;

        case WS_SIZESHEET:
            msg = aPaperFormat;
            break;


        case WS_IDENTSHEET:
            msg << aSheetNumber << wxT( "/" ) << aSheetCount;
            break;

        case WS_FILENAME:
            {
                wxFileName fn( aFileName );
                msg = fn.GetFullName();
            }
            break;

        case WS_FULLSHEETNAME:
            msg = aSheetPathHumanReadable;
            break;

        case WS_COMPANY_NAME:
            msg = aTitleBlock.GetCompany();
            break;

        case WS_TITLE:
            msg = aTitleBlock.GetTitle();
            break;

        case WS_COMMENT1:
            msg = aTitleBlock.GetComment1();
            break;

        case WS_COMMENT2:
            msg = aTitleBlock.GetComment2();
            break;

        case WS_COMMENT3:
            msg = aTitleBlock.GetComment3();
            break;

        case WS_COMMENT4:
            msg = aTitleBlock.GetComment4();
            break;

        default:
            break;
    }

    return msg;
}
