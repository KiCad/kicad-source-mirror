/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

/**
 * @file cadstar_pcb_archive_parser.cpp
 * @brief Reads in a CADSTAR Schematic Archive (*.csa) file
 */

#ifndef CADSTAR_SCH_ARCHIVE_PARSER_H_
#define CADSTAR_SCH_ARCHIVE_PARSER_H_

#include <io/cadstar/cadstar_archive_parser.h>


/**
 * @brief Represents a CADSTAR Schematic Archive (*.csa) file
 */
class CADSTAR_SCH_ARCHIVE_PARSER : public CADSTAR_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_SCH_ARCHIVE_PARSER( const wxString& aFilename ) :
            CADSTAR_ARCHIVE_PARSER(),
            Filename( aFilename ),
            Header(),
            Assignments(),
            KiCadUnitDivider( 10 ),
            m_rootNode( nullptr )
    {
    }

    virtual ~CADSTAR_SCH_ARCHIVE_PARSER()
    {
        delete m_rootNode;
    }

    /**
     * @brief Parses the file
     * @throw IO_ERROR if file could not be opened or there was
     * an error while parsing
     */
    void Parse();


    typedef wxString TERMINALCODE_ID;
    typedef wxString SYMBOL_ID;
    typedef wxString BUS_ID;
    typedef wxString BLOCK_ID;
    typedef wxString SHEET_NAME;


    enum class TERMINAL_SHAPE_TYPE
    {
        ANNULUS,
        BOX,
        BULLET,
        CIRCLE, ///< Keyword "ROUND"
        CROSS,
        DIAMOND,
        FINGER,
        OCTAGON,
        PLUS,
        POINTER,
        RECTANGLE,
        ROUNDED_RECT, ///< Keyword "ROUNDED"
        SQUARE,
        STAR,
        TRIANGLE,
        UNDEFINED ///< Only used for error checking (not a real shape)
    };


    static TERMINAL_SHAPE_TYPE ParseTermShapeType( const wxString& aShapeStr );


    struct TERMINAL_SHAPE : PARSER
    {
        TERMINAL_SHAPE_TYPE ShapeType;
        long                Size            = UNDEFINED_VALUE;
        // Note in the CADSTAR GUI, it only talks about "length", but the file seems to
        // split it in "left length" and "right length" (similar to PADCODE in the PCB)
        // for some terminal shapes such as RECTANGLE but not for others, such as TRIANGLE
        long                LeftLength      = UNDEFINED_VALUE; ///< Might also be total length
        long                RightLength     = UNDEFINED_VALUE; ///< Could be blank
        long                InternalFeature = UNDEFINED_VALUE;
        long                OrientAngle     = 0; ///< 1/1000 of a Degree

        static bool IsTermShape( XNODE* aNode );
        void        Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TERMINALCODE : PARSER
    {
        TERMINALCODE_ID ID;
        wxString        Name;
        TERMINAL_SHAPE  Shape;
        bool            Filled = false;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct CODEDEFS_SCM : CADSTAR_ARCHIVE_PARSER::CODEDEFS
    {
        std::map<TERMINALCODE_ID, TERMINALCODE> TerminalCodes;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct ASSIGNMENTS_SCM : PARSER
    {
        CODEDEFS_SCM Codedefs;
        GRIDS        Grids;
        SETTINGS     Settings;
        bool         NetclassEditAttributeSettings     = false; //< Unclear what this does
        bool         SpacingclassEditAttributeSettings = false; //< Unclear what this does

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TERMINAL : PARSER
    {
        TERMINAL_ID     ID;
        TERMINALCODE_ID TerminalCodeID;
        POINT           Position; ///< Pad position within the component's coordinate frame.
        long            OrientAngle = 0;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct PIN_NUM_LABEL_LOC : CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION
    {
        TERMINAL_ID TerminalID;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SYMDEF_SCM : CADSTAR_ARCHIVE_PARSER::SYMDEF
    {
        std::map<TERMINAL_ID, TERMINAL>          Terminals;
        std::map<TERMINAL_ID, PIN_NUM_LABEL_LOC> PinLabelLocations;
        std::map<TERMINAL_ID, PIN_NUM_LABEL_LOC> PinNumberLocations;


        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct LIBRARY_SCM : PARSER
    {
        std::map<SYMDEF_ID, SYMDEF_SCM> SymbolDefinitions;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SHEETS : PARSER
    {
        std::map<LAYER_ID, SHEET_NAME> SheetNames;
        std::vector<LAYER_ID>          SheetOrder; ///< A vector to also store the order in which
                                                   ///< sheets are to be displayed

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct COMP : PARSER
    {
        wxString           Designator  = wxEmptyString;
        bool               ReadOnly    = false;
        bool               HasLocation = false;
        ATTRIBUTE_LOCATION AttrLoc;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct PARTREF : PARSER
    {
        PART_ID            RefID       = wxEmptyString;
        bool               ReadOnly    = false;
        bool               HasLocation = false;
        ATTRIBUTE_LOCATION AttrLoc;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TERMATTR : PARSER
    {
        TERMINAL_ID                  TerminalID;
        std::vector<ATTRIBUTE_VALUE> Attributes;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SYMPINNAME_LABEL : PARSER
    {
        TERMINAL_ID        TerminalID;
        wxString           NameOrLabel;
        bool               HasLocation = false;
        ATTRIBUTE_LOCATION AttrLoc;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SYMBOLVARIANT : PARSER
    {
        enum class TYPE
        {
            GLOBALSIGNAL,
            SIGNALREF,
            TESTPOINT
            //TODO: there might be others
        };

        TYPE     Type;
        wxString Reference = wxEmptyString;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SIGNALREFERENCELINK : CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION
    {
        wxString Text; ///< This contains the numbers of the other sheets where the
                       ///< signal reference is present separated by commas

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SYMBOL : PARSER
    {
        struct PIN_NUM : PARSER
        {
            TERMINAL_ID        TerminalID;
            long               PinNum;
            bool               HasLocation = false;
            ATTRIBUTE_LOCATION AttrLoc;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        SYMBOL_ID     ID;
        SYMDEF_ID     SymdefID;
        LAYER_ID      LayerID; ///< Sheet on which symbol is located
        POINT         Origin;
        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this symbol is part of a group
        REUSEBLOCKREF ReuseBlockRef;
        long          OrientAngle = 0;
        bool          Mirror      = false;
        bool          Fixed       = false;
        long          ScaleRatioNumerator = 1; ///< Symbols can be arbitrarily scaled in CADSTAR
        long          ScaleRatioDenominator = 1;
        READABILITY   Readability = READABILITY::BOTTOM_TO_TOP;

        bool    IsComponent = false;
        COMP    ComponentRef;
        bool    HasPartRef = false;
        PARTREF PartRef;
        bool    PartNameVisible = true;
        GATE_ID GateID; ///< The gate this symbol represents within the associated Part

        bool                IsSymbolVariant = false;
        SYMBOLVARIANT       SymbolVariant;
        SIGNALREFERENCELINK SigRefLink; ///< Signal References (a special form of global signal)
                                        ///< have annotations showing the location of all the
                                        ///< other sheets where the signal is present

        SYMBOL_ID  VariantParentSymbolID = wxEmptyString;
        VARIANT_ID VariantID             = wxEmptyString;

        std::map<TERMINAL_ID, TERMATTR>         TerminalAttributes;
        std::map<TERMINAL_ID, SYMPINNAME_LABEL> PinLabels;  ///< Equivalent to KiCad's Pin Name
        std::map<TERMINAL_ID, SYMPINNAME_LABEL> PinNames;   ///< Identifier of the pin in the PCB
                                                            ///< Equivalent to KiCad's Pin Number
        std::map<TERMINAL_ID, PIN_NUM>          PinNumbers; ///< This seems to only appear in older
                                                            ///< designs and is similar to PinNames
                                                            ///< but only allowing numerical values
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    /**
     * @brief Net name or bus name label
     */
    struct SIGLOC : CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION
    {
        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct BUS : PARSER
    {
        BUS_ID      ID;
        LINECODE_ID LineCodeID;
        LAYER_ID    LayerID; ///< Sheet on which bus is located
        SHAPE       Shape;
        wxString    Name        = wxEmptyString;
        bool        HasBusLabel = false;
        SIGLOC      BusLabel;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct BLOCK : PARSER
    {
        enum class TYPE
        {
            CLONE, ///< the block is referring to the sheet it is on.
            PARENT,
            CHILD
        };

        BLOCK_ID ID;
        TYPE     Type; ///< Determines what the associated layer is, whether parent, child or clone
        LAYER_ID LayerID = wxEmptyString; ///< The sheet block is on (TODO: verify this is true)
        LAYER_ID AssocLayerID  = wxEmptyString; ///< Parent or Child linked sheet
        wxString Name          = wxEmptyString;
        bool     HasBlockLabel = false;
        ATTRIBUTE_LOCATION BlockLabel;

        std::map<TERMINAL_ID, TERMINAL> Terminals;
        std::map<FIGURE_ID, FIGURE>     Figures;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct NET_SCH : CADSTAR_ARCHIVE_PARSER::NET
    {
        struct JUNCTION_SCH : CADSTAR_ARCHIVE_PARSER::NET::JUNCTION ///< "JPT" nodename.
        {
            TERMINALCODE_ID TerminalCodeID; ///< Usually a circle, but size can be varied
            bool            HasNetLabel = false;
            SIGLOC          NetLabel;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct SYM_TERM : PARSER ///< "TERM" nodename (represents a pin in a SCH symbol)
        {
            NETELEMENT_ID ID; ///< First character is "P"
            SYMBOL_ID     SymbolID;
            TERMINAL_ID   TerminalID;
            bool          HasNetLabel = false;
            SIGLOC        NetLabel;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct BUS_TERM : PARSER ///< "BUSTERM" nodename (represents a connection to a bus)
        {
            NETELEMENT_ID ID; ///< First two characters "BT"
            BUS_ID        BusID;
            POINT         FirstPoint;  ///< Point on the bus itself
            POINT         SecondPoint; ///< Start point for any wires
            bool          HasNetLabel = false;
            SIGLOC        NetLabel;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct BLOCK_TERM : PARSER ///< "BLOCKTERM" nodename (represents a connection to a block)
        {
            NETELEMENT_ID ID; ///< First four characters "BLKT"
            BLOCK_ID      BlockID;
            TERMINAL_ID   TerminalID;
            bool          HasNetLabel = false;
            SIGLOC        NetLabel;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };


        struct DANGLER : PARSER ///< "DANGLER" nodename (represents a dangling wire)
        {
            NETELEMENT_ID   ID; ///< First character "D"
            TERMINALCODE_ID TerminalCodeID;
            LAYER_ID        LayerID;
            POINT           Position;
            bool            HasNetLabel = false;
            SIGLOC          NetLabel;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct CONNECTION_SCH : CADSTAR_ARCHIVE_PARSER::NET::CONNECTION ///< "CONN" nodename
        {
            LAYER_ID           LayerID; ///< Sheet on which the connection is drawn
            std::vector<POINT> Path;
            GROUP_ID           GroupID = wxEmptyString;
            REUSEBLOCKREF      ReuseBlockRef;
            LINECODE_ID        ConnectionLineCode;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        std::map<NETELEMENT_ID, JUNCTION_SCH> Junctions;
        std::map<NETELEMENT_ID, SYM_TERM>     Terminals;
        std::map<NETELEMENT_ID, BUS_TERM>     BusTerminals;
        std::map<NETELEMENT_ID, BLOCK_TERM>   BlockTerminals;
        std::map<NETELEMENT_ID, DANGLER>      Danglers;
        std::vector<CONNECTION_SCH>           Connections;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct CADSTAR_SCHEMATIC : PARSER
    {
        std::map<GROUP_ID, GROUP>                               Groups;
        std::map<REUSEBLOCK_ID, REUSEBLOCK>                     ReuseBlocks;
        std::map<FIGURE_ID, FIGURE>                             Figures;
        std::map<SYMBOL_ID, SYMBOL>                             Symbols;
        std::map<BUS_ID, BUS>                                   Buses;
        std::map<BLOCK_ID, BLOCK>                               Blocks;
        std::map<NET_ID, NET_SCH>                               Nets;
        std::map<TEXT_ID, TEXT>                                 Texts;
        std::map<DOCUMENTATION_SYMBOL_ID, DOCUMENTATION_SYMBOL> DocumentationSymbols;
        VARIANT_HIERARCHY                                       VariantHierarchy;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>                 AttributeValues;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    wxString          Filename;
    HEADER            Header;
    ASSIGNMENTS_SCM   Assignments;
    LIBRARY_SCM       Library;
    PARTS             Parts;
    SHEETS            Sheets;
    CADSTAR_SCHEMATIC Schematic;
    ATTRCOLORS        AttrColors;
    PARTNAMECOL       SymbolPartNameColor;

    int KiCadUnitDivider; ///<Use this value to convert units in this CSA file to KiCad units

private:
    XNODE* m_rootNode; // Currently parsed root node

}; //CADSTAR_SCH_ARCHIVE_PARSER

#endif // CADSTAR_SCH_ARCHIVE_PARSER_H_
