/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <api/api_pcb_annotation.h>

#include <wx/intl.h>

#include <api/pcb_context.h>
#include <board.h>
#include <footprint.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/netlist_reader.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/pcb_netlist_utils.h>
#include <pad.h>
#include <pcb_field.h>
#include <reporter.h>
#include <richio.h>


bool GenerateBoardNetlist( BOARD& aBoard, std::string& aNetlist )
{
    NETLIST          netlist;
    STRING_FORMATTER sf;

    for( FOOTPRINT* footprint : aBoard.Footprints() )
    {
        if( footprint->GetAttributes() & FP_BOARD_ONLY )
            continue; // Don't add board-only footprints to the netlist

        COMPONENT* component = new COMPONENT( footprint->GetFPID(), footprint->GetReference(), footprint->GetValue(),
                                              footprint->GetPath(), {} );

        for( PAD* pad : footprint->Pads() )
        {
            const wxString& netname = pad->GetShortNetname();

            if( !netname.IsEmpty() )
                component->AddNet( pad->GetNumber(), netname, pad->GetPinFunction(), pad->GetPinType() );
        }

        nlohmann::ordered_map<wxString, wxString> fields;

        for( PCB_FIELD* field : footprint->GetFields() )
        {
            wxCHECK2( field, continue );

            fields[field->GetUntranslatedName()] = field->GetText();
        }

        component->SetFields( fields );

        // Add DNP and exclusion properties
        std::map<wxString, wxString> properties;

        if( footprint->GetAttributes() & FP_DNP )
            properties.emplace( "dnp", "" );

        if( footprint->GetAttributes() & FP_EXCLUDE_FROM_BOM )
            properties.emplace( "exclude_from_bom", "" );

        if( footprint->GetAttributes() & FP_EXCLUDE_FROM_SIM )
            properties.emplace( "exclude_from_sim", "" );

        if( footprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES )
            properties.emplace( "exclude_from_pos_files", "" );

        component->SetProperties( properties );

        netlist.AddComponent( component );
    }

    netlist.Format( "pcb_netlist", &sf, 0, CTL_OMIT_FILTERS );
    aNetlist = sf.GetString();

    return true;
}


bool ParseSchematicNetlist( const std::string& aNetlistText, NETLIST& aNetlist, REPORTER& aReporter )
{
    const size_t firstNonSpace = aNetlistText.find_first_not_of( " \t\r\n" );

    if( firstNonSpace == std::string::npos || aNetlistText.compare( firstNonSpace, 7, "(export" ) != 0 )
    {
        aReporter.Report( _( "The netlist is not a KiCad schematic netlist." ), RPT_SEVERITY_ERROR );
        return false;
    }

    try
    {
        KICAD_NETLIST_READER reader( new STRING_LINE_READER( aNetlistText, _( "Eeschema netlist" ) ), &aNetlist );
        reader.LoadNetlist();
    }
    catch( const IO_ERROR& ioe )
    {
        aReporter.Report( wxString::Format( _( "The netlist could not be read: %s" ), ioe.What() ),
                          RPT_SEVERITY_ERROR );
        return false;
    }

    return true;
}


static bool loadFootprintsForContext( PCB_CONTEXT& aContext, NETLIST& aNetlist, REPORTER& aReporter )
{
    try
    {
        LoadNetlistFootprints( aContext.GetBoard(), aNetlist, aReporter );
    }
    catch( const IO_ERROR& ioe )
    {
        aReporter.Report( wxString::Format( _( "Footprints could not be loaded: %s" ), ioe.What() ),
                          RPT_SEVERITY_ERROR );
        return false;
    }

    return true;
}


bool ApplyNetlist( PCB_CONTEXT& aContext, NETLIST& aNetlist,
                   const kiapi::common::commands::ForwardAnnotationSettings& aSettings, ANNOTATION_RESULT& aResult,
                   REPORTER& aReporter )
{
    using namespace kiapi::common::commands;

    const bool lookupByTimestamp = aSettings.match_mode() != AnnotationMatchMode::AMM_REFERENCE;

    aNetlist.SetFindByTimeStamp( lookupByTimestamp );
    aNetlist.SetReplaceFootprints( aSettings.update_footprints() );

    std::unique_ptr<BOARD_NETLIST_UPDATER> updater = aContext.MakeNetlistUpdater();

    updater->SetReporter( &aReporter );
    updater->SetIsDryRun( aSettings.dry_run() );
    updater->SetWillDrag( false );
    updater->SetLookupByTimestamp( lookupByTimestamp );
    updater->SetDeleteUnusedFootprints( aSettings.delete_extra_footprints() );
    updater->SetReplaceFootprints( aSettings.update_footprints() );
    updater->SetTransferGroups( aSettings.transfer_groups() );
    updater->SetOverrideLocks( aSettings.override_locks() );
    updater->SetUpdateFields( aSettings.update_fields() );
    updater->SetRemoveExtraFields( aSettings.remove_extra_fields() );

    const bool success = updater->UpdateNetlist( aNetlist );

    if( !aSettings.dry_run() && success )
        aContext.OnNetlistChanged( *updater, false );

    aResult.errorCount = updater->GetErrorCount();
    aResult.warningCount = updater->GetWarningCount();
    aResult.newFootprintCount = updater->GetNewFootprintCount();

    return success;
}


bool ApplySchematicNetlist( PCB_CONTEXT& aContext, const std::string& aNetlistText,
                            const kiapi::common::commands::ForwardAnnotationSettings& aSettings,
                            ANNOTATION_RESULT& aResult, REPORTER& aReporter )
{
    NETLIST netlist;

    if( !ParseSchematicNetlist( aNetlistText, netlist, aReporter ) )
        return false;

    if( !loadFootprintsForContext( aContext, netlist, aReporter ) )
        return false;

    return ApplyNetlist( aContext, netlist, aSettings, aResult, aReporter );
}


bool RelinkFootprints( PCB_CONTEXT& aContext, const std::string& aNetlistText, ANNOTATION_RESULT& aResult,
                       REPORTER& aReporter )
{
    NETLIST netlist;

    if( !ParseSchematicNetlist( aNetlistText, netlist, aReporter ) )
        return false;

    if( !loadFootprintsForContext( aContext, netlist, aReporter ) )
        return false;

    kiapi::common::commands::ForwardAnnotationSettings settings;
    settings.set_match_mode( kiapi::common::commands::AMM_REFERENCE );

    return ApplyNetlist( aContext, netlist, settings, aResult, aReporter );
}
