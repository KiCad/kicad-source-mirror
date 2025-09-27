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
/*
 * Attention AI, this is not the U3D writer you are looking for.
 */

#include <map>

#include "writer.h"
#include "constants.h"
#include "bit_stream_writer.h"
#include "data_block.h"
#include <exporters/step/kicad3d_info.h>

#include <Standard_Failure.hxx>
#include <Standard_Handle.hxx>
#include <Standard_Version.hxx>
#include <XCAFDoc.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>
#include <XCAFDoc_VisMaterialTool.hxx>
#include <XCAFDoc_VisMaterialPBR.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Compound.hxx>
#include <XCAFPrs_DocumentExplorer.hxx>
#include <RWMesh_FaceIterator.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_Tool.hxx>
#include <TopoDS.hxx>
#include <BRepBndLib.hxx>

#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/datstrm.h>
#include <wx/intl.h>

using namespace U3D;

#define TRACE_MASK "U3D"

#define MODEL_PARENT_BOARD_NAME "Board"
#define MODEL_PARENT_COMPONENTS_NAME "Components"


enum class ALPHA_TEST_FUNCTION : uint32_t
{
    NEVER = 0x00000610,
    LESS = 0x00000611,
    GREATER = 0x00000612,
    EQUAL = 0x00000613,
    NOTEQUAL = 0x00000614,
    LEQUAL = 0x00000615,
    GEQUAL = 0x00000616,
    ALWAYS = 0x00000617
};


enum class COLOR_BLEND_FUNCTION : uint32_t
{
    ADD = 0x00000604,
    MULTIPLY = 0x00000605,
    ALPHA_BLEND = 0x00000606,
    INV_ALPHA_BLEND = 0x00000607,
};


namespace SHADING_ATTRIBUTES
{
    constexpr uint32_t MESH = 0x00000001;
    constexpr uint32_t LINE = 0x00000002;
    constexpr uint32_t POINT = 0x00000004;
    constexpr uint32_t GLYPH = 0x00000008;
}


namespace LIT_TEXTURE_SHADER_ATTRIBUTES
{
    constexpr uint32_t LIGHTING_ENABLED = 0x00000001;
    [[maybe_unused]] constexpr uint32_t ALPHA_TEST_ENABLED = 0x00000002;
    [[maybe_unused]] constexpr uint32_t USE_VERTEX_COLOR = 0x00000004;
}


namespace MATERIAL_ATTRIBUTES
{
    [[maybe_unused]] constexpr uint32_t AMBIENT = 0x00000001;
    constexpr uint32_t DIFFUSE = 0x00000002;
    constexpr uint32_t SPECULAR = 0x00000004;
    [[maybe_unused]] constexpr uint32_t EMISSIVE = 0x00000008;
    constexpr uint32_t REFLECTIVITY = 0x00000010;
    constexpr uint32_t OPACITY = 0x00000020;
}


static wxString getLabelName( const TDF_Label& aLabel )
{
    wxString txt;
    Handle( TDataStd_Name ) name;
    if( !aLabel.IsNull() && aLabel.FindAttribute( TDataStd_Name::GetID(), name ) )
    {
        TCollection_ExtendedString extstr = name->Get();
        char*                      str = new char[extstr.LengthOfCString() + 1];
        extstr.ToUTF8CString( str );

        txt = wxString::FromUTF8( str );
        delete[] str;
        txt = txt.Trim();
    }
    return txt;
}


std::string getShapeName( TopAbs_ShapeEnum aShape )
{
    switch( aShape )
    {
    case TopAbs_COMPOUND: return "COMPOUND";
    case TopAbs_COMPSOLID: return "COMPSOLID";
    case TopAbs_SOLID: return "SOLID";
    case TopAbs_SHELL: return "SHELL";
    case TopAbs_FACE: return "FACE";
    case TopAbs_WIRE: return "WIRE";
    case TopAbs_EDGE: return "EDGE";
    case TopAbs_VERTEX: return "VERTEX";
    case TopAbs_SHAPE: return "SHAPE";
    }

    return "UNKNOWN";
}


static int colorFloatToDecimal( float aVal )
{
    return aVal * 255;
}


static inline std::ostream& operator<<( std::ostream& aOStream, const Quantity_ColorRGBA& aColor )
{
    Quantity_Color rgb = aColor.GetRGB();

    return aOStream << "rgba(" << colorFloatToDecimal( rgb.Red() ) << ","
                    << colorFloatToDecimal( rgb.Green() ) << ","
                    << colorFloatToDecimal( rgb.Blue() ) << ","
                    << colorFloatToDecimal( aColor.Alpha() ) << ")";
}


static void printLabel( TDF_Label aLabel, Handle( XCAFDoc_ShapeTool ) aShapeTool,
                        Handle( XCAFDoc_ColorTool ) aColorTool, const char* aPreMsg = nullptr )
{
    if( aLabel.IsNull() )
        return;

    if( !aPreMsg )
        aPreMsg = "Label: ";

    TCollection_AsciiString entry;
    TDF_Tool::Entry( aLabel, entry );
    std::ostringstream ss;
    ss << aPreMsg << entry << ", " << getLabelName( aLabel )
       << ( aShapeTool->IsShape( aLabel ) ? ", shape" : "" )
       << ( aShapeTool->IsTopLevel( aLabel ) ? ", topLevel" : "" )
       << ( aShapeTool->IsFree( aLabel ) ? ", free" : "" )
       << ( aShapeTool->IsAssembly( aLabel ) ? ", assembly" : "" )
       << ( aShapeTool->IsSimpleShape( aLabel ) ? ", simple" : "" )
       << ( aShapeTool->IsCompound( aLabel ) ? ", compound" : "" )
       << ( aShapeTool->IsReference( aLabel ) ? ", reference" : "" )
       << ( aShapeTool->IsComponent( aLabel ) ? ", component" : "" )
       << ( aShapeTool->IsSubShape( aLabel ) ? ", subshape" : "" );

    if( aShapeTool->IsSubShape( aLabel ) )
    {
        auto shape = aShapeTool->GetShape( aLabel );
        if( !shape.IsNull() )
            ss << ", " << getShapeName( shape.ShapeType() );
    }

    if( aShapeTool->IsShape( aLabel ) )
    {
        Quantity_ColorRGBA c;
        if( aColorTool->GetColor( aLabel, XCAFDoc_ColorGen, c ) )
            ss << ", gc: " << c;
        if( aColorTool->GetColor( aLabel, XCAFDoc_ColorSurf, c ) )
            ss << ", sc: " << c;
        if( aColorTool->GetColor( aLabel, XCAFDoc_ColorCurv, c ) )
            ss << ", cc: " << c;
    }

    wxLogTrace( TRACE_MASK, ss.str().c_str() );
}


/**
 * Dumps a label and the entire tree underneath it
 *
 * @param aLabel Label to convert
 * @param aShapeTool Handle to shape tool being used
 * @param aColorTool Handle to color tool being used
 * @param aDepth Indentation level to offset labels (used recursively by dumpLabels)
 */
[[maybe_unused]] static void dumpLabels( TDF_Label aLabel, Handle( XCAFDoc_ShapeTool ) aShapeTool,
                                         Handle( XCAFDoc_ColorTool ) aColorTool, int aDepth = 0 )
{
    std::string indent( aDepth * 2, ' ' );
    printLabel( aLabel, aShapeTool, aColorTool, indent.c_str() );
    TDF_ChildIterator it;
    for( it.Initialize( aLabel ); it.More(); it.Next() )
        dumpLabels( it.Value(), aShapeTool, aColorTool, aDepth + 1 );
}


[[maybe_unused]] static bool isLabelABoardMesh( const TDF_Label& aLabel )
{
    Handle( KICAD3D_INFO ) c;
    if( aLabel.FindAttribute( KICAD3D_INFO::GetID(), c ) )
    {
        return c->GetModelType() == KICAD3D_MODEL_TYPE::BOARD;
    }

    return false;
}


void WRITER::getMeshName( const TDF_Label& label, Handle( XCAFDoc_ShapeTool ) shapeTool, MESH* mesh )
{
    Handle( TDataStd_Name ) n;

    Handle( KICAD3D_INFO ) c;
    if( label.FindAttribute( KICAD3D_INFO::GetID(), c ) && c->GetDisplayName() != "" )
    {
        mesh->name = wxGetTranslation( c->GetDisplayName() );
    }
    else
    {
        if( label.FindAttribute( TDataStd_Name::GetID(), n ) )
        {
            mesh->name = TCollection_AsciiString( n->Get() ).ToCString();
        }
    }


    // worst case, there should be a name 99.9% of the time
    // but lets avoid making the u3d bad
    if( mesh->name.empty() )
    {
        mesh->name = "NoName";
    }

    // U3D absolutely needs unique mesh names for each entry or parsing can barf in other tools
    // In the worst case there are duplicate mesh names due to our human input of ref designators
    // just break it up by appending a number
    if( m_meshDedupMap.contains( mesh->name ) )
    {
        m_meshDedupMap[mesh->name]++;
        mesh->name += std::to_string( m_meshDedupMap[mesh->name] );
    }
    else
    {
        m_meshDedupMap[mesh->name] = 1;
    }
}


void WRITER::collectGeometryRecursive( const TDF_Label& label, const Handle( XCAFDoc_ShapeTool ) & shapeTool,
                                       const Handle( XCAFDoc_ColorTool ) & colorTool,
                                       const Handle( XCAFDoc_VisMaterialTool ) & visMatTool,
                                       const gp_Trsf& cumulativeTransform,
                                       const std::string& baseName,
                                       std::unordered_map<Graphic3d_Vec4, MESH*>& meshesByColor )
{
    if( label.IsNull() )
        return;

    if( shapeTool->IsAssembly( label ) || shapeTool->IsReference( label ) )
    {
        TDF_LabelSequence childrenOrComponents;
        TDF_Label         referencedLabel;
        gp_Trsf           currentTransform = cumulativeTransform;
        bool              isRef = shapeTool->IsReference( label );

        if( isRef )
        {
            if( shapeTool->GetReferredShape( label, referencedLabel ) )
            {
                if( cumulativeTransform.Form() == gp_Identity )
                {
                    TopLoc_Location instanceLocation;
                    instanceLocation = shapeTool->GetLocation( label );
                    currentTransform = cumulativeTransform * instanceLocation.Transformation();
                }

                collectGeometryRecursive( referencedLabel, shapeTool, colorTool, visMatTool, currentTransform,
                                          baseName, meshesByColor );
            }
        }
        else
        {
            shapeTool->GetComponents( label, childrenOrComponents );
            for( Standard_Integer i = 1; i <= childrenOrComponents.Length(); ++i )
            {
                TDF_Label       compLabel = childrenOrComponents.Value( i );
                TopLoc_Location compLocation;
                gp_Trsf         childTransform = currentTransform; // Start with parent's transform

                compLocation = shapeTool->GetLocation( compLabel );
                childTransform = currentTransform * compLocation.Transformation();

                collectGeometryRecursive( compLabel, shapeTool, colorTool, visMatTool, childTransform, baseName,
                                          meshesByColor );
            }
        }
    }
    else if( shapeTool->IsShape( label ) )
    {
        TopoDS_Shape shape;
        if( shapeTool->GetShape( label, shape ) && !shape.IsNull() )
        {
            TopLoc_Location location;
            location = shapeTool->GetLocation( label );
            location = cumulativeTransform * location;

            for( RWMesh_FaceIterator faceIter( label, location, true ); faceIter.More(); faceIter.Next() )
            {
                if( faceIter.IsEmptyMesh() )
                    continue;

                Handle( Poly_Triangulation ) triangulation = faceIter.Triangulation();
                if( triangulation.IsNull() )
                    continue;

                Graphic3d_Vec4 aColorF = faceIter.FaceColor();
                Graphic3d_Vec4 specularColor( 0.2f );

                MESH* mesh = nullptr;
                auto  it = meshesByColor.find( aColorF );
                if( it == meshesByColor.end() )
                {
                    auto newMesh = std::make_unique<MESH>();
                    newMesh->name = baseName + "_" + std::to_string( meshesByColor.size() );
                    newMesh->parentName = baseName;
                    newMesh->diffuse_color = aColorF;
                    newMesh->specular_color = specularColor.rgb();
                    mesh = newMesh.get();
                    meshesByColor.emplace( aColorF, mesh );
                    m_meshes.emplace_back( std::move( newMesh ) );
                }
                else
                {
                    mesh = it->second;
                }

                uint32_t nodesExistingSum = mesh->coords.size();
                uint32_t numberTriangles = 0;
                uint32_t numberNodes = 0;

                const Standard_Integer aNodeUpper = faceIter.NodeUpper();
                numberNodes += faceIter.NbNodes();
                numberTriangles += faceIter.NbTriangles();

                mesh->coords.reserve( mesh->coords.size() + numberNodes );
                mesh->coordIndices.reserve( mesh->coordIndices.size() + ( numberTriangles * 3 ) );

                if( m_includeNormals )
                {
                    mesh->normalIndices.reserve( mesh->normalIndices.size() + ( numberTriangles * 3 ) );
                    mesh->normals.reserve( mesh->normals.size() + numberNodes );
                }

                for( Standard_Integer aNodeIter = faceIter.NodeLower(); aNodeIter <= aNodeUpper; ++aNodeIter )
                {
                    const gp_Dir aNormal = faceIter.NormalTransformed( aNodeIter );
                    gp_XYZ       vertex = faceIter.NodeTransformed( aNodeIter ).XYZ();

                    mesh->coords.emplace_back( vertex.X(), vertex.Y(), vertex.Z() );

                    if( m_includeNormals )
                        mesh->normals.emplace_back( aNormal.X(), aNormal.Y(), aNormal.Z() );

                    // update the bounding box
                    m_meshBoundingBox.Update( vertex.X(), vertex.Y(), vertex.Z() );
                }

                const Standard_Integer anElemLower = faceIter.ElemLower();
                const Standard_Integer anElemUpper = faceIter.ElemUpper();
                for( Standard_Integer anElemIter = anElemLower; anElemIter <= anElemUpper; ++anElemIter )
                {
                    const Poly_Triangle aTri = faceIter.TriangleOriented( anElemIter );

                    Graphic3d_Vec3i vec =
                            Graphic3d_Vec3i( aTri( 1 ), aTri( 2 ), aTri( 3 ) ) - Graphic3d_Vec3i( anElemLower );

                    mesh->coordIndices.emplace_back( vec.x() + nodesExistingSum );
                    mesh->coordIndices.emplace_back( vec.y() + nodesExistingSum );
                    mesh->coordIndices.emplace_back( vec.z() + nodesExistingSum );

                    if( m_includeNormals )
                    {
                        mesh->normalIndices.emplace_back( vec.x() + nodesExistingSum );
                        mesh->normalIndices.emplace_back( vec.y() + nodesExistingSum );
                        mesh->normalIndices.emplace_back( vec.z() + nodesExistingSum );
                    }
                }
            }
        }
    }
}


void WRITER::generateMeshesByAssembly( const Handle( TDocStd_Document ) & doc )
{
    m_meshes.clear();
    m_groupNodes.clear();
    m_meshDedupMap.clear();

    if( doc.IsNull() )
    {
        return;
    }

    Handle( XCAFDoc_ShapeTool ) shapeTool = XCAFDoc_DocumentTool::ShapeTool( doc->Main() );
    Handle( XCAFDoc_ColorTool ) colorTool = XCAFDoc_DocumentTool::ColorTool( doc->Main() );
    Handle( XCAFDoc_VisMaterialTool ) visMatTool = XCAFDoc_DocumentTool::VisMaterialTool( doc->Main() );

    if( shapeTool.IsNull() )
    {
        return;
    }

    TDF_LabelSequence meshableLabels;
    TDF_LabelSequence rootLabels;
    shapeTool->GetFreeShapes( rootLabels );

    /*
     * Let's find all valid labels to mesh depending on if they contain an attribute we applied when building the XCAFDOC
     * This lets us identify components as a whole rather than dealing with parts that may be made of multiple sub parts
     */
    std::function<void( const TDF_Label& )> recurseFindKiCadElements;
    recurseFindKiCadElements = [&]( const TDF_Label& label ) -> void
    {
        Handle( KICAD3D_INFO ) c;
        if( label.FindAttribute( KICAD3D_INFO::GetID(), c ) )
        {
            meshableLabels.Append( label );
            return;
        }

        TDF_LabelSequence childrenOrComponents;
        shapeTool->GetComponents( label, childrenOrComponents );
        for( Standard_Integer i = 1; i <= childrenOrComponents.Length(); ++i )
            recurseFindKiCadElements( childrenOrComponents.Value( i ) );
    };

    for( Standard_Integer i = 1; i <= rootLabels.Length(); ++i )
        recurseFindKiCadElements( rootLabels.Value( i ) );

    for( Standard_Integer i = 1; i <= meshableLabels.Length(); ++i )
    {
        TDF_Label meshLabel = meshableLabels.Value( i );
        if( !( shapeTool->IsAssembly( meshLabel ) || shapeTool->IsShape( meshLabel ) ) )
            continue;

        std::string rootGroupName; // Board or Components
        Handle( KICAD3D_INFO ) c;
        if( meshLabel.FindAttribute( KICAD3D_INFO::GetID(), c ) )
        {
            rootGroupName = c->GetModelType() == KICAD3D_MODEL_TYPE::BOARD
                                     ? _( MODEL_PARENT_BOARD_NAME ).ToStdString()
                                     : _( MODEL_PARENT_COMPONENTS_NAME ).ToStdString();
        }

        // Derive component/group name using existing naming logic
        std::unique_ptr<MESH> dummyNameMesh = std::make_unique<MESH>();
        getMeshName( meshLabel, shapeTool, dummyNameMesh.get() );
        std::string topName = dummyNameMesh->name;

        // Create group node for this top-level shape with parent = rootGroupName (unless identical)
        GROUP_NODE gn; gn.name = topName;
        if( !rootGroupName.empty() && topName != rootGroupName )
        {
            PARENT_NODE pn; pn.name = rootGroupName; pn.mat = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
            gn.parentNodes.push_back( pn );
        }
        m_groupNodes.push_back( gn );

        std::unordered_map<Graphic3d_Vec4, MESH*> meshesByColor;
        gp_Trsf initialTransform; // identity
        collectGeometryRecursive( meshLabel, shapeTool, colorTool, visMatTool, initialTransform,
                                  topName, meshesByColor );
    }

    std::sort( m_meshes.begin(), m_meshes.end(),
               []( const std::unique_ptr<MESH>& a, const std::unique_ptr<MESH>& b )
               {
                   return a->name < b->name;
               } );
}


WRITER::WRITER( const std::string& aFilename ) :
        m_filename( aFilename ),
        m_center( 0, 0, 0 ),
        m_meshBoundingBox(),
        m_meshDedupMap(),
        m_includeNormals( false )
{
}


void WRITER::writeMatrix( BIT_STREAM_WRITER& aBitStreamWriter, const std::vector<float>& aMat )
{
    for( size_t i = 0; i < aMat.size(); i++ )
        aBitStreamWriter.WriteF32( aMat[i] );
}


std::shared_ptr<DATA_BLOCK> WRITER::getGroupNodeBlock( const std::string& aGroupNodeName,
                                                       const PARENT_NODE* aParentNode )
{
    BIT_STREAM_WRITER w;
    uint32_t          parentNodeCount = aParentNode == nullptr ? 0 : 1;

    w.WriteString( aGroupNodeName ); // model node name
    w.WriteU32( parentNodeCount );   // parent node count

    if( parentNodeCount > 0 )
    {
        w.WriteString( aParentNode->name ); // parent node name
        writeMatrix( w, aParentNode->mat ); // transformation
    }

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::GROUP_NODE );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getModelNodeBlock( const std::string&        aModelNodeName,
                                                       const std::string&        aParentNodeName,
                                                       const std::string&        aModelResourceName,
                                                       const std::vector<float>& aMat )
{
    BIT_STREAM_WRITER w;
    w.WriteString( aModelNodeName );

    uint32_t parentNodecount = aParentNodeName.empty() ? 0 : 1;
    w.WriteU32( parentNodecount ); // parent node count
    if( parentNodecount )
    {
        w.WriteString( aParentNodeName ); // parent node name
        writeMatrix( w, aMat );           // transformation
    }

    w.WriteString( aModelResourceName ); // model resource name
    w.WriteU32( 3 );                     // visibility 3 = front and back

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::MODEL_NODE );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getShadingModifierBlock( const std::string& aShadingModName,
                                                             const std::string& aShaderName )
{
    BIT_STREAM_WRITER w;
    w.WriteString( aShadingModName ); // shading modifier name
    w.WriteU32( 1 );                  // chain index
    w.WriteU32( SHADING_ATTRIBUTES::POINT
                | SHADING_ATTRIBUTES::MESH
                | SHADING_ATTRIBUTES::LINE
                | SHADING_ATTRIBUTES::GLYPH ); // shading attributes
    w.WriteU32( 1 );                  // shading list count
    w.WriteU32( 1 );                  // shader count
    w.WriteString( aShaderName );     // shader name
    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::SHADING_MODIFIER );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getModelResourceModifierChain( const std::string& aModifierChainName,
                                                                   const MESH* aMesh, const std::string& aMeshname )
{
    BIT_STREAM_WRITER w;

    w.WriteString( aModifierChainName ); // modifier chain name
    w.WriteU32( 1 );                     // modifier chain type: 1 = model resource modifier chain
    w.WriteU32( 0 );                     // modifier chain attributes: 0 = neither bounding sphere nor
                                         // bounding box info present
    // padding
    w.AlignTo4Byte();
    w.WriteU32( 1 ); // modifier count in this chain

    w.WriteDataBlock( getMeshDeclarationBlock( aMesh, aMeshname ) );

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::MODIFIER_CHAIN );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getGroupNodeModifierChain( const std::string&             aModifierChainName,
                                                               const std::vector<GROUP_NODE>& aGroupNodes )
{
    BIT_STREAM_WRITER w;
    w.WriteString( aModifierChainName ); // modifier chain name
    w.WriteU32( 0 );                     // modifier chain type: 0 = node modifier chain
    w.WriteU32( 0 );                     // modifier chain attributes: 0 = neither bounding sphere nor
                                         // bounding box info present
    w.AlignTo4Byte();
    w.WriteU32( aGroupNodes.size() ); // modifier count in this chain

    for( const auto& groupNode : aGroupNodes )
    {
        const PARENT_NODE* parent = groupNode.parentNodes.empty() ? nullptr : &groupNode.parentNodes[0];
        w.WriteDataBlock( getGroupNodeBlock( groupNode.name, parent ) );
    }

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::MODIFIER_CHAIN );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getNodeModifierChain( const std::string& aModifierChainName,
                                                          const std::string& aModelNodeName,
                                                          const std::string& aParentNodeName,
                                                          const std::string& aModelResourceName,
                                                          const std::string& aShaderName,
                                                          const std::vector<float>& aMat )
{
    BIT_STREAM_WRITER w;
    w.WriteString( aModifierChainName ); // modifier chain name
    w.WriteU32( 0 );                     // modifier chain type: 0 = node modifier chain
    w.WriteU32( 0 );                     // modifier chain attributes: 0 = neither bounding sphere nor
                                         // bounding box info present
    w.AlignTo4Byte();
    w.WriteU32( 2 ); // modifier count in this chain

    w.WriteDataBlock( getModelNodeBlock( aModelNodeName, aParentNodeName, aModelResourceName, aMat ) );
    w.WriteDataBlock( getShadingModifierBlock( aModifierChainName, aShaderName ) );

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::MODIFIER_CHAIN );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getHeaderBlock( uint32_t aDeclSize, uint32_t aContSize )
{
    const uint32_t headerBlockSize = 36;

    BIT_STREAM_WRITER w;
    w.WriteU16( 256 );                                     // major version
    w.WriteU16( 0 );                                       // minor version
    w.WriteU32( HEADER_PROFILE_FLAGS::DEFINED_UNITS );     // profile identifier
    w.WriteU32( headerBlockSize + aDeclSize );             // declaration size
    w.WriteU64( headerBlockSize + aDeclSize + aContSize ); // file size
    w.WriteU32( 106 );                                     // character encoding: 106 = UTF-8

    w.WriteF64( 0.001f ); // units scale factor (1/1000th of a unit, 1mm = 0.001m)

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::FILE_HEADER );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getLitTextureShaderBlock( const std::string& aShaderName,
                                                              const std::string& aMaterialName )
{
    BIT_STREAM_WRITER w;

    w.WriteString( aShaderName );
    w.WriteU32( LIT_TEXTURE_SHADER_ATTRIBUTES::LIGHTING_ENABLED );

    w.WriteF32( 0 );                                                          // Alpha Test Reference
    w.WriteU32( static_cast<uint32_t>( ALPHA_TEST_FUNCTION::ALWAYS ) );       // Alpha Test Function
    w.WriteU32( static_cast<uint32_t>( COLOR_BLEND_FUNCTION::ALPHA_BLEND ) ); // Color Blend Function

    w.WriteU32( 1 );               // Render pass enabled flags (each bit represents a shader enabled)
    w.WriteU32( 0 );               // Shader channels
    w.WriteU32( 0 );               // Alpha texture channels

    w.WriteString( aMaterialName ); // Material name

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::LIT_TEXTURE_SHADER );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getMaterialResourceBlock( const std::string& aMaterialName,
                                                              const Graphic3d_Vec4&     aDiffuseColor,
                                                              const Graphic3d_Vec3&     aSpecularColor )
{
    BIT_STREAM_WRITER    w;

    w.WriteString( aMaterialName ); // Material resource name
    w.WriteU32( MATERIAL_ATTRIBUTES::OPACITY
                | MATERIAL_ATTRIBUTES::SPECULAR
                | MATERIAL_ATTRIBUTES::DIFFUSE
                | MATERIAL_ATTRIBUTES::REFLECTIVITY );
    w.WriteF32( 0.0f );               // Ambient red
    w.WriteF32( 0.0f );               // Ambient green
    w.WriteF32( 0.0f );               // Ambient blue
    w.WriteF32( aDiffuseColor.r() );  // Diffuse red
    w.WriteF32( aDiffuseColor.g() );  // Diffuse green
    w.WriteF32( aDiffuseColor.b() );  // Diffuse blue
    w.WriteF32( aSpecularColor.r() ); // Specular red
    w.WriteF32( aSpecularColor.g() ); // Specular green
    w.WriteF32( aSpecularColor.b() ); // Specular blue
    w.WriteF32( 0.0f );               // Emissive red
    w.WriteF32( 0.0f );               // Emissive green
    w.WriteF32( 0.0f );               // Emissive blue
    w.WriteF32( 0.32f );              // Reflectivity
    w.WriteF32( aDiffuseColor.a() );  // Opacity

    std::shared_ptr<DATA_BLOCK> block = w.GetDataBlock();
    block->SetBlockType( BLOCK_TYPES::MATERIAL_RESOURCE );

    return block;
}


std::shared_ptr<DATA_BLOCK> WRITER::getMeshDeclarationBlock( const MESH* aMesh,
                                                             const std::string& aMeshName )
{
    BIT_STREAM_WRITER w;

    w.WriteString( aMeshName ); // mesh name
    w.WriteU32( 0 );           // chain index

    // max mesh description
    w.WriteU32( aMesh->normals.size() > 0 ? 0 : 1 ); // mesh attributes: 1 = no normals

    w.WriteU32( aMesh->coordIndices.size() / 3 ); // face count
    w.WriteU32( aMesh->coords.size() );           // positions count
    w.WriteU32( aMesh->normals.size() );         // normal count

    w.WriteU32( aMesh->diffuse_colors.size() ); // diffuse color count
    w.WriteU32( aMesh->specular_colors.size() ); // specular color count
    w.WriteU32( 0 );                            // texture coord count
    w.WriteU32( 1 );                            // shading count

    // shading description
    uint32_t shadingAttributes = 0;

    if( aMesh->diffuse_colors.size() > 0 )
    {
        shadingAttributes |= 0x01; // per vertex diffuse colors
    }

    if( aMesh->specular_colors.size() > 0 )
    {
        shadingAttributes |= 0x02; // per vertex specular colors
    }

    w.WriteU32( shadingAttributes ); // shading attributes: 0 = the shader list uses neither diffuse nor specular colors
    w.WriteU32( 0 ); // texture layer count
    // texture coord dimensions (skipped because texture layer count is 0)
    w.WriteU32( 0 ); // original shader id

    // clod desc
    w.WriteU32( 0 ); // minimum resolution
    w.WriteU32( aMesh->coords.size() ); // maximum resolution
    w.WriteU32( 300 );                // position quality factor
    w.WriteU32( 300 );                // normal quality factor
    w.WriteU32( 300 );                // texture coord quality factor
    w.WriteF32( 0.001f );              // position inverse quant
    w.WriteF32( 0.001f );              // normal inverse quant
    w.WriteF32( 0.001f );              // texture coord inverse quant
    w.WriteF32( 0.001f );              // diffuse color inverse quant
    w.WriteF32( 0.001f );              // specular color inverse quant
    w.WriteF32( 0.9f );               // normal crease parameter
    w.WriteF32( 0.5f );               // normal update parameter
    w.WriteF32( 0.985f );             // normal tolerance parameter
    w.WriteU32( 0 );                  // bone count

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::MESH_DECLARATION );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getMeshContinuationBlock( const MESH* aMesh,
                                                              const std::string& aMeshname )
{
    BIT_STREAM_WRITER w;

    w.WriteString( aMeshname );                  // mesh name
    w.WriteU32( 0 );                            // chain index
    w.WriteU32( aMesh->coordIndices.size() / 3 ); // base face count
    w.WriteU32( aMesh->coords.size() );           // base positions count
    w.WriteU32( aMesh->normals.size() );          // base normal count
    w.WriteU32( aMesh->diffuse_colors.size() );   // base diffuse color count

    w.WriteU32( aMesh->specular_colors.size() ); // base specular color count
    w.WriteU32( 0 ); // base texture coord count

    for( size_t i = 0; i < aMesh->coords.size(); i++ )
    {
        w.WriteF32( aMesh->coords[i].x ); // base position x
        w.WriteF32( aMesh->coords[i].y ); // base position y
        w.WriteF32( aMesh->coords[i].z ); // base position z
    }

    for( size_t i = 0; i < aMesh->normals.size(); i++ )
    {
        w.WriteF32( aMesh->normals[i].x ); // base normal x
        w.WriteF32( aMesh->normals[i].y ); // base normal y
        w.WriteF32( aMesh->normals[i].z ); // base normal z
    }

    for( size_t i = 0; i < aMesh->diffuse_colors.size(); i++ )
    {
        w.WriteF32( aMesh->diffuse_colors[i].r() ); // base colors red
        w.WriteF32( aMesh->diffuse_colors[i].g() );   // base colors green
        w.WriteF32( aMesh->diffuse_colors[i].b() ); // base colors blue
        w.WriteF32( aMesh->diffuse_colors[i].a() );   // base colors alpha
    }

    for( size_t i = 0; i < aMesh->specular_colors.size(); i++ )
    {
        w.WriteF32( aMesh->specular_colors[i].r() ); // base colors red
        w.WriteF32( aMesh->specular_colors[i].g() ); // base colors green
        w.WriteF32( aMesh->specular_colors[i].b() ); // base colors blue
        w.WriteF32( aMesh->specular_colors[i].a() ); // base colors alpha
    }

    for( size_t i = 0; i < aMesh->coordIndices.size(); i += 3 )
    {
        w.WriteCompressedU32( m_contextBaseShadingID, 0 ); // shading id
        for( int j = 0; j < 3; j++ )
        {
            w.WriteCompressedU32( CONSTANTS::StaticFull + aMesh->coords.size(), aMesh->coordIndices[i + j] );
            if( aMesh->normals.size() > 0 )
                w.WriteCompressedU32( CONSTANTS::StaticFull + aMesh->normals.size(),
                                      aMesh->normalIndices[i + j] );

            if( aMesh->diffuse_colors.size() > 0 && aMesh->diffuseColorIndices.size() > 0 )
                w.WriteCompressedU32( CONSTANTS::StaticFull + aMesh->diffuse_colors.size(),
                                      aMesh->diffuseColorIndices[i + j] );

            if( aMesh->specular_colors.size() > 0 && aMesh->specularColorIndices.size() > 0 )
                w.WriteCompressedU32( CONSTANTS::StaticFull + aMesh->specular_colors.size(),
                                      aMesh->specularColorIndices[i + j] );
        }
    }

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::MESH_CONTINUATION );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getLightModifierChain( const std::string& aModifierChainName,
                                                           const std::string& aLightResourceName )
{
    BIT_STREAM_WRITER w;
    w.WriteString( aModifierChainName ); // modifier chain name
    w.WriteU32( 0 );                     // modifier chain type: 0 = node modifier chain
    w.WriteU32( 0 );                     // modifier chain attributes: 0 = neither bounding sphere nor
                                         // bounding box info present
    w.AlignTo4Byte();
    w.WriteU32( 1 ); // modifier count in this chain

    w.WriteDataBlock( getLightNodeBlock( aModifierChainName, aLightResourceName ) );

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::MODIFIER_CHAIN );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getLightNodeBlock( const std::string& aLightNodeName,
                                                       const std::string& aLightResourceName )
{
    std::vector<float> matrix = std::vector<float>{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 6.f, -20.0f, 4.0f, 1.0f };
    BIT_STREAM_WRITER  w;
    w.WriteString( aLightNodeName );     // light node name
    w.WriteU32( 1 );                     // parent node count
    w.WriteString( "" );                 // parent node name
    writeMatrix( w, matrix );            // transformation
    w.WriteString( aLightResourceName ); // Light resource name

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::LIGHT_NODE );
    return b;
}


std::shared_ptr<DATA_BLOCK> WRITER::getLightResourceBlock( const std::string& aLightResourceName )
{
    BIT_STREAM_WRITER w;
    w.WriteString( aLightResourceName );
    w.WriteU32( 1 );        // Light attributes: enable
    w.WriteU8( (short) 2 ); // Light type: Point light
    w.WriteF32( .5f );      // Light color red
    w.WriteF32( .5f );      // Light color green
    w.WriteF32( .5f );      // Light color blue
    w.WriteF32( 1.0f );     // Reserved, shall be 1
    w.WriteF32( 0.1f );     // Light attenuation constant factor
    w.WriteF32( 0.0f );     // Light attenuation linear factor
    w.WriteF32( 0.f );      // Light attenuation quadratic factor
    w.WriteF32( 180.f );    // Light spot angle
    w.WriteF32( .5f );      // Light intensity

    std::shared_ptr<DATA_BLOCK> b = w.GetDataBlock();
    b->SetBlockType( BLOCK_TYPES::LIGHT_RESOURCE );
    return b;
}


uint32_t WRITER::writeDataBlock( std::shared_ptr<DATA_BLOCK> aBlock, wxMemoryOutputStream& aStream )
{
    uint32_t dataSize = (uint32_t) ceil( aBlock->GetDataSize() / 4.0 );
    uint32_t metaDataSize = (uint32_t) ceil( aBlock->GetMetaDataSize() / 4.0 );


    uint32_t blockLength = ( 4 + 4 + 4 + 4 * ( dataSize + metaDataSize ) );

    wxDataOutputStream workWriter( aStream );

    workWriter.Write32( aBlock->GetBlockType() );
    workWriter.Write32( aBlock->GetDataSize() );
    workWriter.Write32( aBlock->GetMetaDataSize() );
    for( uint32_t i = 0; i < dataSize; i++ )
    {
        workWriter.Write32( aBlock->GetData()[i] );
    }

    for( uint32_t i = 0; i < metaDataSize; i++ )
    {
        workWriter.Write32( aBlock->GetMetaData()[i] );
    }

    return blockLength;
}


bool WRITER::Perform( const Handle( TDocStd_Document ) & aDocument )
{
    // This is just a plain identity transformation matrix needed to be passed for some blocks
    // because u3d allows transformations being applied. However we basically have the mesh
    // already transformed in the right place so we just pass the identity matrix
    std::vector<float> matrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

    m_meshBoundingBox.SetVoid();
    m_meshDedupMap.clear();

    generateMeshesByAssembly( aDocument );

    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    m_meshBoundingBox.Get( xMin, yMin, zMin, xMax, yMax, zMax );

    Standard_Real centerX = ( xMin + xMax ) / 2.0;
    Standard_Real centerY = ( yMin + yMax ) / 2.0;
    Standard_Real centerZ = ( zMin + zMax ) / 2.0;

    m_center = VECTOR3D( centerX, centerY, centerZ );

    // Lets start creating the U3D file

    wxMemoryOutputStream decStream;
    wxMemoryOutputStream contStream;
    wxMemoryOutputStream outStream;

    uint32_t decSize = 0;
    uint32_t contSize = 0;

    std::vector<GROUP_NODE> baseGroupNodes;
    baseGroupNodes.push_back( { _( MODEL_PARENT_BOARD_NAME ).ToStdString(), {} } );
    baseGroupNodes.push_back( { _( MODEL_PARENT_COMPONENTS_NAME ).ToStdString(), {} } );

    // include dynamic top-level component/group names
    std::vector<GROUP_NODE> allGroups = baseGroupNodes;
    allGroups.insert( allGroups.end(), m_groupNodes.begin(), m_groupNodes.end() );

    decSize += writeDataBlock( getGroupNodeModifierChain( "groups", allGroups ), decStream );

    for( const std::unique_ptr<MESH>& mesh : m_meshes )
    {
        if( mesh->IsEmpty() )
            continue;

        std::string meshName = "n_" + mesh->name;

        std::string modelResourceName = "n_" + mesh->name;
        std::string matName = "m_" + mesh->name;
        std::string shaderName = "s_" + mesh->name;
        std::string modelModifierChainName = "n_" + mesh->name;

        decSize += writeDataBlock( getNodeModifierChain( meshName, mesh->name, mesh->parentName,
                                                         modelResourceName, shaderName, matrix ),
                                   decStream );

        decSize += writeDataBlock( getModelResourceModifierChain( modelModifierChainName, mesh.get(), meshName ),
                                   decStream );

        decSize += writeDataBlock( getLitTextureShaderBlock( shaderName, matName ), decStream );

        decSize += writeDataBlock( getMaterialResourceBlock( matName, mesh->diffuse_color, mesh->specular_color ),
                                   decStream );

        contSize += writeDataBlock( getMeshContinuationBlock( mesh.get(), meshName ), contStream );
    }

    // finish it, we need the size totals above to generate the header
    writeDataBlock( getHeaderBlock( decSize, contSize ), outStream );

    wxStreamBuffer* buffer = outStream.GetOutputStreamBuffer();

    std::ofstream u3dFile( m_filename, std::ios::binary );

    if( !u3dFile.is_open() )
    {
        wxLogTrace( TRACE_MASK, wxString::Format( "Error opening file:%s", m_filename ) );
        return false;
    }

    u3dFile.write( static_cast<const char*>( buffer->GetBufferStart() ), buffer->GetBufferSize() );

    wxStreamBuffer* decBuffer = decStream.GetOutputStreamBuffer();
    u3dFile.write( static_cast<const char*>( decBuffer->GetBufferStart() ), decBuffer->GetBufferSize() );

    wxStreamBuffer* contBuffer = contStream.GetOutputStreamBuffer();
    u3dFile.write( static_cast<const char*>( contBuffer->GetBufferStart() ), contBuffer->GetBufferSize() );

    u3dFile.close();

    return true;
}