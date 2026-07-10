/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#include <richio.h>
#include <wx/crt.h>
#include <wx/dir.h>
#include <wx/zipstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>

#include <nlohmann/json.hpp>

#include "pcbnew_jobs_handler.h"
#include <board_loader.h>
#include <jobs/scratch_doc.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <diff_merge/project_file_patch.h>
#include <diff_merge/kicad_diff_types.h>
#include <settings/json_settings_internals.h>
#include <drc/drc_engine.h>
#include <board_statistics_report.h>
#include <drc/drc_item.h>
#include <drc/drc_report.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <footprint.h>
#include <jobs/job_fp_export_svg.h>
#include <jobs/job_fp_upgrade.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_export_pcb_ipcd356.h>
#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_export_pcb_gerber.h>
#include <jobs/job_export_pcb_gerbers.h>
#include <jobs/job_export_pcb_hpgl.h>
#include <jobs/job_export_pcb_drill.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_gencad.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_png.h>
#include <jobs/job_export_pcb_pos.h>
#include <jobs/job_export_pcb_ps.h>
#include <jobs/job_export_pcb_stats.h>
#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/job_pcb_render.h>
#include <jobs/job_pcb_drc.h>
#include <jobs/job_pcb_import.h>
#include <jobs/job_import_utils.h>
#include <jobs/job_pcb_upgrade.h>
#include <eda_units.h>
#include <footprint_library_adapter.h>
#include <lset.h>
#include <cli/exit_codes.h>
#include <exporters/place_file_exporter.h>
#include <exporters/step/exporter_step.h>
#include <plotters/plotter_dxf.h>
#include <plotters/plotter_gerber.h>
#include <plotters/plotters_pslike.h>
#include <tool/tool_manager.h>
#include <tools/drc_tool.h>
#include <filename_resolver.h>
#include <gerber_jobfile_writer.h>
#include "gerber_placefile_writer.h"
#include <gendrill_excellon_writer.h>
#include <gendrill_gerber_writer.h>
#include <kiface_base.h>
#include <macros.h>
#include <pad.h>
#include <pcb_marker.h>
#include <project/project_file.h>
#include <exporters/export_gencad_writer.h>
#include <exporters/export_d356.h>
#include <kiface_ids.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/netlist_reader.h>
#include <pcbnew_settings.h>
#include <pcbplot.h>
#include <pcb_plotter.h>
#include <pcb_edit_frame.h>
#include <pcb_track.h>
#include <pgm_base.h>
#include <3d_rendering/raytracing/render_3d_raytrace_ram.h>
#include <3d_rendering/track_ball.h>
#include <project_pcb.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>
#include <reporter.h>
#include <scoped_set_reset.h>
#include <progress_reporter.h>
#include <wildcards_and_files_ext.h>
#include <export_vrml.h>
#include <kiplatform/io.h>
#include <settings/settings_manager.h>
#include <dialogs/dialog_gendrill.h>
#include <dialogs/dialog_gen_footprint_position.h>
#include <dialogs/dialog_export_2581.h>
#include <dialogs/dialog_export_odbpp.h>
#include <dialogs/dialog_export_step.h>
#include <dialogs/dialog_plot.h>
#include <dialogs/dialog_drc_job_config.h>
#include <dialogs/dialog_render_job.h>
#include <dialogs/dialog_gencad_export_options.h>
#include <dialogs/dialog_board_stats_job.h>
#include <paths.h>
#include <tools/zone_filler_tool.h>

#include <locale_io.h>
#include <confirm.h>


#ifdef _WIN32
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif
#endif


PCBNEW_JOBS_HANDLER::PCBNEW_JOBS_HANDLER( KIWAY* aKiway ) :
        JOB_DISPATCHER( aKiway ),
        m_cliBoard( nullptr ),
        m_toolManager( nullptr )
{
    Register( "3d", std::bind( &PCBNEW_JOBS_HANDLER::JobExportStep, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_3D* svgJob = dynamic_cast<JOB_EXPORT_PCB_3D*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( svgJob && editFrame, false );

                  DIALOG_EXPORT_STEP dlg( editFrame, aParent, "", svgJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "render", std::bind( &PCBNEW_JOBS_HANDLER::JobExportRender, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_PCB_RENDER* renderJob = dynamic_cast<JOB_PCB_RENDER*>( job );

                  wxCHECK( renderJob, false );

                  DIALOG_RENDER_JOB dlg( aParent, renderJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "upgrade", std::bind( &PCBNEW_JOBS_HANDLER::JobUpgrade, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "pcb_import", std::bind( &PCBNEW_JOBS_HANDLER::JobImport, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "svg", std::bind( &PCBNEW_JOBS_HANDLER::JobExportSvg, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_SVG* svgJob = dynamic_cast<JOB_EXPORT_PCB_SVG*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( svgJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, svgJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "gencad", std::bind( &PCBNEW_JOBS_HANDLER::JobExportGencad, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_GENCAD* gencadJob = dynamic_cast<JOB_EXPORT_PCB_GENCAD*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( gencadJob && editFrame, false );

                  DIALOG_GENCAD_EXPORT_OPTIONS dlg( editFrame, gencadJob->GetSettingsDialogTitle(), gencadJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "dxf", std::bind( &PCBNEW_JOBS_HANDLER::JobExportDxf, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_DXF* dxfJob = dynamic_cast<JOB_EXPORT_PCB_DXF*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( dxfJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, dxfJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "pdf", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPdf, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_PDF* pdfJob = dynamic_cast<JOB_EXPORT_PCB_PDF*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( pdfJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, pdfJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "png", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPng, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_PNG* pngJob = dynamic_cast<JOB_EXPORT_PCB_PNG*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( pngJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, pngJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "ps", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPs, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_PS* psJob = dynamic_cast<JOB_EXPORT_PCB_PS*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( psJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, psJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "stats", std::bind( &PCBNEW_JOBS_HANDLER::JobExportStats, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_STATS* statsJob = dynamic_cast<JOB_EXPORT_PCB_STATS*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( statsJob && editFrame, false );

                  if( statsJob->m_filename.IsEmpty() && editFrame->GetBoard() )
                  {
                      wxFileName boardName = editFrame->GetBoard()->GetFileName();
                      statsJob->m_filename = boardName.GetFullPath();
                  }

                  wxWindow* parent = aParent ? aParent : static_cast<wxWindow*>( editFrame );

                  DIALOG_BOARD_STATS_JOB dlg( parent, statsJob );

                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "gerber", std::bind( &PCBNEW_JOBS_HANDLER::JobExportGerber, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_GERBER* gJob = dynamic_cast<JOB_EXPORT_PCB_GERBER*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( gJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, gJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "gerbers", std::bind( &PCBNEW_JOBS_HANDLER::JobExportGerbers, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_GERBERS* gJob = dynamic_cast<JOB_EXPORT_PCB_GERBERS*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( gJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, gJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register(
            "hpgl",
            [&]( JOB* aJob )
            {
                m_reporter->Report( _( "Plotting to HPGL is no longer supported as of KiCad 10.0.\n" ),
                                    RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_ARGS;
            },
            [aKiway]( JOB* job, wxWindow* aParent ) -> bool
            {
                PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                wxCHECK( editFrame, false );

                DisplayErrorMessage( editFrame, _( "Plotting to HPGL is no longer supported as of KiCad 10.0." ) );
                return false;
            } );
    Register( "drill", std::bind( &PCBNEW_JOBS_HANDLER::JobExportDrill, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_DRILL* drillJob = dynamic_cast<JOB_EXPORT_PCB_DRILL*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( drillJob && editFrame, false );

                  DIALOG_GENDRILL dlg( editFrame, drillJob, aParent );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "pos", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPos, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_POS* posJob = dynamic_cast<JOB_EXPORT_PCB_POS*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( posJob && editFrame, false );

                  DIALOG_GEN_FOOTPRINT_POSITION dlg( posJob, editFrame, aParent );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "fpupgrade", std::bind( &PCBNEW_JOBS_HANDLER::JobExportFpUpgrade, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "fpsvg", std::bind( &PCBNEW_JOBS_HANDLER::JobExportFpSvg, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "pcb_diff", std::bind( &PCBNEW_JOBS_HANDLER::JobDiff, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "fp_diff", std::bind( &PCBNEW_JOBS_HANDLER::JobFpDiff, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "drc", std::bind( &PCBNEW_JOBS_HANDLER::JobExportDrc, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_PCB_DRC* drcJob = dynamic_cast<JOB_PCB_DRC*>( job );

                  wxCHECK( drcJob, false );

                  DIALOG_DRC_JOB_CONFIG dlg( aParent, drcJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "ipc2581", std::bind( &PCBNEW_JOBS_HANDLER::JobExportIpc2581, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_IPC2581* ipcJob = dynamic_cast<JOB_EXPORT_PCB_IPC2581*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( ipcJob && editFrame, false );

                  DIALOG_EXPORT_2581 dlg( ipcJob, editFrame, aParent );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "ipcd356", std::bind( &PCBNEW_JOBS_HANDLER::JobExportIpcD356, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "odb", std::bind( &PCBNEW_JOBS_HANDLER::JobExportOdb, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_ODB* odbJob = dynamic_cast<JOB_EXPORT_PCB_ODB*>( job );

                  PCB_EDIT_FRAME* editFrame =
                          dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( odbJob && editFrame, false );

                  DIALOG_EXPORT_ODBPP dlg( odbJob, editFrame, aParent );
                  return dlg.ShowModal() == wxID_OK;
              } );
}


PCBNEW_JOBS_HANDLER::~PCBNEW_JOBS_HANDLER()
{
}


void PCBNEW_JOBS_HANDLER::ClearCachedBoard()
{
    m_cliBoard.reset();
    m_toolManager.reset();
}


TOOL_MANAGER* PCBNEW_JOBS_HANDLER::getToolManager( BOARD* aBrd )
{
    TOOL_MANAGER* toolManager = nullptr;
    if( Pgm().IsGUI() )
    {
        // we assume the PCB we are working on here is the one in the frame
        // so use the frame's tool manager
        PCB_EDIT_FRAME* editFrame = (PCB_EDIT_FRAME*) m_kiway->Player( FRAME_PCB_EDITOR, false );
        if( editFrame )
            toolManager = editFrame->GetToolManager();
    }
    else
    {
        if( m_toolManager == nullptr )
        {
            m_toolManager = std::make_unique<TOOL_MANAGER>();
        }

        toolManager = m_toolManager.get();

        toolManager->SetEnvironment( aBrd, nullptr, nullptr, Kiface().KifaceSettings(), nullptr );
    }
    return toolManager;
}


BOARD* PCBNEW_JOBS_HANDLER::getBoard( const wxString& aPath )
{
    BOARD*            brd = nullptr;
    SETTINGS_MANAGER& settingsManager = Pgm().GetSettingsManager();
    wxString loadError;

    auto getProjectForBoard = [&]( const wxString& aBoardPath ) -> PROJECT*
    {
        wxFileName pro = aBoardPath;
        pro.SetExt( FILEEXT::ProjectFileExtension );
        pro.MakeAbsolute();

        PROJECT* project = settingsManager.GetProject( pro.GetFullPath() );

        if( !project )
        {
            settingsManager.LoadProject( pro.GetFullPath(), true );
            project = settingsManager.GetProject( pro.GetFullPath() );
        }

        return project;
    };

    auto loadBoardFromPath = [&]( const wxString& aBoardPath ) -> BOARD*
    {
        PROJECT* project = getProjectForBoard( aBoardPath );

        PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::FindPluginTypeFromBoardPath( aBoardPath, KICTL_KICAD_ONLY );

        if( !project || pluginType == PCB_IO_MGR::FILE_TYPE_NONE )
            return nullptr;

        try
        {
            std::unique_ptr<BOARD> loadedBoard = BOARD_LOADER::Load( aBoardPath, pluginType, project );
            return loadedBoard.release();
        }
        catch( const IO_ERROR& ioe )
        {
            loadError = ioe.What();
            return nullptr;
        }
        catch( ... )
        {
            return nullptr;
        }
    };

    if( !Pgm().IsGUI() && Pgm().GetSettingsManager().IsProjectOpen() )
    {
        wxString pcbPath = aPath;

        if( pcbPath.IsEmpty() )
        {
            wxFileName path = Pgm().GetSettingsManager().Prj().GetProjectFullName();
            path.SetExt( FILEEXT::KiCadPcbFileExtension );
            path.MakeAbsolute();
            pcbPath = path.GetFullPath();
        }

        if( !m_cliBoard )
            m_cliBoard.reset( loadBoardFromPath( pcbPath ) );

        brd = m_cliBoard.get();
    }
    else if( Pgm().IsGUI() && Pgm().GetSettingsManager().IsProjectOpen() )
    {
        PCB_EDIT_FRAME* editFrame = (PCB_EDIT_FRAME*) m_kiway->Player( FRAME_PCB_EDITOR, false );

        if( editFrame )
            brd = editFrame->GetBoard();
    }
    else
    {
        m_cliBoard.reset( loadBoardFromPath( aPath ) );
        brd = m_cliBoard.get();
    }

    if( !brd )
    {
        wxString msg = _( "Failed to load board" );

        if( !loadError.IsEmpty() )
            msg += wxString::Format( wxS( ": %s" ), loadError );

        m_reporter->Report( msg + '\n', RPT_SEVERITY_ERROR );
    }

    return brd;
}


LSEQ PCBNEW_JOBS_HANDLER::convertLayerArg( wxString& aLayerString, BOARD* aBoard ) const
{
    std::map<wxString, LSET> layerUserMasks;
    std::map<wxString, LSET> layerMasks;
    std::map<wxString, LSET> layerGuiMasks;

    // Build list of layer names and their layer mask:
    for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
    {
        // Add user layer name
        if( aBoard )
            layerUserMasks[aBoard->GetLayerName( layer )] = LSET( { layer } );

        // Add layer name used in pcb files
        layerMasks[LSET::Name( layer )] = LSET( { layer } );
        // Add layer name using GUI canonical layer name
        layerGuiMasks[LayerName( layer )] = LSET( { layer } );
    }

    // Add list of grouped layer names used in pcb files
    layerMasks[wxT( "*" )] = LSET::AllLayersMask();
    layerMasks[wxT( "*.Cu" )] = LSET::AllCuMask();
    layerMasks[wxT( "*In.Cu" )] = LSET::InternalCuMask();
    layerMasks[wxT( "F&B.Cu" )] = LSET( { F_Cu, B_Cu } );
    layerMasks[wxT( "*.Adhes" )] = LSET( { B_Adhes, F_Adhes } );
    layerMasks[wxT( "*.Paste" )] = LSET( { B_Paste, F_Paste } );
    layerMasks[wxT( "*.Mask" )] = LSET( { B_Mask, F_Mask } );
    layerMasks[wxT( "*.SilkS" )] = LSET( { B_SilkS, F_SilkS } );
    layerMasks[wxT( "*.Fab" )] = LSET( { B_Fab, F_Fab } );
    layerMasks[wxT( "*.CrtYd" )] = LSET( { B_CrtYd, F_CrtYd } );

    // Add list of grouped layer names using GUI canonical layer names
    layerGuiMasks[wxT( "*.Adhesive" )] = LSET( { B_Adhes, F_Adhes } );
    layerGuiMasks[wxT( "*.Silkscreen" )] = LSET( { B_SilkS, F_SilkS } );
    layerGuiMasks[wxT( "*.Courtyard" )] = LSET( { B_CrtYd, F_CrtYd } );

    LSEQ layerMask;

    auto pushLayers = [&]( const LSET& layerSet )
    {
        for( PCB_LAYER_ID layer : layerSet.Seq() )
            layerMask.push_back( layer );
    };

    if( !aLayerString.IsEmpty() )
    {
        wxStringTokenizer layerTokens( aLayerString, "," );

        while( layerTokens.HasMoreTokens() )
        {
            std::string token = TO_UTF8( layerTokens.GetNextToken().Trim( true ).Trim( false ) );

            if( layerUserMasks.contains( token ) )
                pushLayers( layerUserMasks.at( token ) );
            else if( layerMasks.count( token ) )
                pushLayers( layerMasks.at( token ) );
            else if( layerGuiMasks.count( token ) )
                pushLayers( layerGuiMasks.at( token ) );
            else
                m_reporter->Report( wxString::Format( _( "Invalid layer name '%s'\n" ), token ) );
        }
    }

    return layerMask;
}


int PCBNEW_JOBS_HANDLER::JobExportStep( JOB* aJob )
{
    JOB_EXPORT_PCB_3D* aStepJob = dynamic_cast<JOB_EXPORT_PCB_3D*>( aJob );

    if( aStepJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aStepJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !aStepJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( aStepJob->m_variant );

    if( aStepJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );

        switch( aStepJob->m_format )
        {
        case JOB_EXPORT_PCB_3D::FORMAT::VRML: fn.SetExt( FILEEXT::VrmlFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::STEP: fn.SetExt( FILEEXT::StepFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::BREP: fn.SetExt( FILEEXT::BrepFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::XAO: fn.SetExt( FILEEXT::XaoFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::GLB: fn.SetExt( FILEEXT::GltfBinaryFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::PLY: fn.SetExt( FILEEXT::PlyFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::STL: fn.SetExt( FILEEXT::StlFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::U3D: fn.SetExt( FILEEXT::U3DFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::PDF: fn.SetExt( FILEEXT::PdfFileExtension ); break;
        default:
            m_reporter->Report( _( "Unknown export format" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN; // shouldnt have gotten here
        }

        aStepJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( aStepJob->m_format == JOB_EXPORT_PCB_3D::FORMAT::VRML )
    {
        double scale = 0.0;
        switch( aStepJob->m_vrmlUnits )
        {
        case JOB_EXPORT_PCB_3D::VRML_UNITS::MM: scale = 1.0; break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::METERS: scale = 0.001; break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::TENTHS: scale = 10.0 / 25.4; break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::INCH: scale = 1.0 / 25.4; break;
        }

        EXPORTER_VRML vrmlExporter( brd );
        wxString      messages;

        double originX = pcbIUScale.IUTomm( aStepJob->m_3dparams.m_Origin.x );
        double originY = pcbIUScale.IUTomm( aStepJob->m_3dparams.m_Origin.y );

        if( !aStepJob->m_hasUserOrigin )
        {
            BOX2I bbox = brd->ComputeBoundingBox( true, true );
            originX = pcbIUScale.IUTomm( bbox.GetCenter().x );
            originY = pcbIUScale.IUTomm( bbox.GetCenter().y );
        }

        bool success = vrmlExporter.ExportVRML_File(
                brd->GetProject(), &messages, outPath, scale, aStepJob->m_3dparams.m_IncludeUnspecified,
                aStepJob->m_3dparams.m_IncludeDNP, !aStepJob->m_vrmlModelDir.IsEmpty(), aStepJob->m_vrmlRelativePaths,
                aStepJob->m_vrmlModelDir, originX, originY );

        if( success )
        {
            m_reporter->Report( wxString::Format( _( "Successfully exported VRML to %s" ), outPath ),
                                RPT_SEVERITY_INFO );
        }
        else
        {
            m_reporter->Report( _( "Error exporting VRML" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
    }
    else
    {
        EXPORTER_STEP_PARAMS params = aStepJob->m_3dparams;

        switch( aStepJob->m_format )
        {
        case JOB_EXPORT_PCB_3D::FORMAT::STEP: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::STEP; break;
        case JOB_EXPORT_PCB_3D::FORMAT::STEPZ: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::STEPZ; break;
        case JOB_EXPORT_PCB_3D::FORMAT::BREP: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::BREP; break;
        case JOB_EXPORT_PCB_3D::FORMAT::XAO: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::XAO; break;
        case JOB_EXPORT_PCB_3D::FORMAT::GLB: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::GLB; break;
        case JOB_EXPORT_PCB_3D::FORMAT::PLY: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::PLY; break;
        case JOB_EXPORT_PCB_3D::FORMAT::STL: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::STL; break;
        case JOB_EXPORT_PCB_3D::FORMAT::U3D: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::U3D; break;
        case JOB_EXPORT_PCB_3D::FORMAT::PDF: params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::PDF; break;
        default:
            m_reporter->Report( _( "Unknown export format" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN; // shouldnt have gotten here
        }

        EXPORTER_STEP stepExporter( brd, params, m_reporter );
        stepExporter.m_outputFile = aStepJob->GetFullOutputPath( brd->GetProject() );

        if( !stepExporter.Export() )
            return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportRender( JOB* aJob )
{
    JOB_PCB_RENDER* aRenderJob = dynamic_cast<JOB_PCB_RENDER*>( aJob );

    if( aRenderJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    // Reject width and height being invalid
    // Final bit of sanity because this can blow things up
    if( aRenderJob->m_width <= 0 || aRenderJob->m_height <= 0 )
    {
        m_reporter->Report( _( "Invalid image dimensions" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    BOARD* brd = getBoard( aRenderJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !aRenderJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( aRenderJob->m_variant );

    if( aRenderJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();

        switch( aRenderJob->m_format )
        {
        case JOB_PCB_RENDER::FORMAT::JPEG: fn.SetExt( FILEEXT::JpegFileExtension ); break;
        case JOB_PCB_RENDER::FORMAT::PNG: fn.SetExt( FILEEXT::PngFileExtension ); break;
        default:
            m_reporter->Report( _( "Unknown export format" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN; // shouldnt have gotten here
        }

        // set the name to board name + "side", its lazy but its hard to generate anything truely unique
        // incase someone is doing this in a jobset with multiple jobs, they should be setting the output themselves
        // or we do a hash based on all the options
        fn.SetName( wxString::Format( "%s-%d", fn.GetName(), static_cast<int>( aRenderJob->m_side ) ) );

        aRenderJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    BOARD_ADAPTER boardAdapter;

    boardAdapter.SetBoard( brd );
    boardAdapter.m_IsBoardView = false;

    EDA_3D_VIEWER_SETTINGS cfg;

    if( EDA_3D_VIEWER_SETTINGS* userCfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        cfg.m_Render = userCfg->m_Render;
        cfg.m_Camera = userCfg->m_Camera;
        cfg.m_LayerPresets = userCfg->m_LayerPresets;
    }

    if( aRenderJob->m_appearancePreset.empty() )
    {
        // Force display 3D models
        cfg.m_Render.show_footprints_not_in_posfile = true;
        cfg.m_Render.show_footprints_dnp = true;
        cfg.m_Render.show_footprints_insert = true;
        cfg.m_Render.show_footprints_normal = true;
        cfg.m_Render.show_footprints_virtual = true;
    }

    if( aRenderJob->m_quality == JOB_PCB_RENDER::QUALITY::BASIC )
    {
        // Silkscreen is pixelated without antialiasing
        cfg.m_Render.raytrace_anti_aliasing = true;

        cfg.m_Render.raytrace_backfloor = aRenderJob->m_floor;
        cfg.m_Render.raytrace_post_processing = aRenderJob->m_floor;

        cfg.m_Render.raytrace_procedural_textures = false;
        cfg.m_Render.raytrace_reflections = false;
        cfg.m_Render.raytrace_shadows = aRenderJob->m_floor;

        // Better colors
        cfg.m_Render.differentiate_plated_copper = true;

        // Tracks below soldermask are not visible without refractions
        cfg.m_Render.raytrace_refractions = true;
        cfg.m_Render.raytrace_recursivelevel_refractions = 1;
    }
    else if( aRenderJob->m_quality == JOB_PCB_RENDER::QUALITY::HIGH )
    {
        cfg.m_Render.raytrace_anti_aliasing = true;
        cfg.m_Render.raytrace_backfloor = true;
        cfg.m_Render.raytrace_post_processing = true;
        cfg.m_Render.raytrace_procedural_textures = true;
        cfg.m_Render.raytrace_reflections = true;
        cfg.m_Render.raytrace_shadows = true;
        cfg.m_Render.raytrace_refractions = true;
        cfg.m_Render.differentiate_plated_copper = true;
    }
    else if( aRenderJob->m_quality == JOB_PCB_RENDER::QUALITY::JOB_SETTINGS )
    {
        cfg.m_Render.raytrace_anti_aliasing = aRenderJob->m_antiAlias;
        cfg.m_Render.raytrace_backfloor = aRenderJob->m_floor;
        cfg.m_Render.raytrace_post_processing = aRenderJob->m_postProcess;
        cfg.m_Render.raytrace_procedural_textures = aRenderJob->m_proceduralTextures;
    }

    cfg.m_Render.raytrace_lightColorTop = COLOR4D( aRenderJob->m_lightTopIntensity.x, aRenderJob->m_lightTopIntensity.y,
                                                   aRenderJob->m_lightTopIntensity.z, 1.0 );

    cfg.m_Render.raytrace_lightColorBottom =
            COLOR4D( aRenderJob->m_lightBottomIntensity.x, aRenderJob->m_lightBottomIntensity.y,
                     aRenderJob->m_lightBottomIntensity.z, 1.0 );

    cfg.m_Render.raytrace_lightColorCamera =
            COLOR4D( aRenderJob->m_lightCameraIntensity.x, aRenderJob->m_lightCameraIntensity.y,
                     aRenderJob->m_lightCameraIntensity.z, 1.0 );

    COLOR4D lightColor( aRenderJob->m_lightSideIntensity.x, aRenderJob->m_lightSideIntensity.y,
                        aRenderJob->m_lightSideIntensity.z, 1.0 );

    cfg.m_Render.raytrace_lightColor = {
        lightColor, lightColor, lightColor, lightColor, lightColor, lightColor, lightColor, lightColor,
    };

    int sideElevation = aRenderJob->m_lightSideElevation;

    cfg.m_Render.raytrace_lightElevation = {
        sideElevation,  sideElevation,  sideElevation,  sideElevation,
        -sideElevation, -sideElevation, -sideElevation, -sideElevation,
    };

    cfg.m_Render.raytrace_lightAzimuth = {
        45, 135, 225, 315, 45, 135, 225, 315,
    };

    cfg.m_CurrentPreset = aRenderJob->m_appearancePreset;
    cfg.m_UseStackupColors = aRenderJob->m_useBoardStackupColors;
    boardAdapter.m_Cfg = &cfg;

    // Apply the preset's layer visibility and colors to the render settings
    if( !aRenderJob->m_appearancePreset.empty() )
    {
        wxString presetName = wxString::FromUTF8( aRenderJob->m_appearancePreset );

        if( presetName == FOLLOW_PCB || presetName == FOLLOW_PLOT_SETTINGS )
        {
            boardAdapter.SetVisibleLayers( boardAdapter.GetVisibleLayers() );
        }
        else if( LAYER_PRESET_3D* preset = cfg.FindPreset( presetName ) )
        {
            boardAdapter.SetVisibleLayers( preset->layers );
            boardAdapter.SetLayerColors( preset->colors );

            if( preset->name.Lower() == _( "legacy colors" ) )
                cfg.m_UseStackupColors = false;
        }
    }

    if( aRenderJob->m_bgStyle == JOB_PCB_RENDER::BG_STYLE::TRANSPARENT
        || ( aRenderJob->m_bgStyle == JOB_PCB_RENDER::BG_STYLE::DEFAULT
             && aRenderJob->m_format == JOB_PCB_RENDER::FORMAT::PNG ) )
    {
        boardAdapter.m_ColorOverrides[LAYER_3D_BACKGROUND_TOP] = COLOR4D( 1.0, 1.0, 1.0, 0.0 );
        boardAdapter.m_ColorOverrides[LAYER_3D_BACKGROUND_BOTTOM] = COLOR4D( 1.0, 1.0, 1.0, 0.0 );
    }

    boardAdapter.Set3dCacheManager( PROJECT_PCB::Get3DCacheManager( brd->GetProject() ) );

    static std::map<JOB_PCB_RENDER::SIDE, VIEW3D_TYPE> s_viewCmdMap = {
        { JOB_PCB_RENDER::SIDE::TOP, VIEW3D_TYPE::VIEW3D_TOP },
        { JOB_PCB_RENDER::SIDE::BOTTOM, VIEW3D_TYPE::VIEW3D_BOTTOM },
        { JOB_PCB_RENDER::SIDE::LEFT, VIEW3D_TYPE::VIEW3D_LEFT },
        { JOB_PCB_RENDER::SIDE::RIGHT, VIEW3D_TYPE::VIEW3D_RIGHT },
        { JOB_PCB_RENDER::SIDE::FRONT, VIEW3D_TYPE::VIEW3D_FRONT },
        { JOB_PCB_RENDER::SIDE::BACK, VIEW3D_TYPE::VIEW3D_BACK },
    };

    PROJECTION_TYPE projection = aRenderJob->m_perspective ? PROJECTION_TYPE::PERSPECTIVE : PROJECTION_TYPE::ORTHO;

    wxSize     windowSize( aRenderJob->m_width, aRenderJob->m_height );
    TRACK_BALL camera( 2 * RANGE_SCALE_3D );

    camera.SetProjection( projection );
    camera.SetCurWindowSize( windowSize );

    RENDER_3D_RAYTRACE_RAM raytrace( boardAdapter, camera );
    raytrace.SetCurWindowSize( windowSize );

    for( bool first = true; raytrace.Redraw( false, m_reporter, m_reporter ); first = false )
    {
        if( first )
        {
            const float cmTo3D = boardAdapter.BiuTo3dUnits() * pcbIUScale.mmToIU( 10.0 );

            // First redraw resets lookat point to the board center, so set up the camera here
            camera.ViewCommand_T1( s_viewCmdMap[aRenderJob->m_side] );

            camera.SetLookAtPos_T1( camera.GetLookAtPos_T1()
                                    + SFVEC3F( aRenderJob->m_pivot.x, aRenderJob->m_pivot.y, aRenderJob->m_pivot.z )
                                              * cmTo3D );

            camera.Pan_T1( SFVEC3F( aRenderJob->m_pan.x, aRenderJob->m_pan.y, aRenderJob->m_pan.z ) );

            camera.Zoom_T1( aRenderJob->m_zoom );

            camera.RotateX_T1( DEG2RAD( aRenderJob->m_rotation.x ) );
            camera.RotateY_T1( DEG2RAD( aRenderJob->m_rotation.y ) );
            camera.RotateZ_T1( DEG2RAD( aRenderJob->m_rotation.z ) );

            camera.Interpolate( 1.0f );
            camera.SetT0_and_T1_current_T();
            camera.ParametersChanged();
        }
    }

    uint8_t* rgbaBuffer = raytrace.GetBuffer();
    wxSize   realSize = raytrace.GetRealBufferSize();
    bool     success = !!rgbaBuffer;

    if( rgbaBuffer )
    {
        const unsigned int wxh = realSize.x * realSize.y;

        unsigned char* rgbBuffer = (unsigned char*) malloc( wxh * 3 );
        unsigned char* alphaBuffer = (unsigned char*) malloc( wxh );

        unsigned char* rgbaPtr = rgbaBuffer;
        unsigned char* rgbPtr = rgbBuffer;
        unsigned char* alphaPtr = alphaBuffer;

        for( int y = 0; y < realSize.y; y++ )
        {
            for( int x = 0; x < realSize.x; x++ )
            {
                rgbPtr[0] = rgbaPtr[0];
                rgbPtr[1] = rgbaPtr[1];
                rgbPtr[2] = rgbaPtr[2];
                alphaPtr[0] = rgbaPtr[3];

                rgbaPtr += 4;
                rgbPtr += 3;
                alphaPtr += 1;
            }
        }

        wxImage image( realSize );
        image.SetData( rgbBuffer );
        image.SetAlpha( alphaBuffer );
        image = image.Mirror( false );

        image.SetOption( wxIMAGE_OPTION_QUALITY, 90 );
        image.SaveFile( outPath,
                        aRenderJob->m_format == JOB_PCB_RENDER::FORMAT::PNG ? wxBITMAP_TYPE_PNG : wxBITMAP_TYPE_JPEG );
    }

    if( success )
    {
        m_reporter->Report( _( "Successfully created 3D render image" ) + wxS( "\n" ), RPT_SEVERITY_INFO );
        return CLI::EXIT_CODES::OK;
    }
    else
    {
        m_reporter->Report( _( "Error creating 3D render image" ) + wxS( "\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }
}


int PCBNEW_JOBS_HANDLER::JobExportSvg( JOB* aJob )
{
    JOB_EXPORT_PCB_SVG* aSvgJob = dynamic_cast<JOB_EXPORT_PCB_SVG*>( aJob );

    if( aSvgJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD*        brd = getBoard( aSvgJob->m_filename );
    TOOL_MANAGER* toolManager = getToolManager( brd );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !aSvgJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( aSvgJob->m_variant );

    if( aSvgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE )
    {
        if( aSvgJob->GetConfiguredOutputPath().IsEmpty() )
        {
            wxFileName fn = brd->GetFileName();
            fn.SetName( fn.GetName() );
            fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::SVG ) );

            aSvgJob->SetWorkingOutputPath( fn.GetFullName() );
        }
    }

    wxString outPath = resolveJobOutputPath( aJob, brd, &aSvgJob->m_drawingSheet );

    if( !PATHS::EnsurePathExists( outPath, aSvgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( aSvgJob->m_checkZonesBeforePlot )
    {
        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    if( aSvgJob->m_argLayers )
        aSvgJob->m_plotLayerSequence = convertLayerArg( aSvgJob->m_argLayers.value(), brd );

    if( aSvgJob->m_argCommonLayers )
        aSvgJob->m_plotOnAllLayersSequence = convertLayerArg( aSvgJob->m_argCommonLayers.value(), brd );

    if( aSvgJob->m_plotLayerSequence.size() < 1 )
    {
        m_reporter->Report( _( "At least one layer must be specified\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, aSvgJob, *m_reporter );

    PCB_PLOTTER plotter( brd, m_reporter, plotOpts );

    std::optional<wxString> layerName;
    std::optional<wxString> sheetName;
    std::optional<wxString> sheetPath;
    std::vector<wxString>   outputPaths;

    if( aSvgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE )
    {
        if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
            layerName = aSvgJob->GetVarOverrides().at( wxT( "LAYER" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
            sheetName = aSvgJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
            sheetPath = aSvgJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );
    }

    if( !plotter.Plot( outPath, aSvgJob->m_plotLayerSequence, aSvgJob->m_plotOnAllLayersSequence, false,
                       aSvgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE, layerName, sheetName, sheetPath,
                       &outputPaths ) )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    for( const wxString& outputPath : outputPaths )
        aSvgJob->AddOutput( outputPath );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportDxf( JOB* aJob )
{
    JOB_EXPORT_PCB_DXF* aDxfJob = dynamic_cast<JOB_EXPORT_PCB_DXF*>( aJob );

    if( aDxfJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aDxfJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !aDxfJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( aDxfJob->m_variant );

    TOOL_MANAGER* toolManager = getToolManager( brd );

    if( aDxfJob->m_checkZonesBeforePlot )
    {
        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    if( aDxfJob->m_argLayers )
        aDxfJob->m_plotLayerSequence = convertLayerArg( aDxfJob->m_argLayers.value(), brd );

    if( aDxfJob->m_argCommonLayers )
        aDxfJob->m_plotOnAllLayersSequence = convertLayerArg( aDxfJob->m_argCommonLayers.value(), brd );

    if( aDxfJob->m_plotLayerSequence.size() < 1 )
    {
        m_reporter->Report( _( "At least one layer must be specified\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    if( aDxfJob->m_genMode == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE )
    {
        if( aDxfJob->GetConfiguredOutputPath().IsEmpty() )
        {
            wxFileName fn = brd->GetFileName();
            fn.SetName( fn.GetName() );
            fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::DXF ) );

            aDxfJob->SetWorkingOutputPath( fn.GetFullName() );
        }
    }

    wxString outPath = resolveJobOutputPath( aJob, brd, &aDxfJob->m_drawingSheet );

    if( !PATHS::EnsurePathExists( outPath, aDxfJob->m_genMode == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, aDxfJob, *m_reporter );

    PCB_PLOTTER plotter( brd, m_reporter, plotOpts );

    std::optional<wxString> layerName;
    std::optional<wxString> sheetName;
    std::optional<wxString> sheetPath;

    if( aDxfJob->m_genMode == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE )
    {
        if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
            layerName = aDxfJob->GetVarOverrides().at( wxT( "LAYER" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
            sheetName = aDxfJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
            sheetPath = aDxfJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );
    }

    std::vector<wxString> outputPaths;

    if( !plotter.Plot( outPath, aDxfJob->m_plotLayerSequence, aDxfJob->m_plotOnAllLayersSequence, false,
                       aDxfJob->m_genMode == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE, layerName, sheetName, sheetPath,
                       &outputPaths ) )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    for( const wxString& outputPath : outputPaths )
        aJob->AddOutput( outputPath );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportPdf( JOB* aJob )
{
    bool                plotAllLayersOneFile = false;
    JOB_EXPORT_PCB_PDF* pdfJob = dynamic_cast<JOB_EXPORT_PCB_PDF*>( aJob );

    if( pdfJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( pdfJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !pdfJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( pdfJob->m_variant );

    TOOL_MANAGER* toolManager = getToolManager( brd );

    if( pdfJob->m_checkZonesBeforePlot )
    {
        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    if( pdfJob->m_argLayers )
        pdfJob->m_plotLayerSequence = convertLayerArg( pdfJob->m_argLayers.value(), brd );

    if( pdfJob->m_argCommonLayers )
        pdfJob->m_plotOnAllLayersSequence = convertLayerArg( pdfJob->m_argCommonLayers.value(), brd );

    if( pdfJob->m_pdfGenMode == JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE )
        plotAllLayersOneFile = true;

    if( pdfJob->m_plotLayerSequence.size() < 1 )
    {
        m_reporter->Report( _( "At least one layer must be specified\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    const bool outputIsSingle = plotAllLayersOneFile || pdfJob->m_pdfSingle;

    if( outputIsSingle && pdfJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::PDF ) );

        pdfJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( pdfJob, brd, &pdfJob->m_drawingSheet );

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, pdfJob, *m_reporter );

    PCB_PLOTTER pcbPlotter( brd, m_reporter, plotOpts );

    if( !PATHS::EnsurePathExists( outPath, outputIsSingle ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    std::optional<wxString> layerName;
    std::optional<wxString> sheetName;
    std::optional<wxString> sheetPath;

    if( plotAllLayersOneFile )
    {
        if( pdfJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
            layerName = pdfJob->GetVarOverrides().at( wxT( "LAYER" ) );

        if( pdfJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
            sheetName = pdfJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

        if( pdfJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
            sheetPath = pdfJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );
    }

    std::vector<wxString> outputPaths;

    if( !pcbPlotter.Plot( outPath, pdfJob->m_plotLayerSequence, pdfJob->m_plotOnAllLayersSequence, false,
                          outputIsSingle, layerName, sheetName, sheetPath, &outputPaths ) )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    for( const wxString& outputPath : outputPaths )
        aJob->AddOutput( outputPath );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportPng( JOB* aJob )
{
    JOB_EXPORT_PCB_PNG* pngJob = dynamic_cast<JOB_EXPORT_PCB_PNG*>( aJob );

    if( pngJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( pngJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !pngJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( pngJob->m_variant );

    TOOL_MANAGER* toolManager = getToolManager( brd );

    if( pngJob->m_checkZonesBeforePlot )
    {
        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    if( pngJob->m_argLayers )
        pngJob->m_plotLayerSequence = convertLayerArg( pngJob->m_argLayers.value(), brd );

    if( pngJob->m_argCommonLayers )
        pngJob->m_plotOnAllLayersSequence = convertLayerArg( pngJob->m_argCommonLayers.value(), brd );

    if( pngJob->m_plotLayerSequence.size() < 1 )
    {
        m_reporter->Report( _( "At least one layer must be specified\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    if( pngJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::PNG ) );

        pngJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( pngJob, brd, &pngJob->m_drawingSheet );

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, pngJob, *m_reporter );

    PCB_PLOTTER pcbPlotter( brd, m_reporter, plotOpts );

    if( !PATHS::EnsurePathExists( outPath, false ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    std::vector<wxString> outputPaths;

    if( !pcbPlotter.Plot( outPath, pngJob->m_plotLayerSequence, pngJob->m_plotOnAllLayersSequence, false, false,
                          std::nullopt, std::nullopt, std::nullopt, &outputPaths ) )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    for( const wxString& outputPath : outputPaths )
        aJob->AddOutput( outputPath );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportPs( JOB* aJob )
{
    JOB_EXPORT_PCB_PS* psJob = dynamic_cast<JOB_EXPORT_PCB_PS*>( aJob );

    if( psJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( psJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !psJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( psJob->m_variant );

    TOOL_MANAGER* toolManager = getToolManager( brd );

    if( psJob->m_checkZonesBeforePlot )
    {
        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    if( psJob->m_argLayers )
        psJob->m_plotLayerSequence = convertLayerArg( psJob->m_argLayers.value(), brd );

    if( psJob->m_argCommonLayers )
        psJob->m_plotOnAllLayersSequence = convertLayerArg( psJob->m_argCommonLayers.value(), brd );

    if( psJob->m_plotLayerSequence.size() < 1 )
    {
        m_reporter->Report( _( "At least one layer must be specified\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    bool isSingle = psJob->m_genMode == JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE;

    if( isSingle )
    {
        if( psJob->GetConfiguredOutputPath().IsEmpty() )
        {
            wxFileName fn = brd->GetFileName();
            fn.SetName( fn.GetName() );
            fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::POST ) );

            psJob->SetWorkingOutputPath( fn.GetFullName() );
        }
    }

    wxString outPath = resolveJobOutputPath( psJob, brd, &psJob->m_drawingSheet );

    if( !PATHS::EnsurePathExists( outPath, isSingle ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, psJob, *m_reporter );

    PCB_PLOTTER pcbPlotter( brd, m_reporter, plotOpts );

    std::optional<wxString> layerName;
    std::optional<wxString> sheetName;
    std::optional<wxString> sheetPath;

    if( isSingle )
    {
        if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
            layerName = psJob->GetVarOverrides().at( wxT( "LAYER" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
            sheetName = psJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
            sheetPath = psJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );
    }

    std::vector<wxString> outputPaths;

    if( !pcbPlotter.Plot( outPath, psJob->m_plotLayerSequence, psJob->m_plotOnAllLayersSequence, false, isSingle,
                          layerName, sheetName, sheetPath, &outputPaths ) )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    for( const wxString& outputPath : outputPaths )
        aJob->AddOutput( outputPath );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportGerbers( JOB* aJob )
{
    int                     exitCode = CLI::EXIT_CODES::OK;
    JOB_EXPORT_PCB_GERBERS* aGerberJob = dynamic_cast<JOB_EXPORT_PCB_GERBERS*>( aJob );

    if( aGerberJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aGerberJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !aGerberJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( aGerberJob->m_variant );

    wxString outPath = resolveJobOutputPath( aJob, brd, &aGerberJob->m_drawingSheet );

    if( !PATHS::EnsurePathExists( outPath, false ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    TOOL_MANAGER* toolManager = getToolManager( brd );

    if( aGerberJob->m_checkZonesBeforePlot )
    {
        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    bool hasLayerListSpecified = false; // will be true if the user layer list is not empty

    if( aGerberJob->m_argLayers )
    {
        if( !aGerberJob->m_argLayers.value().empty() )
        {
            aGerberJob->m_plotLayerSequence = convertLayerArg( aGerberJob->m_argLayers.value(), brd );
            hasLayerListSpecified = true;
        }
        else
        {
            aGerberJob->m_plotLayerSequence = LSET::AllLayersMask().SeqStackupForPlotting();
        }
    }

    if( aGerberJob->m_argCommonLayers )
        aGerberJob->m_plotOnAllLayersSequence = convertLayerArg( aGerberJob->m_argCommonLayers.value(), brd );

    PCB_PLOT_PARAMS       boardPlotOptions = brd->GetPlotOptions();
    GERBER_JOBFILE_WRITER jobfile_writer( brd );

    wxString fileExt;

    if( aGerberJob->m_useBoardPlotParams )
    {
        // The board plot options are saved with all copper layers enabled, even those that don't
        // exist in the current stackup. This is done so the layers are automatically enabled in the plot
        // dialog when the user enables them. We need to filter out these not-enabled layers here so
        // we don't plot 32 layers when we only have 4, etc.
        LSET plotLayers = ( boardPlotOptions.GetLayerSelection() & LSET::AllNonCuMask() )
                          | ( brd->GetEnabledLayers() & LSET::AllCuMask() );
        aGerberJob->m_plotLayerSequence = plotLayers.SeqStackupForPlotting();
        aGerberJob->m_plotOnAllLayersSequence = boardPlotOptions.GetPlotOnAllLayersSequence();
    }
    else
    {
        // default to the board enabled layers, but only if the user has not specifed a layer list
        // ( m_plotLayerSequence can be empty with a broken user layer list)
        if( aGerberJob->m_plotLayerSequence.empty() && !hasLayerListSpecified )
            aGerberJob->m_plotLayerSequence = brd->GetEnabledLayers().SeqStackupForPlotting();
    }

    // Ensure layers to plot are restricted to enabled layers of the board to plot
    LSET layersToPlot = LSET( { aGerberJob->m_plotLayerSequence } ) & brd->GetEnabledLayers();

    for( PCB_LAYER_ID layer : layersToPlot.UIOrder() )
    {
        LSEQ plotSequence;

        // Base layer always gets plotted first.
        plotSequence.push_back( layer );

        // Now all the "include on all" layers
        for( PCB_LAYER_ID layer_all : aGerberJob->m_plotOnAllLayersSequence )
        {
            // Don't plot the same layer more than once;
            if( find( plotSequence.begin(), plotSequence.end(), layer_all ) != plotSequence.end() )
                continue;

            plotSequence.push_back( layer_all );
        }

        // Pick the basename from the board file
        wxFileName      fn( brd->GetFileName() );
        wxString        layerName = brd->GetLayerName( layer );
        wxString        sheetName;
        wxString        sheetPath;
        PCB_PLOT_PARAMS plotOpts;

        if( aGerberJob->m_useBoardPlotParams )
            plotOpts = boardPlotOptions;
        else
            PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, aGerberJob, *m_reporter );

        if( plotOpts.GetUseGerberProtelExtensions() )
            fileExt = GetGerberProtelExtension( layer );
        else
            fileExt = FILEEXT::GerberFileExtension;

        PCB_PLOTTER::BuildPlotFileName( &fn, outPath, layerName, fileExt );
        wxString fullname = fn.GetFullName();

        if( m_progressReporter )
        {
            m_progressReporter->AdvancePhase( wxString::Format( _( "Exporting %s" ), fullname ) );
            m_progressReporter->KeepRefreshing();
        }

        jobfile_writer.AddGbrFile( layer, fullname );

        if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
            layerName = aJob->GetVarOverrides().at( wxT( "LAYER" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
            sheetName = aJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
            sheetPath = aJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );

        // We are feeding it one layer at the start here to silence a logic check
        GERBER_PLOTTER* plotter;
        plotter = (GERBER_PLOTTER*) StartPlotBoard( brd, &plotOpts, layer, layerName, fn.GetFullPath(), sheetName,
                                                    sheetPath );

        if( plotter )
        {
            m_reporter->Report( wxString::Format( _( "Plotted to '%s'.\n" ), fn.GetFullPath() ), RPT_SEVERITY_ACTION );

            PlotBoardLayers( brd, plotter, plotSequence, plotOpts );
            plotter->EndPlot();
            aJob->AddOutput( fn.GetFullPath() );
        }
        else
        {
            m_reporter->Report( wxString::Format( _( "Failed to plot to '%s'.\n" ), fn.GetFullPath() ),
                                RPT_SEVERITY_ERROR );
            exitCode = CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }

        delete plotter;
    }

    if( aGerberJob->m_createJobsFile )
    {
        wxFileName fn( brd->GetFileName() );

        // Build gerber job file from basename
        PCB_PLOTTER::BuildPlotFileName( &fn, outPath, wxT( "job" ), FILEEXT::GerberJobFileExtension );
        jobfile_writer.CreateJobFile( fn.GetFullPath() );
        aJob->AddOutput( fn.GetFullPath() );
    }

    return exitCode;
}


int PCBNEW_JOBS_HANDLER::JobExportGencad( JOB* aJob )
{
    JOB_EXPORT_PCB_GENCAD* aGencadJob = dynamic_cast<JOB_EXPORT_PCB_GENCAD*>( aJob );

    if( aGencadJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aGencadJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    GENCAD_EXPORTER exporter( brd );

    VECTOR2I GencadOffset;
    VECTOR2I auxOrigin = brd->GetDesignSettings().GetAuxOrigin();
    GencadOffset.x = aGencadJob->m_useDrillOrigin ? auxOrigin.x : 0;
    GencadOffset.y = aGencadJob->m_useDrillOrigin ? auxOrigin.y : 0;

    exporter.FlipBottomPads( aGencadJob->m_flipBottomPads );
    exporter.UsePinNamesUnique( aGencadJob->m_useUniquePins );
    exporter.UseIndividualShapes( aGencadJob->m_useIndividualShapes );
    exporter.SetPlotOffet( GencadOffset );
    exporter.StoreOriginCoordsInFile( aGencadJob->m_storeOriginCoords );

    if( aGencadJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( FILEEXT::GencadFileExtension );

        aGencadJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( !exporter.WriteFile( outPath ) )
    {
        m_reporter->Report( wxString::Format( _( "Failed to create file '%s'.\n" ), outPath ), RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    aJob->AddOutput( outPath );
    m_reporter->Report( _( "Successfully created genCAD file\n" ), RPT_SEVERITY_INFO );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportStats( JOB* aJob )
{
    JOB_EXPORT_PCB_STATS* statsJob = dynamic_cast<JOB_EXPORT_PCB_STATS*>( aJob );

    if( statsJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( statsJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    BOARD_STATISTICS_DATA data;
    InitializeBoardStatisticsData( data );

    BOARD_STATISTICS_OPTIONS options;
    options.excludeFootprintsWithoutPads = statsJob->m_excludeFootprintsWithoutPads;
    options.subtractHolesFromBoardArea = statsJob->m_subtractHolesFromBoardArea;
    options.subtractHolesFromCopperAreas = statsJob->m_subtractHolesFromCopperAreas;

    ComputeBoardStatistics( brd, options, data );

    wxString projectName;

    if( brd->GetProject() )
        projectName = brd->GetProject()->GetProjectName();

    wxFileName boardFile = brd->GetFileName();

    if( boardFile.GetName().IsEmpty() )
        boardFile = wxFileName( statsJob->m_filename );

    EDA_UNITS unitsForReport = statsJob->m_units == JOB_EXPORT_PCB_STATS::UNITS::MM ? EDA_UNITS::MM : EDA_UNITS::INCH;
    UNITS_PROVIDER unitsProvider( pcbIUScale, unitsForReport );

    wxString report;

    if( statsJob->m_format == JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT::JSON )
        report = FormatBoardStatisticsJson( data, brd, unitsProvider, projectName, boardFile.GetName() );
    else
        report = FormatBoardStatisticsReport( data, brd, unitsProvider, projectName, boardFile.GetName() );

    if( statsJob->GetConfiguredOutputPath().IsEmpty() && statsJob->GetWorkingOutputPath().IsEmpty() )
        statsJob->SetDefaultOutputPath( boardFile.GetFullPath() );

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    FILE* outFile = wxFopen( outPath, wxS( "wt" ) );

    if( !outFile )
    {
        m_reporter->Report( wxString::Format( _( "Failed to create file '%s'.\n" ), outPath ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( fprintf( outFile, "%s", TO_UTF8( report ) ) < 0 )
    {
        fclose( outFile );
        m_reporter->Report( wxString::Format( _( "Error writing file '%s'.\n" ), outPath ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    fclose( outFile );

    m_reporter->Report( wxString::Format( _( "Wrote board statistics to '%s'.\n" ), outPath ), RPT_SEVERITY_ACTION );

    statsJob->AddOutput( outPath );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportGerber( JOB* aJob )
{
    int                    exitCode = CLI::EXIT_CODES::OK;
    JOB_EXPORT_PCB_GERBER* aGerberJob = dynamic_cast<JOB_EXPORT_PCB_GERBER*>( aJob );

    if( aGerberJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aGerberJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !aGerberJob->m_variant.IsEmpty() )
        brd->SetCurrentVariant( aGerberJob->m_variant );

    TOOL_MANAGER* toolManager = getToolManager( brd );

    if( aGerberJob->m_argLayers )
        aGerberJob->m_plotLayerSequence = convertLayerArg( aGerberJob->m_argLayers.value(), brd );

    if( aGerberJob->m_argCommonLayers )
        aGerberJob->m_plotOnAllLayersSequence = convertLayerArg( aGerberJob->m_argCommonLayers.value(), brd );

    if( aGerberJob->m_plotLayerSequence.size() < 1 )
    {
        m_reporter->Report( _( "At least one layer must be specified\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    if( aGerberJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::GERBER ) );

        aGerberJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( aGerberJob->m_checkZonesBeforePlot )
    {
        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, aGerberJob, *m_reporter );
    plotOpts.SetLayerSelection( aGerberJob->m_plotLayerSequence );
    plotOpts.SetPlotOnAllLayersSequence( aGerberJob->m_plotOnAllLayersSequence );

    PCB_LAYER_ID layer = UNDEFINED_LAYER;
    wxString     layerName;
    wxString     sheetName;
    wxString     sheetPath;

    // The first layer will be treated as the layer name for the gerber header,
    // the other layers will be treated equivalent to the "Plot on All Layers" option
    // in the GUI
    if( aGerberJob->m_plotLayerSequence.size() >= 1 )
    {
        layer = aGerberJob->m_plotLayerSequence.front();
        layerName = brd->GetLayerName( layer );
    }

    if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
        layerName = aJob->GetVarOverrides().at( wxT( "LAYER" ) );

    if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
        sheetName = aJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

    if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
        sheetPath = aJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );

    // We are feeding it one layer at the start here to silence a logic check
    PLOTTER* plotter = StartPlotBoard( brd, &plotOpts, layer, layerName, outPath, sheetName, sheetPath );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aGerberJob->m_plotLayerSequence, plotOpts );
        plotter->EndPlot();
    }
    else
    {
        m_reporter->Report( wxString::Format( _( "Failed to plot to '%s'.\n" ), outPath ), RPT_SEVERITY_ERROR );
        exitCode = CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    delete plotter;

    return exitCode;
}

static DRILL_PRECISION precisionListForInches( 2, 4 );
static DRILL_PRECISION precisionListForMetric( 3, 3 );


int PCBNEW_JOBS_HANDLER::JobExportDrill( JOB* aJob )
{
    JOB_EXPORT_PCB_DRILL* aDrillJob = dynamic_cast<JOB_EXPORT_PCB_DRILL*>( aJob );

    if( aDrillJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aDrillJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    std::unique_ptr<GENDRILL_WRITER_BASE> drillWriter;

    if( aDrillJob->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON )
        drillWriter = std::make_unique<EXCELLON_WRITER>( brd );
    else
        drillWriter = std::make_unique<GERBER_WRITER>( brd );

    VECTOR2I offset;

    if( aDrillJob->m_drillOrigin == JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::ABS )
        offset = VECTOR2I( 0, 0 );
    else
        offset = brd->GetDesignSettings().GetAuxOrigin();

    PLOT_FORMAT mapFormat = PLOT_FORMAT::PDF;

    switch( aDrillJob->m_mapFormat )
    {
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::POSTSCRIPT: mapFormat = PLOT_FORMAT::POST; break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::GERBER_X2: mapFormat = PLOT_FORMAT::GERBER; break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::DXF: mapFormat = PLOT_FORMAT::DXF; break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::SVG: mapFormat = PLOT_FORMAT::SVG; break;
    default:
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::PDF: mapFormat = PLOT_FORMAT::PDF; break;
    }


    if( aDrillJob->m_generateReport && aDrillJob->m_reportPath.IsEmpty() )
    {
        wxFileName fn = outPath;
        fn.SetFullName( brd->GetFileName() );
        fn.SetName( fn.GetName() + "-drill" );
        fn.SetExt( FILEEXT::ReportFileExtension );

        aDrillJob->m_reportPath = fn.GetFullPath();
    }

    if( aDrillJob->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON )
    {
        EXCELLON_WRITER::ZEROS_FMT zeroFmt;

        switch( aDrillJob->m_zeroFormat )
        {
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::KEEP_ZEROS: zeroFmt = EXCELLON_WRITER::KEEP_ZEROS; break;

        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_LEADING: zeroFmt = EXCELLON_WRITER::SUPPRESS_LEADING; break;

        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_TRAILING: zeroFmt = EXCELLON_WRITER::SUPPRESS_TRAILING; break;

        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::DECIMAL:
        default: zeroFmt = EXCELLON_WRITER::DECIMAL_FORMAT; break;
        }

        DRILL_PRECISION precision;

        if( aDrillJob->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCH )
            precision = precisionListForInches;
        else
            precision = precisionListForMetric;

        EXCELLON_WRITER* excellonWriter = dynamic_cast<EXCELLON_WRITER*>( drillWriter.get() );

        if( excellonWriter == nullptr )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        excellonWriter->SetFormat( aDrillJob->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MM, zeroFmt,
                                   precision.m_Lhs, precision.m_Rhs );
        excellonWriter->SetOptions( aDrillJob->m_excellonMirrorY, aDrillJob->m_excellonMinimalHeader, offset,
                                    aDrillJob->m_excellonCombinePTHNPTH );
        excellonWriter->SetRouteModeForOvalHoles( aDrillJob->m_excellonOvalDrillRoute );
        excellonWriter->SetMapFileFormat( mapFormat );

        if( !excellonWriter->CreateDrillandMapFilesSet( outPath, true, aDrillJob->m_generateMap, m_reporter ) )
        {
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }

        aDrillJob->AddOutput( outPath );

        if( aDrillJob->m_generateReport )
        {
            wxString reportPath = aDrillJob->ResolveOutputPath( aDrillJob->m_reportPath, true, brd->GetProject() );

            if( !excellonWriter->GenDrillReportFile( reportPath ) )
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }

            aDrillJob->AddOutput( reportPath );
        }
    }
    else if( aDrillJob->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER )
    {
        GERBER_WRITER* gerberWriter = dynamic_cast<GERBER_WRITER*>( drillWriter.get() );

        if( gerberWriter == nullptr )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        // Set gerber precision: only 5 or 6 digits for mantissa are allowed
        // (SetFormat() accept 5 or 6, and any other value set the precision to 5)
        // the integer part precision is always 4, and units always mm
        gerberWriter->SetFormat( aDrillJob->m_gerberPrecision );
        gerberWriter->SetOptions( offset );
        gerberWriter->SetMapFileFormat( mapFormat );

        if( !gerberWriter->CreateDrillandMapFilesSet( outPath, true, aDrillJob->m_generateMap,
                                                      aDrillJob->m_generateTenting, m_reporter ) )
        {
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }

        aDrillJob->AddOutput( outPath );

        if( aDrillJob->m_generateReport )
        {
            wxString reportPath = aDrillJob->ResolveOutputPath( aDrillJob->m_reportPath, true, brd->GetProject() );

            if( !gerberWriter->GenDrillReportFile( reportPath ) )
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }

            aDrillJob->AddOutput( reportPath );
        }
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportPos( JOB* aJob )
{
    JOB_EXPORT_PCB_POS* aPosJob = dynamic_cast<JOB_EXPORT_PCB_POS*>( aJob );

    if( aPosJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aPosJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( aPosJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );

        if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII )
            fn.SetExt( FILEEXT::FootprintPlaceFileExtension );
        else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
            fn.SetExt( FILEEXT::CsvFileExtension );
        else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::GERBER )
            fn.SetExt( FILEEXT::GerberFileExtension );

        aPosJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII || aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
    {
        wxFileName fn( outPath );
        wxString   baseName = fn.GetName();

        auto exportPlaceFile = [&]( bool frontSide, bool backSide, const wxString& curr_outPath ) -> bool
        {
            FILE* file = wxFopen( curr_outPath, wxS( "wt" ) );
            wxCHECK( file, false );

            PLACE_FILE_EXPORTER exporter( brd, aPosJob->m_units == JOB_EXPORT_PCB_POS::UNITS::MM, aPosJob->m_smdOnly,
                                          aPosJob->m_excludeFootprintsWithTh, aPosJob->m_excludeDNP,
                                          aPosJob->m_excludeBOM, frontSide, backSide,
                                          aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV,
                                          aPosJob->m_useDrillPlaceFileOrigin, aPosJob->m_negateBottomX );

            // Set variant for variant-aware DNP/BOM/position file filtering
            exporter.SetVariant( aPosJob->m_variant );

            std::string data = exporter.GenPositionData();
            fputs( data.c_str(), file );
            fclose( file );

            return true;
        };

        if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH && !aPosJob->m_singleFile )
        {
            fn.SetName( PLACE_FILE_EXPORTER::DecorateFilename( baseName, true, false ) );

            if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV && !aPosJob->m_nakedFilename )
                fn.SetName( fn.GetName() + wxT( "-" ) + FILEEXT::FootprintPlaceFileExtension );

            if( exportPlaceFile( true, false, fn.GetFullPath() ) )
            {
                m_reporter->Report( wxString::Format( _( "Wrote front position data to '%s'.\n" ), fn.GetFullPath() ),
                                    RPT_SEVERITY_ACTION );

                aPosJob->AddOutput( fn.GetFullPath() );
            }
            else
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }

            fn.SetName( PLACE_FILE_EXPORTER::DecorateFilename( baseName, false, true ) );

            if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV && !aPosJob->m_nakedFilename )
                fn.SetName( fn.GetName() + wxT( "-" ) + FILEEXT::FootprintPlaceFileExtension );

            if( exportPlaceFile( false, true, fn.GetFullPath() ) )
            {
                m_reporter->Report( wxString::Format( _( "Wrote back position data to '%s'.\n" ), fn.GetFullPath() ),
                                    RPT_SEVERITY_ACTION );

                aPosJob->AddOutput( fn.GetFullPath() );
            }
            else
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }
        else
        {
            bool front = aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::FRONT
                         || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH;

            bool back = aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BACK
                        || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH;

            if( !aPosJob->m_nakedFilename )
            {
                fn.SetName( PLACE_FILE_EXPORTER::DecorateFilename( fn.GetName(), front, back ) );

                if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
                    fn.SetName( fn.GetName() + wxT( "-" ) + FILEEXT::FootprintPlaceFileExtension );
            }

            if( exportPlaceFile( front, back, fn.GetFullPath() ) )
            {
                m_reporter->Report( wxString::Format( _( "Wrote position data to '%s'.\n" ), fn.GetFullPath() ),
                                    RPT_SEVERITY_ACTION );

                aPosJob->AddOutput( fn.GetFullPath() );
            }
            else
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }
    }
    else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::GERBER )
    {
        PLACEFILE_GERBER_WRITER exporter( brd );

        // Set variant for variant-aware DNP/BOM/position file filtering
        exporter.SetVariant( aPosJob->m_variant );

        PCB_LAYER_ID gbrLayer = F_Cu;
        wxString     outPath_base = outPath;

        if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::FRONT || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH )
        {
            if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH || !aPosJob->m_nakedFilename )
                outPath = exporter.GetPlaceFileName( outPath, gbrLayer );

            if( exporter.CreatePlaceFile( outPath, gbrLayer, aPosJob->m_gerberBoardEdge, aPosJob->m_excludeDNP,
                                          aPosJob->m_excludeBOM )
                >= 0 )
            {
                m_reporter->Report( wxString::Format( _( "Wrote front position data to '%s'.\n" ), outPath ),
                                    RPT_SEVERITY_ACTION );

                aPosJob->AddOutput( outPath );
            }
            else
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }

        if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BACK || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH )
        {
            gbrLayer = B_Cu;

            outPath = outPath_base;

            if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH || !aPosJob->m_nakedFilename )
                outPath = exporter.GetPlaceFileName( outPath, gbrLayer );

            if( exporter.CreatePlaceFile( outPath, gbrLayer, aPosJob->m_gerberBoardEdge, aPosJob->m_excludeDNP,
                                          aPosJob->m_excludeBOM )
                >= 0 )
            {
                m_reporter->Report( wxString::Format( _( "Wrote back position data to '%s'.\n" ), outPath ),
                                    RPT_SEVERITY_ACTION );

                aPosJob->AddOutput( outPath );
            }
            else
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportFpUpgrade( JOB* aJob )
{
    JOB_FP_UPGRADE* upgradeJob = dynamic_cast<JOB_FP_UPGRADE*>( aJob );

    if( upgradeJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    PCB_IO_MGR::PCB_FILE_T fileType = PCB_IO_MGR::GuessPluginTypeFromLibPath( upgradeJob->m_libraryPath );

    if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
    {
        if( wxFile::Exists( upgradeJob->m_outputLibraryPath ) || wxDir::Exists( upgradeJob->m_outputLibraryPath ) )
        {
            m_reporter->Report( _( "Output path must not conflict with existing path\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }
    else if( fileType != PCB_IO_MGR::KICAD_SEXP )
    {
        m_reporter->Report( _( "Output path must be specified to convert legacy and non-KiCad libraries\n" ),
                            RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( fileType == PCB_IO_MGR::KICAD_SEXP )
    {
        if( !wxDir::Exists( upgradeJob->m_libraryPath ) )
        {
            m_reporter->Report( _( "Footprint library path does not exist or is not accessible\n" ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        PCB_IO_KICAD_SEXPR pcb_io( CTL_FOR_LIBRARY );
        FP_CACHE           fpLib( &pcb_io, upgradeJob->m_libraryPath );

        try
        {
            fpLib.Load();
        }
        catch( ... )
        {
            m_reporter->Report( _( "Unable to load library\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        if( m_progressReporter )
            m_progressReporter->KeepRefreshing();

        bool shouldSave = upgradeJob->m_force;

        for( const auto& footprint : fpLib.GetFootprints() )
        {
            if( footprint.second->GetFootprint()->GetFileFormatVersionAtLoad() < SEXPR_BOARD_FILE_VERSION )
                shouldSave = true;
        }

        if( shouldSave )
        {
            try
            {
                if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
                    fpLib.SetPath( upgradeJob->m_outputLibraryPath );

                fpLib.Save();
            }
            catch( ... )
            {
                m_reporter->Report( _( "Unable to save library\n" ), RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }
        }
        else
        {
            m_reporter->Report( _( "Footprint library was not updated\n" ), RPT_SEVERITY_ERROR );
        }
    }
    else
    {
        if( !PCB_IO_MGR::ConvertLibrary( {}, upgradeJob->m_libraryPath, upgradeJob->m_outputLibraryPath,
                                         nullptr /* REPORTER */ ) )
        {
            m_reporter->Report( ( "Unable to convert library\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportFpSvg( JOB* aJob )
{
    JOB_FP_EXPORT_SVG* svgJob = dynamic_cast<JOB_FP_EXPORT_SVG*>( aJob );

    if( svgJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    PCB_IO_KICAD_SEXPR pcb_io( CTL_FOR_LIBRARY );
    FP_CACHE           fpLib( &pcb_io, svgJob->m_libraryPath );

    if( svgJob->m_argLayers )
    {
        if( !svgJob->m_argLayers.value().empty() )
            svgJob->m_plotLayerSequence = convertLayerArg( svgJob->m_argLayers.value(), nullptr );
        else
            svgJob->m_plotLayerSequence = LSET::AllLayersMask().SeqStackupForPlotting();
    }

    try
    {
        fpLib.Load();
    }
    catch( ... )
    {
        m_reporter->Report( _( "Unable to load library\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    wxString outPath = svgJob->GetFullOutputPath( nullptr );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    int  exitCode = CLI::EXIT_CODES::OK;
    bool singleFpPlotted = false;

    for( const auto& [fpName, fpCacheEntry] : fpLib.GetFootprints() )
    {
        if( m_progressReporter )
        {
            m_progressReporter->AdvancePhase( wxString::Format( _( "Exporting %s" ), fpName ) );
            m_progressReporter->KeepRefreshing();
        }

        if( !svgJob->m_footprint.IsEmpty() )
        {
            // skip until we find the right footprint
            if( fpName != svgJob->m_footprint )
                continue;
            else
                singleFpPlotted = true;
        }

        exitCode = doFpExportSvg( svgJob, fpCacheEntry->GetFootprint().get() );

        if( exitCode != CLI::EXIT_CODES::OK )
            break;
    }

    if( !svgJob->m_footprint.IsEmpty() && !singleFpPlotted )
    {
        m_reporter->Report( _( "The given footprint could not be found to export." ) + wxS( "\n" ),
                            RPT_SEVERITY_ERROR );
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::doFpExportSvg( JOB_FP_EXPORT_SVG* aSvgJob, const FOOTPRINT* aFootprint )
{
    // the hack for now is we create fake boards containing the footprint and plot the board
    // until we refactor better plot api later
    std::unique_ptr<BOARD> brd = BOARD_LOADER::CreateEmptyBoard( Pgm().GetSettingsManager().GetProject( "" ) );
    brd->GetProject()->ApplyTextVars( aSvgJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aFootprint->Clone() );

    if( fp == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    fp->SetLink( niluuid );
    fp->SetFlags( IS_NEW );
    fp->SetParent( brd.get() );

    for( PAD* pad : fp->Pads() )
    {
        pad->SetLocalRatsnestVisible( false );
        pad->SetNetCode( 0 );
    }

    fp->SetOrientation( ANGLE_0 );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    brd->Add( fp, ADD_MODE::INSERT, true );

    wxFileName outputFile;
    outputFile.SetPath( aSvgJob->GetFullOutputPath( nullptr ) );
    outputFile.SetName( aFootprint->GetFPID().GetLibItemName().wx_str() );
    outputFile.SetExt( FILEEXT::SVGFileExtension );

    m_reporter->Report( wxString::Format( _( "Plotting footprint '%s' to '%s'\n" ),
                                          aFootprint->GetFPID().GetLibItemName().wx_str(), outputFile.GetFullPath() ),
                        RPT_SEVERITY_ACTION );

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, aSvgJob, *m_reporter );

    // always fixed for the svg plot
    plotOpts.SetPlotFrameRef( false );
    plotOpts.SetSvgFitPageToBoard( true );
    plotOpts.SetMirror( false );
    plotOpts.SetSkipPlotNPTH_Pads( false );

    if( plotOpts.GetSketchPadsOnFabLayers() )
    {
        plotOpts.SetPlotPadNumbers( true );
    }

    PCB_PLOTTER plotter( brd.get(), m_reporter, plotOpts );

    if( !plotter.Plot( outputFile.GetFullPath(), aSvgJob->m_plotLayerSequence, aSvgJob->m_plotOnAllLayersSequence,
                       false, true, wxEmptyString, wxEmptyString, wxEmptyString ) )
    {
        m_reporter->Report( _( "Error creating svg file" ) + wxS( "\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    aSvgJob->AddOutput( outputFile.GetFullPath() );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportDrc( JOB* aJob )
{
    JOB_PCB_DRC* drcJob = dynamic_cast<JOB_PCB_DRC*>( aJob );

    if( drcJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( drcJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    // Running DRC requires libraries be loaded, so make sure they have been
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( brd->GetProject() );
    adapter->AsyncLoad();
    adapter->BlockUntilLoaded();

    if( drcJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() + wxS( "-drc" ) );

        if( drcJob->m_format == JOB_PCB_DRC::OUTPUT_FORMAT::JSON )
            fn.SetExt( FILEEXT::JsonFileExtension );
        else
            fn.SetExt( FILEEXT::ReportFileExtension );

        drcJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    EDA_UNITS units;

    switch( drcJob->m_units )
    {
    case JOB_PCB_DRC::UNITS::INCH: units = EDA_UNITS::INCH; break;
    case JOB_PCB_DRC::UNITS::MILS: units = EDA_UNITS::MILS; break;
    case JOB_PCB_DRC::UNITS::MM: units = EDA_UNITS::MM; break;
    default: units = EDA_UNITS::MM; break;
    }

    std::shared_ptr<DRC_ENGINE> drcEngine = brd->GetDesignSettings().m_DRCEngine;
    std::unique_ptr<NETLIST>    netlist = std::make_unique<NETLIST>();

    drcEngine->SetDrawingSheet( getDrawingSheetProxyView( brd ) );

    // BOARD_COMMIT uses TOOL_MANAGER to grab the board internally so we must give it one
    TOOL_MANAGER* toolManager = getToolManager( brd );

    BOARD_COMMIT commit( toolManager );
    bool         checkParity = drcJob->m_parity;
    std::string  netlist_str;

    if( checkParity )
    {
        wxString annotateMsg = _( "Schematic parity tests require a fully annotated schematic." );
        netlist_str = annotateMsg;

        // The KIFACE_NETLIST_SCHEMATIC function has some broken-ness that the schematic
        // frame's version does not, but it is the only one that works in CLI, so we use it
        // if we don't have the sch frame open.
        // TODO: clean this up, see https://gitlab.com/kicad/code/kicad/-/issues/19929
        if( m_kiway->Player( FRAME_SCH, false ) )
        {
            m_kiway->ExpressMail( FRAME_SCH, MAIL_SCH_GET_NETLIST, netlist_str );
        }
        else
        {
            wxFileName schematicPath( drcJob->m_filename );
            schematicPath.MakeAbsolute();
            schematicPath.SetExt( FILEEXT::KiCadSchematicFileExtension );

            if( !schematicPath.Exists() )
                schematicPath.SetExt( FILEEXT::LegacySchematicFileExtension );

            if( !schematicPath.Exists() )
            {
                m_reporter->Report( _( "Failed to fetch schematic netlist for parity tests.\n" ), RPT_SEVERITY_ERROR );
                checkParity = false;
            }
            else
            {
                typedef bool ( *NETLIST_FN_PTR )( const wxString&, std::string& );
                KIFACE*        eeschema = m_kiway->KiFACE( KIWAY::FACE_SCH );
                NETLIST_FN_PTR netlister = (NETLIST_FN_PTR) eeschema->IfaceOrAddress( KIFACE_NETLIST_SCHEMATIC );
                ( *netlister )( schematicPath.GetFullPath(), netlist_str );
            }
        }

        if( netlist_str == annotateMsg )
        {
            m_reporter->Report( wxString( netlist_str ) + wxT( "\n" ), RPT_SEVERITY_ERROR );
            checkParity = false;
        }
    }

    if( checkParity )
    {
        try
        {
            STRING_LINE_READER*  lineReader = new STRING_LINE_READER( netlist_str, _( "Eeschema netlist" ) );
            KICAD_NETLIST_READER netlistReader( lineReader, netlist.get() );

            netlistReader.LoadNetlist();
        }
        catch( const IO_ERROR& )
        {
            m_reporter->Report( _( "Failed to fetch schematic netlist for parity tests.\n" ), RPT_SEVERITY_ERROR );
            checkParity = false;
        }

        drcEngine->SetSchematicNetlist( netlist.get() );
    }

    if( drcJob->m_refillZones )
    {
        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    drcEngine->SetProgressReporter( m_progressReporter );
    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                PCB_MARKER* marker = new PCB_MARKER( aItem, aPos, aLayer );
                aPathGenerator( marker );
                commit.Add( marker );
            } );

    brd->RecordDRCExclusions();
    brd->DeleteMARKERs( true, true );
    drcEngine->RunTests( units, drcJob->m_reportAllTrackErrors, checkParity );
    drcEngine->ClearViolationHandler();

    commit.Push( _( "DRC" ), SKIP_UNDO | SKIP_SET_DIRTY );

    // Update the exclusion status on any excluded markers that still exist.
    brd->ResolveDRCExclusions( false );

    std::shared_ptr<DRC_ITEMS_PROVIDER> markersProvider =
            std::make_shared<DRC_ITEMS_PROVIDER>( brd, MARKER_BASE::MARKER_DRC, MARKER_BASE::MARKER_DRAWING_SHEET );

    std::shared_ptr<DRC_ITEMS_PROVIDER> ratsnestProvider =
            std::make_shared<DRC_ITEMS_PROVIDER>( brd, MARKER_BASE::MARKER_RATSNEST );

    std::shared_ptr<DRC_ITEMS_PROVIDER> fpWarningsProvider =
            std::make_shared<DRC_ITEMS_PROVIDER>( brd, MARKER_BASE::MARKER_PARITY );

    markersProvider->SetSeverities( drcJob->m_severity );
    ratsnestProvider->SetSeverities( drcJob->m_severity );
    fpWarningsProvider->SetSeverities( drcJob->m_severity );

    m_reporter->Report( wxString::Format( _( "Found %d violations\n" ), markersProvider->GetCount() ),
                        RPT_SEVERITY_INFO );
    m_reporter->Report( wxString::Format( _( "Found %d unconnected items\n" ), ratsnestProvider->GetCount() ),
                        RPT_SEVERITY_INFO );

    if( checkParity )
    {
        m_reporter->Report(
                wxString::Format( _( "Found %d schematic parity issues\n" ), fpWarningsProvider->GetCount() ),
                RPT_SEVERITY_INFO );
    }

    DRC_REPORT reportWriter( brd, units, markersProvider, ratsnestProvider, fpWarningsProvider );

    bool wroteReport = false;

    if( drcJob->m_format == JOB_PCB_DRC::OUTPUT_FORMAT::JSON )
        wroteReport = reportWriter.WriteJsonReport( outPath );
    else
        wroteReport = reportWriter.WriteTextReport( outPath );

    if( !wroteReport )
    {
        m_reporter->Report( wxString::Format( _( "Unable to save DRC report to %s\n" ), outPath ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    drcJob->AddOutput( outPath );

    m_reporter->Report( wxString::Format( _( "Saved DRC Report to %s\n" ), outPath ), RPT_SEVERITY_ACTION );

    if( drcJob->m_refillZones && drcJob->m_saveBoard )
    {
        if( BOARD_LOADER::SaveBoard( drcJob->m_filename, brd ) )
        {
            m_reporter->Report( _( "Saved board\n" ), RPT_SEVERITY_ACTION );
        }
        else
        {
            m_reporter->Report( _( "Failed to save board.\n" ), RPT_SEVERITY_ERROR );

            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    if( drcJob->m_exitCodeViolations )
    {
        if( markersProvider->GetCount() > 0 || ratsnestProvider->GetCount() > 0 || fpWarningsProvider->GetCount() > 0 )
        {
            return CLI::EXIT_CODES::ERR_RC_VIOLATIONS;
        }
    }

    return CLI::EXIT_CODES::SUCCESS;
}


int PCBNEW_JOBS_HANDLER::JobExportIpc2581( JOB* aJob )
{
    JOB_EXPORT_PCB_IPC2581* job = dynamic_cast<JOB_EXPORT_PCB_IPC2581*>( aJob );

    if( job == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( job->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !job->m_variant.IsEmpty() )
        brd->SetCurrentVariant( job->m_variant );

    if( job->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetExt( job->m_compress ? std::string( "zip" ) : FILEEXT::Ipc2581FileExtension );

        job->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( !DIALOG_EXPORT_2581::GenerateFile( *job, brd, m_progressReporter, m_reporter ) )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::SUCCESS;
}


int PCBNEW_JOBS_HANDLER::JobExportIpcD356( JOB* aJob )
{
    JOB_EXPORT_PCB_IPCD356* job = dynamic_cast<JOB_EXPORT_PCB_IPCD356*>( aJob );

    if( job == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( job->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( job->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( FILEEXT::IpcD356FileExtension );

        job->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = resolveJobOutputPath( aJob, brd );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    IPC356D_WRITER exporter( brd );

    bool success = exporter.Write( outPath );

    if( success )
    {
        aJob->AddOutput( outPath );
        m_reporter->Report( _( "Successfully created IPC-D-356 file\n" ), RPT_SEVERITY_INFO );
        return CLI::EXIT_CODES::SUCCESS;
    }
    else
    {
        m_reporter->Report( _( "Failed to create IPC-D-356 file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }
}


int PCBNEW_JOBS_HANDLER::JobExportOdb( JOB* aJob )
{
    JOB_EXPORT_PCB_ODB* job = dynamic_cast<JOB_EXPORT_PCB_ODB*>( aJob );

    if( job == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( job->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( !job->m_variant.IsEmpty() )
        brd->SetCurrentVariant( job->m_variant );

    if( job->GetConfiguredOutputPath().IsEmpty() )
    {
        if( job->m_compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE )
        {
            // just basic folder name
            job->SetWorkingOutputPath( "odb" );
        }
        else
        {
            wxFileName fn( brd->GetFileName() );
            fn.SetName( fn.GetName() + wxS( "-odb" ) );

            switch( job->m_compressionMode )
            {
            case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP: fn.SetExt( FILEEXT::ArchiveFileExtension ); break;

            case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ: fn.SetExt( "tgz" ); break;

            default: break;
            };

            job->SetWorkingOutputPath( fn.GetFullName() );
        }
    }

    wxString outPath = resolveJobOutputPath( job, brd );

    // The helper handles output path creation, so hand it a job that already has fully-resolved
    // token context (title block and project overrides applied above).
    CLI_REPORTER reporter;

    if( !m_reporter )
        m_reporter = &reporter;

    if( job->m_checkZonesBeforeExport )
    {
        TOOL_MANAGER* toolManager = getToolManager( brd );

        if( !toolManager->FindTool( ZONE_FILLER_TOOL_NAME ) )
            toolManager->RegisterTool( new ZONE_FILLER_TOOL );

        toolManager->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, m_progressReporter, true );
    }

    DIALOG_EXPORT_ODBPP::GenerateODBPPFiles( *job, brd, nullptr, m_progressReporter, m_reporter );
    aJob->AddOutput( outPath );

    if( m_reporter->HasMessageOfSeverity( RPT_SEVERITY_ERROR ) )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::SUCCESS;
}

int PCBNEW_JOBS_HANDLER::JobUpgrade( JOB* aJob )
{
    JOB_PCB_UPGRADE* job = dynamic_cast<JOB_PCB_UPGRADE*>( aJob );

    if( job == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    bool shouldSave = job->m_force;

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );
        BOARD*              brd = getBoard( job->m_filename );
        if( brd->GetFileFormatVersionAtLoad() < SEXPR_BOARD_FILE_VERSION )
            shouldSave = true;

        if( shouldSave )
        {
            pi->SaveBoard( brd->GetFileName(), brd );
            m_reporter->Report( _( "Successfully saved board file using the latest format\n" ), RPT_SEVERITY_INFO );
        }
        else
        {
            m_reporter->Report( _( "Board file was not updated\n" ), RPT_SEVERITY_ERROR );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg =
                wxString::Format( _( "Error saving board file '%s'.\n%s" ), job->m_filename, ioe.What().GetData() );
        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return CLI::EXIT_CODES::SUCCESS;
}

// Most job handlers need to align the running job with the board before resolving any
// output paths with variables in them like ${REVISION}.
wxString PCBNEW_JOBS_HANDLER::resolveJobOutputPath( JOB* aJob, BOARD* aBoard, const wxString* aDrawingSheet )
{
    aJob->SetTitleBlock( aBoard->GetTitleBlock() );

    if( aDrawingSheet && !aDrawingSheet->IsEmpty() )
        loadOverrideDrawingSheet( aBoard, *aDrawingSheet );

    PROJECT* project = aBoard->GetProject();

    if( project )
        project->ApplyTextVars( aJob->GetVarOverrides() );

    aBoard->SynchronizeProperties();

    return aJob->GetFullOutputPath( project );
}


DS_PROXY_VIEW_ITEM* PCBNEW_JOBS_HANDLER::getDrawingSheetProxyView( BOARD* aBrd )
{
    DS_PROXY_VIEW_ITEM* drawingSheet = new DS_PROXY_VIEW_ITEM( pcbIUScale, &aBrd->GetPageSettings(), aBrd->GetProject(),
                                                               &aBrd->GetTitleBlock(), &aBrd->GetProperties() );

    drawingSheet->SetSheetName( std::string() );
    drawingSheet->SetSheetPath( std::string() );
    drawingSheet->SetIsFirstPage( true );

    drawingSheet->SetFileName( TO_UTF8( aBrd->GetFileName() ) );

    wxString currentVariant = aBrd->GetCurrentVariant();
    wxString variantDesc = aBrd->GetVariantDescription( currentVariant );
    drawingSheet->SetVariantName( TO_UTF8( currentVariant ) );
    drawingSheet->SetVariantDesc( TO_UTF8( variantDesc ) );

    return drawingSheet;
}


void PCBNEW_JOBS_HANDLER::loadOverrideDrawingSheet( BOARD* aBrd, const wxString& aSheetPath )
{
    // dont bother attempting to load a empty path, if there was one
    if( aSheetPath.IsEmpty() )
        return;

    auto loadSheet = [&]( const wxString& path ) -> bool
    {
        BASE_SCREEN::m_DrawingSheetFileName = path;
        FILENAME_RESOLVER resolver;
        resolver.SetProject( aBrd->GetProject() );
        resolver.SetProgramBase( &Pgm() );

        wxString filename = resolver.ResolvePath( BASE_SCREEN::m_DrawingSheetFileName,
                                                  aBrd->GetProject()->GetProjectPath(), { aBrd->GetEmbeddedFiles() } );
        wxString msg;

        if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename, &msg ) )
        {
            m_reporter->Report( wxString::Format( _( "Error loading drawing sheet '%s'." ), path ) + wxS( "\n" ) + msg
                                        + wxS( "\n" ),
                                RPT_SEVERITY_ERROR );
            return false;
        }

        return true;
    };

    if( loadSheet( aSheetPath ) )
        return;

    // failed loading custom path, revert back to default
    loadSheet( aBrd->GetProject()->GetProjectFile().m_BoardDrawingSheetFile );
}


// Resolve a KiCad layer name (canonical board-file name such as "F.Cu", or the GUI display name)
// to its layer id.  Returns UNDEFINED_LAYER when no layer matches.
static PCB_LAYER_ID resolveKiCadLayerName( const wxString& aName )
{
    for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
    {
        if( LSET::Name( layer ) == aName || LayerName( layer ) == aName )
            return layer;
    }

    return UNDEFINED_LAYER;
}


int PCBNEW_JOBS_HANDLER::JobImport( JOB* aJob )
{
    JOB_PCB_IMPORT* job = dynamic_cast<JOB_PCB_IMPORT*>( aJob );

    if( !job )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    // Check that input file exists
    if( !wxFile::Exists( job->m_inputFile ) )
    {
        m_reporter->Report( wxString::Format( _( "Input file not found: '%s'\n" ),
                                              job->m_inputFile ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Map job format to PCB_IO file type
    PCB_IO_MGR::PCB_FILE_T fileType = PCB_IO_MGR::PCB_FILE_UNKNOWN;

    switch( job->m_format )
    {
    case JOB_PCB_IMPORT::FORMAT::AUTO: fileType = PCB_IO_MGR::FindPluginTypeFromBoardPath( job->m_inputFile ); break;

    case JOB_PCB_IMPORT::FORMAT::PADS_ASCII: fileType = PCB_IO_MGR::PADS; break;

    case JOB_PCB_IMPORT::FORMAT::ALTIUM: fileType = PCB_IO_MGR::ALTIUM_DESIGNER; break;

    case JOB_PCB_IMPORT::FORMAT::EAGLE: fileType = PCB_IO_MGR::EAGLE; break;

    case JOB_PCB_IMPORT::FORMAT::CADSTAR: fileType = PCB_IO_MGR::CADSTAR_PCB_ARCHIVE; break;

    case JOB_PCB_IMPORT::FORMAT::FABMASTER: fileType = PCB_IO_MGR::FABMASTER; break;

    case JOB_PCB_IMPORT::FORMAT::PCAD: fileType = PCB_IO_MGR::PCAD; break;

    case JOB_PCB_IMPORT::FORMAT::SOLIDWORKS: fileType = PCB_IO_MGR::SOLIDWORKS_PCB; break;
    }

    // FindPluginTypeFromBoardPath returns FILE_TYPE_NONE (not PCB_FILE_UNKNOWN) when no plugin
    // claims the file.  Quiet sentinel: lets the top-level `import` command try the schematic face.
    if( fileType == PCB_IO_MGR::PCB_FILE_UNKNOWN || fileType == PCB_IO_MGR::FILE_TYPE_NONE )
    {
        m_reporter->Report( wxString::Format( _( "No PCB importer recognizes the file format of "
                                                 "'%s'\n" ),
                                              job->m_inputFile ),
                            RPT_SEVERITY_INFO );
        return CLI::EXIT_CODES::ERR_UNKNOWN_FILE_FORMAT;
    }

    // Determine output path
    wxString outputPath = job->GetConfiguredOutputPath();

    if( outputPath.IsEmpty() )
        outputPath = DefaultImportOutputPath( job->m_inputFile, FILEEXT::KiCadPcbFileExtension );

    BOARD*                board = nullptr;
    wxString              formatName = PCB_IO_MGR::ShowType( fileType );
    std::vector<wxString> warnings;

    // Real source-to-KiCad layer decisions, captured by our mapping callback so the report can
    // show them and so explicit overrides can be validated.
    struct CAPTURED_LAYER
    {
        wxString     m_source;
        PCB_LAYER_ID m_target;
        wxString     m_method;
    };

    std::vector<CAPTURED_LAYER> capturedLayers;
    std::set<wxString>          seenSourceLayers;
    bool                        layersCaptured = false;

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( fileType ) );

        if( !pi )
        {
            m_reporter->Report( wxString::Format( _( "No plugin found for file type '%s'\n" ), formatName ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        // Replace the plugin's default best-guess callback so we can apply explicit overrides and
        // capture the resulting mapping.  Only mappable importers expose their source layers; for
        // others the report falls back to listing the imported board's enabled layers.
        if( LAYER_MAPPABLE_PLUGIN* mappable = dynamic_cast<LAYER_MAPPABLE_PLUGIN*>( pi.get() ) )
        {
            if( !job->m_layerMap.empty() || job->m_reportFormat != IMPORT_REPORT_FORMAT::NONE )
            {
                mappable->RegisterCallback(
                        [&]( const std::vector<INPUT_LAYER_DESC>& aDescs )
                                -> std::map<wxString, PCB_LAYER_ID>
                        {
                            std::map<wxString, PCB_LAYER_ID> result;

                            for( const INPUT_LAYER_DESC& desc : aDescs )
                            {
                                PCB_LAYER_ID target = desc.AutoMapLayer;
                                wxString     method = wxS( "auto" );

                                if( auto it = job->m_layerMap.find( desc.Name );
                                    it != job->m_layerMap.end() )
                                {
                                    PCB_LAYER_ID resolved = resolveKiCadLayerName( it->second );

                                    if( resolved == UNDEFINED_LAYER )
                                    {
                                        warnings.push_back( wxString::Format(
                                                _( "Layer map entry '%s' -> '%s' names an unknown "
                                                   "KiCad layer; using automatic mapping instead" ),
                                                desc.Name, it->second ) );
                                    }
                                    else if( !desc.PermittedLayers.Contains( resolved ) )
                                    {
                                        warnings.push_back( wxString::Format(
                                                _( "Layer map entry '%s' -> '%s' is not a permitted "
                                                   "target for this layer; using automatic mapping "
                                                   "instead" ),
                                                desc.Name, it->second ) );
                                    }
                                    else
                                    {
                                        target = resolved;
                                        method = wxS( "explicit" );
                                    }
                                }

                                if( desc.Required && target == UNDEFINED_LAYER )
                                {
                                    warnings.push_back( wxString::Format(
                                            _( "No KiCad layer mapping for required source layer "
                                               "'%s'; its items will not be imported" ),
                                            desc.Name ) );
                                }

                                result.emplace( desc.Name, target );
                                capturedLayers.push_back( { desc.Name, target, method } );
                                seenSourceLayers.insert( desc.Name );
                            }

                            layersCaptured = true;
                            return result;
                        } );
            }
        }
        else if( !job->m_layerMap.empty() )
        {
            warnings.push_back( _( "A layer map was provided, but this importer does not support "
                                   "layer remapping; it will be ignored" ) );
        }

        m_reporter->Report(
                wxString::Format( _( "Importing '%s' using %s format...\n" ), job->m_inputFile, formatName ),
                RPT_SEVERITY_INFO );

        board = pi->LoadBoard( job->m_inputFile, nullptr, nullptr, nullptr );

        if( !board )
        {
            m_reporter->Report( _( "Failed to load board\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        // Flag explicit map entries that never matched a source layer so typos do not pass silently.
        if( layersCaptured )
        {
            for( const auto& [source, target] : job->m_layerMap )
            {
                if( !seenSourceLayers.contains( source ) )
                {
                    warnings.push_back( wxString::Format(
                            _( "Layer map entry '%s' does not match any source layer in the "
                               "imported file; it will be ignored" ),
                            source ) );
                }
            }
        }

        // Save as KiCad format
        IO_RELEASER<PCB_IO> kicadPlugin( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );
        kicadPlugin->SaveBoard( outputPath, board );

        m_reporter->Report( wxString::Format( _( "Successfully saved imported board to '%s'\n" ), outputPath ),
                            RPT_SEVERITY_INFO );

        // Generate report if requested
        if( job->m_reportFormat != IMPORT_REPORT_FORMAT::NONE )
        {
            IMPORT_REPORT_DATA reportData;

            reportData.m_sourceFile = wxFileName( job->m_inputFile ).GetFullName();
            reportData.m_sourceFormat = formatName;
            reportData.m_outputFile = wxFileName( outputPath ).GetFullName();

            size_t trackCount = 0;
            size_t viaCount = 0;

            for( PCB_TRACK* track : board->Tracks() )
            {
                if( track->Type() == PCB_VIA_T )
                    viaCount++;
                else
                    trackCount++;
            }

            reportData.m_statistics = {
                { wxS( "footprints" ), board->Footprints().size() },
                { wxS( "tracks" ), trackCount },
                { wxS( "vias" ), viaCount },
                { wxS( "zones" ), board->Zones().size() }
            };

            // Build layer mapping info, carried only in the JSON report.  Prefer the real
            // source-to-KiCad decisions captured during load; fall back to the imported board's
            // enabled layers for importers that do not expose a mappable layer set.
            nlohmann::json layerMappings = nlohmann::json::object();

            if( layersCaptured )
            {
                for( const CAPTURED_LAYER& mapped : capturedLayers )
                {
                    std::string kicadLayer = mapped.m_target == UNDEFINED_LAYER
                                                     ? std::string()
                                                     : LSET::Name( mapped.m_target ).ToStdString();

                    layerMappings[mapped.m_source.ToStdString()] = {
                        { "kicad_layer", kicadLayer },
                        { "method", mapped.m_method.ToStdString() }
                    };
                }
            }
            else
            {
                for( PCB_LAYER_ID layer : board->GetEnabledLayers().Seq() )
                {
                    wxString layerName = board->GetLayerName( layer );

                    layerMappings[layerName.ToStdString()] = {
                        { "kicad_layer", LSET::Name( layer ).ToStdString() },
                        { "method", "auto" }
                    };
                }
            }

            reportData.m_extraJson["layer_mapping"] = layerMappings;
            reportData.m_warnings = warnings;

            WriteImportReport( m_reporter, job->m_reportFormat, job->m_reportFile, reportData );
        }
        else
        {
            // No report requested, but explicit-mapping problems still need to surface.
            for( const wxString& warning : warnings )
                m_reporter->Report( warning + wxS( "\n" ), RPT_SEVERITY_WARNING );
        }

        delete board;
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Error during import: %s\n" ), ioe.What() ), RPT_SEVERITY_ERROR );

        delete board;
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return CLI::EXIT_CODES::SUCCESS;
}


// ============================================================================
// JobDiff: pcb_diff implementation
// ============================================================================
#include <diff_merge/diff_job_output.h>
#include <diff_merge/diff_renderer_plotter.h>
#include <diff_merge/diff_scene.h>
#include <diff_merge/pcb_differ.h>
#include <diff_merge/pcb_geometry_extractor.h>
#include <jobs/job_pcb_diff.h>


// Load a board into a SCRATCH_DOC<BOARD> that keeps its project attached for
// the document's lifetime — the differ/applier read PROJECT_FILE-scoped fields
// (drawing-sheet path, DRC severities, net classes). The destructor severs the
// BOARD->project link in the right order. Used by every PCB diff/merge job.
static SCRATCH_DOC<BOARD> loadScratchBoard( SETTINGS_MANAGER& aMgr, const wxString& aPath,
                                            bool aInitializeAfterLoad = true )
{
    return LoadScratchDoc<BOARD>(
            aMgr, aPath,
            [aPath, aInitializeAfterLoad]( PROJECT* aProject ) -> std::unique_ptr<BOARD>
            {
                PCB_IO_MGR::PCB_FILE_T pluginType =
                        PCB_IO_MGR::FindPluginTypeFromBoardPath( aPath, KICTL_KICAD_ONLY );

                if( !aProject || pluginType == PCB_IO_MGR::FILE_TYPE_NONE )
                    return nullptr;

                BOARD_LOADER::OPTIONS opts;
                opts.initialize_after_load = aInitializeAfterLoad;

                try
                {
                    return BOARD_LOADER::Load( aPath, pluginType, aProject, opts );
                }
                catch( ... )
                {
                    return nullptr;
                }
            },
            []( BOARD* aBoard )
            {
                if( aBoard )
                    aBoard->ClearProject();
            } );
}


int PCBNEW_JOBS_HANDLER::JobDiff( JOB* aJob )
{
    JOB_PCB_DIFF* diffJob = dynamic_cast<JOB_PCB_DIFF*>( aJob );

    if( !diffJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    // SCRATCH_DOC<BOARD> keeps each board's project attached for the lifetime
    // of the diff, which the differ needs to read project-file-scoped fields
    // (m_BoardDrawingSheetFile, etc). The previous loadStandaloneBoard +
    // ClearProject-up-front path would null those out before the differ ran.
    SETTINGS_MANAGER& diffMgr = Pgm().GetSettingsManager();

    SCRATCH_DOC<BOARD> aScratch = loadScratchBoard( diffMgr, diffJob->m_inputA );
    SCRATCH_DOC<BOARD> bScratch = loadScratchBoard( diffMgr, diffJob->m_inputB );

    BOARD* boardA = aScratch.doc.get();
    BOARD* boardB = bScratch.doc.get();

    if( !boardA )
    {
        m_reporter->Report( wxString::Format( _( "Failed to load %s\n" ), diffJob->m_inputA ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( !boardB )
    {
        m_reporter->Report( wxString::Format( _( "Failed to load %s\n" ), diffJob->m_inputB ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    KICAD_DIFF::PCB_DIFFER    differ( boardA, boardB, diffJob->m_inputB );
    KICAD_DIFF::DOCUMENT_DIFF result = differ.Diff();

    int diffExitCode = KICAD_DIFF::DiffExitCode( result );

    if( diffJob->m_exitCodeOnly )
        return diffExitCode;

    // The board geometry rendered beneath the change overlay (PNG/SVG only)
    // matches what the interactive dialog draws.
    KICAD_DIFF::DIFF_EMIT_OPTIONS emitOpts =
            KICAD_DIFF::MakeEmitOptions( *diffJob, diffJob->m_inputA, diffJob->m_inputB );
    emitOpts.docKind            = KICAD_DIFF::DOC_KIND::PCB;
    emitOpts.referenceGeometry  = [&]( const KIGFX::COLOR4D& aColor )
                                  { return KICAD_DIFF::ExtractBoardGeometry( *boardA, aColor ); };
    emitOpts.comparisonGeometry = [&]( const KIGFX::COLOR4D& aColor )
                                  { return KICAD_DIFF::ExtractBoardGeometry( *boardB, aColor ); };

    return KICAD_DIFF::EmitDiffResult( result, emitOpts, diffExitCode, *m_reporter );
}


// ============================================================================
// JobMerge: pcb_merge implementation
// ============================================================================
#include <diff_merge/pcb_geometry_extractor.h>
#include <diff_merge/pcb_merge_applier.h>
#include <diff_merge/kicad_merge_engine.h>
#include <dialogs/dialog_kicad_merge_3way.h>


int PCBNEW_JOBS_HANDLER::RunMerge( KICAD_DIFF::DOC_KIND aKind, const wxString& aAncestor,
                                   const wxString& aOurs, const wxString& aTheirs,
                                   const wxString& aOutput, bool aInteractive, bool aSingleFile,
                                   REPORTER* aReporter )
{
    // Restore m_reporter on scope exit so a caller's transient (often
    // stack-local) reporter doesn't outlive this call as a dangling member.
    SCOPED_SET_RESET<REPORTER*> reporterGuard( m_reporter,
                                               aReporter ? aReporter : m_reporter );

    if( aKind == KICAD_DIFF::DOC_KIND::FP_LIB || aKind == KICAD_DIFF::DOC_KIND::FOOTPRINT )
        return runFpLibMerge( aAncestor, aOurs, aTheirs, aOutput, aSingleFile );

    return runPcbMerge( aAncestor, aOurs, aTheirs, aOutput, aInteractive );
}


int PCBNEW_JOBS_HANDLER::runPcbMerge( const wxString& aAncestor, const wxString& aOurs,
                                      const wxString& aTheirs, const wxString& aOutput,
                                      bool aInteractive )
{
    // Use SCRATCH_DOC<BOARD> so each input keeps its project attached for the
    // life of the merge — necessary for any doc-level resolution that mutates
    // PROJECT_FILE-scoped state (DRC severities, net classes) and needs to be
    // saved as a sibling .kicad_pro. SCRATCH_DOC's destructor severs the
    // BOARD->project link in the right order and unloads the project from
    // the manager, avoiding the dangling-PROJECT_FILE::m_BoardSettings pointer
    // the previous up-front-ClearProject loadStandaloneBoard path had to
    // guard against.
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    SCRATCH_DOC<BOARD> ancestorScratch = loadScratchBoard( mgr, aAncestor );
    SCRATCH_DOC<BOARD> oursScratch = loadScratchBoard( mgr, aOurs );
    SCRATCH_DOC<BOARD> theirsScratch = loadScratchBoard( mgr, aTheirs );

    BOARD* ancestor = ancestorScratch.doc.get();
    BOARD* ours = oursScratch.doc.get();
    BOARD* theirs = theirsScratch.doc.get();

    if( !ancestor || !ours || !theirs )
    {
        m_reporter->Report( _( "Failed to load one or more input boards\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    KICAD_DIFF::PCB_DIFFER ourDiff( ancestor, ours );
    KICAD_DIFF::PCB_DIFFER theirDiff( ancestor, theirs );

    KICAD_DIFF::DOCUMENT_DIFF ourDocDiff = ourDiff.Diff();
    KICAD_DIFF::DOCUMENT_DIFF theirDocDiff = theirDiff.Diff();

    KICAD_DIFF::KICAD_MERGE_ENGINE engine;
    KICAD_DIFF::MERGE_PLAN         plan = engine.Plan( ourDocDiff, theirDocDiff );

    // A cancelled dialog leaves plan unresolved and falls through to the
    // marker flow below.
    if( aInteractive && !plan.Resolved() )
    {
        if( !Pgm().IsGUI() )
        {
            m_reporter->Report( _( "--interactive requires a GUI KiCad process; the console "
                                   "kicad-cli cannot open dialogs.\n" ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }

        // Geometry context so the conflict viewer can render the actual
        // boards behind the conflict bbox highlight.
        const KICAD_DIFF::DIFF_COLOR_THEME        theme;
        DIALOG_KICAD_MERGE_3WAY::CONFLICT_CONTEXT ctx;
        ctx.ancestorGeometry = KICAD_DIFF::ExtractBoardGeometry( *ancestor, theme.reference );
        ctx.oursGeometry = KICAD_DIFF::ExtractBoardGeometry( *ours, theme.reference );
        ctx.theirsGeometry = KICAD_DIFF::ExtractBoardGeometry( *theirs, theme.comparison );

        // Build per-side bbox lookups so a "moved on theirs" item highlights
        // at its theirs-side coordinates when the user previews Theirs.
        KICAD_DIFF::CollectChangeBBoxes( ourDocDiff, ctx.oursBBoxes );
        KICAD_DIFF::CollectChangeBBoxes( theirDocDiff, ctx.theirsBBoxes );

        DIALOG_KICAD_MERGE_3WAY dlg( wxTheApp->GetTopWindow(), plan, std::move( ctx ) );

        if( dlg.ShowModal() == wxID_APPLY )
            plan = dlg.GetResolvedPlan();
    }

    // Snapshot of the plan before the applier moves it; drives the
    // unresolved-conflict report below.
    const KICAD_DIFF::MERGE_PLAN planSnapshot = plan;

    KICAD_DIFF::PCB_MERGE_APPLIER applier( ancestor, ours, theirs, std::move( plan ) );
    std::unique_ptr<BOARD>        merged = applier.Apply();

    if( !merged )
    {
        m_reporter->Report( _( "Merge applier failed to produce a board\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    // Serialize to the output path using the canonical PCB IO.
    PCB_IO_KICAD_SEXPR pcbIO;

    try
    {
        pcbIO.SaveBoard( aOutput, merged.get() );
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Failed to save merged board: %s\n" ), ioe.What() ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    // BOARD_DESIGN_SETTINGS fields like m_DRCSeverities serialize to
    // .kicad_pro, not .kicad_pcb. Mirror only those specific fields onto
    // ancestor (still linked to its project via BOARD::SetProject) then
    // save ancestor's project alongside the merged board file. Whole-
    // BOARD_DESIGN_SETTINGS copy would alias shared_ptr<NET_SETTINGS>
    // across BOARDs and crash on ClearProject during SCRATCH_DOC release;
    // single-field mirror avoids that.
    if( applier.GetReport().projectFileTouched && ancestor && ancestor->GetProject() )
    {
        ancestor->GetDesignSettings().m_DRCSeverities = merged->GetDesignSettings().m_DRCSeverities;

        // Mirror net settings (the applier copied them onto the result via
        // NET_SETTINGS::CopyFrom; do the same here to ancestor, which still
        // owns the project's nested-settings registration so SaveProjectCopy
        // walks the right entry).
        if( ancestor->GetDesignSettings().m_NetSettings && merged->GetDesignSettings().m_NetSettings )
        {
            ancestor->GetDesignSettings().m_NetSettings->CopyFrom( *merged->GetDesignSettings().m_NetSettings );
        }

        // The applier stages drawing-sheet resolutions on the report (the
        // result BOARD is project-less). Mirror onto ancestor's project here
        // before SaveProjectCopy walks the PROJECT_FILE.
        if( applier.GetReport().drawingSheetFileSet )
        {
            ancestor->GetProject()->GetProjectFile().m_BoardDrawingSheetFile = applier.GetReport().drawingSheetFile;
        }

        wxFileName proFn( aOutput );
        proFn.SetExt( FILEEXT::ProjectFileExtension );

        // JSON-patch path: flush ancestor's in-memory project to its JSON
        // cache, then patch only the diffed DOC_PROP fields onto the output
        // file.  This preserves any non-diffed fields the user had at the
        // output path (text variables, last paths, layer presets etc.) that
        // a full SaveProjectCopy would silently overwrite.
        PROJECT_FILE& ancProj = ancestor->GetProject()->GetProjectFile();
        ancProj.Store();

        const KICAD_DIFF::PCB_MERGE_APPLIER::REPORT& mergeReport = applier.GetReport();

        // PROJECT_FILE::Store() flushes the project file's own params but not
        // its registered NESTED_SETTINGS. Flush only the nested settings the
        // merge resolved so the surgical patch does not overwrite unrelated
        // project subtrees.
        if( mergeReport.drcSeveritiesTouched )
            ancestor->GetDesignSettings().SaveToFile( wxEmptyString, true );

        if( mergeReport.netClassesTouched && ancestor->GetDesignSettings().m_NetSettings )
            ancestor->GetDesignSettings().m_NetSettings->SaveToFile( wxEmptyString, true );

        std::set<wxString> touched;
        if( mergeReport.drcSeveritiesTouched )
            touched.insert( KICAD_DIFF::DOC_PROP_DRC_SEVERITIES );

        if( mergeReport.netClassesTouched )
            touched.insert( KICAD_DIFF::DOC_PROP_NET_CLASSES );

        if( applier.GetReport().drawingSheetFileSet )
            touched.insert( KICAD_DIFF::DOC_PROP_DRAWING_SHEET );

        if( !KICAD_DIFF::ApplyProjectFilePatches( proFn.GetFullPath(), *ancProj.Internals(), touched ) )
        {
            // Patch failed (existing output unparseable or write error).
            // Fall back to the legacy full-copy path so the user still gets
            // a project file even if it overwrites non-diffed customisations.
            if( !mgr.SaveProjectCopy( proFn.GetFullPath(), ancestor->GetProject() ) )
            {
                m_reporter->Report(
                        wxString::Format( _( "Failed to save merged project file: %s\n" ), proFn.GetFullPath() ),
                        RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }

        // Write a project-dir sibling file from staged report content.  Empty
        // content removes the file so a TAKE_ANCESTOR resolution against an
        // ancestor with no file clears stale content at the output path.
        auto writeStagedFile = [&]( const wxString& aPath, const wxString& aContent, const wxString& aLabel ) -> bool
        {
            if( aContent.IsEmpty() )
            {
                if( wxFileExists( aPath ) )
                    wxRemoveFile( aPath );

                return true;
            }

            wxFile out;

            if( !out.Open( aPath, wxFile::write ) || !out.Write( aContent ) )
            {
                m_reporter->Report( wxString::Format( _( "Failed to save merged %s: %s\n" ), aLabel, aPath ),
                                    RPT_SEVERITY_ERROR );
                return false;
            }

            return true;
        };

        // Custom DRC rules: write the applier's staged content next to the
        // merged board so the chosen side's rules apply at next DRC run.
        if( applier.GetReport().customDrcRulesSet )
        {
            wxFileName druFn( aOutput );
            druFn.SetExt( FILEEXT::DesignRulesFileExtension );

            if( !writeStagedFile( druFn.GetFullPath(), applier.GetReport().customDrcRules, _( "custom DRC rules" ) ) )
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }

        // Footprint / symbol library tables: write into the merged project
        // directory.  Both files have no extension.
        if( applier.GetReport().fpLibTableSet )
        {
            wxFileName fpFn( aOutput );
            fpFn.SetFullName( wxString::FromUTF8( FILEEXT::FootprintLibraryTableFileName ) );

            if( !writeStagedFile( fpFn.GetFullPath(), applier.GetReport().fpLibTable, _( "footprint library table" ) ) )
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }

        if( applier.GetReport().symLibTableSet )
        {
            wxFileName symFn( aOutput );
            symFn.SetFullName( wxString::FromUTF8( FILEEXT::SymbolLibraryTableFileName ) );

            if( !writeStagedFile( symFn.GetFullPath(), applier.GetReport().symLibTable, _( "symbol library table" ) ) )
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }
    }

    // Surface post-apply validator findings (refdes collisions, schema
    // mismatch, missed connectivity rebuild). Advisory — they do not change the
    // exit code, only the merge's resolved/unresolved status does.
    for( const KICAD_DIFF::VALIDATION_FAILURE& f : applier.GetReport().validation.failures )
        m_reporter->Report( wxString::Format( wxS( "%s: %s\n" ), f.validator, f.message ), f.severity );

    // The merged board was written to m_outputPath above, so the output is
    // always a valid file. Unresolved conflicts are reported and signalled via
    // the exit code; the user resolves them with the interactive mergetool.
    if( !planSnapshot.Resolved() )
    {
        m_reporter->Report( wxString::Format( _( "Merge completed with %zu unresolved conflict(s) in %s\n" ),
                                              planSnapshot.ConflictCount(), aOutput ),
                            RPT_SEVERITY_WARNING );
        return CLI::EXIT_CODES::ERR_RC_VIOLATIONS;
    }

    return CLI::EXIT_CODES::SUCCESS;
}


// ============================================================================
// JobFpDiff: fp_diff implementation
// ============================================================================
#include <diff_merge/fp_lib_differ.h>
#include <jobs/job_fp_diff.h>


// Load one side of a footprint-library diff into its owner vector and name map.
// When aAllowEmpty is set an empty path resolves to a clean (empty) side; the
// non-interactive job path leaves it unset so a missing path is an input error.
static int loadFootprintLibrarySide( const wxString& aPath,
                                     std::vector<std::unique_ptr<FOOTPRINT>>& aOwners,
                                     KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP& aMap, bool aAllowEmpty,
                                     REPORTER& aReporter )
{
    if( aAllowEmpty && aPath.IsEmpty() )
        return CLI::EXIT_CODES::SUCCESS;

    try
    {
        auto loaded = KICAD_DIFF::FP_LIB_DIFFER::LoadLibrary( aPath );
        aOwners = std::move( loaded.first );
        aMap = std::move( loaded.second );
        return CLI::EXIT_CODES::SUCCESS;
    }
    catch( const IO_ERROR& ioe )
    {
        aReporter.Report( wxString::Format( _( "Failed to load %s: %s\n" ), aPath, ioe.What() ),
                          RPT_SEVERITY_ERROR );
    }
    catch( const std::exception& e )
    {
        aReporter.Report(
                wxString::Format( _( "Failed to load %s: %s\n" ), aPath, wxString::FromUTF8( e.what() ) ),
                RPT_SEVERITY_ERROR );
    }

    return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
}


// Flatten a footprint-library name map into a single DOCUMENT_GEOMETRY tinted
// with the supplied per-side theme colour.
static KICAD_DIFF::DOCUMENT_GEOMETRY
footprintLibraryGeometry( const KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP& aMap, const KIGFX::COLOR4D& aColor )
{
    KICAD_DIFF::DOCUMENT_GEOMETRY geometry;

    for( const auto& [name, footprint] : aMap )
    {
        if( footprint )
            KICAD_DIFF::AppendGeometry( geometry, KICAD_DIFF::ExtractFootprintGeometry( *footprint, aColor ) );
    }

    return geometry;
}


int PCBNEW_JOBS_HANDLER::JobFpDiff( JOB* aJob )
{
    JOB_FP_DIFF* diffJob = dynamic_cast<JOB_FP_DIFF*>( aJob );

    if( !diffJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    wxFileName dirA( diffJob->m_inputA );
    dirA.MakeAbsolute();
    wxFileName dirB( diffJob->m_inputB );
    dirB.MakeAbsolute();

    std::vector<std::unique_ptr<FOOTPRINT>>  ownersA;
    std::vector<std::unique_ptr<FOOTPRINT>>  ownersB;
    KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP mapA;
    KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP mapB;

    if( int rc = loadFootprintLibrarySide( dirA.GetFullPath(), ownersA, mapA, false, *m_reporter );
        rc != CLI::EXIT_CODES::SUCCESS )
    {
        return rc;
    }

    if( int rc = loadFootprintLibrarySide( dirB.GetFullPath(), ownersB, mapB, false, *m_reporter );
        rc != CLI::EXIT_CODES::SUCCESS )
    {
        return rc;
    }

    KICAD_DIFF::FP_LIB_DIFFER differ( mapA, mapB, diffJob->m_inputB );
    KICAD_DIFF::DOCUMENT_DIFF result = differ.Diff();

    int diffExitCode = KICAD_DIFF::DiffExitCode( result );

    if( diffJob->m_exitCodeOnly )
        return diffExitCode;

    KICAD_DIFF::DIFF_EMIT_OPTIONS emitOpts =
            KICAD_DIFF::MakeEmitOptions( *diffJob, diffJob->m_inputA, diffJob->m_inputB );
    emitOpts.docKind            = KICAD_DIFF::DOC_KIND::FP_LIB;
    emitOpts.referenceGeometry  = [&]( const KIGFX::COLOR4D& aColor )
                                  { return footprintLibraryGeometry( mapA, aColor ); };
    emitOpts.comparisonGeometry = [&]( const KIGFX::COLOR4D& aColor )
                                  { return footprintLibraryGeometry( mapB, aColor ); };

    return KICAD_DIFF::EmitDiffResult( result, emitOpts, diffExitCode, *m_reporter );
}


// ============================================================================
// JobOpenDiffDialog: load two on-disk files and open DIALOG_KICAD_DIFF.
// Dispatched from the project manager / PR-review dialog via KIWAY.
// ============================================================================
#include <dialogs/dialog_kicad_diff.h>
#include <diff_merge/pcb_diff_canvas_context.h>
#include <diff_merge/pcb_geometry_extractor.h>
#include <jobs/scratch_doc.h>


int PCBNEW_JOBS_HANDLER::OpenDiffDialog( KICAD_DIFF::DOC_KIND aKind, const wxString& aFileA,
                                         const wxString& aFileB, const wxString& aLabelA,
                                         const wxString& aLabelB, wxWindow* aParent,
                                         REPORTER* aReporter )
{
    // Restore m_reporter on scope exit so a caller's transient (often
    // stack-local) reporter doesn't outlive this call as a dangling member.
    SCOPED_SET_RESET<REPORTER*> reporterGuard( m_reporter,
                                               aReporter ? aReporter : m_reporter );

    wxWindow* parent = aParent ? aParent : ( wxTheApp ? wxTheApp->GetTopWindow() : nullptr );

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    auto loadBoardScratch = [&]( const wxString& aPath )
    {
        return loadScratchBoard( mgr, aPath, /* aInitializeAfterLoad */ false );
    };

    KICAD_DIFF::DOCUMENT_DIFF     result;
    KICAD_DIFF::DOCUMENT_GEOMETRY refGeometry;
    KICAD_DIFF::DOCUMENT_GEOMETRY compGeometry;

    auto loadFootprintFile = [&]( const wxString& aPath ) -> std::unique_ptr<FOOTPRINT>
    {
        if( aPath.IsEmpty() )
            return nullptr;

        wxFileName fn( aPath );
        fn.MakeAbsolute();

        // A single .kicad_mod's internal (footprint ...) name need not match its
        // filename, so load the file's sole footprint via ImportFootprint rather
        // than FootprintLoad (which treats the directory as a .pretty library and
        // keys by basename), matching runFpLibMerge's single-file path.
        PCB_IO_KICAD_SEXPR io;
        wxString           name;
        return std::unique_ptr<FOOTPRINT>( io.ImportFootprint( fn.GetFullPath(), name ) );
    };

    switch( aKind )
    {
    case KICAD_DIFF::DOC_KIND::PCB:
    {
        SCRATCH_DOC<BOARD> a = loadBoardScratch( aFileA );
        SCRATCH_DOC<BOARD> b = loadBoardScratch( aFileB );

        // Synthesize empty boards for ADDED / REMOVED sides so the differ
        // can still produce a meaningful per-item list rather than failing
        // on an empty input file.
        BOARD emptyA;
        BOARD emptyB;

        if( !a.doc && !aFileA.IsEmpty() )
        {
            m_reporter->Report( wxString::Format( _( "Failed to load %s\n" ), aFileA ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        if( !b.doc && !aFileB.IsEmpty() )
        {
            m_reporter->Report( wxString::Format( _( "Failed to load %s\n" ), aFileB ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        BOARD* boardA = a.doc ? a.doc.get() : &emptyA;
        BOARD* boardB = b.doc ? b.doc.get() : &emptyB;

        KICAD_DIFF::PCB_DIFFER differ( boardA, boardB, aFileB );
        result = differ.Diff();

        // Extract background geometry so the dialog's canvas shows the
        // actual board outline + footprint footprints beneath the diff
        // bbox rectangles. Theme defaults: muted blue (ref) / gold (comp).
        const KICAD_DIFF::DIFF_COLOR_THEME theme;
        refGeometry = KICAD_DIFF::ExtractBoardGeometry( *boardA, theme.reference );
        compGeometry = KICAD_DIFF::ExtractBoardGeometry( *boardB, theme.comparison );

        const wxString labelA = aLabelA.IsEmpty() ? aFileA : aLabelA;
        const wxString labelB = aLabelB.IsEmpty() ? aFileB : aLabelB;

        DIALOG_KICAD_DIFF dlg(
                parent, labelA, labelB, result, std::move( refGeometry ), std::move( compGeometry ),
                [boardA, boardB, color = theme.reference]( WIDGET_DIFF_CANVAS& aCanvas, const KIID_PATH& )
                {
                    KICAD_DIFF::ConfigurePcbDiffCanvasContext( aCanvas, boardA, boardB, color );
                } );
        dlg.ShowModal();

        return CLI::EXIT_CODES::SUCCESS;
    }
    case KICAD_DIFF::DOC_KIND::FP_LIB:
    {
        std::vector<std::unique_ptr<FOOTPRINT>>  ownersA;
        std::vector<std::unique_ptr<FOOTPRINT>>  ownersB;
        KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP mapA;
        KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP mapB;

        if( int rc = loadFootprintLibrarySide( aFileA, ownersA, mapA, true, *m_reporter );
            rc != CLI::EXIT_CODES::SUCCESS )
        {
            return rc;
        }

        if( int rc = loadFootprintLibrarySide( aFileB, ownersB, mapB, true, *m_reporter );
            rc != CLI::EXIT_CODES::SUCCESS )
        {
            return rc;
        }

        KICAD_DIFF::FP_LIB_DIFFER differ( mapA, mapB, aFileB );
        result = differ.Diff();

        const KICAD_DIFF::DIFF_COLOR_THEME theme;
        refGeometry = footprintLibraryGeometry( mapA, theme.reference );
        compGeometry = footprintLibraryGeometry( mapB, theme.comparison );
        break;
    }
    case KICAD_DIFF::DOC_KIND::FOOTPRINT:
    {
        std::unique_ptr<FOOTPRINT> footprintA;
        std::unique_ptr<FOOTPRINT> footprintB;

        try
        {
            footprintA = loadFootprintFile( aFileA );
        }
        catch( const IO_ERROR& ioe )
        {
            m_reporter->Report( wxString::Format( _( "Failed to load %s: %s\n" ), aFileA, ioe.What() ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        try
        {
            footprintB = loadFootprintFile( aFileB );
        }
        catch( const IO_ERROR& ioe )
        {
            m_reporter->Report( wxString::Format( _( "Failed to load %s: %s\n" ), aFileB, ioe.What() ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP mapA;
        KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP mapB;
        const wxString                           nameA = wxFileName( aFileA ).GetName();
        const wxString                           nameB = wxFileName( aFileB ).GetName();
        const wxString                           itemName = !nameB.IsEmpty() ? nameB : nameA;

        if( footprintA )
            mapA[itemName] = footprintA.get();

        if( footprintB )
            mapB[itemName] = footprintB.get();

        KICAD_DIFF::FP_LIB_DIFFER differ( mapA, mapB, aFileB );
        result = differ.Diff();

        const KICAD_DIFF::DIFF_COLOR_THEME theme;

        if( footprintA )
            refGeometry = KICAD_DIFF::ExtractFootprintGeometry( *footprintA, theme.reference );

        if( footprintB )
            compGeometry = KICAD_DIFF::ExtractFootprintGeometry( *footprintB, theme.comparison );

        break;
    }
    default:
        m_reporter->Report( _( "Unsupported document kind for this dispatcher.\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    const wxString labelA = aLabelA.IsEmpty() ? aFileA : aLabelA;
    const wxString labelB = aLabelB.IsEmpty() ? aFileB : aLabelB;

    DIALOG_KICAD_DIFF dlg( parent, labelA, labelB, result, std::move( refGeometry ), std::move( compGeometry ) );
    dlg.ShowModal();

    return CLI::EXIT_CODES::SUCCESS;
}


// ============================================================================
// JobFpLibMerge: 3-way merge of .pretty footprint libraries.
// ============================================================================
#include <diff_merge/fp_lib_differ.h>
#include <diff_merge/fp_lib_merge_applier.h>


int PCBNEW_JOBS_HANDLER::runFpLibMerge( const wxString& aAncestor, const wxString& aOurs,
                                        const wxString& aTheirs, const wxString& aOutput,
                                        bool aSingleFile )
{
    if( aOutput.IsEmpty() )
    {
        m_reporter->Report( _( "--output is required\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    struct LIB_SIDE
    {
        std::vector<std::unique_ptr<FOOTPRINT>>  owners;
        KICAD_DIFF::FP_LIB_DIFFER::FOOTPRINT_MAP map;
    };

    LIB_SIDE ancestor, ours, theirs;

    // Accept either a `.pretty` directory (library mode) or a single `.kicad_
    // mod` file (git's per-file driver mode). Extension autodetection works
    // for native invocations, but git's external driver passes temp paths
    // (`.merge_file_XXX`) with no extension, so the `--single-file` flag
    // overrides on demand.
    auto isSingleFile = [&]( const wxString& aPath )
    {
        if( aSingleFile )
            return true;

        return wxFileName( aPath ).GetExt() == FILEEXT::KiCadFootprintFileExtension;
    };

    auto loadSide = [&]( const wxString& aPath, LIB_SIDE& aSide ) -> int
    {
        try
        {
            if( isSingleFile( aPath ) )
            {
                PCB_IO_KICAD_SEXPR         io;
                wxString                   name;
                std::unique_ptr<FOOTPRINT> fp( io.ImportFootprint( aPath, name ) );

                if( !fp )
                    return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

                // Use the footprint's own item-name (LIB_ID) if set,
                // falling back to the file basename. Both sides must
                // agree for the differ/applier to align them.
                const UTF8&    itemName = fp->GetFPID().GetLibItemName();
                const wxString key = itemName.empty() ? name : itemName.wx_str();

                aSide.map[key] = fp.get();
                aSide.owners.push_back( std::move( fp ) );
                return CLI::EXIT_CODES::SUCCESS;
            }

            auto loaded = KICAD_DIFF::FP_LIB_DIFFER::LoadLibrary( aPath );
            aSide.owners = std::move( loaded.first );
            aSide.map = std::move( loaded.second );
            return CLI::EXIT_CODES::SUCCESS;
        }
        catch( const IO_ERROR& ioe )
        {
            m_reporter->Report( wxString::Format( _( "Failed to load %s: %s\n" ), aPath, ioe.What() ),
                                RPT_SEVERITY_ERROR );
        }
        catch( const std::exception& e )
        {
            m_reporter->Report(
                    wxString::Format( _( "Failed to load %s: %s\n" ), aPath, wxString::FromUTF8( e.what() ) ),
                    RPT_SEVERITY_ERROR );
        }

        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    };

    if( int rc = loadSide( aAncestor, ancestor ); rc != CLI::EXIT_CODES::SUCCESS )
        return rc;

    if( int rc = loadSide( aOurs, ours ); rc != CLI::EXIT_CODES::SUCCESS )
        return rc;

    if( int rc = loadSide( aTheirs, theirs ); rc != CLI::EXIT_CODES::SUCCESS )
        return rc;

    KICAD_DIFF::FP_LIB_DIFFER ourDiff( ancestor.map, ours.map, aOurs );
    KICAD_DIFF::FP_LIB_DIFFER theirDiff( ancestor.map, theirs.map, aTheirs );

    KICAD_DIFF::DOCUMENT_DIFF ourDocDiff = ourDiff.Diff();
    KICAD_DIFF::DOCUMENT_DIFF theirDocDiff = theirDiff.Diff();

    KICAD_DIFF::KICAD_MERGE_ENGINE engine;
    KICAD_DIFF::MERGE_PLAN         plan = engine.Plan( ourDocDiff, theirDocDiff );

    const KICAD_DIFF::MERGE_PLAN planSnapshot = plan;

    KICAD_DIFF::FP_LIB_MERGE_APPLIER        applier( ancestor.map, ours.map, theirs.map, std::move( plan ) );
    std::vector<std::unique_ptr<FOOTPRINT>> merged = applier.Apply();

    // Per-property footprint merge isn't implemented; MERGE_PROPS resolutions
    // are downgraded to TAKE_OURS. Surface that as unresolved so the user sees
    // a marker instead of silent partial-merge.
    const bool hadSilentFallback = applier.GetReport().mergePropsFallback > 0;

    const bool singleFileOutput = isSingleFile( aOutput );

    // .pretty is a directory; .kicad_mod is a single file. wxFileName parses
    // a path ending in `.pretty` as a file with that extension, so library
    // mode uses DirName(); single-file mode keeps the file path as-is and
    // hands it directly to FootprintSave, which auto-detects .kicad_mod via
    // its own extension check.
    wxFileName outFn;

    if( singleFileOutput )
        outFn = wxFileName( aOutput );
    else
        outFn = wxFileName::DirName( aOutput );

    outFn.MakeAbsolute();

    // In library mode wxFileName::DirName treats `foo.pretty` as a directory,
    // so GetPath() returns the .pretty itself. In single-file mode GetPath()
    // returns the file's parent dir. Either way it's the directory we Mkdir
    // into.
    const wxString outDir = outFn.GetPath();

    if( !wxFileName::DirExists( outDir ) && !wxFileName::Mkdir( outDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        m_reporter->Report( wxString::Format( _( "Cannot create output directory %s\n" ), outDir ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    try
    {
        PCB_IO_KICAD_SEXPR io;

        if( singleFileOutput )
        {
            // Git per-file driver mode: one merged footprint -> one .kicad_mod.
            // Multiple survivors would lose data; flag that as an error since
            // single-file input by definition has at most one footprint per
            // side.
            if( merged.size() > 1 )
            {
                m_reporter->Report( _( "Single-file fp merge produced multiple footprints; refusing to "
                                       "collapse into one .kicad_mod\n" ),
                                    RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }

            if( merged.empty() )
            {
                // All sides deleted the footprint. Remove the output file if
                // it existed, leaving nothing where the merged content would
                // have gone.
                if( wxFileName::FileExists( outFn.GetFullPath() ) )
                    wxRemoveFile( outFn.GetFullPath() );
            }
            else if( wxFileName( outFn.GetFullPath() ).GetExt() == FILEEXT::KiCadFootprintFileExtension )
            {
                // FootprintSave's .kicad_mod extension autodetection handles
                // the write to the path as-given.
                io.FootprintSave( outFn.GetFullPath(), merged.front().get(), nullptr );
            }
            else
            {
                // Git driver mode: output is an extension-less temp path
                // (typically `.merge_file_XXX`). FootprintSave's
                // autodetection would treat it as a library directory.
                // Format directly via PRETTIFIED_FILE_OUTPUTFORMATTER, the
                // same writer the sexpr lib cache uses.
                PRETTIFIED_FILE_OUTPUTFORMATTER formatter( outFn.GetFullPath() );
                io.SetOutputFormatter( &formatter );
                io.Format( merged.front().get() );
                formatter.Finish();
            }
        }
        else
        {
            // Library mode. Footprints in `merged` are the survivors. Any
            // footprint already in the output `.pretty` but absent from
            // `merged` is a stale leftover from a previous invocation (or a
            // resolved DELETE / TAKE_ANCESTOR-with-no-ancestor case). Delete
            // those before saving the survivors, otherwise the resolved
            // DELETE never propagates to disk.
            std::set<wxString> mergedNames;

            for( const auto& fp : merged )
            {
                if( fp )
                    mergedNames.insert( fp->GetFPID().GetLibItemName() );
            }

            wxArrayString existing;
            io.FootprintEnumerate( existing, outDir, false, nullptr );

            for( const wxString& name : existing )
            {
                if( !mergedNames.count( name ) )
                    io.FootprintDelete( outDir, name, nullptr );
            }

            for( const auto& fp : merged )
            {
                if( !fp )
                    continue;

                const wxString name = fp->GetFPID().GetLibItemName();

                if( io.FootprintExists( outDir, name, nullptr ) )
                    io.FootprintDelete( outDir, name, nullptr );

                io.FootprintSave( outDir, fp.get(), nullptr );
            }
        }
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Failed to save merged footprint library: %s\n" ), ioe.What() ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    // The merged library was saved above, so the output is always valid.
    if( !planSnapshot.Resolved() || hadSilentFallback )
    {
        // Conflict count = engine-unresolved ∪ applier-downgraded (deduped, so
        // an item that was both unresolved and silently downgraded counts once).
        std::set<KIID_PATH> conflicts( planSnapshot.unresolved.begin(), planSnapshot.unresolved.end() );

        for( const KIID_PATH& id : applier.GetReport().mergePropsFallbackIds )
            conflicts.insert( id );

        m_reporter->Report( wxString::Format( _( "Footprint library merge completed with %zu unresolved "
                                                 "conflict(s) in %s\n" ),
                                              conflicts.size(), aOutput ),
                            RPT_SEVERITY_WARNING );
        return CLI::EXIT_CODES::ERR_RC_VIOLATIONS;
    }

    return CLI::EXIT_CODES::SUCCESS;
}
