/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

#include <pegtl.hpp>

namespace CADSTAR_PARTS_LIB
{
using namespace tao::pegtl;

//-------------------- Grammar definition ----------------------------------------------------

/**
 * Needed, because PEGTL "space" includes newline characters
 */
struct WHITESPACE : one<' ', '\t'>{};

/**
 * Empty line with whitespaces
 */
struct EMPTY_LINE : seq< bol, star<WHITESPACE>, eol>{};

/**
 * Any text in the format can span multiple lines using '&'
 */
struct LINE_CONTINUATION : seq<one<'&'>, eol>{};


struct WHITESPACE_OR_CONTINUATION : sor<WHITESPACE, LINE_CONTINUATION> {};

/**
 * String segment( no line continuation ), with exclusion rules
 */
template <typename... EXCLUSION_RULES>
struct STR_SEGMENT_EXCLUDING : plus<not_at<sor<eolf, LINE_CONTINUATION,
                                               EXCLUSION_RULES...>>, any>{};

/**
 * String with optional line continuation and exclusion rules
 */
template <typename... EXCLUSION_RULES>
struct STRING_EXCLUDING : plus<STR_SEGMENT_EXCLUDING<EXCLUSION_RULES...>,
                               opt<LINE_CONTINUATION>> {};


/**
 * Control character with or without preceding whitespace
 */
template <char... CHAR_TO_FIND>
struct spaced_ch : seq<star<WHITESPACE>, one<CHAR_TO_FIND...>>{};

/**
 * String inside quotation marks
 */
struct QUOTED_STRING : seq<one<'"'>, STRING_EXCLUDING<one<'"'>>, one<'"'>> {};

/**
 * String inside single quotation marks
 */
struct SINGLE_QUOTED_STRING : seq<one<'\''>, STRING_EXCLUDING<one<'\''>>, one<'\''>> {};


/**
 * String inside brackets with preceding spaces
 */
struct STRING_IN_BRACKETS :
                seq
                <
                    spaced_ch<'('>,
                    sor<
                        QUOTED_STRING,
                        STRING_EXCLUDING<one<')'>>
                    >,
                    one<')'>
                >
{};

/**
 * String inside brackets with preceding spaces, ending with EOL or EOF
 */
struct STRING_IN_BRACKETS_EOLF :
                seq
                <
                    spaced_ch<'('>,
                    sor<
                        QUOTED_STRING,
                        STRING_EXCLUDING<seq<one<')'>, eolf>>
                    >,
                    seq<one<')'>, eolf>
                >
{};


// **************
// * FORMAT     *
// **************

// Definition of "Format"
// # FORMAT <current format number>
struct CURRENT_FORMAT_NUMBER : plus<digit> {};
struct FORMAT : seq
                <
                    bol,
                    one<'#'>,
                    star<WHITESPACE>,
                    TAO_PEGTL_ISTRING( "FORMAT" ),
                    star<WHITESPACE>,
                    CURRENT_FORMAT_NUMBER,
                    opt<eol>
                >
{};


// Newer Parts files have possibility of specifying a tree-like structure to show hierarchy
//
// Example:
//+N0 'root' &
//'part1' 'part2'
//+N1 N0 'subnode1' 'part3' 'part4'
struct HIERARCHY_NODE_INDEX :   plus<digit>{};
struct HIERARCHY_CURRENT_NODE : seq<one<'N'>, HIERARCHY_NODE_INDEX>{};
struct HIERARCHY_PARENT_NODE :  seq<one<'N'>, HIERARCHY_NODE_INDEX>{}; // Different action
struct HIERARCHY_NODE_NAME :    SINGLE_QUOTED_STRING {};
struct HIERARCHY_PART_NAME :    SINGLE_QUOTED_STRING {};
struct HIERARCHY_NODE_ENTRY :
                seq
                <
                    bol,
                    one<'+'>,
                    HIERARCHY_CURRENT_NODE,     // N1
                    plus<WHITESPACE_OR_CONTINUATION>,
                    opt<HIERARCHY_PARENT_NODE>, // N0
                    star<WHITESPACE_OR_CONTINUATION>,
                    HIERARCHY_NODE_NAME,       // 'subnode1'
                    star<WHITESPACE_OR_CONTINUATION>,
                    star<HIERARCHY_PART_NAME, star<WHITESPACE_OR_CONTINUATION>>, // 'part1' 'part2'
                    opt<eol>
                >
{};

// **************
// * PART ENTRY *
// **************

// Part Header
// -----------
//.<Part name>_[(<Part number>)][:<Part version>][;<Description>]

    // string filters:
struct PART_HEADER_START : one <'.'>{};
struct PART_NAME_FILTER : sor<spaced_ch<'('>, spaced_ch<':'>, spaced_ch<';'>>{};
struct PART_NUMBER_FILTER : one<')'>{};
struct PART_VERSION_FILTER : spaced_ch<';'>{};

// part header elements:
struct PART_NAME : STRING_EXCLUDING<PART_NAME_FILTER> {};
struct PART_NUMBER : STRING_IN_BRACKETS {};
struct PART_VERSION : STRING_EXCLUDING<PART_VERSION_FILTER> {};
struct PART_DESCRIPTION : STRING_EXCLUDING<> {};

struct PART_HEADER :
                seq
                <
                    bol,
                    one<'.'>,
                    must<PART_NAME>,
                    opt<PART_NUMBER>,
                    opt<seq<spaced_ch<':'>, PART_VERSION>>,
                    opt<seq<spaced_ch<';'>, PART_DESCRIPTION>>,
                    opt<eol>
                >
{};

// Part - PCB Component
// --------------------
//<PCB Component Refname>[_(<PCB Alternate Refname>)]

// string filters:
struct PCB_COMPONENT_FILTER : spaced_ch<'('>{};
struct PCB_ALTERNATE_FILTER : one<')'>{};

// pcb component elements
struct PCB_COMPONENT : STRING_EXCLUDING<PCB_COMPONENT_FILTER> {};
struct PCB_ALTERNATE : STRING_IN_BRACKETS {};

struct PART_PCB_COMPONENT :
                seq
                <
                    bol,
                    PCB_COMPONENT,
                    opt<PCB_ALTERNATE>,
                    opt<eol>
                >
{};

// Part Value
// -----------
//[*VALUE_<Value>]
struct VALUE : STRING_EXCLUDING<> {};
struct PART_VALUE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*VALUE"),
                    plus<WHITESPACE>,
                    VALUE,
                    opt<eol>
                >
{};

struct PINNUM : plus<digit> {};

// Pin Names
//[*PNM_<Pinnum>=<Pinname>[_<Pinnum>=<Pinname>]_etc]
// Maximum 10 characters allowed for Pinname according to documentation
struct PINNAME : rep_min_max<1,10,alnum> {};
struct PINNAME_ENTRY :
                seq
                <
                    plus<WHITESPACE_OR_CONTINUATION>,
                    PINNUM,
                    one<'='>,
                    PINNAME
                >
{};

struct PIN_NAMES_LIST :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*PNM"),
                    plus<PINNAME_ENTRY>,
                    opt<eol>
                >
{};

// Pin Labels
//[*PLB_<Pinnum>=<Pinlabel>[_<Pinnum>=<Pinlabel>]_etc]
struct PINLABEL : sor<QUOTED_STRING, STRING_EXCLUDING<WHITESPACE>> {};
struct PINLABEL_ENTRY :
                seq
                <
                    plus<WHITESPACE_OR_CONTINUATION>,
                    PINNUM,
                    one<'='>,
                    PINLABEL
                >
{};

struct PIN_LABELS_LIST :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*PLB"),
                    plus<PINLABEL_ENTRY>,
                    opt<eol>
                >
{};

// Pin Equivalences
//[*EQU_<PinIdentifier>=<PinIdentifier>[=<PinIdentifier>=<PinIdentifier>_etc ...]]
struct EQUIVALENT_PIN : PINNUM {}; // same grammar but different action to be applied
struct EQUIVALENT_PINS_GROUP :
                seq
                <
                    plus<WHITESPACE_OR_CONTINUATION>,
                    EQUIVALENT_PIN,
                    plus<one<'='>, star<WHITESPACE_OR_CONTINUATION>, EQUIVALENT_PIN>
                >
{};

struct PIN_EQUIVALENCES :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*EQU"),
                    EQUIVALENT_PINS_GROUP,
                    star<one<','>, EQUIVALENT_PINS_GROUP>,
                    opt<eol>
                >
{};


// INTERNAL AND EXTERNAL PIN SWAPPING
// *SYM_[<Element name>]
struct SYM_ELEMENT_NAME : STRING_EXCLUDING<> {};
struct SYM_LINE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*SYM"),
                    star<WHITESPACE>,
                    opt<SYM_ELEMENT_NAME>,
                    opt<eol>
                >
{};

struct GATE_PINS_LIST :
                seq
                <
                    plus<WHITESPACE>,
                    EQUIVALENT_PIN,
                    star
                    <
                        plus<WHITESPACE_OR_CONTINUATION>,
                        EQUIVALENT_PIN
                    >
                >
{};

//[*INT_<Pinnum>_[<Pinnum>_ etc ...]]
struct INTERNAL_SWAP_GATE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*INT"),
                    GATE_PINS_LIST,
                    opt<eol>
                >
{};

//[*EXT_<Pinnum>_[<Pinnum>_ etc ...]]
struct EXTERNAL_SWAP_GATE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*EXT"),
                    GATE_PINS_LIST,
                    opt<eol>
                >
{};

// Internal swapping group E.g.:
//*SYM SYM1
//*INT 2 3
//*INT 4 5
struct INTERNAL_SWAP_GROUP :
                seq
                <
                    SYM_LINE,
                    plus<INTERNAL_SWAP_GATE>
                >
{};

// External swapping group E.g.:
//*SYM SYM1
//*EXT 2 3
//*EXT 4 5
struct EXTERNAL_SWAP_GROUP :
                seq
                <
                    SYM_LINE,
                    plus<EXTERNAL_SWAP_GATE>
                >
{};

// Part Definition
// -----------
// [*DFN_<Definition name>]
struct DEFINITION_NAME : STRING_EXCLUDING<> {};
struct DFN_LINE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*DFN"),
                    plus<WHITESPACE>,
                    DEFINITION_NAME,
                    opt<eol>
                >
{};

// [*NGS]
struct NGS_LINE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*NGS"),
                    opt<eol>
                >
{};

// [*NPV]
struct NPV_LINE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*NPV"),
                    opt<eol>
                >
{};

// [*STM_<Component name stem>]
struct STEM : STRING_EXCLUDING<> {};
struct STM_LINE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*STM"),
                    plus<WHITESPACE>,
                    STEM,
                    opt<eol>
                >
{};

// [*MXP <Maximum number of connector pins>]
struct MAX_PIN_COUNT : plus<digit> {};
struct MXP_LINE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*MXP"),
                    plus<WHITESPACE>,
                    MAX_PIN_COUNT,
                    opt<eol>
                >
{};

// [*SPI_[(<Part name>)]_[<Model>]_<Component Value>]
struct SPICE_PART_NAME : STRING_IN_BRACKETS {};
struct SPICE_MODEL : sor<QUOTED_STRING, STRING_EXCLUDING<>> {};
struct SPI_LINE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*SPI"),
                    plus<WHITESPACE>,
                    opt<SPICE_PART_NAME>,
                    star<WHITESPACE>,
                    SPICE_MODEL,
                    opt<eol>
                >
{};


// [*PAC_(<Part name>)_<Acceptance Text>]
struct ACCEPTANCE_PART_NAME : STRING_IN_BRACKETS {};
struct ACCEPTANCE_TEXT : STRING_EXCLUDING<> {};
struct PAC_LINE :
                seq
                <
                    bol,
                    TAO_PEGTL_ISTRING( "*PAC"),
                    plus<WHITESPACE>,
                    opt<ACCEPTANCE_PART_NAME>,
                    plus<WHITESPACE>,
                    ACCEPTANCE_TEXT,
                    opt<eol>
                >
{};

// User defined part attributes
// -----------
// [*<User-defined name>_<Value>]
struct USER_PART_ATTRIBUTE_NAME : sor<QUOTED_STRING, STRING_EXCLUDING<WHITESPACE>> {};
struct USER_PART_ATTRIBUTE_VALUE : STRING_EXCLUDING<> {};
struct USER_PART_ATTRIBUTE :
                seq
                <
                    bol,
                    one<'*'>,
                    USER_PART_ATTRIBUTE_NAME,
                    plus<WHITESPACE>,
                    USER_PART_ATTRIBUTE_VALUE,
                    opt<eol>
                >
{};

//----------------------------------------------------
// In-built attributes: schematic, PCB, both (sch+pcb) and parts
//----------------------------------------------------
struct READONLY : one <'!'>{};
struct ATTRIBUTE_NAME : sor<QUOTED_STRING, STRING_EXCLUDING< spaced_ch<'('>>> {};
struct ATTRIBUTE_VALUE : STRING_IN_BRACKETS {};
struct ATTRIBUTE_VALUE_EOLF : STRING_IN_BRACKETS_EOLF {};

template<char START_TOKEN>
struct GENERIC_ATTRIBUTE :
                seq
                <
                    bol,
                    one<START_TOKEN>,
                    opt<READONLY>,
                    ATTRIBUTE_NAME,
                    ATTRIBUTE_VALUE_EOLF
                >
{};

// [$[!]<SCM Attribute name>(<Attribute value>)]
struct SCM_ATTRIBUTE : GENERIC_ATTRIBUTE<'$'>{};


// [%[!]<PCB Attribute name>(<Attribute value>)]
struct PCB_ATTRIBUTE : GENERIC_ATTRIBUTE<'%'>{};


// [~[!]<Parts Library Attribute Name>(<Attribute Value>)]
struct PART_ATTRIBUTE : GENERIC_ATTRIBUTE<'~'>{};


// [@[!]<SCM/PCB Attribute name>(<Attribute value>)]
struct SCH_PCB_ATTRIBUTE : GENERIC_ATTRIBUTE<'@'>{};


// [<SCM Symbol Refname>][_(<SCM Alternate Refname>)]
struct SCH_NAME : sor<QUOTED_STRING, STRING_EXCLUDING<spaced_ch<'('>>> {};
struct SCH_ALTERNATE : STRING_IN_BRACKETS {};
struct SCH_SYMBOL_LINE : seq<SCH_NAME, opt<SCH_ALTERNATE>, opt<eol>>{};

// [<PinIdentifier>[.<Position>] [!<Pintype>] [:<Loading>]]
struct PIN_IDENTIFIER : plus<digit>{};
struct PIN_POSITION : range<'0', '3'>{};
struct PIN_TYPE : star<alpha>{};
struct PIN_LOADING : plus<digit>{};

struct PIN_ENTRY :
                seq
                <
                    PIN_IDENTIFIER,
                    one<'.'>,
                    PIN_POSITION,
                    opt< one<'!'>, PIN_TYPE>,
                    opt< one<':'>, PIN_LOADING>
                >
{};

struct PIN_LIST : plus<PIN_ENTRY, star<WHITESPACE>, opt<LINE_CONTINUATION>> {};

struct SYMBOL_ENTRY : seq<SCH_SYMBOL_LINE, PIN_LIST, opt<eol>>{};


// /<Signame>_<PinIdentifier>[.<Position>][!<Pintype>][:<Loading>]
struct PIN_SIGNAL_NAME : seq<one<'/'>, STRING_EXCLUDING<WHITESPACE>> {};
struct HIDDEN_PIN_ENTRY : seq<PIN_SIGNAL_NAME, plus<WHITESPACE>, PIN_LIST, opt<eol>>{};


//******************
// Join all together

struct PART_ENTRY :
                seq
                <
                    PART_HEADER,               // .<Part name>[ (1234): 1 ;<Description>]
                    PART_PCB_COMPONENT,        // <PCB Component Refname> [(Alternate)]

                    // In any order:
                    star<sor<
                        PART_VALUE,            // [*VALUE <Value>]
                        PIN_NAMES_LIST,        // [*PNM <ID><Name>[ <ID><Name>] ...]
                        PIN_LABELS_LIST,       // [*PLB <ID><Label>[ <ID><Label>] ...]
                        PIN_EQUIVALENCES,      // [*EQU_<ID>=<ID>[=<ID>=<ID>_etc ...]]
                        INTERNAL_SWAP_GROUP,   // [*SYM SYM1  |*INT 2 3 |*INT 4 5]
                        EXTERNAL_SWAP_GROUP,   // [*SYM SYM1  |*EXT 2 3 |*EXT 4 5]
                        DFN_LINE,              // [*DFN_<Definition name>]
                        NGS_LINE,              // [*NGS]
                        NPV_LINE,              // [*NPV]
                        STM_LINE,              // [*STM_<Component name stem>]
                        MXP_LINE,              // [*MXP <Maximum number of connector pins>]
                        SPI_LINE,              // [*SPI_[(<Part name>)]_[<Model>]_<Component Value>]
                        PAC_LINE,              // [*PAC_(<Part name>)_<Acceptance Text>]
                        USER_PART_ATTRIBUTE,   // [*<User-defined name>_<Value>]
                        SCM_ATTRIBUTE,         // [$[!]<SCM Attribute name>(<Attribute value>)]
                        PCB_ATTRIBUTE,         // [%[!]<PCB Attribute name>(<Attribute value>)]
                        PART_ATTRIBUTE,        // [~[!]<Parts Library Attribute Name>(<Attribute
                                               // Value>)]
                        SCH_PCB_ATTRIBUTE      // [@[!]<SCM/PCB Attribute name>(<Attribute value>)]
                    >>,
                    star<SYMBOL_ENTRY>,        // [<SCM Symbol Refname>][_(<SCM Alternate Refname>)]
                                               // [Pin entry] [Pin entry] ...

                    star<HIDDEN_PIN_ENTRY>     // [/<Signame>_<Pin entry>]

                >
{};


/**
 * Grammar for CADSTAR Parts Library file format (*.lib).
 */
struct GRAMMAR :
                must<
                    opt<FORMAT>,
                    star<star<EMPTY_LINE>, HIERARCHY_NODE_ENTRY>,
                    plus
                    <
                        sor
                        <
                            PART_ENTRY,
                            EMPTY_LINE // optional empty line
                        >,
                        opt<eol>
                    >,
                    opt<TAO_PEGTL_ISTRING( ".END"), opt<eol>>,
                    star<EMPTY_LINE>,
                    tao::pegtl::eof // just putting "eof" results in ambiguous symbol
                >
{};


/**
 * Grammar to parse the file header only.
 *
 * In general a valid file should have `#FORMAT 32` in the first line but there appear to be some
 * files that omit the format specifier and start straight away with the part definitions. Just
 * in case, we will also allow the first part to be up to 5 lines into the file (arbitrary number
 * just to limit the time spent in reading a file header to determine whether it is valid).
 */
struct VALID_HEADER : sor<FORMAT, seq< rep_max<5,EMPTY_LINE>, PART_HEADER>>{};

} // namespace CADSTAR_PART_LIB
