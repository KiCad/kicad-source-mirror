/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
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

#include <algorithm>
#include <set>

#include "pcb_io_odbpp.h"
#include "progress_reporter.h"
#include "odb_util.h"
#include "odb_attribute.h"

#include "odb_defines.h"
#include "odb_feature.h"
#include "odb_entity.h"
#include "wx/log.h"

#include <footprint.h>
#include <pad.h>
#include <board.h>
#include <board_design_settings.h>
#include <exporters/fab_model/fab_stackup.h>
#include <exporters/fab_model/fab_net_names.h>
#include <netinfo.h>
#include <ki_exception.h>
#include <lset.h>
#include <string_utils.h>


namespace
{
bool HasIntentionalShorts( const BOARD& aBoard )
{
    for( const FOOTPRINT* footprint : aBoard.Footprints() )
    {
        if( !footprint->IsNetTie() )
            continue;

        std::map<wxString, int>      groups = footprint->MapPadNumbersToNetTieGroups();
        std::map<int, std::set<int>> groupNets;

        for( const PAD* pad : footprint->Pads() )
        {
            auto group = groups.find( pad->GetNumber() );

            if( group == groups.end() || group->second < 0 || pad->GetNetCode() <= 0 )
                continue;

            if( groupNets[group->second].insert( pad->GetNetCode() ).second && groupNets[group->second].size() > 1 )
            {
                return true;
            }
        }
    }

    return false;
}
} // namespace


PCB_IO_ODBPP::~PCB_IO_ODBPP()
{
    ClearLoadedFootprints();
}


void PCB_IO_ODBPP::ClearLoadedFootprints()
{
    m_loaded_footprints.clear();
}


void PCB_IO_ODBPP::CreateEntity()
{
    m_maxUid = 0;
    m_netTieFeatures.clear();
    m_netNames.clear();
    m_netNames.emplace( 0, wxS( "$NONE$" ) );
    std::map<int, wxString> bases;
    std::map<int, wxString> anonymous = m_format.m_anonymizeNets ? FabAnonymousNetNames( *m_board )
                                                                  : std::map<int, wxString>{};

    for( const NETINFO_ITEM* net : m_board->GetNetInfo() )
    {
        if( net->GetNetCode() <= 0 )
            continue;

        wxString base = m_format.m_anonymizeNets ? anonymous.at( net->GetNetCode() )
                                                 : ODB::GenLegalNetName( net->GetNetname() );
        bases.emplace( net->GetNetCode(), base );
    }

    std::vector<wxString> baseNames;
    baseNames.reserve( bases.size() );

    for( const auto& entry : bases )
        baseNames.push_back( entry.second );

    std::vector<wxString> names = ODB::UniqueNames( baseNames, { wxS( "$NONE$" ) }, wxString::npos );

    size_t index = 0;

    for( const auto& entry : bases )
        m_netNames.emplace( entry.first, names[index++] );

    Make<ODB_FONTS_ENTITY>();
    Make<ODB_INPUT_ENTITY>();
    Make<ODB_MATRIX_ENTITY>( m_board, this );
    Make<ODB_STEP_ENTITY>( m_board, this );
    Make<ODB_MISC_ENTITY>( m_board, this );
    Make<ODB_SYMBOLS_ENTITY>();
    Make<ODB_USER_ENTITY>();
    Make<ODB_WHEELS_ENTITY>();
}


bool PCB_IO_ODBPP::GenerateFiles( ODB_TREE_WRITER& writer )
{
    for( const auto& entity : m_entities )
    {
        if( !entity->CreateDirectoryTree( writer ) )
            throw std::runtime_error( "Failed in create directory tree process" );

        try
        {
            entity->GenerateFiles( writer );
        }
        catch( const std::exception& e )
        {
            throw std::runtime_error( "Failed in generate files process.\n" + std::string( e.what() ) );
        }

    }
    return true;
}


bool PCB_IO_ODBPP::ExportODB( const wxString& aFileName )
{
    try
    {
        std::shared_ptr<ODB_TREE_WRITER> writer =
                std::make_shared<ODB_TREE_WRITER>( aFileName );
        writer->SetRootPath( writer->GetCurrentPath() );

        if( m_progressReporter )
        {
            m_progressReporter->SetNumPhases( 3 );
            m_progressReporter->BeginPhase( 0 );
            m_progressReporter->Report( _( "Creating ODB++ Structure" ) );
        }

        CreateEntity();

        if( m_progressReporter )
        {
            m_progressReporter->SetCurrentProgress( 1.0 );
        }

        for( auto const& entity : m_entities )
        {
            entity->InitEntityData();
        }

        if( m_progressReporter )
        {
            m_progressReporter->SetCurrentProgress( 1.0 );
            m_progressReporter->AdvancePhase( _( "Exporting board to ODB++" ) );
        }

        if( !GenerateFiles( *writer ) )
            return false;

        return true;
    }
    catch( const std::exception& e )
    {
        Report( wxString::Format( "Exception in ODB++ ExportODB process: %s", e.what() ), RPT_SEVERITY_ERROR );
        std::cerr << e.what() << std::endl;
        return false;
    }
}


std::vector<FOOTPRINT*> PCB_IO_ODBPP::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> retval;
    retval.reserve( m_loaded_footprints.size() );

    for( const auto& fp : m_loaded_footprints )
    {
        retval.push_back( static_cast<FOOTPRINT*>( fp->Clone() ) );
    }

    return retval;
}


const FAB_DRILL_MODEL& PCB_IO_ODBPP::GetFabDrillModel()
{
    if( !m_fabDrillModel )
        m_fabDrillModel = std::make_unique<FAB_DRILL_MODEL>( *m_board );

    return *m_fabDrillModel;
}


void PCB_IO_ODBPP::ConfigureExport( BOARD& aBoard, const ODB_EXPORT_OPTIONS& aOptions )
{
    m_board = &aBoard;
    m_entities.clear();
    m_topeprint_subnets.clear();
    m_plane_subnets.clear();
    m_via_trace_subnets.clear();
    m_fabDrillModel.reset();
    m_fabStackup = std::make_unique<FAB_STACKUP>( aBoard );

    // A second export on this plugin starts with the default format
    m_format = ODB_FORMAT();
    m_format.m_variantName = aBoard.GetCurrentVariant();
    m_format.m_variantNames = ODB::VARIANT_NAMES::Build( aBoard.GetVariantNames() );

    if( aOptions.m_variant )
    {
        m_format.m_variantName = aOptions.m_variant->CmpNoCase( GetDefaultVariantName() ) == 0
                                 ? wxString() : *aOptions.m_variant;
    }

    m_format.m_productModelName = aOptions.m_productModelName;
    m_format.m_mpnField = aOptions.m_mpnField;
    m_format.m_anonymizeNets = aOptions.m_anonymizeNets;
    m_format.m_boardMetadata = aOptions.m_boardMetadata;
    m_format.m_sections = aOptions.m_sections;

    if( aOptions.m_origin == JOB_EXPORT_PCB_ODB::ORIGIN::AUX )
        m_format.m_originOffset = aBoard.GetDesignSettings().GetAuxOrigin();
    else if( aOptions.m_origin == JOB_EXPORT_PCB_ODB::ORIGIN::GRID )
        m_format.m_originOffset = aBoard.GetDesignSettings().GetGridOrigin();

    if( m_format.m_sections && m_format.m_sections->Contains( FAB::SECTION::COMPONENTS )
        && !m_format.m_sections->Contains( FAB::SECTION::PACKAGES ) )
    {
        m_format.m_sections->Set( FAB::SECTION::PACKAGES );
        Report( _( "ODB++ package section added because components reference packages." ),
                RPT_SEVERITY_WARNING );
    }

    for( const ODB_LAYER_OVERRIDE& override : aOptions.m_layerOverrides )
    {
        if( override.m_layer == UNDEFINED_LAYER )
        {
            Report( _( "Ignoring invalid ODB++ layer override." ), RPT_SEVERITY_WARNING );
            continue;
        }

        m_format.m_layerOverrides.push_back( override );
    }

    if( aOptions.m_inch )
    {
        m_format.m_unitsStr = "INCH";
        m_format.m_scale = ( 1.0 / 25.4 ) / PCB_IU_PER_MM;
        m_format.m_symbolScale = ( 1.0 / 25.4 ) / PL_IU_PER_MM;
    }

    int precisionFloor = MinPrecision( aOptions.m_inch );
    m_format.m_sigfig = std::clamp( aOptions.m_precision, precisionFloor, MaxPrecision() );

    if( aOptions.m_precision < precisionFloor )
    {
        Report( wxString::Format( _( "ODB++ precision %d is below the minimum of %d for these units; "
                                     "using %d." ), aOptions.m_precision, precisionFloor, m_format.m_sigfig ),
                RPT_SEVERITY_WARNING );
    }
    else if( aOptions.m_precision > MaxPrecision() )
    {
        Report( wxString::Format( _( "ODB++ precision %d is above the maximum of %d; using %d." ),
                                  aOptions.m_precision, MaxPrecision(), m_format.m_sigfig ), RPT_SEVERITY_WARNING );
    }

    bool componentNets = m_format.Includes( FAB::SECTION::COMPONENTS );
    bool logicalNets = m_format.Includes( FAB::SECTION::LOGICAL_NET );
    bool shortNets = !componentNets && !logicalNets && m_format.Includes( FAB::SECTION::PHYSICAL_NET )
                     && HasIntentionalShorts( aBoard );
    m_format.m_writeEdaNets = componentNets || logicalNets || shortNets;

    if( shortNets )
        Report( _( "ODB++ fabrication data includes EDA nets for intentional shorts." ), RPT_SEVERITY_WARNING );
}


void PCB_IO_ODBPP::Export( const wxString& aFileName, BOARD& aBoard, const ODB_EXPORT_OPTIONS& aOptions )
{
    ConfigureExport( aBoard, aOptions );

    if( !ExportODB( aFileName ) )
        THROW_IO_ERROR( _( "ODB++ export failed. See the messages above for the cause." ) );
}


void PCB_IO_ODBPP::SaveBoard( const wxString& aFileName, BOARD& aBoard, const std::map<std::string, UTF8>* aProperties )
{
    ODB_EXPORT_OPTIONS options;

    if( aProperties )
    {
        if( auto it = aProperties->find( "units" ); it != aProperties->end() )
            options.m_inch = it->second == "inch";

        if( auto it = aProperties->find( "sigfig" ); it != aProperties->end() )
            options.m_precision = std::stoi( it->second );
    }

    Export( aFileName, aBoard, options );
}
