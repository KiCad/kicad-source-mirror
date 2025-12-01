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

std::unordered_map<wxString, FIELD_EXTRACTOR> g_FPInstFieldExtractors =
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


/*
    NET_NAME
    REFDES
    PIN_NUMBER
    PIN_NAME
    PIN_GROUND
    PIN_POWER*/
std::unordered_map<wxString, FIELD_EXTRACTOR> g_ComponentPinFieldExtractors =
{
    {
        "NET_NAME", []( const DB_OBJ& aObj ) -> wxString
        {
            return "??";
        }
    },
    {
        "REFDES", []( const DB_OBJ& aObj ) -> wxString
        {
            return "??";
        }
    },
    {
        "PIN_NUMBER", []( const DB_OBJ& aObj ) -> wxString
        {
            return "??";
        }
    },
    {
        "PIN_NAME", []( const DB_OBJ& aObj ) -> wxString
        {
            return "??";
        }
    },
    {
        "PIN_GROUND", []( const DB_OBJ& aObj ) -> wxString
        {
            // Not sure what this is is, All_Preamp and BB-AI have blank values here
            return "";
        }
    },
    {
        "PIN_POWER", []( const DB_OBJ& aObj ) -> wxString
        {
            // As for PIN_GROUND
            return "";
        }
    }
};


// using FUNCTION_EXTRACTOR = std::function<wxString( const FUNCTION_)


std::unordered_map<wxString, FIELD_EXTRACTOR> g_FunctionFieldExtractors = {
    {
        "FUNC_DES", []( const DB_OBJ& aObj ) -> wxString
        {
            const FUNCTION_INSTANCE& func = static_cast<const FUNCTION_INSTANCE&>( aObj );
            const wxString* name = func.GetName();
            return name ? *name : "";
        },
    },
    {    "REFDES", []( const DB_OBJ& aObj ) -> wxString
        {
            const FUNCTION_INSTANCE& func = static_cast<const FUNCTION_INSTANCE&>( aObj );
            const REFDES& refDes = func.GetRefDes();
            const wxString* refDesStr = refDes.GetRefDesStr();
            return refDesStr ? *refDesStr : "";
        },
    },
    {
        "COMP_DEVICE_TYPE", []( const DB_OBJ& aObj ) -> wxString
        {
            const FUNCTION_INSTANCE& func = static_cast<const FUNCTION_INSTANCE&>( aObj );
            const REFDES& refDes = func.GetRefDes();
            const x06_OBJECT* component = refDes.GetParentComponent();
            if( component )
            {
                const wxString* deviceType = component->GetComponentDeviceType();
                return deviceType ? *deviceType : "";
            }
            return "";
        },
    },
    {
        "FUNC_SLOT_NAME", []( const DB_OBJ& aObj ) -> wxString
        {
            const FUNCTION_INSTANCE& func = static_cast<const FUNCTION_INSTANCE&>( aObj );
            const FUNCTION_SLOT& slot = func.GetFunctionSlot();
            const wxString* name = slot.GetName();
            return name ? *name : "";
        },
    }
};


FIELD_EXTRACTOR GetFieldExtractor( const wxString& aFieldName, EXTRACT_SPEC_PARSER::IR::VIEW_TYPE aViewType )
{
    switch( aViewType )
    {
        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::SYMBOL:
        {
            auto it = g_FPInstFieldExtractors.find( aFieldName );

            if( it != g_FPInstFieldExtractors.end() )
            {
                return it->second;
            }
            break;
        }
        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::COMPONENT_PIN:
        {
            auto it = g_ComponentPinFieldExtractors.find( aFieldName );

            if( it != g_ComponentPinFieldExtractors.end() )
            {
                return it->second;
            }
            break;
        }
        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::FUNCTION:
        {
            auto it = g_FunctionFieldExtractors.find( aFieldName );

            if( it != g_FunctionFieldExtractors.end() )
            {
                return it->second;
            }
            break;
        }
    }

    return nullptr;
}


ASCII_EXTRACTOR::OBJECT_VISITOR ASCII_EXTRACTOR::createObjectVisitor( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    // First build the list of visitors for each symbol.
    std::vector<FIELD_EXTRACTOR> fieldVisitors;

    for( const wxString& field : aBlock.Fields )
    {
        FIELD_EXTRACTOR fieldVisitor = GetFieldExtractor( field, aBlock.ViewType );

        if( !fieldVisitor )
        {
            wxLogWarning( "Unsupported extract field: %s for view type %d", TO_UTF8( field ), static_cast<int>( aBlock.ViewType ) );
            fieldVisitor = [&]( const DB_OBJ& aFp )
            {
                return wxString( "??" );
            };
        }

        fieldVisitors.push_back( fieldVisitor );
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
            FIELD_EXTRACTOR extractor = GetFieldExtractor( condition.FieldName, aBlock.ViewType );

            if( extractor )
            {
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
    const auto objectVisitor = [orConditionTesters, fieldVisitors]( const DB_OBJ&          aFp,
                                                                    std::vector<wxString>& aFieldValues ) -> bool
    {
        bool include = true;

        // No ORed conditions means always include
        if( !orConditionTesters.empty() )
        {
            include = false;

            wxLogTrace( "ALLEGRO_EXTRACT", "Testing %zu ORed conditions", orConditionTesters.size() );

            for( const auto& andConditionTesters : orConditionTesters )
            {
                bool allAndTrue = true;

                wxLogTrace( "ALLEGRO_EXTRACT", "Testing %zu ANDed conditions", andConditionTesters.size() );

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
            return false;

        // Every symbol instance is a new line
        // And within that line, each field is extracted in order
        aFieldValues.clear();

        for( const auto& fieldVisitor : fieldVisitors )
        {
            aFieldValues.emplace_back( fieldVisitor( aFp ) );
        }

        return true;
    };

    return objectVisitor;
}


void ASCII_EXTRACTOR::visitForLine( const DB_OBJ& aObj, char aPrefix, OBJECT_VISITOR& aVisitor )
{
    LINE_FORMATTER lineFormatter( aPrefix, m_Formatter );
    std::vector<wxString> fieldValues;

    // The visitor returns true if the object matched the conditions
    if( aVisitor( aObj, fieldValues ) )
    {
        lineFormatter.Add( fieldValues );
    }
}


void ASCII_EXTRACTOR::extractBoardInfo( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock, char aLinePrefix )
{
    // Board info is special because it's not a DB_OBJ-derived object,
    // but we can still use the same visitor mechanism.

    std::vector<wxString> fieldValues;

    for( const auto& field : aBlock.Fields )
    {
        if( field == "BOARD_NAME" )
        {
            fieldValues.emplace_back( "BRD_NAME" );
                // m_Brd.m_Header->GetBoardName() ? *m_Brd.m_Header->GetBoardName() : "" );
        }
        else if( field == "__EXTRACTION_TIME" )
        {
            fieldValues.emplace_back( wxDateTime::Now().FormatISOCombined( ' ' ) );
        }
        else if( field == "BOARD_EXTENT_XMIN" )
        {
            fieldValues.emplace_back( wxString::Format( "%d", 0 ) );
        }
        else if( field == "BOARD_EXTENT_YMIN" )
        {
            fieldValues.emplace_back( wxString::Format( "%d", 0 ) );
        }
        else if( field == "BOARD_EXTENT_XMAX" )
        {
            fieldValues.emplace_back( wxString::Format( "%d", 4200 ) );
        }
        else if( field == "BOARD_EXTENT_YMAX" )
        {
            fieldValues.emplace_back( wxString::Format( "%d", 4200 ) );
        }
        else if( field == "BOARD_ACCURACY" )
        {
            fieldValues.emplace_back( wxString::Format( "%d", 1 ) );
        }
        else if( field == "BOARD_UNITS" )
        {
            fieldValues.emplace_back( FormatUnits( m_Brd.m_Header->m_BoardUnits ) );
        }
        else if( field == "BOARD_SCHEMATIC_NAME" )
        {
            fieldValues.emplace_back( "SCH_NAME" );
        }
        else if( field == "BOARD_THICKNESS" )
        {
            fieldValues.emplace_back( FormatUnits( m_Brd.m_Header->m_BoardUnits, 42 ) );
        }
        else if( field == "BOARD_LAYERS" )
        {
            fieldValues.emplace_back( wxString::Format( "%d", 2 ) );
        }
        else if( field == "BOARD_DRC_STATUS" )
        {
            fieldValues.emplace_back( "OUT_OF_DATE" );
        }
        else
        {
            wxLogWarning( "Unsupported BOARD extract field: %s", TO_UTF8( field ) );
            fieldValues.emplace_back( "??" );
        }
    }

    LINE_FORMATTER lineFormatter( aLinePrefix, m_Formatter );
    lineFormatter.Add( fieldValues );
}


void ASCII_EXTRACTOR::extractBoardInfo()
{
    EXTRACT_SPEC_PARSER::IR::BLOCK boardBlock;

    boardBlock.ViewType = EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::BOARD;
    boardBlock.Fields = {
        "BOARD_NAME",
        "__EXTRACTION_TIME",
        "BOARD_EXTENT_XMIN",
        "BOARD_EXTENT_YMIN",
        "BOARD_EXTENT_XMAX",
        "BOARD_EXTENT_YMAX",
        "BOARD_ACCURACY",
        "BOARD_UNITS",
        "BOARD_SCHEMATIC_NAME",
        "BOARD_THICKNESS",
        "BOARD_LAYERS",
        "BOARD_DRC_STATUS",
    };

    extractBoardInfo( boardBlock, 'J' );
}

void ASCII_EXTRACTOR::extractSymbolInstances( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    OBJECT_VISITOR objectVisitor = createObjectVisitor( aBlock );

    // And now visit all the footprint instances, and then visit each field on each one
    m_Brd.VisitFootprintDefs( [&]( const FOOTPRINT_DEF& aFpDef )
    {
        m_Brd.VisitFootprintInstances( aFpDef, [&]( const FOOTPRINT_INSTANCE& aFp )
        {
            visitForLine( aFp, 'S', objectVisitor );
        } );
    } );
}


void ASCII_EXTRACTOR::extractComponentPins( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    OBJECT_VISITOR objectVisitor = createObjectVisitor( aBlock );

    // Visit all component pins
    // m_Brd.VisitComponentDefs( [&]( const COMPONENT_DEF& aCompDef )
    // {
    //     m_Brd.VisitComponentPins( aCompDef, [&]( const COMPONENT_PIN& aPin )
    //     {
    //         visitForLine( aPin, 'P', objectVisitor );
    //     } );
    // } );
}


void ASCII_EXTRACTOR::extractFunctions( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    OBJECT_VISITOR objectVisitor = createObjectVisitor( aBlock );

    // Visit all function instances
    m_Brd.VisitFunctionInstances( [&]( const x06_OBJECT& aComponent, const FUNCTION_INSTANCE& aFuncInst )
    {
        visitForLine( aFuncInst, 'F', objectVisitor );
    } );
}


void ASCII_EXTRACTOR::Extract( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    {
        LINE_FORMATTER lineFormatter( 'A', m_Formatter );
        lineFormatter.Add( aBlock.Fields );
    }

    extractBoardInfo();

    // Now, for each block type, do the extraction.
    switch( aBlock.ViewType )
    {
        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::COMPONENT_PIN:
            extractComponentPins( aBlock );
            break;

        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::SYMBOL:
            // Remember that Allegro symbols are KiCad footprints.
            extractSymbolInstances( aBlock );
            break;

        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::BOARD:
            // Board-level info is always extracted in the 'J' line above,
            // but it can still be requested
            extractBoardInfo( aBlock, 'S' );
            break;

        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::FUNCTION:
            extractFunctions( aBlock );
            break;

        default:
            wxLogWarning( "Unsupported extract block type %d", static_cast<int>( aBlock.ViewType ) );
            break;
    }
}
