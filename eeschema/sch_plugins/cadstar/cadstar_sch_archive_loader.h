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
class LIB_PART;
class SCH_COMPONENT;
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
    explicit CADSTAR_SCH_ARCHIVE_LOADER( wxString aFilename )
            : CADSTAR_SCH_ARCHIVE_PARSER( aFilename )
    {
        mSchematic      = nullptr;
        mRootSheet      = nullptr;
        mPlugin         = nullptr;
        mDesignCenter.x = 0;
        mDesignCenter.y = 0;
    }


    ~CADSTAR_SCH_ARCHIVE_LOADER()
    {
    }

    /**
     * @brief Loads a CADSTAR PCB Archive file into the KiCad BOARD object given
     * @param aSchematic Schematic to add the design onto
     * @param aRootSheet Root sheet to add the design onto
     */
    void Load( ::SCHEMATIC* aSchematic, ::SCH_SHEET* aRootSheet,
            SCH_PLUGIN::SCH_PLUGIN_RELEASER* aSchPlugin, const wxFileName& aLibraryFileName );


private:
    typedef std::pair<BLOCK_ID, TERMINAL_ID> BLOCK_PIN_ID;
    typedef std::map<TERMINAL_ID, wxString>  TERMINAL_TO_PINNUM_MAP;

    ::SCHEMATIC*                     mSchematic;
    ::SCH_SHEET*                     mRootSheet;
    SCH_PLUGIN::SCH_PLUGIN_RELEASER* mPlugin;
    wxFileName                       mLibraryFileName;
    wxPoint                          mDesignCenter; ///< Used for calculating the required
                                                    ///< offset to apply to the Cadstar design
                                                    ///< so that it fits in KiCad canvas
    std::set<HATCHCODE_ID> mHatchcodesTested;       ///< Used by checkAndLogHatchCode() to
                                                    ///< avoid multiple duplicate warnings
    std::map<LAYER_ID, SCH_SHEET*> mSheetMap;       ///< Map between Cadstar and KiCad Sheets
    std::map<BLOCK_PIN_ID, SCH_HIERLABEL*>
                                 mSheetPinMap; ///< Map between Cadstar and KiCad Sheets Pins
    std::map<PART_ID, LIB_PART*> mPartMap;     ///< Map between Cadstar and KiCad Parts
    std::map<PART_ID, TERMINAL_TO_PINNUM_MAP> mPinNumsMap;  ///< Map of pin numbers
    std::map<SYMDEF_ID, LIB_PART*>
            mPowerSymLibMap; ///< Map between Cadstar and KiCad Power Symbol Library items
    std::map<SYMBOL_ID, SCH_COMPONENT*>
            mPowerSymMap; ///< Map between Cadstar and KiCad Power Symbols
    std::map<SYMBOL_ID, SCH_GLOBALLABEL*>
            mGlobLabelMap; ///< Map between Cadstar and KiCad Global Labels
    std::map<BUS_ID, std::shared_ptr<BUS_ALIAS>> mBusesMap; ///< Map between Cadstar and KiCad Buses

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
    void loadSheetAndChildSheets( LAYER_ID aCadstarSheetID, wxPoint aPosition, wxSize aSheetSize,
            const SCH_SHEET_PATH& aParentSheet );

    void loadChildSheets( LAYER_ID aCadstarSheetID, const SCH_SHEET_PATH& aSheet );

    std::vector<LAYER_ID> findOrphanSheets();

    int getSheetNumber( LAYER_ID aCadstarSheetID );

    void loadItemOntoKiCadSheet( LAYER_ID aCadstarSheetID, SCH_ITEM* aItem );

    //Helper Functions for loading library items
    void loadSymDefIntoLibrary( const SYMDEF_ID& aSymdefID, const PART* aCadstarPart,
            const GATE_ID& aGateID, LIB_PART* aPart );

    void loadLibrarySymbolShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
            wxPoint aSymbolOrigin, LIB_PART* aPart, int aGateNumber );

    void applyToLibraryFieldAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
            wxPoint aSymbolOrigin, LIB_FIELD* aKiCadField );

    //Helper Functions for loading symbols in schematic
    SCH_COMPONENT* loadSchematicSymbol( const SYMBOL& aCadstarSymbol, const LIB_PART& aKiCadPart,
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
    void      applyTextSettings( const TEXTCODE_ID& aCadstarTextCodeID,
                 const ALIGNMENT& aCadstarAlignment, const JUSTIFICATION& aCadstarJustification,
                 EDA_TEXT* aKiCadTextItem );
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
    wxString       getAttributeValue( const ATTRIBUTE_ID&        aCadstarAttributeID,
                  const std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>& aCadstarAttributeMap );

    PART::DEFINITION::PIN getPartDefinitionPin(
            const PART& aCadstarPart, const GATE_ID& aGateID, const TERMINAL_ID& aTerminalID );

    //Helper Functions for obtaining individual elements as KiCad elements:
    ELECTRICAL_PINTYPE getKiCadPinType( const PART::PIN_TYPE& aPinType );

    int              getKiCadUnitNumberFromGate( const GATE_ID& aCadstarGateID );
    LABEL_SPIN_STYLE getSpinStyle( const long long& aCadstarOrientation, bool aMirror );
    LABEL_SPIN_STYLE getSpinStyleDeciDeg( const double& aOrientationDeciDeg );
    SCH_FIELD*       getFieldByName( SCH_COMPONENT* aComponent );

    //General Graphical manipulation functions
    std::pair<wxPoint, wxSize> getFigureExtentsKiCad( const FIGURE& aCadstarFigure );

    wxPoint getKiCadPoint( wxPoint aCadstarPoint );

    wxPoint getKiCadLibraryPoint( wxPoint aCadstarPoint, wxPoint aCadstarCentre );

    wxPoint applyTransform( const wxPoint& aPoint, const wxPoint& aMoveVector = { 0, 0 },
            const double& aRotationAngleDeciDeg = 0.0, const double& aScalingFactor = 1.0,
            const wxPoint& aTransformCentre = { 0, 0 }, const bool& aMirrorInvert = false );

    int getKiCadLength( long long aCadstarLength )
    {
        return aCadstarLength * KiCadUnitMultiplier;
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

    /**
     * @brief
     * @param aPoint
     * @return Angle in decidegrees of the polar representation of the point, scaled 0..360
     */
    double getPolarAngle( wxPoint aPoint );

    /**
     * @brief
     * @param aPoint
     * @return Radius of polar representation of the point
     */
    double getPolarRadius( wxPoint aPoint );

}; // CADSTAR_SCH_ARCHIVE_LOADER


#endif // CADSTAR_SCH_ARCHIVE_LOADER_H_
