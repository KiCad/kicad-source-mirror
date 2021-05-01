/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
#include <locale_io.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_painter.h>
#include <drawing_sheet/drawing_sheet_reader_lexer.h>
#include <wx/ffile.h>

#include <wx/file.h>
#include <wx/mstream.h>


using namespace TB_READER_T;

/**
 * DRAWING_SHEET_READER_PARSER
 * holds data and functions pertinent to parsing a S-expression file
 * for a DS_DATA_MODEL.
 */
class DRAWING_SHEET_READER_PARSER : public DRAWING_SHEET_READER_LEXER
{
public:
    DRAWING_SHEET_READER_PARSER( const char* aLine, const wxString& aSource );
    void Parse( DS_DATA_MODEL* aLayout );

private:

    /**
     * Function parseInt
     * parses an integer and constrains it between two values.
     * @param aMin is the smallest return value.
     * @param aMax is the largest return value.
     * @return int - the parsed integer.
     */
    int parseInt( int aMin, int aMax );

    /**
     * Function parseDouble
     * parses a double
     * @return double - the parsed double.
     */
    double parseDouble();

    void parseSetup( DS_DATA_MODEL* aLayout );

    /**
     * parse a graphic item starting by "(line" or "(rect" and read parameters.
     */
    void parseGraphic( DS_DATA_ITEM * aItem );

    /**
     * parse a text item starting by "(tbtext" and read parameters.
     */
    void parseText( DS_DATA_ITEM_TEXT * aItem );

    /**
     * parse a polygon item starting by "( polygon" and read parameters.
     * the list of corners included in this description is read by parsePolyOutline
     */
    void parsePolygon( DS_DATA_ITEM_POLYGONS * aItem );

    /**
     * parse a list of corners starting by "( pts" and read coordinates.
     */
    void parsePolyOutline( DS_DATA_ITEM_POLYGONS * aItem );


    /**
     * parse a bitmap item starting by "( bitmap" and read parameters.
     */
    void parseBitmap( DS_DATA_ITEM_BITMAP * aItem );

    void parseCoordinate( POINT_COORD& aCoord);
    void readOption( DS_DATA_ITEM * aItem );
    void readPngdata( DS_DATA_ITEM_BITMAP * aItem );
};

// PCB_PLOT_PARAMS_PARSER

DRAWING_SHEET_READER_PARSER::DRAWING_SHEET_READER_PARSER( const char* aLine,
                                                          const wxString& aSource ) :
        DRAWING_SHEET_READER_LEXER( aLine, aSource )
{
}


wxString convertLegacyVariableRefs( const wxString& aTextbase )
{
    wxString msg;

    /*
     * Legacy formats
     * %% = replaced by %
     * %K = Kicad version
     * %Z = paper format name (A4, USLetter)
     * %Y = company name
     * %D = date
     * %R = revision
     * %S = sheet number
     * %N = number of sheets
     * %L = layer name
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

        if( ++ii >= aTextbase.Len() )
            break;

        wxChar format = aTextbase[ii];

        switch( format )
        {
            case '%': msg += '%';                       break;
            case 'D': msg += wxT( "${ISSUE_DATE}" );    break;
            case 'R': msg += wxT( "${REVISION}" );      break;
            case 'K': msg += wxT( "${KICAD_VERSION}" ); break;
            case 'Z': msg += wxT( "${PAPER}" );         break;
            case 'S': msg += wxT( "${#}" );             break;
            case 'N': msg += wxT( "${##}" );            break;
            case 'F': msg += wxT( "${FILENAME}" );      break;
            case 'L': msg += wxT( "${LAYER}" );         break;
            case 'P': msg += wxT( "${SHEETNAME}" );     break;
            case 'Y': msg += wxT( "${COMPANY}" );       break;
            case 'T': msg += wxT( "${TITLE}" );         break;
            case 'C':
                format = aTextbase[++ii];

                switch( format )
                {
                case '0': msg += wxT( "${COMMENT1}" );  break;
                case '1': msg += wxT( "${COMMENT2}" );  break;
                case '2': msg += wxT( "${COMMENT3}" );  break;
                case '3': msg += wxT( "${COMMENT4}" );  break;
                case '4': msg += wxT( "${COMMENT5}" );  break;
                case '5': msg += wxT( "${COMMENT6}" );  break;
                case '6': msg += wxT( "${COMMENT7}" );  break;
                case '7': msg += wxT( "${COMMENT8}" );  break;
                case '8': msg += wxT( "${COMMENT9}" );  break;
                }
                break;

            default:
                break;
        }
    }

    return msg;
}


void DRAWING_SHEET_READER_PARSER::Parse( DS_DATA_MODEL* aLayout )
{
    DS_DATA_ITEM* item;
    LOCALE_IO     toggle;

    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        if( token == T_page_layout || token == T_drawing_sheet )
            continue;

        switch( token )
        {
        case T_setup:   // Defines default values for graphic items
            parseSetup( aLayout );
            break;

        case T_line:
            item = new DS_DATA_ITEM( DS_DATA_ITEM::DS_SEGMENT );
            parseGraphic( item );
            aLayout->Append( item );
            break;

        case T_rect:
            item = new DS_DATA_ITEM( DS_DATA_ITEM::DS_RECT );
            parseGraphic( item );
            aLayout->Append( item );
            break;

        case T_polygon:
            item = new DS_DATA_ITEM_POLYGONS();
            parsePolygon(  (DS_DATA_ITEM_POLYGONS*) item );
            aLayout->Append( item );
            break;

        case T_bitmap:
            item = new DS_DATA_ITEM_BITMAP( NULL );
            parseBitmap( (DS_DATA_ITEM_BITMAP*) item );
            aLayout->Append( item );
            break;

        case T_tbtext:
            NeedSYMBOLorNUMBER();
            item = new DS_DATA_ITEM_TEXT( convertLegacyVariableRefs( FromUTF8() ) );
            parseText( (DS_DATA_ITEM_TEXT*) item );
            aLayout->Append( item );
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}


void DRAWING_SHEET_READER_PARSER::parseSetup( DS_DATA_MODEL* aLayout )
{
    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        switch( token )
        {
        case T_LEFT:
            break;

        case T_linewidth:
            aLayout->m_DefaultLineWidth = parseDouble();
            NeedRIGHT();
            break;

        case T_textsize:
            aLayout->m_DefaultTextSize.x = parseDouble();
            aLayout->m_DefaultTextSize.y = parseDouble();
            NeedRIGHT();
            break;

        case T_textlinewidth:
            aLayout->m_DefaultTextThickness = parseDouble();
            NeedRIGHT();
            break;

        case T_left_margin:
            aLayout->SetLeftMargin( parseDouble() );
            NeedRIGHT();
            break;

        case T_right_margin:
            aLayout->SetRightMargin( parseDouble() );
            NeedRIGHT();
            break;

        case T_top_margin:
            aLayout->SetTopMargin( parseDouble() );
            NeedRIGHT();
            break;

        case T_bottom_margin:
            aLayout->SetBottomMargin( parseDouble() );
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }

    // The file is well-formed.  If it has no further items, then that's the way the
    // user wants it.
    aLayout->AllowVoidList( true );
}


void DRAWING_SHEET_READER_PARSER::parsePolygon( DS_DATA_ITEM_POLYGONS * aItem )
{
    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_comment:
            NeedSYMBOLorNUMBER();
            aItem->m_Info =  FromUTF8();
            NeedRIGHT();
            break;

        case T_pos:
            parseCoordinate( aItem->m_Pos );
            break;

        case T_name:
            NeedSYMBOLorNUMBER();
            aItem->m_Name =  FromUTF8();
            NeedRIGHT();
            break;

        case T_option:
            readOption( aItem );
            break;

        case T_pts:
            parsePolyOutline( aItem );
            aItem->CloseContour();
            break;

        case T_rotate:
            aItem->m_Orient = parseDouble();
            NeedRIGHT();
            break;

        case T_repeat:
            aItem->m_RepeatCount = parseInt( -1, 100 );
            NeedRIGHT();
            break;

        case T_incrx:
            aItem->m_IncrementVector.x = parseDouble();
            NeedRIGHT();
            break;

        case T_incry:
            aItem->m_IncrementVector.y = parseDouble();
            NeedRIGHT();
            break;

        case T_linewidth:
            aItem->m_LineWidth = parseDouble();
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }

    aItem->SetBoundingBox();
}

void DRAWING_SHEET_READER_PARSER::parsePolyOutline( DS_DATA_ITEM_POLYGONS * aItem )
{
    DPOINT corner;

    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_xy:
            corner.x = parseDouble();
            corner.y = parseDouble();
            aItem->AppendCorner( corner );
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}


void DRAWING_SHEET_READER_PARSER::parseBitmap( DS_DATA_ITEM_BITMAP * aItem )
{
    BITMAP_BASE* image = new BITMAP_BASE;
    aItem->m_ImageBitmap = image;

    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_name:
            NeedSYMBOLorNUMBER();
            aItem->m_Name =  FromUTF8();
            NeedRIGHT();
            break;

        case T_pos:
            parseCoordinate( aItem->m_Pos );
            break;

        case T_repeat:
            aItem->m_RepeatCount = parseInt( -1, 100 );
            NeedRIGHT();
            break;

        case T_incrx:
            aItem->m_IncrementVector.x = parseDouble();
            NeedRIGHT();
            break;

        case T_incry:
            aItem->m_IncrementVector.y = parseDouble();
            NeedRIGHT();
            break;

        case T_linewidth:
            aItem->m_LineWidth = parseDouble();
            NeedRIGHT();
            break;

        case T_scale:
            aItem->m_ImageBitmap->SetScale( parseDouble() );
            NeedRIGHT();
            break;

        case T_pngdata:
            readPngdata( aItem );
            break;

        case T_option:
            readOption( aItem );
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}

void DRAWING_SHEET_READER_PARSER::readPngdata( DS_DATA_ITEM_BITMAP * aItem )
{
    std::string tmp;

    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_data:
            NeedSYMBOLorNUMBER();
            tmp += CurStr();
            tmp += "\n";
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }

    tmp += "EndData";

    wxString msg;
    STRING_LINE_READER str_reader( tmp, wxT("Png kicad_wks data") );

    if( ! aItem->m_ImageBitmap->LoadData( str_reader, msg ) )
        wxLogMessage(msg);
}


void DRAWING_SHEET_READER_PARSER::readOption( DS_DATA_ITEM * aItem )
{
    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        switch( token )
        {
        case T_page1only:  aItem->SetPage1Option( FIRST_PAGE_ONLY );  break;
        case T_notonpage1: aItem->SetPage1Option( SUBSEQUENT_PAGES ); break;
        default:           Unexpected( CurText() ); break;
        }
    }
}


void DRAWING_SHEET_READER_PARSER::parseGraphic( DS_DATA_ITEM * aItem )
{
    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();
        else
        {
            // If another token than T_LEFT is read here, this is an error
            // however, due to a old bug in kicad, the token T_end can be found
            // without T_LEFT in a very few .wks files (perhaps only one in a demo).
            // So this ugly hack disables the error detection.
            if( token != T_end )
                Unexpected( CurText() );
        }

        switch( token )
        {
        case T_comment:
            NeedSYMBOLorNUMBER();
            aItem->m_Info = FromUTF8();
            NeedRIGHT();
            break;

        case T_option:
            readOption( aItem );
            break;

        case T_name:
            NeedSYMBOLorNUMBER();
            aItem->m_Name = FromUTF8();
            NeedRIGHT();
            break;

        case T_start:
            parseCoordinate( aItem->m_Pos );
            break;

        case T_end:
            parseCoordinate( aItem->m_End );
            break;

        case T_repeat:
            aItem->m_RepeatCount = parseInt( -1, 100 );
            NeedRIGHT();
            break;

        case T_incrx:
            aItem->m_IncrementVector.x = parseDouble();
            NeedRIGHT();
            break;

        case T_incry:
            aItem->m_IncrementVector.y = parseDouble();
            NeedRIGHT();
            break;

        case T_linewidth:
            aItem->m_LineWidth = parseDouble();
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}


void DRAWING_SHEET_READER_PARSER::parseText( DS_DATA_ITEM_TEXT* aItem )
{
    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_comment:
            NeedSYMBOLorNUMBER();
            aItem->m_Info =  FromUTF8();
            NeedRIGHT();
            break;

        case T_option:
            readOption( aItem );
            break;

        case T_name:
            NeedSYMBOLorNUMBER();
            aItem->m_Name =  FromUTF8();
            NeedRIGHT();
            break;

        case T_pos:
            parseCoordinate( aItem->m_Pos );
            break;

        case T_repeat:
            aItem->m_RepeatCount = parseInt( -1, 100 );
            NeedRIGHT();
            break;

        case T_incrx:
            aItem->m_IncrementVector.x = parseDouble();
            NeedRIGHT();
            break;

        case T_incry:
            aItem->m_IncrementVector.y = parseDouble();
            NeedRIGHT();
            break;

        case T_incrlabel:
            aItem->m_IncrementLabel = parseInt(INT_MIN, INT_MAX);
            NeedRIGHT();
            break;

        case T_maxlen:
            aItem->m_BoundingBoxSize.x = parseDouble();
            NeedRIGHT();
            break;

        case T_maxheight:
            aItem->m_BoundingBoxSize.y = parseDouble();
            NeedRIGHT();
            break;

        case T_font:
            for( token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
            {
                switch( token )
                {
                case T_LEFT:
                    break;

                case T_bold:
                    aItem->m_Bold = true;
                    break;

                case T_italic:
                    aItem->m_Italic = true;
                    break;

                case T_size:
                    aItem->m_TextSize.x = parseDouble();
                    aItem->m_TextSize.y = parseDouble();
                    NeedRIGHT();
                    break;

                case T_linewidth:
                    aItem->m_LineWidth = parseDouble();
                    NeedRIGHT();
                    break;

                default:
                    Unexpected( CurText() );
                    break;
                }
            }
            break;

        case T_justify:
            for( token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
            {
                switch( token )
                {
                case T_center:
                    aItem->m_Hjustify = GR_TEXT_HJUSTIFY_CENTER;
                    aItem->m_Vjustify = GR_TEXT_VJUSTIFY_CENTER;
                    break;

                case T_left:
                    aItem->m_Hjustify = GR_TEXT_HJUSTIFY_LEFT;
                    break;

                case T_right:
                    aItem->m_Hjustify = GR_TEXT_HJUSTIFY_RIGHT;
                    break;

                case T_top:
                    aItem->m_Vjustify = GR_TEXT_VJUSTIFY_TOP;
                    break;

                case T_bottom:
                    aItem->m_Vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
                    break;

                default:
                    Unexpected( CurText() );
                    break;
                }
            }
            break;

        case T_rotate:
            aItem->m_Orient = parseDouble();
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}

// parse an expression like " 25 1 ltcorner)"
void DRAWING_SHEET_READER_PARSER::parseCoordinate( POINT_COORD& aCoord)
{
    aCoord.m_Pos.x = parseDouble();
    aCoord.m_Pos.y = parseDouble();

    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        switch( token )
        {
        case T_ltcorner: aCoord.m_Anchor = LT_CORNER; break;
        case T_lbcorner: aCoord.m_Anchor = LB_CORNER; break;
        case T_rbcorner: aCoord.m_Anchor = RB_CORNER; break;
        case T_rtcorner: aCoord.m_Anchor = RT_CORNER; break;
        default:         Unexpected( CurText() ); break;
        }
    }
}

int DRAWING_SHEET_READER_PARSER::parseInt( int aMin, int aMax )
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );

    int val = atoi( CurText() );

    if( val < aMin )
        val = aMin;
    else if( val > aMax )
        val = aMax;

    return val;
}


double DRAWING_SHEET_READER_PARSER::parseDouble()
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );

    double val = strtod( CurText(), NULL );

    return val;
}

// defaultDrawingSheet is the default page layout description using the S expr.
extern const char defaultDrawingSheet[];

void DS_DATA_MODEL::SetDefaultLayout()
{
    SetPageLayout( defaultDrawingSheet, false, wxT( "default page" ) );
}

// Returns defaultDrawingSheet as a string;
wxString DS_DATA_MODEL::DefaultLayout()
{
    return wxString( defaultDrawingSheet );
}

// emptyDrawingSheet is a "empty" page layout description using the S expr.
// there is a 0 length line to fool something somewhere.
extern const char emptyDrawingSheet[];

void DS_DATA_MODEL::SetEmptyLayout()
{
    SetPageLayout( emptyDrawingSheet, false, wxT( "empty page" ) );
}


wxString DS_DATA_MODEL::EmptyLayout()
{
    return wxString( emptyDrawingSheet );
}


void DS_DATA_MODEL::SetPageLayout( const char* aPageLayout, bool Append, const wxString& aSource )
{
    if( ! Append )
        ClearList();

    DRAWING_SHEET_READER_PARSER lp_parser( aPageLayout, wxT( "Sexpr_string" ) );

    try
    {
        lp_parser.Parse( this );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogMessage( ioe.What() );
    }
}


bool DS_DATA_MODEL::LoadDrawingSheet( const wxString& aFullFileName, bool Append )
{
    wxString fullFileName = aFullFileName;

    if( !Append )
    {
        if( fullFileName.IsEmpty() )
            wxGetEnv( wxT( "KICAD_WKSFILE" ), &fullFileName );

        if( fullFileName.IsEmpty() || !wxFileExists( fullFileName ) )
        {
            #if 0
            if( !fullFileName.IsEmpty() )
                wxLogMessage( wxT( "Drawing sheet file <%s> not found" ), fullFileName.GetData() );
            #endif
            SetDefaultLayout();
            return false;
        }
    }

    wxFFile wksFile( fullFileName, "rb" );

    if( ! wksFile.IsOpened() )
    {
        if( !Append )
            SetDefaultLayout();
        return false;
    }

    size_t filelen = wksFile.Length();
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(filelen+10);

    if( wksFile.Read( buffer.get(), filelen ) != filelen )
    {
        wxLogMessage( _( "The file \"%s\" was not fully read" ), fullFileName.GetData() );
        return false;
    }
    else
    {
        buffer[filelen]=0;

        if( ! Append )
            ClearList();

        DRAWING_SHEET_READER_PARSER pl_parser( buffer.get(), fullFileName );

        try
        {
            pl_parser.Parse( this );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogMessage( ioe.What() );
            return false;
        }
        catch( const std::bad_alloc& )
        {
            wxLogMessage( "Memory exhaustion reading drawing sheet" );
            return false;
        }
    }

    return true;
}
