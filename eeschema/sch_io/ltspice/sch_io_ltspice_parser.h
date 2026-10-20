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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @brief Parses the data structure produced by LTSPICE_SCHEMATIC into a KiCad
 * schematic file.
 */

#ifndef LTSPICE_SCH_PARSER_H
#define LTSPICE_SCH_PARSER_H

#include <core/typeinfo.h>
#include <layer_ids.h>
#include <sch_io/ltspice/ltspice_schematic.h>
#include <stroke_params.h>


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
    // Field for storing LTspice symbol's InstName in SCH_SHEET.
    static constexpr const wxChar* LT_INSTNAME_FIELD = wxS( "LT_InstName" );

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
    int      ToKicadCoords( int aCoordinate );
    VECTOR2I ToKicadCoords( const VECTOR2I& aPos );
    VECTOR2I ToKicadFontSize( int aLTFontSize );

    /**
     * Method converts kicad coordinate(i.e scale) to ltspice coordinates
     *
     * @param aCoordinate coordinate(i.e scale) in ltspice.
     */
    int      ToLtSpiceCoords( int aCoordinate );
    VECTOR2I ToLtSpiceCoords( const VECTOR2I& aPos );

    /**
     * Create KiCad items based on LTspice data.
     *
     * @param aAscfile the LTspice ASC file data.
     * @param aScreen the screen to add the items to.
     * @param aUniqueAscNames the symbol names to import as subsheets instead of symbols.
     * @param aMapOfScreens the map of screens to assign screens to new subsheets.
     * @param aOriginOffset the offset to apply to the items (in KiCad units).
     */
    void CreateItems( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, SCH_SCREEN* aScreen,
                      const std::vector<wxString>& aUniqueAscNames, std::map<wxString, SCH_SCREEN*>& aMapOfScreens,
                      VECTOR2I aOriginOffset );



    /**
     * Create a library-symbol line from LTspice symbol data.
     */
    void CreateSymLine( LIB_SYMBOL* aSymbol, const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                      SCH_SHAPE* aShape );

    /**
     * Create KiCad symbol objects and subsheets from ASC data.
     */
    void CreateKicadSymbolsAndSubsheets( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, SCH_SCREEN* aScreen,
                                         const std::vector<wxString>&     aUniqueAscNames,
                                         std::map<wxString, SCH_SCREEN*>& aMapOfScreens );

    /**
     * Create a library symbol from LTspice symbol data.
     */
    void CreateSymbol( LTSPICE_SCHEMATIC::LT_SYMBOL& aLtSymbol, LIB_SYMBOL* aLibSymbol );

    /**
     * Rotate and mirror an item according to LTspice symbol flags.
     */
    void RotateMirrorItem( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, SCH_ITEM* aItem );

    /**
     * Rotate and mirror a point according to LTspice symbol flags.
     */
    VECTOR2I RotateMirror( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, const VECTOR2I& aPoint );

    /**
     * Set KiCad symbol orientation based on LTspice symbol orientation.
     */
    void AssignSymOrientation( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, SCH_SYMBOL* aSchSymbol );

    /**
     * Create schematic items from ASC data.
     */
    void CreateKicadSCH_ITEMs( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, SCH_SCREEN* aScreen );

    /**
     * Create a schematic bus tap from ASC data.
     */
    void AddAscBusEntry( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SCREEN* aScreen );

    /**
     * Create a schematic I/O pin from ASC data.
     */
    void AddAscIopin( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SCREEN* aScreen );

    /**
     * Create a schematic line from ASC data.
     */
    void AddAscLine( const LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SCREEN* aScreen );

    /**
     * Create a schematic circle from ASC data.
     */
    void AddAscCircle( const LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SCREEN* aScreen );

    /**
     * Create a schematic arc from ASC data.
     */
    void AddAscArc( const LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SCREEN* aScreen );

    /**
     * Create a schematic rectangle from ASC data.
     */
    void AddAscRect( const LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SCREEN* aScreen );

    SCH_SHEET* AddAscSymAsSubsheet( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol,
                               SCH_SCREEN* aScreen );

    /**
     * Create schematic text.
     */
    SCH_TEXT* CreateSCH_TEXT( const VECTOR2I& aOffset, const wxString& aText, int aFontSize,
                           LTSPICE_SCHEMATIC::JUSTIFICATION aJustification );

    /**
     * Create a schematic wire from ASC data.
     */
    void AddAscWire( const LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex, SCH_SCREEN* aScreen,
                     SCH_LAYER_ID aLayer );

    /**
     * Create a power symbol for the schematic.
     */
    SCH_SYMBOL* CreatePowerSymbol( const VECTOR2I& aOffset, const wxString& aValue, int aFontSize,
                                   SCH_SCREEN* aScreen,
                                   std::vector<LTSPICE_SCHEMATIC::WIRE>& aWires );

    /**
     * Create a schematic label.
     */
    SCH_LABEL_BASE* CreateSCH_LABEL( KICAD_T aType, const VECTOR2I& aOffset, const wxString& aValue, int aFontSize,
                                     SCH_SCREEN* aScreen );

    /**
     * Create and configure symbol fields from LTspice symbol data.
     */
    void AddSymFields( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, SCH_SYMBOL* aSymbol );

    /**
     * Create a symbol rectangle from LTspice symbol data.
     */
    void CreateSymRect( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHAPE* aRectangle );

    /**
     * Create a symbol pin from LTspice symbol data.
     */
    void CreateSymPin( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_PIN* aPin );

    /**
     * Create a symbol arc from LTspice symbol data.
     */
    void CreateSymArc( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHAPE* aArc );

    /**
     * Create a symbol circle from LTspice symbol data.
     */
    void CreateSymCircle( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_SHAPE* aCircle );

    /**
     * Create a symbol text from LTspice symbol data.
     */
    void CreateSymText( const LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex, SCH_TEXT* aText );

private:
    int getLineWidth( const LTSPICE_SCHEMATIC::LINEWIDTH& aLineWidth );

    LINE_STYLE getLineStyle( const LTSPICE_SCHEMATIC::LINESTYLE& aLineStyle );

    STROKE_PARAMS getStroke( const LTSPICE_SCHEMATIC::LINEWIDTH& aLineWidth,
                             const LTSPICE_SCHEMATIC::LINESTYLE& aLineStyle );

    void setTextJustification( EDA_TEXT* aText, LTSPICE_SCHEMATIC::JUSTIFICATION aJustification );

private:
    LTSPICE_SCHEMATIC* m_lt_schematic;
    VECTOR2I           m_originOffset;
    int                m_powerSymbolIndex;
};
#endif // LTSPICE_SCH_PARSER_H
