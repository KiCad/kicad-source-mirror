/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2010 - 2024 Fiji developers (Java source used as reference)
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <exporters/u3d/data_block.h>
#include <exporters/u3d/bit_stream_writer.h>

#include <TopoDS_Shape.hxx>
#include <TDocStd_Document.hxx>
#include <Standard_Handle.hxx>
#include <Graphic3d_Vec4.hxx>
#include <Graphic3d_Vec3.hxx>
#include <Bnd_Box.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_VisMaterialTool.hxx>

#include <math/vector2d.h>
#include <math/vector3.h>

#include <wx/mstream.h>

namespace std
{
template <>
struct hash<Graphic3d_Vec4>
{
    size_t operator()( const Graphic3d_Vec4& v ) const
    {
        size_t h1 = std::hash<float>{}( v.x() );
        size_t h2 = std::hash<float>{}( v.y() );
        size_t h3 = std::hash<float>{}( v.z() );
        size_t h4 = std::hash<float>{}( v.w() );
        return h1 ^ ( h2 << 1 ) ^ ( h3 << 2 ) ^ ( h4 << 3 );
    }
};
} // namespace std

namespace U3D
{
class MESH
{
public:
    std::vector<VECTOR3D>              coords;

    std::vector<VECTOR3D>              normals;

    /**
     * List of all unique diffuse colors
     */
    std::vector<Graphic3d_Vec4>        diffuse_colors;

    /**
     * List of all unique specular colors
     */
    std::vector<Graphic3d_Vec4>        specular_colors;
    /**
     * Index map helps select the color index for a given vertex when creating the colorIndices
     */
    std::unordered_map<Graphic3d_Vec4, uint32_t> diffuse_colors_index_map;
    /**
     * Index map helps select the color index for a given vertex when creating the colorIndices
     */
    std::unordered_map<Graphic3d_Vec4, uint32_t> specular_colors_index_map;

    /**
     * Coordinate indices, maps vertex positions to triangles
     */
    std::vector<uint32_t>              coordIndices;

    /**
     * Coordinate indices, maps vertex normal positions to triangles
     */
    std::vector<uint32_t>              normalIndices;

    /**
     * Coordinate indices, maps each vertex to a diffuse color
     */
    std::vector<uint32_t>              diffuseColorIndices;

    /**
     * Coordinate indices, maps each vertex to a specular color
     */
    std::vector<uint32_t>              specularColorIndices;

    /**
     * Diffuse color used if not using per-vertex color
     */
    Graphic3d_Vec4                     diffuse_color;

    /**
     * Specular color used if not using per-vertex color
     */
    Graphic3d_Vec3                     specular_color;

    /**
     * The name of the mesh, this will be visible in U3D viewers
     */
    std::string                        name;

    /**
     * Parent name (if any) to group the mesh under
     */
    std::string                        parentName;

    bool                               perVertexColor;

    bool IsEmpty()
    {
        return coords.empty() || coordIndices.empty();
    }

    uint32_t GetOrAddUniqueDiffuseColor( const Graphic3d_Vec4& aColor )
    {
        uint32_t colorIndex = 0;
        if( diffuse_colors_index_map.find( aColor ) != diffuse_colors_index_map.end() )
        {
            colorIndex = diffuse_colors_index_map[aColor];
        }
        else
        {
            colorIndex = diffuse_colors.size();
            diffuse_colors_index_map[aColor] = colorIndex;
            diffuse_colors.push_back( aColor );
        }

        return colorIndex;
    }

    uint32_t GetOrAddUniqueSpecularColor( const Graphic3d_Vec4& aColor )
    {
        uint32_t colorIndex = 0;
        if( specular_colors_index_map.find( aColor ) != specular_colors_index_map.end() )
        {
            colorIndex = specular_colors_index_map[aColor];
        }
        else
        {
            colorIndex = specular_colors.size();
            specular_colors_index_map[aColor] = colorIndex;
            specular_colors.push_back( aColor );
        }

        return colorIndex;
    }
};

struct PARENT_NODE
{
    /**
     * Name of the parent node to link to
     */
    std::string        name;

    /**
     * Transform matrix, 4x4, row major
     */
    std::vector<float> mat;
};

struct GROUP_NODE
{
    /**
     * Name of this group node
     */
    std::string              name;

    /**
     * List of parent nodes this group node belongs to
     */
    std::vector<PARENT_NODE> parentNodes;
};

class WRITER
{
public:
    WRITER( const std::string& aFilename );

    bool Perform( const Handle( TDocStd_Document ) & aDocument );

    const VECTOR3D& GetCenter() const { return m_center; }
    const Bnd_Box& GetMeshBoundingBox() const { return m_meshBoundingBox; }
private:
    void writeMatrix( BIT_STREAM_WRITER& aBitStreamWriter, const std::vector<float>& aMat );

    std::shared_ptr<DATA_BLOCK> getGroupNodeBlock( const std::string& aGroupNodeName,
                                                   const PARENT_NODE* aParentNode );

    std::shared_ptr<DATA_BLOCK> getModelNodeBlock( const std::string&        aModelNodeName,
                                                   const std::string&        aParentNodeName,
                                                   const std::string&        aModelResourceName,
                                                   const std::vector<float>& aMat );

    std::shared_ptr<DATA_BLOCK> getShadingModifierBlock( const std::string& aShadingModName,
                                                         const std::string& aShaderName );

    std::shared_ptr<DATA_BLOCK>
    getNodeModifierChain( const std::string& aModifierChainName, const std::string& aModelNodeName,
                          const std::string& aParentNodeName, const std::string& aModelResourceName,
                          const std::string& aShaderName, const std::vector<float>& aMat );


    std::shared_ptr<DATA_BLOCK> getHeaderBlock( uint32_t aDeclSize, uint32_t aContSize );


    std::shared_ptr<DATA_BLOCK> getLitTextureShaderBlock( const std::string& aShaderName,
                                                          const std::string& aMaterialName );


    std::shared_ptr<DATA_BLOCK> getMaterialResourceBlock( const std::string&    aMaterialName,
                                                          const Graphic3d_Vec4& aDiffuseColor,
                                                          const Graphic3d_Vec3& aSpecularColor );

    std::shared_ptr<DATA_BLOCK>
    getModelResourceModifierChain( const std::string& aModifierChainName, const MESH* aMesh,
                                   const std::string& aMeshname );

    std::shared_ptr<DATA_BLOCK> getMeshDeclarationBlock( const MESH*        aMesh,
                                                         const std::string& aMeshName );

    std::shared_ptr<DATA_BLOCK> getMeshContinuationBlock( const MESH*        aMesh,
                                                          const std::string& aMeshName );


    std::shared_ptr<DATA_BLOCK> getLightModifierChain( const std::string& aModifierChainName,
                                                       const std::string& aLightResourceName );

    std::shared_ptr<DATA_BLOCK> getLightNodeBlock( const std::string& aLightNodeName,
                                                   const std::string& aLightResourceName );

    std::shared_ptr<DATA_BLOCK> getLightResourceBlock( const std::string& aLightResourceName );

    std::shared_ptr<DATA_BLOCK>
    getGroupNodeModifierChain( const std::string&             aModifierChainName,
                               const std::vector<GROUP_NODE>& aGroupNodes );

    uint32_t writeDataBlock( std::shared_ptr<DATA_BLOCK> b, wxMemoryOutputStream& aStream );


    void generateMeshesByAssembly( const Handle( TDocStd_Document ) & doc );

    void collectGeometryRecursive( const TDF_Label& label, const Handle( XCAFDoc_ShapeTool ) & shapeTool,
                                   const Handle( XCAFDoc_ColorTool ) & colorTool,
                                   const Handle( XCAFDoc_VisMaterialTool ) & visMatTool,
                                   const gp_Trsf&    cumulativeTransform,
                                   const std::string& baseName,
                                   std::unordered_map<Graphic3d_Vec4, MESH*>& meshesByColor );


    void getMeshName( const TDF_Label& label, Handle( XCAFDoc_ShapeTool ) shapeTool, MESH* mesh );


    std::string                m_filename;
    VECTOR3D                   m_center;
    Bnd_Box                    m_meshBoundingBox;
    std::map<std::string, int> m_meshDedupMap;
    const uint32_t             m_contextBaseShadingID = 1;
    bool                       m_includeNormals;
    std::vector<std::unique_ptr<MESH>> m_meshes;
    std::vector<GROUP_NODE>    m_groupNodes; // dynamic + root grouping nodes
};

} // namespace U3D