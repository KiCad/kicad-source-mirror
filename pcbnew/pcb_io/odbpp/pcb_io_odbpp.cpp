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

#include "pcb_io_odbpp.h"
#include "progress_reporter.h"
#include "odb_util.h"
#include "odb_attribute.h"

#include "odb_defines.h"
#include "odb_feature.h"
#include "odb_entity.h"
#include "wx/log.h"

#include <footprint.h>
#include <ki_exception.h>


double      PCB_IO_ODBPP::m_scale = 1.0 / PCB_IU_PER_MM;
double      PCB_IO_ODBPP::m_symbolScale = 1.0 / PL_IU_PER_MM;
int         PCB_IO_ODBPP::m_sigfig = 4;
std::string PCB_IO_ODBPP::m_unitsStr = "MM";

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
    Make<ODB_FONTS_ENTITY>();
    Make<ODB_INPUT_ENTITY>();
    Make<ODB_MATRIX_ENTITY>( m_board, this );
    Make<ODB_STEP_ENTITY>( m_board, this );
    Make<ODB_MISC_ENTITY>();
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


void PCB_IO_ODBPP::SaveBoard( const wxString& aFileName, BOARD& aBoard, const std::map<std::string, UTF8>* aProperties )
{
    m_board = &aBoard;

    // Defaults for a caller that passes no properties at all; otherwise a prior export in the
    // same process would leak its units/precision into this one
    m_unitsStr = "MM";
    m_scale = 1.0 / PCB_IU_PER_MM;
    m_symbolScale = 1.0 / PL_IU_PER_MM;
    m_sigfig = 6;

    if( aProperties )
    {
        if( auto it = aProperties->find( "units" ); it != aProperties->end() )
        {
            // Only INCH needs setting here; MM is already the default set above
            if( it->second == "inch" )
            {
                m_unitsStr = "INCH";
                m_scale = ( 1.0 / 25.4 ) / PCB_IU_PER_MM;
                m_symbolScale = ( 1.0 / 25.4 ) / PL_IU_PER_MM;
            }
        }

        if( auto it = aProperties->find( "sigfig" ); it != aProperties->end() )
        {
            int requested = std::stoi( it->second );
            int precisionFloor = MinPrecision( m_unitsStr == "INCH" );

            m_sigfig = std::clamp( requested, precisionFloor, MaxPrecision() );

            if( requested < precisionFloor )
            {
                Report( wxString::Format( _( "ODB++ precision %d is below the minimum of %d for these units; "
                                             "using %d." ), requested, precisionFloor, m_sigfig ),
                        RPT_SEVERITY_WARNING );
            }
            else if( requested > MaxPrecision() )
            {
                Report( wxString::Format( _( "ODB++ precision %d is above the maximum of %d; using %d." ),
                                          requested, MaxPrecision(), m_sigfig ), RPT_SEVERITY_WARNING );
            }
        }
    }

    if( !ExportODB( aFileName ) )
        THROW_IO_ERROR( _( "ODB++ export failed. See the messages above for the cause." ) );
}
