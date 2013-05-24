/**
 * @file title_block_shape.cpp
 * @brief description of graphic items and texts to build a title block
 */

/*
 * This file creates a lot of structures which define the shape of a title block
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

#include <fctsys.h>
#include <drawtxt.h>
#include <appl_wxstruct.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <build_version.h>
#include <worksheet_shape_builder.h>

#define GRID_REF_W                      Mm2mils( 1.8 )  // height of the band reference grid
#define TEXTSIZE                        Mm2mils( 1.5 )  // worksheet text size
#define FRMREF_TXTSIZE                  Mm2mils( 1.3 )  // worksheet frame reference text size
#define VARIABLE_BLOCK_START_POSITION   (TEXTSIZE * 10)

// The coordinates below are relative to the bottom right corner of page and
// will be subtracted from this origin.
#define BLOCK_OX                Mm2mils( 106 )
#define BLOCK_KICAD_VERSION_X   BLOCK_OX - TEXTSIZE
#define BLOCK_KICAD_VERSION_Y   TEXTSIZE
#define BLOCK_REV_X             Mm2mils( 22 )
#define BLOCK_REV_Y             (TEXTSIZE * 3)
#define BLOCK_DATE_X            BLOCK_OX - (TEXTSIZE * 15)
#define BLOCK_DATE_Y            (TEXTSIZE * 3)
#define BLOCK_ID_SHEET_X        Mm2mils( 22 )
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

/*
 * Basic texts in Ki_WorkSheetData struct use format "C" type to
 * identify the user text which should be shown, at runtime.
 * Currently formats are % and a letter , or 2 letters
 *
 * %% = replaced by %
 * %K = Kicad version
 * %Z = paper format name (A4, USLetter)
 * %Y = company name
 * %D = date
 * %R = revision
 * %S = sheet number
 * %N = number of sheets
 * %Cx = comment (x = 0 to 9 to identify the comment)
 * %F = filename
 * %P = sheet path or sheet full name
 * %T = title
 * Other fields like Developer, Verifier, Approver could use %Cx
 * and are seen as comments for format
 */

// Text attributes set in m_Flags (ORed bits)
 #define USE_BOLD 1             // has meaning for texts
 #define USE_THICK_LINE 1       // equivalent to bold for lines
 #define USE_ITALIC 2           // has meaning for texts
 #define USE_TEXT_COLOR 4
 #define SET_UPPER_LIMIT 8      // Flag used to calculate variable position items

// Work sheet structure type definitions.
enum TypeKi_WorkSheetData {
    WS_TEXT,
    WS_SEGMENT,
    WS_UPPER_SEGMENT,
    WS_LEFT_SEGMENT
};


// superior horizontal segment: should be after comments
// to know the exact position
Ki_WorkSheetData WS_MostUpperLine =
{
    WS_UPPER_SEGMENT,
    NULL,
    BLOCK_OX,        TEXTSIZE * 16,
    0,               TEXTSIZE * 16,
    NULL
};

// Left vertical segment: should be after comments
// to know the exact position
Ki_WorkSheetData WS_MostLeftLine =
{
    WS_LEFT_SEGMENT,
    &WS_MostUpperLine,
    BLOCK_OX,         TEXTSIZE * 16,
    BLOCK_OX,             0,
    NULL
};

// horizontal segment between filename and comments
Ki_WorkSheetData WS_SeparatorLine =
{
    WS_SEGMENT,
    &WS_MostLeftLine,
    BLOCK_OX,         VARIABLE_BLOCK_START_POSITION,
    0,                VARIABLE_BLOCK_START_POSITION,
    NULL
};

Ki_WorkSheetData        WS_Date =
{
    WS_TEXT,
    &WS_SeparatorLine,
    BLOCK_DATE_X,   BLOCK_DATE_Y,
    0,                          0,
    wxT( "Date: %D" )
};

Ki_WorkSheetData        WS_Licence =
{
    WS_TEXT,
    &WS_Date,
    BLOCK_KICAD_VERSION_X,BLOCK_KICAD_VERSION_Y,
    0,
    0,
    wxT("%K")       // Kicad version
};

Ki_WorkSheetData        WS_Revision =
{
    WS_TEXT,
    &WS_Licence,
    BLOCK_REV_X,   BLOCK_REV_Y,
    0,                      0,
    wxT( "Rev: %R" ),
    USE_BOLD
};

Ki_WorkSheetData        WS_SizeSheet =
{
    WS_TEXT,
    &WS_Revision,
    BLOCK_SIZE_SHEET_X,BLOCK_SIZE_SHEET_Y,
    0,                                   0,
    wxT( "Size: %Z" )   // Paper format name
};

Ki_WorkSheetData        WS_IdentSheet =
{
    WS_TEXT,
    &WS_SizeSheet,
    BLOCK_ID_SHEET_X,BLOCK_ID_SHEET_Y,
    0,                               0,
    wxT( "Id: %S/%N" )
};

Ki_WorkSheetData        WS_Title =
{
    WS_TEXT,
    &WS_IdentSheet,
    BLOCK_TITLE_X,    BLOCK_TITLE_Y,
    0,                         0,
    wxT( "Title: %T" ),
    USE_BOLD
};

Ki_WorkSheetData        WS_SheetFilename =
{
    WS_TEXT,
    &WS_Title,
    BLOCK_FILENAME_X, BLOCK_FILENAME_Y,
    0,                               0,
    wxT( "File: %F" )
};

Ki_WorkSheetData        WS_FullSheetName =
{
    WS_TEXT,
    &WS_SheetFilename,
    BLOCK_FULLSHEETNAME_X,BLOCK_FULLSHEETNAME_Y,
    0,
    0,
    wxT( "Sheet: %P" )  // Full sheet name (sheet path)
};

Ki_WorkSheetData        WS_Company =
{
    WS_TEXT,
    &WS_FullSheetName,
    BLOCK_COMMENT_X,BLOCK_COMPANY_Y,
    0,                             0,
    wxT("%Y"),          // Company name
    USE_BOLD | SET_UPPER_LIMIT | USE_TEXT_COLOR
};

Ki_WorkSheetData        WS_Comment1 =
{
    WS_TEXT,
    &WS_Company,
    BLOCK_COMMENT_X,BLOCK_COMMENT1_Y,
    0,                              0,
    wxT("%C1"),           // Comment 1
    SET_UPPER_LIMIT | USE_TEXT_COLOR
};

Ki_WorkSheetData        WS_Comment2 =
{
    WS_TEXT,
    &WS_Comment1,
    BLOCK_COMMENT_X,BLOCK_COMMENT2_Y,
    0,                              0,
    wxT("%C2"),           // Comment 2
    SET_UPPER_LIMIT | USE_TEXT_COLOR
};

Ki_WorkSheetData        WS_Comment3 =
{
    WS_TEXT,
    &WS_Comment2,
    BLOCK_COMMENT_X,BLOCK_COMMENT3_Y,
    0,                              0,
    wxT("%C3"),           // Comment 3
    SET_UPPER_LIMIT | USE_TEXT_COLOR
};

Ki_WorkSheetData        WS_Comment4 =
{
    WS_TEXT,
    &WS_Comment3,
    BLOCK_COMMENT_X, BLOCK_COMMENT4_Y,
    0,                              0,
    wxT("%C4"),           // Comment 4
    SET_UPPER_LIMIT | USE_TEXT_COLOR
};


// horizontal segment above COMPANY NAME
Ki_WorkSheetData WS_Segm3 =
{
    WS_SEGMENT,
    &WS_Comment4,
    BLOCK_OX,  TEXTSIZE * 6,
    0,         TEXTSIZE * 6,
    NULL
};


// vertical segment of the left REV and SHEET
Ki_WorkSheetData WS_Segm4 =
{
    WS_SEGMENT,
    &WS_Segm3,
    BLOCK_REV_X + TEXTSIZE,TEXTSIZE * 4,
    BLOCK_REV_X + TEXTSIZE,            0,
    NULL
};


Ki_WorkSheetData WS_Segm5 =
{
    WS_SEGMENT,
    &WS_Segm4,
    BLOCK_OX,  TEXTSIZE * 2,
    0,         TEXTSIZE * 2,
    NULL
};


Ki_WorkSheetData WS_Segm6 =
{
    WS_SEGMENT,
    &WS_Segm5,
    BLOCK_OX,  TEXTSIZE * 4,
    0,         TEXTSIZE * 4,
    NULL
};


Ki_WorkSheetData WS_Segm7 =
{
    WS_SEGMENT,
    &WS_Segm6,
    BLOCK_OX - (TEXTSIZE * 11),TEXTSIZE * 4,
    BLOCK_OX - (TEXTSIZE * 11),TEXTSIZE * 2,
    NULL
};

#include <worksheet_shape_builder.h>

// Helper function which returns the text corresponding to the aIdent identifier
static wxString BuildFullText( const wxString& aTextbase,
                          const TITLE_BLOCK& aTitleBlock,
                          const wxString& aPaperFormat,
                          const wxString& aFileName,
                          const wxString& aSheetPathHumanReadable,
                          int aSheetCount, int aSheetNumber );


void WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList(
                       const wxString& aPaperFormat,
                       const wxString& aFileName,
                       const wxString& aSheetPathHumanReadable,
                       const TITLE_BLOCK& aTitleBlock,
                       EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor )
{
    wxSize              textsize( TEXTSIZE * m_milsToIu, TEXTSIZE * m_milsToIu );
    wxSize              size_ref( FRMREF_TXTSIZE * m_milsToIu,
                                  FRMREF_TXTSIZE * m_milsToIu );
    wxString            msg;

    // Left top corner position
    wxPoint lt_corner;
    lt_corner.x = m_LTmargin.x;
    lt_corner.y = m_LTmargin.y;

    // Right bottom corner position
    wxPoint rb_corner;
    rb_corner.x = m_pageSize.x - m_RBmargin.x;
    rb_corner.y = m_pageSize.y - m_RBmargin.y;

    // Draw the border.
    int ii, jj, ipas, gxpas, gypas;

    wxPoint pos = lt_corner;
    wxPoint end = rb_corner;
    for( ii = 0; ii < 2; ii++ )
    {
        Append( new WS_DRAW_ITEM_RECT(
                    wxPoint( pos.x * m_milsToIu, pos.y * m_milsToIu ),
                    wxPoint( end.x * m_milsToIu, end.y * m_milsToIu ),
                    m_penSize, aLineColor ) );

        pos.x += GRID_REF_W;
        pos.y += GRID_REF_W;
        end.x -= GRID_REF_W;
        end.y -= GRID_REF_W;
    }

    ipas    = ( rb_corner.x - lt_corner.x ) / PAS_REF;
    gxpas   = ( rb_corner.x - lt_corner.x ) / ipas;

    for( ii = lt_corner.x + gxpas, jj = 1; ipas > 0; ii += gxpas, jj++, ipas-- )
    {
        msg.Printf( wxT( "%d" ), jj );

        if( ii < rb_corner.x - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( ii * m_milsToIu, lt_corner.y * m_milsToIu ),
                        wxPoint( ii * m_milsToIu, ( lt_corner.y + GRID_REF_W ) * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( msg,
                                       wxPoint( ( ii - gxpas / 2 ) * m_milsToIu,
                                                ( lt_corner.y + GRID_REF_W / 2 ) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );

        if( ii < rb_corner.x - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( ii * m_milsToIu, rb_corner.y * m_milsToIu ),
                        wxPoint( ii * m_milsToIu, (rb_corner.y - GRID_REF_W ) * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( msg,
                                       wxPoint( ( ii - gxpas / 2 ) * m_milsToIu,
                                                ( rb_corner.y - GRID_REF_W / 2) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );
    }

    ipas    = ( rb_corner.y - lt_corner.y ) / PAS_REF;
    gypas   = ( rb_corner.y - lt_corner.y ) / ipas;

    for( ii = lt_corner.y + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        if( jj < 26 )
            msg.Printf( wxT( "%c" ), jj + 'A' );
        else // I hope 52 identifiers are enough...
            msg.Printf( wxT( "%c" ), 'a' + jj - 26 );

        if( ii < rb_corner.y - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( lt_corner.x * m_milsToIu, ii * m_milsToIu ),
                        wxPoint( ( lt_corner.x + GRID_REF_W ) * m_milsToIu, ii * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( msg,
                                       wxPoint( ( lt_corner.x + GRID_REF_W / 2 ) * m_milsToIu,
                                                ( ii - gypas / 2 ) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );

        if( ii < rb_corner.y - PAS_REF / 2 )
        {
            Append( new WS_DRAW_ITEM_LINE(
                        wxPoint( rb_corner.x * m_milsToIu, ii * m_milsToIu ),
                        wxPoint( ( rb_corner.x - GRID_REF_W ) * m_milsToIu, ii * m_milsToIu ),
                        m_penSize, aLineColor ) );
        }

        Append( new WS_DRAW_ITEM_TEXT( msg,
                                       wxPoint( ( rb_corner.x - GRID_REF_W / 2 ) * m_milsToIu,
                                                ( ii - gxpas / 2 ) * m_milsToIu ),
                                       size_ref, m_penSize, aLineColor ) );
    }

    int upperLimit = VARIABLE_BLOCK_START_POSITION;
    rb_corner.x -= GRID_REF_W;
    rb_corner.y -= GRID_REF_W;

    WS_DRAW_ITEM_TEXT* gtext;
    Ki_WorkSheetData*  WsItem;
    int pensize;
    bool bold;
    bool italic = false;
    EDA_COLOR_T color;

    for( WsItem = &WS_Segm7; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x   = (rb_corner.x - WsItem->m_Posx) * m_milsToIu;
        pos.y   = (rb_corner.y - WsItem->m_Posy) * m_milsToIu;

        msg.Empty();

        if( WsItem->m_Type == WS_TEXT && WsItem->m_TextBase )
            msg = BuildFullText( WsItem->m_TextBase, aTitleBlock, aPaperFormat, aFileName,
                             aSheetPathHumanReadable, m_sheetCount, m_sheetNumber );

        switch( WsItem->m_Type )
        {
        case WS_TEXT:
            if( msg.IsEmpty() )
                break;
            bold = false;
            pensize = m_penSize;
            color = aLineColor;
            if( WsItem->m_Flags & USE_TEXT_COLOR )
                color = aTextColor;

            if( WsItem->m_Flags & USE_BOLD )
            {
                bold = true;
                pensize = GetPenSizeForBold( std::min( textsize.x, textsize.y ) );
            }
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos, textsize,
                                                   pensize, color, italic, bold ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );

            if( WsItem->m_Flags & SET_UPPER_LIMIT )
                upperLimit = std::max( upperLimit, WsItem->m_Posy + TEXTSIZE );
            break;

        case WS_UPPER_SEGMENT:

            if( upperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy = upperLimit;
            WS_MostUpperLine.m_Endy = upperLimit;
            WS_MostLeftLine.m_Posy  = upperLimit;
            pos.y = (rb_corner.y - WsItem->m_Posy) * m_milsToIu;

        case WS_SEGMENT:
            end.x  = rb_corner.x - WsItem->m_Endx;
            end.y  = rb_corner.y - WsItem->m_Endy;
            Append( new WS_DRAW_ITEM_LINE( pos,
                                           wxPoint( end.x * m_milsToIu, end.y * m_milsToIu ),
                                           m_penSize, aLineColor ) );
            break;
        }
    }
}

// returns the full text corresponding to the aTextbase,
// after replacing format symbols by the corresponding value
wxString BuildFullText( const wxString& aTextbase,
                        const TITLE_BLOCK& aTitleBlock,
                        const wxString& aPaperFormat,
                        const wxString& aFileName,
                        const wxString& aSheetPathHumanReadable,
                        int aSheetCount, int aSheetNumber )
{
    wxString msg;

    /* Known formats
     * %% = replaced by %
     * %K = Kicad version
     * %Z = paper format name (A4, USLetter)
     * %Y = company name
     * %D = date
     * %R = revision
     * %S = sheet number
     * %N = number of sheets
     * %Cx = comment (x = 0 to 9 to identify the comment)
     * %F = filename
     * %P = sheet path (sheet full name)
     * %T = title
     */

    for( unsigned ii = 0; ii < aTextbase.Len(); ii++ )
    {
        if( aTextbase[ii] != '%' )
        {
            msg << aTextbase[ii];
            continue;
        }
        ii++;
        if( ii >= aTextbase.Len() )
            break;

        wxChar format = aTextbase[ii];
        switch( format )
        {
            case '%':
                msg += '%';
                break;

            case 'D':
                msg += aTitleBlock.GetDate();
                break;

            case 'R':
                msg += aTitleBlock.GetRevision();
                break;

            case 'K':
                msg += g_ProductName + wxGetApp().GetAppName();
                msg += wxT( " " ) + GetBuildVersion();
                break;

            case 'Z':
                msg += aPaperFormat;
                break;

            case 'S':
                msg << aSheetNumber;
                break;

            case 'N':
                msg << aSheetCount;
                break;

            case 'F':
                {
                    wxFileName fn( aFileName );
                    msg += fn.GetFullName();
                }
                break;

            case 'P':
                msg += aSheetPathHumanReadable;
                break;

            case 'Y':
                msg = aTitleBlock.GetCompany();
                break;

            case 'T':
                msg += aTitleBlock.GetTitle();
                break;

            case 'C':
                format = aTextbase[++ii];
                switch( format )
                {
                case '1':
                    msg += aTitleBlock.GetComment1();
                    break;

                case '2':
                    msg += aTitleBlock.GetComment2();
                    break;

                case '3':
                    msg += aTitleBlock.GetComment3();
                    break;

                case '4':
                    msg += aTitleBlock.GetComment4();
                    break;

                default:
                    break;
                }

            default:
                break;
        }
    }

    return msg;
}
