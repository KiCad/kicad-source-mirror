/**
 * @file common/page_layout/page_layout_reader.cpp
 * @brief read an S expression of description of graphic items and texts
 * to build a title block and page layout
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <ws_painter.h>
#include <ws_draw_item.h>
#include <ws_data_model.h>
#include <page_layout/page_layout_reader_lexer.h>

#include <wx/file.h>
#include <wx/mstream.h>


using namespace TB_READER_T;

/**
 * Class PAGE_LAYOUT_READER_PARSER
 * holds data and functions pertinent to parsing a S-expression file
 * for a WS_DATA_MODEL.
 */
class PAGE_LAYOUT_READER_PARSER : public PAGE_LAYOUT_READER_LEXER
{
public:
    PAGE_LAYOUT_READER_PARSER( const char* aLine, const wxString& aSource );
    void Parse( WS_DATA_MODEL* aLayout );

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

    void parseSetup( WS_DATA_MODEL* aLayout );

    /**
     * parse a graphic item starting by "(line" or "(rect" and read parameters.
     */
    void parseGraphic( WS_DATA_ITEM * aItem );

    /**
     * parse a text item starting by "(tbtext" and read parameters.
     */
    void parseText( WS_DATA_ITEM_TEXT * aItem );

    /**
     * parse a polygon item starting by "( polygon" and read parameters.
     * the list of corners included in this description is read by parsePolyOutline
     */
    void parsePolygon( WS_DATA_ITEM_POLYGONS * aItem );

    /**
     * parse a list of corners starting by "( pts" and read coordinates.
     */
    void parsePolyOutline( WS_DATA_ITEM_POLYGONS * aItem );


    /**
     * parse a bitmap item starting by "( bitmap" and read parameters.
     */
    void parseBitmap( WS_DATA_ITEM_BITMAP * aItem );

    void parseCoordinate( POINT_COORD& aCoord);
    void readOption( WS_DATA_ITEM * aItem );
    void readPngdata( WS_DATA_ITEM_BITMAP * aItem );
};

// PCB_PLOT_PARAMS_PARSER

PAGE_LAYOUT_READER_PARSER::PAGE_LAYOUT_READER_PARSER( const char* aLine, const wxString& aSource ) :
    PAGE_LAYOUT_READER_LEXER( aLine, aSource )
{
}


void PAGE_LAYOUT_READER_PARSER::Parse( WS_DATA_MODEL* aLayout )
{
    WS_DATA_ITEM* item;
    LOCALE_IO     toggle;

    for( T token = NextTok(); token != T_RIGHT && token != EOF; token = NextTok() )
    {
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
            item = new WS_DATA_ITEM( WS_DATA_ITEM::WS_SEGMENT );
            parseGraphic( item );
            aLayout->Append( item );
            break;

        case T_rect:
            item = new WS_DATA_ITEM( WS_DATA_ITEM::WS_RECT );
            parseGraphic( item );
            aLayout->Append( item );
            break;

        case T_polygon:
            item = new WS_DATA_ITEM_POLYGONS();
            parsePolygon(  (WS_DATA_ITEM_POLYGONS*) item );
            aLayout->Append( item );
            break;

        case T_bitmap:
            item = new WS_DATA_ITEM_BITMAP( NULL );
            parseBitmap( (WS_DATA_ITEM_BITMAP*) item );
            aLayout->Append( item );
            break;

        case T_tbtext:
            NeedSYMBOLorNUMBER();
            item = new WS_DATA_ITEM_TEXT( FromUTF8() );
            parseText( (WS_DATA_ITEM_TEXT*) item );
            aLayout->Append( item );
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }
}

void PAGE_LAYOUT_READER_PARSER::parseSetup( WS_DATA_MODEL* aLayout )
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
}

void PAGE_LAYOUT_READER_PARSER::parsePolygon( WS_DATA_ITEM_POLYGONS * aItem )
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

void PAGE_LAYOUT_READER_PARSER::parsePolyOutline( WS_DATA_ITEM_POLYGONS * aItem )
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


void PAGE_LAYOUT_READER_PARSER::parseBitmap( WS_DATA_ITEM_BITMAP * aItem )
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

void PAGE_LAYOUT_READER_PARSER::readPngdata( WS_DATA_ITEM_BITMAP * aItem )
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


void PAGE_LAYOUT_READER_PARSER::readOption( WS_DATA_ITEM * aItem )
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


void PAGE_LAYOUT_READER_PARSER::parseGraphic( WS_DATA_ITEM * aItem )
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


void PAGE_LAYOUT_READER_PARSER::parseText( WS_DATA_ITEM_TEXT* aItem )
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
void PAGE_LAYOUT_READER_PARSER::parseCoordinate( POINT_COORD& aCoord)
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

void WS_DATA_MODEL::SetDefaultLayout()
{
    SetPageLayout( defaultPageLayout, false, wxT( "default page" ) );
}

// Returns defaultPageLayout as a string;
wxString WS_DATA_MODEL::DefaultLayout()
{
    return wxString( defaultPageLayout );
}

// emptyPageLayout is a "empty" page layout description
// there is a 0 length line to fool something somewhere.
// using the S expr.
// see page_layout_empty_description.cpp
extern const char emptyPageLayout[];

void WS_DATA_MODEL::SetEmptyLayout()
{
    SetPageLayout( emptyPageLayout, false, wxT( "empty page" ) );
}


wxString WS_DATA_MODEL::EmptyLayout()
{
    return wxString( emptyPageLayout );
}


void WS_DATA_MODEL::SetPageLayout( const char* aPageLayout, bool Append, const wxString& aSource )
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
        wxLogMessage( ioe.What() );
    }
}


void WS_DATA_MODEL::SetPageLayout( const wxString& aFullFileName, bool Append )
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
                wxLogMessage( wxT( "Page layout file <%s> not found" ), fullFileName.GetData() );
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
        wxLogMessage( _("The file \"%s\" was not fully read"), fullFileName.GetData() );
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
            wxLogMessage( ioe.What() );
        }
    }

    delete[] buffer;
}
