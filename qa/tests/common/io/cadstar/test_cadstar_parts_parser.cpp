/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022-2023 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <algorithm>
#include <optional>

#include <pegtl/contrib/analyze.hpp>
#include <pegtl/contrib/trace.hpp>

// Modules under test:
#include <common/io/cadstar/cadstar_parts_lib_grammar.h>
#include <common/io/cadstar/cadstar_parts_lib_parser.h>


BOOST_AUTO_TEST_SUITE( CadstarPartParser );


static std::string getCadstarTestFile( const std::string& aFile )
{
    return KI_TEST::GetEeschemaTestDataDir() + "/io/cadstar/" + aFile;
}


BOOST_AUTO_TEST_CASE( AnalyzeGrammar )
{
    // Verify the grammar has no loops without progress and other issues
    // See: https://github.com/taocpp/PEGTL/blob/3.2.7/doc/Grammar-Analysis.md
    const std::size_t grammarIssues = tao::pegtl::analyze<CADSTAR_PARTS_LIB::GRAMMAR>();
    BOOST_CHECK_EQUAL( grammarIssues, 0 );

    const std::size_t headerIssues = tao::pegtl::analyze<CADSTAR_PARTS_LIB::VALID_HEADER>();
    BOOST_CHECK_EQUAL( headerIssues, 0 );
}


struct CHECK_HEADER_CASE
{
    std::string m_CaseName;
    std::string m_Content;
    bool        m_ExpectedResult;
};


static const std::vector<CHECK_HEADER_CASE> check_header_cases =
{
    { "1: Normal header",                  "# Format 32\r\n",                             true },
    { "2: Normal header, extra content",   "# Format 32\r\nExtraUnrelatedContent",        true },
    { "3: Normal header extra spaces (1)", "# Format    32\r\n",                          true },
    { "4: Normal header extra spaces (2)", "#   FORMAT 32\r\n",                           true },
    { "5: Normal header on 2nd line",      "\r\n# Format 32\r\n",                         false },
    { "6: Normal header prepended",        "+# Format 32\r\n",                            false },
    { "7: Normal header prepended spaces", "    # Format 32\r\n",                         false },

    // There appear to be some files on the internet that just don't have a header and
    // start straight away with the part definitions.
    { "8: No header",                      ".PART-NAME :1 ;Part Descr\r\n",               true },
    { "9: No header, extra content",       ".PART-NAME :1 ;Part Descr\r\nExtra",          true },
    { "10: No header, on 2nd line",        "\r\n.PART-NAME :1 ;Part Descr\r\n",           true },
    { "11: No header, on 3rd line",        "\r\n\r\n.PART-NAME :1 ;Part Descr\r\n",       true },
    { "12: No header, on 4th line",        "\r\n\r\n.PART-NAME :1 ;Part Descr\r\n",       true },
    { "13: No header, on 4th line",        "\r\n\r\n\r\n\r\n.PART-NAME :1 ;P Descr\r\n",  true },
    { "14: No header, on 5th line",        "\r\n\r\n\r\n\r\n\r\n.P-NAME :1 ;PDescr\r\n",  true },
    { "15: No header, on 6th line",        "\r\n\r\n\r\n\r\n\r\n\r\n.P-NAM :1 ;PDes\r\n", false },
    { "16: No header, space prepend",      "    .PART-NAME :1 ;Part Descr\r\n",           false },
    { "17: No header, spaces & 2nd line",  "    \r\n.PART-NAME :1 ;Part Descr\r\n",       true },
    { "18: No header, 2nd line & space",   "    \r\n  .PART-NAME :1 ;Part Descr\r\n",     false },
};


BOOST_AUTO_TEST_CASE( CheckHeader )
{
    for( const auto& c : check_header_cases )
    {
        BOOST_TEST_INFO_SCOPE( c.m_CaseName );
        CADSTAR_PARTS_LIB_PARSER p;

        BOOST_CHECK_EQUAL( p.CheckContentHeader( c.m_Content ), c.m_ExpectedResult );
    }
}


BOOST_AUTO_TEST_CASE( ReadFile )
{
    CADSTAR_PARTS_LIB_PARSER p;
    // Test a programatically generated files (see writeCadstarFile.py)
    std::vector<std::string> testFiles = { "dummycadstarlib.lib", "dummycadstarlibwithheader.lib" };

    for( auto testFile : testFiles )
    {
        auto ret = p.ReadFile( getCadstarTestFile( testFile ) );

        KI_CHECK_OPT_EQUAL( ret.m_FormatNumber, 32 );
        BOOST_CHECK_EQUAL( ret.m_PartEntries.size(), 100 );

        int i = 0;

        for( CADSTAR_PART_ENTRY& partEntry : ret.m_PartEntries )
        {
            // Part header KI_CHECK_OPT_EQUAL
            BOOST_CHECK_EQUAL( partEntry.m_Name, "PartName" + std::to_string( i ) );
            KI_CHECK_OPT_EQUAL( partEntry.m_Number, std::to_string( i * 5 ) );
            KI_CHECK_OPT_EQUAL( partEntry.m_Version, std::to_string( 2 ) );
            KI_CHECK_OPT_EQUAL( partEntry.m_Description,
                            "Part " + std::to_string( i ) + " Description" );

            BOOST_CHECK_EQUAL( partEntry.m_Pcb_component, "FOOTPRINT" + std::to_string( i ) );
            KI_CHECK_OPT_EQUAL( partEntry.m_Pcb_alternate, "variant" + std::to_string( i * 5 ) );
            KI_CHECK_OPT_EQUAL( partEntry.m_Value, std::to_string( i ) + " uH" );
            BOOST_CHECK_EQUAL( partEntry.m_ComponentStem, "L" );
            KI_CHECK_OPT_EQUAL( partEntry.m_MaxPinCount, i + 10 );
            BOOST_CHECK_EQUAL( partEntry.m_GateSwappingAllowed, i % 10 != 1 );
            BOOST_CHECK_EQUAL( partEntry.m_PinsVisible, i % 5 != 1 );

            KI_CHECK_OPT_EQUAL( partEntry.m_SpicePartName, "PartName" + std::to_string( i ) );
            KI_CHECK_OPT_EQUAL( partEntry.m_SpiceModel, std::to_string( i ) + "uH" );

            KI_CHECK_OPT_EQUAL( partEntry.m_AcceptancePartName, "PartName" + std::to_string( i ) );
            KI_CHECK_OPT_EQUAL( partEntry.m_AcceptanceText, "Acceptance" + std::to_string( i ) );

            // User part attributes (* lines)
            BOOST_CHECK_EQUAL( partEntry.m_UserAttributes["UserFieldpartNo"],
                            std::to_string( i * 5 ) );
            BOOST_CHECK_EQUAL( partEntry.m_UserAttributes["UserFieldpartNoCreated by"],
                            "Person" + std::to_string( i ) );

            // SCH attributes ($ lines)
            BOOST_CHECK_EQUAL( partEntry.m_SchAttributes["SCH val1"].m_ReadOnly, false );
            BOOST_CHECK_EQUAL( partEntry.m_SchAttributes["SCH val1"].m_Value,
                            "val" + std::to_string( i ) );
            BOOST_CHECK_EQUAL( partEntry.m_SchAttributes["SCH val2"].m_ReadOnly, true );
            BOOST_CHECK_EQUAL( partEntry.m_SchAttributes["SCH val2"].m_Value,
                            "readOnly" + std::to_string( i ) );

            // PCB attributes (% lines)
            BOOST_CHECK_EQUAL( partEntry.m_PcbAttributes["PCB val1"].m_ReadOnly, false );
            BOOST_CHECK_EQUAL( partEntry.m_PcbAttributes["PCB val1"].m_Value,
                            "val" + std::to_string( i ) );
            BOOST_CHECK_EQUAL( partEntry.m_PcbAttributes["PCB val2"].m_ReadOnly, true );
            BOOST_CHECK_EQUAL( partEntry.m_PcbAttributes["PCB val2"].m_Value,
                            "readOnly" + std::to_string( i ) );

            // Parts attributes (~ lines)
            BOOST_CHECK_EQUAL( partEntry.m_PartAttributes["Part val1"].m_ReadOnly, false );
            BOOST_CHECK_EQUAL( partEntry.m_PartAttributes["Part val1"].m_Value,
                            "val" + std::to_string( i ) );
            BOOST_CHECK_EQUAL( partEntry.m_PartAttributes["Part val2"].m_ReadOnly, true );
            BOOST_CHECK_EQUAL( partEntry.m_PartAttributes["Part val2"].m_Value,
                            "readOnly" + std::to_string( i ) );

            // PCB and SCH attributes (@ lines)
            BOOST_CHECK_EQUAL( partEntry.m_SchAndPcbAttributes["SCH and PCB val1"].m_ReadOnly,
                            false );
            BOOST_CHECK_EQUAL( partEntry.m_SchAndPcbAttributes["SCH and PCB val1"].m_Value,
                            "val" + std::to_string( i ) );
            BOOST_CHECK_EQUAL( partEntry.m_SchAndPcbAttributes["SCH and PCB val2"].m_ReadOnly,
                            true );
            BOOST_CHECK_EQUAL( partEntry.m_SchAndPcbAttributes["SCH and PCB val2"].m_Value,
                            "readOnly" + std::to_string( i ) );

            // Check symbol name and pins
            BOOST_REQUIRE_EQUAL( partEntry.m_Symbols.size(), 1 );
            BOOST_CHECK_EQUAL( partEntry.m_Symbols[0].m_SymbolName,
                            "Symbol" + std::to_string( i ) );
            KI_CHECK_OPT_EQUAL( partEntry.m_Symbols[0].m_SymbolAlternateName,
                            std::optional<std::string>() );
            BOOST_REQUIRE_EQUAL( partEntry.m_Symbols[0].m_Pins.size(), 2 );
            BOOST_CHECK_EQUAL( partEntry.m_Symbols[0].m_Pins[0].m_Identifier, 1 );
            BOOST_CHECK_EQUAL( partEntry.m_Symbols[0].m_Pins[1].m_Identifier, 2 );

            // Check hidden pins
            BOOST_REQUIRE_EQUAL( partEntry.m_HiddenPins.size(), 1 );
            BOOST_CHECK_EQUAL( partEntry.m_HiddenPins.count( "GND" ), 1 );
            i++;
        }
    }
}


BOOST_AUTO_TEST_CASE( ReadContent )
{
    std::string test =
            "# Format 32\r\n"
            "\r\n"
            "\r\n"
            "+N0 'root' &\r\n"
            "'part1' &\r\n"
            "'part2'\r\n"
            "+N1 N0 'subnode1' &\r\n"
            "'part3' &\r\n"
            "'part4'\r\n"
            "\r\n"
            " \r\n"
            "\r\n"
            ".<Part name> (<Part number>):<Part version>;<Description>\r\n"
            "<PCB Component Refname> (<PCB Alternate Refname>)\r\n"
            "*VALUE <Value>\r\n"
            "*PNM 1=A1 2=A2 3=B1 4=B2 5=C1 6=C2\r\n" // <PinId>=<Pinname> <PinId>=<Pinname> etc
            "*PLB 1=\"VCC\" 2=\"GND\" 3=\"'EN\" 4=\"OUT\" 5=\"OUT\" 6=\"IN\"\r\n" // <id>=<label>
            "*EQU 4=5, 6=7=8, 9=10=11\r\n" // <PinId>=<PinId>=<PinId> <PinId>=<PinId> etc ...
            "*SYM Group1\r\n"
            "*INT 4  5\r\n"
            "*INT 6 7\r\n"
            "*SYM Group2\r\n"
            "*EXT 1 2\r\n"
            "*EXT 4 7\r\n"
            "*DFN <Definition name>\r\n"
            "*NGS\r\n"
            "*NPV\r\n"
            "*STM <Component name stem>\r\n"
            "*MXP 32\r\n" //<Maximum number of connector pins>
            "*SPI (<Part name>) <Model> <Value>\r\n"
            "*PAC (<Part name>) <Acceptance Text>\r\n"
            "*userAttribute userAttributeVal\r\n"
            "*\"User spaced name\" userSpacedAttributeVal\r\n"
            "$<SCM Attribute name1>(<Attribute value for name1>)\r\n"
            "$!<SCM Attribute name2>(\"<Attribute value for name2>\")\r\n"
            "%<PCB Attribute name1>(\"<Attribute value1>\")\r\n"
            "%!\"<PCB Attribute name2>\"(<Attribute value2>)\r\n"
            "~<Parts Attribute name1>(<Attribute value1>)\r\n"
            "~!<Parts Attribute name2>(<Attribute value2>)\r\n"
            "@<SCM/PCB Attribute name1>(<Attribute value1>)\r\n"
            "@!<SCM/PCB Attribute name2>(<Attribute value2>)\r\n"
            "<SCM Symbol Refname1> (<SCM Alternate Refname>)\r\n"
            "1.0!TD:2000 2.1!TI 3.2!T\r\n"
            "<SCM Symbol Refname2>\r\n"
            "4.2!U:1000 5.1!I 6.3!Q\r\n"
            "/GND 7.0!G:2000\r\n"
            "/VCC 8.0!P:2000 9.1 10.0\r\n";
    //"etc ...\r\n"
    //"/<Signame> <PinIdentifier>.<Position>!<Pintype>:<Loading>\r\n"
    //"/<Signame> <PinIdentifier>.<Position>!<Pintype>:<Loading>\r\n";

    CADSTAR_PARTS_LIB_PARSER csParser;
    CADSTAR_PARTS_LIB_MODEL result = csParser.ReadContent( test );

    KI_CHECK_OPT_EQUAL( result.m_FormatNumber, 32 );
    BOOST_REQUIRE_EQUAL( result.m_HierarchyNodes.size(), 2 ); // root and subnode

    BOOST_REQUIRE_EQUAL( result.m_PartEntries.size(), 1 );

    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_Name, "<Part name>" );
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_Number.value(), "<Part number>" );
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_Version.value(), "<Part version>" );
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_Description.value(), "<Description>" );
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_Pcb_component, "<PCB Component Refname>" );
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_Pcb_alternate.value(), "<PCB Alternate Refname>" );
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_Value.value(), "<Value>" );

    // Check pin names (*PNM)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_PinNamesMap.size(), 6 );

    std::map<long, std::string>& pinNames = result.m_PartEntries[0].m_PinNamesMap;
    BOOST_CHECK_EQUAL( pinNames[1], "A1" );
    BOOST_CHECK_EQUAL( pinNames[2], "A2" );
    BOOST_CHECK_EQUAL( pinNames[3], "B1" );
    BOOST_CHECK_EQUAL( pinNames[4], "B2" );
    BOOST_CHECK_EQUAL( pinNames[5], "C1" );
    BOOST_CHECK_EQUAL( pinNames[6], "C2" );

    // Check pin labels (*PLB)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_PinLabelsMap.size(), 6 );

    std::map<long, std::string>& pinlabels = result.m_PartEntries[0].m_PinLabelsMap;
    BOOST_CHECK_EQUAL( pinlabels[1], "VCC" );
    BOOST_CHECK_EQUAL( pinlabels[2], "GND" );
    BOOST_CHECK_EQUAL( pinlabels[3], "'EN" );
    BOOST_CHECK_EQUAL( pinlabels[4], "OUT" );
    BOOST_CHECK_EQUAL( pinlabels[5], "OUT" );
    BOOST_CHECK_EQUAL( pinlabels[6], "IN" );

    // Check pin equivalences (*EQU)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_PinEquivalences.size(), 3 );

    std::vector<std::vector<long>>& pinEqus = result.m_PartEntries[0].m_PinEquivalences;
    BOOST_REQUIRE_EQUAL( pinEqus[0].size(), 2 );
    BOOST_REQUIRE_EQUAL( pinEqus[1].size(), 3 );
    BOOST_REQUIRE_EQUAL( pinEqus[2].size(), 3 );

    BOOST_CHECK_EQUAL( pinEqus[0][0], 4 );
    BOOST_CHECK_EQUAL( pinEqus[0][1], 5 );
    BOOST_CHECK_EQUAL( pinEqus[1][0], 6 );
    BOOST_CHECK_EQUAL( pinEqus[1][1], 7 );
    BOOST_CHECK_EQUAL( pinEqus[1][2], 8 );
    BOOST_CHECK_EQUAL( pinEqus[2][0], 9 );
    BOOST_CHECK_EQUAL( pinEqus[2][1], 10 );
    BOOST_CHECK_EQUAL( pinEqus[2][2], 11 );

    // Check internal swap groups equivalences (*INT)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_InternalSwapGroup.size(), 1 );

    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_InternalSwapGroup[0].m_Name.value(), "Group1" );

    std::vector<std::vector<long>>& intgates =
            result.m_PartEntries[0].m_InternalSwapGroup[0].m_Gates;

    BOOST_REQUIRE_EQUAL( intgates[0].size(), 2 );
    BOOST_REQUIRE_EQUAL( intgates[1].size(), 2 );

    BOOST_CHECK_EQUAL( intgates[0][0], 4 );
    BOOST_CHECK_EQUAL( intgates[0][1], 5 );
    BOOST_CHECK_EQUAL( intgates[1][0], 6 );
    BOOST_CHECK_EQUAL( intgates[1][1], 7 );

    // Check external swap groups equivalences (*EXT)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_ExternalSwapGroup.size(), 1 );

    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_ExternalSwapGroup[0].m_Name.value(), "Group2" );

    std::vector<std::vector<long>>& extgates =
            result.m_PartEntries[0].m_ExternalSwapGroup[0].m_Gates;

    BOOST_REQUIRE_EQUAL( extgates[0].size(), 2 );
    BOOST_REQUIRE_EQUAL( extgates[1].size(), 2 );

    BOOST_CHECK_EQUAL( extgates[0][0], 1 );
    BOOST_CHECK_EQUAL( extgates[0][1], 2 );
    BOOST_CHECK_EQUAL( extgates[1][0], 4 );
    BOOST_CHECK_EQUAL( extgates[1][1], 7 );

    // Check part Definition
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_PartDefinitionName.value(), "<Definition name>" );

    // Check *NGS
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_GateSwappingAllowed, false );

    // Check *NPV
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_PinsVisible, false );

    // Check *STM
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_ComponentStem, "<Component name stem>" );

    // Check *MXP
    KI_CHECK_OPT_EQUAL( result.m_PartEntries[0].m_MaxPinCount, 32 );

    // Check *SPI
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_SpicePartName.value(), "<Part name>" );
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_SpiceModel.value(), "<Model> <Value>" );

    // Check *PAC
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_AcceptancePartName.value(), "<Part name>" );
    BOOST_CHECK_EQUAL( result.m_PartEntries[0].m_AcceptanceText.value(), "<Acceptance Text>" );

    // Check user attributes (* lines)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_UserAttributes.size(), 2 );

    std::map<std::string, std::string>& userAtts = result.m_PartEntries[0].m_UserAttributes;
    BOOST_CHECK_EQUAL( userAtts["userAttribute"], "userAttributeVal" );
    BOOST_CHECK_EQUAL( userAtts["User spaced name"], "userSpacedAttributeVal" );

   // Check SCH attributes ($ lines)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_SchAttributes.size(), 2 );

    std::map<std::string, CADSTAR_ATTRIBUTE_VALUE>& schAtts =
            result.m_PartEntries[0].m_SchAttributes;

    BOOST_CHECK_EQUAL( schAtts["<SCM Attribute name1>"].m_ReadOnly, false );
    BOOST_CHECK_EQUAL( schAtts["<SCM Attribute name1>"].m_Value, "<Attribute value for name1>" );

    BOOST_CHECK_EQUAL( schAtts["<SCM Attribute name2>"].m_ReadOnly, true );
    BOOST_CHECK_EQUAL( schAtts["<SCM Attribute name2>"].m_Value, "<Attribute value for name2>" );

   // Check PCB attributes (% lines)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_PcbAttributes.size(), 2 );

    std::map<std::string, CADSTAR_ATTRIBUTE_VALUE>& pcbAtts =
            result.m_PartEntries[0].m_PcbAttributes;

    BOOST_CHECK_EQUAL( pcbAtts["<PCB Attribute name1>"].m_ReadOnly, false );
    BOOST_CHECK_EQUAL( pcbAtts["<PCB Attribute name1>"].m_Value, "<Attribute value1>" );

    BOOST_CHECK_EQUAL( pcbAtts["<PCB Attribute name2>"].m_ReadOnly, true );
    BOOST_CHECK_EQUAL( pcbAtts["<PCB Attribute name2>"].m_Value, "<Attribute value2>" );

   // Check Part attributes (~ lines)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_PartAttributes.size(), 2 );

    std::map<std::string, CADSTAR_ATTRIBUTE_VALUE>& partAtts =
            result.m_PartEntries[0].m_PartAttributes;

    BOOST_CHECK_EQUAL( partAtts["<Parts Attribute name1>"].m_ReadOnly, false );
    BOOST_CHECK_EQUAL( partAtts["<Parts Attribute name1>"].m_Value, "<Attribute value1>" );

    BOOST_CHECK_EQUAL( partAtts["<Parts Attribute name2>"].m_ReadOnly, true );
    BOOST_CHECK_EQUAL( partAtts["<Parts Attribute name2>"].m_Value, "<Attribute value2>" );

   // Check Combined Sch/PCB attributes (@ lines)
    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_SchAndPcbAttributes.size(), 2 );

    std::map<std::string, CADSTAR_ATTRIBUTE_VALUE>schAndPcbAtts =
            result.m_PartEntries[0].m_SchAndPcbAttributes;

    BOOST_CHECK_EQUAL( schAndPcbAtts["<SCM/PCB Attribute name1>"].m_ReadOnly, false );
    BOOST_CHECK_EQUAL( schAndPcbAtts["<SCM/PCB Attribute name1>"].m_Value, "<Attribute value1>" );

    BOOST_CHECK_EQUAL( schAndPcbAtts["<SCM/PCB Attribute name2>"].m_ReadOnly, true );
    BOOST_CHECK_EQUAL( schAndPcbAtts["<SCM/PCB Attribute name2>"].m_Value, "<Attribute value2>" );

   // Check symbols
    std::vector<CADSTAR_PART_SYMBOL_ENTRY> symbols = result.m_PartEntries[0].m_Symbols;

    std::vector<CADSTAR_PART_SYMBOL_ENTRY> expectedSymbols =
    {
        {
            "<SCM Symbol Refname1>",
            "<SCM Alternate Refname>",
            {
                { 1, CADSTAR_PIN_POSITION::TOP_RIGHT,   CADSTAR_PIN_TYPE::TRISTATE_DRIVER,
                    2000 },
                { 2, CADSTAR_PIN_POSITION::TOP_LEFT,    CADSTAR_PIN_TYPE::TRISTATE_INPUT,
                    std::nullopt },
                { 3, CADSTAR_PIN_POSITION::BOTTOM_LEFT, CADSTAR_PIN_TYPE::TRISTATE_BIDIR,
                    std::nullopt }
            }
        },
        {
            "<SCM Symbol Refname2>",
            std::nullopt,
            {
                { 4, CADSTAR_PIN_POSITION::BOTTOM_LEFT,  CADSTAR_PIN_TYPE::UNCOMMITTED,
                    1000 },
                { 5, CADSTAR_PIN_POSITION::TOP_LEFT,     CADSTAR_PIN_TYPE::PIN_INPUT,
                    std::nullopt },
                { 6, CADSTAR_PIN_POSITION::BOTTOM_RIGHT, CADSTAR_PIN_TYPE::OUTPUT_NOT_NORM_OR,
                    std::nullopt }
            }
        }
    };

    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_Symbols.size(), expectedSymbols.size() );

    auto itA = symbols.begin();
    auto itB = expectedSymbols.begin();

    while( itA != symbols.end() || itB != expectedSymbols.end() )
    {
        BOOST_TEST_CONTEXT( "With symbol = " << itB->m_SymbolName
                                             << " Alternate = "
                                             << itB->m_SymbolAlternateName.value_or( "[nullopt]" ) )
        {
            BOOST_CHECK_EQUAL( itA->m_SymbolName, itB->m_SymbolName );
            KI_CHECK_OPT_EQUAL( itA->m_SymbolAlternateName, itB->m_SymbolAlternateName );

            BOOST_REQUIRE_EQUAL( itA->m_Pins.size(), itB->m_Pins.size() );

            auto itPinsA = itA->m_Pins.begin();
            auto itPinsB = itB->m_Pins.begin();

            while( itPinsA != itA->m_Pins.end() || itPinsB != itB->m_Pins.end() )
            {
                BOOST_TEST_CONTEXT( "Pin Identifier = " << itPinsA->m_Identifier )
                {
                    BOOST_CHECK_EQUAL( itPinsA->m_Identifier, itPinsB->m_Identifier );
                    BOOST_CHECK( itPinsA->m_Position == itPinsB->m_Position );
                    BOOST_CHECK( itPinsA->m_Type == itPinsB->m_Type );
                    KI_CHECK_OPT_EQUAL( itPinsA->m_Loading, itPinsB->m_Loading );
                }

                ++itPinsA;
                ++itPinsB;
            }

            ++itA;
            ++itB;
        }
    }

    // Compare hidden pins
    std::map<std::string,std::vector<CADSTAR_PART_PIN>> expectedHiddenPins =
    {
        {
            "GND",
            {
                { 7, CADSTAR_PIN_POSITION::TOP_RIGHT, CADSTAR_PIN_TYPE::GROUND, 2000 }
            }
        },
        {
            "VCC",
            {
               { 8,  CADSTAR_PIN_POSITION::TOP_RIGHT, CADSTAR_PIN_TYPE::POWER,       2000 },
               { 9,  CADSTAR_PIN_POSITION::TOP_LEFT,  CADSTAR_PIN_TYPE::UNCOMMITTED, std::nullopt },
               { 10, CADSTAR_PIN_POSITION::TOP_RIGHT, CADSTAR_PIN_TYPE::UNCOMMITTED, std::nullopt }
            }
        }
    };

    BOOST_REQUIRE_EQUAL( result.m_PartEntries[0].m_HiddenPins.size(), expectedHiddenPins.size() );

    auto itEntryA = result.m_PartEntries[0].m_HiddenPins.begin();
    auto itEntryB = expectedHiddenPins.begin();

    while( itEntryA != result.m_PartEntries[0].m_HiddenPins.end()
           || itEntryB != expectedHiddenPins.end() )
    {
        BOOST_TEST_CONTEXT( "Check Hidden pins - Signal = " << itEntryB->first )
        {
            BOOST_CHECK_EQUAL( itEntryA->first, itEntryB->first );
            BOOST_REQUIRE_EQUAL( itEntryA->second.size(), itEntryB->second.size() );

            auto itPinsA = itEntryA->second.begin();
            auto itPinsB = itEntryB->second.begin();

            while( itPinsA != itEntryA->second.end() || itPinsB != itEntryB->second.end() )
            {
                BOOST_CHECK_EQUAL( itPinsA->m_Identifier, itPinsB->m_Identifier );
                BOOST_CHECK( itPinsA->m_Position == itPinsB->m_Position );
                BOOST_CHECK( itPinsA->m_Type == itPinsB->m_Type );
                KI_CHECK_OPT_EQUAL( itPinsA->m_Loading, itPinsB->m_Loading );

                ++itPinsA;
                ++itPinsB;
            }
        }

        ++itEntryA;
        ++itEntryB;
    }
}


BOOST_AUTO_TEST_SUITE_END()
