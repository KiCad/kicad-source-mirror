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

#include <fctsys.h>
#include "gerber_placefile_writer.h"

#include <vector>

#include <plotter.h>
#include <kicad_string.h>
#include <pcb_edit_frame.h>
#include <pgm_base.h>
#include <build_version.h>

#include <class_board.h>

#include <pcbplot.h>
#include <pcbnew.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <gbr_metadata.h>
#include <class_module.h>
#include <pcbplot.h>


PLACEFILE_GERBER_WRITER::PLACEFILE_GERBER_WRITER( BOARD* aPcb )
{
    m_pcb = aPcb;
    /* Set conversion scale depending on drill file units */
    m_conversionUnits = 1.0 / IU_PER_MM;    // Gerber units = mm
    m_forceSmdItems = false;
    m_plotPad1Marker = true;                // Place a marker to pin 1 (or A1) position
    m_plotOtherPadsMarker = true;           // Place a marker to other pins position
}


int PLACEFILE_GERBER_WRITER::CreatePlaceFile( wxString& aFullFilename,
                                              PCB_LAYER_ID aLayer, bool aIncludeBrdEdges )
{
    m_layer = aLayer;

    // Collect footprints on the right layer
    std::vector<MODULE*> fp_list;

    for( MODULE* footprint : m_pcb->Modules() )
    {
        if( footprint->GetAttributes() & MOD_VIRTUAL )
             continue;

        if( footprint->GetLayer() == aLayer )
           fp_list.push_back( footprint );
    }

    LOCALE_IO dummy_io;     // Use the standard notation for double numbers

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
    PCB_PLOT_PARAMS plotOpts;
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
    for( MODULE* footprint : fp_list )
    {
        // Manage the aperture attribute component position:
        GBR_METADATA gbr_metadata;
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_POSITION );

        // Add object attribute: component reference to flash (mainly usefull for users)
        // using quoted UTF8 string
        wxString ref = ConvertNotAllowedCharsInGerber( footprint->GetReference(),
                                                       allowUtf8, true );

        gbr_metadata.SetCmpReference( ref );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );

        // Add P&P specific attributes
        GBR_CMP_PNP_METADATA pnpAttrib;

        // Add rotation info (rotation is CCW, in degrees):
        pnpAttrib.m_Orientation = mapRotationAngle( footprint->GetOrientationDegrees() );

        // Add component type info (SMD or Through Hole):
        bool is_smd_mount = footprint->GetAttributes() & MOD_CMS;

        // Smd footprints can have through holes (thermal vias).
        // but if a footprint is not set as SMD, it will be set as SMD
        // if it does not have through hole pads
        if( !is_smd_mount && !footprint->HasNonSMDPins() )
            is_smd_mount = true;

        pnpAttrib.m_MountType = is_smd_mount ? GBR_CMP_PNP_METADATA::MOUNT_TYPE_SMD
                                : GBR_CMP_PNP_METADATA::MOUNT_TYPE_TH;

        // Add component value info:
        pnpAttrib.m_Value = ConvertNotAllowedCharsInGerber( footprint->GetValue(), allowUtf8, true );

        // Add component footprint info:
        wxString fp_info = FROM_UTF8( footprint->GetFPID().GetLibItemName().c_str() );
        pnpAttrib.m_Footprint = ConvertNotAllowedCharsInGerber( fp_info, allowUtf8, true );

        // Add footprint lib name:
        fp_info = FROM_UTF8( footprint->GetFPID().GetLibNickname().c_str() );
        pnpAttrib.m_LibraryName = ConvertNotAllowedCharsInGerber( fp_info, allowUtf8, true );

        gbr_metadata.m_NetlistMetadata.SetExtraData( pnpAttrib.FormatCmpPnPMetadata() );

        wxPoint flash_pos = footprint->GetPosition() + m_offset;

        plotter.FlashPadCircle( flash_pos, flash_position_shape_diam, FILLED, &gbr_metadata );
        gbr_metadata.m_NetlistMetadata.ClearExtraData();

        // Now some extra metadata is output, avoid blindly clearing the full metadata list
        gbr_metadata.m_NetlistMetadata.m_TryKeepPreviousAttributes = true;

        // We plot the footprint courtyard when possible.
        // If not, the pads bounding box will be used.
        bool useFpPadsBbox = true;

        if( footprint->BuildPolyCourtyard() )
        {
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_COURTYARD );

            SHAPE_POLY_SET& courtyard = aLayer == B_Cu ?
                                                footprint->GetPolyCourtyardBack():
                                                footprint->GetPolyCourtyardFront();

            for( int ii = 0; ii < courtyard.OutlineCount(); ii++ )
            {
                SHAPE_LINE_CHAIN poly = courtyard.Outline( ii );

                if( !poly.PointCount() )
                    continue;

                poly.Move( m_offset );
                useFpPadsBbox = false;
                plotter.PLOTTER::PlotPoly( poly, NO_FILL, line_thickness, &gbr_metadata );
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
            poly.Move( footprint->GetPosition() + m_offset );
            plotter.PLOTTER::PlotPoly( poly, NO_FILL, line_thickness, &gbr_metadata );
        }

        std::vector<D_PAD*>pad_key_list;

        if( m_plotPad1Marker )
        {
            findPads1( pad_key_list, footprint );

            for( D_PAD* pad1 : pad_key_list )
            {
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_PAD1_POSITION );

                gbr_metadata.SetPadName( pad1->GetName() );
                gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_PAD );

                // Flashes a diamond at pad position:
                plotter.FlashRegularPolygon( pad1->GetPosition() + m_offset, pad1_mark_size,
                                             4, 0.0, FILLED, &gbr_metadata );
            }
        }

        if( m_plotOtherPadsMarker )
        {

            gbr_metadata.SetApertureAttrib(
                    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_PADOTHER_POSITION );
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_PAD );

            for( D_PAD* pad: footprint->Pads() )
            {
                bool skip_pad = false;

                for( D_PAD* pad1 : pad_key_list )
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

                gbr_metadata.SetPadName( pad->GetName() );

                // Flashes a round, 0 sized round shape at pad position
                plotter.FlashPadCircle( pad->GetPosition() + m_offset, other_pads_mark_size,
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
        for( MODULE* footprint : fp_list )
        {
            for( auto item : footprint->GraphicalItems() )
            {
                if( item->Type() == PCB_MODULE_EDGE_T && item->GetLayer() == Edge_Cuts )
                    brd_plotter.Plot_1_EdgeModule( (EDGE_MODULE*) item );
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


void PLACEFILE_GERBER_WRITER::findPads1( std::vector<D_PAD*>& aPadList, MODULE* aFootprint ) const
{
    // Fint the pad "1" or pad "A1"
    // this is possible only if only one pad is found
    // Usefull to place a marker in this position

    for( D_PAD* pad : aFootprint->Pads() )
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
