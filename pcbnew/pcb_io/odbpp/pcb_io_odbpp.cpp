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
#include "odb_export_job.h"
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
    std::set<wxString> reserved = { wxS( "$NONE$" ) };

    for( const NETINFO_ITEM* net : m_board->GetNetInfo() )
    {
        if( net->GetNetCode() <= 0 )
            continue;

        wxString base = m_format.m_anonymizeNets ? wxString::Format( wxS( "N%zu" ), bases.size() + 1 )
                                                 : ODB::GenLegalNetName( net->GetNetname() );
        bases.emplace( net->GetNetCode(), base );
        reserved.insert( base );
    }

    std::set<wxString> assigned = { wxS( "$NONE$" ) };

    for( const auto& [code, base] : bases )
    {
        wxString name = base;

        if( assigned.count( name ) )
        {
            for( unsigned suffix = 2; ; ++suffix )
            {
                name = wxString::Format( wxS( "%s_%u" ), base, suffix );

                if( !reserved.count( name ) && !assigned.count( name ) )
                    break;
            }
        }

        m_netNames.emplace( code, name );
        assigned.insert( name );
    }

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


void PCB_IO_ODBPP::ConfigureExport( BOARD& aBoard, const std::map<std::string, UTF8>* aProperties )
{
    m_board = &aBoard;

    // A second export on this plugin starts with the default format
    m_format = ODB_FORMAT();
    m_format.m_variantName = aBoard.GetCurrentVariant();
    m_format.m_variantNames = ODB::VARIANT_NAMES::Build( aBoard.GetVariantNames() );

    if( aProperties )
    {
        if( auto it = aProperties->find( "variant" ); it != aProperties->end() )
        {
            wxString requested = wxString::FromUTF8( it->second.c_str() );
            m_format.m_variantName = requested.CmpNoCase( GetDefaultVariantName() ) == 0
                                     ? wxString() : requested;
        }

        if( auto it = aProperties->find( "product_model_name" ); it != aProperties->end() )
            m_format.m_productModelName = wxString::FromUTF8( it->second.c_str() );

        if( auto it = aProperties->find( "mpn" ); it != aProperties->end() )
            m_format.m_mpnField = wxString::FromUTF8( it->second.c_str() );

        if( auto it = aProperties->find( "origin" ); it != aProperties->end() )
        {
            if( it->second == "aux" )
                m_format.m_originOffset = aBoard.GetDesignSettings().GetAuxOrigin();
            else if( it->second == "grid" )
                m_format.m_originOffset = aBoard.GetDesignSettings().GetGridOrigin();
        }

        if( auto it = aProperties->find( "net_names" ); it != aProperties->end() )
            m_format.m_anonymizeNets = it->second == "anonymize";

        IPC2581::SECTION_SET sections;
        bool                 filterSections = false;

        if( auto it = aProperties->find( "data_set" ); it != aProperties->end() && it->second != "all" )
        {
            std::optional<IPC2581::MODE> mode = IPC2581::ModeFromToken( wxString::FromUTF8( it->second.c_str() ) );

            if( !mode )
                THROW_IO_ERROR( _( "Unknown ODB++ data set." ) );

            sections = OdbDefaultSections( *mode );
            filterSections = true;
        }

        if( auto it = aProperties->find( "sections" ); it != aProperties->end() && !it->second.empty() )
        {
            if( !IPC2581::SectionSetFromKeyString( wxString::FromUTF8( it->second.c_str() ), sections ) )
                THROW_IO_ERROR( _( "Unknown ODB++ section key." ) );

            filterSections = true;
        }

        if( filterSections )
        {
            if( sections.Contains( IPC2581::SECTION::COMPONENTS ) && !sections.Contains( IPC2581::SECTION::PACKAGES ) )
            {
                sections.Set( IPC2581::SECTION::PACKAGES );
                Report( _( "ODB++ package section added because components reference packages." ),
                        RPT_SEVERITY_WARNING );
            }

            m_format.m_sections = sections;
        }

        if( auto it = aProperties->find( "layers" ); it != aProperties->end() )
        {
            nlohmann::json layers = nlohmann::json::parse( it->second.c_str(), nullptr, false );

            if( !layers.is_array() )
                THROW_IO_ERROR( _( "Invalid ODB++ layer overrides." ) );

            for( const nlohmann::json& entry : layers )
            {
                ODB_LAYER_OVERRIDE override = entry.get<ODB_LAYER_OVERRIDE>();

                if( override.m_layer == UNDEFINED_LAYER )
                {
                    Report( _( "Ignoring invalid ODB++ layer override." ), RPT_SEVERITY_WARNING );
                    continue;
                }

                m_format.m_layerOverrides.push_back( std::move( override ) );
            }
        }

        if( auto it = aProperties->find( "units" ); it != aProperties->end() )
        {
            // Only INCH needs setting here; MM is already the default set above
            if( it->second == "inch" )
            {
                m_format.m_unitsStr = "INCH";
                m_format.m_scale = ( 1.0 / 25.4 ) / PCB_IU_PER_MM;
                m_format.m_symbolScale = ( 1.0 / 25.4 ) / PL_IU_PER_MM;
            }
        }

        if( auto it = aProperties->find( "sigfig" ); it != aProperties->end() )
        {
            int requested = std::stoi( it->second );
            int precisionFloor = MinPrecision( m_format.m_unitsStr == "INCH" );

            m_format.m_sigfig = std::clamp( requested, precisionFloor, MaxPrecision() );

            if( requested < precisionFloor )
            {
                Report( wxString::Format( _( "ODB++ precision %d is below the minimum of %d for these units; "
                                             "using %d." ), requested, precisionFloor, m_format.m_sigfig ),
                        RPT_SEVERITY_WARNING );
            }
            else if( requested > MaxPrecision() )
            {
                Report( wxString::Format( _( "ODB++ precision %d is above the maximum of %d; using %d." ),
                                          requested, MaxPrecision(), m_format.m_sigfig ), RPT_SEVERITY_WARNING );
            }
        }
    }

    bool componentNets = m_format.Includes( IPC2581::SECTION::COMPONENTS );
    bool logicalNets = m_format.Includes( IPC2581::SECTION::LOGICAL_NET );
    bool shortNets = !componentNets && !logicalNets && m_format.Includes( IPC2581::SECTION::PHYSICAL_NET )
                     && HasIntentionalShorts( aBoard );
    m_format.m_writeEdaNets = componentNets || logicalNets || shortNets;

    if( shortNets )
        Report( _( "ODB++ fabrication data includes EDA nets for intentional shorts." ), RPT_SEVERITY_WARNING );
}


void PCB_IO_ODBPP::SaveBoard( const wxString& aFileName, BOARD& aBoard, const std::map<std::string, UTF8>* aProperties )
{
    ConfigureExport( aBoard, aProperties );

    if( !ExportODB( aFileName ) )
        THROW_IO_ERROR( _( "ODB++ export failed. See the messages above for the cause." ) );
}
