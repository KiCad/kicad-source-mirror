/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean_Pierre Charras <jp.charras at wanadoo.fr>
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

/**
 * @file gerber_placefile_writer.cpp
 * @brief Functions to create place files in gerber X2 format.
 */

#include "gerber_placefile_writer.h"

#include <vector>

#include <plotters/plotter_gerber.h>
#include <string_utils.h>
#include <locale_io.h>
#include <macros.h>
#include <pcb_shape.h>
#include <pcbplot.h>
#include <wildcards_and_files_ext.h>
#include <gbr_metadata.h>
#include <footprint.h>
#include <pad.h>


PLACEFILE_GERBER_WRITER::PLACEFILE_GERBER_WRITER( BOARD* aPcb )
{
    m_pcb                 = aPcb;
    m_plotPad1Marker      = true; // Place a marker to pin 1 (or A1) position
    m_plotOtherPadsMarker = true; // Place a marker to other pins position
    m_layer               = PCB_LAYER_ID::UNDEFINED_LAYER; // No layer set
}


int PLACEFILE_GERBER_WRITER::CreatePlaceFile( const wxString& aFullFilename, PCB_LAYER_ID aLayer,
                                              bool aIncludeBrdEdges, bool aExcludeDNP,
                                              bool aExcludeBOM )
{
    m_layer = aLayer;

    PCB_PLOT_PARAMS plotOpts = m_pcb->GetPlotOptions();

    if( plotOpts.GetUseAuxOrigin() )
        m_offset = m_pcb->GetDesignSettings().GetAuxOrigin();

    // Collect footprints on the right layer
    std::vector<FOOTPRINT*> fp_list;

    for( FOOTPRINT* footprint : m_pcb->Footprints() )
    {
        if( footprint->GetExcludedFromPosFilesForVariant( m_variant ) )
             continue;

        if( aExcludeDNP && footprint->GetDNPForVariant( m_variant ) )
            continue;

        if( aExcludeBOM && footprint->GetExcludedFromBOMForVariant( m_variant ) )
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
    plotter.SetViewport( m_offset, pcbIUScale.IU_PER_MILS/10, /* scale */ 1.0, /* mirror */false );

    // has meaning only for gerber plotter. Must be called only after SetViewport
    plotter.SetGerberCoordinatesFormat( 6 );
    plotter.SetCreator( wxT( "PCBNEW" ) );

    // Add the standard X2 FileFunction for P&P files
    // %TF.FileFunction,Component,Ln,[top][bottom]*%
    wxString text;
    text.Printf( wxT( "%%TF.FileFunction,Component,L%d,%s*%%" ),
                 aLayer == B_Cu ? m_pcb->GetCopperLayerCount() : 1,
                 aLayer == B_Cu ? wxT( "Bot" ) : wxT( "Top" ) );
    plotter.AddLineToHeader( text );

    // Add file polarity (positive)
    text = wxT( "%TF.FilePolarity,Positive*%" );
    plotter.AddLineToHeader( text );

    if( !plotter.OpenFile( aFullFilename ) )
        return -1;

    // We need a BRDITEMS_PLOTTER to plot pads
    BRDITEMS_PLOTTER brd_plotter( &plotter, m_pcb, plotOpts );

    plotter.StartPlot( wxT( "1" ) );

    // Some tools in P&P files have the type and size defined.
    // they are position flash (round), pad1 flash (diamond), other pads flash (round)
    // and component outline thickness (polyline)

    // defined size for footprint position shape (circle)
    int flash_position_shape_diam = pcbIUScale.mmToIU( 0.3 );

    // defined size for pad 1 position (diamond)
    int pad1_mark_size = pcbIUScale.mmToIU( 0.36 );

    // Normalized size for other pads (circle)
    // It was initially the size 0, but was changed later to 0.1 mm in rev 2023-08
    // See ComponentPin aperture attribute (see 5.6.10 .AperFunction value)
    int other_pads_mark_size = pcbIUScale.mmToIU( 0.1 );

    // defined size for component outlines
    int line_thickness = pcbIUScale.mmToIU( 0.1 );

    brd_plotter.SetLayerSet( LSET( { aLayer } ) );
    int cmp_count = 0;
    const bool allowUtf8 = true;
    const bool quoteOption = false;

    // Plot components data: position, outlines, pad1 and other pads.
    for( FOOTPRINT* footprint : fp_list )
    {
        // Manage the aperture attribute component position:
        GBR_METADATA metadata;
        metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_POSITION );

        // Add object attribute: component reference to flash (mainly useful for users)
        // using not quoted UTF8 string
        wxString ref = ConvertNotAllowedCharsInGerber( footprint->Reference().GetShownText( false ),
                                                       allowUtf8, quoteOption );

        metadata.SetCmpReference( ref );
        metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );

        // Add P&P specific attributes
        GBR_CMP_PNP_METADATA pnpAttrib;

        // Add rotation info (rotation is CCW, in degrees):
        pnpAttrib.m_Orientation = mapRotationAngle( footprint->GetOrientationDegrees(),
                                                    aLayer == B_Cu ? true : false );

        pnpAttrib.m_MountType = GBR_CMP_PNP_METADATA::MOUNT_TYPE_UNSPECIFIED;

        if( footprint->GetAttributes() & FP_THROUGH_HOLE )
            pnpAttrib.m_MountType = GBR_CMP_PNP_METADATA::MOUNT_TYPE_TH;
        else if( footprint->GetAttributes() & FP_SMD )
            pnpAttrib.m_MountType = GBR_CMP_PNP_METADATA::MOUNT_TYPE_SMD;

        // Add component value info:
        pnpAttrib.m_Value = ConvertNotAllowedCharsInGerber( footprint->Value().GetShownText( false ),
                                                            allowUtf8, quoteOption );

        // Add component footprint info:
        wxString fp_info = From_UTF8( footprint->GetFPID().GetLibItemName().c_str() );
        pnpAttrib.m_Footprint = ConvertNotAllowedCharsInGerber( fp_info, allowUtf8, quoteOption );

        // Add footprint lib name:
        fp_info = From_UTF8( footprint->GetFPID().GetLibNickname().c_str() );
        pnpAttrib.m_LibraryName = ConvertNotAllowedCharsInGerber( fp_info, allowUtf8, quoteOption );

        metadata.m_NetlistMetadata.SetExtraData( pnpAttrib.FormatCmpPnPMetadata() );

        VECTOR2I flash_pos = footprint->GetPosition();

        plotter.FlashPadCircle( flash_pos, flash_position_shape_diam, &metadata );
        metadata.m_NetlistMetadata.ClearExtraData();

        // Now some extra metadata is output, avoid blindly clearing the full metadata list
        metadata.m_NetlistMetadata.m_TryKeepPreviousAttributes = true;

        // We plot the footprint courtyard when possible.
        // If not, the pads bounding box will be used.
        bool useFpPadsBbox = true;
        bool onBack = aLayer == B_Cu;

        footprint->BuildCourtyardCaches();

        int checkFlag = onBack ? MALFORMED_B_COURTYARD : MALFORMED_F_COURTYARD;

        if( ( footprint->GetFlags() & checkFlag ) == 0 )
        {
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_COURTYARD );

            const SHAPE_POLY_SET& courtyard = footprint->GetCourtyard( aLayer );

            for( int ii = 0; ii < courtyard.OutlineCount(); ii++ )
            {
                SHAPE_LINE_CHAIN poly = courtyard.Outline( ii );

                if( !poly.PointCount() )
                    continue;

                useFpPadsBbox = false;
                plotter.PLOTTER::PlotPoly( poly, FILL_T::NO_FILL, line_thickness, &metadata );
            }
        }

        if( useFpPadsBbox )
        {
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_FOOTPRINT );

            // bbox of fp pads, pos 0, rot 0, non flipped
            BOX2I bbox = footprint->GetFpPadsLocalBbox();

            // negate bbox Y values if the fp is flipped (always flipped around X axis
            // in Gerber P&P files).
            int y_sign = aLayer == B_Cu ? -1 : 1;

            SHAPE_LINE_CHAIN poly;
            poly.Append( bbox.GetLeft(), y_sign*bbox.GetTop() );
            poly.Append( bbox.GetLeft(), y_sign*bbox.GetBottom() );
            poly.Append( bbox.GetRight(), y_sign*bbox.GetBottom() );
            poly.Append( bbox.GetRight(), y_sign*bbox.GetTop() );
            poly.SetClosed( true );

            poly.Rotate( footprint->GetOrientation() );
            poly.Move( footprint->GetPosition() );
            plotter.PLOTTER::PlotPoly( poly, FILL_T::NO_FILL, line_thickness, &metadata );
        }

        std::vector<PAD*>pad_key_list;

        if( m_plotPad1Marker )
        {
            findPads1( pad_key_list, footprint );

            for( PAD* pad1 : pad_key_list )
            {
                metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_PAD1_POS );
                metadata.SetPadName( pad1->GetNumber(), allowUtf8, quoteOption );
                metadata.SetPadPinFunction( pad1->GetPinFunction(), allowUtf8, quoteOption );
                metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_PAD );

                // Flashes a diamond at pad position:
                plotter.FlashRegularPolygon( pad1->GetPosition(), pad1_mark_size, 4, ANGLE_0,
                                             &metadata );
            }
        }

        if( m_plotOtherPadsMarker )
        {
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_PADOTHER_POS );
            metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_PAD );

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

                metadata.SetPadName( pad->GetNumber(), allowUtf8, quoteOption );
                metadata.SetPadPinFunction( pad->GetPinFunction(), allowUtf8, quoteOption );

                // Flashes a round, 0 sized round shape at pad position
                plotter.FlashPadCircle( pad->GetPosition(), other_pads_mark_size, &metadata );
            }
        }

        plotter.ClearAllAttributes();    // Unconditionally close all .TO attributes

        cmp_count++;
    }

    // Plot board outlines, if requested
    if( aIncludeBrdEdges )
    {
        brd_plotter.SetLayerSet( LSET( { Edge_Cuts } ) );

        // Plot edge layer and graphic items
        for( const BOARD_ITEM* item : m_pcb->Drawings() )
            brd_plotter.PlotBoardGraphicItem( item );

        // Draw footprint other graphic items:
        for( FOOTPRINT* footprint : fp_list )
        {
            for( BOARD_ITEM* item : footprint->GraphicalItems() )
            {
                if( item->Type() == PCB_SHAPE_T && item->GetLayer() == Edge_Cuts )
                    brd_plotter.PlotShape( static_cast<PCB_SHAPE*>( item ) );
            }
        }
    }

    plotter.EndPlot();

    return cmp_count;
}


double PLACEFILE_GERBER_WRITER::mapRotationAngle( double aAngle, bool aIsFlipped )
{
    // Convert a KiCad footprint orientation to gerber rotation, depending on the layer
    // Gerber rotation is:
    // rot angle > 0 for rot CW, seen from Top side
    // same a Pcbnew for Top side
    // (angle + 180) for Bottom layer i.e flipped around Y axis: X axis coordinates mirrored.
    // because Pcbnew flip around the X axis : Y coord mirrored, that is similar to mirror
    // around Y axis + 180 deg rotation
    if( aIsFlipped )
    {
        double gbr_angle = 180.0 + aAngle;

        // Normalize between -180 ... + 180 deg
        // Not mandatory, but the angle is more easy to read
        if( gbr_angle <= -180 )
            gbr_angle += 360.0;
        else if( gbr_angle > 180 )
            gbr_angle -= 360.0;

        return gbr_angle;
    }

    return aAngle;
}


void PLACEFILE_GERBER_WRITER::findPads1( std::vector<PAD*>& aPadList, FOOTPRINT* aFootprint ) const
{
    // Fint the pad "1" or pad "A1"
    // this is possible only if only one pad is found
    // useful to place a marker in this position

    for( PAD* pad : aFootprint->Pads() )
    {
        if( !pad->IsOnLayer( m_layer ) )
            continue;

        if( pad->GetNumber() == wxT( "1" )  || pad->GetNumber() == wxT( "A1" ) )
            aPadList.push_back( pad );
    }
}


const wxString PLACEFILE_GERBER_WRITER::GetPlaceFileName( const wxString& aFullBaseFilename,
                                                          PCB_LAYER_ID aLayer ) const
{
    // Gerber files extension is always .gbr.
    // Therefore, to mark pnp files, add "-pnp" to the filename, and a layer id.
    wxFileName  fn = aFullBaseFilename;

    wxString post_id = wxT( "-pnp_" );
    post_id += aLayer == B_Cu ? wxT( "bottom" ) : wxT( "top" );
    fn.SetName( fn.GetName() + post_id );
    fn.SetExt( FILEEXT::GerberFileExtension );

    return fn.GetFullPath();
}
