/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/dir.h>
#include "pcbnew_jobs_handler.h"
#include <board_commit.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_report.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <jobs/job_fp_export_svg.h>
#include <jobs/job_fp_upgrade.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_export_pcb_gerber.h>
#include <jobs/job_export_pcb_gerbers.h>
#include <jobs/job_export_pcb_drill.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_gencad.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_pos.h>
#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/job_pcb_render.h>
#include <jobs/job_pcb_drc.h>
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
#include <gendrill_Excellon_writer.h>
#include <gendrill_gerber_writer.h>
#include <kiface_base.h>
#include <macros.h>
#include <pad.h>
#include <pcb_marker.h>
#include <project/project_file.h>
#include <exporters/export_svg.h>
#include <exporters/export_gencad_writer.h>
#include <kiface_ids.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/netlist_reader.h>
#include <pcbnew_settings.h>
#include <pcbplot.h>
#include <pgm_base.h>
#include <3d_rendering/raytracing/render_3d_raytrace_ram.h>
#include <3d_rendering/track_ball.h>
#include <project_pcb.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <export_vrml.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <settings/settings_manager.h>
#include <dialogs/dialog_export_svg.h>
#include <dialogs/dialog_gendrill.h>
#include <dialogs/dialog_gen_footprint_position.h>
#include <dialogs/dialog_export_2581.h>
#include <dialogs/dialog_export_odbpp.h>
#include <dialogs/dialog_export_step.h>
#include <dialogs/dialog_plot.h>
#include <dialogs/dialog_drc_job_config.h>
#include <dialogs/dialog_render_job.h>

#include "pcbnew_scripting_helpers.h"
#include <locale_io.h>


#ifdef _WIN32
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif
#endif


PCBNEW_JOBS_HANDLER::PCBNEW_JOBS_HANDLER( KIWAY* aKiway ) :
        JOB_DISPATCHER( aKiway ),
        m_cliBoard( nullptr )
{
    Register( "3d", std::bind( &PCBNEW_JOBS_HANDLER::JobExportStep, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_3D* svgJob = dynamic_cast<JOB_EXPORT_PCB_3D*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );
                  DIALOG_EXPORT_STEP dlg( editFrame, aParent, "", svgJob );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "render",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportRender, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  DIALOG_RENDER_JOB dlg( aParent, dynamic_cast<JOB_PCB_RENDER*>( job ) );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "svg", std::bind( &PCBNEW_JOBS_HANDLER::JobExportSvg, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_SVG* svgJob = dynamic_cast<JOB_EXPORT_PCB_SVG*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  DIALOG_PLOT dlg( editFrame, aParent, svgJob );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "gencad",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportGencad, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return false;
              } );
    Register( "dxf", std::bind( &PCBNEW_JOBS_HANDLER::JobExportDxf, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_DXF* dxfJob = dynamic_cast<JOB_EXPORT_PCB_DXF*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  DIALOG_PLOT dlg( editFrame, aParent, dxfJob );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "pdf", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPdf, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_PDF* pdfJob = dynamic_cast<JOB_EXPORT_PCB_PDF*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  DIALOG_PLOT dlg( editFrame, aParent, pdfJob );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "gerber",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportGerber, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_GERBER* gJob = dynamic_cast<JOB_EXPORT_PCB_GERBER*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  DIALOG_PLOT dlg( editFrame, aParent, gJob );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "gerbers",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportGerbers, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_GERBERS* gJob = dynamic_cast<JOB_EXPORT_PCB_GERBERS*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  DIALOG_PLOT dlg( editFrame, aParent, gJob );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "drill",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportDrill, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_DRILL* drillJob = dynamic_cast<JOB_EXPORT_PCB_DRILL*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );
                  DIALOG_GENDRILL dlg( editFrame, drillJob, aParent );
                  dlg.ShowModal();
                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "pos", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPos, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
			  {
				  JOB_EXPORT_PCB_POS* posJob = dynamic_cast<JOB_EXPORT_PCB_POS*>( job );

				  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );
				  DIALOG_GEN_FOOTPRINT_POSITION dlg( posJob, editFrame, aParent );
                  dlg.ShowModal();
                  return dlg.GetReturnCode() == wxID_OK;
			  } );
    Register( "fpupgrade",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportFpUpgrade, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return false;
              } );
    Register( "fpsvg",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportFpSvg, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return false;
              } );
    Register( "drc", std::bind( &PCBNEW_JOBS_HANDLER::JobExportDrc, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  DIALOG_DRC_JOB_CONFIG dlg( aParent, dynamic_cast<JOB_PCB_DRC*>( job ) );

                  return dlg.ShowModal() == wxID_SAVE;
              } );
    Register( "ipc2581",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportIpc2581, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_IPC2581* ipcJob = dynamic_cast<JOB_EXPORT_PCB_IPC2581*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );
                  wxCHECK( ipcJob && editFrame, false );

                  DIALOG_EXPORT_2581 dlg( ipcJob, editFrame, aParent );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
    Register( "odb",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportOdb, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_ODB* odbJob = dynamic_cast<JOB_EXPORT_PCB_ODB*>( job );
                  wxCHECK( odbJob, false );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  DIALOG_EXPORT_ODBPP dlg( odbJob, editFrame, aParent );
                  dlg.ShowModal();

                  return dlg.GetReturnCode() == wxID_OK;
              } );
}


BOARD* PCBNEW_JOBS_HANDLER::getBoard( const wxString& aPath )
{
    BOARD* brd = nullptr;

    if( !Pgm().IsGUI() &&
        Pgm().GetSettingsManager().IsProjectOpen() )
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
        {
            m_reporter->Report( _( "Loading board\n" ), RPT_SEVERITY_INFO );
            m_cliBoard = LoadBoard( pcbPath, true );
        }

        brd = m_cliBoard;
    }
    else if( Pgm().IsGUI() && Pgm().GetSettingsManager().IsProjectOpen() )
    {
        PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_kiway->Player( FRAME_PCB_EDITOR, false ) );

        if( editFrame )
        {
            brd = editFrame->GetBoard();
        }
    }
    else
    {
        m_reporter->Report( _( "Loading board\n" ), RPT_SEVERITY_INFO );
        brd = LoadBoard( aPath, true );
    }

    if ( !brd )
    {
        m_reporter->Report( _( "Failed to load board\n" ), RPT_SEVERITY_ERROR );
    }

    return brd;
}


int PCBNEW_JOBS_HANDLER::JobExportStep( JOB* aJob )
{
    JOB_EXPORT_PCB_3D* aStepJob = dynamic_cast<JOB_EXPORT_PCB_3D*>( aJob );

    if( aStepJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aStepJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( aStepJob->GetOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );

        switch( aStepJob->m_format )
        {
        case JOB_EXPORT_PCB_3D::FORMAT::VRML: fn.SetExt( FILEEXT::VrmlFileExtension );
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::STEP: fn.SetExt( FILEEXT::StepFileExtension );
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::BREP: fn.SetExt( FILEEXT::BrepFileExtension );
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::XAO: fn.SetExt( FILEEXT::XaoFileExtension );
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::GLB: fn.SetExt( FILEEXT::GltfBinaryFileExtension );
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::PLY: fn.SetExt( FILEEXT::PlyFileExtension );
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::STL: fn.SetExt( FILEEXT::StlFileExtension );
            break;
        default:
            m_reporter->Report( _( "Unknown export format" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN; // shouldnt have gotten here
        }

        aStepJob->SetOutputPath( fn.GetFullName() );
    }

    if( aStepJob->m_format == JOB_EXPORT_PCB_3D::FORMAT::VRML )
    {

        double scale = 0.0;
        switch ( aStepJob->m_vrmlUnits )
        {
        case JOB_EXPORT_PCB_3D::VRML_UNITS::MILLIMETERS: scale = 1.0;         break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::METERS:      scale = 0.001;       break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::TENTHS:      scale = 10.0 / 25.4; break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::INCHES:      scale = 1.0 / 25.4;  break;
        }

        EXPORTER_VRML vrmlExporter( brd );
        wxString      messages;

        double originX = pcbIUScale.IUTomm( aStepJob->m_3dparams.m_Origin.x );
        double originY = pcbIUScale.IUTomm( aStepJob->m_3dparams.m_Origin.y );

        if( !aStepJob->m_hasUserOrigin )
        {
            BOX2I bbox = brd->ComputeBoundingBox( true );
            originX = pcbIUScale.IUTomm( bbox.GetCenter().x );
            originY = pcbIUScale.IUTomm( bbox.GetCenter().y );
        }

        bool success = vrmlExporter.ExportVRML_File(
                brd->GetProject(), &messages, aStepJob->GetFullOutputPath(), scale,
                aStepJob->m_3dparams.m_IncludeUnspecified, aStepJob->m_3dparams.m_IncludeDNP,
                !aStepJob->m_vrmlModelDir.IsEmpty(), aStepJob->m_vrmlRelativePaths,
                aStepJob->m_vrmlModelDir, originX, originY );

        if ( success )
        {
            m_reporter->Report( wxString::Format( _( "Successfully exported VRML to %s" ),
                                                  aStepJob->GetFullOutputPath() ),
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
        case JOB_EXPORT_PCB_3D::FORMAT::STEP:
            params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::STEP;
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::BREP:
            params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::BREP;
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::XAO:
            params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::XAO;
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::GLB:
            params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::GLB;
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::PLY:
            params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::PLY;
            break;
        case JOB_EXPORT_PCB_3D::FORMAT::STL:
            params.m_Format = EXPORTER_STEP_PARAMS::FORMAT::STL;
            break;
        default:
            m_reporter->Report( _( "Unknown export format" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN; // shouldnt have gotten here
        }

        EXPORTER_STEP stepExporter( brd, params );
        stepExporter.m_outputFile = aStepJob->GetFullOutputPath();

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

    BOARD* brd = getBoard( aRenderJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    BOARD_ADAPTER boardAdapter;

    boardAdapter.SetBoard( brd );
    boardAdapter.m_IsBoardView = false;
    boardAdapter.m_IsPreviewer = true; // Force display 3D models, regardless of 3D viewer options

    SETTINGS_MANAGER&       mgr = Pgm().GetSettingsManager();
    EDA_3D_VIEWER_SETTINGS* cfg = mgr.GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );

    if( aRenderJob->m_quality == JOB_PCB_RENDER::QUALITY::BASIC )
    {
        // Silkscreen is pixelated without antialiasing
        cfg->m_Render.raytrace_anti_aliasing = true;

        cfg->m_Render.raytrace_backfloor = false;
        cfg->m_Render.raytrace_post_processing = false;

        cfg->m_Render.raytrace_procedural_textures = false;
        cfg->m_Render.raytrace_reflections = false;
        cfg->m_Render.raytrace_shadows = false;

        // Better colors
        cfg->m_Render.differentiate_plated_copper = true;

        // Tracks below soldermask are not visible without refractions
        cfg->m_Render.raytrace_refractions = true;
        cfg->m_Render.raytrace_recursivelevel_refractions = 1;
    }
    else if( aRenderJob->m_quality == JOB_PCB_RENDER::QUALITY::HIGH )
    {
        cfg->m_Render.raytrace_anti_aliasing = true;
        cfg->m_Render.raytrace_backfloor = true;
        cfg->m_Render.raytrace_post_processing = true;
        cfg->m_Render.raytrace_procedural_textures = true;
        cfg->m_Render.raytrace_reflections = true;
        cfg->m_Render.raytrace_shadows = true;
        cfg->m_Render.raytrace_refractions = true;
        cfg->m_Render.differentiate_plated_copper = true;
    }

    if( aRenderJob->m_floor )
    {
        cfg->m_Render.raytrace_backfloor = true;
        cfg->m_Render.raytrace_shadows = true;
        cfg->m_Render.raytrace_post_processing = true;
    }

    cfg->m_CurrentPreset = aRenderJob->m_colorPreset;
    boardAdapter.m_Cfg = cfg;

    if( aRenderJob->m_bgStyle == JOB_PCB_RENDER::BG_STYLE::BG_TRANSPARENT
        || ( aRenderJob->m_bgStyle == JOB_PCB_RENDER::BG_STYLE::BG_DEFAULT
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

    PROJECTION_TYPE projection =
            aRenderJob->m_perspective ? PROJECTION_TYPE::PERSPECTIVE : PROJECTION_TYPE::ORTHO;

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

            camera.SetLookAtPos_T1(
                    camera.GetLookAtPos_T1()
                    + SFVEC3F( aRenderJob->m_pivot.x, aRenderJob->m_pivot.y, aRenderJob->m_pivot.z )
                              * cmTo3D );

            camera.Pan_T1(
                    SFVEC3F( aRenderJob->m_pan.x, aRenderJob->m_pan.y, aRenderJob->m_pan.z ) );

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
        image.SaveFile( aRenderJob->GetFullOutputPath(),
                        aRenderJob->m_format == JOB_PCB_RENDER::FORMAT::PNG ? wxBITMAP_TYPE_PNG
                                                                            : wxBITMAP_TYPE_JPEG );
    }

    m_reporter->Report( wxString::Format( _( "Actual image size: %dx%d" ), realSize.x, realSize.y )
                                + wxS( "\n" ),
                        RPT_SEVERITY_INFO );

    if( success )
        m_reporter->Report( _( "Successfully created 3D render image" ) + wxS( "\n" ),
                            RPT_SEVERITY_INFO );
    else
        m_reporter->Report( _( "Error creating 3D render image" ) + wxS( "\n" ),
                            RPT_SEVERITY_ERROR );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportSvg( JOB* aJob )
{
    JOB_EXPORT_PCB_SVG* aSvgJob = dynamic_cast<JOB_EXPORT_PCB_SVG*>( aJob );

    if( aSvgJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    PCB_PLOT_SVG_OPTIONS svgPlotOptions;
    svgPlotOptions.m_blackAndWhite = aSvgJob->m_blackAndWhite;
    svgPlotOptions.m_colorTheme = aSvgJob->m_colorTheme;
    svgPlotOptions.m_outputFile = aSvgJob->GetFullOutputPath();
    svgPlotOptions.m_mirror = aSvgJob->m_mirror;
    svgPlotOptions.m_negative = aSvgJob->m_negative;
    svgPlotOptions.m_pageSizeMode = aSvgJob->m_pageSizeMode;
    svgPlotOptions.m_printMaskLayer = aSvgJob->m_printMaskLayer;
    svgPlotOptions.m_plotFrame = aSvgJob->m_plotDrawingSheet;
    svgPlotOptions.m_sketchPadsOnFabLayers = aSvgJob->m_sketchPadsOnFabLayers;
    svgPlotOptions.m_hideDNPFPsOnFabLayers = aSvgJob->m_hideDNPFPsOnFabLayers;
    svgPlotOptions.m_sketchDNPFPsOnFabLayers = aSvgJob->m_sketchDNPFPsOnFabLayers;
    svgPlotOptions.m_crossoutDNPFPsOnFabLayers = aSvgJob->m_crossoutDNPFPsOnFabLayers;
    svgPlotOptions.m_drillShapeOption = aSvgJob->m_drillShapeOption;
    svgPlotOptions.m_precision = aSvgJob->m_precision;

    BOARD* brd = getBoard( aSvgJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    loadOverrideDrawingSheet( brd, aSvgJob->m_drawingSheet );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( EXPORT_SVG::Plot( brd, svgPlotOptions ) )
    {
        m_reporter->Report( _( "Successfully created svg file" ) + wxS( "\n" ), RPT_SEVERITY_INFO );
        return CLI::EXIT_CODES::OK;
    }
    else
    {
        m_reporter->Report( _( "Error creating svg file" ) + wxS( "\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }
}


int PCBNEW_JOBS_HANDLER::JobExportDxf( JOB* aJob )
{
    JOB_EXPORT_PCB_DXF* aDxfJob = dynamic_cast<JOB_EXPORT_PCB_DXF*>( aJob );

    if( aDxfJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aDxfJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    loadOverrideDrawingSheet( brd, aDxfJob->m_drawingSheet );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( aDxfJob->GetOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::DXF ) );

        aDxfJob->SetOutputPath( fn.GetFullName() );
    }

    PCB_PLOT_PARAMS plotOpts;
    plotOpts.SetFormat( PLOT_FORMAT::DXF );

    plotOpts.SetDXFPlotPolygonMode( aDxfJob->m_plotGraphicItemsUsingContours );
    plotOpts.SetUseAuxOrigin( aDxfJob->m_useDrillOrigin );

    if( aDxfJob->m_dxfUnits == JOB_EXPORT_PCB_DXF::DXF_UNITS::MILLIMETERS )
        plotOpts.SetDXFPlotUnits( DXF_UNITS::MILLIMETERS );
    else
        plotOpts.SetDXFPlotUnits( DXF_UNITS::INCHES );

    plotOpts.SetPlotFrameRef( aDxfJob->m_plotDrawingSheet );
    plotOpts.SetPlotValue( aDxfJob->m_plotFootprintValues );
    plotOpts.SetPlotReference( aDxfJob->m_plotRefDes );
    plotOpts.SetLayerSelection( aDxfJob->m_printMaskLayer );
    plotOpts.SetPlotOnAllLayersSelection( aDxfJob->m_printMaskLayersToIncludeOnAllLayers );

    PCB_LAYER_ID layer = UNDEFINED_LAYER;
    wxString     layerName;
    wxString     sheetName;
    wxString     sheetPath;

    if( aDxfJob->m_printMaskLayer.size() == 1 )
    {
        layer = aDxfJob->m_printMaskLayer.front();
        layerName = brd->GetLayerName( layer );
    }

    if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
        layerName = aJob->GetVarOverrides().at( wxT( "LAYER" ) );

    if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
        sheetName = aJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

    if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
        sheetPath = aJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );

    DXF_PLOTTER* plotter = (DXF_PLOTTER*) StartPlotBoard( brd, &plotOpts, layer, layerName, aDxfJob->GetFullOutputPath(), sheetName,
                                                          sheetPath );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aDxfJob->m_printMaskLayer, plotOpts );
        plotter->EndPlot();
    }

    delete plotter;

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportPdf( JOB* aJob )
{
    JOB_EXPORT_PCB_PDF* aPdfJob = dynamic_cast<JOB_EXPORT_PCB_PDF*>( aJob );

    if( aPdfJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aPdfJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    loadOverrideDrawingSheet( brd, aPdfJob->m_drawingSheet );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( aPdfJob->GetOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::PDF ) );

        aPdfJob->SetOutputPath( fn.GetFullName() );
    }

    PCB_PLOT_PARAMS plotOpts;
    plotOpts.SetFormat( PLOT_FORMAT::PDF );

    plotOpts.SetPlotFrameRef( aPdfJob->m_plotDrawingSheet );
    plotOpts.SetPlotValue( aPdfJob->m_plotFootprintValues );
    plotOpts.SetPlotReference( aPdfJob->m_plotRefDes );

    plotOpts.SetLayerSelection( aPdfJob->m_printMaskLayer );
    plotOpts.SetPlotOnAllLayersSelection( aPdfJob->m_printMaskLayersToIncludeOnAllLayers );

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    plotOpts.SetColorSettings( mgr.GetColorSettings( aPdfJob->m_colorTheme ) );
    plotOpts.SetMirror( aPdfJob->m_mirror );
    plotOpts.SetBlackAndWhite( aPdfJob->m_blackAndWhite );
    plotOpts.SetNegative( aPdfJob->m_negative );

    if( aPdfJob->m_sketchPadsOnFabLayers )
    {
        plotOpts.SetSketchPadsOnFabLayers( true );
        plotOpts.SetPlotPadNumbers( true );
    }

    plotOpts.SetHideDNPFPsOnFabLayers( aPdfJob->m_hideDNPFPsOnFabLayers );
    plotOpts.SetSketchDNPFPsOnFabLayers( aPdfJob->m_sketchDNPFPsOnFabLayers );
    plotOpts.SetCrossoutDNPFPsOnFabLayers( aPdfJob->m_crossoutDNPFPsOnFabLayers );

    switch( aPdfJob->m_drillShapeOption )
    {
    default:
        case 0: plotOpts.SetDrillMarksType( DRILL_MARKS::NO_DRILL_SHAPE );    break;
    case 1: plotOpts.SetDrillMarksType( DRILL_MARKS::SMALL_DRILL_SHAPE ); break;
        case 2: plotOpts.SetDrillMarksType( DRILL_MARKS::FULL_DRILL_SHAPE );  break;
    }

    PCB_LAYER_ID layer = UNDEFINED_LAYER;
    wxString     layerName;
    wxString     sheetName;
    wxString     sheetPath;

    if( aPdfJob->m_printMaskLayer.size() == 1 )
    {
        layer = aPdfJob->m_printMaskLayer.front();
        layerName = brd->GetLayerName( layer );
    }

    if( aPdfJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
        layerName = aPdfJob->GetVarOverrides().at( wxT( "LAYER" ) );

    if( aPdfJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
        sheetName = aPdfJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

    if( aPdfJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
        sheetPath = aPdfJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );

    LOCALE_IO dummy;
    PDF_PLOTTER* plotter = (PDF_PLOTTER*) StartPlotBoard( brd, &plotOpts, layer, layerName,
                                                          aPdfJob->GetFullOutputPath(),
                                                          sheetName, sheetPath );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aPdfJob->m_printMaskLayer, plotOpts );
        PlotInteractiveLayer( brd, plotter, plotOpts );
        plotter->EndPlot();
    }

    delete plotter;

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

    loadOverrideDrawingSheet( brd, aGerberJob->m_drawingSheet );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    PCB_PLOT_PARAMS       boardPlotOptions = brd->GetPlotOptions();
    LSET                  plotOnAllLayersSelection = boardPlotOptions.GetPlotOnAllLayersSelection();
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
        aGerberJob->m_printMaskLayer = plotLayers.SeqStackupForPlotting();
        aGerberJob->m_layersIncludeOnAll = boardPlotOptions.GetPlotOnAllLayersSelection();
    }
    else
    {
        // default to the board enabled layers
        if( aGerberJob->m_printMaskLayer.empty() )
            aGerberJob->m_printMaskLayer = brd->GetEnabledLayers().SeqStackupForPlotting();

        if( aGerberJob->m_layersIncludeOnAllSet )
            aGerberJob->m_layersIncludeOnAll = plotOnAllLayersSelection;
    }

    // Ensure layers to plot are restricted to enabled layers of the board to plot
    LSET layersToPlot = LSET( { aGerberJob->m_printMaskLayer } ) & brd->GetEnabledLayers();

    for( PCB_LAYER_ID layer : layersToPlot.UIOrder() )
    {
        LSEQ plotSequence;

        // Base layer always gets plotted first.
        plotSequence.push_back( layer );

        // Now all the "include on all" layers
        for( PCB_LAYER_ID layer_all : aGerberJob->m_layersIncludeOnAll.UIOrder() )
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
            populateGerberPlotOptionsFromJob( plotOpts, aGerberJob );

        if( plotOpts.GetUseGerberProtelExtensions() )
            fileExt = GetGerberProtelExtension( layer );
        else
            fileExt = FILEEXT::GerberFileExtension;

        BuildPlotFileName( &fn, aGerberJob->GetFullOutputPath(), layerName, fileExt );
        wxString fullname = fn.GetFullName();

        jobfile_writer.AddGbrFile( layer, fullname );

        if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
            layerName = aJob->GetVarOverrides().at( wxT( "LAYER" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
            sheetName = aJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
            sheetPath = aJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );

        // We are feeding it one layer at the start here to silence a logic check
        GERBER_PLOTTER* plotter;
        {
            LOCALE_IO dummy;
            plotter = (GERBER_PLOTTER*) StartPlotBoard( brd, &plotOpts, layer,
                                                        layerName, fn.GetFullPath(),
                                                        sheetName, sheetPath );
        }

        if( plotter )
        {
            m_reporter->Report( wxString::Format( _( "Plotted to '%s'.\n" ), fn.GetFullPath() ),
                                RPT_SEVERITY_ACTION );
            LOCALE_IO dummy;
            PlotBoardLayers( brd, plotter, plotSequence, plotOpts );
            plotter->EndPlot();
        }
        else
        {
            m_reporter->Report( wxString::Format( _( "Failed to plot to '%s'.\n" ),
                                                  fn.GetFullPath() ),
                    RPT_SEVERITY_ERROR );
            exitCode = CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }

        delete plotter;
    }

    wxFileName fn( brd->GetFileName() );

    // Build gerber job file from basename
    BuildPlotFileName( &fn, aGerberJob->GetFullOutputPath(), wxT( "job" ),
                       FILEEXT::GerberJobFileExtension );
    jobfile_writer.CreateJobFile( fn.GetFullPath() );

    return exitCode;
}

int PCBNEW_JOBS_HANDLER::JobExportGencad( JOB* aJob )
{
    JOB_EXPORT_PCB_GENCAD* aGencadJob = dynamic_cast<JOB_EXPORT_PCB_GENCAD*>( aJob );

    if( aGencadJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* aBoard = LoadBoard( aGencadJob->m_filename, true ); // Ensure m_board is of type BOARD*

    if( aBoard == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    GENCAD_EXPORTER exporter( aBoard );

    VECTOR2I GencadOffset;
    VECTOR2I auxOrigin = aBoard->GetDesignSettings().GetAuxOrigin();
    GencadOffset.x = aGencadJob->m_useDrillOrigin ? auxOrigin.x : 0;
    GencadOffset.y = aGencadJob->m_useDrillOrigin ? auxOrigin.y : 0;

    exporter.FlipBottomPads( aGencadJob->m_flipBottomPads );
    exporter.UsePinNamesUnique( aGencadJob->m_useUniquePins );
    exporter.UseIndividualShapes( aGencadJob->m_useIndividualShapes );
    exporter.SetPlotOffet( GencadOffset );
    exporter.StoreOriginCoordsInFile( aGencadJob->m_storeOriginCoords );

    if( aGencadJob->GetOutputPath().IsEmpty() )
    {
        wxFileName fn = aBoard->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::DXF ) );

        aGencadJob->SetOutputPath( fn.GetFullName() );
    }

    if( !exporter.WriteFile( aGencadJob->GetFullOutputPath() ) )
    {
        wxString msg;
        msg.Printf( _( "Failed to create file '%s'.\n" ), aGencadJob->GetFullOutputPath() );

        m_reporter->Report( msg, RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    m_reporter->Report( _( "Successfully created genCAD file\n" ), RPT_SEVERITY_INFO );

    return CLI::EXIT_CODES::OK;
}


void PCBNEW_JOBS_HANDLER::populateGerberPlotOptionsFromJob( PCB_PLOT_PARAMS& aPlotOpts,
                                                            JOB_EXPORT_PCB_GERBER* aJob )
{
    aPlotOpts.SetFormat( PLOT_FORMAT::GERBER );

    aPlotOpts.SetPlotFrameRef( aJob->m_plotDrawingSheet );
    aPlotOpts.SetPlotValue( aJob->m_plotFootprintValues );
    aPlotOpts.SetPlotReference( aJob->m_plotRefDes );

    aPlotOpts.SetSubtractMaskFromSilk( aJob->m_subtractSolderMaskFromSilk );

    // Always disable plot pad holes
    aPlotOpts.SetDrillMarksType( DRILL_MARKS::NO_DRILL_SHAPE );

    aPlotOpts.SetDisableGerberMacros( aJob->m_disableApertureMacros );
    aPlotOpts.SetUseGerberX2format( aJob->m_useX2Format );
    aPlotOpts.SetIncludeGerberNetlistInfo( aJob->m_includeNetlistAttributes );
    aPlotOpts.SetUseAuxOrigin( aJob->m_useAuxOrigin );
    aPlotOpts.SetUseGerberProtelExtensions( aJob->m_useProtelFileExtension );
    aPlotOpts.SetGerberPrecision( aJob->m_precision );
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

    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( aGerberJob->GetOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::GERBER ) );

        aGerberJob->SetOutputPath( fn.GetFullName() );
    }

    PCB_PLOT_PARAMS plotOpts;
    populateGerberPlotOptionsFromJob( plotOpts, aGerberJob );
    plotOpts.SetLayerSelection( aGerberJob->m_printMaskLayer );
    plotOpts.SetPlotOnAllLayersSelection( aGerberJob->m_printMaskLayersToIncludeOnAllLayers );

    PCB_LAYER_ID layer = UNDEFINED_LAYER;
    wxString     layerName;
    wxString     sheetName;
    wxString     sheetPath;

    // The first layer will be treated as the layer name for the gerber header,
    // the other layers will be treated equivalent to the "Plot on All Layers" option
    // in the GUI
    if( aGerberJob->m_printMaskLayer.size() >= 1 )
    {
        layer = aGerberJob->m_printMaskLayer.front();
        layerName = brd->GetLayerName( layer );
    }

    if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
        layerName = aJob->GetVarOverrides().at( wxT( "LAYER" ) );

    if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
        sheetName = aJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

    if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
        sheetPath = aJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );

    // We are feeding it one layer at the start here to silence a logic check
    GERBER_PLOTTER* plotter = (GERBER_PLOTTER*) StartPlotBoard( brd, &plotOpts, layer, layerName,
                                                                aGerberJob->GetFullOutputPath(),
                                                                sheetName, sheetPath );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aGerberJob->m_printMaskLayer, plotOpts );
        plotter->EndPlot();
    }
    else
    {
        m_reporter->Report( wxString::Format( _( "Failed to plot to '%s'.\n" ),
                                              aGerberJob->GetFullOutputPath() ),
                RPT_SEVERITY_ERROR );
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

    // ensure output dir exists
    wxFileName fn( aDrillJob->GetFullOutputPath() + wxT( "/" ) );

    if( !fn.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
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
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::POSTSCRIPT: mapFormat = PLOT_FORMAT::POST;   break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::GERBER_X2:  mapFormat = PLOT_FORMAT::GERBER; break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::DXF:        mapFormat = PLOT_FORMAT::DXF;    break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::SVG:        mapFormat = PLOT_FORMAT::SVG;    break;
    default:
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::PDF:        mapFormat = PLOT_FORMAT::PDF;    break;
    }

    if( aDrillJob->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON )
    {
        EXCELLON_WRITER::ZEROS_FMT zeroFmt;
        switch( aDrillJob->m_zeroFormat )
        {
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::KEEP_ZEROS:
            zeroFmt = EXCELLON_WRITER::KEEP_ZEROS;
            break;
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_LEADING:
            zeroFmt = EXCELLON_WRITER::SUPPRESS_LEADING;
            break;
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_TRAILING:
            zeroFmt = EXCELLON_WRITER::SUPPRESS_TRAILING;
            break;
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::DECIMAL:
        default:
            zeroFmt = EXCELLON_WRITER::DECIMAL_FORMAT;
            break;
        }

        DRILL_PRECISION precision;

        if( aDrillJob->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCHES )
            precision = precisionListForInches;
        else
            precision = precisionListForMetric;

        EXCELLON_WRITER* excellonWriter = dynamic_cast<EXCELLON_WRITER*>( drillWriter.get() );

        if( excellonWriter == nullptr )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        excellonWriter->SetFormat( aDrillJob->m_drillUnits
                                           == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MILLIMETERS,
                                   zeroFmt, precision.m_Lhs, precision.m_Rhs );
        excellonWriter->SetOptions( aDrillJob->m_excellonMirrorY,
                                    aDrillJob->m_excellonMinimalHeader,
                                    offset, aDrillJob->m_excellonCombinePTHNPTH );
        excellonWriter->SetRouteModeForOvalHoles( aDrillJob->m_excellonOvalDrillRoute );
        excellonWriter->SetMapFileFormat( mapFormat );

        if( !excellonWriter->CreateDrillandMapFilesSet( aDrillJob->GetFullOutputPath(), true,
                                                        aDrillJob->m_generateMap, m_reporter ) )
        {
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
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

        if( !gerberWriter->CreateDrillandMapFilesSet( aDrillJob->GetFullOutputPath(), true,
                                                      aDrillJob->m_generateMap, m_reporter ) )
        {
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
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
    {
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( aPosJob->GetOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );

        if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII )
            fn.SetExt( FILEEXT::FootprintPlaceFileExtension );
        else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
            fn.SetExt( FILEEXT::CsvFileExtension );
        else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::GERBER )
            fn.SetExt( FILEEXT::GerberFileExtension );

        aPosJob->SetOutputPath( fn.GetFullName() );
    }

    if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII
        || aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
    {
        FILE* file = nullptr;
        file = wxFopen( aPosJob->GetFullOutputPath(), wxS( "wt" ) );

        if( file == nullptr )
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;

        std::string         data;

        bool frontSide = aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::FRONT
                         || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH;

        bool backSide = aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BACK
                        || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH;

        PLACE_FILE_EXPORTER exporter( brd,
                                      aPosJob->m_units == JOB_EXPORT_PCB_POS::UNITS::MILLIMETERS,
                                      aPosJob->m_smdOnly, aPosJob->m_excludeFootprintsWithTh,
                                      aPosJob->m_excludeDNP,
                                      frontSide, backSide,
                aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV,
                                      aPosJob->m_useDrillPlaceFileOrigin,
                                      aPosJob->m_negateBottomX );
        data = exporter.GenPositionData();

        fputs( data.c_str(), file );
        fclose( file );

        aPosJob->AddOutput( aPosJob->GetFullOutputPath() );
    }
    else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::GERBER )
    {
        PLACEFILE_GERBER_WRITER exporter( brd );

        PCB_LAYER_ID gbrLayer = F_Cu;

        if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BACK )
            gbrLayer = B_Cu;

        if( exporter.CreatePlaceFile( aPosJob->GetFullOutputPath(), gbrLayer, aPosJob->m_gerberBoardEdge )
            >= 0 )
        {
            aPosJob->AddOutput( aPosJob->GetFullOutputPath() );
        }
        else
        {
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    return CLI::EXIT_CODES::OK;
}

extern FOOTPRINT* try_load_footprint( const wxFileName& aFileName, PCB_IO_MGR::PCB_FILE_T aFileType,
                                      const wxString& aName );


int PCBNEW_JOBS_HANDLER::JobExportFpUpgrade( JOB* aJob )
{
    JOB_FP_UPGRADE* upgradeJob = dynamic_cast<JOB_FP_UPGRADE*>( aJob );

    if( upgradeJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    PCB_IO_MGR::PCB_FILE_T fileType = PCB_IO_MGR::GuessPluginTypeFromLibPath( upgradeJob->m_libraryPath );

    if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
    {
        if( wxFile::Exists( upgradeJob->m_outputLibraryPath ) ||
            wxDir::Exists( upgradeJob->m_outputLibraryPath) )
        {
            m_reporter->Report( _( "Output path must not conflict with existing path\n" ),
                                RPT_SEVERITY_ERROR );
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
                                RPT_SEVERITY_INFO );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        m_reporter->Report( _( "Loading footprint library\n" ), RPT_SEVERITY_INFO );

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

        bool shouldSave = upgradeJob->m_force;

        for( const auto& footprint : fpLib.GetFootprints() )
        {
            if( footprint.second->GetFootprint()->GetFileFormatVersionAtLoad()
                < SEXPR_BOARD_FILE_VERSION )
            {
                shouldSave = true;
            }
        }

        if( shouldSave )
        {
            m_reporter->Report( _( "Saving footprint library\n" ), RPT_SEVERITY_INFO );

            try
            {
                if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
                {
                    fpLib.SetPath( upgradeJob->m_outputLibraryPath );
                }

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
            m_reporter->Report( _( "Footprint library was not updated\n" ), RPT_SEVERITY_INFO );
        }
    }
    else
    {
        if( !PCB_IO_MGR::ConvertLibrary( nullptr, upgradeJob->m_libraryPath,
                                         upgradeJob->m_outputLibraryPath, nullptr /* REPORTER */ ) )
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

    m_reporter->Report( _( "Loading footprint library\n" ), RPT_SEVERITY_INFO );

    PCB_IO_KICAD_SEXPR pcb_io( CTL_FOR_LIBRARY );
    FP_CACHE   fpLib( &pcb_io, svgJob->m_libraryPath );

    try
    {
        fpLib.Load();
    }
    catch( ... )
    {
        m_reporter->Report( _( "Unable to load library\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( !svgJob->m_outputDirectory.IsEmpty() && !wxDir::Exists( svgJob->m_outputDirectory ) )
    {
        wxFileName::Mkdir( svgJob->m_outputDirectory );
    }

    int exitCode = CLI::EXIT_CODES::OK;

    // Just plot all the symbols we can
    FP_CACHE_FOOTPRINT_MAP& footprintMap = fpLib.GetFootprints();

    bool singleFpPlotted = false;
    for( FP_CACHE_FOOTPRINT_MAP::iterator it = footprintMap.begin(); it != footprintMap.end();
         ++it )
    {
        const FOOTPRINT* fp = it->second->GetFootprint();
        if( !svgJob->m_footprint.IsEmpty() )
        {
            if( fp->GetFPID().GetLibItemName().wx_str() != svgJob->m_footprint )
            {
                // skip until we find the right footprint
                continue;
            }
            else
            {
                singleFpPlotted = true;
            }
        }

        exitCode = doFpExportSvg( svgJob, fp );
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
    std::unique_ptr<BOARD> brd;
    brd.reset( CreateEmptyBoard() );
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
    outputFile.SetPath( aSvgJob->m_outputDirectory );
    outputFile.SetName( aFootprint->GetFPID().GetLibItemName().wx_str() );
    outputFile.SetExt( FILEEXT::SVGFileExtension );

    m_reporter->Report( wxString::Format( _( "Plotting footprint '%s' to '%s'\n" ),
                                          aFootprint->GetFPID().GetLibItemName().wx_str(),
                                          outputFile.GetFullPath() ),
                        RPT_SEVERITY_ACTION );


    PCB_PLOT_SVG_OPTIONS svgPlotOptions;
    svgPlotOptions.m_blackAndWhite = aSvgJob->m_blackAndWhite;
    svgPlotOptions.m_colorTheme = aSvgJob->m_colorTheme;
    svgPlotOptions.m_outputFile = outputFile.GetFullPath();
    svgPlotOptions.m_mirror = false;
    svgPlotOptions.m_pageSizeMode = 2; // board bounding box
    svgPlotOptions.m_printMaskLayer = aSvgJob->m_printMaskLayer;
    svgPlotOptions.m_sketchPadsOnFabLayers = aSvgJob->m_sketchPadsOnFabLayers;
    svgPlotOptions.m_hideDNPFPsOnFabLayers = aSvgJob->m_hideDNPFPsOnFabLayers;
    svgPlotOptions.m_sketchDNPFPsOnFabLayers = aSvgJob->m_sketchDNPFPsOnFabLayers;
    svgPlotOptions.m_crossoutDNPFPsOnFabLayers = aSvgJob->m_crossoutDNPFPsOnFabLayers;
    svgPlotOptions.m_plotFrame = false;

    if( !EXPORT_SVG::Plot( brd.get(), svgPlotOptions ) )
        m_reporter->Report( _( "Error creating svg file" ) + wxS( "\n" ), RPT_SEVERITY_ERROR );

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

    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( drcJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() + wxS( "-drc" ) );

        if( drcJob->m_format == JOB_PCB_DRC::OUTPUT_FORMAT::JSON )
            fn.SetExt( FILEEXT::JsonFileExtension );
        else
            fn.SetExt( FILEEXT::ReportFileExtension );

        drcJob->SetOutputPath( fn.GetFullName() );
    }

    EDA_UNITS units;

    switch( drcJob->m_units )
    {
    case JOB_PCB_DRC::UNITS::INCHES:      units = EDA_UNITS::INCHES;      break;
    case JOB_PCB_DRC::UNITS::MILS:        units = EDA_UNITS::MILS;        break;
    case JOB_PCB_DRC::UNITS::MILLIMETERS: units = EDA_UNITS::MILLIMETRES; break;
    default:                              units = EDA_UNITS::MILLIMETRES; break;
    }

    std::shared_ptr<DRC_ENGINE> drcEngine = brd->GetDesignSettings().m_DRCEngine;
    std::unique_ptr<NETLIST>    netlist = std::make_unique<NETLIST>();

    drcEngine->SetDrawingSheet( getDrawingSheetProxyView( brd ) );

    // BOARD_COMMIT uses TOOL_MANAGER to grab the board internally so we must give it one
    TOOL_MANAGER* toolManager = new TOOL_MANAGER;
    toolManager->SetEnvironment( brd, nullptr, nullptr, Kiface().KifaceSettings(), nullptr );

    BOARD_COMMIT commit( toolManager );

    m_reporter->Report( _( "Running DRC...\n" ), RPT_SEVERITY_INFO );

    if( drcJob->m_parity )
    {
        typedef bool (*NETLIST_FN_PTR)( const wxString&, std::string& );

        KIFACE*        eeschema = m_kiway->KiFACE( KIWAY::FACE_SCH );
        wxFileName     schematicPath( drcJob->m_filename );
        NETLIST_FN_PTR netlister = (NETLIST_FN_PTR) eeschema->IfaceOrAddress( KIFACE_NETLIST_SCHEMATIC );
        std::string    netlist_str;

        schematicPath.SetExt( FILEEXT::KiCadSchematicFileExtension );

        if( !schematicPath.Exists() )
            schematicPath.SetExt( FILEEXT::LegacySchematicFileExtension );

        if( !schematicPath.Exists() )
        {
            m_reporter->Report( _( "Failed to find schematic for parity tests.\n" ),
                                RPT_SEVERITY_INFO );
        }
        else
        {
            (*netlister)( schematicPath.GetFullPath(), netlist_str );

            try
            {
                auto lineReader = new STRING_LINE_READER( netlist_str, _( "Eeschema netlist" ) );
                KICAD_NETLIST_READER netlistReader( lineReader, netlist.get() );
                netlistReader.LoadNetlist();
            }
            catch( const IO_ERROR& )
            {
                m_reporter->Report( _( "Failed to fetch schematic netlist for parity tests.\n" ),
                                    RPT_SEVERITY_INFO );
            }

            drcEngine->SetSchematicNetlist( netlist.get() );
        }
    }

    drcEngine->SetProgressReporter( nullptr );
    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, VECTOR2I aPos, int aLayer,
                 DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )
            {
                PCB_MARKER* marker = new PCB_MARKER( aItem, aPos, aLayer );
                commit.Add( marker );
            } );

    brd->RecordDRCExclusions();
    brd->DeleteMARKERs( true, true );
    drcEngine->RunTests( units, drcJob->m_reportAllTrackErrors, drcJob->m_parity );
    drcEngine->ClearViolationHandler();

    commit.Push( _( "DRC" ), SKIP_UNDO | SKIP_SET_DIRTY );

    // Update the exclusion status on any excluded markers that still exist.
    brd->ResolveDRCExclusions( false );

    std::shared_ptr<DRC_ITEMS_PROVIDER> markersProvider = std::make_shared<DRC_ITEMS_PROVIDER>(
            brd, MARKER_BASE::MARKER_DRC, MARKER_BASE::MARKER_DRAWING_SHEET );

    std::shared_ptr<DRC_ITEMS_PROVIDER> ratsnestProvider =
            std::make_shared<DRC_ITEMS_PROVIDER>( brd, MARKER_BASE::MARKER_RATSNEST );

    std::shared_ptr<DRC_ITEMS_PROVIDER> fpWarningsProvider =
            std::make_shared<DRC_ITEMS_PROVIDER>( brd, MARKER_BASE::MARKER_PARITY );

    markersProvider->SetSeverities( drcJob->m_severity );
    ratsnestProvider->SetSeverities( drcJob->m_severity );
    fpWarningsProvider->SetSeverities( drcJob->m_severity );

    m_reporter->Report( wxString::Format( _( "Found %d violations\n" ),
                                          markersProvider->GetCount() ),
            RPT_SEVERITY_INFO );
    m_reporter->Report( wxString::Format( _( "Found %d unconnected items\n" ),
                                          ratsnestProvider->GetCount() ),
            RPT_SEVERITY_INFO );

    if( drcJob->m_parity )
    {
        m_reporter->Report( wxString::Format( _( "Found %d schematic parity issues\n" ),
                                              fpWarningsProvider->GetCount() ),
                            RPT_SEVERITY_INFO );
    }

    DRC_REPORT reportWriter( brd, units, markersProvider, ratsnestProvider, fpWarningsProvider );

    bool wroteReport = false;

    if( drcJob->m_format == JOB_PCB_DRC::OUTPUT_FORMAT::JSON )
        wroteReport = reportWriter.WriteJsonReport( drcJob->GetFullOutputPath() );
    else
        wroteReport = reportWriter.WriteTextReport( drcJob->GetFullOutputPath() );

    if( !wroteReport )
    {
        m_reporter->Report( wxString::Format( _( "Unable to save DRC report to %s\n" ),
                                              drcJob->GetFullOutputPath() ),
                RPT_SEVERITY_INFO );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    m_reporter->Report( wxString::Format( _( "Saved DRC Report to %s\n" ), drcJob->GetFullOutputPath() ),
                        RPT_SEVERITY_INFO );

    if( drcJob->m_exitCodeViolations )
    {
        if( markersProvider->GetCount() > 0 || ratsnestProvider->GetCount() > 0
            || fpWarningsProvider->GetCount() > 0 )
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

    if( job->OutputPathFullSpecified() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( FILEEXT::Ipc2581FileExtension );

        job->SetOutputPath( fn.GetName() );
    }

    std::map<std::string, UTF8> props;
    props["units"] = job->m_units == JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MILLIMETERS ? "mm"
                                                                                        : "inch";
    props["sigfig"] = wxString::Format( "%d", job->m_precision );
    props["version"] = job->m_version == JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C ? "C" : "B";
    props["OEMRef"] = job->m_colInternalId;
    props["mpn"] = job->m_colMfgPn;
    props["mfg"] = job->m_colMfg;
    props["dist"] = job->m_colDist;
    props["distpn"] = job->m_colDistPn;

    wxString tempFile = wxFileName::CreateTempFileName( wxS( "pcbnew_ipc" ) );
    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( PCB_IO_MGR::IPC2581 ) );
        pi->SetProgressReporter( m_progressReporter );
        pi->SaveBoard( tempFile, brd, &props );
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Error generating IPC2581 file '%s'.\n%s" ),
                                              job->m_filename, ioe.What() ),
                            RPT_SEVERITY_ERROR );

        wxRemoveFile( tempFile );

        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( job->m_compress )
    {
        wxFileName tempfn = job->GetFullOutputPath();
        tempfn.SetExt( FILEEXT::Ipc2581FileExtension );
        wxFileName zipfn = tempFile;
        zipfn.SetExt( "zip" );

        wxFFileOutputStream fnout( zipfn.GetFullPath() );
        wxZipOutputStream   zip( fnout );
        wxFFileInputStream  fnin( tempFile );

        zip.PutNextEntry( tempfn.GetFullName() );
        fnin.Read( zip );
        zip.Close();
        fnout.Close();

        wxRemoveFile( tempFile );
        tempFile = zipfn.GetFullPath();
    }

    // If save succeeded, replace the original with what we just wrote
    if( !wxRenameFile( tempFile, job->GetFullOutputPath() ) )
    {
        m_reporter->Report( wxString::Format( _( "Error generating IPC2581 file '%s'.\n"
                                                 "Failed to rename temporary file '%s." )
                                                      + wxS( "\n" ),
                                              job->GetFullOutputPath(), tempFile ),
                            RPT_SEVERITY_ERROR );
    }

    return CLI::EXIT_CODES::SUCCESS;
}


int PCBNEW_JOBS_HANDLER::JobExportOdb( JOB* aJob )
{
    JOB_EXPORT_PCB_ODB* job = dynamic_cast<JOB_EXPORT_PCB_ODB*>( aJob );

    if( job == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( job->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( job->OutputPathFullSpecified() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( "zip" );

        job->SetOutputPath( fn.GetName() );
    }

    DIALOG_EXPORT_ODBPP::GenerateODBPPFiles( *job, brd, nullptr, m_progressReporter, m_reporter );

    return CLI::EXIT_CODES::SUCCESS;
}


DS_PROXY_VIEW_ITEM* PCBNEW_JOBS_HANDLER::getDrawingSheetProxyView( BOARD* aBrd )
{
    DS_PROXY_VIEW_ITEM* drawingSheet = new DS_PROXY_VIEW_ITEM( pcbIUScale,
                                                               &aBrd->GetPageSettings(),
                                                               aBrd->GetProject(),
                                                               &aBrd->GetTitleBlock(),
                                                               &aBrd->GetProperties() );

    drawingSheet->SetSheetName( std::string() );
    drawingSheet->SetSheetPath( std::string() );
    drawingSheet->SetIsFirstPage( true );

    drawingSheet->SetFileName( TO_UTF8( aBrd->GetFileName() ) );

    return drawingSheet;
}


void PCBNEW_JOBS_HANDLER::loadOverrideDrawingSheet( BOARD* aBrd, const wxString& aSheetPath )
{
    // dont bother attempting to load a empty path, if there was one
    if( aSheetPath.IsEmpty() )
        return;

    auto loadSheet =
            [&]( const wxString& path ) -> bool
    {
        BASE_SCREEN::m_DrawingSheetFileName = path;
        FILENAME_RESOLVER resolver;
        resolver.SetProject( aBrd->GetProject() );
        resolver.SetProgramBase( &Pgm() );

        wxString filename = resolver.ResolvePath( BASE_SCREEN::m_DrawingSheetFileName,
                                                  aBrd->GetProject()->GetProjectPath(),
                                                  aBrd->GetEmbeddedFiles() );
        wxString msg;

        if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename, &msg ) )
        {
                    m_reporter->Report( wxString::Format( _( "Error loading drawing sheet '%s'." ),
                                                          path )
                                        + wxS( "\n" ) + msg + wxS( "\n" ),
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
