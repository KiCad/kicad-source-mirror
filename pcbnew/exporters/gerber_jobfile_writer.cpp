/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean_Pierre Charras <jp.charras at wanadoo.fr>
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

/**
 * @file gendrill_gerber_writer.cpp
 * @brief Functions to create the Gerber job file in JSON format.
 */

#include <fstream>
#include <iomanip>
#include <vector>

#include <build_version.h>
#include <locale_io.h>
#include <pcb_edit_frame.h>
#include <plotters/plotter.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>

#include <board_stackup_manager/stackup_predefined_prms.h>
#include <gbr_metadata.h>
#include <gerber_jobfile_writer.h>
#include <pcbplot.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>


GERBER_JOBFILE_WRITER::GERBER_JOBFILE_WRITER( BOARD* aPcb, REPORTER* aReporter )
{
    m_pcb = aPcb;
    m_reporter = aReporter;
    m_conversionUnits = 1.0 / pcbIUScale.IU_PER_MM; // Gerber units = mm
}

std::string GERBER_JOBFILE_WRITER::formatStringFromUTF32( const wxString& aText )
{
    std::string fmt_text; // the text after UTF32 to UTF8 conversion
    fmt_text = aText.utf8_string();

    return fmt_text;
}


enum ONSIDE GERBER_JOBFILE_WRITER::hasSilkLayers()
{
    int flag = SIDE_NONE;

    for( PCB_LAYER_ID layer : m_params.m_LayerId )
    {
        if( layer == B_SilkS )
            flag |= SIDE_BOTTOM;

        if( layer == F_SilkS )
            flag |= SIDE_TOP;
    }

    return (enum ONSIDE) flag;
}


enum ONSIDE GERBER_JOBFILE_WRITER::hasSolderMasks()
{
    int flag = SIDE_NONE;

    for( PCB_LAYER_ID layer : m_params.m_LayerId )
    {
        if( layer == B_Mask )
            flag |= SIDE_BOTTOM;

        if( layer == F_Mask )
            flag |= SIDE_TOP;
    }

    return (enum ONSIDE) flag;
}

const char* GERBER_JOBFILE_WRITER::sideKeyValue( enum ONSIDE aValue )
{
    // return the key associated to sides used for some layers
    // "No, TopOnly, BotOnly or Both"
    const char* value = nullptr;

    switch( aValue )
    {
    case SIDE_NONE:   value = "No";      break;
    case SIDE_TOP:    value = "TopOnly"; break;
    case SIDE_BOTTOM: value = "BotOnly"; break;
    case SIDE_BOTH:   value = "Both";    break;
    }

    return value;
}


bool GERBER_JOBFILE_WRITER::CreateJobFile( const wxString& aFullFilename )
{
    bool     success;
    wxString msg;

    success = WriteJSONJobFile( aFullFilename );

    if( !success )
    {
        if( m_reporter )
        {
            msg.Printf( _( "Failed to create file '%s'." ), aFullFilename );
            m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        }
    }
    else if( m_reporter )
    {
        msg.Printf( _( "Created Gerber job file '%s'." ), aFullFilename );
        m_reporter->Report( msg, RPT_SEVERITY_ACTION );
    }

    return success;
}


void GERBER_JOBFILE_WRITER::addJSONHeader()
{
    m_json["Header"] = {
        {
            "GenerationSoftware",
            {
                { "Vendor", "KiCad" },
                { "Application", "Pcbnew" },
                { "Version", GetBuildVersion() }
            }
        },
        {
            // The attribute value must conform to the full version of the ISO 8601
            // date and time format, including time and time zone.
            "CreationDate", GbrMakeCreationDateAttributeString( GBR_NC_STRING_FORMAT_GBRJOB )
        }
    };
}


bool GERBER_JOBFILE_WRITER::WriteJSONJobFile( const wxString& aFullFilename )
{
    // Note: in Gerber job file, dimensions are in mm, and are floating numbers
    std::ofstream file( aFullFilename.ToUTF8() );

    m_json = nlohmann::ordered_json( {} );

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

    file << std::setw( 2 ) << m_json << std::endl;

    return true;
}


double GERBER_JOBFILE_WRITER::mapValue( double aUiValue )
{
    // A helper function to convert aUiValue in Json units (mm) and to have
    // 4 digits in Json  in mantissa when using %g to print it
    // i.e. displays values truncated in 0.1 microns.
    // This is enough for a Json file
    char buffer[128];
    std::snprintf( buffer, sizeof( buffer ), "%.4f", aUiValue * m_conversionUnits );

    long double output;
    sscanf( buffer, "%Lg", &output );

    return output;

}


void GERBER_JOBFILE_WRITER::addJSONGeneralSpecs()
{
    m_json["GeneralSpecs"] = nlohmann::ordered_json( {} );
    m_json["GeneralSpecs"]["ProjectId"] = nlohmann::ordered_json( {} );

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
    wxString   msg = fn.GetFullName();

    // Build a <project GUID>, from the board name
    wxString guid = GbrMakeProjectGUIDfromString( msg );

    // build the <project id> string: this is the board short filename (without ext)
    // and in UTF8 format.
    msg = fn.GetName();

    // build the <rev> string. All non ASCII chars are in UTF8 form
    wxString rev = ExpandTextVars( m_pcb->GetTitleBlock().GetRevision(), m_pcb->GetProject() );

    if( rev.IsEmpty() )
        rev = wxT( "rev?" );

    m_json["GeneralSpecs"]["ProjectId"]["Name"] = msg.utf8_string().c_str();
    m_json["GeneralSpecs"]["ProjectId"]["GUID"] = guid;
    m_json["GeneralSpecs"]["ProjectId"]["Revision"] = rev.utf8_string().c_str();

    // output the board size in mm:
    BOX2I brect = m_pcb->GetBoardEdgesBoundingBox();

    m_json["GeneralSpecs"]["Size"]["X"] = mapValue( brect.GetWidth() );
    m_json["GeneralSpecs"]["Size"]["Y"] = mapValue( brect.GetHeight() );


    // Add some data to the JSON header, GeneralSpecs:
    // number of copper layers
    m_json["GeneralSpecs"]["LayerNumber"] = m_pcb->GetCopperLayerCount();

    // Board thickness
    m_json["GeneralSpecs"]["BoardThickness"] =
            mapValue( m_pcb->GetDesignSettings().GetBoardThickness() );

    // Copper finish
    const BOARD_STACKUP brd_stackup = m_pcb->GetDesignSettings().GetStackupDescriptor();

    if( !brd_stackup.m_FinishType.IsEmpty() )
        m_json["GeneralSpecs"]["Finish"] = brd_stackup.m_FinishType;

    if( brd_stackup.m_HasDielectricConstrains )
        m_json["GeneralSpecs"]["ImpedanceControlled"] = true;

    #if 0   // Old way to set property
    if( brd_stackup.m_CastellatedPads )
        m_json["GeneralSpecs"]["Castellated"] = true;
    #endif
    if( m_pcb->GetPadWithCastellatedAttrCount() )
        m_json["GeneralSpecs"]["Castellated"] = true;

    if( m_pcb->GetPadWithPressFitAttrCount() )
        m_json["GeneralSpecs"]["Press-fit"] = true;

    if( brd_stackup.m_EdgePlating )
        m_json["GeneralSpecs"]["EdgePlating"] = true;

    if( brd_stackup.m_EdgeConnectorConstraints )
    {
        m_json["GeneralSpecs"]["EdgeConnector"] = true;

        m_json["GeneralSpecs"]["EdgeConnectorBevelled"] =
                ( brd_stackup.m_EdgeConnectorConstraints == BS_EDGE_CONNECTOR_BEVELLED );
    }

#if 0 // Not yet in use
    /* The board type according to IPC-2221. There are six primary board types:
    - Type 1 - Single-sided
    - Type 2 - Double-sided
    - Type 3 - Multilayer, TH components only
    - Type 4 - Multilayer, with TH, blind and/or buried vias.
    - Type 5 - Multilayer metal-core board, TH components only
    - Type 6 - Multilayer metal-core
    */
    m_json["GeneralSpecs"]["IPC-2221-Type"] = 4;

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
    m_json["GeneralSpecs"]["ViaProtection"] = "Ib";
#endif
}


void GERBER_JOBFILE_WRITER::addJSONFilesAttributes()
{
    // Add the Files Attributes section in JSON format to m_JSONbuffer
    m_json["FilesAttributes"] = nlohmann::ordered_json::array();

    for( unsigned ii = 0; ii < m_params.m_GerberFileList.GetCount(); ii++ )
    {
        wxString&    name = m_params.m_GerberFileList[ii];
        PCB_LAYER_ID layer = m_params.m_LayerId[ii];
        wxString     gbr_layer_id;
        bool         skip_file = false; // true to skip files which should not be in job file
        const char*  polarity = "Positive";

        nlohmann::ordered_json file_json;

        if( IsCopperLayer( layer ) )
        {
            gbr_layer_id = wxT( "Copper,L" );

            if( layer == B_Cu )
                gbr_layer_id << m_pcb->GetCopperLayerCount();
            else if( layer == F_Cu )
                gbr_layer_id << 1;
            else    // Copper layers are numbered B_Cu + n*2 for inner layer n (n = 1 ... val max)
                    // and gbr_layer_id = 2 ... val max
                gbr_layer_id << (layer-B_Cu) / 2 + 1;

            gbr_layer_id << wxT( "," );

            if( layer == B_Cu )
                gbr_layer_id << wxT( "Bot" );
            else if( layer == F_Cu )
                gbr_layer_id << wxT( "Top" );
            else
                gbr_layer_id << wxT( "Inr" );
        }

        else
        {
            switch( layer )
            {
            case B_Adhes:
                gbr_layer_id = wxT( "Glue,Bot" );
                break;
            case F_Adhes:
                gbr_layer_id = wxT( "Glue,Top" );
                break;

            case B_Paste:
                gbr_layer_id = wxT( "SolderPaste,Bot" );
                break;
            case F_Paste:
                gbr_layer_id = wxT( "SolderPaste,Top" );
                break;

            case B_SilkS:
                gbr_layer_id = wxT( "Legend,Bot" );
                break;
            case F_SilkS:
                gbr_layer_id = wxT( "Legend,Top" );
                break;

            case B_Mask:
                gbr_layer_id = wxT( "SolderMask,Bot" );
                polarity = "Negative";
                break;
            case F_Mask:
                gbr_layer_id = wxT( "SolderMask,Top" );
                polarity = "Negative";
                break;

            case Edge_Cuts:
                gbr_layer_id = wxT( "Profile" );
                break;

            case B_Fab:
                gbr_layer_id = wxT( "AssemblyDrawing,Bot" );
                break;
            case F_Fab:
                gbr_layer_id = wxT( "AssemblyDrawing,Top" );
                break;

            case Margin:
            case B_CrtYd:
            case F_CrtYd:
                skip_file = true;
                break;

            case Dwgs_User:
            case Cmts_User:
            case Eco1_User:
            case Eco2_User:
            case User_1:
            case User_2:
            case User_3:
            case User_4:
            case User_5:
            case User_6:
            case User_7:
            case User_8:
            case User_9:
                gbr_layer_id = wxT( "Other,User" );
                break;

            default:
                skip_file = true;

                if( m_reporter )
                    m_reporter->Report( wxT( "Unexpected layer id in job file" ), RPT_SEVERITY_ERROR );

                break;
            }
        }

        if( !skip_file )
        {
            // name can contain non ASCII7 chars.
            // Ensure the name is JSON compatible.
            std::string strname = formatStringFromUTF32( name );

            file_json["Path"] = strname.c_str();
            file_json["FileFunction"] = gbr_layer_id;
            file_json["FilePolarity"] = polarity;

            m_json["FilesAttributes"] += file_json;
        }
    }
}


void GERBER_JOBFILE_WRITER::addJSONDesignRules()
{
    // Add the Design Rules section in JSON format to m_JSONbuffer
    // Job file support a few design rules:
    std::shared_ptr<NET_SETTINGS>& netSettings = m_pcb->GetDesignSettings().m_NetSettings;

    int  minclearanceOuter = netSettings->GetDefaultNetclass()->GetClearance();
    bool hasInnerLayers = m_pcb->GetCopperLayerCount() > 2;

    // Search a smaller clearance in other net classes, if any.
    for( const auto& [name, netclass] : netSettings->GetNetclasses() )
        minclearanceOuter = std::min( minclearanceOuter, netclass->GetClearance() );

    // job file knows different clearance types.
    // Kicad knows only one clearance for pads and tracks
    int minclearance_track2track = minclearanceOuter;

    // However, pads can have a specific clearance defined for a pad or a footprint,
    // and min clearance can be dependent on layers.
    // Search for a minimal pad clearance:
    int minPadClearanceOuter = netSettings->GetDefaultNetclass()->GetClearance();
    int minPadClearanceInner = netSettings->GetDefaultNetclass()->GetClearance();

    for( FOOTPRINT* footprint : m_pcb->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            for( PCB_LAYER_ID layer : pad->GetLayerSet() )
            {
                int padClearance = pad->GetOwnClearance( layer );

                if( layer == B_Cu || layer == F_Cu )
                    minPadClearanceOuter = std::min( minPadClearanceOuter, padClearance );
                else
                    minPadClearanceInner = std::min( minPadClearanceInner, padClearance );
            }
        }
    }

    m_json["DesignRules"] = { {
        { "Layers", "Outer" },
        { "PadToPad", mapValue( minPadClearanceOuter ) },
        { "PadToTrack", mapValue( minPadClearanceOuter ) },
        { "TrackToTrack", mapValue( minclearance_track2track ) }
    } };

    // Until this is changed in Kicad, use the same value for internal tracks
    int minclearanceInner = minclearanceOuter;

    // Output the minimal track width
    int mintrackWidthOuter = INT_MAX;
    int mintrackWidthInner = INT_MAX;

    for( PCB_TRACK* track : m_pcb->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            continue;

        if( track->GetLayer() == B_Cu || track->GetLayer() == F_Cu )
            mintrackWidthOuter = std::min( mintrackWidthOuter, track->GetWidth() );
        else
            mintrackWidthInner = std::min( mintrackWidthInner, track->GetWidth() );
    }

    if( mintrackWidthOuter != INT_MAX )
        m_json["DesignRules"][0]["MinLineWidth"] = mapValue( mintrackWidthOuter );

    // Output the minimal zone to xx clearance
    // Note: zones can have a zone clearance set to 0
    // if happens, the actual zone clearance is the clearance of its class
    minclearanceOuter = INT_MAX;
    minclearanceInner = INT_MAX;

    for( ZONE* zone : m_pcb->Zones() )
    {
        if( zone->GetIsRuleArea() || !zone->IsOnCopperLayer() )
            continue;

        for( PCB_LAYER_ID layer : zone->GetLayerSet() )
        {
            int zclerance = zone->GetOwnClearance( layer );

            if( layer == B_Cu || layer == F_Cu )
                minclearanceOuter = std::min( minclearanceOuter, zclerance );
            else
                minclearanceInner = std::min( minclearanceInner, zclerance );
        }
    }

    if( minclearanceOuter != INT_MAX )
        m_json["DesignRules"][0]["TrackToRegion"] = mapValue( minclearanceOuter );

    if( minclearanceOuter != INT_MAX )
        m_json["DesignRules"][0]["RegionToRegion"] = mapValue( minclearanceOuter );

    if( hasInnerLayers )
    {
        m_json["DesignRules"] += nlohmann::ordered_json( {
            { "Layers", "Inner" },
            { "PadToPad", mapValue( minPadClearanceInner ) },
            { "PadToTrack", mapValue( minPadClearanceInner ) },
            { "TrackToTrack", mapValue( minclearance_track2track  ) }
        } );

        if( mintrackWidthInner != INT_MAX )
            m_json["DesignRules"][1]["MinLineWidth"] = mapValue( mintrackWidthInner );

        if( minclearanceInner != INT_MAX )
            m_json["DesignRules"][1]["TrackToRegion"] = mapValue( minclearanceInner  );

        if( minclearanceInner != INT_MAX )
            m_json["DesignRules"][1]["RegionToRegion"] = mapValue( minclearanceInner );
    }
}


void GERBER_JOBFILE_WRITER::addJSONMaterialStackup()
{
    // Add the Material Stackup section in JSON format to m_JSONbuffer
    m_json["MaterialStackup"] = nlohmann::ordered_json::array();

    // Build the candidates list:
    LSET maskLayer;
    BOARD_STACKUP brd_stackup = m_pcb->GetDesignSettings().GetStackupDescriptor();

    // Ensure brd_stackup is up to date (i.e. no change made by SynchronizeWithBoard() )
    bool uptodate = not brd_stackup.SynchronizeWithBoard( &m_pcb->GetDesignSettings() );

    if( m_reporter && !uptodate && m_pcb->GetDesignSettings().m_HasStackup )
        m_reporter->Report( _( "Board stackup settings not up to date." ), RPT_SEVERITY_ERROR );

    PCB_LAYER_ID last_copper_layer = F_Cu;

    // Generate the list (top to bottom):
    for( int ii = 0; ii < brd_stackup.GetCount(); ++ii )
    {
        BOARD_STACKUP_ITEM* item = brd_stackup.GetStackupLayer( ii );

        int sub_layer_count =
                item->GetType() == BS_ITEM_TYPE_DIELECTRIC ? item->GetSublayersCount() : 1;

        for( int sub_idx = 0; sub_idx < sub_layer_count; sub_idx++ )
        {
            // layer thickness is always in mm
            double      thickness = mapValue( item->GetThickness( sub_idx ) );
            wxString    layer_type;
            std::string layer_name; // for comment

            nlohmann::ordered_json layer_json;

            switch( item->GetType() )
            {
            case BS_ITEM_TYPE_COPPER:
                layer_type = wxT( "Copper" );
                layer_name = formatStringFromUTF32( m_pcb->GetLayerName( item->GetBrdLayerId() ) );
                last_copper_layer = item->GetBrdLayerId();
                break;

            case BS_ITEM_TYPE_SILKSCREEN:
                layer_type = wxT( "Legend" );
                layer_name = formatStringFromUTF32( item->GetTypeName() );
                break;

            case BS_ITEM_TYPE_SOLDERMASK:
                layer_type = wxT( "SolderMask" );
                layer_name = formatStringFromUTF32( item->GetTypeName() );
                break;

            case BS_ITEM_TYPE_SOLDERPASTE:
                layer_type = wxT( "SolderPaste" );
                layer_name = formatStringFromUTF32( item->GetTypeName() );
                break;

            case BS_ITEM_TYPE_DIELECTRIC:
                layer_type = wxT( "Dielectric" );
                // The option core or prepreg is not added here, as it creates constraints
                // in build process, not necessary wanted.
                if( sub_layer_count > 1 )
                {
                    layer_name =
                            formatStringFromUTF32( wxString::Format( wxT( "dielectric layer %d - %d/%d" ),
                                    item->GetDielectricLayerId(), sub_idx + 1, sub_layer_count ) );
                }
                else
                    layer_name = formatStringFromUTF32( wxString::Format(
                            wxT( "dielectric layer %d" ), item->GetDielectricLayerId() ) );
                break;

            default:
                break;
            }

            layer_json["Type"] = layer_type;

            if( item->IsColorEditable() && uptodate )
            {
                if( IsPrmSpecified( item->GetColor( sub_idx ) ) )
                {
                    wxString colorName = item->GetColor( sub_idx );

                    if( colorName.StartsWith( wxT( "#" ) ) )    // This is a user defined color,
                                                                // not in standard color list.
                    {
                        // In job file a color can be given by its RGB values (0...255)
                        // like R<number><G<number>B<number> notation
                        wxColor color( COLOR4D( colorName ).ToColour() );
                        colorName.Printf( wxT( "R%dG%dB%d" ),
                                          color.Red(),
                                          color.Green(),
                                          color.Blue() );
                    }
                    else
                    {
                        const std::vector<FAB_LAYER_COLOR>& color_list =
                                                    GetStandardColors( item->GetType() );

                        // Colors for dielectric use a color list that is mainly not normalized in
                        // job file names. So if a color is in the dielectric standard color list
                        // it can be a standard name or not.
                        // Colors for solder mask and silk screen use a mainly normalized
                        // color list, but this list can also contain not normalized colors.
                        // If not normalized, use the R<number><G<number>B<number> notation
                        for( const FAB_LAYER_COLOR& prm_color : color_list  )
                        {
                            if( colorName == prm_color.GetName() )
                            {
                                colorName = prm_color.GetColorAsString();
                                break;
                            }
                        }
                    }

                    layer_json["Color"] = colorName;
                }
            }

            if( item->IsThicknessEditable() && uptodate )
                layer_json["Thickness"] = thickness;

            if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            {
                if( item->HasMaterialValue() )
                {
                    layer_json["Material"] = item->GetMaterial( sub_idx );

                    // These constrains are only written if the board has impedance controlled tracks.
                    // If the board is not impedance controlled,  they are useless.
                    // Do not add constrains that create more expensive boards.

                    if( brd_stackup.m_HasDielectricConstrains )
                    {
                        // Generate Epsilon R if > 1.0 (value <= 1.0 means not specified: it is not
                        // a possible value
                        if( item->GetEpsilonR() > 1.0 )
                            layer_json["DielectricConstant"] = item->FormatEpsilonR( sub_idx );

                        // Generate LossTangent > 0.0 (value <= 0.0 means not specified: it is not
                        // a possible value
                        if( item->GetLossTangent() > 0.0 )
                            layer_json["LossTangent"] = item->FormatLossTangent( sub_idx );
                    }
                }

                // Copper layers IDs use only even values like 0, 2, 4 ...
                // and first layer = F_Cu = 0, last layer = B_Cu = 2
                // inner layers Ids are 4, 6 , 8 ...
                PCB_LAYER_ID next_copper_layer = ( PCB_LAYER_ID )( last_copper_layer + 2 );

                if( last_copper_layer == F_Cu )
                    next_copper_layer = In1_Cu;

                // If the next_copper_layer is the last copper layer, the next layer id is B_Cu
                if( next_copper_layer/2 >= m_pcb->GetCopperLayerCount() )
                    next_copper_layer = B_Cu;

                wxString subLayerName;

                if( sub_layer_count > 1 )
                    subLayerName.Printf( wxT( " (%d/%d)" ), sub_idx + 1, sub_layer_count );

                wxString name = wxString::Format( wxT( "%s/%s%s" ),
                                                  formatStringFromUTF32( m_pcb->GetLayerName( last_copper_layer ) ),
                                                  formatStringFromUTF32( m_pcb->GetLayerName( next_copper_layer ) ),
                                                  subLayerName );

                layer_json["Name"] = name;

                // Add a comment ("Notes"):
                wxString note;

                note << wxString::Format( wxT( "Type: %s" ), layer_name.c_str() );

                note << wxString::Format( wxT( " (from %s to %s)" ),
                                          formatStringFromUTF32( m_pcb->GetLayerName( last_copper_layer ) ),
                                          formatStringFromUTF32( m_pcb->GetLayerName( next_copper_layer ) ) );

                layer_json["Notes"] = note;
            }
            else if( item->GetType() == BS_ITEM_TYPE_SOLDERMASK
                     || item->GetType() == BS_ITEM_TYPE_SILKSCREEN )
            {
                if( item->HasMaterialValue() )
                {
                    layer_json["Material"] = item->GetMaterial();

                    // These constrains are only written if the board has impedance controlled tracks.
                    // If the board is not impedance controlled,  they are useless.
                    // Do not add constrains that create more expensive boards.
                    if( brd_stackup.m_HasDielectricConstrains )
                    {
                        // Generate Epsilon R if > 1.0 (value <= 1.0 means not specified: it is not
                        // a possible value
                        if( item->GetEpsilonR() > 1.0 )
                            layer_json["DielectricConstant"] = item->FormatEpsilonR();

                        // Generate LossTangent > 0.0 (value <= 0.0 means not specified: it is not
                        // a possible value
                        if( item->GetLossTangent() > 0.0 )
                            layer_json["LossTangent"] = item->FormatLossTangent();
                    }
                }

                layer_json["Name"] = layer_name.c_str();
            }
            else
            {
                layer_json["Name"] = layer_name.c_str();
            }

            m_json["MaterialStackup"].insert( m_json["MaterialStackup"].end(), layer_json );
        }
    }
}
