/**
 * @file common/page_layout/page_layout_reader.cpp
 * @brief read an S expression of description of graphic items and texts
 * to build a title block and page layout
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
#include <base_struct.h>
#include <worksheet.h>
#include <worksheet_shape_builder.h>
#include <class_worksheet_dataitem.h>
#include <page_layout_reader_lexer.h>


using namespace TB_READER_T;

/**
 * Class PAGE_LAYOUT_READER_PARSER
 * holds data and functions pertinent to parsing a S-expression file
 * for a WORKSHEET_LAYOUT.
 */
class PAGE_LAYOUT_READER_PARSER : public PAGE_LAYOUT_READER_LEXER
{
public:
    PAGE_LAYOUT_READER_PARSER( const char* aLine, const wxString& aSource );
    void Parse( WORKSHEET_LAYOUT* aLayout )
                throw( PARSE_ERROR, IO_ERROR );

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

    void parseSetup( WORKSHEET_LAYOUT* aLayout ) throw( IO_ERROR, PARSE_ERROR );
    void parseGraphic( WORKSHEET_DATAITEM * aItem ) throw( IO_ERROR, PARSE_ERROR );
    void parseText( WORKSHEET_DATAITEM_TEXT * aItem ) throw( IO_ERROR, PARSE_ERROR );
    void parsePolygon( WORKSHEET_DATAITEM_POLYPOLYGON * aItem )
        throw( IO_ERROR, PARSE_ERROR );
    void parsePolyOutline( WORKSHEET_DATAITEM_POLYPOLYGON * aItem )
        throw( IO_ERROR, PARSE_ERROR );
    void parseBitmap( WORKSHEET_DATAITEM_BITMAP * aItem )
        throw( IO_ERROR, PARSE_ERROR );
    void parseCoordinate( POINT_COORD& aCoord) throw( IO_ERROR, PARSE_ERROR );
    void readOption( WORKSHEET_DATAITEM * aItem ) throw( IO_ERROR, PARSE_ERROR );
    void readPngdata( WORKSHEET_DATAITEM_BITMAP * aItem ) throw( IO_ERROR, PARSE_ERROR );
};

// PCB_PLOT_PARAMS_PARSER

PAGE_LAYOUT_READER_PARSER::PAGE_LAYOUT_READER_PARSER( const char* aLine, const wxString& aSource ) :
    PAGE_LAYOUT_READER_LEXER( aLine, aSource )
{
}


void PAGE_LAYOUT_READER_PARSER::Parse( WORKSHEET_LAYOUT* aLayout )
                             throw( PARSE_ERROR, IO_ERROR )
{
    T token;
    WORKSHEET_DATAITEM * item;

    LOCALE_IO toggle;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

        if( token == T_LEFT )
            token = NextTok();

        if( token == T_page_layout )
            continue;

        switch( token )
        {
        case T_setup:   // Defines default values for graphic items
            parseSetup( aLayout );
            break;

        case T_line:
            item = new WORKSHEET_DATAITEM( WORKSHEET_DATAITEM::WS_SEGMENT );
            parseGraphic( item );
            aLayout->Append( item );
            break;

        case T_rect:
            item = new WORKSHEET_DATAITEM( WORKSHEET_DATAITEM::WS_RECT );
            parseGraphic( item );
            aLayout->Append( item );
            break;

        case T_polygon:
            item = new WORKSHEET_DATAITEM_POLYPOLYGON();
            parsePolygon(  (WORKSHEET_DATAITEM_POLYPOLYGON*) item );
            aLayout->Append( item );
            break;

        case T_bitmap:
            item = new WORKSHEET_DATAITEM_BITMAP( NULL );
            parseBitmap( (WORKSHEET_DATAITEM_BITMAP*) item );
            aLayout->Append( item );
            break;

        case T_tbtext:
            NeedSYMBOLorNUMBER();
            item = new WORKSHEET_DATAITEM_TEXT( FromUTF8() );
            parseText( (WORKSHEET_DATAITEM_TEXT*) item );
            aLayout->Append( item );
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}

void PAGE_LAYOUT_READER_PARSER::parseSetup( WORKSHEET_LAYOUT* aLayout )
    throw( IO_ERROR, PARSE_ERROR )
{
    T token;
    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

        switch( token )
        {
        case T_LEFT:
            break;

        case T_linewidth:
            WORKSHEET_DATAITEM::m_DefaultLineWidth = parseDouble();
            NeedRIGHT();
            break;

        case T_textsize:
            WORKSHEET_DATAITEM::m_DefaultTextSize.x = parseDouble();
            WORKSHEET_DATAITEM::m_DefaultTextSize.y = parseDouble();
            NeedRIGHT();
            break;

        case T_textlinewidth:
            WORKSHEET_DATAITEM::m_DefaultTextThickness = parseDouble();
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
}

void PAGE_LAYOUT_READER_PARSER::parsePolygon( WORKSHEET_DATAITEM_POLYPOLYGON * aItem )
    throw( IO_ERROR, PARSE_ERROR )
{
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

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

void PAGE_LAYOUT_READER_PARSER::parsePolyOutline( WORKSHEET_DATAITEM_POLYPOLYGON * aItem )
    throw( IO_ERROR, PARSE_ERROR )
{
    DPOINT corner;
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

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

#include <wx/mstream.h>
void PAGE_LAYOUT_READER_PARSER::parseBitmap( WORKSHEET_DATAITEM_BITMAP * aItem )
    throw( IO_ERROR, PARSE_ERROR )
{
    T token;
    BITMAP_BASE* image = new BITMAP_BASE;
    aItem->m_ImageBitmap = image;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

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
            aItem->m_ImageBitmap->m_Scale = parseDouble();
            NeedRIGHT();
            break;

        case T_pngdata:
            readPngdata( aItem );
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}

void PAGE_LAYOUT_READER_PARSER::readPngdata( WORKSHEET_DATAITEM_BITMAP * aItem )
            throw( IO_ERROR, PARSE_ERROR )
{
    std::string tmp;
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

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
    STRING_LINE_READER reader( tmp, wxT("Png kicad_wks data") );
    if( ! aItem->m_ImageBitmap->LoadData( reader, msg ) )
    {
        wxLogMessage(msg);
    }
}


void PAGE_LAYOUT_READER_PARSER::readOption( WORKSHEET_DATAITEM * aItem )
    throw( IO_ERROR, PARSE_ERROR )
{
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

        switch( token )
        {
        case T_page1only:
            aItem->SetPage1Option( 1 );
            break;

        case T_notonpage1:
            aItem->SetPage1Option( -1 );
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}


void PAGE_LAYOUT_READER_PARSER::parseGraphic( WORKSHEET_DATAITEM * aItem )
    throw( IO_ERROR, PARSE_ERROR )
{
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

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


void PAGE_LAYOUT_READER_PARSER::parseText( WORKSHEET_DATAITEM_TEXT* aItem )
    throw( IO_ERROR, PARSE_ERROR )
{
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

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
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_EOF)
                   break;

                switch( token )
                {
                case T_LEFT:
                    break;

                case T_bold:
                    aItem->SetBold( true );
                    break;

                case T_italic:
                    aItem->SetItalic( true );
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
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_EOF)
                   break;

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
void PAGE_LAYOUT_READER_PARSER::parseCoordinate( POINT_COORD& aCoord)
    throw( IO_ERROR, PARSE_ERROR )
{
    T token;

    aCoord.m_Pos.x = parseDouble();
    aCoord.m_Pos.y = parseDouble();

    while( ( token = NextTok() ) != T_RIGHT )
    {
        switch( token )
        {
            case T_ltcorner:
                aCoord.m_Anchor = LT_CORNER;   // left top corner
                break;

            case T_lbcorner:
                aCoord.m_Anchor = LB_CORNER;      // left bottom corner
                break;

            case T_rbcorner:
                aCoord.m_Anchor = RB_CORNER;      // right bottom corner
                break;

            case T_rtcorner:
                aCoord.m_Anchor = RT_CORNER;      // right top corner
                break;

            default:
                Unexpected( CurText() );
                break;
        }
    }
}

int PAGE_LAYOUT_READER_PARSER::parseInt( int aMin, int aMax )
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


double PAGE_LAYOUT_READER_PARSER::parseDouble()
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );

    double val = strtod( CurText(), NULL );

    return val;
}

// defaultPageLayout is the default page layout description
// using the S expr.
// see page_layout_default_shape.cpp
extern const char defaultPageLayout[];

void WORKSHEET_LAYOUT::SetDefaultLayout()
{
    ClearList();
    PAGE_LAYOUT_READER_PARSER lp_parser( defaultPageLayout, wxT( "default page" ) );

    try
    {
        lp_parser.Parse( this );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogMessage( ioe.errorText );
    }
}

/**
 * Populates the list from a S expr description stored in a string
 * @param aPageLayout = the S expr string
 */
void WORKSHEET_LAYOUT::SetPageLayout( const char* aPageLayout, bool Append )
{
    if( ! Append )
        ClearList();

    PAGE_LAYOUT_READER_PARSER lp_parser( aPageLayout, wxT( "Sexpr_string" ) );

    try
    {
        lp_parser.Parse( this );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogMessage( ioe.errorText );
    }
}

#include <wx/file.h>

// SetLayout() try to load the aFullFileName custom layout file,
// if aFullFileName is empty, try the filename defined by the
// environment variable KICAD_WKSFILE (a *.kicad_wks filename).
// if does not exists, loads the default page layout.
void WORKSHEET_LAYOUT::SetPageLayout( const wxString& aFullFileName, bool Append )
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
            {
                wxLogMessage( wxT("Page layout file <%s> not found"),
                              fullFileName.GetData() );
            }
            #endif
            SetDefaultLayout();
            return;
        }
    }

    wxFile wksFile( fullFileName );

    if( ! wksFile.IsOpened() )
    {
        if( !Append )
            SetDefaultLayout();
        return;
    }

    int filelen = wksFile.Length();
    char * buffer = new char[filelen+10];

    if( wksFile.Read( buffer, filelen ) != filelen )
        wxLogMessage( _("The file <%s> was not fully read"),
                      fullFileName.GetData() );
    else
    {
        buffer[filelen]=0;

        if( ! Append )
            ClearList();

        PAGE_LAYOUT_READER_PARSER pl_parser( buffer, fullFileName );

        try
        {
            pl_parser.Parse( this );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogMessage( ioe.errorText );
        }
    }

    delete[] buffer;
}

