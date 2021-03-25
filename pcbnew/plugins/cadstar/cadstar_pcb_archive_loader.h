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
 * @file cadstar_pcb_archive_loader.h
 * @brief Loads a cpa file into a KiCad BOARD object
 */

#ifndef CADSTAR_PCB_ARCHIVE_LOADER_H_
#define CADSTAR_PCB_ARCHIVE_LOADER_H_

#include <cadstar_pcb_archive_parser.h>
#include <cadstar_pcb_archive_plugin.h>
#include <board.h>
#include <set>

class BOARD;
class DIMENSION_BASE;

class CADSTAR_PCB_ARCHIVE_LOADER : public CADSTAR_PCB_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_PCB_ARCHIVE_LOADER(
            wxString aFilename, LAYER_MAPPING_HANDLER aLayerMappingHandler, bool aLogLayerWarnings )
            : CADSTAR_PCB_ARCHIVE_PARSER( aFilename )
    {
        m_layerMappingHandler     = aLayerMappingHandler;
        m_logLayerWarnings        = aLogLayerWarnings;
        m_board                   = nullptr;
        m_project                 = nullptr;
        m_designCenter.x          = 0;
        m_designCenter.y          = 0;
        m_doneCopperWarning       = false;
        m_doneSpacingClassWarning = false;
        m_doneNetClassWarning     = false;
        m_numNets                 = 0;
    }


    ~CADSTAR_PCB_ARCHIVE_LOADER()
    {
        for( std::pair<SYMDEF_ID, FOOTPRINT*> libItem : m_libraryMap )
        {
            FOOTPRINT* footprint = libItem.second;

            if( footprint )
                delete footprint;
        }
    }

    /**
     * @brief Loads a CADSTAR PCB Archive file into the KiCad BOARD object given
     * @param aBoard
     */
    void Load( BOARD* aBoard, PROJECT* aProject );

    /**
     * @brief Return a copy of the loaded library footprints (caller owns the objects)
     * @return Container with all the footprint definitions that were loaded
     */
    std::vector<FOOTPRINT*> GetLoadedLibraryFootpints() const;

private:
    LAYER_MAPPING_HANDLER            m_layerMappingHandler; ///< Callback to get layer mapping
    bool                             m_logLayerWarnings;    ///< Used in loadBoardStackup()
    BOARD*                           m_board;
    PROJECT*                         m_project;
    std::map<LAYER_ID, PCB_LAYER_ID> m_layermap;         ///< Map between Cadstar and KiCad Layers.
                                                         ///< Populated by loadBoardStackup().
    std::map<SYMDEF_ID, FOOTPRINT*>  m_libraryMap;       ///< Map between Cadstar and KiCad
                                                         ///< components in the library. Populated
                                                         ///< by loadComponentLibrary(). Owns the
                                                         ///< FOOTPRINT objects.
    std::map<GROUP_ID, PCB_GROUP*>   m_groupMap;         ///< Map between Cadstar and KiCad
                                                         ///< groups. Does NOT ownthe PCB_GROUP
                                                         ///< objects (these should have been
                                                         ///< loaded to m_board).
    std::map<COMPONENT_ID, FOOTPRINT*> m_componentMap;   ///< Map between Cadstar and KiCad
                                                         ///< components on the board. Does NOT own
                                                         ///< the FOOTPRINT objects (these should
                                                         ///< have been loaded to m_board).
    std::map<NET_ID, NETINFO_ITEM*>       m_netMap;      ///< Map between Cadstar and KiCad Nets
    std::map<ROUTECODE_ID, NETCLASSPTR>   m_netClassMap; ///< Map between Cadstar and KiCad classes
    std::map<TEMPLATE_ID, ZONE*> m_zonesMap;             ///< Map between Cadstar and KiCad zones
    std::vector<LAYER_ID> m_powerPlaneLayers;            ///< List of layers that are marked as
                                                         ///< power plane in CADSTAR. This is used
                                                         ///< by "loadtemplates"
    wxPoint m_designCenter;                              ///< Used for calculating the required
                                                         ///< offset to apply to the Cadstar design
                                                         ///< so that it fits in KiCad canvas
    std::set<HATCHCODE_ID> m_hatchcodesTested;           ///< Used by checkAndLogHatchCode() to
                                                         ///< avoid multiple duplicate warnings
    std::set<PADCODE_ID> m_padcodesTested;               ///< Used by getKiCadPad() to avoid
                                                         ///< multiple duplicate warnings
    bool m_doneCopperWarning;                            ///< Used by loadCoppers() to avoid
                                                         ///< multiple duplicate warnings
    bool m_doneSpacingClassWarning;                      ///< Used by getKiCadNet() to avoid
                                                         ///< multiple duplicate warnings
    bool m_doneNetClassWarning;                          ///< Used by getKiCadNet() to avoid
                                                         ///< multiple duplicate warnings
    int m_numNets;                                       ///< Number of nets loaded so far


    // Functions for loading individual elements:
    void loadBoardStackup();
    void remapUnsureLayers(); ///< Callback m_layerMappingHandler for layers we aren't sure of
    void loadDesignRules();
    void loadComponentLibrary();
    void loadGroups();
    void loadBoards();
    void loadFigures();
    void loadTexts();
    void loadDimensions();
    void loadAreas();
    void loadComponents();
    void loadDocumentationSymbols();
    void loadTemplates();
    void loadCoppers();
    void loadNets();
    void loadTextVariables();

    // Helper functions for element loading:
    void logBoardStackupWarning( const wxString& aCadstarLayerName,
                                 const PCB_LAYER_ID& aKiCadLayer );
    void logBoardStackupMessage( const wxString& aCadstarLayerName,
                                 const PCB_LAYER_ID& aKiCadLayer );
    void initStackupItem( const LAYER& aCadstarLayer, BOARD_STACKUP_ITEM* aKiCadItem,
                          int aDielectricSublayer );
    void loadLibraryFigures( const SYMDEF_PCB& aComponent, FOOTPRINT* aFootprint );
    void loadLibraryCoppers( const SYMDEF_PCB& aComponent, FOOTPRINT* aFootprint );
    void loadLibraryAreas( const SYMDEF_PCB& aComponent, FOOTPRINT* aFootprint );
    void loadLibraryPads( const SYMDEF_PCB& aComponent, FOOTPRINT* aFootprint );
    void loadComponentAttributes( const COMPONENT& aComponent, FOOTPRINT* aFootprint );
    void loadNetTracks( const NET_ID& aCadstarNetID, const NET_PCB::ROUTE& aCadstarRoute,
                        long aStartWidth = std::numeric_limits<long>::max(),
                        long aEndWidth = std::numeric_limits<long>::max() );

    /// Load via and return via size
    int loadNetVia( const NET_ID& aCadstarNetID, const NET_PCB::VIA& aCadstarVia );
    void checkAndLogHatchCode( const HATCHCODE_ID& aCadstarHatchcodeID );
    void applyDimensionSettings( const DIMENSION& aCadstarDim, DIMENSION_BASE* aKiCadDim );


    /**
     * @brief Tries to make a best guess as to the zone priorities based on the pour status.
     */
    void calculateZonePriorities();

    //Helper functions for drawing /loading objects onto screen:

    /**
     * @brief
     * @param aCadstarText
     * @param aContainer to draw on (e.g. m_board)
     * @param aCadstarGroupID to add the text to
     * @param aCadstarLayerOverride if not empty, overrides the LayerID in aCadstarText
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, it inverts the Mirror status of aCadstarText
     */
    void drawCadstarText( const TEXT& aCadstarText, BOARD_ITEM_CONTAINER* aContainer,
                          const GROUP_ID& aCadstarGroupID = wxEmptyString,
                          const LAYER_ID& aCadstarLayerOverride = wxEmptyString,
                          const wxPoint& aMoveVector = { 0, 0 },
                          const double& aRotationAngle = 0.0,
                          const double& aScalingFactor = 1.0,
                          const wxPoint& aTransformCentre = { 0, 0 },
                          const bool& aMirrorInvert = false );

    /**
     * @brief
     * @param aCadstarShape
     * @param aCadstarLayerID KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     * @param aShapeName for reporting warnings/errors to the user
     * @param aContainer to draw on (e.g. m_board)
     * @param aCadstarGroupID to add the shape to
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, mirrors the shape
     */
    void drawCadstarShape( const SHAPE& aCadstarShape, const PCB_LAYER_ID& aKiCadLayer,
                           const int& aLineThickness, const wxString& aShapeName,
                           BOARD_ITEM_CONTAINER* aContainer,
                           const GROUP_ID& aCadstarGroupID = wxEmptyString,
                           const wxPoint& aMoveVector = { 0, 0 },
                           const double& aRotationAngle = 0.0, const double& aScalingFactor = 1.0,
                           const wxPoint& aTransformCentre = { 0, 0 },
                           const bool& aMirrorInvert = false );

    /**
     * @brief Uses PCB_SHAPE to draw the cutouts on m_board object
     * @param aVertices
     * @param aKiCadLayer KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     * @param aContainer to draw on (e.g. m_board)
     * @param aCadstarGroupID to add the shape to
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, mirrors the drawsegments
     */
    void drawCadstarCutoutsAsSegments( const std::vector<CUTOUT>& aCutouts,
                                       const PCB_LAYER_ID& aKiCadLayer, const int& aLineThickness,
                                       BOARD_ITEM_CONTAINER* aContainer,
                                       const GROUP_ID& aCadstarGroupID = wxEmptyString,
                                       const wxPoint& aMoveVector = { 0, 0 },
                                       const double& aRotationAngle = 0.0,
                                       const double& aScalingFactor = 1.0,
                                       const wxPoint& aTransformCentre = { 0, 0 },
                                       const bool& aMirrorInvert = false );

    /**
     * @brief Uses PCB_SHAPE to draw the vertices on m_board object
     * @param aCadstarVertices
     * @param aKiCadLayer KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     * @param aContainer to draw on (e.g. m_board)
     * @param aCadstarGroupID to add the shape to
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, mirrors the drawsegment
     * @param aCadstarGroupID to add the shape to
     */
    void drawCadstarVerticesAsSegments( const std::vector<VERTEX>& aCadstarVertices,
                                        const PCB_LAYER_ID& aKiCadLayer, const int& aLineThickness,
                                        BOARD_ITEM_CONTAINER* aContainer,
                                        const GROUP_ID& aCadstarGroupID = wxEmptyString,
                                        const wxPoint& aMoveVector = { 0, 0 },
                                        const double& aRotationAngle = 0.0,
                                        const double& aScalingFactor = 1.0,
                                        const wxPoint& aTransformCentre = { 0, 0 },
                                        const bool& aMirrorInvert = false );

    /**
     * @brief Returns a vector of pointers to PCB_SHAPE objects. Caller owns the objects.
     * @param aCadstarVertices
     * @param aContainer to draw on (e.g. m_board). Can be nullptr.
     * @param aCadstarGroupID to add the shape to
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, mirrors the drawsegment
     * @return
     */
    std::vector<PCB_SHAPE*> getDrawSegmentsFromVertices( const std::vector<VERTEX>& aCadstarVertices,
                                                         BOARD_ITEM_CONTAINER* aContainer = nullptr,
                                                         const GROUP_ID& aCadstarGroupID = wxEmptyString,
                                                         const wxPoint& aMoveVector = { 0, 0 },
                                                         const double& aRotationAngle = 0.0,
                                                         const double& aScalingFactor = 1.0,
                                                         const wxPoint& aTransformCentre = { 0, 0 },
                                                         const bool& aMirrorInvert = false );

    /**
     * @brief Returns a pointer to a PCB_SHAPE object. Caller owns the object.
     * @param aCadstarStartPoint
     * @param aCadstarVertex
     * @param aContainer to draw on (e.g. m_board). Can be nullptr.
     * @param aCadstarGroupID to add the shape to
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, mirrors the drawsegment
     * @return
     */
    PCB_SHAPE* getDrawSegmentFromVertex( const POINT& aCadstarStartPoint,
                                         const VERTEX& aCadstarVertex,
                                         BOARD_ITEM_CONTAINER* aContainer = nullptr,
                                         const GROUP_ID& aCadstarGroupID = wxEmptyString,
                                         const wxPoint& aMoveVector = { 0, 0 },
                                         const double& aRotationAngle = 0.0,
                                         const double& aScalingFactor = 1.0,
                                         const wxPoint& aTransformCentre = { 0, 0 },
                                         const bool& aMirrorInvert = false );

    /**
     * @brief
     * @param aCadstarShape
     * @param aLineThickness Thickness of line to draw with
     * @param aParentContainer Parent object (e.g. BOARD or FOOTPRINT pointer)
     * @return Pointer to ZONE. Caller owns the object.
     */
    ZONE* getZoneFromCadstarShape( const SHAPE& aCadstarShape, const int& aLineThickness,
                                   BOARD_ITEM_CONTAINER* aParentContainer );

    /**
     * @brief Returns a SHAPE_POLY_SET object from a Cadstar SHAPE
     * @param aCadstarShape
     * @param aLineThickness Thickness of line is used for expanding the polygon by half.
     * @param aContainer to draw on (e.g. m_board). Can be nullptr.
     * @param aMoveVector move draw segment by this amount (in KiCad coordinates)
     * @param aRotationAngle rotate draw segment by this amount (in tenth degrees)
     * @param aScalingFactor scale draw segment by this amount
     * @param aTransformCentre around which all transforms are applied (KiCad coordinates)
     * @param aMirrorInvert if true, mirrors the shape
     * @return
     */
    SHAPE_POLY_SET getPolySetFromCadstarShape( const SHAPE& aCadstarShape,
                                               const int& aLineThickness = -1,
                                               BOARD_ITEM_CONTAINER* aContainer = nullptr,
                                               const wxPoint& aMoveVector = { 0, 0 },
                                               const double& aRotationAngle = 0.0,
                                               const double& aScalingFactor = 1.0,
                                               const wxPoint& aTransformCentre = { 0, 0 },
                                               const bool& aMirrorInvert = false );

    /**
     * @brief Returns a SHAPE_LINE_CHAIN object from a series of PCB_SHAPE objects
     * @param aDrawSegments
     * @return
     */
    SHAPE_LINE_CHAIN getLineChainFromDrawsegments( const std::vector<PCB_SHAPE*> aDrawSegments );

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
    std::vector<TRACK*> makeTracksFromDrawsegments( const std::vector<PCB_SHAPE*> aDrawsegments,
                                                    BOARD_ITEM_CONTAINER* aParentContainer,
                                                    NETINFO_ITEM* aNet = nullptr,
                                                    PCB_LAYER_ID aLayerOverride = UNDEFINED_LAYER,
                                                    int aWidthOverride = -1 );

    /**
     * @brief Adds a CADSTAR Attribute to a KiCad footprint
     * @param aCadstarAttrLoc
     * @param aCadstarAttributeID
     * @param aFootprint
     * @param aAttributeValue
     */
    void addAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
                       const ATTRIBUTE_ID& aCadstarAttributeID, FOOTPRINT* aFootprint,
                       const wxString& aAttributeValue );

    /**
     * @brief CADSTAR's Post Processor does an action called "Route Offset" which
     * is applied when a route is wider than the pad on which it is terminating or
     * when there are different widths of route, in order to reduce overlap.
     * @param aPointToOffset Point that we want to offset by aOffsetAmount
     * @param aRefPoint Reference point to use for determine the angle of the offset
     * @param aOffsetAmount
    */
    void applyRouteOffset( wxPoint* aPointToOffset, const wxPoint& aRefPoint,
                           const long& aOffsetAmount );

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
    wxString   getAttributeValue( const ATTRIBUTE_ID&        aCadstarAttributeID,
              const std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>& aCadstarAttributeMap );

    // Helper Functions for obtaining individual elements as KiCad elements:
    double     getHatchCodeAngleDegrees( const HATCHCODE_ID& aCadstarHatchcodeID );
    PAD*       getKiCadPad( const COMPONENT_PAD& aCadstarPad, FOOTPRINT* aParent );
    FOOTPRINT* getFootprintFromCadstarID( const COMPONENT_ID& aCadstarComponentID );
    int        getKiCadHatchCodeThickness( const HATCHCODE_ID& aCadstarHatchcodeID );
    int        getKiCadHatchCodeGap( const HATCHCODE_ID& aCadstarHatchcodeID );
    PCB_GROUP* getKiCadGroup( const GROUP_ID& aCadstarGroupID );

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
        // CADSTAR v6 (which outputted Format Version 8) and earlier versions used 1/10 degree
        // as the unit for angles/orientations. It is assumed that CADSTAR version 7 (i.e. Format
        // Version 9 and later) is the version that introduced 1/1000 degree for angles.
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
     * @brief Searches m_netMap and returns the NETINFO_ITEM pointer if exists. Otherwise
     * creates a new one and adds it to m_board.
     * @param aCadstarNetID
     * @return
     */
    NETINFO_ITEM* getKiCadNet( const NET_ID& aCadstarNetID );

    /**
     * @brief
     * @param aLayerNum Physical / logical layer number (starts at 1)
     * @param aDetectMaxLayer If true, returns B.Cu if the requested layer is the maximum layer
     * @return PCB_LAYER_ID
     */
    PCB_LAYER_ID getKiCadCopperLayerID( unsigned int aLayerNum, bool aDetectMaxLayer = true );

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


    bool isFootprint( BOARD_ITEM_CONTAINER* aContainer )
    {
        return aContainer && aContainer->Type() == PCB_FOOTPRINT_T;
    }


    void addToGroup( const GROUP_ID& aCadstarGroupID, BOARD_ITEM* aKiCadItem );

    /**
     * @brief Adds a new PCB_GROUP* to m_groupMap
     * @param aName Name to give the group. If name already exists, append "_1", "_2", etc.
     * to the end to ensure it is unique
     * @return
     */
    GROUP_ID createUniqueGroupID( const wxString& aName );
};


#endif // CADSTAR_PCB_ARCHIVE_LOADER_H_
