/**
 * @file page_layout_writer.cpp
 * @brief write an S expression of description of graphic items and texts
 * to build a title block and page layout
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2019 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <math/vector2d.h>
#include <page_layout/page_layout_reader_lexer.h>
#include <macros.h>
#include <convert_to_biu.h>


using namespace TB_READER_T;

#define double2Str Double2Str

// A helper function to write tokens:
static const char* getTokenName( T aTok )
{
    return PAGE_LAYOUT_READER_LEXER::TokenName( aTok );
}

// A basic helper class to write a page layout description
// Not used alone, a file writer or a string writer should be
// derived to use it
// Therefore the constructor is protected
class WS_DATA_MODEL_IO
{
protected:
    OUTPUTFORMATTER* m_out;

    WS_DATA_MODEL_IO() { m_out = NULL; }
    virtual ~WS_DATA_MODEL_IO() {}

public:
    void Format( WS_DATA_MODEL* aModel ) const;

    void Format( WS_DATA_MODEL* aModel, WS_DATA_ITEM* aItem, int aNestLevel ) const;

private:
    void format( WS_DATA_ITEM_TEXT* aItem, int aNestLevel ) const;
    void format( WS_DATA_MODEL* aModel, WS_DATA_ITEM* aItem, int aNestLevel ) const;
    void format( WS_DATA_ITEM_POLYGONS* aItem, int aNestLevel )
                 const;
    void format( WS_DATA_ITEM_BITMAP* aItem, int aNestLevel ) const;
    void formatCoordinate( const char * aToken, POINT_COORD & aCoord ) const;
    void formatRepeatParameters( WS_DATA_ITEM* aItem ) const;
    void formatOptions( WS_DATA_ITEM* aItem ) const;
};


// A helper class to write a page layout description to a file
class WS_DATA_MODEL_FILEIO: public WS_DATA_MODEL_IO
{
    FILE_OUTPUTFORMATTER * m_fileout;

public:
    WS_DATA_MODEL_FILEIO( const wxString& aFilename ):
        WS_DATA_MODEL_IO()
    {
        try
        {
            m_fileout = new FILE_OUTPUTFORMATTER( aFilename );
            m_out = m_fileout;
        }
        catch( const IO_ERROR& ioe )
        {
            wxMessageBox( ioe.What(), _( "Error writing page layout design file" ) );
        }
    }

    ~WS_DATA_MODEL_FILEIO()
    {
        delete m_fileout;
    }
};


// A helper class to write a page layout description to a string
class WS_DATA_MODEL_STRINGIO: public WS_DATA_MODEL_IO
{
    STRING_FORMATTER * m_writer;
    wxString & m_output;

public:
    WS_DATA_MODEL_STRINGIO( wxString& aOutputString ):
        WS_DATA_MODEL_IO(), m_output( aOutputString )
    {
        try
        {
            m_writer = new STRING_FORMATTER();
            m_out = m_writer;
        }
        catch( const IO_ERROR& ioe )
        {
            wxMessageBox( ioe.What(), _( "Error writing page layout design file" ) );
        }
    }

    ~WS_DATA_MODEL_STRINGIO()
    {
        m_output = FROM_UTF8( m_writer->GetString().c_str() );
        delete m_writer;
    }
};


/*
 * Save the description in a file
 */
void WS_DATA_MODEL::Save( const wxString& aFullFileName )
{
    WS_DATA_MODEL_FILEIO writer( aFullFileName );
    writer.Format( this );
}


/* Save the description in a buffer
 */
void WS_DATA_MODEL::SaveInString( wxString& aOutputString )
{
    WS_DATA_MODEL_STRINGIO writer( aOutputString );
    writer.Format( this );
}


void WS_DATA_MODEL::SaveInString( std::vector<WS_DATA_ITEM*> aItemsList, wxString& aOutputString )
{
    WS_DATA_MODEL_STRINGIO writer( aOutputString );

    LOCALE_IO   toggle;     // switch on/off the locale "C" notation

    for( WS_DATA_ITEM* item : aItemsList )
        writer.Format( this, item, 0 );
}


void WS_DATA_MODEL_IO::Format( WS_DATA_MODEL* aModel, WS_DATA_ITEM* aItem, int aNestLevel ) const
{
    switch( aItem->GetType() )
    {
    case WS_DATA_ITEM::WS_TEXT:
        format( (WS_DATA_ITEM_TEXT*) aItem, aNestLevel );
        break;

    case WS_DATA_ITEM::WS_SEGMENT:
    case WS_DATA_ITEM::WS_RECT:
        format( aModel, aItem, aNestLevel );
        break;

    case WS_DATA_ITEM::WS_POLYPOLYGON:
        format( (WS_DATA_ITEM_POLYGONS*) aItem, aNestLevel );
        break;

    case WS_DATA_ITEM::WS_BITMAP:
        format( (WS_DATA_ITEM_BITMAP*) aItem, aNestLevel );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format item" ) );
    }
}


void WS_DATA_MODEL_IO::Format( WS_DATA_MODEL* aPageLayout ) const
{
    LOCALE_IO   toggle;     // switch on/off the locale "C" notation

    m_out->Print( 0, "(page_layout\n" );

    // Setup
    int nestLevel = 1;
    // Write default values:
    m_out->Print( nestLevel, "(%s ", getTokenName( T_setup ) );
    m_out->Print( 0, "(textsize %s %s)",
                  double2Str( aPageLayout->m_DefaultTextSize.x ).c_str(),
                  double2Str( aPageLayout->m_DefaultTextSize.y ).c_str() );
    m_out->Print( 0, "(linewidth %s)",
                  double2Str( aPageLayout->m_DefaultLineWidth ).c_str() );
    m_out->Print( 0, "(textlinewidth %s)",
                  double2Str( aPageLayout->m_DefaultTextThickness ).c_str() );
    m_out->Print( 0, "\n" );

    // Write margin values
    m_out->Print( nestLevel, "(%s %s)", getTokenName( T_left_margin ),
                  double2Str( aPageLayout->GetLeftMargin() ).c_str() );
    m_out->Print( 0, "(%s %s)", getTokenName( T_right_margin ),
                  double2Str( aPageLayout->GetRightMargin() ).c_str() );
    m_out->Print( 0, "(%s %s)", getTokenName( T_top_margin ),
                  double2Str( aPageLayout->GetTopMargin() ).c_str() );
    m_out->Print( 0, "(%s %s)", getTokenName( T_bottom_margin ),
                  double2Str( aPageLayout->GetBottomMargin() ).c_str() );
    m_out->Print( 0, ")\n" );

    // Save the graphical items on the page layout
    for( unsigned ii = 0; ii < aPageLayout->GetCount(); ii++ )
    {
        WS_DATA_ITEM* item = aPageLayout->GetItem( ii );
        Format( aPageLayout, item, nestLevel );
    }

    m_out->Print( 0, ")\n" );
}


void WS_DATA_MODEL_IO::format( WS_DATA_ITEM_TEXT* aItem, int aNestLevel ) const
{
    m_out->Print( aNestLevel, "(%s", getTokenName( T_tbtext ) );
    m_out->Print( 0, " %s", m_out->Quotew( aItem->m_TextBase ).c_str() );
    m_out->Print( 0, " (%s %s)", getTokenName( T_name ),
                  m_out->Quotew( aItem->m_Name ).c_str() );

    formatCoordinate( getTokenName( T_pos ), aItem->m_Pos );
    formatOptions( aItem );

    if( aItem->m_Orient )
        m_out->Print( 0, " (%s %s)", getTokenName( T_rotate ),
                      double2Str(aItem->m_Orient ).c_str() );

    // Write font info, only if it is not the default setup
    bool write_size = aItem->m_TextSize.x != 0.0 || aItem->m_TextSize.y != 0.0;
    bool write_thickness = aItem->m_LineWidth != 0.0;

    if( write_thickness || write_size || aItem->m_Bold || aItem->m_Italic )
    {
        m_out->Print( 0, " (%s", getTokenName( T_font ) );

        if( write_thickness )
        {
            m_out->Print( 0, " (%s %s)", getTokenName( T_linewidth ),
                          double2Str(aItem->m_LineWidth ).c_str() );
        }

        if( write_size )
        {
            m_out->Print( 0, " (%s %s %s)", getTokenName( T_size ),
                          double2Str(aItem->m_TextSize.x ).c_str(),
                          double2Str(aItem->m_TextSize.y ).c_str() );
        }
        if( aItem->m_Bold )
            m_out->Print( 0, " %s", getTokenName( T_bold ) );

        if( aItem->m_Italic )
            m_out->Print( 0, " %s", getTokenName( T_italic ) );

        m_out->Print( 0, ")" );
    }

    // Write text justification
    if( aItem->m_Hjustify != GR_TEXT_HJUSTIFY_LEFT ||
        aItem->m_Vjustify != GR_TEXT_VJUSTIFY_CENTER )
    {
        m_out->Print( 0, " (%s", getTokenName( T_justify ) );

        // Write T_center opt first, because it is
        // also a center for both m_Hjustify and m_Vjustify
        if( aItem->m_Hjustify == GR_TEXT_HJUSTIFY_CENTER )
            m_out->Print( 0, " %s", getTokenName( T_center ) );

        if( aItem->m_Hjustify == GR_TEXT_HJUSTIFY_RIGHT )
            m_out->Print( 0, " %s", getTokenName( T_right ) );

        if( aItem->m_Vjustify == GR_TEXT_VJUSTIFY_TOP )
            m_out->Print( 0, " %s", getTokenName( T_top ) );

        if( aItem->m_Vjustify == GR_TEXT_VJUSTIFY_BOTTOM )
            m_out->Print( 0, " %s", getTokenName( T_bottom ) );

        m_out->Print( 0, ")" );
    }

    // write constraints
    if( aItem->m_BoundingBoxSize.x )
        m_out->Print( 0, " (%s %s)", getTokenName( T_maxlen ),
                      double2Str(aItem->m_BoundingBoxSize.x ).c_str() );

    if( aItem->m_BoundingBoxSize.y )
        m_out->Print( 0, " (%s %s)", getTokenName( T_maxheight ),
                      double2Str(aItem->m_BoundingBoxSize.y ).c_str() );

    formatRepeatParameters( aItem );

    m_out->Print( 0, ")\n" );
}

void WS_DATA_MODEL_IO::format( WS_DATA_MODEL* aModel, WS_DATA_ITEM* aItem, int aNestLevel ) const
{
    if( aItem->GetType() == WS_DATA_ITEM::WS_RECT )
        m_out->Print( aNestLevel, "(%s", getTokenName( T_rect ) );
    else
        m_out->Print( aNestLevel, "(%s", getTokenName( T_line ) );

    m_out->Print( 0, " (%s %s)", getTokenName( T_name ),
                  m_out->Quotew( aItem->m_Name ).c_str() );

    formatCoordinate( getTokenName( T_start ), aItem->m_Pos );
    formatCoordinate( getTokenName( T_end ), aItem->m_End );
    formatOptions( aItem );

    if( aItem->m_LineWidth && aItem->m_LineWidth != aModel->m_DefaultLineWidth )
        m_out->Print( 0, " (linewidth %s)", double2Str( aItem->m_LineWidth ).c_str() );

    formatRepeatParameters( aItem );

    m_out->Print( 0, ")\n" );
}


void WS_DATA_MODEL_IO::format( WS_DATA_ITEM_POLYGONS* aItem, int aNestLevel ) const
{
    m_out->Print( aNestLevel, "(%s", getTokenName( T_polygon ) );
    m_out->Print( 0, " (%s %s)", getTokenName( T_name ),
                  m_out->Quotew( aItem->m_Name ).c_str() );
    formatCoordinate( getTokenName( T_pos ), aItem->m_Pos );
    formatOptions( aItem );

    formatRepeatParameters( aItem );

    if( aItem->m_Orient )
        m_out->Print( 0, " (%s %s)", getTokenName( T_rotate ),
                      double2Str(aItem->m_Orient ).c_str() );

    if( aItem->m_LineWidth )
        m_out->Print( 0, " (linewidth %s)\n", double2Str( aItem->m_LineWidth ).c_str() );

    // Write polygon corners list
    for( int kk = 0; kk < aItem->GetPolyCount(); kk++ )
    {
        m_out->Print( aNestLevel+1, "(%s", getTokenName( T_pts ) );
        // Create current polygon corners list
        unsigned ist = aItem->GetPolyIndexStart( kk );
        unsigned iend = aItem->GetPolyIndexEnd( kk );
        int ii = 0;

        while( ist <= iend )
        {
            DPOINT pos = aItem->m_Corners[ist++];
            int nestLevel = 0;

            if( ii++ > 4)
            {
                m_out->Print( 0, "\n" );
                nestLevel = aNestLevel+2;
                ii = 0;
            }

            m_out->Print( nestLevel, " (%s %s %s)", getTokenName( T_xy ),
                          double2Str( pos.x ).c_str(),
                          double2Str( pos.y ).c_str() );
        }

        m_out->Print( 0, ")\n" );
    }

    m_out->Print( aNestLevel, ")\n" );
}


void WS_DATA_MODEL_IO::format( WS_DATA_ITEM_BITMAP* aItem, int aNestLevel ) const
{
    m_out->Print( aNestLevel, "(%s", getTokenName( T_bitmap ) );
    m_out->Print( 0, " (%s %s)", getTokenName( T_name ),
                  m_out->Quotew( aItem->m_Name ).c_str() );
    formatCoordinate( getTokenName( T_pos ), aItem->m_Pos );
    formatOptions( aItem );

    m_out->Print( 0, " (%s %s)", getTokenName( T_scale ),
                  double2Str( aItem->m_ImageBitmap->GetScale() ).c_str() );

    formatRepeatParameters( aItem );
    m_out->Print( 0,"\n");

    // Write image in png readable format
    m_out->Print( aNestLevel, "(%s\n", getTokenName( T_pngdata ) );
    wxArrayString pngStrings;
    aItem->m_ImageBitmap->SaveData( pngStrings );

    for( unsigned ii = 0; ii < pngStrings.GetCount(); ii++ )
        m_out->Print( aNestLevel+1, "(data \"%s\")\n", TO_UTF8(pngStrings[ii]) );

    m_out->Print( aNestLevel+1, ")\n" );

    m_out->Print( aNestLevel, ")\n" );
}


void WS_DATA_MODEL_IO::formatCoordinate( const char * aToken,
                                            POINT_COORD & aCoord ) const
{
    m_out->Print( 0, " (%s %s %s", aToken,
                  double2Str( aCoord.m_Pos.x ).c_str(),
                  double2Str( aCoord.m_Pos.y ).c_str() );

    switch( aCoord.m_Anchor )
    {
    case RB_CORNER: break;
    case LT_CORNER: m_out->Print( 0, " %s", getTokenName( T_ltcorner ) ); break;
    case LB_CORNER: m_out->Print( 0, " %s", getTokenName( T_lbcorner ) ); break;
    case RT_CORNER: m_out->Print( 0, " %s", getTokenName( T_rtcorner ) ); break;
    }

    m_out->Print( 0, ")" );
}


void WS_DATA_MODEL_IO::formatRepeatParameters( WS_DATA_ITEM* aItem ) const
{
    if( aItem->m_RepeatCount <= 1 )
        return;

    m_out->Print( 0, " (repeat %d)", aItem->m_RepeatCount );

    if( aItem->m_IncrementVector.x )
        m_out->Print( 0, " (incrx %s)", double2Str(aItem-> m_IncrementVector.x ).c_str() );

    if( aItem->m_IncrementVector.y )
        m_out->Print( 0, " (incry %s)", double2Str( aItem->m_IncrementVector.y ).c_str() );

    if( aItem->m_IncrementLabel != 1 && aItem->GetType() == WS_DATA_ITEM::WS_TEXT )
        m_out->Print( 0, " (incrlabel %d)", aItem->m_IncrementLabel );
}


void WS_DATA_MODEL_IO::formatOptions( WS_DATA_ITEM* aItem ) const
{
    if( aItem->GetPage1Option() == FIRST_PAGE_ONLY )
        m_out->Print( 0, " (%s %s)", getTokenName( T_option ), getTokenName(T_page1only ) );
    else if( aItem->GetPage1Option() == SUBSEQUENT_PAGES )
        m_out->Print( 0, " (%s %s)", getTokenName( T_option ), getTokenName( T_notonpage1 ) );
}
