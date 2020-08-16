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
 * @file cadstar_pcb.h
 * @brief Converts a CADSTAR_PCB_ARCHIVE_PARSER object into a KiCad BOARD object
 */

#ifndef CADSTAR_PCB_H_
#define CADSTAR_PCB_H_

#include <cadstar_pcb_archive_parser.h>

class BOARD;

class CADSTAR_PCB : public CADSTAR_PCB_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_PCB( wxString aFilename ) : CADSTAR_PCB_ARCHIVE_PARSER( aFilename )
    {
        mDesignCenter.x = 0;
        mDesignCenter.y = 0;
    }


    /**
     * @brief Loads a CADSTAR PCB Archive file into the KiCad BOARD object given
     * @param aBoard 
     */
    void Load( ::BOARD* aBoard );

private:
    ::BOARD*                              mBoard;
    std::map<LAYER_ID, PCB_LAYER_ID>      mLayermap; ///<Map between Cadstar and KiCad Layers TODO: convert to a map of LSET to allow multiple layer mappings (e.g. ALLDOC, ALLELEC)
    std::map<PHYSICAL_LAYER_ID, LAYER_ID> mCopperLayers;
    wxPoint                               mDesignCenter; ///< Used for calculating the required offset to apply to the Cadstar design so that it fits in KiCad canvas

    // Functions for loading individual elements:

    void loadBoardStackup();
    void loadBoards();


    /**
     * @brief 
     * @param aCadstarShape 
     * @param aCadstarLayerID KiCad layer to draw on
     * @param aCadstarLinecodeID Thickness of line to draw with
     */
    void drawCadstarShape( const SHAPE& aCadstarShape, const PCB_LAYER_ID& aKiCadLayer,
            const LINECODE_ID& aCadstarLinecodeID );


    /**
     * @brief Uses DRAWSEGMENT to draw the cutouts on mBoard object
     * @param aVertices 
     * @param aKiCadLayer KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     */
    void drawCadstarCutoutsAsSegments( const std::vector<CUTOUT>& aCutouts,
            const PCB_LAYER_ID& aKiCadLayer, const int& aLineThickness );


    /**
     * @brief Uses DRAWSEGMENT to draw the vertices on mBoard object
     * @param aCadstarVertices 
     * @param aKiCadLayer KiCad layer to draw on
     * @param aLineThickness Thickness of line to draw with
     */
    void drawCadstarVerticesAsSegments( const std::vector<VERTEX>& aCadstarVertices,
            const PCB_LAYER_ID& aKiCadLayer, const int& aLineThickness );

    /**
     * @brief Returns a vector of pointers to DRAWSEGMENT objects 
     * @param aCadstarVertices 
     * @return 
     */
    std::vector<DRAWSEGMENT *> getDrawSegments( const std::vector<VERTEX>& aCadstarVertices );


    /**
     * @brief If the LineCode ID is found, returns the thickness as defined in the Linecode,
     *  otherwise returns the default line thickness in Edge_Cuts KiCad layer
     * @param aCadstarLineCodeID 
     * @return 
     */
    int getLineThickness( const LINECODE_ID& aCadstarLineCodeID );


    /**
     * @brief Scales, offsets and inverts y axis to make the point usable directly in KiCad
     * @param aCadstarPoint 
     * @return 
     */
    wxPoint getKiCadPoint( wxPoint aCadstarPoint );


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
     * @return PCB_LAYER_ID
     */
    PCB_LAYER_ID getKiCadLayer( const LAYER_ID& aCadstarLayerID );
};


#endif // CADSTAR_PCB_H_
