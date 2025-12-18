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

#include <set>

#include <wx/datetime.h>
#include <wx/log.h>

#include <string_utils.h>

#include <cmath>
#include <cstdio>

using namespace ALLEGRO;


static std::string FormatDouble3( double aValue )
{
    char buf[32];
    std::snprintf( buf, sizeof( buf ), "%.3f", aValue ); //format:allow locale handled below

    for( char* p = buf; *p; ++p )
    {
        if( *p == ',' )
            *p = '.';
    }

    return buf;
}


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
            return wxString( FormatDouble2Str( aValue ) ) + wxS( " mil" );
        case ALLEGRO::BOARD_UNITS::METRIC:
            return wxString( FormatDouble2Str( aValue ) ) + wxS( " mm" );
    }

    return wxString( FormatDouble2Str( aValue ) ) + wxS( " unknown_units" );
}


static wxString YesNoBlank( std::optional<bool> aValue )
{
    if( !aValue.has_value() )
        return "";

    return aValue.value() ? "YES" : "NO";
}


using NET_EXTRACTOR = std::function<std::optional<int>( const NET& aNet )>;

const auto ExtractLinearFromNet( const VIEW_OBJS& aObj, NET_EXTRACTOR aNetExtractor ) -> wxString
{
    const NET* net = aObj.m_Net;

    wxCHECK2( net, "" );

    std::optional<int> optVal = aNetExtractor( *net );

    if( optVal.has_value() )
    {
        return FormatUnits( aObj.m_Board->m_Header->m_BoardUnits, optVal.value() );
    }
    else
    {
        return "";
    }
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
        m_Formatter.Print( "%s!", FormatDouble3( aDegrees / 1000.0 ).c_str() );
    }

    void AddYesNo( bool aValue )
    {
        m_Formatter.Print( "%s!", aValue ? "YES" : "NO" );
    }

private:
    OUTPUTFORMATTER& m_Formatter;
};


using VTYPE = ALLEGRO::EXTRACT_SPEC_PARSER::IR::VIEW_TYPE;
/**
 * Some function that extracts one field from a VIEW_OBJS in the context of a given view type.
 */
using FIELD_EXTRACTOR = std::function<wxString( const VIEW_OBJS& aViewObjs, VTYPE aViewType )>;


struct FIELD_EXTRACTOR_DEF
{
    FIELD_EXTRACTOR m_Extractor;
    std::set<VTYPE> m_SupportedViewTypes;
};


std::unordered_map<wxString, FIELD_EXTRACTOR_DEF> g_Extractors = {
    {
            "SYM_TYPE",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        return "PACKAGE";
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "REFDES",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        // wxCHECK2( aObj.m_FootprintInstance, "" );

                        const COMPONENT_INST* compInst = nullptr;

                        // FIXME
                        if( aObj.m_FootprintInstance )
                        {
                            const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                            compInst = fp.GetComponentInstance();
                        }
                        else if( aObj.m_ComponentInstance )
                        {
                            compInst = aObj.m_ComponentInstance;
                        }

                        if( !compInst )
                            return "";

                        const wxString* refDesStr = compInst->GetRefDesStr();
                        return refDesStr ? *refDesStr : "";
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT, VTYPE::COMPONENT_PIN, VTYPE::FUNCTION },
            },
    },
    {
            "SYM_NAME",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                        const wxString*           name = fp.GetName();
                        return name ? *name : "";
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "SYM_X",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                        return wxString( FormatDouble2Str( fp.m_X ) );
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "SYM_Y",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                        return wxString( FormatDouble2Str( fp.m_Y ) );
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "SYM_CENTER_X",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        // HACK: this is actually done via TEXT position and falls back to PLACE_BOUND center
                        const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                        return wxString( FormatDouble2Str( fp.m_X ) );
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "SYM_CENTER_Y",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                        return wxString( FormatDouble2Str( fp.m_Y ) );
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "SYM_ROTATE",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                        return wxString( FormatDouble3( fp.m_Rotation / 1000.0 ) );
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "SYM_MIRROR",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                        return fp.m_Mirrored ? "YES" : "NO";
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "SYM_LIBRARY_PATH",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FOOTPRINT_INSTANCE& fp = *aObj.m_FootprintInstance;
                        return fp.m_Parent->GetLibPath() ? *fp.m_Parent->GetLibPath() : "";
                    },
                    { VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "COMP_VALUE",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const COMPONENT* component = aObj.m_Component;

                        if( !component )
                            return "";

                        // if( component )
                        // {
                        //     const wxString* value = component->GetComponentValue();
                        //     return value ? *value : "";
                        // }
                        return "??";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::SYMBOL, VTYPE::COMPONENT },
            },
    },
    {
            "COMP_DEVICE_TYPE",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const COMPONENT* component = aObj.m_Component;

                        if( !component )
                            return "";

                        const wxString* deviceType = component->GetComponentDeviceType();
                        return deviceType ? *deviceType : "";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::FUNCTION },
            },
    },
    {
            "FUNC_DES",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FUNCTION_INSTANCE* func = aObj.m_Function;

                        wxCHECK2( func, "" );

                        const wxString* name = func->GetName();
                        return name ? *name : "";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::FUNCTION },
            },
    },
    {
            "FUNC_SLOT_NAME",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const FUNCTION_INSTANCE* func = aObj.m_Function;

                        wxCHECK2( func, "" );

                        const FUNCTION_SLOT& slot = func->GetFunctionSlot();
                        const wxString*      name = slot.GetName();
                        return name ? *name : "";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::FUNCTION },
            },
    },
    {
            "NET_NAME",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const NET* net = aObj.m_Net;

                        wxCHECK2( net, "" );

                        const wxString* name = net->GetName();
                        return name ? *name : "";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::NET },
            },
    },
    {
            "NET_MIN_LINE_WIDTH",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        return ExtractLinearFromNet( aObj,
                                                     []( const NET& aNet ) -> std::optional<int>
                                                     {
                                                         return aNet.GetNetMinLineWidth();
                                                     } );
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::NET },
            },
    },
    {
            "NET_MAX_LINE_WIDTH",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        return ExtractLinearFromNet( aObj,
                                                     []( const NET& aNet ) -> std::optional<int>
                                                     {
                                                         return aNet.GetNetMaxLineWidth();
                                                     } );
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::NET },
            },
    },
    {
            "NET_MIN_NECK_WIDTH",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        return ExtractLinearFromNet( aObj,
                                                     []( const NET& aNet ) -> std::optional<int>
                                                     {
                                                         return aNet.GetNetMinNeckWidth();
                                                     } );
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::NET },
            },
    },
    {
            "NET_STATUS",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const NET* net = aObj.m_Net;

                        wxCHECK2( net, "" );

                        switch( net->GetStatus() )
                        {
                        case NET::STATUS::REGULAR: return "REGULAR";
                        case NET::STATUS::SCHEDULED: return "SCHEDULED";
                        case NET::STATUS::NO_RAT: return "NO_RAT";
                        }

                        return "??";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::NET },
            },
    },
    {
            "LOGICAL_PATH",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const NET* net = aObj.m_Net;

                        wxCHECK2( net, "" );

                        const wxString* path = net->GetLogicalPath();
                        return path ? *path : "";
                    },
                    { VTYPE::NET },
            },
    },
    {
            "PIN_NAME",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const PLACED_PAD* pad = aObj.m_Pad;

                        wxCHECK2( pad, "" );

                        const wxString* name = pad->GetPinName();
                        return name ? *name : "";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::LOGICAL_PIN },
            },
    },
    {
            "PIN_NUMBER",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        const PLACED_PAD* pad = aObj.m_Pad;

                        wxCHECK2( pad, "" );

                        const wxString* number = pad->GetPinNumber();
                        return number ? *number : "";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::LOGICAL_PIN },
            },
    },
    {
            "PIN_POWER",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        // std::optional<bool> padIsPowerPin = std::nullopt;
                        // return YesNoBlank( pad->IsPowerPin() );
                        return "??";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::LOGICAL_PIN },
            },
    },
    {
            "PIN_GROUND",
            {
                    []( const VIEW_OBJS& aObj, VTYPE aViewType ) -> wxString
                    {
                        return "??";
                    },
                    { VTYPE::COMPONENT_PIN, VTYPE::LOGICAL_PIN },
            },
    },
};


FIELD_EXTRACTOR GetFieldExtractor( const wxString& aFieldName )
{
    auto it = g_Extractors.find( aFieldName );

    if( it == g_Extractors.end() )
    {
        return nullptr;
    }

    // Wrap the core field extractors in common checks
    const auto extractor = [extractorDef = it->second, aFieldName]( const VIEW_OBJS& aViewObjs,
                                                                    VTYPE            aViewType ) -> wxString
    {
        // Check that the view type is supported
        if( extractorDef.m_SupportedViewTypes.find( aViewType ) == extractorDef.m_SupportedViewTypes.end() )
        {
            wxLogWarning( "Field %s not supported for view type %d", aFieldName, static_cast<int>( aViewType ) );
            return "";
        }

        return extractorDef.m_Extractor( aViewObjs, aViewType );
    };

    return extractor;
}


ASCII_EXTRACTOR::OBJECT_VISITOR ASCII_EXTRACTOR::createObjectVisitor( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    // First build the list of visitors for each symbol.
    std::vector<FIELD_EXTRACTOR> fieldVisitors;

    for( const wxString& field : aBlock.Fields )
    {
        FIELD_EXTRACTOR fieldVisitor = GetFieldExtractor( field );

        if( !fieldVisitor )
        {
            wxLogWarning( "Unsupported extract field: %s for view type %d", TO_UTF8( field ),
                          static_cast<int>( aBlock.ViewType ) );
            fieldVisitor = [&]( const VIEW_OBJS& aObj, VTYPE aViewType )
            {
                return wxString( "??" );
            };
        }

        fieldVisitors.push_back( fieldVisitor );
    }

    using CONDITION_TESTER = std::function<bool( const VIEW_OBJS& aObj )>;

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
            FIELD_EXTRACTOR extractor = GetFieldExtractor( condition.FieldName );

            if( extractor )
            {
                wxLogTrace( "ALLEGRO_EXTRACT", "Adding condition tester for field: %s",
                            TO_UTF8( condition.FieldName ) );

                andConditionTesters.push_back(
                        [=]( const VIEW_OBJS& aViewObjs ) -> bool
                        {
                            wxString value = extractor( aViewObjs, aBlock.ViewType );

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
    const auto objectVisitor = [orConditionTesters, fieldVisitors, viewType = aBlock.ViewType](
                                       const VIEW_OBJS& aViewObjs, std::vector<wxString>& aFieldValues ) -> bool
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
                    if( !tester( aViewObjs ) )
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
            aFieldValues.emplace_back( fieldVisitor( aViewObjs, viewType ) );
        }

        return true;
    };

    return objectVisitor;
}


void ASCII_EXTRACTOR::visitForLine( const VIEW_OBJS& aObj, char aPrefix, OBJECT_VISITOR& aVisitor )
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
    const auto fpInstVisitor = [&]( const VIEW_OBJS& aFpInstObjs )
    {
        visitForLine( aFpInstObjs, 'S', objectVisitor );
    };

    m_Brd.VisitFootprintInstances( fpInstVisitor );
}


void ASCII_EXTRACTOR::extractComponentPins( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    OBJECT_VISITOR objectVisitor = createObjectVisitor( aBlock );

    const auto pinVisitor = [&]( const VIEW_OBJS& aPinObjs )
    {
        visitForLine( aPinObjs, 'S', objectVisitor );
    };

    m_Brd.VisitComponentPins( pinVisitor );
}


void ASCII_EXTRACTOR::extractFunctions( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    OBJECT_VISITOR objectVisitor = createObjectVisitor( aBlock );

    // Visit all function instances
    m_Brd.VisitFunctionInstances(
            [&]( const VIEW_OBJS& aObjs )
            {
                visitForLine( aObjs, 'S', objectVisitor );
            } );
}


void ASCII_EXTRACTOR::extractNets( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    OBJECT_VISITOR objectVisitor = createObjectVisitor( aBlock );

    // Visit all nets
    m_Brd.VisitNets(
            [&]( const VIEW_OBJS& aObjs )
            {
                visitForLine( aObjs, 'S', objectVisitor );
            } );
}


void ASCII_EXTRACTOR::extractFullGeometry( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock )
{
    // This is a bit of a hassle, because I can't find a single list that traverses these items
    OBJECT_VISITOR objectVisitor = createObjectVisitor( aBlock );

    // Visit connects first
    m_Brd.VisitConnectedGeometry(
            [&]( const VIEW_OBJS& aObjs )
            {
                visitForLine( aObjs, 'S', objectVisitor );
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

        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::NET:
            extractNets( aBlock );
            break;

        case EXTRACT_SPEC_PARSER::IR::VIEW_TYPE::FULL_GEOMETRY:
            extractFullGeometry( aBlock );
            break;

        default:
            wxLogWarning( "Unsupported extract block type %d", static_cast<int>( aBlock.ViewType ) );
            break;
    }
}
