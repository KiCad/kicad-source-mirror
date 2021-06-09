/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see change_log.txt for contributors.
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
#include <kicad_string.h>
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
extern const wxString GetPCBDefaultLayerName( LAYER_NUM aLayerNumber );


GBR_TO_PCB_EXPORTER::GBR_TO_PCB_EXPORTER( GERBVIEW_FRAME* aFrame, const wxString& aFileName )
{
    m_gerbview_frame    = aFrame;
    m_pcb_file_name     = aFileName;
    m_fp                = NULL;
    m_pcbCopperLayersCount = 2;
}


GBR_TO_PCB_EXPORTER::~GBR_TO_PCB_EXPORTER()
{
}


bool GBR_TO_PCB_EXPORTER::ExportPcb( const LAYER_NUM* aLayerLookUpTable, int aCopperLayers )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    m_fp = wxFopen( m_pcb_file_name, wxT( "wt" ) );

    if( m_fp == NULL )
    {
        wxString msg;
        msg.Printf( _( "Cannot create file \"%s\"" ), m_pcb_file_name );
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
        EXCELLON_IMAGE* excellon = dynamic_cast<EXCELLON_IMAGE*>( images->GetGbrImage( layer ) );

        if( excellon == NULL )    // Layer not yet used or not a drill image
            continue;

        for(  GERBER_DRAW_ITEM* gerb_item : excellon->GetItems() )
            collect_hole( gerb_item );
    }

    // Next: non copper layers:
    for( unsigned layer = 0; layer < images->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = images->GetGbrImage( layer );

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        LAYER_NUM pcb_layer_number = aLayerLookUpTable[layer];

        if( !IsPcbLayer( pcb_layer_number ) )
            continue;

        if( pcb_layer_number <= pcbCopperLayerMax ) // copper layer
            continue;

        for(  GERBER_DRAW_ITEM* gerb_item : gerber->GetItems() )
            export_non_copper_item( gerb_item, pcb_layer_number );
    }

    // Copper layers
    for( unsigned layer = 0; layer < images->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = images->GetGbrImage( layer );

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        LAYER_NUM pcb_layer_number = aLayerLookUpTable[layer];

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
    m_fp = NULL;
    return true;
}


void GBR_TO_PCB_EXPORTER::export_non_copper_item( const GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    // used when a D_CODE is not found. default D_CODE to draw a flashed item
    static D_CODE  dummyD_CODE( 0 );

    wxPoint        seg_start   = aGbrItem->m_Start;
    wxPoint        seg_end     = aGbrItem->m_End;
    D_CODE*        d_codeDescr = aGbrItem->GetDcodeDescr();
    SHAPE_POLY_SET polygon;

    if( d_codeDescr == NULL )
        d_codeDescr = &dummyD_CODE;

    switch( aGbrItem->m_Shape )
    {
    case GBR_POLYGON:
        writePcbPolygon( aGbrItem->m_Polygon, aLayer );
        break;

    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        d_codeDescr->ConvertShapeToPolygon();
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
                     Double2Str( MapToPcbUnits(seg_start.x) ).c_str(),
                     Double2Str( MapToPcbUnits(seg_start.y) ).c_str(),
                     Double2Str( MapToPcbUnits(seg_end.x) ).c_str(),
                     Double2Str( MapToPcbUnits(seg_end.y) ).c_str(),
                     TO_UTF8( GetPCBDefaultLayerName( aLayer ) ),
                     Double2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str()
                     );
        }
        else
        {
            fprintf( m_fp, "(gr_arc (start %s %s) (end %s %s) (angle %s) (layer %s) (width %s))\n",
                     Double2Str( MapToPcbUnits(seg_start.x) ).c_str(),
                     Double2Str( MapToPcbUnits(seg_start.y) ).c_str(),
                     Double2Str( MapToPcbUnits(seg_end.x) ).c_str(),
                     Double2Str( MapToPcbUnits(seg_end.y) ).c_str(),
                     Double2Str( angle ).c_str(),
                     TO_UTF8( GetPCBDefaultLayerName( aLayer ) ),
                     Double2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str()
                     );
        }
    }
        break;

    case GBR_CIRCLE:
        // Reverse Y axis:
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;

        fprintf( m_fp, "(gr_circle (start %s %s) (end %s %s) (layer %s) (width %s))\n",
                 Double2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                 Double2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                 Double2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                 Double2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                 TO_UTF8( GetPCBDefaultLayerName( aLayer ) ),
                 Double2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str() );
        break;

    case GBR_SEGMENT:
        // Reverse Y axis:
        seg_start.y = -seg_start.y;
        seg_end.y = -seg_end.y;

        fprintf( m_fp, "(gr_line (start %s %s) (end %s %s) (layer %s) (width %s))\n",
                 Double2Str( MapToPcbUnits( seg_start.x ) ).c_str(),
                 Double2Str( MapToPcbUnits( seg_start.y ) ).c_str(),
                 Double2Str( MapToPcbUnits( seg_end.x ) ).c_str(),
                 Double2Str( MapToPcbUnits( seg_end.y ) ).c_str(),
                 TO_UTF8( GetPCBDefaultLayerName( aLayer ) ),
                 Double2Str( MapToPcbUnits( aGbrItem->m_Size.x ) ).c_str() );
        break;
    }
}


/*
 * Many holes will be pads, but we have no way to create those without footprints, and creating
 * a footprint per pad is not really viable.
 *
 * So we use vias to mimic holes, with the loss of any hole shape (as we only have round holes
 * in vias at present).
 *
 * We start out with a via size minimally larger than the hole.  We'll leave it this way if
 * the pad gets drawn as a copper polygon, or increase it to the proper size if it has a
 * circular, concentric copper flashing.
 */
void GBR_TO_PCB_EXPORTER::collect_hole( const GERBER_DRAW_ITEM* aGbrItem )
{
    int size = std::min( aGbrItem->m_Size.x, aGbrItem->m_Size.y );
    m_vias.emplace_back( aGbrItem->m_Start, size + 1, size );
}


void GBR_TO_PCB_EXPORTER::export_via( const EXPORT_VIA& aVia )
{
    wxPoint via_pos = aVia.m_Pos;

    // Reverse Y axis:
    via_pos.y = -via_pos.y;

    // Layers are Front to Back
    fprintf( m_fp, " (via (at %s %s) (size %s) (drill %s)",
                  Double2Str( MapToPcbUnits( via_pos.x ) ).c_str(),
                  Double2Str( MapToPcbUnits( via_pos.y ) ).c_str(),
                  Double2Str( MapToPcbUnits( aVia.m_Size ) ).c_str(),
                  Double2Str( MapToPcbUnits( aVia.m_Drill ) ).c_str() );

    fprintf( m_fp, " (layers %s %s))\n",
                  TO_UTF8( GetPCBDefaultLayerName( F_Cu ) ),
                  TO_UTF8( GetPCBDefaultLayerName( B_Cu ) ) );
}


void GBR_TO_PCB_EXPORTER::export_copper_item( const GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    switch( aGbrItem->m_Shape )
    {
    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
        export_flashed_copper_item( aGbrItem, aLayer );
        break;

    case GBR_ARC:
        export_segarc_copper_item( aGbrItem, aLayer );
        break;

    case GBR_POLYGON:
        // One can use a polygon or a zone to output a Gerber region.
        // none are perfect.
        // The current way is use a polygon, as the zone export
        // is experimental and only for tests.
#if 1
        writePcbPolygon( aGbrItem->m_Polygon, aLayer );
#else
        // Only for tests:
        writePcbZoneItem( aGbrItem, aLayer );
#endif
        break;

    default:
        export_segline_copper_item( aGbrItem, aLayer );
        break;
    }
}


void GBR_TO_PCB_EXPORTER::export_segline_copper_item( const GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    wxPoint seg_start, seg_end;

    seg_start   = aGbrItem->m_Start;
    seg_end     = aGbrItem->m_End;

    // Reverse Y axis:
    seg_start.y = -seg_start.y;
    seg_end.y = -seg_end.y;

    writeCopperLineItem( seg_start, seg_end, aGbrItem->m_Size.x, aLayer );
}


void GBR_TO_PCB_EXPORTER::writeCopperLineItem( const wxPoint& aStart,
                                               const wxPoint& aEnd,
                                               int aWidth, LAYER_NUM aLayer )
{
  fprintf( m_fp, "(segment (start %s %s) (end %s %s) (width %s) (layer %s) (net 0))\n",
                  Double2Str( MapToPcbUnits(aStart.x) ).c_str(),
                  Double2Str( MapToPcbUnits(aStart.y) ).c_str(),
                  Double2Str( MapToPcbUnits(aEnd.x) ).c_str(),
                  Double2Str( MapToPcbUnits(aEnd.y) ).c_str(),
                  Double2Str( MapToPcbUnits( aWidth ) ).c_str(),
                  TO_UTF8( GetPCBDefaultLayerName( aLayer ) ) );
}


void GBR_TO_PCB_EXPORTER::export_segarc_copper_item( const GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    double  a = atan2( (double) ( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y ),
                       (double) ( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
    double  b = atan2( (double) ( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                       (double) ( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

    wxPoint start   = aGbrItem->m_Start;
    wxPoint end     = aGbrItem->m_End;

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

    wxPoint curr_start = start;
    wxPoint seg_start, seg_end;

    int     ii = 1;

    for( double rot = a; rot < (b - DELTA_ANGLE); rot += DELTA_ANGLE, ii++ )
    {
        seg_start = curr_start;
        wxPoint curr_end = start;
        RotatePoint( &curr_end, aGbrItem->m_ArcCentre,
                     -RAD2DECIDEG( DELTA_ANGLE * ii ) );
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


/*
 * Flashed items are usually pads or vias.  Pads are problematic because we have no way to
 * represent one in Pcbnew outside of a footprint (and creating a footprint per pad isn't really
 * viable).
 * If we've already created a via from a hole, and the flashed copper item is a simple circle
 * then we'll enlarge the via to the proper size.  Otherwise we create a copper polygon to
 * represent the flashed item (which is presumably a pad).
 */
void GBR_TO_PCB_EXPORTER::export_flashed_copper_item( const GERBER_DRAW_ITEM* aGbrItem,
                                                      LAYER_NUM aLayer )
{
    static D_CODE  flashed_item_D_CODE( 0 );

    D_CODE*        d_codeDescr = aGbrItem->GetDcodeDescr();
    SHAPE_POLY_SET polygon;

    if( d_codeDescr == NULL )
        d_codeDescr = &flashed_item_D_CODE;

    if( aGbrItem->m_Shape == GBR_SPOT_CIRCLE )
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

    d_codeDescr->ConvertShapeToPolygon();
    wxPoint offset = aGbrItem->GetABPosition( aGbrItem->m_Start );

    writePcbPolygon( d_codeDescr->m_Polygon, aLayer, offset );
}


void GBR_TO_PCB_EXPORTER::writePcbHeader( const LAYER_NUM* aLayerLookUpTable )
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


void GBR_TO_PCB_EXPORTER::writePcbPolygon( const SHAPE_POLY_SET& aPolys, LAYER_NUM aLayer,
                                           const wxPoint& aOffset )
{
    SHAPE_POLY_SET polys = aPolys;

    // Cleanup the polygon
    polys.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    // Ensure the polygon is valid:
    if( polys.OutlineCount() == 0 )
        return;

    polys.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    SHAPE_LINE_CHAIN& poly = polys.Outline( 0 );

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
                Double2Str( MapToPcbUnits( poly.CPoint( ii ).x + aOffset.x ) ).c_str(),
                Double2Str( MapToPcbUnits( -poly.CPoint( ii ).y + aOffset.y ) ).c_str() );
    }

    fprintf( m_fp, ")" );

    if( jj != MAX_COORD_CNT )
        fprintf( m_fp, "\n" );

    fprintf( m_fp, "(layer %s) (width 0) )\n",
             TO_UTF8( GetPCBDefaultLayerName( aLayer ) ) );
}


void GBR_TO_PCB_EXPORTER::writePcbZoneItem( const GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    SHAPE_POLY_SET polys = aGbrItem->m_Polygon;
    polys.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

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

        fprintf( m_fp, " (xy %s %s)", Double2Str( MapToPcbUnits( poly.CPoint( ii ).x ) ).c_str(),
                 Double2Str( MapToPcbUnits( -poly.CPoint( ii ).y ) ).c_str() );
    }

    fprintf( m_fp, ")\n" );

    fprintf( m_fp, "  )\n)\n" );
}
