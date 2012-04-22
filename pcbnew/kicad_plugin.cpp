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
#include <zones.h>
#include <kicad_plugin.h>

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

    formatter.Print( 0, "(kicad-board (version %d) (host pcbnew %s)\n", SEXPR_BOARD_FILE_VERSION,
                     formatter.Quotew( GetBuildVersion() ).c_str() );
    Format( aBoard, (OUTPUTFORMATTER*) &formatter, 1, 0 );
    formatter.Print( 0, ")\n" );
}


void PCB_IO::Format( BOARD_ITEM* aItem, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
    throw( IO_ERROR )
{
    switch( aItem->Type() )
    {
    case PCB_T:
        format( (BOARD*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_DIMENSION_T:
        format( ( DIMENSION*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_LINE_T:
        format( (DRAWSEGMENT*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_MODULE_EDGE_T:
        format( (EDGE_MODULE*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_TARGET_T:
        format( (PCB_TARGET*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_MODULE_T:
        format( (MODULE*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_PAD_T:
        format( (D_PAD*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_TEXT_T:
        format( (TEXTE_PCB*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_MODULE_TEXT_T:
        format( (TEXTE_MODULE*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_TRACE_T:
    case PCB_VIA_T:
        format( (TRACK*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    case PCB_ZONE_AREA_T:
        format( (ZONE_CONTAINER*) aItem, aFormatter, aNestLevel, aControlBits );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format item " ) + aItem->GetClass() );
    }
}


void PCB_IO::format( BOARD* aBoard, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
    throw( IO_ERROR )
{
    aFormatter->Print( aNestLevel, ")\n" );
    aFormatter->Print( 0, "\n" );

    aFormatter->Print( aNestLevel, "(general\n" );
    aFormatter->Print( aNestLevel+1, "(links %d)\n", aBoard->GetRatsnestsCount() );
    aFormatter->Print( aNestLevel+1, "(no_connects %d)\n", aBoard->m_NbNoconnect );

    // Write Bounding box info
    aFormatter->Print( aNestLevel+1,  "(area %s %s %s %s)\n",
                       FMTIU( aBoard->GetBoundingBox().GetX() ).c_str(),
                       FMTIU( aBoard->GetBoundingBox().GetY() ).c_str(),
                       FMTIU( aBoard->GetBoundingBox().GetRight() ).c_str(),
                       FMTIU( aBoard->GetBoundingBox().GetBottom() ).c_str() );
    aFormatter->Print( aNestLevel+1, "(thickness %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_BoardThickness ).c_str() );

    aFormatter->Print( aNestLevel+1, "(drawings %d)\n", aBoard->m_Drawings.GetCount() );
    aFormatter->Print( aNestLevel+1, "(tracks %d)\n", aBoard->GetNumSegmTrack() );
    aFormatter->Print( aNestLevel+1, "(zones %d)\n", aBoard->GetNumSegmZone() );
    aFormatter->Print( aNestLevel+1, "(modules %d)\n", aBoard->m_Modules.GetCount() );
    aFormatter->Print( aNestLevel+1, "(nets %d)\n", aBoard->GetNetCount() );
    aFormatter->Print( aNestLevel, ")\n\n" );

    aBoard->GetPageSettings().Format( aFormatter, aNestLevel, aControlBits );
    aBoard->GetTitleBlock().Format( aFormatter, aNestLevel, aControlBits );

    // Layers.
    aFormatter->Print( aNestLevel, "(layers %d %08X", aBoard->GetCopperLayerCount(),
                       aBoard->GetEnabledLayers() );

    if( aBoard->GetEnabledLayers() != aBoard->GetVisibleLayers() )
        aFormatter->Print( 0, " %08X", aBoard->GetVisibleLayers() );

    aFormatter->Print( 0, "\n" );

    unsigned layerMask = ALL_CU_LAYERS & aBoard->GetEnabledLayers();

    for( int layer = 0;  layerMask;  ++layer, layerMask >>= 1 )
    {
        if( layerMask & 1 )
        {
            aFormatter->Print( aNestLevel+1, "(layer%d %s %s)\n", layer,
                               aFormatter->Quotew( aBoard->GetLayerName( layer ) ).c_str(),
                               LAYER::ShowType( aBoard->GetLayerType( layer ) ) );
        }
    }

    aFormatter->Print( aNestLevel, ")\n\n" );

    // Setup
    aFormatter->Print( aNestLevel, "(setup\n" );

    // Save current default track width, for compatibility with older Pcbnew version;
    aFormatter->Print( aNestLevel+1, "(last_trace_width %s)\n",
                       FMTIU( aBoard->m_TrackWidthList[aBoard->m_TrackWidthSelector] ).c_str() );

    // Save custom tracks width list (the first is not saved here: this is the netclass value
    for( unsigned ii = 1; ii < aBoard->m_TrackWidthList.size(); ii++ )
        aFormatter->Print( aNestLevel+1, "(user_trace_width%d %s)\n",
                           ii+1, FMTIU( aBoard->m_TrackWidthList[ii] ).c_str() );

    aFormatter->Print( aNestLevel+1, "(trace_clearance %s)\n",
                       FMTIU( aBoard->m_NetClasses.GetDefault()->GetClearance() ).c_str() );

    // ZONE_SETTINGS
    aFormatter->Print( aNestLevel+1, "(zone_clearance %s)\n",
                       FMTIU( aBoard->GetZoneSettings().m_ZoneClearance ).c_str() );
    aFormatter->Print( aNestLevel+1, "(zone_45_only %d)\n",
                       aBoard->GetZoneSettings().m_Zone_45_Only );

    aFormatter->Print( aNestLevel+1, "(trace_min %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_TrackMinWidth ).c_str() );

    aFormatter->Print( aNestLevel+1, "(segment_width %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_DrawSegmentWidth ).c_str() );
    aFormatter->Print( aNestLevel+1, "(edge_width %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_EdgeSegmentWidth ).c_str() );

    // Save current default via size, for compatibility with older Pcbnew version;
    aFormatter->Print( aNestLevel+1, "(via_size %s)\n",
                       FMTIU( aBoard->m_NetClasses.GetDefault()->GetViaDiameter() ).c_str() );
    aFormatter->Print( aNestLevel+1, "(via_drill %s)\n",
                       FMTIU( aBoard->m_NetClasses.GetDefault()->GetViaDrill() ).c_str() );
    aFormatter->Print( aNestLevel+1, "(via_min_size %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_ViasMinSize ).c_str() );
    aFormatter->Print( aNestLevel+1, "(via_min_drill %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_ViasMinDrill ).c_str() );

    // Save custom vias diameters list (the first is not saved here: this is
    // the netclass value
    for( unsigned ii = 1; ii < aBoard->m_ViasDimensionsList.size(); ii++ )
        aFormatter->Print( aNestLevel+1, "(user_via%d %s %s)\n", ii,
                           FMTIU( aBoard->m_ViasDimensionsList[ii].m_Diameter ).c_str(),
                           FMTIU( aBoard->m_ViasDimensionsList[ii].m_Drill ).c_str() );

    // for old versions compatibility:
    aFormatter->Print( aNestLevel+1, "(uvia_size %s)\n",
                       FMTIU( aBoard->m_NetClasses.GetDefault()->GetuViaDiameter() ).c_str() );
    aFormatter->Print( aNestLevel+1, "(uvia_drill %s)\n",
                       FMTIU( aBoard->m_NetClasses.GetDefault()->GetuViaDrill() ).c_str() );
    aFormatter->Print( aNestLevel+1, "(uvias_allow %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_MicroViasAllowed ).c_str() );
    aFormatter->Print( aNestLevel+1, "(uvia_min_size %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_MicroViasMinSize ).c_str() );
    aFormatter->Print( aNestLevel+1, "(uvia_min_drill %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_MicroViasMinDrill ).c_str() );

    aFormatter->Print( aNestLevel+1, "(pcb_text_width %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_PcbTextWidth ).c_str() );
    aFormatter->Print( aNestLevel+1, "(pcb_text_size %s %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_PcbTextSize.x ).c_str(),
                       FMTIU( aBoard->GetDesignSettings().m_PcbTextSize.y ).c_str() );

    aFormatter->Print( aNestLevel+1, "(mod_edge_width %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_ModuleSegmentWidth ).c_str() );
    aFormatter->Print( aNestLevel+1, "(mod_text_size %s %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_ModuleTextSize.x ).c_str(),
                       FMTIU( aBoard->GetDesignSettings().m_ModuleTextSize.y ).c_str() );
    aFormatter->Print( aNestLevel+1, "(mod_text_width %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_ModuleTextWidth ).c_str() );

    aFormatter->Print( aNestLevel+1, "(pad_size %s %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetSize().x ).c_str(),
                       FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetSize().x ).c_str() );
    aFormatter->Print( aNestLevel+1, "(pad_drill %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetDrillSize().x ).c_str() );

    aFormatter->Print( aNestLevel+1, "(pad_to_mask_clearance %s)\n",
                       FMTIU( aBoard->GetDesignSettings().m_SolderMaskMargin ).c_str() );

    if( aBoard->GetDesignSettings().m_SolderPasteMargin != 0 )
        aFormatter->Print( aNestLevel+1, "(pad_to_paste_clearance %s)\n",
                           FMTIU( aBoard->GetDesignSettings().m_SolderPasteMargin ).c_str() );

    if( aBoard->GetDesignSettings().m_SolderPasteMarginRatio != 0 )
        aFormatter->Print( aNestLevel+1, "(pad_to_paste_clearance_ratio %g)\n",
                           aBoard->GetDesignSettings().m_SolderPasteMarginRatio );

    aFormatter->Print( aNestLevel+1, "(aux_axis_origin %s %s)\n",
                       FMTIU( aBoard->GetOriginAxisPosition().x ).c_str(),
                       FMTIU( aBoard->GetOriginAxisPosition().y ).c_str() );

    aFormatter->Print( aNestLevel+1, "(visible_elements %X)\n",
                       aBoard->GetDesignSettings().GetVisibleElements() );

    aFormatter->Print( aNestLevel, ")\n\n" );


    int netcount = aBoard->GetNetCount();

    for( int i = 0;  i < netcount;  ++i )
        aFormatter->Print( aNestLevel, "(net %d %s)\n",
                           aBoard->FindNet( i )->GetNet(),
                           aFormatter->Quotew( aBoard->FindNet( i )->GetNetname() ).c_str() );

    aFormatter->Print( 0, "\n" );

    // Save the default net class first.
    aBoard->m_NetClasses.GetDefault()->Format( aFormatter, aNestLevel, aControlBits );

    // Save the rest of the net classes alphabetically.
    for( NETCLASSES::const_iterator it = aBoard->m_NetClasses.begin();
         it != aBoard->m_NetClasses.end();
         ++it )
    {
        NETCLASS* netclass = it->second;
        netclass->Format( aFormatter, aNestLevel, aControlBits );
    }

    // Save the graphical items on the board (not owned by a module)
    for( BOARD_ITEM* item = aBoard->m_Drawings;  item;  item = item->Next() )
        Format( item, aFormatter, aNestLevel, aControlBits );

    aFormatter->Print( 0, "\n" );

    // Save the modules.
    for( MODULE* module = aBoard->m_Modules;  module;  module = (MODULE*) module->Next() )
    {
        Format( module, aFormatter, aNestLevel, aControlBits );
        aFormatter->Print( 0, "\n" );
    }

    // Do not save MARKER_PCBs, they can be regenerated easily.

    // Save the tracks and vias.
    for( TRACK* track = aBoard->m_Track;  track; track = track->Next() )
        Format( track, aFormatter, aNestLevel, aControlBits );

    /// @todo Add warning here that the old segment filed zones are no longer supported and
    ///       will not be saved.

    aFormatter->Print( 0, "\n" );

    // Save the polygon (which are the newer technology) zones.
    for( int i=0;  i < aBoard->GetAreaCount();  ++i )
        Format( aBoard->GetArea( i ), aFormatter, aNestLevel, aControlBits );
}


void PCB_IO::format( DIMENSION* aDimension, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
    throw( IO_ERROR )
{
    aFormatter->Print( aNestLevel, "(dimension %s (width %s) (layer %s)",
                       FMT_IU( aDimension->m_Value ).c_str(),
                       FMT_IU( aDimension->m_Width ).c_str(),
                        aFormatter->Quotew( aDimension->GetLayerName() ).c_str()  );

     if( aDimension->GetTimeStamp() )
         aFormatter->Print( 0, " (tstamp %lX)", aDimension->GetTimeStamp() );

     aFormatter->Print( 0, "\n" );

     Format( (TEXTE_PCB*) &aDimension->m_Text, aFormatter, aNestLevel+1, aControlBits );

     aFormatter->Print( aNestLevel+1, "(feature1 pts((xy %s %s) (xy %s %s)))\n",
                        FMT_IU( aDimension->m_featureLineDOx ).c_str(),
                        FMT_IU( aDimension->m_featureLineDOy ).c_str(),
                        FMT_IU( aDimension->m_featureLineDFx ).c_str(),
                        FMT_IU( aDimension->m_featureLineDFy ).c_str() );

     aFormatter->Print( aNestLevel+1, "(feature2 pts((xy %s %s) (xy %s %s)))\n",
                        FMT_IU( aDimension->m_featureLineGOx ).c_str(),
                        FMT_IU( aDimension->m_featureLineGOy ).c_str(),
                        FMT_IU( aDimension->m_featureLineGFx ).c_str(),
                        FMT_IU( aDimension->m_featureLineGFy ).c_str() );

     aFormatter->Print( aNestLevel+1, "(crossbar pts((xy %s %s) (xy %s %s)))\n",
                        FMT_IU( aDimension->m_crossBarOx ).c_str(),
                        FMT_IU( aDimension->m_crossBarOy ).c_str(),
                        FMT_IU( aDimension->m_crossBarFx ).c_str(),
                        FMT_IU( aDimension->m_crossBarFy ).c_str() );

     aFormatter->Print( aNestLevel+1, "(arrow1a pts((xy %s %s) (xy %s %s)))\n",
                        FMT_IU( aDimension->m_arrowD1Ox ).c_str(),
                        FMT_IU( aDimension->m_arrowD1Oy ).c_str(),
                        FMT_IU( aDimension->m_arrowD1Fx ).c_str(),
                        FMT_IU( aDimension->m_arrowD1Fy ).c_str() );

     aFormatter->Print( aNestLevel+1, "(arrow1b pts((xy %s %s) (xy %s %s)))\n",
                        FMT_IU( aDimension->m_arrowD2Ox ).c_str(),
                        FMT_IU( aDimension->m_arrowD2Oy ).c_str(),
                        FMT_IU( aDimension->m_arrowD2Fx ).c_str(),
                        FMT_IU( aDimension->m_arrowD2Fy ).c_str() );

     aFormatter->Print( aNestLevel+1, "(arrow2a pts((xy %s %s) (xy %s %s)))\n",
                        FMT_IU( aDimension->m_arrowG1Ox ).c_str(),
                        FMT_IU( aDimension->m_arrowG1Oy ).c_str(),
                        FMT_IU( aDimension->m_arrowG1Fx ).c_str(),
                        FMT_IU( aDimension->m_arrowG1Fy ).c_str() );

     aFormatter->Print( aNestLevel+1, "(arrow2b pts((xy %s %s) (xy %s %s)))\n",
                        FMT_IU( aDimension->m_arrowG2Ox ).c_str(),
                        FMT_IU( aDimension->m_arrowG2Oy ).c_str(),
                        FMT_IU( aDimension->m_arrowG2Fx ).c_str(),
                        FMT_IU( aDimension->m_arrowG2Fy ).c_str() );

     aFormatter->Print( aNestLevel, ")\n" );
 }


 void PCB_IO::format( DRAWSEGMENT* aSegment, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                      int aControlBits ) const
     throw( IO_ERROR )
 {
     unsigned i;

     switch( aSegment->GetShape() )
     {
     case S_SEGMENT:  // Line
         aFormatter->Print( aNestLevel, "(gr_line (pts (xy %s) (xy %s))",
                            FMT_IU( aSegment->GetStart() ).c_str(),
                            FMT_IU( aSegment->GetEnd() ).c_str() );
         break;

     case S_CIRCLE:  // Circle
         aFormatter->Print( aNestLevel, "(gr_circle (center (xy %s)) (end (xy %s))",
                            FMT_IU( aSegment->GetStart() ).c_str(),
                            FMT_IU( aSegment->GetEnd() ).c_str() );
         break;

     case S_ARC:     // Arc
         aFormatter->Print( aNestLevel, "(gr_arc (start (xy %s)) (end (xy %s)) (angle %s)",
                            FMT_IU( aSegment->GetStart() ).c_str(),
                            FMT_IU( aSegment->GetEnd() ).c_str(),
                            FMT_ANGLE( aSegment->GetAngle() ).c_str() );
         break;

     case S_POLYGON: // Polygon
         aFormatter->Print( aNestLevel, "(gr_line (pts" );

         for( i = 0;  i < aSegment->GetPolyPoints().size();  ++i )
             aFormatter->Print( 0, " (xy %s)", FMT_IU( aSegment->GetPolyPoints()[i] ).c_str() );

         aFormatter->Print( 0, ")" );
         break;

     case S_CURVE:   // Bezier curve
         aFormatter->Print( aNestLevel, "(gr_curve pts(xy(%s) xy(%s) xy(%s) xy(%s))",
                            FMT_IU( aSegment->GetStart() ).c_str(),
                            FMT_IU( aSegment->GetBezControl1() ).c_str(),
                            FMT_IU( aSegment->GetBezControl2() ).c_str(),
                            FMT_IU( aSegment->GetEnd() ).c_str() );
         break;

     default:
         wxFAIL_MSG( wxT( "Cannot format invalid DRAWSEGMENT type." ) );
     };

     aFormatter->Print( 0, " (layer %s)", aFormatter->Quotew( aSegment->GetLayerName() ).c_str() );

    if( aSegment->GetWidth() != 0 )
        aFormatter->Print( 0, " (width %s)", FMT_IU( aSegment->GetWidth() ).c_str() );

    if( aSegment->GetTimeStamp() )
        aFormatter->Print( 0, " (tstamp %lX)", aSegment->GetTimeStamp() );

    if( aSegment->GetStatus() )
        aFormatter->Print( 0, " (status %X)", aSegment->GetStatus() );

    aFormatter->Print( 0, ")\n" );
}


void PCB_IO::format( EDGE_MODULE* aModuleDrawing, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
    throw( IO_ERROR )
{
    switch( aModuleDrawing->GetShape() )
    {
    case S_SEGMENT:  // Line
        aFormatter->Print( aNestLevel, "(fp_line (pts (xy %s) (xy %s)",
                           FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                           FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    case S_CIRCLE:  // Circle
        aFormatter->Print( aNestLevel, "(fp_circle (center (xy %s)) (end (xy %s)",
                           FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                           FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    case S_ARC:     // Arc
        aFormatter->Print( aNestLevel, "(fp_arc (start (xy %s)) (end (xy %s)) (angle %s)",
                           FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                           FMT_IU( aModuleDrawing->GetEnd0() ).c_str(),
                           FMT_ANGLE( aModuleDrawing->GetAngle() ).c_str() );
        break;

    case S_POLYGON: // Polygon
        aFormatter->Print( aNestLevel, "(fp_poly (pts" );

        for( unsigned i = 0;  i < aModuleDrawing->GetPolyPoints().size();  ++i )
            aFormatter->Print( 0, " (xy %s)",
                               FMT_IU( aModuleDrawing->GetPolyPoints()[i] ).c_str() );

        break;

    case S_CURVE:   // Bezier curve
        aFormatter->Print( aNestLevel, "(fp_curve pts(xy(%s) xy(%s) xy(%s) xy(%s))",
                           FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                           FMT_IU( aModuleDrawing->GetBezControl1() ).c_str(),
                           FMT_IU( aModuleDrawing->GetBezControl2() ).c_str(),
                           FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format invalid DRAWSEGMENT type." ) );
    };

    aFormatter->Print( 0, " (layer %s)",
                       aFormatter->Quotew( aModuleDrawing->GetLayerName() ).c_str() );

    if( aModuleDrawing->GetWidth() != 0 )
        aFormatter->Print( 0, " (width %s)", FMT_IU( aModuleDrawing->GetWidth() ).c_str() );

    if( aModuleDrawing->GetTimeStamp() )
        aFormatter->Print( 0, " (tstamp %lX)", aModuleDrawing->GetTimeStamp() );

    if( aModuleDrawing->GetStatus() )
        aFormatter->Print( 0, " (status %X)", aModuleDrawing->GetStatus() );

    aFormatter->Print( 0, ")\n" );
}


void PCB_IO::format( PCB_TARGET* aTarget, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
    throw( IO_ERROR )
{
    aFormatter->Print( aNestLevel, "(target %c (pos (xy %s)) (size %s)",
                       ( aTarget->GetShape() ) ? 'x' : '+',
                       FMT_IU( aTarget->GetPosition() ).c_str(),
                       FMT_IU( aTarget->GetSize() ).c_str() );

    if( aTarget->GetWidth() != 0 )
        aFormatter->Print( aNestLevel, " (width %s)", FMT_IU( aTarget->GetWidth() ).c_str() );

    aFormatter->Print( aNestLevel, " (layer %d)", aTarget->GetLayer() );

    if( aTarget->GetTimeStamp() )
        aFormatter->Print( aNestLevel, " (tstamp %lX)", aTarget->GetTimeStamp() );

    aFormatter->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( MODULE* aModule, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
    throw( IO_ERROR )
{
    aFormatter->Print( aNestLevel, "(module %s", aFormatter->Quotew( aModule->m_LibRef ).c_str() );

    if( aModule->IsLocked() )
        aFormatter->Print( aNestLevel, " locked" );

    if( aModule->IsPlaced() )
        aFormatter->Print( aNestLevel, " placed" );

    aFormatter->Print( 0, " (tedit %lX) (tstamp %lX)\n",
                       aModule->GetLastEditTime(), aModule->GetTimeStamp() );

    aFormatter->Print( aNestLevel+1, "(at %s", FMT_IU( aModule->m_Pos ).c_str() );

    if( aModule->m_Orient != 0.0 )
        aFormatter->Print( 0, " %s", FMT_ANGLE( aModule->m_Orient ).c_str() );

    aFormatter->Print( 0, ")\n" );

    if( !aModule->m_Doc.IsEmpty() )
        aFormatter->Print( aNestLevel+1, "(descr %s)\n",
                           aFormatter->Quotew( aModule->m_Doc ).c_str() );

    if( !aModule->m_KeyWord.IsEmpty() )
        aFormatter->Print( aNestLevel+1, "(tags %s)\n",
                           aFormatter->Quotew( aModule->m_KeyWord ).c_str() );

    if( !aModule->m_Path.IsEmpty() )
        aFormatter->Print( aNestLevel+1, "(path %s)\n",
                           aFormatter->Quotew( aModule->m_Path ).c_str() );

    if( aModule->m_CntRot90 != 0 )
        aFormatter->Print( aNestLevel+1, "(autoplace_cost90 %d)\n", aModule->m_CntRot90 );

    if( aModule->m_CntRot180 != 0 )
        aFormatter->Print( aNestLevel+1, "(autoplace_cost180 %d)\n", aModule->m_CntRot180 );

    if( aModule->GetLocalSolderMaskMargin() != 0 )
        aFormatter->Print( aNestLevel+1, "(solder_mask_margin %s)\n",
                           FMT_IU( aModule->GetLocalSolderMaskMargin() ).c_str() );

    if( aModule->GetLocalSolderPasteMargin() != 0 )
        aFormatter->Print( aNestLevel+1, "(solder_paste_margin %s)\n",
                           FMT_IU( aModule->GetLocalSolderPasteMargin() ).c_str() );

    if( aModule->GetLocalSolderPasteMarginRatio() != 0 )
        aFormatter->Print( aNestLevel+1, "(solder_paste_ratio %g)\n",
                           aModule->GetLocalSolderPasteMarginRatio() );

    if( aModule->GetLocalClearance() != 0 )
        aFormatter->Print( aNestLevel+1, "(clearance %s)\n",
                           FMT_IU( aModule->GetLocalClearance() ).c_str() );

    if( aModule->m_ZoneConnection != UNDEFINED_CONNECTION )
        aFormatter->Print( aNestLevel+1, "(zone_connect %d)\n", aModule->m_ZoneConnection );

    if( aModule->m_ThermalWidth != 0 )
        aFormatter->Print( aNestLevel+1, "(thermal_width %s)\n",
                           FMT_IU( aModule->m_ThermalWidth ).c_str() );

    if( aModule->m_ThermalGap != 0 )
        aFormatter->Print( aNestLevel+1, "(thermal_gap %s)\n",
                           FMT_IU( aModule->m_ThermalGap ).c_str() );

    // Attributes
    if( aModule->m_Attributs != MOD_DEFAULT )
    {
        aFormatter->Print( aNestLevel+1, "(attr " );

        if( aModule->m_Attributs & MOD_CMS )
            aFormatter->Print( 0, " smd" );

        if( aModule->m_Attributs & MOD_VIRTUAL )
            aFormatter->Print( 0, " virtual" );

        aFormatter->Print( 0, ")\n" );
    }

    Format( (BOARD_ITEM*) aModule->m_Reference, aFormatter, aNestLevel+1, aControlBits );
    Format( (BOARD_ITEM*) aModule->m_Value, aFormatter, aNestLevel+1, aControlBits );

    // Save drawing elements.
    for( BOARD_ITEM* gr = aModule->m_Drawings;  gr;  gr = gr->Next() )
        Format( gr, aFormatter, aNestLevel+1, aControlBits );

    // Save pads.
    for( D_PAD* pad = aModule->m_Pads;  pad;  pad = pad->Next() )
        Format( pad, aFormatter, aNestLevel+1, aControlBits );

    // Save 3D info.
    for( S3D_MASTER* t3D = aModule->m_3D_Drawings;  t3D;  t3D = t3D->Next() )
    {
        if( !t3D->m_Shape3DName.IsEmpty() )
        {
            aFormatter->Print( aNestLevel+1, "(3d_shape %s\n",
                               aFormatter->Quotew( t3D->m_Shape3DName ).c_str() );

            aFormatter->Print( aNestLevel+2, "(at (xyz %.16g %.16g %.16g))\n",
                               t3D->m_MatPosition.x,
                               t3D->m_MatPosition.y,
                               t3D->m_MatPosition.z );

            aFormatter->Print( aNestLevel+2, "(scale (xyz %.16g %.16g %.16g))\n",
                               t3D->m_MatScale.x,
                               t3D->m_MatScale.y,
                               t3D->m_MatScale.z );

            aFormatter->Print( aNestLevel+2, "(rotate (xyz %.16g %.16g %.16g))\n",
                               t3D->m_MatRotation.x,
                               t3D->m_MatRotation.y,
                               t3D->m_MatRotation.z );

            aFormatter->Print( aNestLevel+1, ")\n" );
        }
    }

    aFormatter->Print( aNestLevel, ")\n" );
}




void PCB_IO::format( D_PAD* aPad, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
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

    aFormatter->Print( aNestLevel, "(pad %s %s %s (size %s)\n",
                       aFormatter->Quotew( aPad->GetPadName() ).c_str(),
                       type.c_str(), shape.c_str(),
                       FMT_IU( aPad->GetSize() ).c_str() );
    aFormatter->Print( aNestLevel+1, "(at %s", FMT_IU( aPad->GetPos0() ).c_str() );

    if( aPad->GetOrientation() != 0.0 )
        aFormatter->Print( 0, " %s", FMT_ANGLE( aPad->GetOrientation() ).c_str() );

    aFormatter->Print( 0, ")\n" );

    if( (aPad->GetDrillSize().GetWidth() > 0) || (aPad->GetDrillSize().GetHeight() > 0) )
    {
        std::string drill = (aPad->GetDrillSize().GetHeight() > 0) ?
                            FMT_IU( aPad->GetDrillSize() ).c_str() :
                            FMT_IU( aPad->GetDrillSize().GetWidth() ).c_str();
        aFormatter->Print( aNestLevel+1, "(drill %s", drill.c_str() );

        if( (aPad->GetOffset().x > 0) || (aPad->GetOffset().y > 0) )
        {
            std::string drillOffset = ( aPad->GetOffset().x > 0 ) ?
                                      FMT_IU( aPad->GetOffset() ).c_str() :
                                      FMT_IU( aPad->GetOffset().x ).c_str();
            aFormatter->Print( 0, " (offset %s)", drillOffset.c_str() );
        }

        aFormatter->Print( 0, ")\n" );
    }

    aFormatter->Print( aNestLevel+1, "(layers %08X)\n", aPad->GetLayerMask() );

    aFormatter->Print( aNestLevel+1, "(net %d %s)\n",
                       aPad->GetNet(), aFormatter->Quotew( aPad->GetNetname() ).c_str() );

    if( aPad->GetDieLength() != 0 )
        aFormatter->Print( aNestLevel+1, "(die_length %s)\n",
                           FMT_IU( aPad->GetDieLength() ).c_str() );

    if( aPad->GetLocalSolderMaskMargin() != 0 )
        aFormatter->Print( aNestLevel+1, "(solder_mask_margin %s)\n",
                           FMT_IU( aPad->GetLocalSolderMaskMargin() ).c_str() );

    if( aPad->GetLocalSolderPasteMargin() != 0 )
        aFormatter->Print( aNestLevel+1, "(solder_paste_margin %s)\n",
                           FMT_IU( aPad->GetLocalSolderPasteMargin() ).c_str() );

    if( aPad->GetLocalSolderPasteMarginRatio() != 0 )
        aFormatter->Print( aNestLevel+1, "(solder_paste_margin_ratio %g)\n",
                           aPad->GetLocalSolderPasteMarginRatio() );

    if( aPad->GetLocalClearance() != 0 )
        aFormatter->Print( aNestLevel+1, "(clearance %s)\n",
                           FMT_IU( aPad->GetLocalClearance() ).c_str() );

    if( aPad->GetZoneConnection() != UNDEFINED_CONNECTION )
        aFormatter->Print( aNestLevel+1, "(zone_connect %d)\n", aPad->GetZoneConnection() );

    if( aPad->GetThermalWidth() != 0 )
        aFormatter->Print( aNestLevel+1, "(thermal_width %s)\n",
                           FMT_IU( aPad->GetThermalWidth() ).c_str() );

    if( aPad->GetThermalGap() != 0 )
        aFormatter->Print( aNestLevel+1, "(thermal_gap %s)\n",
                           FMT_IU( aPad->GetThermalGap() ).c_str() );

    aFormatter->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TEXTE_PCB* aText, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
    throw( IO_ERROR )
{
    aFormatter->Print( aNestLevel, "(gr_text %s (at %s %s) (layer %s)",
                       aFormatter->Quotew( aText->GetText() ).c_str(),
                       FMT_IU( aText->GetPosition() ).c_str(),
                       FMT_ANGLE( aText->GetOrientation() ).c_str(),
                       aFormatter->Quotew( aText->GetLayerName() ).c_str() );

    if( aText->GetTimeStamp() )
        aFormatter->Print( 0, " (tstamp %lX)", aText->GetTimeStamp() );

    aFormatter->Print( 0, "\n" );

    aText->EDA_TEXT::Format( aFormatter, aNestLevel, aControlBits );

    aFormatter->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TEXTE_MODULE* aText, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
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

    aFormatter->Print( aNestLevel, "(fp_text %s %s (at %s %s)%s\n",
                       aFormatter->Quotew( type ).c_str(),
                       aFormatter->Quotew( aText->GetText() ).c_str(),
                       FMT_IU( aText->GetPos0() ).c_str(), FMT_ANGLE( orient ).c_str(),
                       (!aText->IsVisible()) ? " hide" : "" );

    aText->EDA_TEXT::Format( aFormatter, aNestLevel, aControlBits );

    aFormatter->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TRACK* aTrack, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
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

        aFormatter->Print( aNestLevel, "(via %s (at %s) (size %s)", type.c_str(),
                           FMT_IU( aTrack->GetStart() ).c_str(),
                           FMT_IU( aTrack->GetWidth() ).c_str() );

        if( aTrack->GetDrill() != UNDEFINED_DRILL_DIAMETER )
            aFormatter->Print( 0, " (drill %s)", FMT_IU( aTrack->GetDrill() ).c_str() );

        aFormatter->Print( 0, " (layers %s %s) (net %d)",
                           aFormatter->Quotew( board->GetLayerName( layer1 ) ).c_str(),
                           aFormatter->Quotew( board->GetLayerName( layer2 ) ).c_str(),
                           aTrack->GetNet() );
    }
    else
    {
        aFormatter->Print( aNestLevel, "(segment (start %s) (end %s) (width %s)",
                           FMT_IU( aTrack->GetStart() ).c_str(), FMT_IU( aTrack->GetEnd() ).c_str(),
                           FMT_IU( aTrack->GetWidth() ).c_str() );

        aFormatter->Print( 0, " (layer %s) (net %d)",
                           aFormatter->Quotew( aTrack->GetLayerName() ).c_str(),
                           aTrack->GetNet() );
    }

    if( aTrack->GetTimeStamp() != 0 )
        aFormatter->Print( 0, " (tstamp %lX)", aTrack->GetTimeStamp() );

    if( aTrack->GetStatus() != 0 )
        aFormatter->Print( 0, " (status %X)", aTrack->GetStatus() );

    aFormatter->Print( 0, ")\n" );
}


void PCB_IO::format( ZONE_CONTAINER* aZone, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                     int aControlBits ) const
    throw( IO_ERROR )
{
    aFormatter->Print( aNestLevel, "(zone (net %d %s) (layer %s) (tstamp %lX)\n",
                       aZone->GetNet(), aFormatter->Quotew( aZone->GetNetName() ).c_str(),
                       aFormatter->Quotew( aZone->GetLayerName() ).c_str(),
                       aZone->GetTimeStamp() );


    // Save the outline aux info
    std::string hatch;

    switch( aZone->GetHatchStyle() )
    {
    default:
    case CPolyLine::NO_HATCH:       hatch = "none";    break;
    case CPolyLine::DIAGONAL_EDGE:  hatch = "edge";    break;
    case CPolyLine::DIAGONAL_FULL:  hatch = "full";    break;
    }

    aFormatter->Print( aNestLevel+1, "(hatch %s)\n", hatch.c_str() );

    if( aZone->GetPriority() > 0 )
        aFormatter->Print( aNestLevel+1, "(priority %d)\n", aZone->GetPriority() );

    // Save pad option and clearance
    std::string padoption;

    switch( aZone->GetPadConnection() )
    {
    default:
    case PAD_IN_ZONE:       padoption = "yes";          break;
    case THERMAL_PAD:       padoption = "use_thermal";  break;
    case PAD_NOT_IN_ZONE:   padoption = "no";           break;
    }

    aFormatter->Print( aNestLevel+1, "(connect_pads %s (clearance %s))\n",
                       padoption.c_str(), FMT_IU( aZone->GetZoneClearance() ).c_str() );

    aFormatter->Print( aNestLevel+1, "(min_thickness %s)\n",
                       FMT_IU( aZone->GetMinThickness() ).c_str() );

    aFormatter->Print( aNestLevel+1,
                       "(fill %s (mode %s) (arc_segments %d) (thermal_gap %s) (thermal_bridge_width %s)\n",
                       (aZone->IsFilled()) ? "yes" : "no",
                       (aZone->GetFillMode()) ? "segment" : "polygon",
                       aZone->m_ArcToSegmentsCount,
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

    aFormatter->Print( aNestLevel+1, "(smoothing %s (radius %s))\n",
                       smoothing.c_str(), FMT_IU( aZone->GetCornerRadius() ).c_str() );

    const std::vector< CPolyPt >& cv = aZone->m_Poly->corner;

    if( cv.size() )
    {
        aFormatter->Print( aNestLevel+1, "(polygon\n");
        aFormatter->Print( aNestLevel+2, "(pts\n" );

        for( std::vector< CPolyPt >::const_iterator it = cv.begin();  it != cv.end();  ++it )
        {
            aFormatter->Print( aNestLevel+3, "(xy %s %s)\n",
                               FMT_IU( it->x ).c_str(), FMT_IU( it->y ).c_str() );

            if( it->end_contour )
            {
                aFormatter->Print( aNestLevel+2, ")\n" );

                if( it+1 != cv.end() )
                {
                    aFormatter->Print( aNestLevel+1, ")\n" );
                    aFormatter->Print( aNestLevel+1, "(polygon\n" );
                    aFormatter->Print( aNestLevel+2, "(pts\n" );
                }
            }
        }

        aFormatter->Print( aNestLevel+1, ")\n" );
    }

    // Save the PolysList
    const std::vector< CPolyPt >& fv = aZone->m_FilledPolysList;

    if( fv.size() )
    {
        aFormatter->Print( aNestLevel+1, "(filled_polygon\n" );
        aFormatter->Print( aNestLevel+2, "(pts\n" );

        for( std::vector< CPolyPt >::const_iterator it = fv.begin();  it != fv.end();  ++it )
        {
            aFormatter->Print( aNestLevel+3, "(xy %s %s)\n",
                               FMT_IU( it->x ).c_str(), FMT_IU( it->y ).c_str() );

            if( it->end_contour )
            {
                aFormatter->Print( aNestLevel+2, ")\n" );

                if( it+1 != fv.end() )
                {
                    aFormatter->Print( aNestLevel+1, ")\n" );
                    aFormatter->Print( aNestLevel+1, "(filled_polygon\n" );
                    aFormatter->Print( aNestLevel+2, "(pts\n" );
                }
            }
        }

        aFormatter->Print( aNestLevel+1, ")\n" );
    }

    // Save the filling segments list
    const std::vector< SEGMENT >& segs = aZone->m_FillSegmList;

    if( segs.size() )
    {
        aFormatter->Print( aNestLevel+1, "(fill_segments\n" );

        for( std::vector< SEGMENT >::const_iterator it = segs.begin();  it != segs.end();  ++it )
        {
            aFormatter->Print( aNestLevel+2, "(pts (xy %s) (xy %s))\n",
                               FMT_IU( it->m_Start ).c_str(),
                               FMT_IU( it->m_End ).c_str() );
        }

        aFormatter->Print( aNestLevel+1, ")\n" );
    }

    aFormatter->Print( aNestLevel, ")\n" );
}
