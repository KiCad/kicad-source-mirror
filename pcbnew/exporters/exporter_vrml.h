/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

// minimum width (mm) of a VRML line
#define MIN_VRML_LINEWIDTH 0.05  // previously 0.12

// offset for art layers, mm (silk, paste, etc)
#define  ART_OFFSET 0.025
// offset for plating
#define  PLATE_OFFSET 0.005


enum VRML_COLOR_INDEX
{
    VRML_COLOR_NONE = -1,
    VRML_COLOR_PCB = 0,
    VRML_COLOR_COPPER,
    VRML_COLOR_SOLDMASK,
    VRML_COLOR_PASTE,
    VRML_COLOR_SILK,
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
                float er, float eg, float eb,
                float am, float tr, float sh )
    {
        diffuse_red = dr;
        diffuse_grn = dg;
        diffuse_blu = db;
        spec_red = sr;
        spec_grn = sg;
        spec_blu = sb;
        emit_red = er;
        emit_grn = eg;
        emit_blu = eb;

        ambient = am;
        transp  = tr;
        shiny   = sh;
    }
};


// Handle the board ans its board items to convert them to a VRML representation:
class EXPORTER_PCB_VRML
{
private:
    VRML_COLOR  vrml_colors_list[VRML_COLOR_LAST];
    double      m_layer_z[PCB_LAYER_ID_COUNT];
    SHAPE_POLY_SET  m_pcbOutlines;          // stores the board main outlines

    int         m_iMaxSeg;                  // max. sides to a small circle
    double      m_arcMinLen, m_arcMaxLen;   // min and max lengths of an arc chord
    int         m_precision;                // precision factor when exportin fp shapes
                                            // to separate files
    SGNODE*     m_sgmaterial[VRML_COLOR_LAST];

public:
    IFSG_TRANSFORM m_OutputPCB;
    VRML_LAYER  m_holes;
    VRML_LAYER  m_3D_board;
    VRML_LAYER  m_top_copper;
    VRML_LAYER  m_bot_copper;
    VRML_LAYER  m_top_silk;
    VRML_LAYER  m_bot_silk;
    VRML_LAYER  m_top_soldermask;
    VRML_LAYER  m_bot_soldermask;
    VRML_LAYER  m_top_paste;
    VRML_LAYER  m_bot_paste;
    VRML_LAYER  m_plated_holes;

    std::list< SGNODE* > m_components;
    S3D_CACHE* m_Cache3Dmodels;
    BOARD*     m_Pcb;

    /* true to use VRML inline{} syntax for footprint 3D models, like:
     * Inline { url "F:/tmp/pic_programmer/shapes3D/DIP-18_W7.62mm_Socket.wrl"  }
     * false to merge VRML 3D modeles in the .wrl board file
     */
    bool m_UseInlineModelsInBrdfile;

    // 3D subdirectory to copy footprint vrml 3D models when not merged in board file
    wxString m_Subdir3DFpModels;

    // true to use relative paths in VRML inline{} for footprint 3D models
    // used only if m_UseInlineModelsInBrdfile = true
    bool m_UseRelPathIn3DModelFilename;

    // true to reuse component definitions
    bool m_ReuseDef;

    // scaling from 0.1 inch to desired VRML unit
    double m_WorldScale = 1.0;

    // scaling from mm to desired VRML world scale
    double m_BoardToVrmlScale;

    double m_minLineWidth;    // minimum width of a VRML line segment

    double  m_tx;             // global translation along X
    double  m_ty;             // global translation along Y

    double m_brd_thickness; // depth of the PCB

    LAYER_NUM m_text_layer;
    int m_text_width;

    EXPORTER_PCB_VRML();
    ~EXPORTER_PCB_VRML();

    VRML_COLOR& GetColor( VRML_COLOR_INDEX aIndex )
    {
        return vrml_colors_list[aIndex];
    }

    void SetOffset( double aXoff, double aYoff );

    double GetLayerZ( LAYER_NUM aLayer )
    {
        if( unsigned( aLayer ) >= arrayDim( m_layer_z ) )
            return 0;

        return m_layer_z[ aLayer ];
    }

    void SetLayerZ( LAYER_NUM aLayer, double aValue )
    {
        m_layer_z[aLayer] = aValue;
    }

    // set the scaling of the VRML world
    bool SetScale( double aWorldScale );

    // Build and export the solder mask layer
    void ExportVrmlSolderMask();

    // Build and exports the board outlines (board body)
    void ExportVrmlBoard();

    // Export zones except zones on solder mask layers because these layers are
    // negative layers and must be handled in ExportVrmlSolderMask.
    void ExportVrmlZones();

    void ExportVrmlTracks();

    void ExportVrmlVia( const VIA* aVia );

    void ExportVrmlDrawsegment( PCB_SHAPE* drawseg );

    void ExportVrmlPcbtext( PCB_TEXT* text );

    void ExportVrmlFpText( FP_TEXT* item );

    void ExportFp3DModelsAsLinkedFile( const wxString& aFullFileName );

    void ExportRoundPadstack( double x, double y, double r,
                              LAYER_NUM bottom_layer, LAYER_NUM top_layer,
                              double hole );

    // Basic graphic shapes:
    void ExportVrmlLine( LAYER_NUM layer,
                         double startx, double starty,
                         double endx, double endy, double width );

    void ExportVrmlFpShape( FP_SHAPE* aOutline, FOOTPRINT* aFootprint );

    void ExportVrmlPadHole( PAD* aPad );

    void ExportVrmlFootprint( FOOTPRINT* aFootprint, std::ostream* aOutputFile );

    void ExportVrmlDrawings();

    void ExportVrmlArc( LAYER_NUM layer,
                        double centerx, double centery,
                        double arc_startx, double arc_starty,
                        double width, double arc_angle );

    void ExportVrmlCircle( LAYER_NUM layer,
                           double startx, double starty,
                           double endx, double endy, double width );

    void ExportVrmlPolygon( LAYER_NUM layer, PCB_SHAPE *aOutline,
                            double aOrientation, wxPoint aPos );

    // Exoprt a set of polygons without holes.
    // Polygons in SHAPE_POLY_SET must be without hole, i.e. holes must be linked
    // previously to their main outline.
    void ExportVrmlPolygonSet( VRML_LAYER* aVlayer, const SHAPE_POLY_SET& aOutlines );

    void writeLayers( const char* aFileName, OSTREAM* aOutputFile );

    // select the VRML layer object to draw on
    // return true if a layer has been selected.
    bool GetLayer3D( LAYER_NUM layer, VRML_LAYER** vlayer );

    // Build the Z position of 3D layers
    void ComputeLayer3D_Zpos();

private:
    void write_triangle_bag( std::ostream& aOut_file, const VRML_COLOR& aColor,
                             VRML_LAYER* aLayer, bool aPlane, bool aTop,
                             double aTop_z, double aBottom_z );

    void create_vrml_shell( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID,
                               VRML_LAYER* layer, double top_z, double bottom_z );

    void create_vrml_plane( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID,
                               VRML_LAYER* layer, double aHeight, bool aTopPlane );

    SGNODE* getSGColor( VRML_COLOR_INDEX colorIdx );
};
