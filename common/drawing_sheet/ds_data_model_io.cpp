/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <string_utils.h>
#include <locale_io.h>
#include <macros.h>
#include <math/vector2d.h>
#include <drawing_sheet/ds_painter.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/drawing_sheet_lexer.h>
#include <drawing_sheet/ds_file_versions.h>

#include <wx/msgdlg.h>

using namespace DRAWINGSHEET_T;

#define double2Str Double2Str

// A helper function to write tokens:
static const char* getTokenName( T aTok )
{
    return DRAWING_SHEET_LEXER::TokenName( aTok );
}

// A basic helper class to write a drawing sheet file
// Not used alone, a file writer or a string writer should be derived to use it.
// Therefore the constructor is protected.
class DS_DATA_MODEL_IO
{
public:
    void Format( DS_DATA_MODEL* aSheet ) const;

    void Format( DS_DATA_MODEL* aModel, std::vector<DS_DATA_ITEM*>& aItemsList ) const;
    void Format( DS_DATA_MODEL* aModel, DS_DATA_ITEM* aItem, int aNestLevel ) const;

protected:
    DS_DATA_MODEL_IO() { m_out = NULL; }
    virtual ~DS_DATA_MODEL_IO() {}

private:
    void format( DS_DATA_ITEM_TEXT* aItem, int aNestLevel ) const;
    void format( DS_DATA_MODEL* aModel, DS_DATA_ITEM* aItem, int aNestLevel ) const;
    void format( DS_DATA_ITEM_POLYGONS* aItem, int aNestLevel ) const;
    void format( DS_DATA_ITEM_BITMAP* aItem, int aNestLevel ) const;
    void formatCoordinate( const char* aToken, POINT_COORD& aCoord ) const;
    void formatRepeatParameters( DS_DATA_ITEM* aItem ) const;
    void formatOptions( DS_DATA_ITEM* aItem ) const;

protected:
    OUTPUTFORMATTER* m_out;
};


// A helper class to write a drawing sheet to a file
class DS_DATA_MODEL_FILEIO : public DS_DATA_MODEL_IO
{
public:
    DS_DATA_MODEL_FILEIO( const wxString& aFilename ) :
            DS_DATA_MODEL_IO()
    {
        try
        {
            m_fileout = new FILE_OUTPUTFORMATTER( aFilename );
            m_out = m_fileout;
        }
        catch( const IO_ERROR& ioe )
        {
            wxMessageBox( ioe.What(), _( "Error writing drawing sheet file" ) );
        }
    }

    ~DS_DATA_MODEL_FILEIO()
    {
        delete m_fileout;
    }

private:
    FILE_OUTPUTFORMATTER* m_fileout;
};


// A helper class to write a drawing sheet to a string
class DS_DATA_MODEL_STRINGIO : public DS_DATA_MODEL_IO
{
public:
    DS_DATA_MODEL_STRINGIO( wxString* aOutputString ) :
            DS_DATA_MODEL_IO(),
            m_output( aOutputString )
    {
        try
        {
            m_writer = new STRING_FORMATTER();
            m_out = m_writer;
        }
        catch( const IO_ERROR& ioe )
        {
            wxMessageBox( ioe.What(), _( "Error writing drawing sheet file" ) );
        }
    }

    ~DS_DATA_MODEL_STRINGIO()
    {
        *m_output = FROM_UTF8( m_writer->GetString().c_str() );
        delete m_writer;
    }

private:
    STRING_FORMATTER* m_writer;
    wxString*         m_output;
};


void DS_DATA_MODEL::Save( const wxString& aFullFileName )
{
    DS_DATA_MODEL_FILEIO writer( aFullFileName );
    writer.Format( this );
}


void DS_DATA_MODEL::SaveInString( wxString* aOutputString )
{
    DS_DATA_MODEL_STRINGIO writer( aOutputString );
    writer.Format( this );
}


void DS_DATA_MODEL::SaveInString( std::vector<DS_DATA_ITEM*>& aItemsList, wxString* aOutputString )
{
    DS_DATA_MODEL_STRINGIO writer( aOutputString );
    writer.Format( this, aItemsList );
}


void DS_DATA_MODEL_IO::Format( DS_DATA_MODEL* aModel, std::vector<DS_DATA_ITEM*>& aItemsList ) const
{
    LOCALE_IO   toggle;     // switch on/off the locale "C" notation

    m_out->Print( 0, "(kicad_wks (version %d) (generator pl_editor)\n",
                  SEXPR_WORKSHEET_FILE_VERSION );

    for( DS_DATA_ITEM* item : aItemsList )
        Format( aModel, item, 1 );

    m_out->Print( 0, ")\n" );
}


void DS_DATA_MODEL_IO::Format( DS_DATA_MODEL* aModel, DS_DATA_ITEM* aItem, int aNestLevel ) const
{
    switch( aItem->GetType() )
    {
    case DS_DATA_ITEM::DS_TEXT:
        format( (DS_DATA_ITEM_TEXT*) aItem, aNestLevel );
        break;

    case DS_DATA_ITEM::DS_SEGMENT:
    case DS_DATA_ITEM::DS_RECT:
        format( aModel, aItem, aNestLevel );
        break;

    case DS_DATA_ITEM::DS_POLYPOLYGON:
        format( (DS_DATA_ITEM_POLYGONS*) aItem, aNestLevel );
        break;

    case DS_DATA_ITEM::DS_BITMAP:
        format( (DS_DATA_ITEM_BITMAP*) aItem, aNestLevel );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format item" ) );
    }
}


void DS_DATA_MODEL_IO::Format( DS_DATA_MODEL* aSheet ) const
{
    LOCALE_IO   toggle;     // switch on/off the locale "C" notation

    m_out->Print( 0, "(kicad_wks (version %d) (generator pl_editor)\n",
                  SEXPR_WORKSHEET_FILE_VERSION );

    // Setup
    int nestLevel = 1;
    // Write default values:
    m_out->Print( nestLevel, "(setup " );
    m_out->Print( 0, "(textsize %s %s)",
                  double2Str( aSheet->m_DefaultTextSize.x ).c_str(),
                  double2Str( aSheet->m_DefaultTextSize.y ).c_str() );
    m_out->Print( 0, "(linewidth %s)", double2Str( aSheet->m_DefaultLineWidth ).c_str() );
    m_out->Print( 0, "(textlinewidth %s)", double2Str( aSheet->m_DefaultTextThickness ).c_str() );
    m_out->Print( 0, "\n" );

    // Write margin values
    m_out->Print( nestLevel, "(left_margin %s)", double2Str( aSheet->GetLeftMargin() ).c_str() );
    m_out->Print( 0, "(right_margin %s)", double2Str( aSheet->GetRightMargin() ).c_str() );
    m_out->Print( 0, "(top_margin %s)", double2Str( aSheet->GetTopMargin() ).c_str() );
    m_out->Print( 0, "(bottom_margin %s)", double2Str( aSheet->GetBottomMargin() ).c_str() );
    m_out->Print( 0, ")\n" );

    // Save the graphical items on the drawing sheet
    for( unsigned ii = 0; ii < aSheet->GetCount(); ii++ )
    {
        DS_DATA_ITEM* item = aSheet->GetItem( ii );
        Format( aSheet, item, nestLevel );
    }

    m_out->Print( 0, ")\n" );
}


void DS_DATA_MODEL_IO::format( DS_DATA_ITEM_TEXT* aItem, int aNestLevel ) const
{
    m_out->Print( aNestLevel, "(tbtext" );
    m_out->Print( 0, " %s", m_out->Quotew( aItem->m_TextBase ).c_str() );
    m_out->Print( 0, " (name %s)", m_out->Quotew( aItem->m_Name ).c_str() );

    formatCoordinate( getTokenName( T_pos ), aItem->m_Pos );
    formatOptions( aItem );

    if( aItem->m_Orient )
        m_out->Print( 0, " (rotate %s)", double2Str(aItem->m_Orient ).c_str() );

    // Write font info, only if it is not the default setup
    bool write_size = aItem->m_TextSize.x != 0.0 || aItem->m_TextSize.y != 0.0;
    bool write_thickness = aItem->m_LineWidth != 0.0;

    if( write_thickness || write_size || aItem->m_Bold || aItem->m_Italic )
    {
        m_out->Print( 0, " (font" );

        if( write_thickness )
            m_out->Print( 0, " (linewidth %s)", double2Str( aItem->m_LineWidth ).c_str() );

        if( write_size )
        {
            m_out->Print( 0, " (size %s %s)",
                          double2Str( aItem->m_TextSize.x ).c_str(),
                          double2Str( aItem->m_TextSize.y ).c_str() );
        }

        if( aItem->m_Bold )
            m_out->Print( 0, " bold" );

        if( aItem->m_Italic )
            m_out->Print( 0, " italic" );

        m_out->Print( 0, ")" );
    }

    // Write text justification
    if( aItem->m_Hjustify != GR_TEXT_HJUSTIFY_LEFT || aItem->m_Vjustify != GR_TEXT_VJUSTIFY_CENTER )
    {
        m_out->Print( 0, " (justify" );

        // Write T_center opt first, because it is
        // also a center for both m_Hjustify and m_Vjustify
        if( aItem->m_Hjustify == GR_TEXT_HJUSTIFY_CENTER )
            m_out->Print( 0, " center" );
        else if( aItem->m_Hjustify == GR_TEXT_HJUSTIFY_RIGHT )
            m_out->Print( 0, " right" );

        if( aItem->m_Vjustify == GR_TEXT_VJUSTIFY_TOP )
            m_out->Print( 0, " top" );
        else if( aItem->m_Vjustify == GR_TEXT_VJUSTIFY_BOTTOM )
            m_out->Print( 0, " bottom" );

        m_out->Print( 0, ")" );
    }

    // write constraints
    if( aItem->m_BoundingBoxSize.x )
        m_out->Print( 0, " (maxlen %s)", double2Str(aItem->m_BoundingBoxSize.x ).c_str() );

    if( aItem->m_BoundingBoxSize.y )
        m_out->Print( 0, " (maxheight %s)", double2Str(aItem->m_BoundingBoxSize.y ).c_str() );

    formatRepeatParameters( aItem );

    if( !aItem->m_Info.IsEmpty() )
        m_out->Print( 0, " (comment %s)\n", m_out->Quotew( aItem->m_Info ).c_str() );

    m_out->Print( 0, ")\n" );
}


void DS_DATA_MODEL_IO::format( DS_DATA_MODEL* aModel, DS_DATA_ITEM* aItem, int aNestLevel ) const
{
    if( aItem->GetType() == DS_DATA_ITEM::DS_RECT )
        m_out->Print( aNestLevel, "(rect" );
    else
        m_out->Print( aNestLevel, "(line" );

    m_out->Print( 0, " (name %s)", m_out->Quotew( aItem->m_Name ).c_str() );

    formatCoordinate( getTokenName( T_start ), aItem->m_Pos );
    formatCoordinate( getTokenName( T_end ), aItem->m_End );
    formatOptions( aItem );

    if( aItem->m_LineWidth && aItem->m_LineWidth != aModel->m_DefaultLineWidth )
        m_out->Print( 0, " (linewidth %s)", double2Str( aItem->m_LineWidth ).c_str() );

    formatRepeatParameters( aItem );

    if( !aItem->m_Info.IsEmpty() )
        m_out->Print( 0, " (comment %s)\n", m_out->Quotew( aItem->m_Info ).c_str() );

    m_out->Print( 0, ")\n" );
}


void DS_DATA_MODEL_IO::format( DS_DATA_ITEM_POLYGONS* aItem, int aNestLevel ) const
{
    m_out->Print( aNestLevel, "(polygon" );
    m_out->Print( 0, " (name %s)", m_out->Quotew( aItem->m_Name ).c_str() );
    formatCoordinate( "pos", aItem->m_Pos );
    formatOptions( aItem );

    formatRepeatParameters( aItem );

    if( aItem->m_Orient )
        m_out->Print( 0, " (rotate %s)", double2Str(aItem->m_Orient ).c_str() );

    if( aItem->m_LineWidth )
        m_out->Print( 0, " (linewidth %s)\n", double2Str( aItem->m_LineWidth ).c_str() );

    if( !aItem->m_Info.IsEmpty() )
        m_out->Print( 0, " (comment %s)\n", m_out->Quotew( aItem->m_Info ).c_str() );

    // Write polygon corners list
    for( int kk = 0; kk < aItem->GetPolyCount(); kk++ )
    {
        m_out->Print( aNestLevel+1, "(pts" );
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

            m_out->Print( nestLevel, " (xy %s %s)",
                          double2Str( pos.x ).c_str(),
                          double2Str( pos.y ).c_str() );
        }

        m_out->Print( 0, ")\n" );
    }

    m_out->Print( aNestLevel, ")\n" );
}


void DS_DATA_MODEL_IO::format( DS_DATA_ITEM_BITMAP* aItem, int aNestLevel ) const
{
    m_out->Print( aNestLevel, "(bitmap" );
    m_out->Print( 0, " (name %s)", m_out->Quotew( aItem->m_Name ).c_str() );
    formatCoordinate( "pos", aItem->m_Pos );
    formatOptions( aItem );

    m_out->Print( 0, " (scale %s)", double2Str( aItem->m_ImageBitmap->GetScale() ).c_str() );

    formatRepeatParameters( aItem );
    m_out->Print( 0,"\n");

    if( !aItem->m_Info.IsEmpty() )
        m_out->Print( 0, " (comment %s)\n", m_out->Quotew( aItem->m_Info ).c_str() );

    // Write image in png readable format
    m_out->Print( aNestLevel, "(pngdata\n" );
    wxArrayString pngStrings;
    aItem->m_ImageBitmap->SaveData( pngStrings );

    for( unsigned ii = 0; ii < pngStrings.GetCount(); ii++ )
        m_out->Print( aNestLevel+1, "(data \"%s\")\n", TO_UTF8(pngStrings[ii]) );

    m_out->Print( aNestLevel+1, ")\n" );

    m_out->Print( aNestLevel, ")\n" );
}


void DS_DATA_MODEL_IO::formatCoordinate( const char * aToken, POINT_COORD & aCoord ) const
{
    m_out->Print( 0, " (%s %s %s", aToken,
                  double2Str( aCoord.m_Pos.x ).c_str(),
                  double2Str( aCoord.m_Pos.y ).c_str() );

    switch( aCoord.m_Anchor )
    {
    case RB_CORNER: break;
    case LT_CORNER: m_out->Print( 0, " ltcorner" ); break;
    case LB_CORNER: m_out->Print( 0, " lbcorner" ); break;
    case RT_CORNER: m_out->Print( 0, " rtcorner" ); break;
    }

    m_out->Print( 0, ")" );
}


void DS_DATA_MODEL_IO::formatRepeatParameters( DS_DATA_ITEM* aItem ) const
{
    if( aItem->m_RepeatCount <= 1 )
        return;

    m_out->Print( 0, " (repeat %d)", aItem->m_RepeatCount );

    if( aItem->m_IncrementVector.x )
        m_out->Print( 0, " (incrx %s)", double2Str(aItem-> m_IncrementVector.x ).c_str() );

    if( aItem->m_IncrementVector.y )
        m_out->Print( 0, " (incry %s)", double2Str( aItem->m_IncrementVector.y ).c_str() );

    if( aItem->m_IncrementLabel != 1 && aItem->GetType() == DS_DATA_ITEM::DS_TEXT )
        m_out->Print( 0, " (incrlabel %d)", aItem->m_IncrementLabel );
}


void DS_DATA_MODEL_IO::formatOptions( DS_DATA_ITEM* aItem ) const
{
    if( aItem->GetPage1Option() == FIRST_PAGE_ONLY )
        m_out->Print( 0, " (option page1only)" );
    else if( aItem->GetPage1Option() == SUBSEQUENT_PAGES )
        m_out->Print( 0, " (option notonpage1)" );
}
