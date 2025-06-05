/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <charconv>
#include <fmt/format.h>
#include <wx/base64.h>
#include <wx/ffile.h>
#include <wx/log.h>

#include <eda_item.h>
#include <locale_io.h>
#include <string_utils.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_painter.h>
#include <drawing_sheet/drawing_sheet_lexer.h>
#include <drawing_sheet/ds_file_versions.h>
#include <font/font.h>

using namespace DRAWINGSHEET_T;

/**
 * Hold data and functions pertinent to parsing a S-expression file for a #DS_DATA_MODEL.
 */
class DRAWING_SHEET_PARSER : public DRAWING_SHEET_LEXER
{
public:
    DRAWING_SHEET_PARSER( const char* aLine, const wxString& aSource );
    void Parse( DS_DATA_MODEL* aLayout );

private:
    /**
     * Parse the data specified at the very beginning of the file, like version and the
     * application used to create this file.
     */
    void parseHeader( T aHeaderType );

    /**
     * Parse an integer.
     */
    int parseInt();

    /**
     * Parse an integer and constrain it between two values.
     *
     * @param aMin is the smallest return value.
     * @param aMax is the largest return value.
     * @return int - the parsed integer.
     */
    int parseInt( int aMin, int aMax );

    /**
     * Parse a double.
     *
     * @return double - the parsed double.
     */
    double parseDouble();

    void parseSetup( DS_DATA_MODEL* aLayout );

    /**
     * Parse a graphic item starting by "(line" or "(rect" and read parameters.
     */
    void parseGraphic( DS_DATA_ITEM * aItem );

    /**
     * Parse a text item starting by "(tbtext" and read parameters.
     */
    void parseText( DS_DATA_ITEM_TEXT * aItem );

    /**
     * Parse a polygon item starting by "( polygon" and read parameters.
     * the list of corners included in this description is read by parsePolyOutline.
     */
    void parsePolygon( DS_DATA_ITEM_POLYGONS * aItem );

    /**
     * Parse a list of corners starting by "( pts" and read coordinates.
     */
    void parsePolyOutline( DS_DATA_ITEM_POLYGONS * aItem );


    /**
     * Parse a bitmap item starting by "( bitmap" and read parameters.
     */
    void parseBitmap( DS_DATA_ITEM_BITMAP * aItem );

    void parseCoordinate( POINT_COORD& aCoord);
    void readOption( DS_DATA_ITEM * aItem );
    void readPngdata( DS_DATA_ITEM_BITMAP * aItem );

private:
    int      m_requiredVersion;
    wxString m_generatorVersion;
};


DRAWING_SHEET_PARSER::DRAWING_SHEET_PARSER( const char* aLine,
                                            const wxString& aSource ) :
        DRAWING_SHEET_LEXER( aLine, aSource ),
        m_requiredVersion( 0 )
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
            case 'P': msg += wxT( "${SHEETPATH}" );     break;
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


void DRAWING_SHEET_PARSER::Parse( DS_DATA_MODEL* aLayout )
{
    DS_DATA_ITEM* item;
    LOCALE_IO     toggle;

    NeedLEFT();
    T token = NextTok();

    parseHeader( token );
    aLayout->SetFileFormatVersionAtLoad( m_requiredVersion );

    auto checkVersion =
            [&]()
            {
                if( m_requiredVersion > SEXPR_WORKSHEET_FILE_VERSION )
                {
                    throw FUTURE_FORMAT_ERROR( fmt::format( "{}", m_requiredVersion ),
                                               m_generatorVersion );
                }
            };

    for( token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_generator:
            // (generator "genname"); we don't care about it at the moment.
            NeedSYMBOL();
            NeedRIGHT();
            break;

        case T_generator_version:
            NextTok();
            m_generatorVersion = FromUTF8();
            NeedRIGHT();
            break;

        case T_setup:   // Defines default values for graphic items
            // Check the version here, because the generator and generator_version (if available)
            // will have been parsed by now given the order the formatter writes them in
            checkVersion();
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
            parsePolygon( (DS_DATA_ITEM_POLYGONS*) item );
            aLayout->Append( item );
            break;

        case T_bitmap:
            item = new DS_DATA_ITEM_BITMAP( NULL );
            parseBitmap( (DS_DATA_ITEM_BITMAP*) item );

            // Drop invalid bitmaps
            if( static_cast<DS_DATA_ITEM_BITMAP*>( item )->m_ImageBitmap->GetOriginalImageData() )
            {
                aLayout->Append( item );
            }
            else
            {
                delete static_cast<DS_DATA_ITEM_BITMAP*>( item )->m_ImageBitmap;
                delete item;
            }

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


void DRAWING_SHEET_PARSER::parseHeader( T aHeaderType )
{
    // The older files had no versioning and their first token after the initial left parenthesis
    // was a `page_layout` or `drawing_sheet` token. The newer files have versions and have a
    // `kicad_wks` token instead.

    if( aHeaderType == T_kicad_wks || aHeaderType == T_drawing_sheet )
    {
        NeedLEFT();

        T tok = NextTok();

        if( tok == T_version )
        {
            m_requiredVersion = parseInt();

            NeedRIGHT();
        }
        else
        {
            Expecting( T_version );
        }
    }
    else
    {
        // We assign version 0 to files that were created before there was any versioning of
        // worksheets. The below line is not strictly necessary, as `m_requiredVersion` is already
        // initialized to 0 in the constructor.
        m_requiredVersion = 0;
    }
}


void DRAWING_SHEET_PARSER::parseSetup( DS_DATA_MODEL* aLayout )
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


void DRAWING_SHEET_PARSER::parsePolygon( DS_DATA_ITEM_POLYGONS * aItem )
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
            aItem->m_Orient = EDA_ANGLE( parseDouble(), DEGREES_T );
            NeedRIGHT();
            break;

        case T_repeat:
            aItem->m_RepeatCount = parseInt( 1, 100 );
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


void DRAWING_SHEET_PARSER::parsePolyOutline( DS_DATA_ITEM_POLYGONS * aItem )
{
    VECTOR2D corner;

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


void DRAWING_SHEET_PARSER::parseBitmap( DS_DATA_ITEM_BITMAP * aItem )
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
            aItem->m_RepeatCount = parseInt( 1, 100 );
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

        case T_comment:
            NeedSYMBOLorNUMBER();
            aItem->m_Info = FromUTF8();
            NeedRIGHT();
            break;

        case T_data:
        {
            token = NextTok();

            wxString data;

            // Reserve 512K because most image files are going to be larger than the default
            // 1K that wxString reserves.
            data.reserve( 1 << 19 );

            while( token != T_RIGHT )
            {
                if( !IsSymbol( token ) )
                    Expecting( "base64 image data" );

                data += FromUTF8();
                token = NextTok();
            }

            wxMemoryBuffer buffer = wxBase64Decode( data );

            if( !aItem->m_ImageBitmap->ReadImageFile( buffer ) )
                THROW_IO_ERROR( _( "Failed to read image data." ) );

            break;
        }

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


void DRAWING_SHEET_PARSER::readPngdata( DS_DATA_ITEM_BITMAP * aItem )
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

    aItem->m_ImageBitmap->LoadLegacyData( str_reader, msg );
}


void DRAWING_SHEET_PARSER::readOption( DS_DATA_ITEM * aItem )
{
    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        switch( token )
        {
        case T_page1only:  aItem->SetPage1Option( FIRST_PAGE_ONLY );  break;
        case T_notonpage1: aItem->SetPage1Option( SUBSEQUENT_PAGES ); break;
        default:           Unexpected( CurText() );                   break;
        }
    }
}


void DRAWING_SHEET_PARSER::parseGraphic( DS_DATA_ITEM * aItem )
{
    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
        if( token == T_LEFT )
        {
            token = NextTok();
        }
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
            aItem->m_RepeatCount = parseInt( 1, 100 );
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


void DRAWING_SHEET_PARSER::parseText( DS_DATA_ITEM_TEXT* aItem )
{
    if( m_requiredVersion < 20210606 )
        aItem->m_TextBase = ConvertToNewOverbarNotation( aItem->m_TextBase );

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
            aItem->m_RepeatCount = parseInt( 1, 100 );
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
        {
            wxString faceName;

            aItem->m_TextColor = COLOR4D::UNSPECIFIED;

            for( token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
            {
                switch( token )
                {
                case T_LEFT:
                    break;

                case T_face:
                    NeedSYMBOL();
                    faceName = FromUTF8();
                    NeedRIGHT();
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

                case T_color:
                    aItem->m_TextColor.r = parseInt( 0, 255 ) / 255.0;
                    aItem->m_TextColor.g = parseInt( 0, 255 ) / 255.0;
                    aItem->m_TextColor.b = parseInt( 0, 255 ) / 255.0;
                    aItem->m_TextColor.a = std::clamp( parseDouble(), 0.0, 1.0 );
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

            if( !faceName.IsEmpty() )
                aItem->m_Font = KIFONT::FONT::GetFont( faceName, aItem->m_Bold, aItem->m_Italic );

            break;
        }

        case T_justify:
            for( token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
            {
                switch( token )
                {
                case T_center:
                    aItem->m_Hjustify = GR_TEXT_H_ALIGN_CENTER;
                    aItem->m_Vjustify = GR_TEXT_V_ALIGN_CENTER;
                    break;

                case T_left:
                    aItem->m_Hjustify = GR_TEXT_H_ALIGN_LEFT;
                    break;

                case T_right:
                    aItem->m_Hjustify = GR_TEXT_H_ALIGN_RIGHT;
                    break;

                case T_top:
                    aItem->m_Vjustify = GR_TEXT_V_ALIGN_TOP;
                    break;

                case T_bottom:
                    aItem->m_Vjustify = GR_TEXT_V_ALIGN_BOTTOM;
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


void DRAWING_SHEET_PARSER::parseCoordinate( POINT_COORD& aCoord)
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
        default:         Unexpected( CurText() );     break;
        }
    }
}


int DRAWING_SHEET_PARSER::parseInt()
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );

    return atoi( CurText() );
}


int DRAWING_SHEET_PARSER::parseInt( int aMin, int aMax )
{
    int val = parseInt();

    if( val < aMin )
        val = aMin;
    else if( val > aMax )
        val = aMax;

    return val;
}


double DRAWING_SHEET_PARSER::parseDouble()
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );


    return DSNLEXER::parseDouble();
}


// defaultDrawingSheet is the default drawing sheet using the S expr.
extern const char defaultDrawingSheet[];


void DS_DATA_MODEL::SetDefaultLayout()
{
    SetPageLayout( defaultDrawingSheet, false, wxT( "default page" ) );
}


wxString DS_DATA_MODEL::DefaultLayout()
{
    return wxString( defaultDrawingSheet );
}


// emptyDrawingSheet is a "empty" drawing sheet using the S expr.
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

    DRAWING_SHEET_PARSER parser( aPageLayout, wxT( "Sexpr_string" ) );

    try
    {
        parser.Parse( this );
    }
    catch( ... )
    {
        // best efforts
    }
}


bool DS_DATA_MODEL::LoadDrawingSheet( const wxString& aFullFileName, wxString* aMsg, bool aAppend )
{
    if( !aAppend )
    {
        if( aFullFileName.IsEmpty() )
        {
            SetDefaultLayout();
            return true; // we assume its fine / default init
        }

        if( !wxFileExists( aFullFileName ) )
        {
            if( aMsg )
                *aMsg = _( "File not found." );

            SetDefaultLayout();
            return false;
        }
    }

    wxFFile wksFile( aFullFileName, wxS( "rb" ) );

    if( ! wksFile.IsOpened() )
    {
        if( aMsg )
            *aMsg = _( "File could not be opened." );

        if( !aAppend )
            SetDefaultLayout();

        return false;
    }

    size_t filelen = wksFile.Length();
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(filelen+10);

    if( wksFile.Read( buffer.get(), filelen ) != filelen )
    {
        if( aMsg )
            *aMsg = _( "Drawing sheet was not fully read." );

        return false;
    }
    else
    {
        buffer[filelen]=0;

        if( ! aAppend )
            ClearList();

        DRAWING_SHEET_PARSER parser( buffer.get(), aFullFileName );

        try
        {
            parser.Parse( this );
        }
        catch( const IO_ERROR& ioe )
        {
            if( aMsg )
                *aMsg = ioe.What();

            return false;
        }
        catch( const std::bad_alloc& )
        {
            if( aMsg )
                *aMsg = _( "Ran out of memory." );

            return false;
        }
    }

    return true;
}
