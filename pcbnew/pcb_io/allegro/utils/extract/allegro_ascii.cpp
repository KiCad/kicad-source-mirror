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


/**
 * Some funciton that extracts one field from a DB_OBJ.
 */
using FIELD_EXTRACTOR = std::function<wxString( const DB_OBJ& )>;

std::map<wxString, FIELD_EXTRACTOR> g_FPInstFieldExtractors =
{
    {
        "SYM_TYPE", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            return "PACKAGE";
        }
    },
    {
        "REFDES", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            const wxString* refDes = fp.GetRefDes();
            return refDes ? *refDes : "";
        }
    },
    {
        "SYM_NAME", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            const wxString* name = fp.GetName();
            return name ? *name : "";
        }
    },
    {
        "SYM_X", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            return wxString::Format( "%g", fp.m_X );
        }
    },
    {
        "SYM_Y", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            return wxString::Format( "%g", fp.m_Y );
        }
    },
    {
        "SYM_CENTER_X", []( const DB_OBJ& aObj ) -> wxString
        {
            // HACK: this isactually done via TEXT position and falls back to PLACE_BOUND center
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            return wxString::Format( "%g", fp.m_X );
        }
    },
    {
        "SYM_CENTER_Y", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            return wxString::Format( "%g", fp.m_Y );
        }
    },
    {
        "SYM_ROTATE", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            return wxString::Format( "%.3f", fp.m_Rotation / 1000.0 );
        }
    },
    {
        "SYM_MIRROR", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            return fp.m_Mirrored ? "YES" : "NO";
        }
    },
    {
        "SYM_LIBRARY_PATH", []( const DB_OBJ& aObj ) -> wxString
        {
            const FOOTPRINT_INSTANCE& fp = static_cast<const FOOTPRINT_INSTANCE&>( aObj );
            return fp.m_Parent->GetLibPath() ? *fp.m_Parent->GetLibPath() : "";
        }
    }
};


void ASCII_EXTRACTOR::extractSymbolInstances( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    // First build the list of visitors for each symbol.
    std::vector<FIELD_EXTRACTOR> fieldVisitors;

    for( const wxString& field : aBlock.Fields )
    {
        auto it = g_FPInstFieldExtractors.find( field );

        if( it != g_FPInstFieldExtractors.end() )
        {
            const FIELD_EXTRACTOR& extractor = it->second;

            wxLogTrace( "ALLEGRO_EXTRACT", "Adding extractor for field: %s", TO_UTF8( field ) );

            fieldVisitors.push_back( extractor );
        }
        else
        {
            wxLogWarning( "Unsupported SYMBOL extract field: %s", TO_UTF8( field ) );
            fieldVisitors.push_back( [&]( const DB_OBJ& aFp )
            {
                return wxString( "??" );
            } );
        }
    }

    using CONDITION_TESTER = std::function<bool ( const DB_OBJ& aObj )>;

    // ORed conditions, each of which is a set of ANDed conditions.
    std::vector<std::vector<CONDITION_TESTER>> orConditionTesters;

    /*
     * Iterate over ORed conditions, each of which is a set of ANDed conditions,
     * and build the testers.
     */
    for( const auto& andCondition : aBlock.OrConditions)
    {
        std::vector<CONDITION_TESTER> andConditionTesters;

        for( const auto& condition : andCondition )
        {
            auto it = g_FPInstFieldExtractors.find( condition.FieldName );

            if( it != g_FPInstFieldExtractors.end() )
            {
                const FIELD_EXTRACTOR& extractor = it->second;

                wxLogTrace( "ALLEGRO_EXTRACT", "Adding condition tester for field: %s", TO_UTF8( condition.FieldName ) );

                andConditionTesters.push_back( [=]( const DB_OBJ& aFp ) -> bool
                {
                    wxString value = extractor( aFp );

                    if( condition.Value.IsEmpty() )
                    {
                        // And empty field just means "is set" or "is not set"
                        if( condition.Equals )
                        {
                            return value.IsEmpty();
                        }
                        else
                        {
                            return !value.IsEmpty();
                        }
                    }

                    // Otherwise do a straight string comparison
                    if( condition.Equals )
                    {
                        return value == condition.Value;
                    }
                    else
                    {
                        return value != condition.Value;
                    }
                } );
            }
            else
            {
                wxLogWarning( "Unsupported SYMBOL extract condition field: %s", TO_UTF8( condition.FieldName ) );
            }
        }

        orConditionTesters.push_back( andConditionTesters );
    }


    /**
     * For each object, determine if it matches the conditions,
     * then extract the fields if so.
     */
    const auto objectVisitor = [&]( const DB_OBJ& aFp )
    {
        bool include = true;

        // No ORed conditions means always include
        if( !orConditionTesters.empty() )
        {
            include = false;

            for( const auto& andConditionTesters : orConditionTesters )
            {
                bool allAndTrue = true;

                for( const auto& tester : andConditionTesters )
                {
                    // The first ANDed condition that fails resolves this AND group to false
                    if( !tester( aFp ) )
                    {
                        allAndTrue = false;
                        break;
                    }
                }

                // The first ANDed condition that succeeds resolves the whole OR
                // set to 'include'
                if( allAndTrue )
                {
                    include = true;
                    break;
                }
            }
        }

        // If not included, skip entirely
        if( !include )
            return;

        // Every symbol instance is a new line
        // And within that line, each field is extracted in order
        LINE_FORMATTER lineFormatter( 'S', m_Formatter );

        for( const auto& fieldVisitor : fieldVisitors )
        {
            lineFormatter.Add( fieldVisitor( aFp ) );
        }
    };

    // And now visit all the footprint instances, and then visit each field on each one
    m_Brd.VisitFootprintDefs( [&]( const FOOTPRINT_DEF& aFpDef )
    {
        m_Brd.VisitFootprintInstances( aFpDef, [&]( const FOOTPRINT_INSTANCE& aFp )
        {
            objectVisitor( aFp );
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
