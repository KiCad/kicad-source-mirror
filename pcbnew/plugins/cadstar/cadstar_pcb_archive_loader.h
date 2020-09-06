/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cadstar_pcb_archive_loader.h
 * @brief Loads a cpa file into a KiCad BOARD object
 */

#ifndef CADSTAR_PCB_ARCHIVE_LOADER_H_
#define CADSTAR_PCB_ARCHIVE_LOADER_H_

#include <cadstar_pcb_archive_parser.h>
#include <class_board.h>
#include <set>

class BOARD;

class CADSTAR_PCB_ARCHIVE_LOADER : public CADSTAR_PCB_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_PCB_ARCHIVE_LOADER( wxString aFilename )
            : CADSTAR_PCB_ARCHIVE_PARSER( aFilename )
    {
        mBoard                   = nullptr;
        mDesignCenter.x          = 0;
        mDesignCenter.y          = 0;
        mDoneCopperWarning       = false;
        mDoneSpacingClassWarning = false;
        mDoneNetClassWarning     = false;
        mNumNets                 = 0;
    }


    ~CADSTAR_PCB_ARCHIVE_LOADER()
    {
        for( std::pair<SYMDEF_ID, MODULE*> libItem : mLibraryMap )
        {
            MODULE* mod = libItem.second;

            if( mod )
                delete mod;
        }
    }

    /**
     * @brief Loads a CADSTAR PCB Archive file into the KiCad BOARD object given
     * @param aBoard 
     */
    void Load( ::BOARD* aBoard );


private:
    ::BOARD*                         mBoard;
    std::map<LAYER_ID, PCB_LAYER_ID> mLayermap;          ///< Map between Cadstar and KiCad Layers.
                                                         ///< Populated by loadBoardStackup().
    std::map<SYMDEF_ID, MODULE*> mLibraryMap;            ///< Map between Cadstar and KiCad
                                                         ///< components in the library. Populated
                                                         ///< by loadComponentLibrary(). Owns the
                                                         ///< MODULE objects.
    std::map<COMPONENT_ID, MODULE*> mComponentMap;       ///< Map between Cadstar and KiCad
                                                         ///< components on the board. Does NOT own
                                                         ///< the MODULE objects (these should have
                                                         ///< been loaded to mBoard).
    std::map<NET_ID, NETINFO_ITEM*>       mNetMap;       ///< Map between Cadstar and KiCad Nets
    std::map<ROUTECODE_ID, NETCLASSPTR>   mNetClassMap;  ///< Map between Cadstar and KiCad classes
    std::map<PHYSICAL_LAYER_ID, LAYER_ID> mCopperLayers; ///< Map of CADSTAR Physical layers to
                                                         ///< CADSTAR Layer IDs
    std::vector<LAYER_ID> mPowerPlaneLayers;             ///< List of layers that are marked as 
                                                         ///< power plane in CADSTAR. This is used 
                                                         ///< by "loadtemplates"
    wxPoint mDesignCenter;                               ///< Used for calculating the required
                                                         ///< offset to apply to the Cadstar design
                                                         ///< so that it fits in KiCad canvas
    std::set<HATCHCODE_ID> mHatchcodesTested;            ///< Used by checkAndLogHatchCode() to
                                                         ///< avoid multiple duplicate warnings
    bool mDoneCopperWarning;                             ///< Used by loadCoppers() to avoid
                                                         ///< multiple duplicate warnings
    bool mDoneSpacingClassWarning;                       ///< Used by getKiCadNet() to avoid
                                                         ///< multiple duplicate warnings
    bool mDoneNetClassWarning;                           ///< Used by getKiCadNet() to avoid
                                                         ///< multiple duplicate warnings
    int mNumNets;                                        ///< Number of nets loaded so far 


    // Functions for loading individual elements:
    void loadBoardStackup();
    void loadDesignRules();
    void loadComponentLibrary();
    void loadBoards();
    void loadFigures();
    void loadTexts();
    void loadAreas();
    void loadComponents();
    void loadDocumentationSymbols();
    void loadTemplates();
    void loadCoppers();
    void loadNets();

    // Helper functions for element loading:
    void logBoardStackupWarning(
            const wxString& aCadstarLayerName, const PCB_LAYER_ID& aKiCadLayer );
    void loadLibraryFigures( const SYMDEF& aComponent, MODULE* aModule );
    void loadLibraryPads( const SYMDEF& aComponent, MODULE* aModule );
    void loadComponentAttributes( const COMPONENT& aComponent, MODULE* aModule );
    void loadNetTracks( const NET_ID& aCadstarNetID, const NET::ROUTE& aCadstarRoute );
    void loadNetVia( const NET_ID& aCadstarNetID, const NET::VIA& aCadstarVia );
    void checkAndLogHatchCode( const HATCHCODE_ID& aCadstarHatchcodeID );

    //Helper functions for drawing /loading objects onto screen:
    /**
     * @brief 
     * @param aCadstarText 
     * @param aContainer to draw on (e.g. mBoard)
     * @param aCadstarLayerOverride if not empty, overrides the LayerID in aCadstarText
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, it inverts the Mirror status of aCadstarText
     */
    void drawCadstarText( const TEXT& aCadstarText, BOARD_ITEM_CONTAINER* aContainer,
            const LAYER_ID& aCadstarLayerOverride = wxEmptyString,
            const wxPoint& aMoveVector = { 0, 0 }, const double& aRotationAngle = 0.0,
            const double& aScalingFactor = 1.0, const wxPoint& aTransformCentre = { 0, 0 },
            const bool& aMirrorInvert = false );

    /**
     * @brief 
     * @param aCadstarShape 
     * @param aCadstarLayerID KiCad layer to draw on
     * @param aCadstarLinecodeID Thickness of line to draw with
     * @param aShapeName for reporting warnings/errors to the user
     * @param aContainer to draw on (e.g. mBoard)
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, it inverts the Mirror status of aCadstarText
     */
    void drawCadstarShape( const SHAPE& aCadstarShape, const PCB_LAYER_ID& aKiCadLayer,
            const LINECODE_ID& aCadstarLinecodeID, const wxString& aShapeName,
            BOARD_ITEM_CONTAINER* aContainer, const wxPoint& aMoveVector = { 0, 0 },
            const double& aRotationAngle = 0.0, const double& aScalingFactor = 1.0,
            const wxPoint& aTransformCentre = { 0, 0 }, const bool& aMirrorInvert = false );

    /**
     * @brief Uses DRAWSEGMENT to draw the cutouts on mBoard object
     * @param aVertices 
     * @param aKiCadLayer KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     * @param aContainer to draw on (e.g. mBoard)
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, it inverts the Mirror status of aCadstarText
     */
    void drawCadstarCutoutsAsSegments( const std::vector<CUTOUT>& aCutouts,
            const PCB_LAYER_ID& aKiCadLayer, const int& aLineThickness,
            BOARD_ITEM_CONTAINER* aContainer, const wxPoint& aMoveVector = { 0, 0 },
            const double& aRotationAngle = 0.0, const double& aScalingFactor = 1.0,
            const wxPoint& aTransformCentre = { 0, 0 }, const bool& aMirrorInvert = false );

    /**
     * @brief Uses DRAWSEGMENT to draw the vertices on mBoard object
     * @param aCadstarVertices 
     * @param aKiCadLayer KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     * @param aContainer to draw on (e.g. mBoard)
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, it inverts the Mirror status of aCadstarText
     */
    void drawCadstarVerticesAsSegments( const std::vector<VERTEX>& aCadstarVertices,
            const PCB_LAYER_ID& aKiCadLayer, const int& aLineThickness,
            BOARD_ITEM_CONTAINER* aContainer, const wxPoint& aMoveVector = { 0, 0 },
            const double& aRotationAngle = 0.0, const double& aScalingFactor = 1.0,
            const wxPoint& aTransformCentre = { 0, 0 }, const bool& aMirrorInvert = false );

    /**
     * @brief Returns a vector of pointers to DRAWSEGMENT objects. Caller owns the objects.
     * @param aCadstarVertices
     * @param aContainer to draw on (e.g. mBoard). Can be nullptr.
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, it inverts the Mirror status of aCadstarText
     * @return 
     */
    std::vector<DRAWSEGMENT*> getDrawSegmentsFromVertices(
            const std::vector<VERTEX>& aCadstarVertices, BOARD_ITEM_CONTAINER* aContainer = nullptr,
            const wxPoint& aMoveVector = { 0, 0 }, const double& aRotationAngle = 0.0,
            const double& aScalingFactor = 1.0, const wxPoint& aTransformCentre = { 0, 0 },
            const bool& aMirrorInvert = false );

    /**
     * @brief Returns a pointer to a DRAWSEGMENT object. Caller owns the object.
     * @param aCadstarStartPoint
     * @param aCadstarVertex
     * @param aContainer to draw on (e.g. mBoard). Can be nullptr.
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, it inverts the Mirror status of aCadstarText
     * @return 
     */
    DRAWSEGMENT* getDrawSegmentFromVertex( const POINT& aCadstarStartPoint,
            const VERTEX& aCadstarVertex, BOARD_ITEM_CONTAINER* aContainer = nullptr,
            const wxPoint& aMoveVector = { 0, 0 }, const double& aRotationAngle = 0.0,
            const double& aScalingFactor = 1.0, const wxPoint& aTransformCentre = { 0, 0 },
            const bool& aMirrorInvert = false );

    /**
     * @brief 
     * @param aCadstarShape 
     * @param aLineThickness Thickness of line to draw with
     * @return Pointer to ZONE_CONTAINER. Caller owns the object.
     */
    ZONE_CONTAINER* getZoneFromCadstarShape(
            const SHAPE& aCadstarShape, const int& aLineThickness );

    /**
     * @brief Returns a SHAPE_POLY_SET object from a Cadstar SHAPE
     * @param aCadstarShape
     * @param aLineThickness Thickness of line is used for expanding the polygon by half.
     * @param aContainer to draw on (e.g. mBoard). Can be nullptr.
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, it inverts the Mirror status of aCadstarText
     * @return 
     */
    SHAPE_POLY_SET getPolySetFromCadstarShape( const SHAPE& aCadstarShape,
            const int& aLineThickness = -1, BOARD_ITEM_CONTAINER* aContainer = nullptr,
            const wxPoint& aMoveVector = { 0, 0 }, const double& aRotationAngle = 0.0,
            const double& aScalingFactor = 1.0, const wxPoint& aTransformCentre = { 0, 0 },
            const bool& aMirrorInvert = false );

    /**
     * @brief Returns a SHAPE_LINE_CHAIN object from a series of DRAWSEGMENT objects
     * @param aDrawSegments
     * @return 
     */
    SHAPE_LINE_CHAIN getLineChainFromDrawsegments( const std::vector<DRAWSEGMENT*> aDrawSegments );

    /**
     * @brief Returns a vector of pointers to TRACK/ARC objects. Caller owns the objects
     * @param aDrawsegments 
     * @param aParentContainer sets this as the parent of each TRACK object and Add()s it to the parent
     * @param aNet sets all the tracks to this net, unless nullptr
     * @param aLayerOverride Sets all tracks to this layer, or, if it is UNDEFINED_LAYER, uses the layers
     *                       in the DrawSegments
     * @param aWidthOverride Sets all tracks to this width, or, if it is UNDEFINED_LAYER, uses the width
     *                       in the DrawSegments
     * @return 
    */
    std::vector<TRACK*> makeTracksFromDrawsegments( const std::vector<DRAWSEGMENT*> aDrawsegments,
            BOARD_ITEM_CONTAINER* aParentContainer, NETINFO_ITEM* aNet = nullptr,
            const PCB_LAYER_ID& aLayerOverride = PCB_LAYER_ID::UNDEFINED_LAYER,
            int                 aWidthOverride = -1 );

    /**
     * @brief Adds a CADSTAR Attribute to a KiCad module
     * @param aCadstarAttrLoc 
     * @param aCadstarAttributeID
     * @param aModule 
     * @param aAttributeValue 
     */
    void addAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
            const ATTRIBUTE_ID& aCadstarAttributeID, MODULE* aModule,
            const wxString& aAttributeValue );

    //Helper Functions for obtaining CADSTAR elements in the parsed structures
    int        getLineThickness( const LINECODE_ID& aCadstarLineCodeID );
    COPPERCODE getCopperCode( const COPPERCODE_ID& aCadstaCopperCodeID );
    HATCHCODE  getHatchCode( const HATCHCODE_ID& aCadstarHatchcodeID );
    LAYERPAIR  getLayerPair( const LAYERPAIR_ID& aCadstarLayerPairID );
    PADCODE    getPadCode( const PADCODE_ID& aCadstarPadCodeID );
    PART       getPart( const PART_ID& aCadstarPartID );
    ROUTECODE  getRouteCode( const ROUTECODE_ID& aCadstarRouteCodeID );
    TEXTCODE   getTextCode( const TEXTCODE_ID& aCadstarTextCodeID );
    VIACODE    getViaCode( const VIACODE_ID& aCadstarViaCodeID );
    wxString   getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID );
    wxString  getAttributeValue( const ATTRIBUTE_ID&        aCadstarAttributeID,
            const std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>& aCadstarAttributeMap );

    // Helper Functions for obtaining individual elements as KiCad elements:
    double    getHatchCodeAngleDegrees( const HATCHCODE_ID& aCadstarHatchcodeID );
    MODULE*   getModuleFromCadstarID( const COMPONENT_ID& aCadstarComponentID );
    int       getKiCadHatchCodeThickness( const HATCHCODE_ID& aCadstarHatchcodeID );
    int       getKiCadHatchCodeGap( const HATCHCODE_ID& aCadstarHatchcodeID );

    /**
     * @brief Scales, offsets and inverts y axis to make the point usable directly in KiCad
     * @param aCadstarPoint 
     * @return 
     */
    wxPoint getKiCadPoint( wxPoint aCadstarPoint );

    /**
     * @brief 
     * @param aCadstarLength 
     * @return 
    */
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
        return (double) aCadstarAngle / 100.0;
    }

    /**
     * @brief 
     * @param aCadstarAngle 
     * @return 
     */
    double getAngleDegrees( const long long& aCadstarAngle )
    {
        return (double) aCadstarAngle / 1000.0;
    }

    /**
     * @brief 
     * @param aPoint 
     * @return Angle in decidegrees of the polar representation of the point, scaled 0..360
     */
    double getPolarAngle( wxPoint aPoint );

    /**
     * @brief Searches mNetMap and returns the NETINFO_ITEM pointer if exists. Otherwise
     * creates a new one and adds it to mBoard.
     * @param aCadstarNetID 
     * @return 
     */
    NETINFO_ITEM* getKiCadNet( const NET_ID& aCadstarNetID );

    /**
     * @brief 
     * @param aLayerNum Physical / logical layer number (starts at 1)
     * @return PCB_LAYER_ID
     */
    PCB_LAYER_ID getKiCadCopperLayerID( unsigned int aLayerNum );

    /**
     * @brief 
     * @param aCadstarLayerID 
     * @return true if the layer corresponds to a KiCad LSET or false if the layer maps directly
     */
    bool isLayerSet( const LAYER_ID& aCadstarLayerID );

    /**
     * @brief 
     * @param aCadstarLayerID 
     * @return PCB_LAYER_ID
     */
    PCB_LAYER_ID getKiCadLayer( const LAYER_ID& aCadstarLayerID );

    /**
     * @brief 
     * @param aCadstarLayerID 
     * @return LSET
     */
    LSET getKiCadLayerSet( const LAYER_ID& aCadstarLayerID );


    bool isModule( BOARD_ITEM_CONTAINER* aContainer )
    {
        return aContainer && aContainer->GetClass() == wxT( "MODULE" );
    }
};


#endif // CADSTAR_PCB_ARCHIVE_LOADER_H_
