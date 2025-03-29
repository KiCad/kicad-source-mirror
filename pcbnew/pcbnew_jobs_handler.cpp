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
#include <jobs/job_export_pcb_ipcd356.h>
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
#include <exporters/export_d356.h>
#include <kiface_ids.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/netlist_reader.h>
#include <pcbnew_settings.h>
#include <pcbplot.h>
#include <pcb_plotter.h>
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
#include <dialogs/dialog_gendrill.h>
#include <dialogs/dialog_gen_footprint_position.h>
#include <dialogs/dialog_export_2581.h>
#include <dialogs/dialog_export_odbpp.h>
#include <dialogs/dialog_export_step.h>
#include <dialogs/dialog_plot.h>
#include <dialogs/dialog_drc_job_config.h>
#include <dialogs/dialog_render_job.h>
#include <paths.h>

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

                  wxCHECK( svgJob && editFrame, false );

                  DIALOG_EXPORT_STEP dlg( editFrame, aParent, "", svgJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "render",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportRender, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  DIALOG_RENDER_JOB dlg( aParent, dynamic_cast<JOB_PCB_RENDER*>( job ) );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "svg", std::bind( &PCBNEW_JOBS_HANDLER::JobExportSvg, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_SVG* svgJob = dynamic_cast<JOB_EXPORT_PCB_SVG*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( svgJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, svgJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "gencad",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportGencad, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "dxf", std::bind( &PCBNEW_JOBS_HANDLER::JobExportDxf, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_DXF* dxfJob = dynamic_cast<JOB_EXPORT_PCB_DXF*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( dxfJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, dxfJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "pdf", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPdf, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_PDF* pdfJob = dynamic_cast<JOB_EXPORT_PCB_PDF*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( pdfJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, pdfJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "gerber",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportGerber, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_GERBER* gJob = dynamic_cast<JOB_EXPORT_PCB_GERBER*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( gJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, gJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "gerbers",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportGerbers, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_GERBERS* gJob = dynamic_cast<JOB_EXPORT_PCB_GERBERS*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( gJob && editFrame, false );

                  DIALOG_PLOT dlg( editFrame, aParent, gJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "drill",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportDrill, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_DRILL* drillJob = dynamic_cast<JOB_EXPORT_PCB_DRILL*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( drillJob && editFrame, false );

                  DIALOG_GENDRILL dlg( editFrame, drillJob, aParent );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "pos", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPos, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
			  {
				  JOB_EXPORT_PCB_POS* posJob = dynamic_cast<JOB_EXPORT_PCB_POS*>( job );

				  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( posJob && editFrame, false );

				  DIALOG_GEN_FOOTPRINT_POSITION dlg( posJob, editFrame, aParent );
                  return dlg.ShowModal() == wxID_OK;
			  } );
    Register( "fpupgrade",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportFpUpgrade, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "fpsvg",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportFpSvg, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "drc", std::bind( &PCBNEW_JOBS_HANDLER::JobExportDrc, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  DIALOG_DRC_JOB_CONFIG dlg( aParent, dynamic_cast<JOB_PCB_DRC*>( job ) );

                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "ipc2581",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportIpc2581, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_IPC2581* ipcJob = dynamic_cast<JOB_EXPORT_PCB_IPC2581*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( ipcJob && editFrame, false );

                  DIALOG_EXPORT_2581 dlg( ipcJob, editFrame, aParent );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "ipcd356",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportIpcD356, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "odb",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportOdb, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_PCB_ODB* odbJob = dynamic_cast<JOB_EXPORT_PCB_ODB*>( job );

                  PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>(
                          aKiway->Player( FRAME_PCB_EDITOR, false ) );

                  wxCHECK( odbJob && editFrame, false );

                  DIALOG_EXPORT_ODBPP dlg( odbJob, editFrame, aParent );
                  return dlg.ShowModal() == wxID_OK;
              } );
}


BOARD* PCBNEW_JOBS_HANDLER::getBoard( const wxString& aPath )
{
    BOARD* brd = nullptr;

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
            m_cliBoard = LoadBoard( pcbPath, true );

        brd = m_cliBoard;
    }
    else if( Pgm().IsGUI() && Pgm().GetSettingsManager().IsProjectOpen() )
    {
        PCB_EDIT_FRAME* editFrame = (PCB_EDIT_FRAME*) m_kiway->Player( FRAME_PCB_EDITOR, false );

        if( editFrame )
            brd = editFrame->GetBoard();
    }
    else
    {
        brd = LoadBoard( aPath, true );
    }

    if( !brd )
        m_reporter->Report( _( "Failed to load board\n" ), RPT_SEVERITY_ERROR );

    return brd;
}


LSEQ PCBNEW_JOBS_HANDLER::convertLayerArg( wxString& aLayerString, BOARD* aBoard ) const
{
    std::map<wxString, LSET> layerMasks;
    std::map<wxString, LSET> layerGuiMasks;

    // Build list of layer names and their layer mask:
    for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
    {
        // Add layer name used in pcb files
        layerMasks[ LSET::Name( layer ) ] = LSET( { layer } );
        // Add layer name using GUI canonical layer name
        layerGuiMasks[ LayerName( layer ) ] = LSET( { layer } );

        // Add user layer name
        if( aBoard )
            layerGuiMasks[ aBoard->GetLayerName( layer ) ] = LSET( { layer } );
    }

    // Add list of grouped layer names used in pcb files
    layerMasks[ wxT( "*" ) ]       = LSET::AllLayersMask();
    layerMasks[ wxT( "*.Cu" ) ]    = LSET::AllCuMask();
    layerMasks[ wxT( "*In.Cu" ) ]  = LSET::InternalCuMask();
    layerMasks[ wxT( "F&B.Cu" ) ]  = LSET( { F_Cu, B_Cu } );
    layerMasks[ wxT( "*.Adhes" ) ] = LSET( { B_Adhes, F_Adhes } );
    layerMasks[ wxT( "*.Paste" ) ] = LSET( { B_Paste, F_Paste } );
    layerMasks[ wxT( "*.Mask" ) ]  = LSET( { B_Mask, F_Mask } );
    layerMasks[ wxT( "*.SilkS" ) ] = LSET( { B_SilkS, F_SilkS } );
    layerMasks[ wxT( "*.Fab" ) ]   = LSET( { B_Fab, F_Fab } );
    layerMasks[ wxT( "*.CrtYd" ) ] = LSET( { B_CrtYd, F_CrtYd } );

    // Add list of grouped layer names using GUI canonical layer names
    layerGuiMasks[ wxT( "*.Adhesive" ) ]   = LSET( { B_Adhes, F_Adhes } );
    layerGuiMasks[ wxT( "*.Silkscreen" ) ] = LSET( { B_SilkS, F_SilkS } );
    layerGuiMasks[ wxT( "*.Courtyard" ) ]  = LSET( { B_CrtYd, F_CrtYd } );

    LSEQ layerMask;

    if( !aLayerString.IsEmpty() )
    {
        wxStringTokenizer layerTokens( aLayerString, "," );

        while( layerTokens.HasMoreTokens() )
        {
            std::string token = TO_UTF8( layerTokens.GetNextToken() );

            // Search for a layer name in canonical layer name used in .kicad_pcb files:
            if( layerMasks.count( token ) )
            {
                for( PCB_LAYER_ID layer : layerMasks.at( token ).Seq() )
                    layerMask.push_back( layer );
            }
            // Search for a layer name in canonical layer name used in GUI (not translated):
            else if( layerGuiMasks.count( token ) )
            {
                for( PCB_LAYER_ID layer : layerGuiMasks.at( token ).Seq() )
                    layerMask.push_back( layer );
            }
            else
            {
                m_reporter->Report( wxString::Format( _( "Invalid layer name \"%s\"\n" ),
                                                      token ) );
            }
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

    aJob->SetTitleBlock( brd->GetTitleBlock() );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( aStepJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );

        switch( aStepJob->m_format )
        {
        case JOB_EXPORT_PCB_3D::FORMAT::VRML: fn.SetExt( FILEEXT::VrmlFileExtension );       break;
        case JOB_EXPORT_PCB_3D::FORMAT::STEP: fn.SetExt( FILEEXT::StepFileExtension );       break;
        case JOB_EXPORT_PCB_3D::FORMAT::BREP: fn.SetExt( FILEEXT::BrepFileExtension );       break;
        case JOB_EXPORT_PCB_3D::FORMAT::XAO:  fn.SetExt( FILEEXT::XaoFileExtension );        break;
        case JOB_EXPORT_PCB_3D::FORMAT::GLB:  fn.SetExt( FILEEXT::GltfBinaryFileExtension ); break;
        case JOB_EXPORT_PCB_3D::FORMAT::PLY:  fn.SetExt( FILEEXT::PlyFileExtension );        break;
        case JOB_EXPORT_PCB_3D::FORMAT::STL:  fn.SetExt( FILEEXT::StlFileExtension );        break;
        default:
            m_reporter->Report( _( "Unknown export format" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN; // shouldnt have gotten here
        }

        aStepJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = aStepJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( aStepJob->m_format == JOB_EXPORT_PCB_3D::FORMAT::VRML )
    {

        double scale = 0.0;
        switch ( aStepJob->m_vrmlUnits )
        {
        case JOB_EXPORT_PCB_3D::VRML_UNITS::MM:     scale = 1.0;         break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::METERS: scale = 0.001;       break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::TENTHS: scale = 10.0 / 25.4; break;
        case JOB_EXPORT_PCB_3D::VRML_UNITS::INCH:   scale = 1.0 / 25.4;  break;
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

        bool success = vrmlExporter.ExportVRML_File( brd->GetProject(),
                                                     &messages,
                                                     outPath,
                                                     scale,
                                                     aStepJob->m_3dparams.m_IncludeUnspecified,
                                                     aStepJob->m_3dparams.m_IncludeDNP,
                                                     !aStepJob->m_vrmlModelDir.IsEmpty(),
                                                     aStepJob->m_vrmlRelativePaths,
                                                     aStepJob->m_vrmlModelDir,
                                                     originX,
                                                     originY );

        if ( success )
        {
            m_reporter->Report( wxString::Format( _( "Successfully exported VRML to %s" ),
                                                  outPath ),
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

    aJob->SetTitleBlock( brd->GetTitleBlock() );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( aRenderJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();

        switch( aRenderJob->m_format )
        {
        case JOB_PCB_RENDER::FORMAT::JPEG: fn.SetExt( FILEEXT::JpegFileExtension ); break;
        case JOB_PCB_RENDER::FORMAT::PNG:  fn.SetExt( FILEEXT::PngFileExtension );  break;
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

    wxString outPath = aRenderJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

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

    cfg->m_Render.raytrace_lightColorTop = COLOR4D(aRenderJob->m_lightTopIntensity.x,
                                                   aRenderJob->m_lightTopIntensity.y,
                                                   aRenderJob->m_lightTopIntensity.z, 1.0);

    cfg->m_Render.raytrace_lightColorBottom = COLOR4D(aRenderJob->m_lightBottomIntensity.x,
                                                      aRenderJob->m_lightBottomIntensity.y,
                                                      aRenderJob->m_lightBottomIntensity.z, 1.0);

    cfg->m_Render.raytrace_lightColorCamera = COLOR4D( aRenderJob->m_lightCameraIntensity.x,
                                                       aRenderJob->m_lightCameraIntensity.y,
                                                       aRenderJob->m_lightCameraIntensity.z, 1.0 );

    COLOR4D lightColor( aRenderJob->m_lightSideIntensity.x,
                        aRenderJob->m_lightSideIntensity.y,
                        aRenderJob->m_lightSideIntensity.z, 1.0 );

    cfg->m_Render.raytrace_lightColor = {
        lightColor, lightColor, lightColor, lightColor,
        lightColor, lightColor, lightColor, lightColor,
    };

    int sideElevation = aRenderJob->m_lightSideElevation;

    cfg->m_Render.raytrace_lightElevation = {
        sideElevation,  sideElevation,  sideElevation,  sideElevation,
        -sideElevation, -sideElevation, -sideElevation, -sideElevation,
    };

    cfg->m_Render.raytrace_lightAzimuth = {
        45, 135, 225, 315, 45, 135, 225, 315,
    };

    cfg->m_CurrentPreset = aRenderJob->m_colorPreset;
    boardAdapter.m_Cfg = cfg;

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

    PROJECTION_TYPE projection =  aRenderJob->m_perspective ? PROJECTION_TYPE::PERSPECTIVE
                                                            : PROJECTION_TYPE::ORTHO;

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
        image.SaveFile( outPath,
                        aRenderJob->m_format == JOB_PCB_RENDER::FORMAT::PNG ? wxBITMAP_TYPE_PNG
                                                                            : wxBITMAP_TYPE_JPEG );
    }

    if( success )
    {
        m_reporter->Report( _( "Successfully created 3D render image" ) + wxS( "\n" ),
                            RPT_SEVERITY_INFO );
    }
    else
    {
        m_reporter->Report( _( "Error creating 3D render image" ) + wxS( "\n" ),
                            RPT_SEVERITY_ERROR );
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportSvg( JOB* aJob )
{
    JOB_EXPORT_PCB_SVG* aSvgJob = dynamic_cast<JOB_EXPORT_PCB_SVG*>( aJob );

    if( aSvgJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = getBoard( aSvgJob->m_filename );

    if( !brd )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( brd->GetTitleBlock() );

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

    wxString outPath = aSvgJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, aSvgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    loadOverrideDrawingSheet( brd, aSvgJob->m_drawingSheet );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();
    aSvgJob->m_plotLayerSequence = convertLayerArg( aSvgJob->m_argLayers, brd );
    aSvgJob->m_plotOnAllLayersSequence = convertLayerArg( aSvgJob->m_argCommonLayers, brd );

    if( aSvgJob->m_plotLayerSequence.size() < 1 )
    {
        m_reporter->Report( _( "At least one layer must be specified\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, aSvgJob, *m_reporter );

    PCB_PLOTTER     plotter( brd, m_reporter, plotOpts );

    std::optional<wxString> layerName;
    std::optional<wxString> sheetName;
    std::optional<wxString> sheetPath;

    if( aSvgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE )
    {
        if( aJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
            layerName = aSvgJob->GetVarOverrides().at( wxT( "LAYER" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
            sheetName = aSvgJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

        if( aJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
            sheetPath = aSvgJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );
    }

    LOCALE_IO dummy;

    if( !plotter.Plot( outPath, aSvgJob->m_plotLayerSequence, aSvgJob->m_plotOnAllLayersSequence,
                       false, aSvgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE,
                       layerName, sheetName, sheetPath ) )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

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

    aJob->SetTitleBlock( brd->GetTitleBlock() );
    loadOverrideDrawingSheet( brd, aDxfJob->m_drawingSheet );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();
    aDxfJob->m_plotLayerSequence = convertLayerArg( aDxfJob->m_argLayers, brd );
    aDxfJob->m_plotOnAllLayersSequence = convertLayerArg( aDxfJob->m_argCommonLayers, brd );

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

    wxString outPath = aDxfJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, aDxfJob->m_genMode == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, aDxfJob, *m_reporter);

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

    if( !plotter.Plot( outPath, aDxfJob->m_plotLayerSequence, aDxfJob->m_plotOnAllLayersSequence,
                       false, aDxfJob->m_genMode == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE,
                       layerName, sheetName, sheetPath ) )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

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

    aJob->SetTitleBlock( brd->GetTitleBlock() );
    loadOverrideDrawingSheet( brd, aPdfJob->m_drawingSheet );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();
    aPdfJob->m_plotLayerSequence = convertLayerArg( aPdfJob->m_argLayers, brd );
    aPdfJob->m_plotOnAllLayersSequence = convertLayerArg( aPdfJob->m_argCommonLayers, brd );

    if( aPdfJob->m_plotLayerSequence.size() < 1 )
    {
        m_reporter->Report( _( "At least one layer must be specified\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    if( aPdfJob->m_pdfGenMode == JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE
        && aPdfJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::PDF ) );

        aPdfJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    PCB_PLOT_PARAMS plotOpts;
    PCB_PLOTTER::PlotJobToPlotOpts( plotOpts, aPdfJob, *m_reporter );

    int returnCode = CLI::EXIT_CODES::OK;

    // ensure this is set for this one gen mode
    if( aPdfJob->m_pdfGenMode == JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE )
    {
        plotOpts.m_PDFSingle = true;
    }

    PCB_PLOTTER pcbPlotter( brd, m_reporter, plotOpts );

    wxString outPath = aPdfJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, aPdfJob->m_pdfGenMode == JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    std::optional<wxString> layerName;
    std::optional<wxString> sheetName;
    std::optional<wxString> sheetPath;
    if( aPdfJob->m_pdfGenMode == JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE )
    {
        if( aPdfJob->GetVarOverrides().contains( wxT( "LAYER" ) ) )
            layerName = aPdfJob->GetVarOverrides().at( wxT( "LAYER" ) );

        if( aPdfJob->GetVarOverrides().contains( wxT( "SHEETNAME" ) ) )
            sheetName = aPdfJob->GetVarOverrides().at( wxT( "SHEETNAME" ) );

        if( aPdfJob->GetVarOverrides().contains( wxT( "SHEETPATH" ) ) )
            sheetPath = aPdfJob->GetVarOverrides().at( wxT( "SHEETPATH" ) );
    }


    LOCALE_IO dummy;

    if( !pcbPlotter.Plot( outPath, aPdfJob->m_plotLayerSequence,
                          aPdfJob->m_plotOnAllLayersSequence, false,
                          aPdfJob->m_pdfGenMode == JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE,
                          layerName, sheetName, sheetPath ) )
    {
        returnCode = CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return returnCode;
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

    wxString outPath = aGerberJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, false ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    aJob->SetTitleBlock( brd->GetTitleBlock() );
    loadOverrideDrawingSheet( brd, aGerberJob->m_drawingSheet );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

    if( !aGerberJob->m_argLayers.empty() )
        aGerberJob->m_plotLayerSequence = convertLayerArg( aGerberJob->m_argLayers, nullptr );
    else
        aGerberJob->m_plotLayerSequence = LSET::AllLayersMask().SeqStackupForPlotting();

    aGerberJob->m_plotOnAllLayersSequence = convertLayerArg( aGerberJob->m_argCommonLayers, brd );

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
        // default to the board enabled layers
        if( aGerberJob->m_plotLayerSequence.empty() )
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
            populateGerberPlotOptionsFromJob( plotOpts, aGerberJob );

        if( plotOpts.GetUseGerberProtelExtensions() )
            fileExt = GetGerberProtelExtension( layer );
        else
            fileExt = FILEEXT::GerberFileExtension;

        BuildPlotFileName( &fn, outPath, layerName, fileExt );
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
            plotter = (GERBER_PLOTTER*) StartPlotBoard( brd, &plotOpts, layer, layerName,
                                                        fn.GetFullPath(), sheetName, sheetPath );
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

    if( aGerberJob->m_createJobsFile )
    {
        wxFileName fn( brd->GetFileName() );

        // Build gerber job file from basename
        BuildPlotFileName( &fn, outPath, wxT( "job" ), FILEEXT::GerberJobFileExtension );
        jobfile_writer.CreateJobFile( fn.GetFullPath() );
    }

    return exitCode;
}

int PCBNEW_JOBS_HANDLER::JobExportGencad( JOB* aJob )
{
    JOB_EXPORT_PCB_GENCAD* aGencadJob = dynamic_cast<JOB_EXPORT_PCB_GENCAD*>( aJob );

    if( aGencadJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    BOARD* brd = LoadBoard( aGencadJob->m_filename, true ); // Ensure m_board is of type BOARD*

    if( brd == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

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

    wxString outPath = aGencadJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( !exporter.WriteFile( outPath ) )
    {
        m_reporter->Report( wxString::Format( _( "Failed to create file '%s'.\n" ), outPath ),
                            RPT_SEVERITY_ERROR );

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
    aPlotOpts.SetUseAuxOrigin( aJob->m_useDrillOrigin );
    aPlotOpts.SetUseGerberProtelExtensions( aJob->m_useProtelFileExtension );
    aPlotOpts.SetGerberPrecision( aJob->m_precision );
}


void PCBNEW_JOBS_HANDLER::populateGerberPlotOptionsFromJob( PCB_PLOT_PARAMS& aPlotOpts,
                                                            JOB_EXPORT_PCB_GERBERS* aJob )
{
    populateGerberPlotOptionsFromJob( aPlotOpts, static_cast<JOB_EXPORT_PCB_GERBER*>( aJob ) );

    aPlotOpts.SetCreateGerberJobFile( aJob->m_createJobsFile );
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

    aJob->SetTitleBlock( brd->GetTitleBlock() );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();
    aGerberJob->m_plotLayerSequence = convertLayerArg( aGerberJob->m_argLayers, brd );
    aGerberJob->m_plotOnAllLayersSequence = convertLayerArg( aGerberJob->m_argCommonLayers, brd );

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

    PCB_PLOT_PARAMS plotOpts;
    populateGerberPlotOptionsFromJob( plotOpts, aGerberJob );
    plotOpts.SetLayerSelection( aGerberJob->m_plotLayerSequence );
    plotOpts.SetPlotOnAllLayersSequence( aGerberJob->m_plotOnAllLayersSequence );

    PCB_LAYER_ID layer = UNDEFINED_LAYER;
    wxString     layerName;
    wxString     sheetName;
    wxString     sheetPath;
    wxString     outPath = aGerberJob->GetFullOutputPath( brd->GetProject() );

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
    PLOTTER* plotter = StartPlotBoard( brd, &plotOpts, layer, layerName, outPath, sheetName,
                                       sheetPath );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aGerberJob->m_plotLayerSequence, plotOpts );
        plotter->EndPlot();
    }
    else
    {
        m_reporter->Report( wxString::Format( _( "Failed to plot to '%s'.\n" ), outPath ),
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

    aJob->SetTitleBlock( brd->GetTitleBlock() );

    wxString outPath = aDrillJob->GetFullOutputPath( brd->GetProject() );

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

        if( aDrillJob->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCH )
            precision = precisionListForInches;
        else
            precision = precisionListForMetric;

        EXCELLON_WRITER* excellonWriter = dynamic_cast<EXCELLON_WRITER*>( drillWriter.get() );

        if( excellonWriter == nullptr )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        excellonWriter->SetFormat( aDrillJob->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MM,
                                   zeroFmt, precision.m_Lhs, precision.m_Rhs );
        excellonWriter->SetOptions( aDrillJob->m_excellonMirrorY,
                                    aDrillJob->m_excellonMinimalHeader,
                                    offset, aDrillJob->m_excellonCombinePTHNPTH );
        excellonWriter->SetRouteModeForOvalHoles( aDrillJob->m_excellonOvalDrillRoute );
        excellonWriter->SetMapFileFormat( mapFormat );

        if( !excellonWriter->CreateDrillandMapFilesSet( outPath, true, aDrillJob->m_generateMap,
                                                        m_reporter ) )
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

        if( !gerberWriter->CreateDrillandMapFilesSet( outPath, true, aDrillJob->m_generateMap,
                                                      m_reporter ) )
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
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( brd->GetTitleBlock() );

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

    wxString outPath = aPosJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII
        || aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
    {
        wxFileName fn( outPath );
        wxString   baseName = fn.GetName();

        auto exportPlaceFile =
                [&]( bool frontSide, bool backSide, const wxString& curr_outPath ) -> bool
                {
                    FILE* file = wxFopen( curr_outPath, wxS( "wt" ) );
                    wxCHECK( file, false );

                    PLACE_FILE_EXPORTER exporter( brd,
                                          aPosJob->m_units == JOB_EXPORT_PCB_POS::UNITS::MM,
                                          aPosJob->m_smdOnly,
                                          aPosJob->m_excludeFootprintsWithTh,
                                          aPosJob->m_excludeDNP,
                                          frontSide,
                                          backSide,
                                          aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV,
                                          aPosJob->m_useDrillPlaceFileOrigin,
                                          aPosJob->m_negateBottomX );

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
                m_reporter->Report( wxString::Format( _( "Wrote front position data to '%s'.\n" ),
                                                      fn.GetFullPath() ),
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
                m_reporter->Report( wxString::Format( _( "Wrote back position data to '%s'.\n" ),
                                                      fn.GetFullPath() ),
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
                m_reporter->Report( wxString::Format( _( "Wrote position data to '%s'.\n" ),
                                                      fn.GetFullPath() ),
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
        PCB_LAYER_ID            gbrLayer = F_Cu;
        wxString                outPath_base = outPath;

        if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::FRONT
                || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH )
        {
            if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH || !aPosJob->m_nakedFilename )
                outPath = exporter.GetPlaceFileName( outPath, gbrLayer );

            if( exporter.CreatePlaceFile( outPath, gbrLayer, aPosJob->m_gerberBoardEdge ) >= 0 )
            {
                m_reporter->Report( wxString::Format( _( "Wrote front position data to '%s'.\n" ),
                                                      outPath ),
                                    RPT_SEVERITY_ACTION );

                aPosJob->AddOutput( outPath );
            }
            else
            {
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }

        if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BACK
                || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH )
        {
            gbrLayer = B_Cu;

            outPath = outPath_base;

            if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH || !aPosJob->m_nakedFilename )
                outPath = exporter.GetPlaceFileName( outPath, gbrLayer );

            if( exporter.CreatePlaceFile( outPath, gbrLayer, aPosJob->m_gerberBoardEdge ) >= 0 )
            {
                m_reporter->Report( wxString::Format( _( "Wrote back position data to '%s'.\n" ),
                                                      outPath ),
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
        if( wxFile::Exists( upgradeJob->m_outputLibraryPath )
            || wxDir::Exists( upgradeJob->m_outputLibraryPath) )
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

    PCB_IO_KICAD_SEXPR pcb_io( CTL_FOR_LIBRARY );
    FP_CACHE   fpLib( &pcb_io, svgJob->m_libraryPath );

    if( !svgJob->m_argLayers.empty() )
        svgJob->m_plotLayerSequence = convertLayerArg( svgJob->m_argLayers, nullptr );
    else
        svgJob->m_plotLayerSequence = LSET::AllLayersMask().SeqStackupForPlotting();

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

    int  exitCode = CLI::EXIT_CODES::OK;
    bool singleFpPlotted = false;

    for( const auto& [fpName, fpCacheEntry] : fpLib.GetFootprints() )
    {
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

    aJob->SetTitleBlock( brd->GetTitleBlock() );
    brd->GetProject()->ApplyTextVars( aJob->GetVarOverrides() );
    brd->SynchronizeProperties();

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

    wxString outPath = drcJob->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    EDA_UNITS units;

    switch( drcJob->m_units )
    {
    case JOB_PCB_DRC::UNITS::INCH: units = EDA_UNITS::INCH;   break;
    case JOB_PCB_DRC::UNITS::MILS: units = EDA_UNITS::MILS; break;
    case JOB_PCB_DRC::UNITS::MM:   units = EDA_UNITS::MM;   break;
    default:                       units = EDA_UNITS::MM;   break;
    }

    std::shared_ptr<DRC_ENGINE> drcEngine = brd->GetDesignSettings().m_DRCEngine;
    std::unique_ptr<NETLIST>    netlist = std::make_unique<NETLIST>();

    drcEngine->SetDrawingSheet( getDrawingSheetProxyView( brd ) );

    // BOARD_COMMIT uses TOOL_MANAGER to grab the board internally so we must give it one
    TOOL_MANAGER* toolManager = new TOOL_MANAGER;
    toolManager->SetEnvironment( brd, nullptr, nullptr, Kiface().KifaceSettings(), nullptr );

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
            schematicPath.SetExt( FILEEXT::KiCadSchematicFileExtension );

            if( !schematicPath.Exists() )
                schematicPath.SetExt( FILEEXT::LegacySchematicFileExtension );

            if( !schematicPath.Exists() )
            {
                m_reporter->Report( _( "Failed to fetch schematic netlist for parity tests.\n" ),
                                    RPT_SEVERITY_ERROR );
                checkParity = false;
            }
            else
            {
                typedef bool ( *NETLIST_FN_PTR )( const wxString&, std::string& );
                KIFACE* eeschema = m_kiway->KiFACE( KIWAY::FACE_SCH );
                NETLIST_FN_PTR netlister =
                        (NETLIST_FN_PTR) eeschema->IfaceOrAddress( KIFACE_NETLIST_SCHEMATIC );
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
            STRING_LINE_READER* lineReader = new STRING_LINE_READER( netlist_str,
                                                                     _( "Eeschema netlist" ) );
            KICAD_NETLIST_READER netlistReader( lineReader, netlist.get() );

            netlistReader.LoadNetlist();
        }
        catch( const IO_ERROR& )
        {
            m_reporter->Report( _( "Failed to fetch schematic netlist for parity tests.\n" ),
                                RPT_SEVERITY_ERROR );
            checkParity = false;
        }

        drcEngine->SetSchematicNetlist( netlist.get() );
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
    drcEngine->RunTests( units, drcJob->m_reportAllTrackErrors, checkParity );
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

    if( checkParity )
    {
        m_reporter->Report( wxString::Format( _( "Found %d schematic parity issues\n" ),
                                              fpWarningsProvider->GetCount() ),
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
        m_reporter->Report( wxString::Format( _( "Unable to save DRC report to %s\n" ), outPath ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    m_reporter->Report( wxString::Format( _( "Saved DRC Report to %s\n" ), outPath ),
                        RPT_SEVERITY_ACTION );

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

    aJob->SetTitleBlock( brd->GetTitleBlock() );

    if( job->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( FILEEXT::Ipc2581FileExtension );

        job->SetWorkingOutputPath( fn.GetName() );
    }

    wxString outPath = job->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    std::map<std::string, UTF8> props;
    props["units"] = job->m_units == JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM ? "mm" : "inch";
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
        m_reporter->Report( wxString::Format( _( "Error generating IPC-2581 file '%s'.\n%s" ),
                                              job->m_filename,
                                              ioe.What() ),
                            RPT_SEVERITY_ERROR );

        wxRemoveFile( tempFile );

        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( job->m_compress )
    {
        wxFileName tempfn = outPath;
        tempfn.SetExt( FILEEXT::Ipc2581FileExtension );
        wxFileName zipfn = tempFile;
        zipfn.SetExt( "zip" );

        {
            wxFFileOutputStream fnout( zipfn.GetFullPath() );
            wxZipOutputStream   zip( fnout );
            wxFFileInputStream  fnin( tempFile );

            zip.PutNextEntry( tempfn.GetFullName() );
            fnin.Read( zip );
        }

        wxRemoveFile( tempFile );
        tempFile = zipfn.GetFullPath();
    }

    // If save succeeded, replace the original with what we just wrote
    if( !wxRenameFile( tempFile, outPath ) )
    {
        m_reporter->Report( wxString::Format( _( "Error generating IPC-2581 file '%s'.\n"
                                                 "Failed to rename temporary file '%s." ),
                                              outPath,
                                              tempFile ),
                            RPT_SEVERITY_ERROR );
    }

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

    aJob->SetTitleBlock( brd->GetTitleBlock() );

    if( job->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( FILEEXT::IpcD356FileExtension );

        job->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = job->GetFullOutputPath( brd->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    IPC356D_WRITER exporter( brd );

    bool success = exporter.Write( outPath );

    if( success )
    {
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

    aJob->SetTitleBlock( brd->GetTitleBlock() );

    wxString   path = job->GetConfiguredOutputPath();

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
            case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP:
                fn.SetExt( FILEEXT::ArchiveFileExtension );
                break;
            case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ:
                fn.SetExt( "tgz" );
                break;
            default:
                break;
            };

            job->SetWorkingOutputPath( fn.GetFullName() );
        }
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
