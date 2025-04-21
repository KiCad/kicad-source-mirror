/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Chetan Subhash Shinde<chetanshinde2001@gmail.com>
 * Copyright (C) 2023 CERN
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
 * @brief Parses the datastructure produced by the LTSPICE_SCHEMATIC into a KiCad
 * schematic file.
 */

#ifndef LTSPICE_SCH_PARSER_H
#define LTSPICE_SCH_PARSER_H

#include <sch_io/sch_io_mgr.h>
#include <pin_type.h>
#include <core/typeinfo.h>
#include <layer_ids.h>
#include <plotters/plotter.h>
#include <sch_io/ltspice/ltspice_schematic.h>


class EDA_TEXT;
class SCH_PIN;
class SCH_LABEL_BASE;
class SCH_SYMBOL;
class SCH_TEXT;
class SCH_SHAPE;

/**
 * The class is been used for loading the asc and asy files in intermediate data structure.
 * <p>
 * The intermediate data structure is a structure, from where the kicad objects will be
 * initialised.
 * <p>
 * The intermediate datastructure is been defined below in the class, with comments for
 * individual struct is in place.
 * <p>
 * parser() - This is the main function responsible for parsing the data into intermediate
 * datastructure.
 * <p>
 * Data Processing Methods - The individual comments for function are given. In general.
 * these methods are been used for manipulating the data.
 * <p>
 * Data Check Methods - The individual comments for functions under this section is given.
 * In general these methods are been used for validating the data.
 * <p>
 * Enum Conversion Methods - The individual comments for functions under this section is
 * given. In general these methods are used to convert the string into enum.
 */
class SCH_IO_LTSPICE_PARSER
{
public:
    explicit SCH_IO_LTSPICE_PARSER( LTSPICE_SCHEMATIC* aLTSchematic ) :
            m_lt_schematic( aLTSchematic ),
            m_powerSymbolIndex( 0 )
    { }

    ~SCH_IO_LTSPICE_PARSER() {}

    /**
     * Method converts ltspice coordinate(i.e scale) to kicad coordinates
     *
     * @param aCoordinate coordinate(i.e scale) in ltspice.
     */
    int ToKicadCoords( int aCoordinate );
    VECTOR2I ToKicadCoords( const VECTOR2I& aPos );

    VECTOR2I ToKicadFontSize( int aLTFontSize );

    /**
     * Method converts kicad coordinate(i.e scale) to ltspice coordinates
     *
     * @param aCoordinate coordinate(i.e scale) in ltspice.
     */
    int ToLtSpiceCoords( int aCoordinate );

    /**
     * Function responsible for loading the .asc and .asy files in intermediate data structure.
     *
     * @param aSheet variable representing the current sheet.
     * @param mapOfFiles a string map containing all the data from .asc and .asy files.
     */
    void Parse( SCH_SHEET_PATH* aSheet, std::vector<LTSPICE_SCHEMATIC::LT_ASC>& outLT_ASCs,
                const std::vector<wxString>& aAsyFileNames );


    /**
     * Method for plotting Lines from Asy files
     *
     * @param aAscfile object representing asc file.
     * @param aIndex index.
     */
    void CreateLines( LIB_SYMBOL* aSymbol, LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                      SCH_SHAPE* aShape );

    /**
     * Method for plotting Schematic Lines from Asy files
     *
     * @param aAscfile object representing asc file.
     * @param aIndex index.
     * @param aSheet a object on which the circle is represented.
     */
    void CreateLines( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Main Method for loading indermediate data to kicacd object from asy files
     *
     * @param aAscfiles array object representing asc files.
     * @param aSheet a object on which the symbols are represented.
     */
    void CreateKicadSYMBOLs( SCH_SHEET_PATH* aSheet,
                             std::vector<LTSPICE_SCHEMATIC::LT_ASC>& outLT_ASCs,
                             const std::vector<wxString>& aAsyFiles );

    void CreateSymbol( LTSPICE_SCHEMATIC::LT_SYMBOL& aLtSymbol, LIB_SYMBOL* aLibSymbol );

    /**
     * Methods for rotating and mirroring objects
     */
    void RotateMirror( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, SCH_SYMBOL* aSchSymbol );
    void RotateMirrorShape( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, SCH_SHAPE* aShape );

    /**
     * Method for plotting wires
     *
     * @param aLTSymbol object representing asc file.
     * @param aIndex index.
     * @param aSheet a object on which the symbols is represented.
     */
    void CreateWires( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                      SCH_SHEET_PATH* aSheet );

    /**
     * Main method for loading intermediate data structure from Asc file to kicad.
     *
     * @param outLT_ASCs
     * @param aSheet a object on which the circle is represented.
     */
    void CreateKicadSCH_ITEMs( SCH_SHEET_PATH* aSheet,
                               std::vector<LTSPICE_SCHEMATIC::LT_ASC>& outLT_ASCs );

    /**
     * Method for plotting Bustap from Asc files
     *
     * @param aAscfile object representing asc file.
     * @param aIndex index.
     * @param aSheet a object on which the circle is represented.
     */
    void CreateBusEntry( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Method for plotting Iopin from Asc files
     *
     * @param aAscfile object representing asc file.
     * @param aIndex index.
     * @param aSheet a object on which the circle is represented.
     */
    void CreatePin( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Method for plotting Line from Asc files
     *
     * @param aAscfile object representing asc file.
     * @param aIndex index.
     * @param aSheet a object on which the circle is represented.
     */
    void CreateLine( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Method for plotting circle from Asc files
     *
     * @param aAscfile object representing asc file.
     * @param aIndex index.
     * @param aSheet a object on which the circle is represented.
     */
    void CreateCircle( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Method for plotting Arc from Asc files
     *
     * @param aAscfile object representing asc file.
     * @param aIndex index.
     * @param aSheet a object on which the Arc is represented.
     */
    void CreateArc( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Method for plotting rectangle from Asc files
     *
     * @param aAscfile object representing asc file.
     * @param aIndex index.
     * @param aSheet a object on which the rectangle is represented.
     */
    void CreateRect( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Create schematic text.
     */
    SCH_TEXT* CreateSCH_TEXT( const VECTOR2I& aOffset, const wxString& aText, int aFontSize,
                          LTSPICE_SCHEMATIC::JUSTIFICATION aJustification );

    /**
     * Create a schematic wire.
     */
    void CreateWire( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SHEET_PATH* aSheet,
                     SCH_LAYER_ID aLayer );

    /**
     * Create a power symbol.
     * @param aWires Schematic wires.  (Allows us to correctly orient the power symbol.)
     */
    SCH_SYMBOL* CreatePowerSymbol( const VECTOR2I& aOffset, const wxString& aValue, int aFontSize,
                                   SCH_SHEET_PATH* aSheet,
                                   std::vector<LTSPICE_SCHEMATIC::WIRE>& aWires );

    /**
     * Create a label.
     * @param aType Currently supported types: SCH_LABEL_T, SCH_DIRECTIVE_LABEL_T
     */
    SCH_LABEL_BASE* CreateSCH_LABEL( KICAD_T aType, const VECTOR2I& aOffset, const wxString& aValue,
                                     int aFontSize, std::vector<LTSPICE_SCHEMATIC::WIRE>& aWires );

    void CreateFields( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, SCH_SYMBOL* aSymbol,
                       SCH_SHEET_PATH* aSheet );

    /**
     * Create a symbol rect.
     */
    void CreateRect( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHAPE* aRectangle );

    /**
     * Create a schematic rect.
     */
    void CreateRect( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Create a pin from an asy file.
     */
    void CreatePin( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_PIN* aPin );

    /**
     * Create a symbol arc.
     */
    void CreateArc( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHAPE* aArc );

    /**
     * Create a schematic arc.
     */
    void CreateArc( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHEET_PATH* aSheet );

    /**
     * Create a symbol circle.
     */
    void CreateCircle( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHAPE* aCircle );

    /**
     * Create a schematic circle.
     */
    void CreateCircle( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHEET_PATH* aSheet );

private:
    int getLineWidth( const LTSPICE_SCHEMATIC::LINEWIDTH& aLineWidth );

    LINE_STYLE getLineStyle( const LTSPICE_SCHEMATIC::LINESTYLE& aLineStyle );

    STROKE_PARAMS getStroke( const LTSPICE_SCHEMATIC::LINEWIDTH& aLineWidth,
                             const LTSPICE_SCHEMATIC::LINESTYLE& aLineStyle );

    void setTextJustification( EDA_TEXT* aText, LTSPICE_SCHEMATIC::JUSTIFICATION aJustification );

    void readIncludes( std::vector<LTSPICE_SCHEMATIC::LT_ASC>& outLT_ASCs );

private:
    LTSPICE_SCHEMATIC*           m_lt_schematic;
    VECTOR2I                     m_originOffset;
    std::map<wxString, wxString> m_includes;
    int                          m_powerSymbolIndex;
};
#endif // LTSPICE_SCH_PARSER_H
