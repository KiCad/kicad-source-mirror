/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <dialogs/dialog_color_picker.h>
#include <export_vrml.h>

// offset for art layers, mm (silk, paste, etc)
#define  ART_OFFSET 0.025
// offset for plating
#define  PLATE_OFFSET 0.005

class PROJECT;

enum VRML_COLOR_INDEX
{
    VRML_COLOR_NONE = -1,
    VRML_COLOR_PCB = 0,
    VRML_COLOR_COPPER,
    VRML_COLOR_TOP_SOLDMASK,
    VRML_COLOR_BOT_SOLDMASK,
    VRML_COLOR_PASTE,
    VRML_COLOR_TOP_SILK,
    VRML_COLOR_BOT_SILK,
    VRML_COLOR_LAST         // Sentinel
};


struct VRML_COLOR
{
    float diffuse_red;
    float diffuse_grn;
    float diffuse_blu;

    float spec_red;
    float spec_grn;
    float spec_blu;

    float emit_red;
    float emit_grn;
    float emit_blu;

    float ambient;
    float transp;
    float shiny;

    VRML_COLOR()
    {
        // default green
        diffuse_red = 0.13f;
        diffuse_grn = 0.81f;
        diffuse_blu = 0.22f;
        spec_red = 0.01f;
        spec_grn = 0.08f;
        spec_blu = 0.02f;
        emit_red = 0.0f;
        emit_grn = 0.0f;
        emit_blu = 0.0f;

        ambient = 0.8f;
        transp = 0.0f;
        shiny = 0.02f;
    }

    VRML_COLOR( float dr, float dg, float db,
                float sr, float sg, float sb,
                float am, float tr, float sh )
    {
        diffuse_red = dr;
        diffuse_grn = dg;
        diffuse_blu = db;
        spec_red = sr;
        spec_grn = sg;
        spec_blu = sb;
        emit_red = 0.0f;
        emit_grn = 0.0f;
        emit_blu = 0.0f;

        ambient = am;
        transp  = tr;
        shiny   = sh;
    }
};


// Handle the board and its board items to convert them to a VRML representation:
class EXPORTER_PCB_VRML
{
public:
    EXPORTER_PCB_VRML( BOARD* aBoard );
    ~EXPORTER_PCB_VRML();

    /**
     * Export a VRML file image of the board.
     * @param aProject is the current project (cannot be null)
     * @param aMessages will contain error message(s)
     * @param aFullFileName the full filename of the file to create
     * @param aMMtoWRMLunit the VRML scaling factor: 1.0 to export in mm. 0.001 for meters
     * @param aExport3DFiles true to copy 3D shapes in the subir a3D_Subdir
     * @param aUseRelativePaths set to true to use relative paths instead of absolute paths
     *                          in the board VRML file URLs.
     * @param a3D_Subdir sub directory where 3D shapes files are copied.  This is only used
     *                   when aExport3DFiles == true.
     * @param aXRef X value of PCB (0,0) reference point.
     * @param aYRef Y value of PCB (0,0) reference point.
     * @return true if Ok.
     */
    bool ExportVRML_File( PROJECT* aProject, wxString *aMessages,
                          const wxString& aFullFileName, double  aMMtoWRMLunit,
                          bool aIncludeUnspecified, bool aIncludeDNP,
                          bool aExport3DFiles, bool aUseRelativePaths,
                          const wxString& a3D_Subdir,
                          double aXRef, double aYRef );

private:
    VRML_COLOR& GetColor( VRML_COLOR_INDEX aIndex )
    {
        return vrml_colors_list[aIndex];
    }

    void SetOffset( double aXoff, double aYoff );

    double GetLayerZ( int aLayer )
    {
        auto it = m_layer_z.find( aLayer );

        if( it == m_layer_z.end() )
            return 0;

        return it->second;
    }

    void SetLayerZ( int aLayer, double aValue )
    {
        m_layer_z[aLayer] = aValue;
    }

    // set the scaling of the VRML world
    bool SetScale( double aWorldScale );

    // Initialize the list of colors used in VRML export.
    void initStaticColorList();

    // Build and export the solder mask layer, that is a negative layer
    void ExportVrmlSolderMask();

    // Build and export the 4 layers F_Cu, B_Cu, F_SilkS, B_SilkS
    void ExportStandardLayers();

    void ExportVrmlFootprint( FOOTPRINT* aFootprint, std::ostream* aOutputFile );

    // Build and exports the board outlines (board body)
    void ExportVrmlBoard();

    // Export all via holes
    void ExportVrmlViaHoles();

    void ExportFp3DModelsAsLinkedFile( const wxString& aFullFileName );

    void ExportVrmlPadHole( PAD* aPad );

    // Export a set of polygons without holes.
    // Polygons in SHAPE_POLY_SET must be without hole, i.e. holes must be linked
    // previously to their main outline.
    void ExportVrmlPolygonSet( VRML_LAYER* aVlayer, const SHAPE_POLY_SET& aOutlines );

    void writeLayers( const char* aFileName, OSTREAM* aOutputFile );

    // select the VRML layer object to draw on
    // return true if a layer has been selected.
    bool GetLayer3D( int layer, VRML_LAYER** vlayer );

    // Build the Z position of 3D layers
    void ComputeLayer3D_Zpos();

    void write_triangle_bag( std::ostream& aOut_file, const VRML_COLOR& aColor, VRML_LAYER* aLayer,
                             bool aPlane, bool aTop, double aTop_z, double aBottom_z );

    void create_vrml_shell( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID, VRML_LAYER* layer,
                            double top_z, double bottom_z );

    void create_vrml_plane( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID, VRML_LAYER* layer,
                            double aHeight, bool aTopPlane );

    SGNODE* getSGColor( VRML_COLOR_INDEX colorIdx );

    static CUSTOM_COLORS_LIST   m_SilkscreenColors;
    static CUSTOM_COLORS_LIST   m_MaskColors;
    static CUSTOM_COLORS_LIST   m_PasteColors;
    static CUSTOM_COLORS_LIST   m_FinishColors;
    static CUSTOM_COLORS_LIST   m_BoardColors;

    static KIGFX::COLOR4D       m_DefaultBackgroundTop;
    static KIGFX::COLOR4D       m_DefaultBackgroundBot;
    static KIGFX::COLOR4D       m_DefaultSilkscreen;
    static KIGFX::COLOR4D       m_DefaultSolderMask;
    static KIGFX::COLOR4D       m_DefaultSolderPaste;
    static KIGFX::COLOR4D       m_DefaultSurfaceFinish;
    static KIGFX::COLOR4D       m_DefaultBoardBody;

    IFSG_TRANSFORM     m_OutputPCB;
    VRML_LAYER         m_holes;
    VRML_LAYER         m_3D_board;
    VRML_LAYER         m_top_copper;
    VRML_LAYER         m_bot_copper;
    VRML_LAYER         m_top_silk;
    VRML_LAYER         m_bot_silk;
    VRML_LAYER         m_top_soldermask;
    VRML_LAYER         m_bot_soldermask;
    VRML_LAYER         m_top_paste;
    VRML_LAYER         m_bot_paste;
    VRML_LAYER         m_plated_holes;

    std::list<SGNODE*> m_components;
    S3D_CACHE*         m_Cache3Dmodels;

    /* true to use VRML inline{} syntax for footprint 3D models, like:
     * Inline { url "F:/tmp/pic_programmer/shapes3D/DIP-18_W7.62mm_Socket.wrl"  }
     * false to merge VRML 3D modules in the .wrl board file
     */
    bool     m_UseInlineModelsInBrdfile;

    // 3D subdirectory to copy footprint vrml 3D models when not merged in board file
    wxString m_Subdir3DFpModels;

    // true to use relative paths in VRML inline{} for footprint 3D models
    // used only if m_UseInlineModelsInBrdfile = true
    bool     m_UseRelPathIn3DModelFilename;

    // true to reuse component definitions
    bool     m_ReuseDef;

    // true if unspecified components should be included
    bool     m_includeUnspecified;

    // true if DNP components should be included
    bool     m_includeDNP;

    // scaling from 0.1 inch to desired VRML unit
    double   m_WorldScale = 1.0;

    // scaling from mm to desired VRML world scale
    double   m_BoardToVrmlScale;

    double   m_tx;             // global translation along X
    double   m_ty;             // global translation along Y

    double   m_brd_thickness; // depth of the PCB

private:
    BOARD*                m_board;
    VRML_COLOR            vrml_colors_list[VRML_COLOR_LAST];
    std::map<int, double> m_layer_z;
    SHAPE_POLY_SET        m_pcbOutlines; // stores the board main outlines

    int         m_precision;                // precision factor when exporting fp shapes
                                            // to separate files
    SGNODE*     m_sgmaterial[VRML_COLOR_LAST];
};
