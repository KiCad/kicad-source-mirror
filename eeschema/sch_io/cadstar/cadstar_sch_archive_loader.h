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
 * @file cadstar_sch_archive_loader.h
 * @brief Loads a csa file into a KiCad SCHEMATIC object
 */

#ifndef CADSTAR_SCH_ARCHIVE_LOADER_H_
#define CADSTAR_SCH_ARCHIVE_LOADER_H_

#include <sch_io/cadstar/cadstar_sch_archive_parser.h>

#include <layer_ids.h>          // SCH_LAYER_ID
#include <plotters/plotter.h>   // LINE_STYLE
#include <pin_type.h>           // ELECTRICAL_PINTYPE

#include <memory>
#include <map>

#include <wx/filename.h>
#include <wx/string.h>

struct CADSTAR_PART_ENTRY;
class BUS_ALIAS;
class EDA_TEXT;
class SPIN_STYLE;
class LIB_SYMBOL;
class REPORTER;
class SCH_SYMBOL;
class SCH_ITEM;
class SCH_FIELD;
class SCH_GLOBALLABEL;
class SCH_HIERLABEL;
class SCH_SHEET;
class SCH_SHEET_PATH;
class SCH_TEXT;
class SCHEMATIC;

class CADSTAR_SCH_ARCHIVE_LOADER : public CADSTAR_SCH_ARCHIVE_PARSER
{
public:
    // Size of tiny net labels when none present in original design
    const int SMALL_LABEL_SIZE = KiROUND( (double) SCH_IU_PER_MM * 0.4 );
    const double ARC_ACCURACY = SCH_IU_PER_MM * 0.01; // 0.01mm

    explicit CADSTAR_SCH_ARCHIVE_LOADER( const wxString& aFilename, REPORTER* aReporter,
                                         PROGRESS_REPORTER* aProgressReporter ) :
            CADSTAR_SCH_ARCHIVE_PARSER( aFilename )
    {
        m_schematic = nullptr;
        m_rootSheet = nullptr;
        m_designCenter.x = 0;
        m_designCenter.y = 0;
        m_reporter = aReporter;
        m_progressReporter = aProgressReporter;
        m_fileName = aFilename;

        // Assume that the PCB footprint library name will be the same as the schematic filename
        wxFileName schFilename( Filename );
        m_footprintLibName = schFilename.GetName();
    }


    ~CADSTAR_SCH_ARCHIVE_LOADER()
    {
    }

    std::vector<LIB_SYMBOL*> LoadPartsLib( const wxString& aFilename );

    const std::vector<LIB_SYMBOL*>& GetLoadedSymbols() const { return m_loadedSymbols; }

    void SetFpLibName( const wxString& aLibName ) { m_footprintLibName = aLibName; };


    /**
     * @brief Loads a CADSTAR Schematic Archive file into the KiCad SCHEMATIC object given
     * @param aSchematic Schematic to add the design onto
     * @param aRootSheet Root sheet to add the design onto
     */
    void Load( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet );

    static wxString CreateLibName( const wxFileName& aFileName, const SCH_SHEET* aRootSheet );

private:
    typedef std::pair<BLOCK_ID, TERMINAL_ID> BLOCK_PIN_ID;
    typedef std::pair<PART_ID, GATE_ID> PART_GATE_ID;

    /**
     * Map between a terminal ID in a symbol definition to the pin number that should
     * be imported into KiCad.
     */
    typedef std::map<TERMINAL_ID, wxString> TERMINAL_TO_PINNUM_MAP;

    typedef std::map<wxString, TERMINAL_ID> PINNUM_TO_TERMINAL_MAP;

    REPORTER*  m_reporter;
    SCHEMATIC* m_schematic;
    SCH_SHEET* m_rootSheet;
    wxFileName m_fileName;
    wxString   m_footprintLibName; ///< Name of the footprint library to prepend all footprints with

    /**
     * Required for calculating the offset to apply to the Cadstar design so that it fits
     * in the KiCad canvas
     */
    VECTOR2I m_designCenter;

    std::map<LAYER_ID, SCH_SHEET*>         m_sheetMap;    ///< Cadstar->KiCad Sheets
    std::map<BLOCK_PIN_ID, SCH_HIERLABEL*> m_sheetPinMap; ///< Cadstar->KiCad Sheet Pins
    std::map<PART_ID, LIB_SYMBOL*>         m_partMap;     ///< Cadstar->KiCad Parts
    std::map<SYMBOL_ID, SCH_SYMBOL*>       m_powerSymMap; ///< Cadstar->KiCad Power Symbols
    std::map<wxString, LIB_SYMBOL*>        m_powerSymLibMap;  ///< NetName->KiCad Power Lib Symbol
    std::map<SYMBOL_ID, SCH_GLOBALLABEL*>  m_globalLabelsMap; ///< Cadstar->KiCad Global Labels
    std::map<BUS_ID, std::shared_ptr<BUS_ALIAS>> m_busesMap;  ///< Cadstar->KiCad Buses
    std::map<PART_ID, TERMINAL_TO_PINNUM_MAP> m_pinNumsMap; ///< Cadstar Part->KiCad Pin number map
    std::map<SYMDEF_ID, PINNUM_TO_TERMINAL_MAP> m_symDefTerminalsMap;

    /**
     * Cache storing symbol names and alternates to symdef IDs
     */
    std::map<std::pair<wxString, wxString>, SYMDEF_ID> m_SymDefNamesCache;

    /**
     * Cache storing symbol names (default alternate) to symdef IDs
     */
    std::map<wxString, SYMDEF_ID> m_DefaultSymDefNamesCache;

    /**
     * Cadstar->KiCad Lib Symbols loaded so far. Note that in CADSTAR each symbol represents just a
     * gate, so the LIB_SYMBOLs contained here are not imported directly - they are just an interim
     * step.
     */
    std::map<SYMDEF_ID, std::unique_ptr<const LIB_SYMBOL>> m_symDefMap;

    std::vector<LIB_SYMBOL*> m_loadedSymbols; ///< Loaded symbols so far
    std::map<PART_GATE_ID, SYMDEF_ID> m_partSymbolsMap; ///< Map holding the symbols loaded so far
                                                        ///  for a particular PART_ID and GATE_ID

    void loadSheets();
    void loadHierarchicalSheetPins();
    void loadPartsLibrary();
    void loadSchematicSymbolInstances();
    void loadBusses();
    void loadNets();
    void loadFigures();
    void loadTexts();
    void loadDocumentationSymbols();
    void loadTextVariables();

    std::unique_ptr<LIB_SYMBOL> loadLibPart( const CADSTAR_PART_ENTRY& aPart );

    void copySymbolItems( std::unique_ptr<LIB_SYMBOL>& aSourceSym,
                          std::unique_ptr<LIB_SYMBOL>& aDestSym, int aDestUnit,
                          bool aOverrideFields = true );

    //Helper Functions for loading sheets
    void loadSheetAndChildSheets( const LAYER_ID& aCadstarSheetID, const VECTOR2I& aPosition,
                                  const VECTOR2I& aSheetSize, const SCH_SHEET_PATH& aParentSheet );

    void loadChildSheets( const LAYER_ID& aCadstarSheetID, const SCH_SHEET_PATH& aSheet );

    std::vector<LAYER_ID> findOrphanSheets();

    int getSheetNumber( const LAYER_ID& aCadstarSheetID );

    void loadItemOntoKiCadSheet( const LAYER_ID& aCadstarSheetID, SCH_ITEM* aItem );

    //Helper Functions for loading library items
    const LIB_SYMBOL* loadSymdef( const SYMDEF_ID& aSymdefID );

    void loadSymbolGateAndPartFields( const SYMDEF_ID& aSymdefID, const PART& aCadstarPart,
                                      const GATE_ID& aGateID, LIB_SYMBOL* aSymbol );

    void setFootprintOnSymbol( std::unique_ptr<LIB_SYMBOL>& aKiCadSymbol,
                               const wxString&              aFootprintName,
                               const wxString&              aFootprintAlternate );

    void loadLibrarySymbolShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
                                         const VECTOR2I& aSymbolOrigin, LIB_SYMBOL* aSymbol,
                                         int aGateNumber, int aLineThickness );

    void applyToLibraryFieldAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
                                       const VECTOR2I& aSymbolOrigin, SCH_FIELD* aKiCadField );

    //Helper Functions for loading symbols in schematic
    SCH_SYMBOL* loadSchematicSymbol( const SYMBOL& aCadstarSymbol, const LIB_SYMBOL& aKiCadPart,
                                     EDA_ANGLE& aComponentOrientation );


    void loadSymbolFieldAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
                                   const EDA_ANGLE& aComponentOrientation, bool aIsMirrored,
                                   SCH_FIELD* aKiCadField );

    int getComponentOrientation( const EDA_ANGLE& aOrientAngle, EDA_ANGLE& aReturnedOrientation );

    //Helper functions for loading nets
    POINT getLocationOfNetElement( const NET_SCH& aNet, const NETELEMENT_ID& aNetElementID );

    wxString getNetName( const NET_SCH& aNet );

    //Helper functions for loading figures / graphical items
    void loadShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
                            LINECODE_ID aCadstarLineCodeID, LAYER_ID aCadstarSheetID,
                            SCH_LAYER_ID aKiCadSchLayerID, const VECTOR2I& aMoveVector = { 0, 0 },
                            const EDA_ANGLE& aRotation = ANGLE_0,
                            const double&    aScalingFactor = 1.0,
                            const VECTOR2I&  aTransformCentre = { 0, 0 },
                            const bool&      aMirrorInvert = false );

    void loadFigure( const FIGURE& aCadstarFigure, const LAYER_ID& aCadstarSheetIDOverride,
                     SCH_LAYER_ID aKiCadSchLayerID, const VECTOR2I& aMoveVector = { 0, 0 },
                     const EDA_ANGLE& aRotation = ANGLE_0, const double& aScalingFactor = 1.0,
                     const VECTOR2I& aTransformCentre = { 0, 0 },
                     const bool&     aMirrorInvert = false );

    //Helper functions for loading text elements
    void applyTextCodeIfExists( EDA_TEXT* aKiCadTextItem, const TEXTCODE_ID& aCadstarTextCodeID );

    void applyTextSettings( EDA_TEXT* aKiCadTextItem,
                            const TEXTCODE_ID& aCadstarTextCodeID,
                            const ALIGNMENT&     aCadstarAlignment,
                            const JUSTIFICATION& aCadstarJustification,
                            const long long aCadstarOrientAngle = 0,
                            bool aMirrored = false );

    SCH_TEXT* getKiCadSchText( const TEXT& aCadstarTextElement );


    //Helper Functions for obtaining CADSTAR elements from the parsed structures
    SYMDEF_ID getSymDefFromName( const wxString& aSymdefName, const wxString& aSymDefAlternate );
    bool      isAttributeVisible( const ATTRIBUTE_ID& aCadstarAttributeID );

    int            getLineThickness( const LINECODE_ID& aCadstarLineCodeID );
    LINE_STYLE     getLineStyle( const LINECODE_ID& aCadstarLineCodeID );
    PART           getPart( const PART_ID& aCadstarPartID );
    ROUTECODE      getRouteCode( const ROUTECODE_ID& aCadstarRouteCodeID );
    TEXTCODE       getTextCode( const TEXTCODE_ID& aCadstarTextCodeID );
    int            getTextHeightFromTextCode( const TEXTCODE_ID& aCadstarTextCodeID );
    wxString       getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID );

    PART::DEFINITION::PIN getPartDefinitionPin( const PART& aCadstarPart, const GATE_ID& aGateID,
                                                const TERMINAL_ID& aTerminalID );

    //Helper Functions for obtaining individual elements as KiCad elements:
    ELECTRICAL_PINTYPE getKiCadPinType( const CADSTAR_PIN_TYPE& aPinType );

    int             getKiCadUnitNumberFromGate( const GATE_ID& aCadstarGateID );
    SPIN_STYLE      getSpinStyle( const long long& aCadstarOrientation, bool aMirror );
    SPIN_STYLE      getSpinStyle( const EDA_ANGLE& aOrientation );
    ALIGNMENT       mirrorX( const ALIGNMENT& aCadstarAlignment );
    ALIGNMENT       rotate180( const ALIGNMENT& aCadstarAlignment );

    //General Graphical manipulation functions

    LIB_SYMBOL* getScaledLibPart( const LIB_SYMBOL* aSymbol, long long aScalingFactorNumerator,
                                  long long aScalingFactorDenominator );

    void fixUpLibraryPins( LIB_SYMBOL* aSymbolToFix, int aGateNumber );

    std::pair<VECTOR2I, VECTOR2I> getFigureExtentsKiCad( const FIGURE& aCadstarFigure );

    VECTOR2I getKiCadPoint( const VECTOR2I& aCadstarPoint );

    VECTOR2I getKiCadLibraryPoint( const VECTOR2I& aCadstarPoint, const VECTOR2I& aCadstarCentre );

    VECTOR2I applyTransform( const VECTOR2I& aPoint, const VECTOR2I& aMoveVector = { 0, 0 },
                             const EDA_ANGLE& aRotation = ANGLE_0,
                             const double&    aScalingFactor = 1.0,
                             const VECTOR2I&  aTransformCentre = { 0, 0 },
                             const bool&      aMirrorInvert = false );

    void checkDesignLimits();

    int getKiCadLength( long long aCadstarLength )
    {
        int mod = aCadstarLength % KiCadUnitDivider;
        int absmod = sign( mod ) * mod;
        int offset = 0;

        // Round half-way cases away from zero
        if( absmod >= KiCadUnitDivider / 2 )
            offset = sign( aCadstarLength );

        return ( aCadstarLength / KiCadUnitDivider ) + offset;
    }

    EDA_ANGLE getAngle( const long long& aCadstarAngle )
    {
        // CADSTAR v6 (which outputted Schematic Format Version 8) and earlier used 1/10 degree
        // as the unit for angles/orientations. It is assumed that CADSTAR version 7 (i.e. Schematic
        // Format Version 9 and later) is the version that introduced 1/1000 degree for angles.
        if( Header.Format.Version > 8 )
        {
            return EDA_ANGLE( (double) aCadstarAngle / 1000.0, DEGREES_T );
        }
        else
        {
            return EDA_ANGLE( (double) aCadstarAngle, TENTHS_OF_A_DEGREE_T );
        }
    }

    long long getCadstarAngle( const EDA_ANGLE& aAngle )
    {
        // CADSTAR v6 (which outputted Schematic Format Version 8) and earlier used 1/10 degree
        // as the unit for angles/orientations. It is assumed that CADSTAR version 7 (i.e. Schematic
        // Format Version 9 and later) is the version that introduced 1/1000 degree for angles.
        if( Header.Format.Version > 8 )
        {
            return KiROUND( aAngle.AsDegrees() * 1000.0 );
        }
        else
        {
            return aAngle.AsTenthsOfADegree();
        }
    }

    /**
     * @brief
     * @param aPoint
     * @return Radius of polar representation of the point
     */
    double getPolarRadius( const VECTOR2I& aPoint );


    static SCH_FIELD* addNewFieldToSymbol( const wxString&              aFieldName,
                                           std::unique_ptr<LIB_SYMBOL>& aKiCadSymbol );

}; // CADSTAR_SCH_ARCHIVE_LOADER


#endif // CADSTAR_SCH_ARCHIVE_LOADER_H_
