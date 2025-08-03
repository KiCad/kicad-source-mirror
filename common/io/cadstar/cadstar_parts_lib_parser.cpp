/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "cadstar_parts_lib_parser.h"
#include "cadstar_parts_lib_grammar.h"

#include <fmt.h>
#include <set>
#include <string>

using namespace CADSTAR_PARTS_LIB;


/**
 * Struture that will be populated by the PEGTL parser
 */
struct CADSTAR_LIB_PARSER_STATE
{
    std::string                   m_CurrentString;
    std::string                   m_CurrentAttrName;
    std::string                   m_CurrentSignalName;
    long                          m_CurrentLong = 0;
    CADSTAR_PART_NODE             m_CurrentNode;
    long                          m_CurrentNodeIdx = 0;
    std::vector<long>             m_CurrentPinEquivalenceGroup;
    std::set<std::string>         m_CurrentElementsParsed;
    bool                          m_ReadOnly = false;
    CADSTAR_SWAP_GROUP            m_CurrentSwapGroup;
    CADSTAR_PART_PIN              m_CurrentPin;
    std::vector<CADSTAR_PART_PIN> m_CurrentPinList;
    CADSTAR_PART_SYMBOL_ENTRY     m_CurrentSymbol;
    CADSTAR_PART_ENTRY            m_CurrentPart;

    CADSTAR_PARTS_LIB_MODEL m_ParsedModel;
};


// Default action: Do nothing
template <typename Rule>
struct CADSTAR_LIB_PARSER_ACTION : tao::pegtl::nothing<Rule>
{
};


long helperStringToLong( std::string aString )
{
    std::stringstream ss( aString );
    long              number;
    ss >> number;
    return number;
};


//
// CONTENT TO NUMBER ACTIONS:
// Take the current content string, convert it to a long and store it in StateVariable
//
#define DEFINE_CONTENT_TO_NUMBER_ACTION( Rule, StateVariable )                                   \
template <>                                                                                      \
struct CADSTAR_LIB_PARSER_ACTION<Rule>                                                           \
{                                                                                                \
    template <typename ActionInput>                                                              \
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )                      \
    {                                                                                            \
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );                          \
        s.StateVariable = helperStringToLong( in.string() );                                     \
    }                                                                                            \
}

DEFINE_CONTENT_TO_NUMBER_ACTION( CURRENT_FORMAT_NUMBER, m_ParsedModel.m_FormatNumber );
DEFINE_CONTENT_TO_NUMBER_ACTION( PINNUM,                m_CurrentLong );
DEFINE_CONTENT_TO_NUMBER_ACTION( MAX_PIN_COUNT,         m_CurrentPart.m_MaxPinCount );
DEFINE_CONTENT_TO_NUMBER_ACTION( PIN_IDENTIFIER,        m_CurrentPin.m_Identifier );
DEFINE_CONTENT_TO_NUMBER_ACTION( PIN_LOADING,           m_CurrentPin.m_Loading );
DEFINE_CONTENT_TO_NUMBER_ACTION( HIERARCHY_NODE_INDEX,  m_CurrentLong );


// unfortunately the one below needs to be defined separately
template <>
struct CADSTAR_LIB_PARSER_ACTION<EQUIVALENT_PIN>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );
        s.m_CurrentPinEquivalenceGroup.push_back( helperStringToLong( in.string() ) );
    }
};


//
// CONTENT TO CURRENT STRING ACTIONS:
// Take the current content string, store it in the state current string
//
#define DEFINE_CONTENT_TO_STRING_ACTION( Rule )                                                  \
template <>                                                                                      \
struct CADSTAR_LIB_PARSER_ACTION<Rule>                                                           \
{                                                                                                \
    template <typename ActionInput>                                                              \
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )                      \
    {                                                                                            \
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );                          \
        s.m_CurrentString = in.string();                                                         \
    }                                                                                            \
}

DEFINE_CONTENT_TO_STRING_ACTION( PINNAME );

//
// STRING REPLACEMENT ACTIONS:
// Take the current string in the parser state and store it in StateVariable
//
#define DEFINE_STRING_ACTION( Rule, StateVariable )                                              \
template <>                                                                                      \
struct CADSTAR_LIB_PARSER_ACTION<Rule>                                                           \
{                                                                                                \
    /* @todo : convert to use apply0 to improve performance( once fully tested ) */              \
    template <typename ActionInput>                                                              \
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )                      \
    {                                                                                            \
        assert( in.string().size() >= s.m_CurrentString.size() );                                \
        s.StateVariable = s.m_CurrentString;                                                     \
        s.m_CurrentString = "";                                                                  \
    }                                                                                            \
}                                                                                                \

DEFINE_STRING_ACTION( PART_NAME,                m_CurrentPart.m_Name );
DEFINE_STRING_ACTION( PART_VERSION,             m_CurrentPart.m_Version );
DEFINE_STRING_ACTION( PART_NUMBER,              m_CurrentPart.m_Number );
DEFINE_STRING_ACTION( PART_DESCRIPTION,         m_CurrentPart.m_Description );
DEFINE_STRING_ACTION( PCB_COMPONENT,            m_CurrentPart.m_Pcb_component );
DEFINE_STRING_ACTION( PCB_ALTERNATE,            m_CurrentPart.m_Pcb_alternate );
DEFINE_STRING_ACTION( VALUE,                    m_CurrentPart.m_Value );
DEFINE_STRING_ACTION( DEFINITION_NAME,          m_CurrentPart.m_PartDefinitionName );
DEFINE_STRING_ACTION( STEM,                     m_CurrentPart.m_ComponentStem );
DEFINE_STRING_ACTION( SYM_ELEMENT_NAME,         m_CurrentSwapGroup.m_Name );
DEFINE_STRING_ACTION( USER_PART_ATTRIBUTE_NAME, m_CurrentAttrName );
DEFINE_STRING_ACTION( ATTRIBUTE_NAME,           m_CurrentAttrName );
DEFINE_STRING_ACTION( PIN_SIGNAL_NAME,          m_CurrentSignalName );
DEFINE_STRING_ACTION( ACCEPTANCE_PART_NAME,     m_CurrentPart.m_AcceptancePartName );
DEFINE_STRING_ACTION( ACCEPTANCE_TEXT,          m_CurrentPart.m_AcceptanceText );
DEFINE_STRING_ACTION( SPICE_PART_NAME,          m_CurrentPart.m_SpicePartName );
DEFINE_STRING_ACTION( SPICE_MODEL,              m_CurrentPart.m_SpiceModel );
DEFINE_STRING_ACTION( SCH_NAME,                 m_CurrentSymbol.m_SymbolName );
DEFINE_STRING_ACTION( SCH_ALTERNATE,            m_CurrentSymbol.m_SymbolAlternateName );


// STRING SEGMENT action
// Any strings we match, append to the current state string (the state string gets
// reset after we extract the string to store somewhere else).
// The reason we append is because in the fileformat, there can be line continuations,
// which we don't want to have in the final string - saves post-processing.
template <typename... EXCLUSION_RULES>
struct CADSTAR_LIB_PARSER_ACTION<STR_SEGMENT_EXCLUDING<EXCLUSION_RULES...>>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )
    {
        s.m_CurrentString += in.string();
    }
};


//
// HIERARCHY actions
//
template <>
struct CADSTAR_LIB_PARSER_ACTION<HIERARCHY_NODE_ENTRY>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );
        s.m_ParsedModel.m_HierarchyNodes.insert(
                { s.m_CurrentNodeIdx, std::move( s.m_CurrentNode ) } );
        s.m_CurrentNode = CADSTAR_PART_NODE();
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<HIERARCHY_CURRENT_NODE>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );
        s.m_CurrentNodeIdx = s.m_CurrentLong;
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<HIERARCHY_PARENT_NODE>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );
        s.m_CurrentNode.m_ParentNodeIdx = s.m_CurrentLong;
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<HIERARCHY_NODE_NAME>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentAttrName == "" );
        s.m_CurrentNode.m_Name = std::move( s.m_CurrentString );
        s.m_CurrentString = "";
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<HIERARCHY_PART_NAME>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentAttrName == "" );
        s.m_CurrentNode.m_PartNames.push_back( std::move( s.m_CurrentString ) );
        s.m_CurrentString = "";
    }
};


// PART_ENTRY action
// We just push the part to the vector of parts in our state
template <>
struct CADSTAR_LIB_PARSER_ACTION<PART_ENTRY>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );
        //Finish the entry
        s.m_ParsedModel.m_PartEntries.push_back( s.m_CurrentPart );
        s.m_CurrentPart = CADSTAR_PART_ENTRY();
        s.m_CurrentElementsParsed.clear();
        // Todo-we could add progress reporting here?
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<READONLY>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );
        s.m_ReadOnly = true;
    }
};


//
// SINGLE RULE ACTIONS:
// Make sure that this rule is only matched once per part and throw a parse error
// when this is not the case.
//
#define DECLARE_SINGLE_MATCH_RULE( Rule, ExtraCode )                                             \
template <>                                                                                      \
struct CADSTAR_LIB_PARSER_ACTION<Rule>                                                           \
{                                                                                                \
    template <typename ActionInput>                                                              \
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )                      \
    {                                                                                            \
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );                          \
                                                                                                 \
        if( s.m_CurrentElementsParsed.count( #Rule ) )                                           \
        {                                                                                        \
            throw parse_error( #Rule                                                             \
                               " was already defined for this part!",                            \
                    in );                                                                        \
        }                                                                                        \
                                                                                                 \
        s.m_CurrentElementsParsed.insert( #Rule );                                               \
        ExtraCode;                                                                               \
    }                                                                                            \
}                                                                                                \

DECLARE_SINGLE_MATCH_RULE( PART_VALUE, );
DECLARE_SINGLE_MATCH_RULE( DFN_LINE, );
DECLARE_SINGLE_MATCH_RULE( NGS_LINE, s.m_CurrentPart.m_GateSwappingAllowed = false );
DECLARE_SINGLE_MATCH_RULE( NPV_LINE, s.m_CurrentPart.m_PinsVisible = false );
DECLARE_SINGLE_MATCH_RULE( STM_LINE, );
DECLARE_SINGLE_MATCH_RULE( MXP_LINE, );
DECLARE_SINGLE_MATCH_RULE( SPI_LINE, );
DECLARE_SINGLE_MATCH_RULE( PAC_LINE, );


template <>
struct CADSTAR_LIB_PARSER_ACTION<PINNAME_ENTRY>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentAttrName == "" );
        // m_CurrentLong should have been parsed as part of the PINNUM action
        // m_CurrentString should have been parsed as part of the PINNAME action
        s.m_CurrentPart.m_PinNamesMap.insert( { s.m_CurrentLong, s.m_CurrentString } );
        s.m_CurrentString = "";
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<PINLABEL_ENTRY>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        assert( s.m_CurrentAttrName == "" );
        // m_CurrentLong should have been parsed as part of the PINNUM action
        // m_CurrentString should have been parsed as part of the PINLABEL action
        s.m_CurrentPart.m_PinLabelsMap.insert( { s.m_CurrentLong, s.m_CurrentString } );
        s.m_CurrentString = "";
    }
};

//
// PIN EQUIVALENCE GROUP ACTIONS:
// Take the current m_CurrentPinEquivalenceGroup in the parser state and store it in StateVariable
// then clear m_CurrentPinEquivalenceGroup.
// Note that m_CurrentPinEquivalenceGroup should have been parsed as part of EQUIVALENT_PIN action
//
#define DEFINE_PIN_GROUP_ACTION( Rule, StateVariable )                                           \
template <>                                                                                      \
struct CADSTAR_LIB_PARSER_ACTION<Rule>                                                           \
{                                                                                                \
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )                                            \
    {                                                                                            \
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );                          \
        s.StateVariable.push_back( s.m_CurrentPinEquivalenceGroup );                             \
        s.m_CurrentPinEquivalenceGroup.clear();                                                  \
    }                                                                                            \
}                                                                                                \

DEFINE_PIN_GROUP_ACTION( EQUIVALENT_PINS_GROUP, m_CurrentPart.m_PinEquivalences );
DEFINE_PIN_GROUP_ACTION( INTERNAL_SWAP_GATE,    m_CurrentSwapGroup.m_Gates );
DEFINE_PIN_GROUP_ACTION( EXTERNAL_SWAP_GATE,    m_CurrentSwapGroup.m_Gates );


//
// SWAP GROUP ACTIONS:
// Take the current m_CurrentSwapGroup in the parser state and store it in StateVariable
// then reset m_CurrentSwapGroup.
//
#define DEFINE_SWAP_GROUP_ACTION( Rule, StateVariable )                                          \
template <>                                                                                      \
struct CADSTAR_LIB_PARSER_ACTION<Rule>                                                           \
{                                                                                                \
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )                                            \
    {                                                                                            \
        assert( s.m_CurrentString == "" && s.m_CurrentAttrName == "" );                          \
        s.StateVariable.push_back( s.m_CurrentSwapGroup );                                       \
        s.m_CurrentSwapGroup = CADSTAR_SWAP_GROUP();                                             \
    }                                                                                            \
}                                                                                                \

DEFINE_SWAP_GROUP_ACTION( INTERNAL_SWAP_GROUP, m_CurrentPart.m_InternalSwapGroup );
DEFINE_SWAP_GROUP_ACTION( EXTERNAL_SWAP_GROUP, m_CurrentPart.m_ExternalSwapGroup );


template <>
struct CADSTAR_LIB_PARSER_ACTION<USER_PART_ATTRIBUTE>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )
    {
        // The format allows user defined "part" attrbutes, but the ones listed here are in-built
        // with special meaning
        static const std::set<std::string> reservedWordsStarLines = { "VALUE", "PNM", "PLB", "EQU",
                                                                      "SYM",   "INT", "EXT", "DFN",
                                                                      "NGS",   "NPV", "STM", "MXP",
                                                                      "SPI",   "PAC" };

        if( s.m_CurrentPart.m_UserAttributes.count( s.m_CurrentAttrName ) )
        {
            throw parse_error( fmt::format( "Duplicate attribute name '{}'", s.m_CurrentAttrName ),
                               in );
        }

        if( reservedWordsStarLines.count( s.m_CurrentAttrName ) )
        {
            throw parse_error(
                    fmt::format(
                            "Invalid use of in-built attribute name '{}'. Either the attribute "
                            "was already defined for this part or it has an unexpected syntax.",
                            s.m_CurrentAttrName ),
                    in );
        }

        s.m_CurrentPart.m_UserAttributes.insert( { s.m_CurrentAttrName, s.m_CurrentString } );
        s.m_CurrentAttrName = "";
        s.m_CurrentString = "";
    }
};

#define DEFINE_ATTRIBUTE_ACTION( Rule, StateVariable )                                           \
template <>                                                                                      \
struct CADSTAR_LIB_PARSER_ACTION<Rule>                                                           \
{                                                                                                \
    template <typename ActionInput>                                                              \
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )                      \
    {                                                                                            \
        if( s.StateVariable.count( s.m_CurrentAttrName ) )                                       \
        {                                                                                        \
            throw parse_error(                                                                   \
                    fmt::format( "Duplicate attribute name '{}'", s.m_CurrentAttrName ),         \
                in );                                                                            \
        }                                                                                        \
                                                                                                 \
        CADSTAR_ATTRIBUTE_VALUE val;                                                             \
        val.m_ReadOnly = s.m_ReadOnly;                                                           \
        val.m_Value = s.m_CurrentString;                                                         \
                                                                                                 \
        s.StateVariable.insert( { s.m_CurrentAttrName, val } );                                  \
        s.m_CurrentAttrName = "";                                                                \
        s.m_CurrentString = "";                                                                  \
        s.m_ReadOnly = false;                                                                    \
    }                                                                                            \
}                                                                                                \

DEFINE_ATTRIBUTE_ACTION( SCM_ATTRIBUTE, m_CurrentPart.m_SchAttributes );
DEFINE_ATTRIBUTE_ACTION( PCB_ATTRIBUTE, m_CurrentPart.m_PcbAttributes );
DEFINE_ATTRIBUTE_ACTION( PART_ATTRIBUTE, m_CurrentPart.m_PartAttributes );
DEFINE_ATTRIBUTE_ACTION( SCH_PCB_ATTRIBUTE, m_CurrentPart.m_SchAndPcbAttributes );


template <>
struct CADSTAR_LIB_PARSER_ACTION<SYMBOL_ENTRY>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        s.m_CurrentSymbol.m_Pins.swap( s.m_CurrentPinList );
        s.m_CurrentPart.m_Symbols.push_back( std::move( s.m_CurrentSymbol ) );
        s.m_CurrentSymbol = CADSTAR_PART_SYMBOL_ENTRY();
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<PIN_ENTRY>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        s.m_CurrentPinList.push_back( std::move( s.m_CurrentPin ) );
        s.m_CurrentPin = CADSTAR_PART_PIN();
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<HIDDEN_PIN_ENTRY>
{
    static void apply0( CADSTAR_LIB_PARSER_STATE& s )
    {
        s.m_CurrentPart.m_HiddenPins.insert(
                { std::move( s.m_CurrentSignalName ), std::move( s.m_CurrentPinList ) } );
        s.m_CurrentPinList.clear();
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<PIN_POSITION>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )
    {
        s.m_CurrentPin.m_Position = CADSTAR_PIN_POSITION( helperStringToLong( in.string() ) );
    }
};


template <>
struct CADSTAR_LIB_PARSER_ACTION<PIN_TYPE>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, CADSTAR_LIB_PARSER_STATE& s )
    {
        // The format allows user defined "part" attrbutes, but the ones listed here are in-built
        // with special meaning
        static const std::map<std::string, CADSTAR_PIN_TYPE> tokenToPinType = {
            { "U", CADSTAR_PIN_TYPE::UNCOMMITTED },
            { "I", CADSTAR_PIN_TYPE::PIN_INPUT },
            { "N", CADSTAR_PIN_TYPE::OUTPUT_NOT_OR },
            { "Y", CADSTAR_PIN_TYPE::OUTPUT_OR },
            { "Q", CADSTAR_PIN_TYPE::OUTPUT_NOT_NORM_OR },
            { "P", CADSTAR_PIN_TYPE::POWER },
            { "G", CADSTAR_PIN_TYPE::GROUND },
            { "T", CADSTAR_PIN_TYPE::TRISTATE_BIDIR },
            { "TI", CADSTAR_PIN_TYPE::TRISTATE_INPUT },
            { "TD", CADSTAR_PIN_TYPE::TRISTATE_DRIVER }
        };

        if( !tokenToPinType.count( in.string() ) )
            throw parse_error( fmt::format( "Unexpected pin type '{}'", in.string() ), in );

        s.m_CurrentPin.m_Type = tokenToPinType.at( in.string() );
    }
};


template <typename INPUT_TYPE>
bool checkHeaderHelper( INPUT_TYPE& aInput )
{
    try
    {
        if( !parse<VALID_HEADER>( aInput ) )
            return false;
    }
    catch( const parse_error& )
    {
        return false;
    }

    return true;
}


bool CADSTAR_PARTS_LIB_PARSER::CheckContentHeader( const std::string& aSource ) const
{
    string_input in( aSource, "from_content" );
    return checkHeaderHelper( in );
}


bool CADSTAR_PARTS_LIB_PARSER::CheckFileHeader( const std::filesystem::path& aPath ) const
{
    file_input in( aPath );
    return checkHeaderHelper( in );
}


template<typename INPUT_TYPE>
CADSTAR_PARTS_LIB_MODEL readCadstarHelper( INPUT_TYPE& aInput )
{
    CADSTAR_LIB_PARSER_STATE s;

    try
    {
        // Todo: We could reserve space for the partEntries vector
        // to improve performance? E.g.:
        // s.m_ParsedModel.m_PartEntries.reserve( expectedNumParts );

        if( !parse<GRAMMAR, CADSTAR_LIB_PARSER_ACTION>( aInput, s ) )
            printf( "Some error occurred!\n" );
    }
    catch( const parse_error& e )
    {
        const auto& p = e.positions().front();
        std::cerr << "Error at line " << p.line << ", column " << p.column << std::endl
                  << aInput.line_at( p ) << std::endl
                  << std::setw( p.column ) << '^' << std::endl
                  << e.message() << std::endl;
    }

    return s.m_ParsedModel;
}


CADSTAR_PARTS_LIB_MODEL CADSTAR_PARTS_LIB_PARSER::ReadContent( const std::string& aSource ) const
{
    string_input in( aSource, "from_content" );
    return readCadstarHelper( in );
}


CADSTAR_PARTS_LIB_MODEL
CADSTAR_PARTS_LIB_PARSER::ReadFile( const std::filesystem::path& aPath ) const
{
    file_input in( aPath );
    return readCadstarHelper( in );
}
