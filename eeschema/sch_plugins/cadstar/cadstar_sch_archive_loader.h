/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_plugins/cadstar/cadstar_sch_archive_parser.h>

#include <layers_id_colors_and_visibility.h> // SCH_LAYER_ID
#include <plotter.h>                         // PLOT_DASH_TYPE
#include <pin_type.h>                        // ELECTRICAL_PINTYPE
#include <sch_io_mgr.h>
#include <wx/filename.h>

class BUS_ALIAS;
class EDA_TEXT;
class LABEL_SPIN_STYLE;
class LIB_FIELD;
class LIB_SYMBOL;
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
    explicit CADSTAR_SCH_ARCHIVE_LOADER( wxString aFilename, REPORTER* aReporter )
            : CADSTAR_SCH_ARCHIVE_PARSER( aFilename )
    {
        m_schematic      = nullptr;
        m_rootSheet      = nullptr;
        m_plugin         = nullptr;
        m_designCenter.x = 0;
        m_designCenter.y = 0;
        m_reporter       = aReporter;
    }


    ~CADSTAR_SCH_ARCHIVE_LOADER()
    {
    }

    /**
     * @brief Loads a CADSTAR PCB Archive file into the KiCad BOARD object given
     * @param aSchematic Schematic to add the design onto
     * @param aRootSheet Root sheet to add the design onto
     */
    void Load( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
            SCH_PLUGIN::SCH_PLUGIN_RELEASER* aSchPlugin, const wxFileName& aLibraryFileName );


private:
    typedef std::pair<BLOCK_ID, TERMINAL_ID> BLOCK_PIN_ID;
    typedef std::pair<PART_ID, GATE_ID> PART_GATE_ID;

    /**
     * Map between a terminal ID in a symbol definition to the pin number that should
     * be imported into KiCad.
     */
    typedef std::map<TERMINAL_ID, wxString>  TERMINAL_TO_PINNUM_MAP;

    REPORTER*                        m_reporter;
    SCHEMATIC*                       m_schematic;
    SCH_SHEET*                       m_rootSheet;
    SCH_PLUGIN::SCH_PLUGIN_RELEASER* m_plugin;
    wxFileName                       m_libraryFileName;
    wxPoint                          m_designCenter; ///< Used for calculating the required
                                                     ///< offset to apply to the Cadstar design
                                                     ///< so that it fits in KiCad canvas
    std::map<LAYER_ID, SCH_SHEET*> m_sheetMap;       ///< Map between Cadstar and KiCad Sheets
    std::map<BLOCK_PIN_ID, SCH_HIERLABEL*>
                                 m_sheetPinMap; ///< Map between Cadstar and KiCad Sheets Pins
    std::map<PART_ID, LIB_SYMBOL*> m_partMap;     ///< Map between Cadstar and KiCad Parts
    std::map<PART_GATE_ID, SYMDEF_ID> m_partSymbolsMap; ///< Map holding the symbols loaded so far
                                                        ///  for a particular PART_ID and GATE_ID
    std::map<PART_ID, TERMINAL_TO_PINNUM_MAP> m_pinNumsMap; ///< Map of pin numbers in CADSTAR parts
    std::map<wxString, LIB_SYMBOL*> m_powerSymLibMap; ///< Map of KiCad Power Symbol Library items
    std::map<SYMBOL_ID, SCH_SYMBOL*>
            m_powerSymMap; ///< Map between Cadstar and KiCad Power Symbols
    std::map<SYMBOL_ID, SCH_GLOBALLABEL*>
            m_globalLabelsMap; ///< Map between Cadstar and KiCad Global Labels
    std::map<BUS_ID, std::shared_ptr<BUS_ALIAS>> m_busesMap; ///< Map of Cadstar and KiCad Buses

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

    //Helper Functions for loading sheets
    void loadSheetAndChildSheets( LAYER_ID aCadstarSheetID, const wxPoint& aPosition,
                                  wxSize aSheetSize, const SCH_SHEET_PATH& aParentSheet );

    void loadChildSheets( LAYER_ID aCadstarSheetID, const SCH_SHEET_PATH& aSheet );

    std::vector<LAYER_ID> findOrphanSheets();

    int getSheetNumber( LAYER_ID aCadstarSheetID );

    void loadItemOntoKiCadSheet( LAYER_ID aCadstarSheetID, SCH_ITEM* aItem );

    //Helper Functions for loading library items
    void loadSymDefIntoLibrary( const SYMDEF_ID& aSymdefID, const PART* aCadstarPart,
            const GATE_ID& aGateID, LIB_SYMBOL* aSymbol );

    void loadLibrarySymbolShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
            wxPoint aSymbolOrigin, LIB_SYMBOL* aSymbol, int aGateNumber );

    void applyToLibraryFieldAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
            wxPoint aSymbolOrigin, LIB_FIELD* aKiCadField );

    //Helper Functions for loading symbols in schematic
    SCH_SYMBOL* loadSchematicSymbol( const SYMBOL& aCadstarSymbol, const LIB_SYMBOL& aKiCadPart,
            double& aComponentOrientationDeciDeg );

    void loadSymbolFieldAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
                                   const double& aComponentOrientationDeciDeg, bool aIsMirrored,
                                   SCH_FIELD* aKiCadField );

    int getComponentOrientation( double aOrientAngleDeciDeg, double& aReturnedOrientationDeciDeg );

    //Helper functions for loading nets
    POINT getLocationOfNetElement( const NET_SCH& aNet, const NETELEMENT_ID& aNetElementID );

    wxString getNetName( const NET_SCH& aNet );

    //Helper functions for loading figures / graphical items
    void loadGraphicStaightSegment( const wxPoint& aStartPoint, const wxPoint& aEndPoint,
            const LINECODE_ID& aCadstarLineCodeID, const LAYER_ID& aCadstarSheetID,
            const SCH_LAYER_ID& aKiCadSchLayerID, const wxPoint& aMoveVector = { 0, 0 },
            const double& aRotationAngleDeciDeg = 0.0, const double& aScalingFactor = 1.0,
            const wxPoint& aTransformCentre = { 0, 0 }, const bool& aMirrorInvert = false );

    void loadShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
            LINECODE_ID aCadstarLineCodeID, LAYER_ID aCadstarSheetID, SCH_LAYER_ID aKiCadSchLayerID,
            const wxPoint& aMoveVector = { 0, 0 }, const double& aRotationAngleDeciDeg = 0.0,
            const double& aScalingFactor = 1.0, const wxPoint& aTransformCentre = { 0, 0 },
            const bool& aMirrorInvert = false );

    void loadFigure( const FIGURE& aCadstarFigure, const LAYER_ID& aCadstarSheetIDOverride,
            SCH_LAYER_ID aKiCadSchLayerID, const wxPoint& aMoveVector = { 0, 0 },
            const double& aRotationAngleDeciDeg = 0.0, const double& aScalingFactor = 1.0,
            const wxPoint& aTransformCentre = { 0, 0 }, const bool& aMirrorInvert = false );

    //Helper functions for loading text elements
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
    PLOT_DASH_TYPE getLineStyle( const LINECODE_ID& aCadstarLineCodeID );
    PART           getPart( const PART_ID& aCadstarPartID );
    ROUTECODE      getRouteCode( const ROUTECODE_ID& aCadstarRouteCodeID );
    TEXTCODE       getTextCode( const TEXTCODE_ID& aCadstarTextCodeID );
    wxString       getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID );

    PART::DEFINITION::PIN getPartDefinitionPin(
            const PART& aCadstarPart, const GATE_ID& aGateID, const TERMINAL_ID& aTerminalID );

    //Helper Functions for obtaining individual elements as KiCad elements:
    ELECTRICAL_PINTYPE getKiCadPinType( const PART::PIN_TYPE& aPinType );

    int              getKiCadUnitNumberFromGate( const GATE_ID& aCadstarGateID );
    LABEL_SPIN_STYLE getSpinStyle( const long long& aCadstarOrientation, bool aMirror );
    LABEL_SPIN_STYLE getSpinStyleDeciDeg( const double& aOrientationDeciDeg );
    ALIGNMENT        mirrorX( const ALIGNMENT& aCadstarAlignment );
    ALIGNMENT        rotate180( const ALIGNMENT& aCadstarAlignment );

    //General Graphical manipulation functions

    LIB_SYMBOL* getScaledLibPart( const LIB_SYMBOL* aSymbol, long long aScalingFactorNumerator,
                                  long long aScalingFactorDenominator );

    void fixUpLibraryPins( LIB_SYMBOL* aSymbolToFix, int aGateNumber );

    std::pair<wxPoint, wxSize> getFigureExtentsKiCad( const FIGURE& aCadstarFigure );

    wxPoint getKiCadPoint( const wxPoint& aCadstarPoint );

    wxPoint getKiCadLibraryPoint( const wxPoint& aCadstarPoint, const wxPoint& aCadstarCentre );

    wxPoint applyTransform( const wxPoint& aPoint, const wxPoint& aMoveVector = { 0, 0 },
            const double& aRotationAngleDeciDeg = 0.0, const double& aScalingFactor = 1.0,
            const wxPoint& aTransformCentre = { 0, 0 }, const bool& aMirrorInvert = false );

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

    /**
     * @brief
     * @param aCadstarAngle
     * @return
    */
    double getAngleTenthDegree( const long long& aCadstarAngle )
    {
        // CADSTAR v6 (which outputted Schematic Format Version 8) and earlier used 1/10 degree
        // as the unit for angles/orientations. It is assumed that CADSTAR version 7 (i.e. Schematic
        // Format Version 9 and later) is the version that introduced 1/1000 degree for angles.
        if( Header.Format.Version > 8 )
        {
            return (double) aCadstarAngle / 100.0;
        }
        else
        {
            return (double) aCadstarAngle;
        }
    }

    /**
     * @brief
     * @param aCadstarAngle
     * @return
     */
    double getAngleDegrees( const long long& aCadstarAngle )
    {
        return getAngleTenthDegree( aCadstarAngle ) / 10.0;
    }


    long long getCadstarAngle( const double& aAngleTenthDegree )
    {
        return KiROUND( ( aAngleTenthDegree / getAngleTenthDegree( aAngleTenthDegree ) )
                        * aAngleTenthDegree );
    }

    /**
     * @brief
     * @param aPoint
     * @return Angle in decidegrees of the polar representation of the point, scaled 0..360
     */
    double getPolarAngle( const wxPoint& aPoint );

    /**
     * @brief
     * @param aPoint
     * @return Radius of polar representation of the point
     */
    double getPolarRadius( const wxPoint& aPoint );

}; // CADSTAR_SCH_ARCHIVE_LOADER


#endif // CADSTAR_SCH_ARCHIVE_LOADER_H_
