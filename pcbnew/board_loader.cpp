/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <board_loader.h>

#include <base_screen.h>
#include <board.h>
#include <board_design_settings.h>
#include <drawing_sheet/ds_data_model.h>
#include <drc/drc_engine.h>
#include <filename_resolver.h>
#include <ki_exception.h>
#include <pcb_marker.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <properties/property.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <unordered_set>
#include <wx/image.h>
#include <wx/log.h>


std::unique_ptr<BOARD> BOARD_LOADER::Load( const wxString& aFileName,
                                           PCB_IO_MGR::PCB_FILE_T aFormat,
                                           PROJECT* aProject,
                                           const OPTIONS& aOptions )
{
    if( !aProject || aFormat == PCB_IO_MGR::FILE_TYPE_NONE )
        return nullptr;

    // TODO(JE) need to factor out for MDI
    BASE_SCREEN::m_DrawingSheetFileName = aProject->GetProjectFile().m_BoardDrawingSheetFile;

    BOARD* loaded = nullptr;

    if( aOptions.plugin_configurator || aOptions.reporter )
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( aFormat ) );

        if( !pi )
            return nullptr;

        if( aOptions.plugin_configurator )
            aOptions.plugin_configurator( *pi );

        if( aOptions.reporter )
            pi->SetReporter( aOptions.reporter );

        pi->SetProgressReporter( aOptions.progress_reporter );
        loaded = pi->LoadBoard( aFileName, nullptr, aOptions.properties, aProject );
    }
    else
    {
        loaded = PCB_IO_MGR::Load( aFormat, aFileName, nullptr, aOptions.properties,
                                   aProject, aOptions.progress_reporter );
    }

    std::unique_ptr<BOARD> board( loaded );

    if( !board )
        return nullptr;

    if( aOptions.initialize_after_load )
        initializeLoadedBoard( board.get(), aFileName, aProject, aOptions );

    return board;
}


std::unique_ptr<BOARD> BOARD_LOADER::Load( const wxString& aFileName,
                                           PCB_IO_MGR::PCB_FILE_T aFormat,
                                           PROJECT* aProject )
{
    return Load( aFileName, aFormat, aProject, OPTIONS{} );
}


void BOARD_LOADER::initializeLoadedBoard( BOARD* aBoard, const wxString& aFileName,
                                          PROJECT* aProject, const OPTIONS& aOptions )
{
    if( !aBoard || !aProject )
        return;

    FILENAME_RESOLVER resolver;
    resolver.SetProject( aProject );
    resolver.SetProgramBase( PgmOrNull() );

    wxString filename = resolver.ResolvePath( BASE_SCREEN::m_DrawingSheetFileName,
                                              aProject->GetProjectPath(),
                                              { aBoard->GetEmbeddedFiles() } );

    wxString msg;

    if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename, &msg ) )
    {
        if( aOptions.drawing_sheet_error_callback )
            aOptions.drawing_sheet_error_callback( BASE_SCREEN::m_DrawingSheetFileName, msg );
    }

    ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

    layerEnum.Choices().Clear();
    layerEnum.Undefined( UNDEFINED_LAYER );

    for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
    {
        layerEnum.Map( layer, LSET::Name( layer ) );
        layerEnum.Map( layer, aBoard->GetLayerName( layer ) );
    }

    aBoard->SetProject( aProject );

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();
    bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( aBoard, &bds );

    try
    {
        wxFileName rules = aFileName;
        rules.SetExt( FILEEXT::DesignRulesFileExtension );
        bds.m_DRCEngine->InitEngine( rules );
    }
    catch( ... )
    {
        // Only matters if user tries to run DRC later; will be reported at that time
    }

    for( PCB_MARKER* marker : aBoard->ResolveDRCExclusions( true ) )
        aBoard->Add( marker );

    aBoard->BuildConnectivity();
    aBoard->BuildListOfNets();
    aBoard->SynchronizeNetsAndNetClasses( true );

    // Apply component class assignment rules from the project. Without this, conditions like
    // A.hasComponentClass('Inductor') in custom DRC rules never match because no footprints
    // have any dynamic component class assigned. The interactive load path does this in
    // PCB_EDIT_FRAME::OpenProjectFiles; CLI and API consumers go through here.
    if( !aBoard->SynchronizeComponentClasses( std::unordered_set<wxString>() )
        && aOptions.reporter )
    {
        aOptions.reporter->Report( _( "Could not load component class assignment rules" ),
                                   RPT_SEVERITY_WARNING );
    }

    aBoard->SynchronizeTuningProfileProperties();

    aBoard->UpdateUserUnits( aBoard, nullptr );
}


std::unique_ptr<BOARD> BOARD_LOADER::CreateEmptyBoard( PROJECT* aProject )
{
    std::unique_ptr<BOARD> brd = std::make_unique<BOARD>();
    brd->SetProject( aProject );
    return brd;
}


bool BOARD_LOADER::SaveBoard( wxString& aFileName, BOARD* aBoard, PCB_IO_MGR::PCB_FILE_T aFormat )
{
    aBoard->BuildConnectivity();
    aBoard->SynchronizeNetsAndNetClasses( false );

    try
    {
        PCB_IO_MGR::Save( aFormat, aFileName, aBoard, nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( _( "Cannot save board '%s': %s" ), aFileName, ioe.What() );
        return false;
    }
    catch( const std::exception& e )
    {
        // Rethrow so std::bad_alloc and similar aren't silently turned into a false return.
        wxLogError( _( "Unexpected error saving board '%s': %s" ), aFileName,
                    wxString::FromUTF8( e.what() ) );
        throw;
    }

    return true;
}


bool BOARD_LOADER::SaveBoard( wxString& aFileName, BOARD* aBoard )
{
    return SaveBoard( aFileName, aBoard, PCB_IO_MGR::KICAD_SEXP );
}
