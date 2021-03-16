/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021
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
    VRML_COLOR_SILK,
    VRML_COLOR_TIN,
    VRML_COLOR_LAST
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


extern VRML_COLOR colors[VRML_COLOR_LAST];


// Handle the board ans its board items to convert them to a VRML representation:
class MODEL_VRML
{
private:
    double      m_layer_z[PCB_LAYER_ID_COUNT];
    SHAPE_POLY_SET  m_pcbOutlines;          // stores the board main outlines

    int         m_iMaxSeg;                  // max. sides to a small circle
    double      m_arcMinLen, m_arcMaxLen;   // min and max lengths of an arc chord

public:
    IFSG_TRANSFORM m_OutputPCB;
    VRML_LAYER  m_holes;
    VRML_LAYER  m_board;
    VRML_LAYER  m_top_copper;
    VRML_LAYER  m_bot_copper;
    VRML_LAYER  m_top_silk;
    VRML_LAYER  m_bot_silk;
    VRML_LAYER  m_top_tin;
    VRML_LAYER  m_bot_tin;
    VRML_LAYER  m_plated_holes;

    std::list< SGNODE* > m_components;

    bool m_plainPCB;

    double m_minLineWidth;    // minimum width of a VRML line segment

    double  m_tx;             // global translation along X
    double  m_ty;             // global translation along Y

    double m_brd_thickness; // depth of the PCB

    LAYER_NUM m_text_layer;
    int m_text_width;

    MODEL_VRML();
    ~MODEL_VRML();

    VRML_COLOR& GetColor( VRML_COLOR_INDEX aIndex )
    {
        return colors[aIndex];
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
    void ExportVrmlSolderMask( BOARD* aPcb );

    // Build and exports the board outlines (board body)
    void ExportVrmlBoard( BOARD* aPcb );

    void ExportVrmlZones( BOARD* aPcb, COMMIT* aCommit );

    void ExportVrmlTracks( BOARD* pcb );

    void ExportVrmlVia( BOARD* aPcb, const VIA* aVia );

    void ExportVrmlDrawsegment( PCB_SHAPE* drawseg );

    void ExportVrmlPcbtext( PCB_TEXT* text );

    void ExportRoundPadstack( BOARD* pcb,
                                double x, double y, double r,
                                LAYER_NUM bottom_layer, LAYER_NUM top_layer,
                                double hole );

    // Basic graphic shapes:
    void ExportVrmlLine( LAYER_NUM layer,
                         double startx, double starty,
                         double endx, double endy, double width );

    void ExportVrmlFpShape( FP_SHAPE* aOutline, FOOTPRINT* aFootprint );

    void ExportVrmlPad( BOARD* aPcb, PAD* aPad );

    void ExportVrmlFootprint( BOARD* aPcb, FOOTPRINT* aFootprint,
                                   std::ostream* aOutputFile );

    void ExportVrmlPadshape( VRML_LAYER* aTinLayer, PAD* aPad );

    void ExportVrmlDrawings( BOARD* pcb );

    void ExportVrmlArc( LAYER_NUM layer,
                          double centerx, double centery,
                          double arc_startx, double arc_starty,
                          double width, double arc_angle );

    void ExportVrmlCircle( LAYER_NUM layer,
                           double startx, double starty,
                           double endx, double endy, double width );

    void ExportVrmlPolygon( LAYER_NUM layer, PCB_SHAPE *aOutline,
                              double aOrientation, wxPoint aPos );

    // select the VRML layer object to draw on
    // return true if a layer has been selected.
    bool GetLayer3D( LAYER_NUM layer, VRML_LAYER** vlayer );

    // Build the Z position of 3D layers
    void ComputeLayer3D_Zpos(BOARD* pcb );

};
