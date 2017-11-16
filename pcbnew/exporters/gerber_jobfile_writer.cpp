/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file gendrill_gerber_writer.cpp
 * @brief Functions to create drill files in gerber X2 format.
 */

#include <fctsys.h>

#include <vector>

#include <class_plotter.h>
#include <wxPcbStruct.h>
#include <build_version.h>

#include <class_board.h>
#include <class_zone.h>
#include <class_module.h>

#include <pcbplot.h>
#include <pcbnew.h>
#include <gerber_jobfile_writer.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <plot_auxiliary_data.h>


GERBER_JOBFILE_WRITER::GERBER_JOBFILE_WRITER( BOARD* aPcb, REPORTER* aReporter )
{
    m_pcb = aPcb;
    m_reporter = aReporter;
    m_conversionUnits = 1.0 / IU_PER_MM;    // Gerber units = mm
}

enum ONSIDE GERBER_JOBFILE_WRITER::hasSilkLayers()
{
    int flag = SIDE_NONE;

    for( unsigned ii = 0; ii < m_params.m_LayerId.size(); ii++ )
    {
        if( m_params.m_LayerId[ii] == B_SilkS )
            flag |= SIDE_BOTTOM;

        if( m_params.m_LayerId[ii] == F_SilkS )
            flag |= SIDE_TOP;
    }

    return (enum ONSIDE)flag;
}


enum ONSIDE GERBER_JOBFILE_WRITER::hasSolderMasks()
{
    int flag = SIDE_NONE;

    for( unsigned ii = 0; ii < m_params.m_LayerId.size(); ii++ )
    {
        if( m_params.m_LayerId[ii] == B_Mask )
            flag |= SIDE_BOTTOM;

        if( m_params.m_LayerId[ii] == F_Mask )
            flag |= SIDE_TOP;
    }

    return (enum ONSIDE)flag;
}

const char* GERBER_JOBFILE_WRITER::sideKeyValue( enum ONSIDE aValue )
{
    // return the key associated to sides used for some layers
    // "No, TopOnly, BotOnly or Both"
    const char* value = nullptr;

    switch( aValue )
    {
        case SIDE_NONE:
            value = "No"; break;

        case SIDE_TOP:
            value = "TopOnly"; break;

        case SIDE_BOTTOM:
            value = "BotOnly"; break;

        case SIDE_BOTH:
            value = "Both"; break;

    }

    return value;
}


extern void BuildGerberX2Header( const BOARD *aBoard, wxArrayString& aHeader );


bool GERBER_JOBFILE_WRITER::CreateJobFile( const wxString& aFullFilename )
{
    // Note: in Gerber job file, dimensions are in mm, and are floating numbers
    FILE* jobFile = wxFopen( aFullFilename, "wt" );

    wxString msg;

    if( jobFile == nullptr )
    {
        if( m_reporter )
        {
            msg.Printf( _( "Unable to create job file '%s'" ), aFullFilename );
            m_reporter->Report( msg, REPORTER::RPT_ERROR );
        }
        return false;
    }

    LOCALE_IO dummy;

    // output the job file header
    bool hasInnerLayers = m_pcb->GetCopperLayerCount() > 2;
    wxArrayString header;

    fputs( "G04 Gerber job file with board parameters*\n"
           "%TF.FileFunction,JobInfo*%\n"
           "%TF.Part,SinglePCB*%\n", jobFile );
    fputs( "G04 Single PCB fabrication instructions*\n", jobFile );

    BuildGerberX2Header( m_pcb, header );

    for( unsigned ii = 0; ii < header.GetCount(); ii++ )
    {
        if( header[ii].Contains( "TF.SameCoordinates" ) )
            continue;   // This attribute is not useful in job file, skip it

        fputs( TO_UTF8( header[ii] ), jobFile );
        fputs( "\n", jobFile );
    }


    fputs( "%MOMM*%\n", jobFile );

    fputs( "G04 Overall board parameters*\n", jobFile );
    // output the bord size in mm:
    EDA_RECT brect = m_pcb->GetBoardEdgesBoundingBox();
    fprintf( jobFile, "%%TJ.B_Size_X,%.3f*%%\n", brect.GetWidth()*m_conversionUnits );
    fprintf( jobFile, "%%TJ.B_Size_Y,%.3f*%%\n", brect.GetHeight()*m_conversionUnits );

    // number of copper layers
    fprintf( jobFile, "%%TJ.B_LayerNum,%d*%%\n", m_pcb->GetCopperLayerCount() );

    // Board thickness
    fprintf( jobFile, "%%TJ.B_Overall_Thickness,%.3f*%%\n",
             m_pcb->GetDesignSettings().GetBoardThickness()*m_conversionUnits );

    fprintf( jobFile, "%%TJ.B_Legend_Present,%s*%%\n", sideKeyValue( hasSilkLayers() ) );

    fprintf( jobFile, "%%TJ.B_SolderMask_Present,%s*%%\n", sideKeyValue( hasSolderMasks() ) );

    // Job file support a few design rules:
    fputs( "G04 board design rules*\n", jobFile );
    const BOARD_DESIGN_SETTINGS& dsnSettings = m_pcb->GetDesignSettings();
    NETCLASS defaultNC = *dsnSettings.GetDefault();
    int minclearanceOuter = defaultNC.GetClearance();

    // Search a smaller clearance in other net classes, if any.
    for( NETCLASSES::const_iterator it = dsnSettings.m_NetClasses.begin();
         it != dsnSettings.m_NetClasses.end();
         ++it )
    {
        NETCLASS netclass = *it->second;
        minclearanceOuter = std::min( minclearanceOuter, netclass.GetClearance() );
    }

    // job file knows different clearance types.
    // Kicad knows only one clearance for pads and tracks
    // However, pads can have a specific clearance defined for a pad or a footprint,
    // and min clearance can be dependent on layers.
    // Search for a minimal pad clearance:
    int minPadClearanceOuter = defaultNC.GetClearance();
    int minPadClearanceInner = defaultNC.GetClearance();

    for( MODULE* module : m_pcb->Modules() )
    {
        for( auto& pad : module->Pads() )
        {
            if( ( pad->GetLayerSet() & LSET::InternalCuMask() ).any() )
               minPadClearanceInner = std::min( minPadClearanceInner, pad->GetClearance() );

            if( ( pad->GetLayerSet() & LSET::ExternalCuMask() ).any() )
               minPadClearanceOuter = std::min( minPadClearanceOuter, pad->GetClearance() );
        }
    }


    fprintf( jobFile, "%%TJ.D_PadToPad_Out,%.3f*%%\n", minPadClearanceOuter*m_conversionUnits );

    if( hasInnerLayers )
        fprintf( jobFile, "%%TJ.D_PadToPad_Inr,%.3f*%%\n", minPadClearanceInner*m_conversionUnits );

    fprintf( jobFile, "%%TJ.D_PadToTrack_Out,%.3f*%%\n", minPadClearanceOuter*m_conversionUnits );

    if( hasInnerLayers )
        fprintf( jobFile, "%%TJ.D_PadToTrack_Inr,%.3f*%%\n", minPadClearanceInner*m_conversionUnits );

    // Until this is changed in Kicad, use the same value for internal tracks
    int minclearanceInner = minclearanceOuter;

    fprintf( jobFile, "%%TJ.D_TrackToTrack_Out,%.3f*%%\n", minclearanceOuter*m_conversionUnits );

    if( hasInnerLayers )
        fprintf( jobFile, "%%TJ.D_TrackToTrack_Inr,%.3f*%%\n", minclearanceInner*m_conversionUnits );

    // Output the minimal track width
    int mintrackWidthOuter = INT_MAX;
    int mintrackWidthInner = INT_MAX;

    for( TRACK* track : m_pcb->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            continue;

        if( track->GetLayer() == B_Cu || track->GetLayer() == F_Cu )
            mintrackWidthOuter = std::min( mintrackWidthOuter, track->GetWidth() );
        else
            mintrackWidthInner = std::min( mintrackWidthInner, track->GetWidth() );
    }

    if( mintrackWidthOuter != INT_MAX )
        fprintf( jobFile, "%%TJ.D_MinLineWidth_Out,%.3f*%%\n", mintrackWidthOuter*m_conversionUnits );

    if( mintrackWidthInner != INT_MAX )
        fprintf( jobFile, "%%TJ.D_MinLineWidth_Inr,%.3f*%%\n", mintrackWidthInner*m_conversionUnits );

    // Output the minimal zone to xx clearance
    // Note: zones can have a zone clearance set to 0
    // if happens, the actual zone clearance is the clearance of its class
    minclearanceOuter = INT_MAX;
    minclearanceInner = INT_MAX;

    for( int ii = 0; ii < m_pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = m_pcb->GetArea( ii );

        if( zone->GetIsKeepout() || !zone->IsOnCopperLayer() )
            continue;

        int zclerance = zone->GetClearance();

        if( zone->GetLayer() == B_Cu || zone->GetLayer() == F_Cu )
            minclearanceOuter = std::min( minclearanceOuter, zclerance );
        else
            minclearanceInner = std::min( minclearanceInner, zclerance );
    }

    if( minclearanceOuter != INT_MAX )
        fprintf( jobFile, "%%TJ.D_TrackToRegion_Out,%.3f*%%\n", minclearanceOuter*m_conversionUnits );

    if( hasInnerLayers && minclearanceInner != INT_MAX )
        fprintf( jobFile, "%%TJ.D_TrackToRegion_Inr,%.3f*%%\n", minclearanceInner*m_conversionUnits );

    if( minclearanceOuter != INT_MAX )
        fprintf( jobFile, "%%TJ.D_RegionToRegion_Out,%.3f*%%\n", minclearanceOuter*m_conversionUnits );

    if( hasInnerLayers && minclearanceInner != INT_MAX )
        fprintf( jobFile, "%%TJ.D_RegionToRegion_Inr,%.3f*%%\n", minclearanceInner*m_conversionUnits );

    // output the gerber file list:
    fputs( "G04 Layer Structure*\n", jobFile );

    for( unsigned ii = 0; ii < m_params.m_GerberFileList.GetCount(); ii ++ )
    {
        wxString& name = m_params.m_GerberFileList[ii];
        PCB_LAYER_ID layer = m_params.m_LayerId[ii];
        wxString gbr_layer_id;
        bool skip_file = false;     // true to skip files which should not be in job file
        const char* polarity = "Positive";

        if( layer <= B_Cu )
        {
            gbr_layer_id = "Copper,L";

            if( layer == B_Cu )
                gbr_layer_id << m_pcb->GetCopperLayerCount();
            else
                gbr_layer_id << layer+1;

            gbr_layer_id << ",";

            if( layer == B_Cu )
                gbr_layer_id << "Bot";
            else if( layer == F_Cu )
                gbr_layer_id << "Top";
            else
                gbr_layer_id << "Inr";
        }

        else
        {
            switch( layer )
            {
                case B_Adhes:
                    gbr_layer_id = "Glue,Bot"; break;
                case F_Adhes:
                    gbr_layer_id = "Glue,Top"; break;

                case B_Paste:
                    gbr_layer_id = "SolderPaste,Bot"; break;
                case F_Paste:
                    gbr_layer_id = "SolderPaste,Top"; break;

                case B_SilkS:
                    gbr_layer_id = "Legend,Bot"; break;
                case F_SilkS:
                    gbr_layer_id = "Legend,Top"; break;

                case B_Mask:
                    gbr_layer_id = "SolderMask,Bot"; polarity = "Negative"; break;
                case F_Mask:
                    gbr_layer_id = "SolderMask,Top"; polarity = "Negative"; break;

                case Edge_Cuts:
                    gbr_layer_id = "Profile"; break;

                case B_Fab:
                    gbr_layer_id = "AssemblyDrawing,Bot"; break;
                case F_Fab:
                    gbr_layer_id = "AssemblyDrawing,Top"; break;

                case Dwgs_User:
                case Cmts_User:
                case Eco1_User:
                case Eco2_User:
                case Margin:
                case B_CrtYd:
                case F_CrtYd:
                   skip_file = true; break;

                default:
                    skip_file = true;
                    m_reporter->Report( "Unexpected layer id in job file",
                                        REPORTER::RPT_ERROR );
                    break;
            }
        }

        if( !skip_file )
        {
            // name can contain non ASCII7 chars.
            // Only ASCII7 chars are accepted in gerber files. others must be converted to
            // a gerber hexa sequence.
            std::string strname = formatStringToGerber( name );
            fprintf( jobFile, "%%TJ.L_\"%s\",%s,%s*%%\n", TO_UTF8( gbr_layer_id ),
                     polarity, strname.c_str() );
        }
    }

    // Close job file
    fputs( "M02*\n", jobFile );

    fclose( jobFile );

    if( m_reporter )
    {
        msg.Printf( _( "Create Gerber job file '%s'" ), aFullFilename );
        m_reporter->Report( msg, REPORTER::RPT_ACTION );
    }

    return true;
}
