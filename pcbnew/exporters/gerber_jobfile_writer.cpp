/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Functions to create the Gerber job file in JSON format.
 */

#include <fctsys.h>

#include <vector>

#include <plotter.h>
#include <pcb_edit_frame.h>
#include <build_version.h>

#include <class_board.h>
#include <class_zone.h>
#include <class_module.h>

#include <pcbplot.h>
#include <pcbnew.h>
#include <gerber_jobfile_writer.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <gbr_metadata.h>


GERBER_JOBFILE_WRITER::GERBER_JOBFILE_WRITER( BOARD* aPcb, REPORTER* aReporter )
{
    m_pcb = aPcb;
    m_reporter = aReporter;
    m_conversionUnits = 1.0 / IU_PER_MM;    // Gerber units = mm
    m_indent = 0;
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


bool GERBER_JOBFILE_WRITER::CreateJobFile( const wxString& aFullFilename )
{
    bool success;
    wxString msg;

    success = WriteJSONJobFile( aFullFilename );

    if( !success )
    {
        if( m_reporter )
        {
            msg.Printf( _( "Unable to create job file \"%s\"" ), aFullFilename );
            m_reporter->Report( msg, REPORTER::RPT_ERROR );
        }
    }
    else if( m_reporter )
    {
        msg.Printf( _( "Create Gerber job file \"%s\"" ), aFullFilename );
        m_reporter->Report( msg, REPORTER::RPT_ACTION );
    }

    return success;
}


void GERBER_JOBFILE_WRITER::addJSONHeader()
{
    wxString text;
    openBlock();
    addJSONObject( "\"Header\":\n" );
    openBlock();

    // Creates the GenerationSoftware
    addJSONObject( "\"GenerationSoftware\":\n" );
    openBlock();
    addJSONObject( "\"Vendor\":      \"KiCad\",\n" );
    addJSONObject( "\"Application\": \"Pcbnew\",\n" );
    text.Printf( "\"Version\":     \"%s\"\n", GetBuildVersion() );
    addJSONObject( text );
    closeBlockWithSep();

    // creates the CreationDate attribute:
    // The attribute value must conform to the full version of the ISO 8601
    // date and time format, including time and time zone.
    text = GbrMakeCreationDateAttributeString( GBR_NC_STRING_FORMAT_GBRJOB ) + "\n";
    addJSONObject( text );

    closeBlockWithSep();
}


void GERBER_JOBFILE_WRITER::removeJSONSepararator()
{
    if( m_JSONbuffer.Last() == ',' )
    {
        m_JSONbuffer.RemoveLast();
        return;
    }

    if( m_JSONbuffer.Last() == '\n' )
    {
        m_JSONbuffer.RemoveLast();

        if( m_JSONbuffer.Last() == ',' )
            m_JSONbuffer.RemoveLast();

        m_JSONbuffer.Append( '\n' );
    }
}

bool GERBER_JOBFILE_WRITER::WriteJSONJobFile( const wxString& aFullFilename )
{
    // Note: in Gerber job file, dimensions are in mm, and are floating numbers
    FILE* jobFile = wxFopen( aFullFilename, "wt" );

    m_JSONbuffer.Empty();
    m_indent = 0;

    if( jobFile == nullptr )
        return false;

    LOCALE_IO dummy;

    // output the job file header
    addJSONHeader();

    // Add the General Specs
    addJSONGeneralSpecs();

    // Job file support a few design rules:
    addJSONDesignRules();

    // output the gerber file list:
    addJSONFilesAttributes();

    // output the board stackup:
    addJSONMaterialStackup();

    // Close job file full block data
    removeJSONSepararator();    // remove the last separator
    closeBlock();

    fputs( TO_UTF8( m_JSONbuffer ), jobFile );

    fclose( jobFile );

    return true;
}


void GERBER_JOBFILE_WRITER::addJSONGeneralSpecs()
{
    addJSONObject( "\"GeneralSpecs\":\n" );
    openBlock();

    addJSONObject( "\"ProjectId\":\n" );
    openBlock();

    // Creates the ProjectId. Format is (from Gerber file format doc):
    // ProjectId,<project id>,<project GUID>,<revision id>*%
    // <project id> is the name of the project, restricted to basic ASCII symbols only,
    // and comma not accepted
    // All illegal chars will be replaced by underscore
    // Rem: <project id> accepts only ASCII 7 code (only basic ASCII codes are allowed in gerber files).
    //
    // <project GUID> is a string which is an unique id of a project.
    // However Kicad does not handle such a project GUID, so it is built from the board name
    wxFileName fn = m_pcb->GetFileName();
    wxString msg = fn.GetFullName();

    // Build a <project GUID>, from the board name
    wxString guid = GbrMakeProjectGUIDfromString( msg );

    // build the <project id> string: this is the board short filename (without ext)
    // and all non ASCII chars are replaced by '_'
    msg = fn.GetName();

    // build the <rec> string. All non ASCII chars and comma are replaced by '_'
    wxString rev = m_pcb->GetTitleBlock().GetRevision();

    if( rev.IsEmpty() )
        rev = wxT( "rev?" );

    addJSONObject( wxString::Format( "\"Name\": \"%s\",\n", msg.ToAscii() ) );
    addJSONObject( wxString::Format( "\"GUID\": \"%s\",\n", guid ) );
    addJSONObject( wxString::Format( "\"Revision\": \"%s\"\n", rev.ToAscii() ) );

    closeBlockWithSep();

    // output the bord size in mm:
    EDA_RECT brect = m_pcb->GetBoardEdgesBoundingBox();
    addJSONObject( "\"Size\":\n" );
    openBlock();

    addJSONObject( wxString::Format( "\"X\": %.3f,\n", brect.GetWidth()*m_conversionUnits ) );
    addJSONObject( wxString::Format( "\"Y\": %.3f\n", brect.GetHeight()*m_conversionUnits ) );
    closeBlockWithSep();

    // Add some data to the JSON header, GeneralSpecs:
    // number of copper layers
    addJSONObject( wxString::Format( "\"LayerNumber\": %d,\n", m_pcb->GetCopperLayerCount() ) );

    // Board thickness
    addJSONObject( wxString::Format( "\"BoardThickness\":  %.3f,\n",
             m_pcb->GetDesignSettings().GetBoardThickness()*m_conversionUnits ) );

#if 0    // Not yet in use
    /* The board type according to IPC-2221. There are six primary board types:
    - Type 1 - Single-sided
    - Type 2 - Double-sided
    - Type 3 - Multilayer, TH components only
    - Type 4 - Multilayer, with TH, blind and/or buried vias.
    - Type 5 - Multilayer metal-core board, TH components only
    - Type 6 - Multilayer metal-core
    */
    addJSONObject( wxString::Format( "\"IPC-2221-Type\": \"%d\",\n", 4 ) );

    /* Via protection: key words:
    Ia Tented - Single-sided
    Ib Tented - Double-sided
    IIa Tented and Covered - Single-sided
    IIb Tented and Covered - Double-sided
    IIIa Plugged - Single-sided
    IIIb Plugged - Double-sided
    IVa Plugged and Covered - Single-sided
    IVb Plugged and Covered - Double-sided
    V Filled (fully plugged)
    VI Filled and Covered
    VIII Filled and Capped
    None...No protection
    */
    addJSONObject( wxString::Format( "\"ViaProtection\": \"%s\",\n", "Ib" ) );
#endif
    removeJSONSepararator();
    closeBlockWithSep();
}


void GERBER_JOBFILE_WRITER::addJSONFilesAttributes()
{
    // Add the Files Attributes section in JSON format to m_JSONbuffer
    addJSONObject( "\"FilesAttributes\":\n" );
    openArrayBlock();

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

            openBlock();
            addJSONObject( wxString::Format( "\"Path\":  \"%s\",\n", strname.c_str() ) );
            addJSONObject( wxString::Format( "\"FileFunction\":  \"%s\",\n", gbr_layer_id ) ),
            addJSONObject( wxString::Format( "\"FilePolarity\":  \"%s\"\n", polarity ) );
            closeBlockWithSep();
        }
    }
    // Close the file list:
    removeJSONSepararator();    // remove the last separator
    closeArrayBlockWithSep();
}


void GERBER_JOBFILE_WRITER::addJSONDesignRules()
{
    // Add the Design Rules section in JSON format to m_JSONbuffer
    // Job file support a few design rules:
    const BOARD_DESIGN_SETTINGS& dsnSettings = m_pcb->GetDesignSettings();
    NETCLASS defaultNC = *dsnSettings.GetDefault();
    int minclearanceOuter = defaultNC.GetClearance();
    bool hasInnerLayers = m_pcb->GetCopperLayerCount() > 2;

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
    int minclearance_track2track = minclearanceOuter;

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


    addJSONObject( "\"DesignRules\":\n" );
    openArrayBlock();

    openBlock();
    addJSONObject( "\"Layers\": \"Outer\",\n" );
    addJSONObject( wxString::Format( "\"PadToPad\":  %.3f,\n", minPadClearanceOuter*m_conversionUnits ) );
    addJSONObject( wxString::Format( "\"PadToTrack\":  %.3f,\n", minPadClearanceOuter*m_conversionUnits ) );
    addJSONObject( wxString::Format( "\"TrackToTrack\":  %.3f,\n", minclearance_track2track*m_conversionUnits ) );

    // Until this is changed in Kicad, use the same value for internal tracks
    int minclearanceInner = minclearanceOuter;

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
        addJSONObject( wxString::Format( "\"MinLineWidth\":  %.3f,\n",
                                mintrackWidthOuter*m_conversionUnits ) );

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
        addJSONObject( wxString::Format( "\"TrackToRegion\":  %.3f,\n",
                            minclearanceOuter*m_conversionUnits ) );

    if( minclearanceOuter != INT_MAX )
        addJSONObject( wxString::Format( "\"RegionToRegion\":  %.3f,\n",
                            minclearanceOuter*m_conversionUnits ) );

    removeJSONSepararator();    // remove the last separator

    if( !hasInnerLayers )
        closeBlock();
    else
        closeBlockWithSep();


    if( hasInnerLayers )
    {
        openBlock();
        addJSONObject( "\"Layers\": \"Inner\",\n" );
        addJSONObject( wxString::Format( "\"PadToPad\":  %.3f,\n", minPadClearanceInner*m_conversionUnits ) );
        addJSONObject( wxString::Format( "\"PadToTrack\":  %.3f,\n", minPadClearanceInner*m_conversionUnits ) );
        addJSONObject( wxString::Format( "\"TrackToTrack\":  %.3f,\n", minclearance_track2track*m_conversionUnits ) );

        if( mintrackWidthInner != INT_MAX )
            addJSONObject( wxString::Format( "\"MinLineWidth\":  %.3f,\n", mintrackWidthInner*m_conversionUnits ) );

        if( minclearanceInner != INT_MAX )
            addJSONObject( wxString::Format( "\"TrackToRegion\":  %.3f,\n", minclearanceInner*m_conversionUnits ) );

        if( minclearanceInner != INT_MAX )
            addJSONObject( wxString::Format( "\"RegionToRegion\":  %.3f,\n", minclearanceInner*m_conversionUnits ) );

        removeJSONSepararator();    // remove the last separator
        closeBlock();
    }

    // Close DesignRules
    closeArrayBlockWithSep();
}


void GERBER_JOBFILE_WRITER::addJSONMaterialStackup()
{
    // Add the Material Stackup section in JSON format to m_JSONbuffer
    addJSONObject( "\"MaterialStackup\":\n" );
    openArrayBlock();

    // Build the candidates: only layers on a board are candidates:
    LSET maskLayer;

    for( unsigned ii = 0; ii < m_params.m_GerberFileList.GetCount(); ii ++ )
    {
        PCB_LAYER_ID layer = m_params.m_LayerId[ii];

        if( layer <= B_Cu )
            maskLayer.set( layer );
        else
        {
            switch( layer )
            {
                case B_Paste:
                case F_Paste:
                case B_SilkS:
                case F_SilkS:
                case B_Mask:
                case F_Mask:
                    maskLayer.set( layer );
                    break;

                case Edge_Cuts:
                case B_Adhes:
                case F_Adhes:
                case B_Fab:
                case F_Fab:
                case Dwgs_User:
                case Cmts_User:
                case Eco1_User:
                case Eco2_User:
                case Margin:
                case B_CrtYd:
                case F_CrtYd:
                   break;

                default:
                    m_reporter->Report(
                                    wxString::Format( "Unexpected layer id %d in job file", layer ),
                                    REPORTER::RPT_ERROR );
                    break;
            }
        }
    }

    // build a candidate list (in reverse order: bottom to top):
    LSEQ list = maskLayer.SeqStackupBottom2Top();
    // Generate the list (top to bottom):
    for( int ii = list.size()-1; ii >= 0; --ii )
    {
        PCB_LAYER_ID layer = list[ii];
        wxString layer_type;
        wxString color;
        wxString dielectric;
        double thickness = 0.0;     // layer thickness in mm

        if( layer <= B_Cu )
        {
            layer_type = "Copper";
            //thickness = 0.035;
        }
        else
        {
            switch( layer )
            {
            case B_Paste:
            case F_Paste:
                layer_type = "SolderPaste";
                break;

            case B_SilkS:
            case F_SilkS:
                //color = "White";
                layer_type = "Legend";
                break;

            case B_Mask:
            case F_Mask:
                //color = "Green";
                //thickness = 0.025;
                layer_type = "SolderMask";
                break;

            default:
                break;
            }
        }

        openBlock();
        addJSONObject( wxString::Format( "\"Type\":  \"%s\",\n", layer_type ) );

        if( !color.IsEmpty() )
            addJSONObject( wxString::Format( "\"Color\":  \"%s\",\n", color ) );

        if( thickness > 0.0 )
            addJSONObject( wxString::Format( "\"Thickness\":  %f,\n", thickness ) );

        std::string strname = formatStringToGerber(  m_pcb->GetLayerName( layer ) );
        addJSONObject( wxString::Format( "\"Notes\":  \"Layer %s\",\n", strname.c_str() ) );
        removeJSONSepararator();
        closeBlockWithSep();

        if( layer < B_Cu )     // Add dielectric between copper layers
        {
            dielectric = "FR4";     // Temporary

            openBlock();
            addJSONObject( wxString::Format( "\"Type\":  \"%s\",\n", "Dielectric" ) );

            if( thickness > 0.0 )
                addJSONObject( wxString::Format( "\"Thickness\":  %f,\n", color ) );

            if( !dielectric.IsEmpty() )
                addJSONObject( wxString::Format( "\"Material\":  \"%s\",\n", dielectric ) );

            addJSONObject( wxString::Format( "\"Notes\":  \"Layers L%d/L%d\",\n",
                          layer+1, layer+2 ) );
            removeJSONSepararator();
            closeBlockWithSep();
        }
    }

    removeJSONSepararator();
    closeArrayBlockWithSep();
}
