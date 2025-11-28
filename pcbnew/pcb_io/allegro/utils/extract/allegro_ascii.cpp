/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "utils/extract/allegro_ascii.h"

#include <string_utils.h>

#include <wx/datetime.h>
#include <wx/log.h>


using namespace ALLEGRO;


static wxString FormatUnits( ALLEGRO::BOARD_UNITS aUnits )
{
    switch( aUnits )
    {
        case ALLEGRO::BOARD_UNITS::IMPERIAL:
            return "mils";
        case ALLEGRO::BOARD_UNITS::METRIC:
            return "mm";
    }

    return "unknown_units";
}


static wxString FormatUnits( ALLEGRO::BOARD_UNITS aUnits, double aValue )
{
    switch( aUnits )
    {
        case ALLEGRO::BOARD_UNITS::IMPERIAL:
            return wxString::Format( "%g mil", aValue );
        case ALLEGRO::BOARD_UNITS::METRIC:
            return wxString::Format( "%g mm", aValue );
    }

    return wxString::Format( "%g unknown_units", aValue );
}


class LINE_FORMATTER
{
public:
    LINE_FORMATTER( char aPrefix, OUTPUTFORMATTER& aFormatter ) :
            m_Formatter( aFormatter )
    {
        m_Formatter.Print( "%c!", aPrefix );
    }

    ~LINE_FORMATTER()
    {
        m_Formatter.Print( "\n" );
    }

    void Add( const wxString& aField )
    {
        m_Formatter.Print( "%s!", TO_UTF8( aField ) );
    }

    void Add( const wxString* aField )
    {
        m_Formatter.Print( "%s!", aField ? TO_UTF8( *aField ) : "" );
    }

    void Add( int aField )
    {
        m_Formatter.Print( "%d!", aField );
    }

    void Add( const std::vector<wxString>& aFields )
    {
        for( const auto& field : aFields )
        {
            Add( field );
        }
    }

    void AddDegrees( double aDegrees )
    {
        m_Formatter.Print( "%.3f!", aDegrees / 1000.0 );
    }

    void AddYesNo( bool aValue )
    {
        m_Formatter.Print( "%s!", aValue ? "YES" : "NO" );
    }

private:
    OUTPUTFORMATTER& m_Formatter;
};


using FP_INST_FIELD_VISITOR = std::function<void ( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )>;


void ASCII_EXTRACTOR::extractSymbolInstances( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    // First build the list of visitors for each symbol.
    std::vector<FP_INST_FIELD_VISITOR> visitors;

    for( const wxString& field : aBlock.Fields )
    {
        if( field == "SYM_TYPE" )
        {
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                aLineFormatter.Add( "PACKAGE" );
            } );
        }
        else if( field == "SYM_NAME" )
        {
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                // The symbol name is the parent footprint definition name
                aLineFormatter.Add( aFp.GetName() );
            } );
        }
        else if( field == "REFDES" )
        {
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                aLineFormatter.Add( aFp.GetRefDes() );
            } );
        }
        // this is a hack - CENTER_X/Y aren't always the same
        else if( field == "SYM_X" || field == "SYM_CENTER_X" )
        {
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                aLineFormatter.Add( aFp.m_X );
            } );
        }
        else if( field == "SYM_Y" || field == "SYM_CENTER_Y" )
        {
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                aLineFormatter.Add( aFp.m_Y );
            } );
        }
        else if( field == "SYM_ROTATE" )
        {
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                aLineFormatter.AddDegrees( aFp.m_Rotation );
            } );
        }
        else if( field == "SYM_MIRROR" )
        {
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                aLineFormatter.AddYesNo( aFp.m_Mirrored );
            } );
        }
        else if( field == "SYM_LIBRARY_PATH" )
        {
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                aLineFormatter.Add( aFp.m_Parent->GetLibPath() );
            } );
        }
        else
        {
            wxLogWarning( "Unsupported SYMBOL extract field: %s", TO_UTF8( field ) );
            visitors.push_back( [&]( const FOOTPRINT_INSTANCE& aFp, LINE_FORMATTER& aLineFormatter )
            {
                aLineFormatter.Add( "??" );
            } );
        }
    }

    // And now visit all the footprint instances, and then visit each field on each one
    m_Brd.VisitFootprintDefs( [&]( const FOOTPRINT_DEF& aFpDef )
    {
        m_Brd.VisitFootprintInstances( aFpDef, [&]( const FOOTPRINT_INSTANCE& aFp )
        {
            // Every symbol instance is a new line
            LINE_FORMATTER lineFormatter( 'S', m_Formatter );

            for( const auto& visitor : visitors )
            {
                visitor( aFp, lineFormatter );
            }
        } );
    } );
}


void ASCII_EXTRACTOR::Extract( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    {
        LINE_FORMATTER lineFormatter( 'A', m_Formatter );
        lineFormatter.Add( aBlock.Fields );
    }

    {
        LINE_FORMATTER lineFormatter( 'S', m_Formatter );
        lineFormatter.Add( wxString( "FILENAME.brd" ) );

        {
            wxString date = wxDateTime::Now().Format( wxT( "%a %b %d %H:%M:%S %Y" ) );
            lineFormatter.Add( date );
        }
        lineFormatter.Add( 0 ); // Placeholder for X origin
        lineFormatter.Add( 0 ); // Placeholder for Y origin
        lineFormatter.Add( 0 ); // Placeholder for board width
        lineFormatter.Add( 0 ); // Placeholder for board height
        lineFormatter.Add( m_Brd.m_Header->m_UnitsDivisor );
        lineFormatter.Add( FormatUnits( m_Brd.m_Header->m_BoardUnits ) );
        lineFormatter.Add( wxString( "SCH_NAME" ) );
        lineFormatter.Add( FormatUnits( m_Brd.m_Header->m_BoardUnits, 42 ) ); // Placeholder for board thickness
        lineFormatter.Add( 2 );                                               // Placeholder for layers count
        lineFormatter.Add( "OUT OF DATE" );                                   // Placeholder for status?
    }

    // Now, for each block type, do the extraction.
    switch( aBlock.Type )
    {
        case EXTRACT_SPEC_PARSER::IR::BLOCK_TYPE::SYMBOL:
            // Remember that Allegro symbols are KiCad footprints.
            extractSymbolInstances( aBlock );
            break;

        default:
            wxLogWarning( "Unsupported extract block type %d", static_cast<int>( aBlock.Type ) );
            break;
    }
}
