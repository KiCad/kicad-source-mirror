/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2023 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <vector>

#include <export_to_pcbnew.h>

#include <confirm.h>
#include <string_utils.h>
#include <lset.h>
#include <macros.h>
#include <trigo.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <build_version.h>
#include <wildcards_and_files_ext.h>
#include "excellon_image.h"
#include <wx/log.h>
#include <convert_basic_shapes_to_polygon.h>


GBR_TO_PCB_EXPORTER::GBR_TO_PCB_EXPORTER( GERBVIEW_FRAME* aFrame, const wxString& aFileName )
{
    m_gerbview_frame    = aFrame;
    m_pcb_file_name     = aFileName;
    m_fp                = nullptr;
    m_pcbCopperLayersCount = 2;
}


GBR_TO_PCB_EXPORTER::~GBR_TO_PCB_EXPORTER()
{
}


bool GBR_TO_PCB_EXPORTER::ExportPcb( const int* aLayerLookUpTable, int aCopperLayers )
{
    m_fp = wxFopen( m_pcb_file_name, wxT( "wt" ) );

    if( m_fp == nullptr )
    {
        wxString msg;
        msg.Printf( _( "Failed to create file '%s'." ), m_pcb_file_name );
        DisplayError( m_gerbview_frame, msg );
        return false;
    }

    m_pcbCopperLayersCount = aCopperLayers;

    writePcbHeader( aLayerLookUpTable );

    // create an image of gerber data
    GERBER_FILE_IMAGE_LIST* images = m_gerbview_frame->GetGerberLayout()->GetImagesList();

    // First collect all the holes.  We'll use these to generate pads, vias, etc.
    for( unsigned layer = 0; layer < images->ImagesMaxCount(); ++layer )
    {
        int pcb_layer_number = aLayerLookUpTable[layer];
        EXCELLON_IMAGE* excellon = dynamic_cast<EXCELLON_IMAGE*>( images->GetGbrImage( layer ) );
        GERBER_FILE_IMAGE* gerb  = dynamic_cast<GERBER_FILE_IMAGE*>( images->GetGbrImage( layer ) );

        if( excellon )
        {
            for( GERBER_DRAW_ITEM* gerb_item : excellon->GetItems() )
                collect_hole( gerb_item );
        }
        else if( gerb && pcb_layer_number == UNDEFINED_LAYER )  // PCB_LAYER_ID doesn't have an entry for Hole Data,
                                                                // but the dialog returns UNDEFINED_LAYER for it
        {
            for( GERBER_DRAW_ITEM* gerb_item : gerb->GetItems() )
                collect_hole( gerb_item );
        }
        else
        {
            continue;
        }
    }

    // Next: non copper layers:
    for( unsigned layer = 0; layer < images->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = images->GetGbrImage( layer );

        if( gerber == nullptr )    // Graphic layer not yet used
            continue;

        int pcb_layer_number = aLayerLookUpTable[layer];

        if( !IsPcbLayer( pcb_layer_number ) || IsCopperLayer( pcb_layer_number ) )
            continue;

        for( GERBER_DRAW_ITEM* gerb_item : gerber->GetItems() )
            export_non_copper_item( gerb_item, pcb_layer_number );
    }

    // Copper layers
    for( unsigned layer = 0; layer < images->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = images->GetGbrImage( layer );

        if( gerber == nullptr )    // Graphic layer not yet used
            continue;

        int pcb_layer_number = aLayerLookUpTable[layer];

        if( !IsCopperLayer( pcb_layer_number ) )
            continue;

        for( GERBER_DRAW_ITEM* gerb_item : gerber->GetItems() )
            export_copper_item( gerb_item, pcb_layer_number );
    }

    // Now write out the holes we collected earlier as vias
    for( const EXPORT_VIA& via : m_vias )
        export_via( via );

    for( const EXPORT_SLOT& slot : m_slots )
        export_slot( slot );

    fprintf( m_fp, ")\n" );

    fclose( m_fp );
    m_fp = nullptr;
    return true;
}


void GBR_TO_PCB_EXPORTER::export_non_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    if( aGbrItem->GetLayerPolarity() )
        return;

    // used when a D_CODE is not found. default D_CODE to draw a flashed item
    static D_CODE  dummyD_CODE( 0 );

    VECTOR2I       seg_start = aGbrItem->m_Start;
    VECTOR2I       seg_end = aGbrItem->m_End;
    D_CODE*        d_codeDescr = aGbrItem->GetDcodeDescr();
    SHAPE_POLY_SET polygon;

    if( d_codeDescr == nullptr )
        d_codeDescr = &dummyD_CODE;

    switch( aGbrItem->m_ShapeType )
    {
    case GBR_POLYGON:
        writePcbPolygon( aGbrItem->m_ShapeAsPolygon, aLayer );
        break;

    case GBR_SPOT_CIRCLE:
    {
        VECTOR2I center = aGbrItem->GetABPosition( seg_start );
        int radius = d_codeDescr->m_Size.x / 2;
        writePcbFilledCircle( center, radius, aLayer );
        break;
    }

    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
    {
        d_codeDescr->ConvertShapeToPolygon( aGbrItem );
        SHAPE_POLY_SET polyshape = d_codeDescr->m_Polygon;

        // Compensate the Y axis orientation ( writePcbPolygon invert the Y coordinate )
        polyshape.Outline( 0 ).Mirror( { 0, 0 }, FLIP_DIRECTION::TOP_BOTTOM );
        writePcbPolygon( polyshape, aLayer, aGbrItem->GetABPosition( seg_start ) );
        break;
    }

    case GBR_ARC:
        export_non_copper_arc( aGbrItem, aLayer );
        break;

    case GBR_CIRCLE:
        // Reverse Y axis:
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;

        fprintf( m_fp, "\t(gr_circle (start %s %s) (end %s %s) (layer %s)\n",
                 FormatDouble2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                 LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );
        export_stroke_info( aGbrItem->m_Size.x );
        fprintf( m_fp, "\t)\n" );
        break;

    case GBR_SEGMENT:
        if( d_codeDescr->m_ApertType == APT_RECT )
        {
            // Using a rectangular aperture to draw a line is deprecated since 2020
            // However old gerber file can use it (rare case) and can generate
            // strange shapes, because the rect aperture is not rotated to match the
            // line orientation.
            // So draw this line as polygon
            SHAPE_POLY_SET polyshape;
            aGbrItem->ConvertSegmentToPolygon( &polyshape );
            writePcbPolygon( polyshape, aLayer );
        }
        else
        {
            // Reverse Y axis:
            seg_start.y = -seg_start.y;
            seg_end.y = -seg_end.y;

            fprintf( m_fp, "\t(gr_line\n\t\t(start %s %s) (end %s %s) (layer %s)\n",
                     FormatDouble2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                     LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );

            export_stroke_info( aGbrItem->m_Size.x );
            fprintf( m_fp, "\t)\n" );
        }

        break;
    }
}


void GBR_TO_PCB_EXPORTER::export_non_copper_arc( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    double a = atan2( (double) ( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y ),
                      (double) ( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
    double b = atan2( (double) ( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                      (double) ( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

    VECTOR2I arc_center = aGbrItem->m_ArcCentre;
    VECTOR2I seg_start = aGbrItem->m_Start;
    VECTOR2I seg_end = aGbrItem->m_End;

    if( a > b )
        b += 2 * M_PI;

    if( seg_start == seg_end )
    {
        // Reverse Y axis:
        arc_center.y = -arc_center.y;
        seg_end.y = -seg_end.y;

        fprintf( m_fp, "\t(gr_circle\n\t\t(center %s %s) (end %s %s) (layer %s)\n",
                 FormatDouble2Str( MapToPcbUnits( arc_center.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( arc_center.y ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                 LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );
        export_stroke_info( aGbrItem->m_Size.x );
        fprintf( m_fp, "\t)\n" );
    }
    else
    {
        VECTOR2I seg_middle = GetRotated( seg_start, arc_center,
                                          -EDA_ANGLE( (b-a)/2, RADIANS_T ));

        // Reverse Y axis:
        seg_middle.y = -seg_middle.y;
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;

        fprintf( m_fp, "\t(gr_arc\n\t\t(start %s %s) (mid %s %s) (end %s %s) (layer %s)\n",
                 FormatDouble2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_middle.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_middle.y ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                 LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );

        export_stroke_info( aGbrItem->m_Size.x );
        fprintf( m_fp, "\t)\n" );
    }
}


void GBR_TO_PCB_EXPORTER::collect_hole( const GERBER_DRAW_ITEM* aGbrItem )
{
    if( aGbrItem->m_ShapeType == GBR_SPOT_CIRCLE )
        m_vias.emplace_back( aGbrItem->m_Start, aGbrItem->m_Size.x + 1, aGbrItem->m_Size.x );
    else if( aGbrItem->m_ShapeType == GBR_SEGMENT )
        m_slots.emplace_back( aGbrItem->m_Start, aGbrItem->m_End, aGbrItem->m_Size.x );
}


void GBR_TO_PCB_EXPORTER::export_via( const EXPORT_VIA& aVia )
{
    VECTOR2I via_pos = aVia.m_Pos;

    // Reverse Y axis:
    via_pos.y = -via_pos.y;

    // Layers are Front to Back
    fprintf( m_fp, "\t(via (at %s %s) (size %s) (drill %s)",
             FormatDouble2Str( MapToPcbUnits( via_pos.x ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( via_pos.y ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aVia.m_Size ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aVia.m_Drill ) ).c_str() );

    fprintf( m_fp, " (layers %s %s))\n",
             LSET::Name( F_Cu ).ToStdString().c_str(),
             LSET::Name( B_Cu ).ToStdString().c_str() );
}


void GBR_TO_PCB_EXPORTER::export_slot( const EXPORT_SLOT& aSlot )
{
    VECTOR2I start = aSlot.m_Start;
    VECTOR2I end = aSlot.m_End;

    // Reverse Y axis:
    start.y = -start.y;
    end.y = -end.y;

    VECTOR2I dir = end - start;
    int      minorAxis = aSlot.m_Width;
    int      majorAxis = aSlot.m_Width + dir.EuclideanNorm();
    VECTOR2I center = ( start + end ) / 2;

    fprintf( m_fp, "\t(footprint \"slot\" (pad 1 thru_hole oval (at %s %s %s) (size %s %s) (drill oval %s %s)))\n",
             FormatDouble2Str( MapToPcbUnits( center.x ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( center.y ) ).c_str(),
             FormatDouble2Str( EDA_ANGLE( dir ).AsDegrees() ).c_str(),
             FormatDouble2Str( MapToPcbUnits( majorAxis + 1 ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( minorAxis + 1 ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( majorAxis ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( minorAxis ) ).c_str() );
}


void GBR_TO_PCB_EXPORTER::export_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    if( aGbrItem->GetLayerPolarity() )
        return;

    switch( aGbrItem->m_ShapeType )
    {
    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        export_flashed_copper_item( aGbrItem, aLayer );
        break;

    case GBR_CIRCLE:
    case GBR_ARC:
        export_segarc_copper_item( aGbrItem, aLayer );
        break;

    case GBR_POLYGON:
        // One can use a polygon or a zone to output a Gerber region.
        // none are perfect.
        // The current way is use a polygon, as the zone export
        // is experimental and only for tests.
#if 1
        writePcbPolygon( aGbrItem->m_ShapeAsPolygon, aLayer );
#else
        // Only for tests:
        writePcbZoneItem( aGbrItem, aLayer );
#endif
        break;

    case GBR_SEGMENT:
    {
        D_CODE* code = aGbrItem->GetDcodeDescr();

        if( code && code->m_ApertType == APT_RECT )
        {
            if( aGbrItem->m_ShapeAsPolygon.OutlineCount() == 0 )
                const_cast<GERBER_DRAW_ITEM*>( aGbrItem )->ConvertSegmentToPolygon();

            writePcbPolygon( aGbrItem->m_ShapeAsPolygon, aLayer );
        }
        else
        {
            export_segline_copper_item( aGbrItem, aLayer );
        }

        break;
    }

    default:
        break;
    }
}


void GBR_TO_PCB_EXPORTER::export_segline_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    VECTOR2I seg_start, seg_end;

    seg_start   = aGbrItem->m_Start;
    seg_end     = aGbrItem->m_End;

    // Reverse Y axis:
    seg_start.y = -seg_start.y;
    seg_end.y = -seg_end.y;

    writeCopperLineItem( seg_start, seg_end, aGbrItem->m_Size.x, aLayer );
}


void GBR_TO_PCB_EXPORTER::writeCopperLineItem( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                               int aWidth, int aLayer )
{
  fprintf( m_fp, "\t(segment (start %s %s) (end %s %s) (width %s) (layer %s) (net 0))\n",
           FormatDouble2Str( MapToPcbUnits(aStart.x) ).c_str(),
           FormatDouble2Str( MapToPcbUnits(aStart.y) ).c_str(),
           FormatDouble2Str( MapToPcbUnits(aEnd.x) ).c_str(),
           FormatDouble2Str( MapToPcbUnits(aEnd.y) ).c_str(),
           FormatDouble2Str( MapToPcbUnits( aWidth ) ).c_str(),
           LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );
}


void GBR_TO_PCB_EXPORTER::export_stroke_info( double aWidth )
{
    fprintf( m_fp, "\t\t(stroke (width %s) (type solid))\n",
                FormatDouble2Str( MapToPcbUnits( aWidth ) ).c_str() );
}


void GBR_TO_PCB_EXPORTER::export_segarc_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    double  a = atan2( (double) ( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y ),
                       (double) ( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
    double  b = atan2( (double) ( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                       (double) ( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

    if( a > b )
        b += 2 * M_PI;

    VECTOR2I arc_center = aGbrItem->m_ArcCentre;
    VECTOR2I seg_end = aGbrItem->m_End;
    VECTOR2I seg_start = aGbrItem->m_Start;

    VECTOR2I seg_middle = GetRotated( seg_start, arc_center,
                                      -EDA_ANGLE( (b-a)/2, RADIANS_T ));

    // Reverse Y axis:
    seg_end.y = -seg_end.y;
    seg_start.y = -seg_start.y;
    seg_middle.y = -seg_middle.y;

    fprintf( m_fp, "\t(arc\n\t\t(start %s %s) (mid %s %s) (end %s %s) (layer %s)\n",
             FormatDouble2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( seg_middle.x ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( seg_middle.y ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
             LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );

    fprintf( m_fp, "\t\t(width %s) (net 0 )\n",
             FormatDouble2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str() );
    fprintf( m_fp, "\t)\n" );
}


void GBR_TO_PCB_EXPORTER::export_flashed_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    static D_CODE  flashed_item_D_CODE( 0 );

    D_CODE*        d_codeDescr = aGbrItem->GetDcodeDescr();

    if( d_codeDescr == nullptr )
        d_codeDescr = &flashed_item_D_CODE;

    if( aGbrItem->m_ShapeType == GBR_SPOT_CIRCLE )
    {
        // See if there's a via that we can enlarge to fit this flashed item
        for( EXPORT_VIA& via : m_vias )
        {
            if( via.m_Pos == aGbrItem->m_Start )
            {
                via.m_Size = std::max( via.m_Size, aGbrItem->m_Size.x );
                return;
            }
        }
    }

    VECTOR2I offset = aGbrItem->GetABPosition( aGbrItem->m_Start );

    if( aGbrItem->m_ShapeType == GBR_SPOT_CIRCLE ||
        ( aGbrItem->m_ShapeType == GBR_SPOT_OVAL && d_codeDescr->m_Size.x == d_codeDescr->m_Size.y ) )
    {
        // export it as filled circle
        VECTOR2I center = offset;
        int radius = d_codeDescr->m_Size.x / 2;
        writePcbFilledCircle( center, radius, aLayer );
        return;
    }

    APERTURE_MACRO* macro = d_codeDescr->GetMacro();

    if( macro )     // export a GBR_SPOT_MACRO
    {
        SHAPE_POLY_SET macroShape;
        macroShape = *macro->GetApertureMacroShape( aGbrItem, VECTOR2I( 0, 0 ) );

        // Compensate the Y axis orientation ( writePcbPolygon invert the Y coordinate )
        macroShape.Outline( 0 ).Mirror( { 0, 0 }, FLIP_DIRECTION::TOP_BOTTOM );

        writePcbPolygon( macroShape, aLayer, offset );
    }
    else
    {
        // Should cover primitives: GBR_SPOT_RECT, GBR_SPOT_OVAL, GBR_SPOT_POLY
        d_codeDescr->ConvertShapeToPolygon( aGbrItem );
        writePcbPolygon( d_codeDescr->m_Polygon, aLayer, offset );
    }
}


void GBR_TO_PCB_EXPORTER::writePcbFilledCircle( const VECTOR2I& aCenterPosition, int aRadius,
                                                int aLayer )
{
    fprintf( m_fp, "\t(gr_circle\n\t\t(center %s %s) (end %s %s)\n",
             FormatDouble2Str( MapToPcbUnits( aCenterPosition.x ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aCenterPosition.y ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aCenterPosition.x + aRadius ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aCenterPosition.y ) ).c_str() );

    export_stroke_info( 0 );
    fprintf( m_fp, "\t\t(fill yes) (layer %s)",
             LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );
    fprintf( m_fp, "\n\t)\n" );
}


void GBR_TO_PCB_EXPORTER::writePcbHeader( const int* aLayerLookUpTable )
{
    // Note: the .kicad_pcb version used here is after layers_id changes
    fprintf( m_fp, "(kicad_pcb (version 20240928)\n" );
    fprintf( m_fp, "\t(generator \"gerbview\")\n\t(generator_version \"%s\")\n\n",
             GetMajorMinorVersion().c_str().AsChar() );

    // Write layers section
    fprintf( m_fp, "\t(layers \n" );

    LSET layer_set = LSET::AllCuMask( m_pcbCopperLayersCount ) | LSET::AllTechMask() | LSET::UserMask();

    for( auto cu_it = layer_set.copper_layers_begin(); cu_it != layer_set.copper_layers_end(); ++cu_it )
    {
        fprintf( m_fp, "\t\t(%d %s signal)\n",
                 *cu_it, LSET::Name( *cu_it ).ToStdString().c_str() );
    }

    for( auto non_cu_it = layer_set.non_copper_layers_begin(); non_cu_it != layer_set.non_copper_layers_end(); ++non_cu_it )
    {
        fprintf( m_fp, "\t\t(%d %s user)\n",
                 *non_cu_it, LSET::Name( *non_cu_it ).ToStdString().c_str() );
    }

    fprintf( m_fp, "\t)\n\n" );
}


void GBR_TO_PCB_EXPORTER::writePcbPolygon( const SHAPE_POLY_SET& aPolys, int aLayer,
                                           const VECTOR2I& aOffset )
{
    // Ensure the polygon is valid:
    if( aPolys.OutlineCount() < 1 )
        return;

    // aPolys is expected having only one outline and no hole
    // (because it comes from a gerber file or is built from a aperture )
    const SHAPE_LINE_CHAIN& poly = aPolys.COutline( 0 );

    fprintf( m_fp, "\t(gr_poly\n\t\t(pts\n\t\t\t" );

    #define MAX_COORD_CNT 4
    int jj = MAX_COORD_CNT;
    int cnt_max = poly.PointCount() -1;

    // Do not generate last corner, if it is the same point as the first point:
    if( poly.CPoint( 0 ) == poly.CPoint( cnt_max ) )
        cnt_max--;

    for( int ii = 0; ii <= cnt_max; ii++ )
    {
        if( --jj == 0 )
        {
            jj = MAX_COORD_CNT;
            fprintf( m_fp, "\n\t\t\t" );
        }

        fprintf( m_fp, " (xy %s %s)",
                 FormatDouble2Str( MapToPcbUnits( poly.CPoint( ii ).x + aOffset.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( -poly.CPoint( ii ).y + aOffset.y ) ).c_str() );
    }

    fprintf( m_fp, ")" );

    fprintf( m_fp, "\n" );
    export_stroke_info( 0 );
    fprintf( m_fp, "\t\t(fill yes) (layer %s)",
             LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );
    fprintf( m_fp, "\n\t)\n" );
}


void GBR_TO_PCB_EXPORTER::writePcbZoneItem( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    SHAPE_POLY_SET polys = aGbrItem->m_ShapeAsPolygon.CloneDropTriangulation();
    polys.Simplify();

    if( polys.OutlineCount() == 0 )
        return;

    fprintf( m_fp, "\t(zone (net 0) (net_name \"\") (layer %s) (tstamp 0000000) (hatch edge 0.508)\n",
             LSET::Name( PCB_LAYER_ID( aLayer ) ).ToStdString().c_str() );

    fprintf( m_fp, "  (connect_pads (clearance 0.0))\n" );

    fprintf( m_fp, "  (min_thickness 0.1) (filled_areas_thickness no)\n"
                   "  (fill (thermal_gap 0.3) (thermal_bridge_width 0.3))\n" );

    // Now, write the zone outlines with holes.
    // first polygon is the main outline, next are holes
    // One cannot know the initial zone outline.
    // However most of (if not all) holes are just items with clearance,
    // not really a hole in the initial zone outline.
    // So we build a zone outline only with no hole.
    fprintf( m_fp, "  (polygon\n    (pts" );

    SHAPE_LINE_CHAIN& poly = polys.Outline( 0 );

    #define MAX_COORD_CNT 4
    int jj = MAX_COORD_CNT;
    int cnt_max = poly.PointCount() -1;

    // Do not generate last corner, if it is the same point as the first point:
    if( poly.CPoint( 0 ) == poly.CPoint( cnt_max ) )
        cnt_max--;

    for( int ii = 0; ii <= cnt_max; ii++ )
    {
        if( --jj == 0 )
        {
            jj = MAX_COORD_CNT;
            fprintf( m_fp, "\n   " );
        }

        fprintf( m_fp, " (xy %s %s)", FormatDouble2Str( MapToPcbUnits( poly.CPoint( ii ).x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( -poly.CPoint( ii ).y ) ).c_str() );
    }

    fprintf( m_fp, ")\n" );

    fprintf( m_fp, "  )\n)\n" );
}
