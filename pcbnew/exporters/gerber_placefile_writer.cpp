/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file gerber_placefile_writer.cpp
 * @brief Functions to create place files in gerber X2 format.
 */

#include "gerber_placefile_writer.h"

#include <vector>

#include <plotter.h>
#include <plotters_specific.h>
#include <kicad_string.h>
#include <locale_io.h>
#include <pcb_edit_frame.h>
#include <pgm_base.h>

#include <board.h>

#include <pcbplot.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <gbr_metadata.h>
#include <footprint.h>


PLACEFILE_GERBER_WRITER::PLACEFILE_GERBER_WRITER( BOARD* aPcb )
{
    m_pcb                 = aPcb;
    m_plotPad1Marker      = true; // Place a marker to pin 1 (or A1) position
    m_plotOtherPadsMarker = true; // Place a marker to other pins position
    m_layer               = PCB_LAYER_ID::UNDEFINED_LAYER; // No layer set
}


int PLACEFILE_GERBER_WRITER::CreatePlaceFile( wxString& aFullFilename, PCB_LAYER_ID aLayer,
                                              bool aIncludeBrdEdges )
{
    m_layer = aLayer;

    PCB_PLOT_PARAMS plotOpts = m_pcb->GetPlotOptions();

    if( plotOpts.GetUseAuxOrigin() )
        m_offset = m_pcb->GetDesignSettings().m_AuxOrigin;

    // Collect footprints on the right layer
    std::vector<FOOTPRINT*> fp_list;

    for( FOOTPRINT* footprint : m_pcb->Footprints() )
    {
        if( footprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES )
             continue;

        if( footprint->GetLayer() == aLayer )
           fp_list.push_back( footprint );
    }

    LOCALE_IO dummy_io;     // Use the standard notation for float numbers

    GERBER_PLOTTER plotter;

    // Gerber drill file imply X2 format:
    plotter.UseX2format( true );
    plotter.UseX2NetAttributes( true );

    // Add the standard X2 header, without FileFunction
    AddGerberX2Header( &plotter, m_pcb );
    plotter.SetViewport( m_offset, IU_PER_MILS/10, /* scale */ 1.0, /* mirror */false );
    // has meaning only for gerber plotter. Must be called only after SetViewport
    plotter.SetGerberCoordinatesFormat( 6 );
    plotter.SetCreator( wxT( "PCBNEW" ) );

    // Add the standard X2 FileFunction for P&P files
    // %TF.FileFunction,Component,Ln,[top][bottom]*%
    wxString text;
    text.Printf( "%%TF.FileFunction,Component,L%d,%s*%%",
                 aLayer == B_Cu ? m_pcb->GetCopperLayerCount() : 1,
                 aLayer == B_Cu ? "Bot" : "Top" );
    plotter.AddLineToHeader( text );

    // Add file polarity (positive)
    text = "%TF.FilePolarity,Positive*%";
    plotter.AddLineToHeader( text );

    if( !plotter.OpenFile( aFullFilename ) )
        return -1;

    // We need a BRDITEMS_PLOTTER to plot pads
    BRDITEMS_PLOTTER brd_plotter( &plotter, m_pcb, plotOpts );

    plotter.StartPlot();

    // Some tools in P&P files have the type and size defined.
    // they are position flash (round), pad1 flash (diamond), other pads flash (round)
    // and component outline thickness (polyline)
    int flash_position_shape_diam = Millimeter2iu( 0.3 );   // defined size for position shape (circle)
    int pad1_mark_size = Millimeter2iu( 0.36 );             // defined size for pad 1 position (diamond)
    int other_pads_mark_size = 0;                           // defined size for position shape (circle)
    int line_thickness = Millimeter2iu( 0.1 );              // defined size for component outlines

    brd_plotter.SetLayerSet( LSET( aLayer ) );
    int cmp_count = 0;
    bool allowUtf8 = true;

    // Plot components data: position, outlines, pad1 and other pads.
    for( FOOTPRINT* footprint : fp_list )
    {
        // Manage the aperture attribute component position:
        GBR_METADATA gbr_metadata;
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_POSITION );

        // Add object attribute: component reference to flash (mainly usefull for users)
        // using quoted UTF8 string
        wxString ref = ConvertNotAllowedCharsInGerber( footprint->Reference().GetShownText(),
                                                       allowUtf8, true );

        gbr_metadata.SetCmpReference( ref );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );

        // Add P&P specific attributes
        GBR_CMP_PNP_METADATA pnpAttrib;

        // Add rotation info (rotation is CCW, in degrees):
        pnpAttrib.m_Orientation = mapRotationAngle( footprint->GetOrientationDegrees() );

        pnpAttrib.m_MountType = GBR_CMP_PNP_METADATA::MOUNT_TYPE_UNSPECIFIED;

        if( footprint->GetAttributes() & FP_THROUGH_HOLE )
            pnpAttrib.m_MountType = GBR_CMP_PNP_METADATA::MOUNT_TYPE_TH;
        else if( footprint->GetAttributes() & FP_SMD )
            pnpAttrib.m_MountType = GBR_CMP_PNP_METADATA::MOUNT_TYPE_SMD;

        // Add component value info:
        pnpAttrib.m_Value = ConvertNotAllowedCharsInGerber( footprint->Value().GetShownText(),
                                                            allowUtf8, true );

        // Add component footprint info:
        wxString fp_info = FROM_UTF8( footprint->GetFPID().GetLibItemName().c_str() );
        pnpAttrib.m_Footprint = ConvertNotAllowedCharsInGerber( fp_info, allowUtf8, true );

        // Add footprint lib name:
        fp_info = FROM_UTF8( footprint->GetFPID().GetLibNickname().c_str() );
        pnpAttrib.m_LibraryName = ConvertNotAllowedCharsInGerber( fp_info, allowUtf8, true );

        gbr_metadata.m_NetlistMetadata.SetExtraData( pnpAttrib.FormatCmpPnPMetadata() );

        wxPoint flash_pos = footprint->GetPosition();

        plotter.FlashPadCircle( flash_pos, flash_position_shape_diam, FILLED, &gbr_metadata );
        gbr_metadata.m_NetlistMetadata.ClearExtraData();

        // Now some extra metadata is output, avoid blindly clearing the full metadata list
        gbr_metadata.m_NetlistMetadata.m_TryKeepPreviousAttributes = true;

        // We plot the footprint courtyard when possible.
        // If not, the pads bounding box will be used.
        bool useFpPadsBbox = true;
        bool onBack = aLayer == B_Cu;

        footprint->BuildPolyCourtyards();

        int checkFlag = onBack ? MALFORMED_B_COURTYARD : MALFORMED_F_COURTYARD;

        if( ( footprint->GetFlags() & checkFlag ) == 0 )
        {
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_COURTYARD );

            const SHAPE_POLY_SET& courtyard = onBack ? footprint->GetPolyCourtyardBack()
                                                     : footprint->GetPolyCourtyardFront();

            for( int ii = 0; ii < courtyard.OutlineCount(); ii++ )
            {
                SHAPE_LINE_CHAIN poly = courtyard.Outline( ii );

                if( !poly.PointCount() )
                    continue;

                useFpPadsBbox = false;
                plotter.PLOTTER::PlotPoly( poly, FILL_TYPE::NO_FILL, line_thickness, &gbr_metadata );
            }
        }

        if( useFpPadsBbox )
        {
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_FOOTPRINT );

            // bbox of fp pads, pos 0, rot 0, non flipped
            EDA_RECT bbox = footprint->GetFpPadsLocalBbox();

            // negate bbox Y values if the fp is flipped (always flipped around X axis
            // in Gerber P&P files).
            int y_sign = aLayer == B_Cu ? -1 : 1;

            SHAPE_LINE_CHAIN poly;
            poly.Append( bbox.GetLeft(), y_sign*bbox.GetTop() );
            poly.Append( bbox.GetLeft(), y_sign*bbox.GetBottom() );
            poly.Append( bbox.GetRight(), y_sign*bbox.GetBottom() );
            poly.Append( bbox.GetRight(), y_sign*bbox.GetTop() );
            poly.SetClosed( true );

            poly.Rotate( -footprint->GetOrientationRadians(), VECTOR2I( 0, 0 ) );
            poly.Move( footprint->GetPosition() );
            plotter.PLOTTER::PlotPoly( poly, FILL_TYPE::NO_FILL, line_thickness, &gbr_metadata );
        }

        std::vector<PAD*>pad_key_list;

        if( m_plotPad1Marker )
        {
            findPads1( pad_key_list, footprint );

            for( PAD* pad1 : pad_key_list )
            {
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_PAD1_POSITION );

                gbr_metadata.SetPadName( pad1->GetName(), allowUtf8, true );

                gbr_metadata.SetPadPinFunction( pad1->GetPinFunction(), allowUtf8, true );

                gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_PAD );

                // Flashes a diamond at pad position:
                plotter.FlashRegularPolygon( pad1->GetPosition(),
                                             pad1_mark_size,
                                             4, 0.0, FILLED, &gbr_metadata );
            }
        }

        if( m_plotOtherPadsMarker )
        {

            gbr_metadata.SetApertureAttrib(
                    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_PADOTHER_POSITION );
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_PAD );

            for( PAD* pad: footprint->Pads() )
            {
                bool skip_pad = false;

                for( PAD* pad1 : pad_key_list )
                {
                    if( pad == pad1 )   // Already plotted
                    {
                        skip_pad = true;
                        break;
                    }
                }

                if( skip_pad )
                    continue;

                // Skip also pads not on the current layer, like pads only
                // on a tech layer
                if( !pad->IsOnLayer( aLayer ) )
                    continue;

                gbr_metadata.SetPadName( pad->GetName(), allowUtf8, true );

                gbr_metadata.SetPadPinFunction( pad->GetPinFunction(), allowUtf8, true );

                // Flashes a round, 0 sized round shape at pad position
                plotter.FlashPadCircle( pad->GetPosition(),
                                        other_pads_mark_size,
                                        FILLED, &gbr_metadata );
            }
        }

        plotter.ClearAllAttributes();    // Unconditionally close all .TO attributes

        cmp_count++;
    }

    // Plot board outlines, if requested
    if( aIncludeBrdEdges )
    {
        brd_plotter.SetLayerSet( LSET( Edge_Cuts ) );

         // Plot edge layer and graphic items
        brd_plotter.PlotBoardGraphicItems();

        // Draw footprint other graphic items:
        for( FOOTPRINT* footprint : fp_list )
        {
            for( BOARD_ITEM* item : footprint->GraphicalItems() )
            {
                if( item->Type() == PCB_FP_SHAPE_T && item->GetLayer() == Edge_Cuts )
                    brd_plotter.PlotFootprintGraphicItem( (FP_SHAPE*) item );
            }
        }
    }


    plotter.EndPlot();

    return cmp_count;
}


double PLACEFILE_GERBER_WRITER::mapRotationAngle( double aAngle )
{
    // convert a kicad footprint orientation to gerber rotation, depending on the layer
    // Currently, same notation as kicad
    return aAngle;
}


void PLACEFILE_GERBER_WRITER::findPads1( std::vector<PAD*>& aPadList, FOOTPRINT* aFootprint ) const
{
    // Fint the pad "1" or pad "A1"
    // this is possible only if only one pad is found
    // Usefull to place a marker in this position

    for( PAD* pad : aFootprint->Pads() )
    {
        if( !pad->IsOnLayer( m_layer ) )
            continue;

        if( pad->GetName() == "1" || pad->GetName() == "A1")
            aPadList.push_back( pad );
    }
}


const wxString PLACEFILE_GERBER_WRITER::GetPlaceFileName( const wxString& aFullBaseFilename,
                                                          PCB_LAYER_ID aLayer ) const
{
    // Gerber files extension is always .gbr.
    // Therefore, to mark pnp files, add "-pnp" to the filename, and a layer id.
    wxFileName  fn = aFullBaseFilename;

    wxString post_id = "-pnp_";
    post_id += aLayer == B_Cu ? "bottom" : "top";
    fn.SetName( fn.GetName() + post_id );
    fn.SetExt( GerberFileExtension );

    return fn.GetFullPath();
}
