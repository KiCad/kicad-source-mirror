/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <locale_io.h>
#include <macros.h>
#include <trigo.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <build_version.h>
#include <wildcards_and_files_ext.h>
#include "excellon_image.h"

// Imported function
extern const wxString GetPCBDefaultLayerName( int aLayerNumber );


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
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

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
    const int pcbCopperLayerMax = 31;
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

        if( !IsPcbLayer( pcb_layer_number ) )
            continue;

        if( pcb_layer_number <= pcbCopperLayerMax ) // copper layer
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

        if( pcb_layer_number < 0 || pcb_layer_number > pcbCopperLayerMax )
            continue;

        for( GERBER_DRAW_ITEM* gerb_item : gerber->GetItems() )
            export_copper_item( gerb_item, pcb_layer_number );
    }

    // Now write out the holes we collected earlier as vias
    for( const EXPORT_VIA& via : m_vias )
        export_via( via );

    fprintf( m_fp, ")\n" );

    fclose( m_fp );
    m_fp = nullptr;
    return true;
}


void GBR_TO_PCB_EXPORTER::export_non_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
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
    }
        break;

    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        d_codeDescr->ConvertShapeToPolygon( aGbrItem );
        writePcbPolygon( d_codeDescr->m_Polygon, aLayer, aGbrItem->GetABPosition( seg_start ) );
        break;

    case GBR_ARC:
    {
        double a = atan2( (double) ( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y ),
                          (double) ( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
        double b = atan2( (double) ( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                          (double) ( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

        double angle = RAD2DEG(b - a);
        seg_start = aGbrItem->m_ArcCentre;

        // Ensure arc orientation is CCW
        if( angle < 0 )
            angle += 360.0;

        // Reverse Y axis:
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;

        if( angle == 360.0 ||  angle == 0 )
        {
            fprintf( m_fp, "(gr_circle (center %s %s) (end %s %s) (layer %s) (width %s))\n",
                     FormatDouble2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                     TO_UTF8( GetPCBDefaultLayerName( aLayer ) ),
                     FormatDouble2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str() );
        }
        else
        {
            fprintf( m_fp, "(gr_arc (start %s %s) (end %s %s) (angle %s) (layer %s) (width %s))\n",
                     FormatDouble2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                     FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                     FormatDouble2Str( angle ).c_str(),
                     TO_UTF8( GetPCBDefaultLayerName( aLayer ) ),
                     FormatDouble2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str() );
        }
    }
        break;

    case GBR_CIRCLE:
        // Reverse Y axis:
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;

        fprintf( m_fp, "(gr_circle (start %s %s) (end %s %s) (layer %s) (width %s))\n",
                 FormatDouble2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                 TO_UTF8( GetPCBDefaultLayerName( aLayer ) ),
                 FormatDouble2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str() );
        break;

    case GBR_SEGMENT:
        // Reverse Y axis:
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;

        fprintf( m_fp, "(gr_line (start %s %s) (end %s %s) (layer %s) (width %s))\n",
                 FormatDouble2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                 TO_UTF8( GetPCBDefaultLayerName( aLayer ) ),
                 FormatDouble2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str() );
        break;
    }
}


void GBR_TO_PCB_EXPORTER::collect_hole( const GERBER_DRAW_ITEM* aGbrItem )
{
    int size = std::min( aGbrItem->m_Size.x, aGbrItem->m_Size.y );
    m_vias.emplace_back( aGbrItem->m_Start, size + 1, size );
}


void GBR_TO_PCB_EXPORTER::export_via( const EXPORT_VIA& aVia )
{
    VECTOR2I via_pos = aVia.m_Pos;

    // Reverse Y axis:
    via_pos.y = -via_pos.y;

    // Layers are Front to Back
    fprintf( m_fp, " (via (at %s %s) (size %s) (drill %s)",
             FormatDouble2Str( MapToPcbUnits( via_pos.x ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( via_pos.y ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aVia.m_Size ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aVia.m_Drill ) ).c_str() );

    fprintf( m_fp, " (layers %s %s))\n",
             TO_UTF8( GetPCBDefaultLayerName( F_Cu ) ),
             TO_UTF8( GetPCBDefaultLayerName( B_Cu ) ) );
}


void GBR_TO_PCB_EXPORTER::export_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
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
  fprintf( m_fp, "(segment (start %s %s) (end %s %s) (width %s) (layer %s) (net 0))\n",
           FormatDouble2Str( MapToPcbUnits(aStart.x) ).c_str(),
           FormatDouble2Str( MapToPcbUnits(aStart.y) ).c_str(),
           FormatDouble2Str( MapToPcbUnits(aEnd.x) ).c_str(),
           FormatDouble2Str( MapToPcbUnits(aEnd.y) ).c_str(),
           FormatDouble2Str( MapToPcbUnits( aWidth ) ).c_str(),
           TO_UTF8( GetPCBDefaultLayerName( aLayer ) ) );
}


void GBR_TO_PCB_EXPORTER::export_segarc_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    double  a = atan2( (double) ( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y ),
                       (double) ( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
    double  b = atan2( (double) ( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                       (double) ( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

    VECTOR2I start = aGbrItem->m_Start;
    VECTOR2I end = aGbrItem->m_End;

    /* Because Pcbnew does not know arcs in tracks,
     * approximate arc by segments (SEG_COUNT__CIRCLE segment per 360 deg)
     * The arc is drawn anticlockwise from the start point to the end point.
     */
    #define SEG_COUNT_CIRCLE    16
    #define DELTA_ANGLE         2 * M_PI / SEG_COUNT_CIRCLE

    // calculate the number of segments from a to b.
    // we want CNT_PER_360 segments fo a circle
    if( a > b )
        b += 2 * M_PI;

    VECTOR2I curr_start = start;
    VECTOR2I seg_start, seg_end;

    int     ii = 1;

    for( double rot = a; rot < (b - DELTA_ANGLE); rot += DELTA_ANGLE, ii++ )
    {
        seg_start = curr_start;
        VECTOR2I curr_end = start;
        RotatePoint( curr_end, aGbrItem->m_ArcCentre, -EDA_ANGLE( DELTA_ANGLE, RADIANS_T ) * ii );
        seg_end = curr_end;

        // Reverse Y axis:
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;
        writeCopperLineItem( seg_start, seg_end, aGbrItem->m_Size.x, aLayer );
        curr_start = curr_end;
    }

    if( end != curr_start )
    {
        seg_start   = curr_start;
        seg_end     = end;

        // Reverse Y axis:
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;
        writeCopperLineItem( seg_start, seg_end, aGbrItem->m_Size.x, aLayer );
    }
}

#include <wx/log.h>
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
        macroShape.Outline( 0 ).Mirror( false, true );

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

    fprintf( m_fp, "(gr_circle (center %s %s) (end %s %s)",
             FormatDouble2Str( MapToPcbUnits( aCenterPosition.x ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aCenterPosition.y ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aCenterPosition.x + aRadius ) ).c_str(),
             FormatDouble2Str( MapToPcbUnits( aCenterPosition.y ) ).c_str() );


    fprintf( m_fp, "(layer %s) (width 0) (fill solid) )\n",
             TO_UTF8( GetPCBDefaultLayerName( aLayer ) ) );
}


void GBR_TO_PCB_EXPORTER::writePcbHeader( const int* aLayerLookUpTable )
{
    fprintf( m_fp, "(kicad_pcb (version 4) (generator gerbview)\n\n" );

    // Write layers section
    fprintf( m_fp, "  (layers \n" );

    for( int ii = 0; ii < m_pcbCopperLayersCount; ii++ )
    {
        int id = ii;

        if( ii == m_pcbCopperLayersCount-1)
            id = B_Cu;

        fprintf( m_fp, "    (%d %s signal)\n", id, TO_UTF8( GetPCBDefaultLayerName( id ) ) );
    }

    for( int ii = B_Adhes; ii < PCB_LAYER_ID_COUNT; ii++ )
    {
        if( GetPCBDefaultLayerName( ii ).IsEmpty() )    // Layer not available for export
            continue;

        fprintf( m_fp, "    (%d %s user)\n", ii, TO_UTF8( GetPCBDefaultLayerName( ii ) ) );
    }

    fprintf( m_fp, "  )\n\n" );
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

    fprintf( m_fp, "(gr_poly (pts " );

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
            fprintf( m_fp, "\n" );
        }

        fprintf( m_fp, " (xy %s %s)",
                 FormatDouble2Str( MapToPcbUnits( poly.CPoint( ii ).x + aOffset.x ) ).c_str(),
                 FormatDouble2Str( MapToPcbUnits( -poly.CPoint( ii ).y + aOffset.y ) ).c_str() );
    }

    fprintf( m_fp, ")" );

    if( jj != MAX_COORD_CNT )
        fprintf( m_fp, "\n" );

    fprintf( m_fp, "(layer %s) (width 0) )\n", TO_UTF8( GetPCBDefaultLayerName( aLayer ) ) );
}


void GBR_TO_PCB_EXPORTER::writePcbZoneItem( const GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    SHAPE_POLY_SET polys = aGbrItem->m_ShapeAsPolygon.CloneDropTriangulation();
    polys.Simplify( SHAPE_POLY_SET::PM_FAST );

    if( polys.OutlineCount() == 0 )
        return;

    fprintf( m_fp, "(zone (net 0) (net_name \"\") (layer %s) (tstamp 0000000) (hatch edge 0.508)\n",
             TO_UTF8( GetPCBDefaultLayerName( aLayer ) ) );

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
