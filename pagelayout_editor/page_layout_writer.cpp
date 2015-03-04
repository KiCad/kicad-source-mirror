/**
 * @file page_layout_writer.cpp
 * @brief write an S expression of description of graphic items and texts
 * to build a title block and page layout
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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
#include <worksheet.h>
#include <worksheet_shape_builder.h>
#include <class_worksheet_dataitem.h>
#include <math/vector2d.h>
#include <page_layout_reader_lexer.h>
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
class WORKSHEET_LAYOUT_IO
{
protected:
    OUTPUTFORMATTER* m_out;

    WORKSHEET_LAYOUT_IO() { m_out = NULL; }
    virtual ~WORKSHEET_LAYOUT_IO() {}

public:
    void Format( WORKSHEET_LAYOUT* aPageLayout ) const
        throw( IO_ERROR );

    void Format( WORKSHEET_DATAITEM* aItem, int aNestLevel ) const
        throw( IO_ERROR );

private:
    void format( WORKSHEET_LAYOUT* aPageLayout ) const
        throw( IO_ERROR );

    void format( WORKSHEET_DATAITEM_TEXT* aItem, int aNestLevel ) const throw( IO_ERROR );
    void format( WORKSHEET_DATAITEM* aItem, int aNestLevel ) const throw( IO_ERROR );
    void format( WORKSHEET_DATAITEM_POLYPOLYGON* aItem, int aNestLevel )
                 const throw( IO_ERROR );
    void format( WORKSHEET_DATAITEM_BITMAP* aItem, int aNestLevel ) const
                 throw( IO_ERROR );
    void formatCoordinate( const char * aToken, POINT_COORD & aCoord ) const
                           throw( IO_ERROR );
    void formatRepeatParameters( WORKSHEET_DATAITEM* aItem ) const throw( IO_ERROR );
    void formatOptions( WORKSHEET_DATAITEM* aItem ) const throw( IO_ERROR );
};

// A helper class to write a page layout description to a file
class WORKSHEET_LAYOUT_FILEIO: public WORKSHEET_LAYOUT_IO
{
    FILE_OUTPUTFORMATTER * m_fileout;

public:
    WORKSHEET_LAYOUT_FILEIO( const wxString& aFilename ):
        WORKSHEET_LAYOUT_IO()
    {
        try
        {
            m_fileout = new FILE_OUTPUTFORMATTER( aFilename );
            m_out = m_fileout;
        }
        catch( const IO_ERROR& ioe )
        {
            wxMessageBox( ioe.errorText, _("Error writing page layout descr file" ) );
        }
    }

    ~WORKSHEET_LAYOUT_FILEIO()
    {
        delete m_fileout;
    }
};

// A helper class to write a page layout description to a string
class WORKSHEET_LAYOUT_STRINGIO: public WORKSHEET_LAYOUT_IO
{
    STRING_FORMATTER * m_writer;
    wxString & m_output;

public:
    WORKSHEET_LAYOUT_STRINGIO( wxString& aOutputString ):
        WORKSHEET_LAYOUT_IO(), m_output( aOutputString )
    {
        try
        {
            m_writer = new STRING_FORMATTER();
            m_out = m_writer;
        }
        catch( const IO_ERROR& ioe )
        {
            wxMessageBox( ioe.errorText, _("Error writing page layout descr file" ) );
        }
    }

    ~WORKSHEET_LAYOUT_STRINGIO()
    {
        m_output = FROM_UTF8( m_writer->GetString().c_str() );
        delete m_writer;
    }
};

/*
 * Save the description in a file
 */
void WORKSHEET_LAYOUT::Save( const wxString& aFullFileName )
{
    WORKSHEET_LAYOUT_FILEIO writer( aFullFileName );
    writer.Format( this );
}

/* Save the description in a buffer
 */
void WORKSHEET_LAYOUT::SaveInString( wxString& aOutputString )
{
    WORKSHEET_LAYOUT_STRINGIO writer( aOutputString );
    writer.Format( this );
}


void WORKSHEET_LAYOUT_IO::Format( WORKSHEET_DATAITEM* aItem, int aNestLevel ) const
    throw( IO_ERROR )
{
    switch( aItem->GetType() )
    {
    case WORKSHEET_DATAITEM::WS_TEXT:
        format( (WORKSHEET_DATAITEM_TEXT*) aItem, aNestLevel );
        break;

    case WORKSHEET_DATAITEM::WS_SEGMENT:
    case WORKSHEET_DATAITEM::WS_RECT:
        format( aItem, aNestLevel );
        break;

    case WORKSHEET_DATAITEM::WS_POLYPOLYGON:
        format( (WORKSHEET_DATAITEM_POLYPOLYGON*) aItem, aNestLevel );
        break;

    case WORKSHEET_DATAITEM::WS_BITMAP:
        format( (WORKSHEET_DATAITEM_BITMAP*) aItem, aNestLevel );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format item" ) );
    }
}

void WORKSHEET_LAYOUT_IO::Format( WORKSHEET_LAYOUT* aPageLayout ) const
    throw( IO_ERROR )
{
    LOCALE_IO   toggle;     // switch on/off the locale "C" notation

    m_out->Print( 0, "( page_layout\n" );

    // Setup
    int nestLevel = 1;
    // Write default values:
    m_out->Print( nestLevel, "(%s", getTokenName( T_setup ) );
    m_out->Print( 0, "(textsize %s %s)",
                  double2Str( WORKSHEET_DATAITEM::m_DefaultTextSize.x ).c_str(),
                  double2Str( WORKSHEET_DATAITEM::m_DefaultTextSize.y ).c_str() );
    m_out->Print( 0, "(linewidth %s)", double2Str( WORKSHEET_DATAITEM::m_DefaultLineWidth ).c_str() );
    m_out->Print( 0, "(textlinewidth %s)", double2Str( WORKSHEET_DATAITEM::m_DefaultTextThickness ).c_str() );
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
        WORKSHEET_DATAITEM* item = aPageLayout->GetItem( ii );
        Format( item, nestLevel );
    }

    m_out->Print( 0, ")\n" );
}

void WORKSHEET_LAYOUT_IO::format( WORKSHEET_DATAITEM_TEXT* aItem, int aNestLevel ) const
    throw( IO_ERROR )
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

    // Write font info
    bool write_size = aItem->m_TextSize.x != 0.0 && aItem->m_TextSize.y != 0.0;
    if( write_size || aItem->IsBold() || aItem->IsItalic() )
    {
        m_out->Print( 0, " (%s", getTokenName( T_font ) );

        if( write_size )
        {
            m_out->Print( 0, " (%s %s %s)", getTokenName( T_size ),
                          double2Str(aItem->m_TextSize.x ).c_str(),
                          double2Str(aItem->m_TextSize.y ).c_str() );
        }
        if( aItem->IsBold() )
            m_out->Print( 0, " %s", getTokenName( T_bold ) );

        if( aItem->IsItalic() )
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

void WORKSHEET_LAYOUT_IO::format( WORKSHEET_DATAITEM* aItem, int aNestLevel ) const
    throw( IO_ERROR )
{
    if( aItem->GetType() == WORKSHEET_DATAITEM::WS_RECT )
        m_out->Print( aNestLevel, "(%s", getTokenName( T_rect ) );
    else
        m_out->Print( aNestLevel, "(%s", getTokenName( T_line ) );

    m_out->Print( 0, " (%s %s)", getTokenName( T_name ),
                  m_out->Quotew( aItem->m_Name ).c_str() );

    formatCoordinate( getTokenName( T_start ), aItem->m_Pos );
    formatCoordinate( getTokenName( T_end ), aItem->m_End );
    formatOptions( aItem );

    if( aItem->m_LineWidth && aItem->m_LineWidth != aItem->m_DefaultLineWidth )
        m_out->Print( 0, " (linewidth %s)", double2Str( aItem->m_LineWidth ).c_str() );

    formatRepeatParameters( aItem );

    m_out->Print( 0, ")\n" );
}


void WORKSHEET_LAYOUT_IO::format( WORKSHEET_DATAITEM_POLYPOLYGON* aItem, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "( %s", getTokenName( T_polygon ) );
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
        m_out->Print( aNestLevel+1, "( %s", getTokenName( T_pts ) );
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

void WORKSHEET_LAYOUT_IO::format( WORKSHEET_DATAITEM_BITMAP* aItem, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "( %s", getTokenName( T_bitmap ) );
    m_out->Print( 0, " (%s %s)", getTokenName( T_name ),
                  m_out->Quotew( aItem->m_Name ).c_str() );
    formatCoordinate( getTokenName( T_pos ), aItem->m_Pos );
    formatOptions( aItem );

    m_out->Print( 0, " (%s %s)", getTokenName( T_scale ),
                  double2Str(aItem->m_ImageBitmap->m_Scale ).c_str() );

    formatRepeatParameters( aItem );
    m_out->Print( 0,"\n");

    // Write image in png readable format
    m_out->Print( aNestLevel, "( %s\n", getTokenName( T_pngdata ) );
    wxArrayString pngStrings;
    aItem->m_ImageBitmap->SaveData( pngStrings );

    for( unsigned ii = 0; ii < pngStrings.GetCount(); ii++ )
        m_out->Print( aNestLevel+1, "(data \"%s\")\n", TO_UTF8(pngStrings[ii]) );

    m_out->Print( aNestLevel+1, ")\n" );

    m_out->Print( aNestLevel, ")\n" );
}

void WORKSHEET_LAYOUT_IO::formatCoordinate( const char * aToken,
                                            POINT_COORD & aCoord ) const
                       throw( IO_ERROR )
{
    m_out->Print( 0, " (%s %s %s", aToken,
                  double2Str( aCoord.m_Pos.x ).c_str(),
                  double2Str( aCoord.m_Pos.y ).c_str() );

    switch( aCoord.m_Anchor )
    {
        case RB_CORNER:
            break;

        case LT_CORNER:
            m_out->Print( 0, " %s", getTokenName(T_ltcorner ) );
            break;

        case LB_CORNER:
            m_out->Print( 0, " %s", getTokenName(T_lbcorner ) );
            break;

        case RT_CORNER:
            m_out->Print( 0, " %s", getTokenName(T_rtcorner ) );
            break;
    }

    m_out->Print( 0, ")" );
}

void WORKSHEET_LAYOUT_IO::formatRepeatParameters( WORKSHEET_DATAITEM* aItem ) const
                       throw( IO_ERROR )
{
    if( aItem->m_RepeatCount <= 1 )
        return;

    m_out->Print( 0, " (repeat %d)", aItem->m_RepeatCount );

    if( aItem->m_IncrementVector.x )
        m_out->Print( 0, " (incrx %s)", double2Str(aItem-> m_IncrementVector.x ).c_str() );

    if( aItem->m_IncrementVector.y )
        m_out->Print( 0, " (incry %s)", double2Str( aItem->m_IncrementVector.y ).c_str() );

    if( aItem->m_IncrementLabel != 1 &&
        aItem->GetType() == WORKSHEET_DATAITEM::WS_TEXT )
        m_out->Print( 0, " (incrlabel %d)", aItem->m_IncrementLabel );
}

void WORKSHEET_LAYOUT_IO::formatOptions( WORKSHEET_DATAITEM* aItem ) const
                       throw( IO_ERROR )
{
    switch( aItem->GetPage1Option() )
    {
        default:
        case 0:
            break;

        case 1:
            m_out->Print( 0, " (%s %s)", getTokenName(T_option ),
                          getTokenName(T_page1only ) );
            break;

        case -1:
            m_out->Print( 0, " (%s %s)", getTokenName(T_option ),
                          getTokenName(T_notonpage1 ) );
            break;
    }
}
