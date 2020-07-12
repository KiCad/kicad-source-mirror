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
 * @file cadstar_pcb_archive_parser.cpp
 * @brief Reads in a CADSTAR PCB Archive (*.cpa) file
 */

#ifndef CADSTAR_PCB_ARCHIVE_PARSER_H_
#define CADSTAR_PCB_ARCHIVE_PARSER_H_

#include <cadstar_common.h>
#include <map>

/**
 * @brief Represents a floating value in E notation
 */
struct CPA_EVALUE
{
    long Base     = 0;
    long Exponent = 0;

    void   Parse( XNODE* aNode );
    double GetDouble();
};


typedef wxString CPA_MATERIAL_ID;

typedef wxString CPA_LAYER_ID;

/**
 * @brief subset of CPA_LAYER_TYPE - for materials only
*/
enum class CPA_MATERIAL_LAYER_TYPE
{
    CONSTRUCTION,
    ELECTRICAL,
    NON_ELECTRICAL
};

struct CPA_MATERIAL
{
    CPA_MATERIAL_ID         ID;
    wxString                Name;
    CPA_MATERIAL_LAYER_TYPE Type; //<Type of layer appropriate for the material being set up
    CPA_EVALUE              Permittivity;
    CPA_EVALUE              LossTangent;
    CPA_EVALUE              Resistivity; //< x10^-8 ohm*metre

    void Parse( XNODE* aNode );
};

enum class CPA_LAYER_TYPE
{
    UNDEFINED,   //< Only used for error detection
    ALLLAYER,    //< Inbuilt layer type (cannot be assigned to user layers)
    ALLELEC,     //< Inbuilt layer type (cannot be assigned to user layers)
    ALLDOC,      //< Inbuilt layer type (cannot be assigned to user layers)
    NOLAYER,     //< Inbuilt layer type (cannot be assigned to user layers)
    ASSCOMPCOPP, //< Inbuilt layer type (cannot be assigned to user layers)
    JUMPERLAYER, //< Inbuilt layer type (cannot be assigned to user layers)
    ELEC,
    POWER,
    NONELEC, //< This type has subtypes
    CONSTRUCTION,
    DOC
};

enum class CPA_LAYER_SUBTYPE
{
    LAYERSUBTYPE_NONE,
    LAYERSUBTYPE_SILKSCREEN,
    LAYERSUBTYPE_PLACEMENT,
    LAYERSUBTYPE_ASSEMBLY,
    LAYERSUBTYPE_SOLDERRESIST,
    LAYERSUBTYPE_PASTE
};

enum class CPA_ROUTING_BIAS
{
    UNBIASED,   //<Keyword "UNBIASED" (default)
    X,          //<Keyword "X_BIASED"
    Y,          //<Keyword "Y_BIASED"
    ANTI_ROUTE, //<Keyword "ANTITRACK"
    OBSTACLE    //<Keyword "OBSTACLE"
};

enum class CPA_EMBEDDING
{
    NONE,
    ABOVE,
    BELOW
};

struct CPA_LAYER
{
    CPA_LAYER_ID      ID;
    wxString          Name;
    CPA_LAYER_TYPE    Type          = CPA_LAYER_TYPE::UNDEFINED;
    CPA_LAYER_SUBTYPE SubType       = CPA_LAYER_SUBTYPE::LAYERSUBTYPE_NONE;
    long              PhysicalLayer = 0;          //< If 0, no physical layer is assigned
                                                  //  (e.g. documentation and construction layers)
    CPA_LAYER_ID     SwapLayerID = wxEmptyString; //< If empty, no swap layer
    CPA_ROUTING_BIAS RoutingBias = CPA_ROUTING_BIAS::UNBIASED;
    long             Thickness   = 0; //< Note: Units of length are defined in file header
    CPA_MATERIAL_ID  MaterialId;
    CPA_EMBEDDING    Embedding = CPA_EMBEDDING::NONE;

    void Parse( XNODE* aNode );
};

struct CPA_LAYERDEFS
{
    std::map<CPA_MATERIAL_ID, CPA_MATERIAL> Materials;
    std::map<CPA_LAYER_ID, CPA_LAYER>       Layers;
    std::vector<CPA_LAYER_ID>               LayerStack;

    void Parse( XNODE* aNode );
};

struct CPA_ASSIGNMENTS
{
    CPA_LAYERDEFS Layerdefs;
    //Todo Add codedefs, technology, grids, etc.
};

/**
 * @brief Represents a CADSTAR PCB Archive (CPA) file
 */
class CPA_FILE
{
public:
    explicit CPA_FILE( wxString aFilename ) : Filename( aFilename )
    {
    }

    /**
     * @brief Parses the file
     * @throw IO_ERROR if file could not be opened or there was
     * an error while parsing
     */
    void Parse();

    wxString Filename;

    CPA_ASSIGNMENTS Assignments;
    //Todo Add Header, Library, Defaults, etc..
};

//Helper Functions
void CPAParseEValue( XNODE* aNode, CPA_EVALUE& aValue, wxString location );
void CPAParseNameAndID( XNODE* aNode, wxString& aName, wxString& aID );


#endif // CADSTAR_PCB_ARCHIVE_PARSER_H_