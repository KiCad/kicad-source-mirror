/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2014-2017  Cirilo Bernardo
 * Copyright (C) 2018 Jean-Pierre Charras jp.charras at wanadoo.fr
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

#include <exception>
#include <fstream>
#include <iomanip>
#include <vector>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>

#include "3d_cache/3d_cache.h"
#include "3d_cache/3d_info.h"
#include "board.h"
#include "board_design_settings.h"
#include <footprint_library_adapter.h>
#include "footprint.h"
#include "pad.h"
#include "pcb_text.h"
#include "pcb_track.h"
#include <project_pcb.h>
#include <core/arraydim.h>
#include <filename_resolver.h>
#include "plugins/3dapi/ifsg_all.h"
#include "streamwrapper.h"
#include "vrml_layer.h"
#include "pcb_edit_frame.h"

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h>
#include <macros.h>

#include <exporter_vrml.h>

EXPORTER_VRML::EXPORTER_VRML( BOARD* aBoard )
{
    pcb_exporter = new EXPORTER_PCB_VRML( aBoard );
}


bool EXPORTER_VRML::ExportVRML_File( PROJECT* aProject, wxString *aMessages,
                              const wxString& aFullFileName, double aMMtoWRMLunit,
                              bool aIncludeUnspecified, bool aIncludeDNP,
                              bool aExport3DFiles, bool aUseRelativePaths,
                              const wxString& a3D_Subdir,
                              double aXRef, double aYRef )
{
    return pcb_exporter->ExportVRML_File( aProject, aMessages,
                                          aFullFileName, aMMtoWRMLunit,
                                          aIncludeUnspecified, aIncludeDNP,
                                          aExport3DFiles, aUseRelativePaths,
                                          a3D_Subdir, aXRef, aYRef );
}


EXPORTER_VRML::~EXPORTER_VRML()
{
    delete pcb_exporter;
}


// The max error (in mm) to approximate arcs to segments:
#define ERR_APPROX_MAX_MM 0.005


CUSTOM_COLORS_LIST   EXPORTER_PCB_VRML::m_SilkscreenColors;
CUSTOM_COLORS_LIST   EXPORTER_PCB_VRML::m_MaskColors;
CUSTOM_COLORS_LIST   EXPORTER_PCB_VRML::m_PasteColors;
CUSTOM_COLORS_LIST   EXPORTER_PCB_VRML::m_FinishColors;
CUSTOM_COLORS_LIST   EXPORTER_PCB_VRML::m_BoardColors;

KIGFX::COLOR4D       EXPORTER_PCB_VRML::m_DefaultSilkscreen;
KIGFX::COLOR4D       EXPORTER_PCB_VRML::m_DefaultSolderMask;
KIGFX::COLOR4D       EXPORTER_PCB_VRML::m_DefaultSolderPaste;
KIGFX::COLOR4D       EXPORTER_PCB_VRML::m_DefaultSurfaceFinish;
KIGFX::COLOR4D       EXPORTER_PCB_VRML::m_DefaultBoardBody;

static bool          g_ColorsLoaded = false;


EXPORTER_PCB_VRML::EXPORTER_PCB_VRML( BOARD* aBoard ) :
        m_OutputPCB( nullptr )
{
    m_board = aBoard;
    m_ReuseDef = true;
    m_precision = 6;
    m_WorldScale = 1.0;
    m_Cache3Dmodels = nullptr;
    m_includeDNP = false;
    m_includeUnspecified = false;
    m_UseInlineModelsInBrdfile = false;
    m_UseRelPathIn3DModelFilename = false;
    m_BoardToVrmlScale = pcbIUScale.MM_PER_IU;

    for( int ii = 0; ii < VRML_COLOR_LAST; ++ii )
        m_sgmaterial[ii] = nullptr;

    // this default only makes sense if the output is in mm
    m_brd_thickness = pcbIUScale.IUTomm( m_board->GetDesignSettings().GetBoardThickness() );

    // TODO: figure out a way to share all these stackup color definitions...
    initStaticColorList();

    COLOR4D topSilk = m_DefaultSilkscreen;
    COLOR4D botSilk = m_DefaultSilkscreen;
    COLOR4D topMask = m_DefaultSolderMask;
    COLOR4D botMask = m_DefaultSolderMask;
    COLOR4D paste   = m_DefaultSolderPaste;
    COLOR4D finish  = m_DefaultSurfaceFinish;
    COLOR4D boardBody( 0, 0, 0, 0 );

    const BOARD_STACKUP& stackup = m_board->GetDesignSettings().GetStackupDescriptor();

    // Can't do a const KIGFX::COLOR4D& return type here because there are temporary variables
    auto findColor =
            []( const wxString& aColorName, const CUSTOM_COLORS_LIST& aColorSet ) -> const KIGFX::COLOR4D
            {
                if( aColorName.StartsWith( wxT( "#" ) ) )
                {
                    return KIGFX::COLOR4D( aColorName );
                }
                else
                {
                    for( const CUSTOM_COLOR_ITEM& color : aColorSet )
                    {
                        if( color.m_ColorName == aColorName )
                            return color.m_Color;
                    }
                }

                return KIGFX::COLOR4D();
            };

    for( const BOARD_STACKUP_ITEM* stackupItem : stackup.GetList() )
    {
        wxString colorName = stackupItem->GetColor();

        switch( stackupItem->GetType() )
        {
        case BS_ITEM_TYPE_SILKSCREEN:
            if( stackupItem->GetBrdLayerId() == F_SilkS )
                topSilk = findColor( colorName, m_SilkscreenColors );
            else
                botSilk = findColor( colorName, m_SilkscreenColors );
            break;

        case BS_ITEM_TYPE_SOLDERMASK:
            if( stackupItem->GetBrdLayerId() == F_Mask )
                topMask = findColor( colorName, m_MaskColors );
            else
                botMask = findColor( colorName, m_MaskColors );

            break;

        case BS_ITEM_TYPE_DIELECTRIC:
        {
            KIGFX::COLOR4D layerColor = findColor( colorName, m_BoardColors );

            if( boardBody == COLOR4D( 0, 0, 0, 0 ) )
                boardBody = layerColor;
            else
                boardBody = boardBody.Mix( layerColor, 1.0 - layerColor.a );

            boardBody.a += ( 1.0 - boardBody.a ) * layerColor.a / 2;
            break;
        }

        default:
            break;
        }
    }

    if( boardBody == COLOR4D( 0, 0, 0, 0 ) )
        boardBody = m_DefaultBoardBody;

    const wxString& finishName = stackup.m_FinishType;

    if( finishName.EndsWith( wxT( "OSP" ) ) )
    {
        finish = findColor( wxT( "Copper" ), m_FinishColors );
    }
    else if( finishName.EndsWith( wxT( "IG" ) )
          || finishName.EndsWith( wxT( "gold" ) ) )
    {
        finish = findColor( wxT( "Gold" ), m_FinishColors );
    }
    else if( finishName.StartsWith( wxT( "HAL" ) )
          || finishName.StartsWith( wxT( "HASL" ) )
          || finishName.EndsWith( wxT( "tin" ) )
          || finishName.EndsWith( wxT( "nickel" ) ) )
    {
        finish = findColor( wxT( "Tin" ), m_FinishColors );
    }
    else if( finishName.EndsWith( wxT( "silver" ) ) )
    {
        finish = findColor( wxT( "Silver" ), m_FinishColors );
    }

    auto toVRMLColor =
            []( const COLOR4D& aColor, double aSpecular, double aAmbient, double aShiny )
            {
                COLOR4D diff  = aColor;
                COLOR4D spec  = aColor.Brightened( aSpecular );

                return VRML_COLOR( diff.r, diff.g, diff.b,
                                   spec.r, spec.g, spec.b,
                                   aAmbient, 1.0 - aColor.a, aShiny );
            };

    vrml_colors_list[VRML_COLOR_TOP_SILK] =     toVRMLColor( topSilk,   0.1, 0.7, 0.02 );
    vrml_colors_list[VRML_COLOR_BOT_SILK] =     toVRMLColor( botSilk,   0.1, 0.7, 0.02 );
    vrml_colors_list[VRML_COLOR_TOP_SOLDMASK] = toVRMLColor( topMask,   0.3, 0.8, 0.30 );
    vrml_colors_list[VRML_COLOR_BOT_SOLDMASK] = toVRMLColor( botMask,   0.3, 0.8, 0.30 );
    vrml_colors_list[VRML_COLOR_PASTE] =        toVRMLColor( paste,     0.6, 0.7, 0.70 );
    vrml_colors_list[VRML_COLOR_COPPER] =       toVRMLColor( finish,    0.6, 0.7, 0.90 );
    vrml_colors_list[VRML_COLOR_PCB] =          toVRMLColor( boardBody, 0.1, 0.7, 0.01 );

    SetOffset( 0.0, 0.0 );
}


EXPORTER_PCB_VRML::~EXPORTER_PCB_VRML()
{
    // destroy any unassociated material appearances
    for( int j = 0; j < VRML_COLOR_LAST; ++j )
    {
        if( m_sgmaterial[j] && nullptr == S3D::GetSGNodeParent( m_sgmaterial[j] ) )
            S3D::DestroyNode( m_sgmaterial[j] );

        m_sgmaterial[j] = nullptr;
    }

    if( !m_components.empty() )
    {
        IFSG_TRANSFORM tmp( false );

        for( auto i : m_components )
        {
            tmp.Attach( i );
            tmp.SetParent( nullptr );
        }

        m_components.clear();
        m_OutputPCB.Destroy();
    }
}

void EXPORTER_PCB_VRML::initStaticColorList()
{
    // Initialize the list of colors used in VRML export, but only once.
    // (The list is static)
    if( g_ColorsLoaded )
        return;

#define ADD_COLOR( list, r, g, b, a, name ) \
    list.emplace_back( r/255.0, g/255.0, b/255.0, a, name )

    ADD_COLOR( m_SilkscreenColors, 245, 245, 245, 1.0, _HKI( "Not specified" ) ); // White
    ADD_COLOR( m_SilkscreenColors,  20,  51,  36, 1.0, wxT( "Green" ) );
    ADD_COLOR( m_SilkscreenColors, 181,  19,  21, 1.0, wxT( "Red" ) );
    ADD_COLOR( m_SilkscreenColors,   2,  59, 162, 1.0, wxT( "Blue" ) );
    ADD_COLOR( m_SilkscreenColors,  11,  11,  11, 1.0, wxT( "Black" ) );
    ADD_COLOR( m_SilkscreenColors, 245, 245, 245, 1.0, wxT( "White" ) );
    ADD_COLOR( m_SilkscreenColors,  32,   2,  53, 1.0, wxT( "Purple" ) );
    ADD_COLOR( m_SilkscreenColors, 194,  195,  0, 1.0, wxT( "Yellow" ) );

    ADD_COLOR( m_MaskColors,  20,  51,  36, 0.83, _HKI( "Not specified" ) ); // Green
    ADD_COLOR( m_MaskColors,  20,  51,  36, 0.83, wxT( "Green" ) );
    ADD_COLOR( m_MaskColors,  91, 168,  12, 0.83, wxT( "Light Green" ) );
    ADD_COLOR( m_MaskColors,  13, 104,  11, 0.83, wxT( "Saturated Green" ) );
    ADD_COLOR( m_MaskColors, 181,  19,  21, 0.83, wxT( "Red" ) );
    ADD_COLOR( m_MaskColors, 210,  40,  14, 0.83, wxT( "Light Red" ) );
    ADD_COLOR( m_MaskColors, 239,  53,  41, 0.83, wxT( "Red/Orange" ) );
    ADD_COLOR( m_MaskColors,   2,  59, 162, 0.83, wxT( "Blue" ) );
    ADD_COLOR( m_MaskColors,  54,  79, 116, 0.83, wxT( "Light Blue 1" ) );
    ADD_COLOR( m_MaskColors,  61,  85, 130, 0.83, wxT( "Light Blue 2" ) );
    ADD_COLOR( m_MaskColors,  21,  70,  80, 0.83, wxT( "Green/Blue" ) );
    ADD_COLOR( m_MaskColors,  11,  11,  11, 0.83, wxT( "Black" ) );
    ADD_COLOR( m_MaskColors, 245, 245, 245, 0.83, wxT( "White" ) );
    ADD_COLOR( m_MaskColors,  32,   2,  53, 0.83, wxT( "Purple" ) );
    ADD_COLOR( m_MaskColors, 119,  31,  91, 0.83, wxT( "Light Purple" ) );
    ADD_COLOR( m_MaskColors, 194,  195,  0, 0.83, wxT( "Yellow" ) );

    ADD_COLOR( m_PasteColors, 128, 128, 128, 1.0, wxT( "Grey" ) );
    ADD_COLOR( m_PasteColors,  90,  90,  90, 1.0, wxT( "Dark Grey" ) );
    ADD_COLOR( m_PasteColors, 213, 213, 213, 1.0, wxT( "Silver" ) );

    ADD_COLOR( m_FinishColors, 184, 115,  50, 1.0, wxT( "Copper" ) );
    ADD_COLOR( m_FinishColors, 178, 156,   0, 1.0, wxT( "Gold" ) );
    ADD_COLOR( m_FinishColors, 213, 213, 213, 1.0, wxT( "Silver" ) );
    ADD_COLOR( m_FinishColors, 160, 160, 160, 1.0, wxT( "Tin" ) );

    ADD_COLOR( m_BoardColors,  51,  43,  22, 0.83, wxT( "FR4 natural, dark" ) );
    ADD_COLOR( m_BoardColors, 109, 116,  75, 0.83, wxT( "FR4 natural" ) );
    ADD_COLOR( m_BoardColors, 252, 252, 250, 0.90, wxT( "PTFE natural" ) );
    ADD_COLOR( m_BoardColors, 205, 130,   0, 0.68, wxT( "Polyimide" ) );
    ADD_COLOR( m_BoardColors,  92,  17,   6, 0.90, wxT( "Phenolic natural" ) );
    ADD_COLOR( m_BoardColors, 146,  99,  47, 0.83, wxT( "Brown 1" ) );
    ADD_COLOR( m_BoardColors, 160, 123,  54, 0.83, wxT( "Brown 2" ) );
    ADD_COLOR( m_BoardColors, 146,  99,  47, 0.83, wxT( "Brown 3" ) );
    ADD_COLOR( m_BoardColors, 213, 213, 213,  1.0, wxT( "Aluminum" ) );

    m_DefaultSilkscreen =    COLOR4D( 0.94, 0.94, 0.94,  1.0 );
    m_DefaultSolderMask =    COLOR4D( 0.08, 0.20, 0.14, 0.83 );
    m_DefaultSolderPaste =   COLOR4D( 0.50, 0.50, 0.50,  1.0 );
    m_DefaultSurfaceFinish = COLOR4D( 0.75, 0.61, 0.23,  1.0 );
    m_DefaultBoardBody =     COLOR4D( 0.43, 0.45, 0.30, 0.90 );
#undef ADD_COLOR

    g_ColorsLoaded = true;
}


bool EXPORTER_PCB_VRML::SetScale( double aWorldScale )
{
    // set the scaling of the VRML world
    if( aWorldScale < 0.001 || aWorldScale > 10.0 )
        throw( std::runtime_error( "WorldScale out of range (valid range is 0.001 to 10.0)" ) );

    m_OutputPCB.SetScale( aWorldScale * 2.54 );
    m_WorldScale = aWorldScale * 2.54;

    return true;
}


void EXPORTER_PCB_VRML::SetOffset( double aXoff, double aYoff )
{
    m_tx = aXoff;
    m_ty = -aYoff;

    m_holes.SetVertexOffsets( aXoff, aYoff );
    m_3D_board.SetVertexOffsets( aXoff, aYoff );
    m_top_copper.SetVertexOffsets( aXoff, aYoff );
    m_bot_copper.SetVertexOffsets( aXoff, aYoff );
    m_top_silk.SetVertexOffsets( aXoff, aYoff );
    m_bot_silk.SetVertexOffsets( aXoff, aYoff );
    m_top_paste.SetVertexOffsets( aXoff, aYoff );
    m_bot_paste.SetVertexOffsets( aXoff, aYoff );
    m_top_soldermask.SetVertexOffsets( aXoff, aYoff );
    m_bot_soldermask.SetVertexOffsets( aXoff, aYoff );
    m_plated_holes.SetVertexOffsets( aXoff, aYoff );
}


bool EXPORTER_PCB_VRML::GetLayer3D( int layer, VRML_LAYER** vlayer )
{
    // select the VRML layer object to draw on; return true if
    // a layer has been selected.
    switch( layer )
    {
    case B_Cu:    *vlayer = &m_bot_copper;      return true;
    case F_Cu:    *vlayer = &m_top_copper;      return true;
    case B_SilkS: *vlayer = &m_bot_silk;        return true;
    case F_SilkS: *vlayer = &m_top_silk;        return true;
    case B_Mask:  *vlayer = &m_bot_soldermask;  return true;
    case F_Mask:  *vlayer = &m_top_soldermask;  return true;
    case B_Paste: *vlayer = &m_bot_paste;       return true;
    case F_Paste: *vlayer = &m_top_paste;       return true;
    default:                                    return false;
    }
}

void EXPORTER_PCB_VRML::ExportVrmlSolderMask()
{
    SHAPE_POLY_SET holes, outlines = m_pcbOutlines;

    // holes is the solder mask opening.
    // the actual shape is the negative shape of mask opening.
    PCB_LAYER_ID pcb_layer = F_Mask;
    VRML_LAYER* vrmllayer = &m_top_soldermask;

    for( int lcnt = 0; lcnt < 2; lcnt++ )
    {
        holes.RemoveAllContours();
        outlines.RemoveAllContours();
        outlines = m_pcbOutlines;
        m_board->ConvertBrdLayerToPolygonalContours( pcb_layer, holes );

        outlines.BooleanSubtract( holes );
        outlines.Fracture();
        ExportVrmlPolygonSet( vrmllayer, outlines );

        pcb_layer = B_Mask;
        vrmllayer = &m_bot_soldermask;
    }
}


void EXPORTER_PCB_VRML::ExportStandardLayers()
{
    SHAPE_POLY_SET outlines;

    PCB_LAYER_ID pcb_layer[] =
    {
        F_Cu, B_Cu, F_SilkS, B_SilkS, F_Paste, B_Paste
    };

    VRML_LAYER* vrmllayer[] =
    {
        &m_top_copper, &m_bot_copper, &m_top_silk, &m_bot_silk, &m_top_paste, &m_bot_paste,
        nullptr     // Sentinel
    };

    for( int lcnt = 0; ; lcnt++ )
    {
        if( vrmllayer[lcnt] == nullptr )
            break;

        outlines.RemoveAllContours();
        m_board->ConvertBrdLayerToPolygonalContours( pcb_layer[lcnt], outlines );
        outlines.BooleanIntersection( m_pcbOutlines );
        outlines.Fracture();

        ExportVrmlPolygonSet( vrmllayer[lcnt], outlines );
    }
}


void EXPORTER_PCB_VRML::write_triangle_bag( std::ostream& aOut_file, const VRML_COLOR& aColor,
                                            VRML_LAYER* aLayer, bool aPlane, bool aTop,
                                            double aTop_z, double aBottom_z )
{
    // A lot of nodes are not required, but blender sometimes chokes without them.
    static const char* shape_boiler[] =
    {
        "Transform {\n",
        "  children [\n",
        "    Group {\n",
        "      children [\n",
        "        Shape {\n",
        "          appearance Appearance {\n",
        "            material Material {\n",
        0,                                      // Material marker
        "            }\n",
        "          }\n",
        "          geometry IndexedFaceSet {\n",
        "            solid TRUE\n",
        "            coord Coordinate {\n",
        "              point [\n",
        0,                                      // Coordinates marker
        "              ]\n",
        "            }\n",
        "            coordIndex [\n",
        0,                                      // Index marker
        "            ]\n",
        "          }\n",
        "        }\n",
        "      ]\n",
        "    }\n",
        "  ]\n",
        "}\n",
        0    // End marker
    };

    int marker_found = 0, lineno = 0;

    while( marker_found < 4 )
    {
        if( shape_boiler[lineno] )
        {
            aOut_file << shape_boiler[lineno];
        }
        else
        {
            marker_found++;

            switch( marker_found )
            {
            case 1:    // Material marker
            {
                std::streamsize lastPrecision = aOut_file.precision();
                aOut_file << "              diffuseColor " << std::setprecision(3);
                aOut_file << aColor.diffuse_red << " ";
                aOut_file << aColor.diffuse_grn << " ";
                aOut_file << aColor.diffuse_blu << "\n";

                aOut_file << "              specularColor ";
                aOut_file << aColor.spec_red << " ";
                aOut_file << aColor.spec_grn << " ";
                aOut_file << aColor.spec_blu << "\n";

                aOut_file << "              emissiveColor ";
                aOut_file << aColor.emit_red << " ";
                aOut_file << aColor.emit_grn << " ";
                aOut_file << aColor.emit_blu << "\n";

                aOut_file << "              ambientIntensity " << aColor.ambient << "\n";
                aOut_file << "              transparency " << aColor.transp << "\n";
                aOut_file << "              shininess " << aColor.shiny << "\n";
                aOut_file.precision( lastPrecision );
            }
                break;

            case 2:

                if( aPlane )
                    aLayer->WriteVertices( aTop_z, aOut_file, m_precision );
                else
                    aLayer->Write3DVertices( aTop_z, aBottom_z, aOut_file, m_precision );

                aOut_file << "\n";
                break;

            case 3:

                if( aPlane )
                    aLayer->WriteIndices( aTop, aOut_file );
                else
                    aLayer->Write3DIndices( aOut_file );

                aOut_file << "\n";
                break;

            default:
                break;
            }
        }

        lineno++;
    }
}


void EXPORTER_PCB_VRML::writeLayers( const char* aFileName, OSTREAM* aOutputFile )
{
    // VRML_LAYER board;
    m_3D_board.Tesselate( &m_holes );
    double brdz = m_brd_thickness / 2.0
                  - ( pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) ) * m_BoardToVrmlScale;

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_PCB ),
                            &m_3D_board, false, false, brdz, -brdz );
    }
    else
    {
        create_vrml_shell( m_OutputPCB, VRML_COLOR_PCB, &m_3D_board, brdz, -brdz );
    }

    // VRML_LAYER m_top_copper;
    m_top_copper.Tesselate( &m_holes );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_COPPER ),
                            &m_top_copper, true, true, GetLayerZ( F_Cu ), 0 );
    }
    else
    {
        create_vrml_plane( m_OutputPCB, VRML_COLOR_COPPER, &m_top_copper,
                           GetLayerZ( F_Cu ), true );
    }

    // VRML_LAYER m_top_paste;
    m_top_paste.Tesselate( &m_holes );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_PASTE ),
                            &m_top_paste, true, true,
                            GetLayerZ( F_Cu ) + pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                            m_BoardToVrmlScale,
                            0 );
    }
    else
    {
        create_vrml_plane( m_OutputPCB, VRML_COLOR_PASTE, &m_top_paste,
                           GetLayerZ( F_Cu ) + pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                           m_BoardToVrmlScale,
                           true );
    }

    // VRML_LAYER m_top_soldermask;
    m_top_soldermask.Tesselate( &m_holes );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_TOP_SOLDMASK ),
                            &m_top_soldermask, true, true,
                            GetLayerZ( F_Cu ) + pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                            m_BoardToVrmlScale,
                            0 );
    }
    else
    {
        create_vrml_plane( m_OutputPCB, VRML_COLOR_TOP_SOLDMASK, &m_top_soldermask,
                           GetLayerZ( F_Cu ) + pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                           m_BoardToVrmlScale,
                           true );
    }

    // VRML_LAYER m_bot_copper;
    m_bot_copper.Tesselate( &m_holes );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_COPPER ),
                            &m_bot_copper, true, false, GetLayerZ( B_Cu ), 0 );
    }
    else
    {
        create_vrml_plane( m_OutputPCB, VRML_COLOR_COPPER, &m_bot_copper,
                           GetLayerZ( B_Cu ), false );
    }

    // VRML_LAYER m_bot_paste;
    m_bot_paste.Tesselate( &m_holes );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_PASTE ),
                            &m_bot_paste, true, false,
                            GetLayerZ( B_Cu )
                            - pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) * m_BoardToVrmlScale,
                            0 );
    }
    else
    {
        create_vrml_plane( m_OutputPCB, VRML_COLOR_PASTE, &m_bot_paste,
                           GetLayerZ( B_Cu ) - pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                           m_BoardToVrmlScale,
                           false );
    }

    // VRML_LAYER m_bot_mask:
    m_bot_soldermask.Tesselate( &m_holes );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_BOT_SOLDMASK ),
                            &m_bot_soldermask, true, false,
                            GetLayerZ( B_Cu ) - pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                            m_BoardToVrmlScale,
                            0 );
    }
    else
    {
        create_vrml_plane( m_OutputPCB, VRML_COLOR_BOT_SOLDMASK, &m_bot_soldermask,
                           GetLayerZ( B_Cu ) - pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                           m_BoardToVrmlScale,
                           false );
    }

    // VRML_LAYER PTH;
    m_plated_holes.Tesselate( nullptr, true );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_PASTE ),
                            &m_plated_holes, false, false,
                            GetLayerZ( F_Cu ) + pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                            m_BoardToVrmlScale,
                            GetLayerZ( B_Cu ) - pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                            m_BoardToVrmlScale );
    }
    else
    {
        create_vrml_shell( m_OutputPCB, VRML_COLOR_PASTE, &m_plated_holes,
                           GetLayerZ( F_Cu ) + pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                           m_BoardToVrmlScale,
                           GetLayerZ( B_Cu ) - pcbIUScale.mmToIU( ART_OFFSET / 2.0 ) *
                           m_BoardToVrmlScale );
    }

    // VRML_LAYER m_top_silk;
    m_top_silk.Tesselate( &m_holes );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_TOP_SILK ), &m_top_silk,
                            true, true, GetLayerZ( F_SilkS ), 0 );
    }
    else
    {
        create_vrml_plane( m_OutputPCB, VRML_COLOR_TOP_SILK, &m_top_silk,
                           GetLayerZ( F_SilkS ), true );
    }

    // VRML_LAYER m_bot_silk;
    m_bot_silk.Tesselate( &m_holes );

    if( m_UseInlineModelsInBrdfile )
    {
        write_triangle_bag( *aOutputFile, GetColor( VRML_COLOR_BOT_SILK ), &m_bot_silk,
                            true, false, GetLayerZ( B_SilkS ), 0 );
    }
    else
    {
        create_vrml_plane( m_OutputPCB, VRML_COLOR_BOT_SILK, &m_bot_silk,
                           GetLayerZ( B_SilkS ), false );
    }

    if( !m_UseInlineModelsInBrdfile )
        S3D::WriteVRML( aFileName, true, m_OutputPCB.GetRawPtr(), true, true );
}


void EXPORTER_PCB_VRML::ComputeLayer3D_Zpos()
{
    int copper_layers = m_board->GetCopperLayerCount();

    // We call it 'layer' thickness, but it's the whole board thickness!
    m_brd_thickness = m_board->GetDesignSettings().GetBoardThickness() * m_BoardToVrmlScale;
    double half_thickness = m_brd_thickness / 2;

    // Compute each layer's Z value, more or less like the 3d view
    int orderFromTop = 0;

    for( PCB_LAYER_ID layer : LSET::AllCuMask( copper_layers ).CuStack() )
    {
        SetLayerZ( layer, half_thickness - m_brd_thickness * orderFromTop / ( copper_layers - 1 ) );
        orderFromTop++;
    }

    // To avoid rounding interference, we apply an epsilon to each successive layer
    double epsilon_z = pcbIUScale.mmToIU( ART_OFFSET ) * m_BoardToVrmlScale;
    SetLayerZ( B_Paste, -half_thickness - epsilon_z );
    SetLayerZ( B_Adhes, -half_thickness - epsilon_z );
    SetLayerZ( B_SilkS, -half_thickness - epsilon_z * 3 );
    SetLayerZ( B_Mask, -half_thickness - epsilon_z * 2 );
    SetLayerZ( F_Mask, half_thickness + epsilon_z * 2 );
    SetLayerZ( F_SilkS, half_thickness + epsilon_z * 3 );
    SetLayerZ( F_Adhes, half_thickness + epsilon_z );
    SetLayerZ( F_Paste, half_thickness + epsilon_z );
    SetLayerZ( Dwgs_User, half_thickness + epsilon_z * 5 );
    SetLayerZ( Cmts_User, half_thickness + epsilon_z * 6 );
    SetLayerZ( Eco1_User, half_thickness + epsilon_z * 7 );
    SetLayerZ( Eco2_User, half_thickness + epsilon_z * 8 );
    SetLayerZ( Edge_Cuts, 0 );
}


void EXPORTER_PCB_VRML::ExportVrmlPolygonSet( VRML_LAYER* aVlayer, const SHAPE_POLY_SET& aOutlines )
{
    // Polygons in SHAPE_POLY_SET must be without hole, i.e. holes must be linked
    // previously to their main outline.
    for( int icnt = 0; icnt < aOutlines.OutlineCount(); icnt++ )
    {
        const SHAPE_LINE_CHAIN& outline = aOutlines.COutline( icnt );

        int seg = aVlayer->NewContour();

        for( int jj = 0; jj < outline.PointCount(); jj++ )
        {
            if( !aVlayer->AddVertex( seg, outline.CPoint( jj ).x * m_BoardToVrmlScale,
                                     -outline.CPoint( jj ).y * m_BoardToVrmlScale ) )
                throw( std::runtime_error( aVlayer->GetError() ) );
        }

        aVlayer->EnsureWinding( seg, false );
    }
}


void EXPORTER_PCB_VRML::ExportVrmlBoard()
{
    if( !m_board->GetBoardPolygonOutlines( m_pcbOutlines, true ) )
    {
        wxLogWarning( _( "Board outline is malformed. Run DRC for a full analysis." ) );
    }

    int seg;

    for( int cnt = 0; cnt < m_pcbOutlines.OutlineCount(); cnt++ )
    {
        const SHAPE_LINE_CHAIN& outline = m_pcbOutlines.COutline( cnt );

        seg = m_3D_board.NewContour();

        for( int j = 0; j < outline.PointCount(); j++ )
        {
            m_3D_board.AddVertex( seg, (double)outline.CPoint(j).x * m_BoardToVrmlScale,
                                  -((double)outline.CPoint(j).y * m_BoardToVrmlScale ) );

        }

        m_3D_board.EnsureWinding( seg, false );

        // Generate board holes from outlines:
        for( int ii = 0; ii < m_pcbOutlines.HoleCount( cnt ); ii++ )
        {
            const SHAPE_LINE_CHAIN& hole = m_pcbOutlines.Hole( cnt, ii );

            seg = m_holes.NewContour();

            if( seg < 0 )
            {
                wxLogError( _( "VRML Export Failed: Could not add holes to contours." ) );
                return;
            }

            for( int j = 0; j < hole.PointCount(); j++ )
            {
                m_holes.AddVertex( seg, (double) hole.CPoint(j).x * m_BoardToVrmlScale,
                                   -( (double) hole.CPoint(j).y * m_BoardToVrmlScale ) );
            }

            m_holes.EnsureWinding( seg, true );
        }
    }
}



void EXPORTER_PCB_VRML::ExportVrmlViaHoles()
{
    PCB_LAYER_ID top_layer, bottom_layer;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

        via->LayerPair( &top_layer, &bottom_layer );

        // do not render a buried via
        if( top_layer != F_Cu && bottom_layer != B_Cu )
            continue;

        // Export all via holes to m_holes
        double hole_radius = via->GetDrillValue() * m_BoardToVrmlScale / 2.0;

        if( hole_radius <= 0 )
            continue;

        double x    = via->GetStart().x * m_BoardToVrmlScale;
        double y    = via->GetStart().y * m_BoardToVrmlScale;

        // Set the optimal number of segments to approximate a circle.
        // SetArcParams needs a count max, and the minimal and maximal length
        // of segments
        double max_error = ERR_APPROX_MAX_MM;

        if( m_UseInlineModelsInBrdfile )
            max_error /= 2.54;      // The board is exported with a size reduced by 2.54

        int nsides = GetArcToSegmentCount( via->GetDrillValue(), pcbIUScale.mmToIU( max_error ),
                                           FULL_CIRCLE );

        double minSegLength = M_PI * 2.0 * hole_radius / nsides;
        double maxSegLength = minSegLength*2.0;

        m_holes.SetArcParams( nsides*2, minSegLength, maxSegLength );
        m_plated_holes.SetArcParams( nsides*2, minSegLength, maxSegLength );

        m_holes.AddCircle( x, -y, hole_radius, true, true );
        m_plated_holes.AddCircle( x, -y, hole_radius, true, false );

        m_holes.ResetArcParams();
        m_plated_holes.ResetArcParams();
    }
}


void EXPORTER_PCB_VRML::ExportVrmlPadHole( PAD* aPad )
{
    double  hole_drill_w    = (double) aPad->GetDrillSize().x * m_BoardToVrmlScale / 2.0;
    double  hole_drill_h    = (double) aPad->GetDrillSize().y * m_BoardToVrmlScale / 2.0;
    double  hole_drill      = std::min( hole_drill_w, hole_drill_h );
    double  hole_x          = aPad->GetPosition().x * m_BoardToVrmlScale;
    double  hole_y          = aPad->GetPosition().y * m_BoardToVrmlScale;

    // Export the hole on the edge layer
    if( hole_drill > 0 )
    {
        double max_error = ERR_APPROX_MAX_MM;

        if( m_UseInlineModelsInBrdfile )
            max_error /= 2.54;      // The board is exported with a size reduced by 2.54

        int nsides = GetArcToSegmentCount( hole_drill, pcbIUScale.mmToIU( max_error ),
                                           FULL_CIRCLE );
        double minSegLength = M_PI * hole_drill / nsides;
        double maxSegLength = minSegLength*2.0;

        m_holes.SetArcParams( nsides*2, minSegLength, maxSegLength );
        m_plated_holes.SetArcParams( nsides*2, minSegLength, maxSegLength );

        bool pth = false;

        if( ( aPad->GetAttribute() != PAD_ATTRIB::NPTH ) )
            pth = true;

        if( aPad->GetDrillShape() == PAD_DRILL_SHAPE::OBLONG )
        {
            // Oblong hole (slot)

            if( pth )
            {
                m_holes.AddSlot( hole_x, -hole_y, hole_drill_w * 2.0 + PLATE_OFFSET,
                                 hole_drill_h * 2.0 + PLATE_OFFSET,
                                 aPad->GetOrientation().AsDegrees(), true, true );

                m_plated_holes.AddSlot( hole_x, -hole_y,
                                        hole_drill_w * 2.0, hole_drill_h * 2.0,
                                        aPad->GetOrientation().AsDegrees(), true, false );
            }
            else
            {
                m_holes.AddSlot( hole_x, -hole_y, hole_drill_w * 2.0, hole_drill_h * 2.0,
                                 aPad->GetOrientation().AsDegrees(), true, false );

            }
        }
        else
        {
            // Drill a round hole
            if( pth )
            {
                m_holes.AddCircle( hole_x, -hole_y, hole_drill + PLATE_OFFSET, true, true );
                m_plated_holes.AddCircle( hole_x, -hole_y, hole_drill, true, false );
            }
            else
            {
                m_holes.AddCircle( hole_x, -hole_y, hole_drill, true, false );
            }

        }

        m_holes.ResetArcParams();
        m_plated_holes.ResetArcParams();
    }
}


// From axis/rot to quaternion
static void build_quat( double x, double y, double z, double a, double q[4] )
{
    double sina = sin( a / 2 );

    q[0] = x * sina;
    q[1] = y * sina;
    q[2] = z * sina;
    q[3] = cos( a / 2 );
}


// From quaternion to axis/rot
static void from_quat( double q[4], double rot[4] )
{
    rot[3] = acos( q[3] ) * 2;

    for( int i = 0; i < 3; i++ )
        rot[i] = q[i] / sin( rot[3] / 2 );
}


// Quaternion composition
static void compose_quat( double q1[4], double q2[4], double qr[4] )
{
    double tmp[4];

    tmp[0] = q2[3] * q1[0] + q2[0] * q1[3] + q2[1] * q1[2] - q2[2] * q1[1];
    tmp[1] = q2[3] * q1[1] + q2[1] * q1[3] + q2[2] * q1[0] - q2[0] * q1[2];
    tmp[2] = q2[3] * q1[2] + q2[2] * q1[3] + q2[0] * q1[1] - q2[1] * q1[0];
    tmp[3] = q2[3] * q1[3] - q2[0] * q1[0] - q2[1] * q1[1] - q2[2] * q1[2];

    qr[0] = tmp[0];
    qr[1] = tmp[1];
    qr[2] = tmp[2];
    qr[3] = tmp[3];
}


void EXPORTER_PCB_VRML::ExportVrmlFootprint( FOOTPRINT* aFootprint, std::ostream* aOutputFile )
{
    // Note: if m_UseInlineModelsInBrdfile is false, the 3D footprint shape is copied to
    // the vrml board file, and aOutputFile is not used (can be nullptr)
    // if m_UseInlineModelsInBrdfile is true, the 3D footprint shape is copied to
    // aOutputFile (with the suitable rotation/translation/scale transform, and the vrml board
    // file contains only the filename of 3D shapes to add to the full vrml scene
    wxCHECK( aFootprint, /* void */ );

    wxString libraryName = aFootprint->GetFPID().GetLibNickname();
    wxString footprintBasePath = wxEmptyString;

    if( m_board->GetProject() )
    {
        std::optional<LIBRARY_TABLE_ROW*> fpRow =
                            PROJECT_PCB::FootprintLibAdapter( m_board->GetProject() )->GetRow( libraryName );
        if( fpRow )
            footprintBasePath = LIBRARY_MANAGER::GetFullURI( *fpRow, true );
    }


    // Export pad holes
    for( PAD* pad : aFootprint->Pads() )
        ExportVrmlPadHole( pad );

    if( !m_includeUnspecified
        && ( !( aFootprint->GetAttributes() & ( FP_THROUGH_HOLE | FP_SMD ) ) ) )
    {
        return;
    }

    if( !m_includeDNP
            && aFootprint->GetDNPForVariant( m_board ? m_board->GetCurrentVariant() : wxString() ) )
        return;

    std::vector<const EMBEDDED_FILES*> embeddedFilesStack;
    bool isFlipped = aFootprint->GetLayer() == B_Cu;

    // Export the object VRML model(s)
    auto sM = aFootprint->Models().begin();
    auto eM = aFootprint->Models().end();


    while( sM != eM )
    {
        if( !sM->m_Show )
        {
            ++sM;
            continue;
        }

        embeddedFilesStack.clear();
        embeddedFilesStack.push_back( aFootprint->GetEmbeddedFiles() );
        embeddedFilesStack.push_back( m_board->GetEmbeddedFiles() );

        SGNODE* mod3d = (SGNODE*) m_Cache3Dmodels->Load( sM->m_Filename, footprintBasePath,
                                                         std::move( embeddedFilesStack ) );

        /* Calculate 3D shape rotation:
         * this is the rotation parameters, with an additional 180 deg rotation
         * for footprints that are flipped
         * When flipped, axis rotation is the horizontal axis (X axis)
         */
        double rotx = -sM->m_Rotation.x;
        double roty = -sM->m_Rotation.y;
        double rotz = -sM->m_Rotation.z;

        if( isFlipped )
        {
            rotx += 180.0;
            roty = -roty;
            rotz = -rotz;
        }

        // Do some quaternion munching
        double q1[4], q2[4], rot[4];
        build_quat( 1, 0, 0, DEG2RAD( rotx ), q1 );
        build_quat( 0, 1, 0, DEG2RAD( roty ), q2 );
        compose_quat( q1, q2, q1 );
        build_quat( 0, 0, 1, DEG2RAD( rotz ), q2 );
        compose_quat( q1, q2, q1 );

        // Note here aFootprint->GetOrientation() is in 0.1 degrees, so footprint rotation
        // has to be converted to radians
        build_quat( 0, 0, 1, aFootprint->GetOrientation().AsRadians(), q2 );
        compose_quat( q1, q2, q1 );
        from_quat( q1, rot );

        double offsetFactor = 1000.0f * pcbIUScale.IU_PER_MILS / 25.4f;

        // adjust 3D shape local offset position
        // they are given in mm, so they are converted in board IU.
        double offsetx = sM->m_Offset.x * offsetFactor;
        double offsety = sM->m_Offset.y * offsetFactor;
        double offsetz = sM->m_Offset.z * offsetFactor;

        if( isFlipped )
            offsetz = -offsetz;
        else
            offsety = -offsety;  // In normal mode, Y axis is reversed in Pcbnew.

        RotatePoint( &offsetx, &offsety, aFootprint->GetOrientation() );

        SGPOINT trans;
        trans.x = ( offsetx + aFootprint->GetPosition().x ) * m_BoardToVrmlScale + m_tx;
        trans.y = -( offsety + aFootprint->GetPosition().y) * m_BoardToVrmlScale - m_ty;
        trans.z = (offsetz * m_BoardToVrmlScale ) + GetLayerZ( aFootprint->GetLayer() );

        if( m_UseInlineModelsInBrdfile )
        {
            wxCHECK( aOutputFile, /* void */ );

            int old_precision = aOutputFile->precision();
            aOutputFile->precision( m_precision );

            embeddedFilesStack.clear();
            embeddedFilesStack.push_back( aFootprint->GetEmbeddedFiles() );
            embeddedFilesStack.push_back( m_board->GetEmbeddedFiles() );

            wxFileName srcFile = m_Cache3Dmodels->GetResolver()->ResolvePath( sM->m_Filename, footprintBasePath,
                                                                              std::move( embeddedFilesStack ) );
            if( !srcFile.FileExists() ) {
		// skip model where the file cannot be resolved
                ++sM;
                continue;
            }

            wxFileName dstFile;
            dstFile.SetPath( m_Subdir3DFpModels );
            dstFile.SetName( srcFile.GetName() );
            dstFile.SetExt( wxT( "wrl" ) );

            // copy the file if necessary
            wxDateTime srcModTime = srcFile.GetModificationTime();
            wxDateTime destModTime = wxDateTime();

            if( dstFile.FileExists() )
                destModTime = dstFile.GetModificationTime();

            if( srcModTime != destModTime )
            {
                wxString fileExt = srcFile.GetExt();
                fileExt.LowerCase();

                // copy VRML models and use the scenegraph library to
                // translate other model types
                if( fileExt == wxT( "wrl" ) )
                {
                    if( !wxCopyFile( srcFile.GetFullPath(), dstFile.GetFullPath() ) )
                    {
                        ++sM;
                        continue;
                    }
                }
                else if( fileExt == wxT( "wrz" ) )
                {
                    wxFileInputStream input_file_stream( srcFile.GetFullPath() );
                    if( !input_file_stream.IsOk() || input_file_stream.GetSize() == wxInvalidSize )
                    {
                        ++sM;
                        continue;
                    }

                    wxZlibInputStream   zlib_input_stream( input_file_stream, wxZLIB_GZIP );
                    wxFFileOutputStream output_file_stream( dstFile.GetFullPath() );
                    if( !zlib_input_stream.IsOk() || !output_file_stream.IsOk() )
                    {
                        output_file_stream.Close();
                        ++sM;
                        continue;
                    }

                    output_file_stream.Write( zlib_input_stream );
                    output_file_stream.Close();
                }
                else
                {

                    if( ( nullptr == mod3d) ||
                        ( !S3D::WriteVRML( dstFile.GetFullPath().ToUTF8(), true, mod3d, m_ReuseDef,
                                         true ) ) )
                    {
                        ++sM;
                        continue;
                    }
                }
            }

            (*aOutputFile) << "Transform {\n";

            // only write a rotation if it is >= 0.1 deg
            if( std::abs( rot[3] ) > 0.0001745 )
            {
                (*aOutputFile) << "  rotation ";
                (*aOutputFile) << rot[0] << " " << rot[1] << " " << rot[2] << " " << rot[3] << "\n";
            }

            (*aOutputFile) << "  translation ";
            (*aOutputFile) << trans.x << " ";
            (*aOutputFile) << trans.y << " ";
            (*aOutputFile) << trans.z << "\n";

            (*aOutputFile) << "  scale ";
            (*aOutputFile) << sM->m_Scale.x << " ";
            (*aOutputFile) << sM->m_Scale.y << " ";
            (*aOutputFile) << sM->m_Scale.z << "\n";

            (*aOutputFile) << "  children [\n    Inline {\n      url \"";

            if( m_UseRelPathIn3DModelFilename )
            {
                wxFileName tmp = dstFile;
                tmp.SetExt( wxT( "" ) );
                tmp.SetName( wxT( "" ) );
                tmp.RemoveLastDir();
                dstFile.MakeRelativeTo( tmp.GetPath() );
            }

            wxString fn = dstFile.GetFullPath();
            fn.Replace( wxT( "\\" ), wxT( "/" ) );
            (*aOutputFile) << TO_UTF8( fn ) << "\"\n    } ]\n";
            (*aOutputFile) << "  }\n";

            aOutputFile->precision( old_precision );
        }
        else
        {
	    if( nullptr == mod3d )
	    {
		++sM;
		continue;
	    }

            IFSG_TRANSFORM* modelShape = new IFSG_TRANSFORM( m_OutputPCB.GetRawPtr() );

            // only write a rotation if it is >= 0.1 deg
            if( std::abs( rot[3] ) > 0.0001745 )
                modelShape->SetRotation( SGVECTOR( rot[0], rot[1], rot[2] ), rot[3] );

            modelShape->SetTranslation( trans );
            modelShape->SetScale( SGPOINT( sM->m_Scale.x, sM->m_Scale.y, sM->m_Scale.z ) );

            if( nullptr == S3D::GetSGNodeParent( mod3d ) )
            {
                m_components.push_back( mod3d );
                modelShape->AddChildNode( mod3d );
            }
            else
            {
                modelShape->AddRefNode( mod3d );
            }

        }

        ++sM;
    }
}



bool EXPORTER_PCB_VRML::ExportVRML_File( PROJECT* aProject, wxString *aMessages,
                                         const wxString& aFullFileName, double aMMtoWRMLunit,
                                         bool aIncludeUnspecified, bool aIncludeDNP,
                                         bool aExport3DFiles, bool aUseRelativePaths,
                                         const wxString& a3D_Subdir,
                                         double aXRef, double aYRef )
{
    if( aProject == nullptr )
    {
        if( aMessages )
            *aMessages = _( "No project when exporting the VRML file");

        return false;
    }

    SetScale( aMMtoWRMLunit );
    m_UseInlineModelsInBrdfile = aExport3DFiles;

    wxFileName subdir( a3D_Subdir, wxT( "" ) );
    // convert the subdir path to a absolute full one with the output file as the cwd
    m_Subdir3DFpModels = subdir.GetAbsolutePath( wxFileName( aFullFileName ).GetPath() );

    m_UseRelPathIn3DModelFilename = aUseRelativePaths;
    m_includeUnspecified = aIncludeUnspecified;
    m_includeDNP = aIncludeDNP;
    m_Cache3Dmodels = PROJECT_PCB::Get3DCacheManager( aProject );

    // When 3D models are separate files, for historical reasons the VRML unit
    // is expected to be 0.1 inch (2.54mm) instead of 1mm, so we adjust the m_BoardToVrmlScale
    // to match the VRML scale of these external files.
    // Otherwise we use 1mm as VRML unit
    if( m_UseInlineModelsInBrdfile )
    {
        m_BoardToVrmlScale = pcbIUScale.MM_PER_IU / 2.54;
        SetOffset( -aXRef / 2.54, aYRef / 2.54 );
    }
    else
    {
        m_BoardToVrmlScale = pcbIUScale.MM_PER_IU;
        SetOffset( -aXRef, aYRef );
    }

    bool              success  = true;

    try
    {
        // Preliminary computation: the z value for each layer
        ComputeLayer3D_Zpos();

        // board edges and cutouts
        ExportVrmlBoard();

        // Draw solder mask layer (negative layer)
        ExportVrmlSolderMask();
        ExportVrmlViaHoles();
        ExportStandardLayers();

        if( m_UseInlineModelsInBrdfile )
        {
            // Copy fp 3D models in a folder, and link these files in
            // the board .vrml file
            ExportFp3DModelsAsLinkedFile( aFullFileName );
        }
        else
        {
            // merge footprints in the .vrml board file
            for( FOOTPRINT* footprint : m_board->Footprints() )
                ExportVrmlFootprint( footprint, nullptr );

            // write out the board and all layers
            writeLayers( TO_UTF8( aFullFileName ), nullptr );
        }
    }
    catch( const std::exception& e )
    {
        if( aMessages )
            *aMessages << _( "VRML Export Failed:\n" ) << From_UTF8( e.what() );

        success = false;
    }

    return success;
}

bool PCB_EDIT_FRAME::ExportVRML_File( const wxString& aFullFileName, double aMMtoWRMLunit,
                                      bool aIncludeUnspecified, bool aIncludeDNP,
                                      bool aExport3DFiles, bool aUseRelativePaths,
                                      const wxString& a3D_Subdir,
                                      double aXRef, double aYRef )
{
    bool     success;
    wxString msgs;
    EXPORTER_VRML model3d( GetBoard() );

    success = model3d.ExportVRML_File( &Prj(), &msgs, aFullFileName, aMMtoWRMLunit,
                                       aIncludeUnspecified, aIncludeDNP,
                                       aExport3DFiles, aUseRelativePaths,
                                       a3D_Subdir, aXRef, aYRef );

    if( !msgs.IsEmpty() )
        wxMessageBox( msgs );

    return success;
}


void EXPORTER_PCB_VRML::ExportFp3DModelsAsLinkedFile( const wxString& aFullFileName )
{
    // check if the 3D Subdir exists - create if not
    if( !wxDir::Exists( m_Subdir3DFpModels ) )
    {
        if( !wxDir::Make( m_Subdir3DFpModels ) )
            throw( std::runtime_error( "Could not create 3D model subdirectory" ) );
    }

    OPEN_OSTREAM( output_file, TO_UTF8( aFullFileName ) );

    if( output_file.fail() )
    {
        std::ostringstream ostr;
        ostr << "Could not open file '" << TO_UTF8( aFullFileName ) << "'";
        throw( std::runtime_error( ostr.str().c_str() ) );
    }

    output_file.imbue( std::locale::classic() );

    // Begin with the usual VRML boilerplate
    wxString fn = aFullFileName;
    fn.Replace( wxT( "\\" ) , wxT( "/" ) );
    output_file << "#VRML V2.0 utf8\n";
    output_file << "WorldInfo {\n";
    output_file << "  title \"" << TO_UTF8( fn ) << " - Generated by Pcbnew\"\n";
    output_file << "}\n";
    output_file << "Transform {\n";
    output_file << "  scale " << std::setprecision( m_precision );
    output_file << m_WorldScale << " ";
    output_file << m_WorldScale << " ";
    output_file << m_WorldScale << "\n";
    output_file << "  children [\n";

    // Export footprints
    for( FOOTPRINT* footprint : m_board->Footprints() )
        ExportVrmlFootprint( footprint, &output_file );

    // write out the board and all layers
    writeLayers( TO_UTF8( aFullFileName ), &output_file );

    // Close the outer 'transform' node
    output_file << "]\n}\n";

    CLOSE_STREAM( output_file );
}

SGNODE* EXPORTER_PCB_VRML::getSGColor( VRML_COLOR_INDEX colorIdx )
{
    if( colorIdx == -1 )
        colorIdx = VRML_COLOR_PCB;
    else if( colorIdx == VRML_COLOR_LAST )
        return nullptr;

    if( m_sgmaterial[colorIdx] )
        return m_sgmaterial[colorIdx];

    IFSG_APPEARANCE vcolor( (SGNODE*) nullptr );
    VRML_COLOR* cp = &vrml_colors_list[colorIdx];

    vcolor.SetSpecular( cp->spec_red, cp->spec_grn, cp->spec_blu );
    vcolor.SetDiffuse( cp->diffuse_red, cp->diffuse_grn, cp->diffuse_blu );
    vcolor.SetShininess( cp->shiny );
    // NOTE: XXX - replace with a better equation; using this definition
    // of ambient will not yield the best results
    vcolor.SetAmbient( cp->ambient, cp->ambient, cp->ambient );
    vcolor.SetTransparency( cp->transp );

    m_sgmaterial[colorIdx] = vcolor.GetRawPtr();

    return m_sgmaterial[colorIdx];
}


void EXPORTER_PCB_VRML::create_vrml_plane( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID,
                                           VRML_LAYER* layer, double top_z, bool aTopPlane )
{
    std::vector< double > vertices;
    std::vector< int > idxPlane;

    if( !( *layer ).Get2DTriangles( vertices, idxPlane, top_z, aTopPlane ) )
    {
        return;
    }

    if( ( idxPlane.size() % 3 ) )
    {
        throw( std::runtime_error( "[BUG] index lists are not a multiple of 3 (not a triangle "
                                   "list)" ) );
    }

    std::vector< SGPOINT > vlist;
    size_t nvert = vertices.size() / 3;
    size_t j = 0;

    for( size_t i = 0; i < nvert; ++i, j+= 3 )
        vlist.emplace_back( vertices[j], vertices[j+1], vertices[j+2] );

    // create the intermediate scenegraph
    IFSG_TRANSFORM tx0( PcbOutput.GetRawPtr() );    // tx0 = Transform for this outline
    IFSG_SHAPE shape( tx0 );    // shape will hold (a) all vertices and (b) a local list of normals
    IFSG_FACESET face( shape );         // this face shall represent the top and bottom planes
    IFSG_COORDS cp( face );             // coordinates for all faces
    cp.SetCoordsList( nvert, &vlist[0] );
    IFSG_COORDINDEX coordIdx( face );   // coordinate indices for top and bottom planes only
    coordIdx.SetIndices( idxPlane.size(), &idxPlane[0] );
    IFSG_NORMALS norms( face );         // normals for the top and bottom planes

    // set the normals
    if( aTopPlane )
    {
        for( size_t i = 0; i < nvert; ++i )
            norms.AddNormal( 0.0, 0.0, 1.0 );
    }
    else
    {
        for( size_t i = 0; i < nvert; ++i )
            norms.AddNormal( 0.0, 0.0, -1.0 );
    }

    // assign a color from the palette
    SGNODE* modelColor = getSGColor( colorID );

    if( nullptr != modelColor )
    {
        if( nullptr == S3D::GetSGNodeParent( modelColor ) )
            shape.AddChildNode( modelColor );
        else
            shape.AddRefNode( modelColor );
    }
}


void EXPORTER_PCB_VRML::create_vrml_shell( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID,
                                           VRML_LAYER* layer, double top_z, double bottom_z )
{
    std::vector< double > vertices;
    std::vector< int > idxPlane;
    std::vector< int > idxSide;

    if( top_z < bottom_z )
    {
        double tmp = top_z;
        top_z = bottom_z;
        bottom_z = tmp;
    }

    if( !( *layer ).Get3DTriangles( vertices, idxPlane, idxSide, top_z, bottom_z )
            || idxPlane.empty() || idxSide.empty() )
    {
        return;
    }

    if( ( idxPlane.size() % 3 ) || ( idxSide.size() % 3 ) )
    {
        throw( std::runtime_error( "[BUG] index lists are not a multiple of 3 (not a "
                                   "triangle list)" ) );
    }

    std::vector< SGPOINT > vlist;
    size_t nvert = vertices.size() / 3;
    size_t j = 0;

    for( size_t i = 0; i < nvert; ++i, j+= 3 )
        vlist.emplace_back( vertices[j], vertices[j+1], vertices[j+2] );

    // create the intermediate scenegraph
    IFSG_TRANSFORM tx0( PcbOutput.GetRawPtr() );    // tx0 = Transform for this outline
    IFSG_SHAPE shape( tx0 );    // shape will hold (a) all vertices and (b) a local list of normals
    IFSG_FACESET face( shape );         // this face shall represent the top and bottom planes
    IFSG_COORDS cp( face );             // coordinates for all faces
    cp.SetCoordsList( nvert, &vlist[0] );
    IFSG_COORDINDEX coordIdx( face );   // coordinate indices for top and bottom planes only
    coordIdx.SetIndices( idxPlane.size(), &idxPlane[0] );
    IFSG_NORMALS norms( face );         // normals for the top and bottom planes

    // number of TOP (and bottom) vertices
    j = nvert / 2;

    // set the TOP normals
    for( size_t i = 0; i < j; ++i )
        norms.AddNormal( 0.0, 0.0, 1.0 );

    // set the BOTTOM normals
    for( size_t i = 0; i < j; ++i )
        norms.AddNormal( 0.0, 0.0, -1.0 );

    // assign a color from the palette
    SGNODE* modelColor = getSGColor( colorID );

    if( nullptr != modelColor )
    {
        if( nullptr == S3D::GetSGNodeParent( modelColor ) )
            shape.AddChildNode( modelColor );
        else
            shape.AddRefNode( modelColor );
    }

    // create a second shape describing the vertical walls of the extrusion
    // using per-vertex-per-face-normals
    shape.NewNode( tx0 );
    shape.AddRefNode( modelColor );    // set the color to be the same as the top/bottom
    face.NewNode( shape );
    cp.NewNode( face );               // new vertex list
    norms.NewNode( face );            // new normals list
    coordIdx.NewNode( face );         // new index list

    // populate the new per-face vertex list and its indices and normals
    std::vector< int >::iterator sI = idxSide.begin();
    std::vector< int >::iterator eI = idxSide.end();

    size_t sidx = 0;    // index to the new coord set
    SGPOINT p1, p2, p3;
    SGVECTOR vnorm;

    while( sI != eI )
    {
        p1 = vlist[*sI];
        cp.AddCoord( p1 );
        ++sI;

        p2 = vlist[*sI];
        cp.AddCoord( p2 );
        ++sI;

        p3 = vlist[*sI];
        cp.AddCoord( p3 );
        ++sI;

        vnorm.SetVector( S3D::CalcTriNorm( p1, p2, p3 ) );
        norms.AddNormal( vnorm );
        norms.AddNormal( vnorm );
        norms.AddNormal( vnorm );

        coordIdx.AddIndex( (int)sidx );
        ++sidx;
        coordIdx.AddIndex( (int)sidx );
        ++sidx;
        coordIdx.AddIndex( (int)sidx );
        ++sidx;
    }
}
