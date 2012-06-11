/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
 *
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

#include <fctsys.h>
#include <kicad_string.h>
#include <common.h>
#include <build_version.h>      // LEGACY_BOARD_FILE_VERSION
#include <macros.h>
#include <3d_struct.h>

#include <class_board.h>
#include <class_module.h>
#include <class_pcb_text.h>
#include <class_dimension.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_mire.h>
#include <class_edge_mod.h>
#include <pcb_plot_params.h>
#include <zones.h>
#include <kicad_plugin.h>
#include <pcb_parser.h>

#include <wx/wfstream.h>

#define FMTIU        BOARD_ITEM::FormatInternalUnits


void PCB_IO::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    m_board = aBoard;

    wxFileOutputStream fs( aFileName );

    if( !fs.IsOk() )
    {
        m_error.Printf( _( "cannot open file '%s'" ), aFileName.GetData() );
        THROW_IO_ERROR( m_error );
    }

    STREAM_OUTPUTFORMATTER formatter( fs );

    m_out = &formatter;     // no ownership

    m_out->Print( 0, "(kicad_pcb (version %d) (host pcbnew %s)\n", SEXPR_BOARD_FILE_VERSION,
                  formatter.Quotew( GetBuildVersion() ).c_str() );

    Format( aBoard, 1 );

    m_out->Print( 0, ")\n" );
}


void PCB_IO::Format( BOARD_ITEM* aItem, int aNestLevel ) const
    throw( IO_ERROR )
{
    switch( aItem->Type() )
    {
    case PCB_T:
        format( (BOARD*) aItem, aNestLevel );
        break;

    case PCB_DIMENSION_T:
        format( ( DIMENSION*) aItem, aNestLevel );
        break;

    case PCB_LINE_T:
        format( (DRAWSEGMENT*) aItem, aNestLevel );
        break;

    case PCB_MODULE_EDGE_T:
        format( (EDGE_MODULE*) aItem, aNestLevel );
        break;

    case PCB_TARGET_T:
        format( (PCB_TARGET*) aItem, aNestLevel );
        break;

    case PCB_MODULE_T:
        format( (MODULE*) aItem, aNestLevel );
        break;

    case PCB_PAD_T:
        format( (D_PAD*) aItem, aNestLevel );
        break;

    case PCB_TEXT_T:
        format( (TEXTE_PCB*) aItem, aNestLevel );
        break;

    case PCB_MODULE_TEXT_T:
        format( (TEXTE_MODULE*) aItem, aNestLevel );
        break;

    case PCB_TRACE_T:
    case PCB_VIA_T:
        format( (TRACK*) aItem, aNestLevel );
        break;

    case PCB_ZONE_AREA_T:
        format( (ZONE_CONTAINER*) aItem, aNestLevel );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format item " ) + aItem->GetClass() );
    }
}


void PCB_IO::formatLayer( const BOARD_ITEM* aItem ) const
{
#if USE_LAYER_NAMES
    m_out->Print( 0, " (layer %s)", m_out->Quotew( aItem->GetLayerName() ).c_str() );
#else
    m_out->Print( 0, " (layer %d)", aItem->GetLayer() );
#endif
}


void PCB_IO::format( BOARD* aBoard, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel, "(general\n" );
    m_out->Print( aNestLevel+1, "(links %d)\n", aBoard->GetRatsnestsCount() );
    m_out->Print( aNestLevel+1, "(no_connects %d)\n", aBoard->m_NbNoconnect );

    // Write Bounding box info
    m_out->Print( aNestLevel+1,  "(area %s %s %s %s)\n",
                  FMTIU( aBoard->GetBoundingBox().GetX() ).c_str(),
                  FMTIU( aBoard->GetBoundingBox().GetY() ).c_str(),
                  FMTIU( aBoard->GetBoundingBox().GetRight() ).c_str(),
                  FMTIU( aBoard->GetBoundingBox().GetBottom() ).c_str() );
    m_out->Print( aNestLevel+1, "(thickness %s)\n",
                  FMTIU( aBoard->GetDesignSettings().GetBoardThickness() ).c_str() );

    m_out->Print( aNestLevel+1, "(drawings %d)\n", aBoard->m_Drawings.GetCount() );
    m_out->Print( aNestLevel+1, "(tracks %d)\n", aBoard->GetNumSegmTrack() );
    m_out->Print( aNestLevel+1, "(zones %d)\n", aBoard->GetNumSegmZone() );
    m_out->Print( aNestLevel+1, "(modules %d)\n", aBoard->m_Modules.GetCount() );
    m_out->Print( aNestLevel+1, "(nets %d)\n", aBoard->GetNetCount() );
    m_out->Print( aNestLevel, ")\n\n" );

    aBoard->GetPageSettings().Format( m_out, aNestLevel, m_ctl );
    aBoard->GetTitleBlock().Format( m_out, aNestLevel, m_ctl );

    // Layers.
    m_out->Print( aNestLevel, "(layers\n" );

    unsigned mask = LAYER_FRONT;
    unsigned layer = LAYER_N_FRONT;

    // Save only the used copper layers from front to back.
    while( mask != 0 )
    {
        if( mask & aBoard->GetEnabledLayers() )
        {
#if USE_LAYER_NAMES
            m_out->Print( aNestLevel+1, "(%s %s",
                          m_out->Quotew( aBoard->GetLayerName( layer ) ).c_str(),
                          LAYER::ShowType( aBoard->GetLayerType( layer ) ) );
#else
            m_out->Print( aNestLevel+1, "(%d %s %s", layer,
                          m_out->Quotew( aBoard->GetLayerName( layer ) ).c_str(),
                          LAYER::ShowType( aBoard->GetLayerType( layer ) ) );
#endif

            if( !( aBoard->GetVisibleLayers() & mask ) )
                m_out->Print( 0, "hide" );

            m_out->Print( 0, ")\n" );
        }

        mask >>= 1;
        layer--;
    }

    mask = ADHESIVE_LAYER_BACK;
    layer = ADHESIVE_N_BACK;

    // Save used non-copper layers in the order they are defined.
    while( layer < LAYER_COUNT )
    {
        if( mask & aBoard->GetEnabledLayers() )
        {
#if USE_LAYER_NAMES
            m_out->Print( aNestLevel+1, "(%s user",
                          m_out->Quotew( aBoard->GetLayerName( layer ) ).c_str() );
#else
            m_out->Print( aNestLevel+1, "(%d %s user", layer,
                          m_out->Quotew( aBoard->GetLayerName( layer ) ).c_str() );
#endif

            if( !( aBoard->GetVisibleLayers() & mask ) )
                m_out->Print( 0, "hide" );

            m_out->Print( 0, ")\n" );
        }

        mask <<= 1;
        layer++;
    }

    m_out->Print( aNestLevel, ")\n\n" );

    // Setup
    m_out->Print( aNestLevel, "(setup\n" );

    // Save current default track width, for compatibility with older Pcbnew version;
    m_out->Print( aNestLevel+1, "(last_trace_width %s)\n",
                  FMTIU( aBoard->GetCurrentTrackWidth() ).c_str() );

    // Save custom tracks width list (the first is not saved here: this is the netclass value
    for( unsigned ii = 1; ii < aBoard->m_TrackWidthList.size(); ii++ )
        m_out->Print( aNestLevel+1, "(user_trace_width %s)\n",
                      FMTIU( aBoard->m_TrackWidthList[ii] ).c_str() );

    m_out->Print( aNestLevel+1, "(trace_clearance %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetClearance() ).c_str() );

    // ZONE_SETTINGS
    m_out->Print( aNestLevel+1, "(zone_clearance %s)\n",
                  FMTIU( aBoard->GetZoneSettings().m_ZoneClearance ).c_str() );
    m_out->Print( aNestLevel+1, "(zone_45_only %s)\n",
                  aBoard->GetZoneSettings().m_Zone_45_Only ? "yes" : "no" );

    m_out->Print( aNestLevel+1, "(trace_min %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_TrackMinWidth ).c_str() );

    m_out->Print( aNestLevel+1, "(segment_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_DrawSegmentWidth ).c_str() );
    m_out->Print( aNestLevel+1, "(edge_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_EdgeSegmentWidth ).c_str() );

    // Save current default via size, for compatibility with older Pcbnew version;
    m_out->Print( aNestLevel+1, "(via_size %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetViaDiameter() ).c_str() );
    m_out->Print( aNestLevel+1, "(via_drill %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetViaDrill() ).c_str() );
    m_out->Print( aNestLevel+1, "(via_min_size %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ViasMinSize ).c_str() );
    m_out->Print( aNestLevel+1, "(via_min_drill %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ViasMinDrill ).c_str() );

    // Save custom vias diameters list (the first is not saved here: this is
    // the netclass value
    for( unsigned ii = 1; ii < aBoard->m_ViasDimensionsList.size(); ii++ )
        m_out->Print( aNestLevel+1, "(user_via %s %s)\n",
                      FMTIU( aBoard->m_ViasDimensionsList[ii].m_Diameter ).c_str(),
                      FMTIU( aBoard->m_ViasDimensionsList[ii].m_Drill ).c_str() );

    // for old versions compatibility:
    m_out->Print( aNestLevel+1, "(uvia_size %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetuViaDiameter() ).c_str() );
    m_out->Print( aNestLevel+1, "(uvia_drill %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetuViaDrill() ).c_str() );
    m_out->Print( aNestLevel+1, "(uvias_allowed %s)\n",
                  ( aBoard->GetDesignSettings().m_MicroViasAllowed ) ? "yes" : "no" );
    m_out->Print( aNestLevel+1, "(uvia_min_size %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_MicroViasMinSize ).c_str() );
    m_out->Print( aNestLevel+1, "(uvia_min_drill %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_MicroViasMinDrill ).c_str() );

    m_out->Print( aNestLevel+1, "(pcb_text_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_PcbTextWidth ).c_str() );
    m_out->Print( aNestLevel+1, "(pcb_text_size %s %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_PcbTextSize.x ).c_str(),
                  FMTIU( aBoard->GetDesignSettings().m_PcbTextSize.y ).c_str() );

    m_out->Print( aNestLevel+1, "(mod_edge_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ModuleSegmentWidth ).c_str() );
    m_out->Print( aNestLevel+1, "(mod_text_size %s %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ModuleTextSize.x ).c_str(),
                  FMTIU( aBoard->GetDesignSettings().m_ModuleTextSize.y ).c_str() );
    m_out->Print( aNestLevel+1, "(mod_text_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ModuleTextWidth ).c_str() );

    m_out->Print( aNestLevel+1, "(pad_size %s %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetSize().x ).c_str(),
                  FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetSize().y ).c_str() );
    m_out->Print( aNestLevel+1, "(pad_drill %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetDrillSize().x ).c_str() );

    m_out->Print( aNestLevel+1, "(pad_to_mask_clearance %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_SolderMaskMargin ).c_str() );

    if( aBoard->GetDesignSettings().m_SolderPasteMargin != 0 )
        m_out->Print( aNestLevel+1, "(pad_to_paste_clearance %s)\n",
                      FMTIU( aBoard->GetDesignSettings().m_SolderPasteMargin ).c_str() );

    if( aBoard->GetDesignSettings().m_SolderPasteMarginRatio != 0 )
        m_out->Print( aNestLevel+1, "(pad_to_paste_clearance_ratio %g)\n",
                      aBoard->GetDesignSettings().m_SolderPasteMarginRatio );

    m_out->Print( aNestLevel+1, "(aux_axis_origin %s %s)\n",
                  FMTIU( aBoard->GetOriginAxisPosition().x ).c_str(),
                  FMTIU( aBoard->GetOriginAxisPosition().y ).c_str() );

    m_out->Print( aNestLevel+1, "(visible_elements %X)\n",
                  aBoard->GetDesignSettings().GetVisibleElements() );

//    aBoard->GetPlotOptions().Format( m_out, aNestLevel+1 );

    m_out->Print( aNestLevel, ")\n\n" );


    int netcount = aBoard->GetNetCount();

    for( int i = 0;  i < netcount;  ++i )
        m_out->Print( aNestLevel, "(net %d %s)\n",
                      aBoard->FindNet( i )->GetNet(),
                      m_out->Quotew( aBoard->FindNet( i )->GetNetname() ).c_str() );

    m_out->Print( 0, "\n" );

    // Save the default net class first.
    aBoard->m_NetClasses.GetDefault()->Format( m_out, aNestLevel, m_ctl );

    // Save the rest of the net classes alphabetically.
    for( NETCLASSES::const_iterator it = aBoard->m_NetClasses.begin();
         it != aBoard->m_NetClasses.end();
         ++it )
    {
        NETCLASS* netclass = it->second;
        netclass->Format( m_out, aNestLevel, m_ctl );
    }

    // Save the graphical items on the board (not owned by a module)
    for( BOARD_ITEM* item = aBoard->m_Drawings;  item;  item = item->Next() )
        Format( item, aNestLevel );

    m_out->Print( 0, "\n" );

    // Save the modules.
    for( MODULE* module = aBoard->m_Modules;  module;  module = (MODULE*) module->Next() )
    {
        Format( module, aNestLevel );
        m_out->Print( 0, "\n" );
    }

    // Do not save MARKER_PCBs, they can be regenerated easily.

    // Save the tracks and vias.
    for( TRACK* track = aBoard->m_Track;  track; track = track->Next() )
        Format( track, aNestLevel );

    /// @todo Add warning here that the old segment filed zones are no longer supported and
    ///       will not be saved.

    m_out->Print( 0, "\n" );

    // Save the polygon (which are the newer technology) zones.
    for( int i=0;  i < aBoard->GetAreaCount();  ++i )
        Format( aBoard->GetArea( i ), aNestLevel );
}


void PCB_IO::format( DIMENSION* aDimension, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(dimension %s (width %s)",
                  FMT_IU( aDimension->m_Value ).c_str(),
                  FMT_IU( aDimension->m_Width ).c_str() );

    formatLayer( aDimension );

    if( aDimension->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aDimension->GetTimeStamp() );

    m_out->Print( 0, "\n" );

    Format( (TEXTE_PCB*) &aDimension->m_Text, aNestLevel+1 );

    m_out->Print( aNestLevel+1, "(feature1 (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_featureLineDOx ).c_str(),
                  FMT_IU( aDimension->m_featureLineDOy ).c_str(),
                  FMT_IU( aDimension->m_featureLineDFx ).c_str(),
                  FMT_IU( aDimension->m_featureLineDFy ).c_str() );

    m_out->Print( aNestLevel+1, "(feature2 (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_featureLineGOx ).c_str(),
                  FMT_IU( aDimension->m_featureLineGOy ).c_str(),
                  FMT_IU( aDimension->m_featureLineGFx ).c_str(),
                  FMT_IU( aDimension->m_featureLineGFy ).c_str() );

    m_out->Print( aNestLevel+1, "(crossbar (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_crossBarOx ).c_str(),
                  FMT_IU( aDimension->m_crossBarOy ).c_str(),
                  FMT_IU( aDimension->m_crossBarFx ).c_str(),
                  FMT_IU( aDimension->m_crossBarFy ).c_str() );

    m_out->Print( aNestLevel+1, "(arrow1a (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_arrowD1Ox ).c_str(),
                  FMT_IU( aDimension->m_arrowD1Oy ).c_str(),
                  FMT_IU( aDimension->m_arrowD1Fx ).c_str(),
                  FMT_IU( aDimension->m_arrowD1Fy ).c_str() );

    m_out->Print( aNestLevel+1, "(arrow1b (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_arrowD2Ox ).c_str(),
                  FMT_IU( aDimension->m_arrowD2Oy ).c_str(),
                  FMT_IU( aDimension->m_arrowD2Fx ).c_str(),
                  FMT_IU( aDimension->m_arrowD2Fy ).c_str() );

    m_out->Print( aNestLevel+1, "(arrow2a (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_arrowG1Ox ).c_str(),
                  FMT_IU( aDimension->m_arrowG1Oy ).c_str(),
                  FMT_IU( aDimension->m_arrowG1Fx ).c_str(),
                  FMT_IU( aDimension->m_arrowG1Fy ).c_str() );

    m_out->Print( aNestLevel+1, "(arrow2b (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_arrowG2Ox ).c_str(),
                  FMT_IU( aDimension->m_arrowG2Oy ).c_str(),
                  FMT_IU( aDimension->m_arrowG2Fx ).c_str(),
                  FMT_IU( aDimension->m_arrowG2Fy ).c_str() );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( DRAWSEGMENT* aSegment, int aNestLevel ) const
    throw( IO_ERROR )
{
    unsigned i;

    switch( aSegment->GetShape() )
    {
    case S_SEGMENT:  // Line
        m_out->Print( aNestLevel, "(gr_line (pts (xy %s) (xy %s)) (angle %s)",
                      FMT_IU( aSegment->GetStart() ).c_str(),
                      FMT_IU( aSegment->GetEnd() ).c_str(),
                      FMT_ANGLE( aSegment->GetAngle() ).c_str() );
        break;

    case S_CIRCLE:  // Circle
        m_out->Print( aNestLevel, "(gr_circle (center (xy %s)) (end (xy %s))",
                      FMT_IU( aSegment->GetStart() ).c_str(),
                      FMT_IU( aSegment->GetEnd() ).c_str() );
        break;

    case S_ARC:     // Arc
        m_out->Print( aNestLevel, "(gr_arc (start (xy %s)) (end (xy %s)) (angle %s)",
                      FMT_IU( aSegment->GetStart() ).c_str(),
                      FMT_IU( aSegment->GetEnd() ).c_str(),
                      FMT_ANGLE( aSegment->GetAngle() ).c_str() );
        break;

    case S_POLYGON: // Polygon
        m_out->Print( aNestLevel, "(gr_poly (pts" );

        for( i = 0;  i < aSegment->GetPolyPoints().size();  ++i )
            m_out->Print( 0, " (xy %s)", FMT_IU( aSegment->GetPolyPoints()[i] ).c_str() );

        m_out->Print( 0, ")" );
        break;

    case S_CURVE:   // Bezier curve
        m_out->Print( aNestLevel, "(gr_curve (pts (xy %s) (xy %s) (xy %s) (xy %s))",
                      FMT_IU( aSegment->GetStart() ).c_str(),
                      FMT_IU( aSegment->GetBezControl1() ).c_str(),
                      FMT_IU( aSegment->GetBezControl2() ).c_str(),
                      FMT_IU( aSegment->GetEnd() ).c_str() );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format invalid DRAWSEGMENT type." ) );
    };

    formatLayer( aSegment );

    if( aSegment->GetWidth() != 0 )
        m_out->Print( 0, " (width %s)", FMT_IU( aSegment->GetWidth() ).c_str() );

    if( aSegment->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aSegment->GetTimeStamp() );

    if( aSegment->GetStatus() )
        m_out->Print( 0, " (status %X)", aSegment->GetStatus() );

    m_out->Print( 0, ")\n" );
}


void PCB_IO::format( EDGE_MODULE* aModuleDrawing, int aNestLevel ) const
    throw( IO_ERROR )
{
    switch( aModuleDrawing->GetShape() )
    {
    case S_SEGMENT:  // Line
        m_out->Print( aNestLevel, "(fp_line (pts (xy %s) (xy %s))",
                      FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                      FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    case S_CIRCLE:  // Circle
        m_out->Print( aNestLevel, "(fp_circle (center (xy %s)) (end (xy %s))",
                      FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                      FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    case S_ARC:     // Arc
        m_out->Print( aNestLevel, "(fp_arc (start (xy %s)) (end (xy %s)) (angle %s)",
                      FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                      FMT_IU( aModuleDrawing->GetEnd0() ).c_str(),
                      FMT_ANGLE( aModuleDrawing->GetAngle() ).c_str() );
        break;

    case S_POLYGON: // Polygon
        m_out->Print( aNestLevel, "(fp_poly (pts" );

        for( unsigned i = 0;  i < aModuleDrawing->GetPolyPoints().size();  ++i )
            m_out->Print( 0, " (xy %s)",
                          FMT_IU( aModuleDrawing->GetPolyPoints()[i] ).c_str() );

        m_out->Print( 0, ")\n" );
        break;

    case S_CURVE:   // Bezier curve
        m_out->Print( aNestLevel, "(fp_curve (pts (xy %s) (xy %s) (xy %s) (xy %s))",
                      FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                      FMT_IU( aModuleDrawing->GetBezControl1() ).c_str(),
                      FMT_IU( aModuleDrawing->GetBezControl2() ).c_str(),
                      FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format invalid DRAWSEGMENT type." ) );
    };

    formatLayer( aModuleDrawing );

    if( aModuleDrawing->GetWidth() != 0 )
        m_out->Print( 0, " (width %s)", FMT_IU( aModuleDrawing->GetWidth() ).c_str() );

    if( aModuleDrawing->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aModuleDrawing->GetTimeStamp() );

    if( aModuleDrawing->GetStatus() )
        m_out->Print( 0, " (status %X)", aModuleDrawing->GetStatus() );

    m_out->Print( 0, ")\n" );
}


void PCB_IO::format( PCB_TARGET* aTarget, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(target %s (at %s) (size %s)",
                  ( aTarget->GetShape() ) ? "x" : "plus",
                  FMT_IU( aTarget->GetPosition() ).c_str(),
                  FMT_IU( aTarget->GetSize() ).c_str() );

    if( aTarget->GetWidth() != 0 )
        m_out->Print( aNestLevel, " (width %s)", FMT_IU( aTarget->GetWidth() ).c_str() );

    formatLayer( aTarget );

    if( aTarget->GetTimeStamp() )
        m_out->Print( aNestLevel, " (tstamp %lX)", aTarget->GetTimeStamp() );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( MODULE* aModule, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(module %s", m_out->Quotew( aModule->m_LibRef ).c_str() );

    if( aModule->IsLocked() )
        m_out->Print( aNestLevel, " locked" );

    if( aModule->IsPlaced() )
        m_out->Print( aNestLevel, " placed" );

    formatLayer( aModule );

    m_out->Print( 0, " (tedit %lX) (tstamp %lX)\n",
                       aModule->GetLastEditTime(), aModule->GetTimeStamp() );

    m_out->Print( aNestLevel+1, "(at %s", FMT_IU( aModule->m_Pos ).c_str() );

    if( aModule->m_Orient != 0.0 )
        m_out->Print( 0, " %s", FMT_ANGLE( aModule->m_Orient ).c_str() );

    m_out->Print( 0, ")\n" );

    if( !aModule->m_Doc.IsEmpty() )
        m_out->Print( aNestLevel+1, "(descr %s)\n",
                      m_out->Quotew( aModule->m_Doc ).c_str() );

    if( !aModule->m_KeyWord.IsEmpty() )
        m_out->Print( aNestLevel+1, "(tags %s)\n",
                      m_out->Quotew( aModule->m_KeyWord ).c_str() );

    if( !aModule->m_Path.IsEmpty() )
        m_out->Print( aNestLevel+1, "(path %s)\n",
                      m_out->Quotew( aModule->m_Path ).c_str() );

    if( aModule->m_CntRot90 != 0 )
        m_out->Print( aNestLevel+1, "(autoplace_cost90 %d)\n", aModule->m_CntRot90 );

    if( aModule->m_CntRot180 != 0 )
        m_out->Print( aNestLevel+1, "(autoplace_cost180 %d)\n", aModule->m_CntRot180 );

    if( aModule->GetLocalSolderMaskMargin() != 0 )
        m_out->Print( aNestLevel+1, "(solder_mask_margin %s)\n",
                      FMT_IU( aModule->GetLocalSolderMaskMargin() ).c_str() );

    if( aModule->GetLocalSolderPasteMargin() != 0 )
        m_out->Print( aNestLevel+1, "(solder_paste_margin %s)\n",
                      FMT_IU( aModule->GetLocalSolderPasteMargin() ).c_str() );

    if( aModule->GetLocalSolderPasteMarginRatio() != 0 )
        m_out->Print( aNestLevel+1, "(solder_paste_ratio %g)\n",
                      aModule->GetLocalSolderPasteMarginRatio() );

    if( aModule->GetLocalClearance() != 0 )
        m_out->Print( aNestLevel+1, "(clearance %s)\n",
                      FMT_IU( aModule->GetLocalClearance() ).c_str() );

    if( aModule->m_ZoneConnection != UNDEFINED_CONNECTION )
        m_out->Print( aNestLevel+1, "(zone_connect %d)\n", aModule->m_ZoneConnection );

    if( aModule->m_ThermalWidth != 0 )
        m_out->Print( aNestLevel+1, "(thermal_width %s)\n",
                      FMT_IU( aModule->m_ThermalWidth ).c_str() );

    if( aModule->m_ThermalGap != 0 )
        m_out->Print( aNestLevel+1, "(thermal_gap %s)\n",
                      FMT_IU( aModule->m_ThermalGap ).c_str() );

    // Attributes
    if( aModule->m_Attributs != MOD_DEFAULT )
    {
        m_out->Print( aNestLevel+1, "(attr" );

        if( aModule->m_Attributs & MOD_CMS )
            m_out->Print( 0, " smd" );

        if( aModule->m_Attributs & MOD_VIRTUAL )
            m_out->Print( 0, " virtual" );

        m_out->Print( 0, ")\n" );
    }

    Format( (BOARD_ITEM*) aModule->m_Reference, aNestLevel+1 );
    Format( (BOARD_ITEM*) aModule->m_Value, aNestLevel+1 );

    // Save drawing elements.
    for( BOARD_ITEM* gr = aModule->m_Drawings;  gr;  gr = gr->Next() )
        Format( gr, aNestLevel+1 );

    // Save pads.
    for( D_PAD* pad = aModule->m_Pads;  pad;  pad = pad->Next() )
        Format( pad, aNestLevel+1 );

    // Save 3D info.
    for( S3D_MASTER* t3D = aModule->m_3D_Drawings;  t3D;  t3D = t3D->Next() )
    {
        if( !t3D->m_Shape3DName.IsEmpty() )
        {
            m_out->Print( aNestLevel+1, "(model %s\n",
                          m_out->Quotew( t3D->m_Shape3DName ).c_str() );

            m_out->Print( aNestLevel+2, "(at (xyz %.16g %.16g %.16g))\n",
                          t3D->m_MatPosition.x,
                          t3D->m_MatPosition.y,
                          t3D->m_MatPosition.z );

            m_out->Print( aNestLevel+2, "(scale (xyz %.16g %.16g %.16g))\n",
                          t3D->m_MatScale.x,
                          t3D->m_MatScale.y,
                          t3D->m_MatScale.z );

            m_out->Print( aNestLevel+2, "(rotate (xyz %.16g %.16g %.16g))\n",
                          t3D->m_MatRotation.x,
                          t3D->m_MatRotation.y,
                          t3D->m_MatRotation.z );

            m_out->Print( aNestLevel+1, ")\n" );
        }
    }

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( D_PAD* aPad, int aNestLevel ) const
    throw( IO_ERROR )
{
    std::string shape;

    switch( aPad->GetShape() )
    {
    case PAD_CIRCLE:    shape = "circle";     break;
    case PAD_RECT:      shape = "rectangle";  break;
    case PAD_OVAL:      shape = "oval";       break;
    case PAD_TRAPEZOID: shape = "trapezoid";  break;

    default:
        THROW_IO_ERROR( wxString::Format( _( "unknown pad type: %d"), aPad->GetShape() ) );
    }

    std::string type;

    switch( aPad->GetAttribute() )
    {
    case PAD_STANDARD:          type = "thru_hole";      break;
    case PAD_SMD:               type = "smd";            break;
    case PAD_CONN:              type = "connect";        break;
    case PAD_HOLE_NOT_PLATED:   type = "np_thru_hole";   break;

    default:
        THROW_IO_ERROR( wxString::Format( _( "unknown pad attribute: %d" ),
                                          aPad->GetAttribute() ) );
    }

    m_out->Print( aNestLevel, "(pad %s %s %s (size %s)\n",
                  m_out->Quotew( aPad->GetPadName() ).c_str(),
                  type.c_str(), shape.c_str(),
                  FMT_IU( aPad->GetSize() ).c_str() );
    m_out->Print( aNestLevel+1, "(at %s", FMT_IU( aPad->GetPos0() ).c_str() );

    if( aPad->GetOrientation() != 0.0 )
        m_out->Print( 0, " %s", FMT_ANGLE( aPad->GetOrientation() ).c_str() );

    m_out->Print( 0, ")\n" );

    if( (aPad->GetDrillSize().GetWidth() > 0) || (aPad->GetDrillSize().GetHeight() > 0) )
    {
        std::string drill = (aPad->GetDrillSize().GetHeight() > 0) ?
                            FMT_IU( aPad->GetDrillSize() ).c_str() :
                            FMT_IU( aPad->GetDrillSize().GetWidth() ).c_str();
        m_out->Print( aNestLevel+1, "(drill %s", drill.c_str() );

        if( (aPad->GetOffset().x > 0) || (aPad->GetOffset().y > 0) )
        {
            std::string drillOffset = ( aPad->GetOffset().x > 0 ) ?
                                      FMT_IU( aPad->GetOffset() ).c_str() :
                                      FMT_IU( aPad->GetOffset().x ).c_str();
            m_out->Print( 0, " (offset %s)", drillOffset.c_str() );
        }

        m_out->Print( 0, ")\n" );
    }


    m_out->Print( aNestLevel+1, "(layers" );

    unsigned layerMask = aPad->GetLayerMask() & m_board->GetEnabledLayers();

    for( int layer = 0;  layerMask;  ++layer, layerMask >>= 1 )
    {
        if( layerMask & 1 )
        {
#if USE_LAYER_NAMES
            m_out->Print( 0, " %s", m_out->Quotew( m_board->GetLayerName( layer ) ).c_str() );
#else
            m_out->Print( 0, " %d", layer );
#endif
        }
    }

    m_out->Print( 0, ")\n" );

    m_out->Print( aNestLevel+1, "(net %d %s)\n",
                  aPad->GetNet(), m_out->Quotew( aPad->GetNetname() ).c_str() );

    if( aPad->GetDieLength() != 0 )
        m_out->Print( aNestLevel+1, "(die_length %s)\n",
                      FMT_IU( aPad->GetDieLength() ).c_str() );

    if( aPad->GetLocalSolderMaskMargin() != 0 )
        m_out->Print( aNestLevel+1, "(solder_mask_margin %s)\n",
                      FMT_IU( aPad->GetLocalSolderMaskMargin() ).c_str() );

    if( aPad->GetLocalSolderPasteMargin() != 0 )
        m_out->Print( aNestLevel+1, "(solder_paste_margin %s)\n",
                      FMT_IU( aPad->GetLocalSolderPasteMargin() ).c_str() );

    if( aPad->GetLocalSolderPasteMarginRatio() != 0 )
        m_out->Print( aNestLevel+1, "(solder_paste_margin_ratio %g)\n",
                      aPad->GetLocalSolderPasteMarginRatio() );

    if( aPad->GetLocalClearance() != 0 )
        m_out->Print( aNestLevel+1, "(clearance %s)\n",
                      FMT_IU( aPad->GetLocalClearance() ).c_str() );

    if( aPad->GetZoneConnection() != UNDEFINED_CONNECTION )
        m_out->Print( aNestLevel+1, "(zone_connect %d)\n", aPad->GetZoneConnection() );

    if( aPad->GetThermalWidth() != 0 )
        m_out->Print( aNestLevel+1, "(thermal_width %s)\n",
                      FMT_IU( aPad->GetThermalWidth() ).c_str() );

    if( aPad->GetThermalGap() != 0 )
        m_out->Print( aNestLevel+1, "(thermal_gap %s)\n",
                      FMT_IU( aPad->GetThermalGap() ).c_str() );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TEXTE_PCB* aText, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(gr_text %s (at %s %s)",
                  m_out->Quotew( aText->GetText() ).c_str(),
                  FMT_IU( aText->GetPosition() ).c_str(),
                  FMT_ANGLE( aText->GetOrientation() ).c_str() );

    formatLayer( aText );

    if( aText->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aText->GetTimeStamp() );

    m_out->Print( 0, "\n" );

    aText->EDA_TEXT::Format( m_out, aNestLevel, m_ctl );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TEXTE_MODULE* aText, int aNestLevel ) const
    throw( IO_ERROR )
{
    MODULE*  parent = (MODULE*) aText->GetParent();
    double   orient = aText->GetOrientation();
    wxString type;

    switch( aText->GetType() )
    {
    case 0:      type = wxT( "reference" );     break;
    case 1:      type = wxT( "value" );         break;
    default:     type = wxT( "user" );
    }

    // Due to the Pcbnew history, m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    if( parent )
        orient += parent->GetOrientation();

    m_out->Print( aNestLevel, "(fp_text %s %s (at %s %s)",
                  m_out->Quotew( type ).c_str(),
                  m_out->Quotew( aText->GetText() ).c_str(),
                  FMT_IU( aText->GetPos0() ).c_str(), FMT_ANGLE( orient ).c_str() );

    formatLayer( aText );

    if( !aText->IsVisible() )
        m_out->Print( 0, " hide" );

    m_out->Print( 0, "\n" );

    aText->EDA_TEXT::Format( m_out, aNestLevel, m_ctl );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TRACK* aTrack, int aNestLevel ) const
    throw( IO_ERROR )
{
    if( aTrack->Type() == PCB_VIA_T )
    {
        std::string type;
        int layer1, layer2;

        SEGVIA* via = (SEGVIA*) aTrack;
        BOARD* board = (BOARD*) via->GetParent();

        wxCHECK_RET( board != 0, wxT( "Via " ) + via->GetSelectMenuText() +
                     wxT( " has no parent." ) );

        via->ReturnLayerPair( &layer1, &layer2 );

        switch( aTrack->GetShape() )
        {
        case VIA_THROUGH:       type = "thru";     break;
        case VIA_BLIND_BURIED:  type = "blind";    break;
        case VIA_MICROVIA:      type = "micro";    break;
        default:
            THROW_IO_ERROR( wxString::Format( _( "unknown via type %d"  ), aTrack->GetShape() ) );
        }

        m_out->Print( aNestLevel, "(via %s (at %s) (size %s)", type.c_str(),
                      FMT_IU( aTrack->GetStart() ).c_str(),
                      FMT_IU( aTrack->GetWidth() ).c_str() );

        if( aTrack->GetDrill() != UNDEFINED_DRILL_DIAMETER )
            m_out->Print( 0, " (drill %s)", FMT_IU( aTrack->GetDrill() ).c_str() );

#if USE_LAYER_NAMES
        m_out->Print( 0, " (layers %s %s)",
                      m_out->Quotew( m_board->GetLayerName( layer1 ) ).c_str(),
                      m_out->Quotew( m_board->GetLayerName( layer2 ) ).c_str() );
#else
        m_out->Print( 0, " (layers %d %d)", layer1, layer2 );
#endif
    }
    else
    {
        m_out->Print( aNestLevel, "(segment (start %s) (end %s) (width %s)",
                      FMT_IU( aTrack->GetStart() ).c_str(), FMT_IU( aTrack->GetEnd() ).c_str(),
                      FMT_IU( aTrack->GetWidth() ).c_str() );

#if USE_LAYER_NAMES
        m_out->Print( 0, " (layer %s)", m_out->Quotew( aTrack->GetLayerName() ).c_str() );
#else
        m_out->Print( 0, " (layer %d)", aTrack->GetLayer() );
#endif
    }

    m_out->Print( 0, " (net %d)", aTrack->GetNet() );

    if( aTrack->GetTimeStamp() != 0 )
        m_out->Print( 0, " (tstamp %lX)", aTrack->GetTimeStamp() );

    if( aTrack->GetStatus() != 0 )
        m_out->Print( 0, " (status %X)", aTrack->GetStatus() );

    m_out->Print( 0, ")\n" );
}


void PCB_IO::format( ZONE_CONTAINER* aZone, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(zone (net %d) (net_name %s)",
                  aZone->GetNet(), m_out->Quotew( aZone->GetNetName() ).c_str() );

    formatLayer( aZone );

    m_out->Print( 0, " (tstamp %lX)", aZone->GetTimeStamp() );

    // Save the outline aux info
    std::string hatch;

    switch( aZone->GetHatchStyle() )
    {
    default:
    case CPolyLine::NO_HATCH:       hatch = "none";    break;
    case CPolyLine::DIAGONAL_EDGE:  hatch = "edge";    break;
    case CPolyLine::DIAGONAL_FULL:  hatch = "full";    break;
    }

    m_out->Print( 0, " (hatch %s %s)\n", hatch.c_str(),
                  FMT_IU( aZone->m_Poly->GetHatchPitch() ).c_str() );

    if( aZone->GetPriority() > 0 )
        m_out->Print( aNestLevel+1, " (priority %d)\n", aZone->GetPriority() );

    // Save pad option and clearance
    std::string padoption;

    switch( aZone->GetPadConnection() )
    {
    default:
    case PAD_IN_ZONE:       padoption = "yes";          break;
    case THERMAL_PAD:       padoption = "use_thermal";  break;
    case PAD_NOT_IN_ZONE:   padoption = "no";           break;
    }

    m_out->Print( aNestLevel+1, "(connect_pads %s (clearance %s))\n",
                  padoption.c_str(), FMT_IU( aZone->GetZoneClearance() ).c_str() );

    m_out->Print( aNestLevel+1, "(min_thickness %s)\n",
                  FMT_IU( aZone->GetMinThickness() ).c_str() );

    m_out->Print( aNestLevel+1,
                  "(fill %s (mode %s) (arc_segments %d) (thermal_gap %s) (thermal_bridge_width %s)\n",
                  (aZone->IsFilled()) ? "yes" : "no",
                  (aZone->GetFillMode()) ? "segment" : "polygon",
                  aZone->GetArcSegCount(),
                  FMT_IU( aZone->GetThermalReliefGap() ).c_str(),
                  FMT_IU( aZone->GetThermalReliefCopperBridge() ).c_str() );

    std::string smoothing;

    switch( aZone->GetCornerSmoothingType() )
    {
    case ZONE_SETTINGS::SMOOTHING_NONE:      smoothing = "none";      break;
    case ZONE_SETTINGS::SMOOTHING_CHAMFER:   smoothing = "chamfer";   break;
    case ZONE_SETTINGS::SMOOTHING_FILLET:    smoothing = "fillet";    break;
    default:
        THROW_IO_ERROR( wxString::Format( _( "unknown zone corner smoothing type %d"  ),
                                          aZone->GetCornerSmoothingType() ) );
    }

    m_out->Print( aNestLevel+1, "(smoothing %s) (radius %s))\n",
                  smoothing.c_str(), FMT_IU( aZone->GetCornerRadius() ).c_str() );

    const std::vector< CPolyPt >& cv = aZone->m_Poly->corner;

    if( cv.size() )
    {
        m_out->Print( aNestLevel+1, "(polygon\n");
        m_out->Print( aNestLevel+2, "(pts\n" );

        for( std::vector< CPolyPt >::const_iterator it = cv.begin();  it != cv.end();  ++it )
        {
            m_out->Print( aNestLevel+3, "(xy %s %s)\n",
                          FMT_IU( it->x ).c_str(), FMT_IU( it->y ).c_str() );

            if( it->end_contour )
            {
                m_out->Print( aNestLevel+2, ")\n" );

                if( it+1 != cv.end() )
                {
                    m_out->Print( aNestLevel+1, ")\n" );
                    m_out->Print( aNestLevel+1, "(polygon\n" );
                    m_out->Print( aNestLevel+2, "(pts\n" );
                }
            }
        }

        m_out->Print( aNestLevel+1, ")\n" );
    }

    // Save the PolysList
    const std::vector< CPolyPt >& fv = aZone->GetFilledPolysList();

    if( fv.size() )
    {
        m_out->Print( aNestLevel+1, "(filled_polygon\n" );
        m_out->Print( aNestLevel+2, "(pts\n" );

        for( std::vector< CPolyPt >::const_iterator it = fv.begin();  it != fv.end();  ++it )
        {
            m_out->Print( aNestLevel+3, "(xy %s %s)\n",
                          FMT_IU( it->x ).c_str(), FMT_IU( it->y ).c_str() );

            if( it->end_contour )
            {
                m_out->Print( aNestLevel+2, ")\n" );

                if( it+1 != fv.end() )
                {
                    m_out->Print( aNestLevel+1, ")\n" );
                    m_out->Print( aNestLevel+1, "(filled_polygon\n" );
                    m_out->Print( aNestLevel+2, "(pts\n" );
                }
            }
        }

        m_out->Print( aNestLevel+1, ")\n" );
    }

    // Save the filling segments list
    const std::vector< SEGMENT >& segs = aZone->m_FillSegmList;

    if( segs.size() )
    {
        m_out->Print( aNestLevel+1, "(fill_segments\n" );

        for( std::vector< SEGMENT >::const_iterator it = segs.begin();  it != segs.end();  ++it )
        {
            m_out->Print( aNestLevel+2, "(pts (xy %s) (xy %s))\n",
                          FMT_IU( it->m_Start ).c_str(),
                          FMT_IU( it->m_End ).c_str() );
        }

        m_out->Print( aNestLevel+1, ")\n" );
    }

    m_out->Print( aNestLevel, ")\n" );
}


PCB_IO::PCB_IO()
{
    m_out = &m_sf;
}


BOARD* PCB_IO::Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    wxFFile file( aFileName, "r" );

    if( !file.IsOpened() )
    {
        THROW_IO_ERROR( _( "Unable to read file \"" ) + GetChars( aFileName ) + wxT( "\"" ) );
    }

    PCB_PARSER parser( new FILE_LINE_READER( file.fp(), aFileName ), aAppendToMe );

    return (BOARD*) parser.Parse();
}
