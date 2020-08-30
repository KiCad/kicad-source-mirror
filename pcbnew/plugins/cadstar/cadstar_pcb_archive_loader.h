/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <@Qbort>
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

class BOARD;

class CADSTAR_PCB_ARCHIVE_LOADER : public CADSTAR_PCB_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_PCB_ARCHIVE_LOADER( wxString aFilename ) : CADSTAR_PCB_ARCHIVE_PARSER( aFilename )
    {
        mBoard          = nullptr;
        mDesignCenter.x = 0;
        mDesignCenter.y = 0;
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
                                                         ///< by loadComponentLibrary().
    std::map<PHYSICAL_LAYER_ID, LAYER_ID> mCopperLayers; ///< Map of CADSTAR Physical layers to
                                                         ///< CADSTAR Layer IDs
    wxPoint mDesignCenter;                               ///< Used for calculating the required
                                                         ///< offset to apply to the Cadstar design
                                                         ///< so that it fits in KiCad canvas

    // Functions for loading individual elements:
    void loadBoardStackup();
    void loadComponentLibrary();
    void loadBoards();
    void loadFigures();
    void loadAreas();
    void loadComponents();

    // Helper functions for loading:
    void logBoardStackupWarning(
            const wxString& aCadstarLayerName, const PCB_LAYER_ID& aKiCadLayer );
    void loadLibraryFigures( const SYMDEF& aComponent, MODULE* aModule );
    void loadLibraryPads( const SYMDEF& aComponent, MODULE* aModule );
    void loadComponentAttributes( const COMPONENT& aComponent, MODULE* aModule );

    /**
     * @brief 
     * @param aCadstarShape 
     * @param aCadstarLayerID KiCad layer to draw on
     * @param aCadstarLinecodeID Thickness of line to draw with
     * @param aShapeName for reporting warnings/errors to the user
     * @param aContainer to draw on (e.g. mBoard)
     */
    void drawCadstarShape( const SHAPE& aCadstarShape, const PCB_LAYER_ID& aKiCadLayer,
            const LINECODE_ID& aCadstarLinecodeID, const wxString& aShapeName,
            BOARD_ITEM_CONTAINER* aContainer );


    /**
     * @brief Uses DRAWSEGMENT to draw the cutouts on mBoard object
     * @param aVertices 
     * @param aKiCadLayer KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     * @param aContainer to draw on (e.g. mBoard)
     */
    void drawCadstarCutoutsAsSegments( const std::vector<CUTOUT>& aCutouts,
            const PCB_LAYER_ID& aKiCadLayer, const int& aLineThickness,
            BOARD_ITEM_CONTAINER* aContainer );


    /**
     * @brief Uses DRAWSEGMENT to draw the vertices on mBoard object
     * @param aCadstarVertices 
     * @param aKiCadLayer KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     * @param aContainer to draw on (e.g. mBoard)
     */
    void drawCadstarVerticesAsSegments( const std::vector<VERTEX>& aCadstarVertices,
            const PCB_LAYER_ID& aKiCadLayer, const int& aLineThickness,
            BOARD_ITEM_CONTAINER* aContainer );

    /**
     * @brief Returns a vector of pointers to DRAWSEGMENT objects. Caller owns the objects.
     * @param aCadstarVertices      * 
     * @param aContainer to draw on (e.g. mBoard). Can be nullptr.
     * @return 
     */
    std::vector<DRAWSEGMENT*> getDrawSegmentsFromVertices(
            const std::vector<VERTEX>& aCadstarVertices,
            BOARD_ITEM_CONTAINER*      aContainer = nullptr );


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
     * @return 
     */
    SHAPE_POLY_SET getPolySetFromCadstarShape( const SHAPE& aCadstarShape,
            const int& aLineThickness = -1, BOARD_ITEM_CONTAINER* aContainer = nullptr );

    /**
     * @brief Returns a SHAPE_LINE_CHAIN object from a series of DRAWSEGMENT objects
     * @param aDrawSegments
     * @return 
     */
    SHAPE_LINE_CHAIN getLineChainFromDrawsegments( const std::vector<DRAWSEGMENT*> aDrawSegments );

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

    /**
     * @brief If the LineCode ID is found, returns the thickness as defined in the Linecode,
     *  otherwise returns the default line thickness in Edge_Cuts KiCad layer
     * @param aCadstarLineCodeID 
     * @return 
     */
    int getLineThickness( const LINECODE_ID& aCadstarLineCodeID );


    TEXTCODE getTextCode( const TEXTCODE_ID& aCadstarTextCodeID );


    PADCODE getPadCode( const PADCODE_ID& aCadstarPadCodeID );


    wxString getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID );


    wxString getAttributeValue( const ATTRIBUTE_ID&        aCadstarAttributeID,
            const std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>& aCadstarAttributeMap );

    PART getPart( const PART_ID& aCadstarPartID );

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
    double getKiCadAngle( const long long& aCadstarAngle )
    {
        return (double) aCadstarAngle / 100.0;
    }

    /**
     * @brief 
     * @param aPoint 
     * @return Angle in decidegrees of the polar representation of the point, scaled 0..360
     */
    double getPolarAngle( wxPoint aPoint );


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
