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

#include "eeschema_jobs_handler.h"
#include <algorithm>
#include <common.h>
#include <pgm_base.h>
#include <kiface_base.h>
#include <kiway_player.h>
#include <cli/exit_codes.h>
#include <sch_plotter.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <font/kicad_font_name.h>
#include <jobs/job_export_bom.h>
#include <jobs/job_export_sch_pythonbom.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_sch_erc.h>
#include <jobs/job_sch_import.h>
#include <jobs/job_sch_upgrade.h>
#include <jobs/job_sch_downgrade.h>
#include <downgrade_target.h>
#include <downgrade/sch_downgrade.h>
#include <downgrade_scan.h>
#include <downgrade_copy.h>
#include <bus_alias.h>
#include <lib_symbol.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_sheet_path.h>
#include <set>
#include <wx/ffile.h>
#include <jobs/job_import_utils.h>
#include <jobs/job_sym_export_svg.h>
#include <jobs/job_sym_upgrade.h>
#include <schematic.h>
#include <import_net_map.h>
#include <schematic_settings.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_commit.h>
#include <symbol_import_reconciler.h>
#include <save_project_utils.h>
#include <tool/tool_manager.h>
#include <project.h>
#include <project/project_file.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <memory>
#include <connection_graph.h>
#include "eeschema_helpers.h"
#include <filename_resolver.h>
#include <kiway.h>
#include <sch_painter.h>
#include <locale_io.h>
#include <erc/erc.h>
#include <erc/erc_report.h>
#include <erc/erc_settings.h>
#include <wildcards_and_files_ext.h>
#include <plotters/plotters_pslike.h>
#include <drawing_sheet/ds_data_model.h>
#include <paths.h>
#include <reporter.h>
#include <scoped_set_reset.h>
#include <string_utils.h>

#include <settings/settings_manager.h>

#include <sch_file_versions.h>
#include <sch_io/sch_io.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_lib_cache.h>

#include <netlist.h>
#include <netlist_exporter_base.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_spice.h>
#include <netlist_exporter_spice_model.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_xml.h>
#include <netlist_exporter_pads.h>
#include <netlist_exporter_allegro.h>

#include <symbol_fields_data_model.h>

#include <dialogs/dialog_export_netlist.h>
#include <dialogs/dialog_plot_schematic.h>
#include <dialogs/dialog_erc_job_config.h>
#include <dialogs/dialog_symbol_fields_table.h>
#include <confirm.h>
#include <project_sch.h>

#include <libraries/symbol_library_adapter.h>


EESCHEMA_JOBS_HANDLER::EESCHEMA_JOBS_HANDLER( KIWAY* aKiway ) :
        JOB_DISPATCHER( aKiway ),
        m_cliSchematic( nullptr )
{
    Register( "bom", std::bind( &EESCHEMA_JOBS_HANDLER::JobExportBom, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_BOM* bomJob = dynamic_cast<JOB_EXPORT_BOM*>( job );

                  SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( aKiway->Player( FRAME_SCH, false ) );

                  wxCHECK( bomJob && editFrame, false );

                  DIALOG_SYMBOL_FIELDS_TABLE dlg( editFrame, bomJob );

                  if( dlg.WasAborted() )
                      return false;

                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "pythonbom", std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPythonBom, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "netlist", std::bind( &EESCHEMA_JOBS_HANDLER::JobExportNetlist, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_SCH_NETLIST* netJob = dynamic_cast<JOB_EXPORT_SCH_NETLIST*>( job );

                  SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( aKiway->Player( FRAME_SCH, false ) );

                  wxCHECK( netJob && editFrame, false );

                  DIALOG_EXPORT_NETLIST dlg( editFrame, aParent, netJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "plot", std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPlot, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_SCH_PLOT* plotJob = dynamic_cast<JOB_EXPORT_SCH_PLOT*>( job );

                  SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( aKiway->Player( FRAME_SCH, false ) );

                  wxCHECK( plotJob && editFrame, false );

                  if( plotJob->m_plotFormat == SCH_PLOT_FORMAT::HPGL )
                  {
                      DisplayErrorMessage( editFrame,
                                           _( "Plotting to HPGL is no longer supported as of KiCad 10.0." ) );
                      return false;
                  }

                  DIALOG_PLOT_SCHEMATIC dlg( editFrame, aParent, plotJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "symupgrade", std::bind( &EESCHEMA_JOBS_HANDLER::JobSymUpgrade, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "symsvg", std::bind( &EESCHEMA_JOBS_HANDLER::JobSymExportSvg, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "sch_diff", std::bind( &EESCHEMA_JOBS_HANDLER::JobSchDiff, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "sym_diff", std::bind( &EESCHEMA_JOBS_HANDLER::JobSymDiff, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "erc", std::bind( &EESCHEMA_JOBS_HANDLER::JobSchErc, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_SCH_ERC* ercJob = dynamic_cast<JOB_SCH_ERC*>( job );

                  wxCHECK( ercJob, false );

                  DIALOG_ERC_JOB_CONFIG dlg( aParent, ercJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "upgrade", std::bind( &EESCHEMA_JOBS_HANDLER::JobUpgrade, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "downgrade", std::bind( &EESCHEMA_JOBS_HANDLER::JobDowngrade, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "sch_import", std::bind( &EESCHEMA_JOBS_HANDLER::JobImport, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
}


void EESCHEMA_JOBS_HANDLER::ClearCachedSchematic()
{
    if( m_cliSchematic )
        m_cliSchematic->SetProject( nullptr );

    delete m_cliSchematic;
    m_cliSchematic = nullptr;
    m_cliSchematicRootValidated = false;
}


SCHEMATIC* EESCHEMA_JOBS_HANDLER::getSchematic( const wxString& aPath, bool aRequireRoot )
{
    SCHEMATIC* sch = nullptr;

    if( !Pgm().IsGUI() && Pgm().GetSettingsManager().IsProjectOpenNotDummy() )
    {
        PROJECT& project = Pgm().GetSettingsManager().Prj();
        wxString schPath = aPath;

        if( schPath.IsEmpty() )
        {
            wxFileName path = project.GetProjectFullName();
            path.SetExt( FILEEXT::KiCadSchematicFileExtension );
            path.MakeAbsolute();
            schPath = path.GetFullPath();
        }

        if( m_cliSchematic && aRequireRoot && !m_cliSchematicRootValidated )
            ClearCachedSchematic();

        if( !m_cliSchematic )
        {
            m_cliSchematic = EESCHEMA_HELPERS::LoadSchematic(
                    schPath, true, false, &project, true, aRequireRoot ? m_reporter : nullptr );
            m_cliSchematicRootValidated = aRequireRoot;
        }

        sch = m_cliSchematic;
    }
    else if( Pgm().IsGUI() && Pgm().GetSettingsManager().IsProjectOpen() )
    {
        SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( m_kiway->Player( FRAME_SCH, false ) );

        if( editFrame )
            sch = &editFrame->Schematic();
    }
    else if( !aPath.IsEmpty() )
    {
        sch = EESCHEMA_HELPERS::LoadSchematic(
                aPath, true, false, nullptr, true, aRequireRoot ? m_reporter : nullptr );
    }

    if( !sch )
        m_reporter->Report( _( "Failed to load schematic\n" ), RPT_SEVERITY_ERROR );

    return sch;
}

void EESCHEMA_JOBS_HANDLER::InitRenderSettings( SCH_RENDER_SETTINGS* aRenderSettings, const wxString& aTheme,
                                                SCHEMATIC* aSch, const wxString& aDrawingSheetOverride )
{
    COLOR_SETTINGS* cs = ::GetColorSettings( aTheme );
    aRenderSettings->LoadColors( cs );
    aRenderSettings->m_ShowHiddenPins = false;
    aRenderSettings->m_ShowHiddenFields = false;
    aRenderSettings->m_ShowPinAltIcons = false;

    aRenderSettings->SetDefaultPenWidth( aSch->Settings().m_DefaultLineWidth );
    aRenderSettings->m_LabelSizeRatio = aSch->Settings().m_LabelSizeRatio;
    aRenderSettings->m_TextOffsetRatio = aSch->Settings().m_TextOffsetRatio;
    aRenderSettings->m_PinSymbolSize = aSch->Settings().m_PinSymbolSize;
    aRenderSettings->m_ShowDNPMarkers = aSch->Settings().m_ShowDNPMarkers;

    aRenderSettings->SetDashLengthRatio( aSch->Settings().m_DashedLineDashRatio );
    aRenderSettings->SetGapLengthRatio( aSch->Settings().m_DashedLineGapRatio );

    // Load the drawing sheet from the filename stored in BASE_SCREEN::m_DrawingSheetFileName.
    // If empty, or not existing, the default drawing sheet is loaded.

    auto loadSheet = [&]( const wxString& path ) -> bool
    {
        wxString msg;

        if( !DS_DATA_MODEL::GetTheInstance().LoadFromName( path, wxGetCwd(), &aSch->Project(),
                                                           { aSch->GetEmbeddedFiles() }, &msg ) )
        {
            m_reporter->Report( wxString::Format( _( "Error loading drawing sheet '%s'." ), path ) + wxS( "\n" ) + msg
                                        + wxS( "\n" ),
                                RPT_SEVERITY_ERROR );
            return false;
        }

        return true;
    };

    // try to load the override first
    if( !aDrawingSheetOverride.IsEmpty() && loadSheet( aDrawingSheetOverride ) )
        return;

    if( aSch->Settings().m_SchDrawingSheetFileName == wxS( "empty.kicad_wks" ) )
    {
        DS_DATA_MODEL::GetTheInstance().SetEmptyLayout();
        return;
    }

    // no override or failed override continues here
    loadSheet( aSch->Settings().m_SchDrawingSheetFileName );
}


int EESCHEMA_JOBS_HANDLER::JobExportPlot( JOB* aJob )
{
    JOB_EXPORT_SCH_PLOT* aPlotJob = dynamic_cast<JOB_EXPORT_SCH_PLOT*>( aJob );

    wxCHECK( aPlotJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    if( aPlotJob->m_plotFormat == SCH_PLOT_FORMAT::HPGL )
    {
        m_reporter->Report( _( "Plotting to HPGL is no longer supported as of KiCad 10.0.\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    SCHEMATIC* sch = getSchematic( aPlotJob->m_filename );

    if( !sch )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( sch->RootScreen()->GetTitleBlock() );
    sch->Project().ApplyTextVars( aJob->GetVarOverrides() );

    // Determine the variant to use.  The dialog edit path writes m_variant (the scalar),
    // while the CLI path populates m_variantNames directly.  Prefer the scalar so a
    // dialog-edited selection always wins over a stale list left over from CLI input.
    wxString variantName;

    if( !aPlotJob->m_variant.IsEmpty() )
        variantName = aPlotJob->m_variant;
    else if( !aPlotJob->m_variantNames.empty() )
        variantName = aPlotJob->m_variantNames.front();

    if( !variantName.IsEmpty() && variantName != wxS( "all" ) )
        sch->SetCurrentVariant( variantName );

    std::unique_ptr<SCH_RENDER_SETTINGS> renderSettings = std::make_unique<SCH_RENDER_SETTINGS>();
    InitRenderSettings( renderSettings.get(), aPlotJob->m_theme, sch, aPlotJob->m_drawingSheet );

    wxString font = aPlotJob->m_defaultFont;

    if( font.IsEmpty() )
    {
        EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
        font = cfg ? cfg->m_Appearance.default_font : wxString( KICAD_FONT_NAME );
    }

    renderSettings->SetDefaultFont( font );
    renderSettings->SetMinPenWidth( aPlotJob->m_minPenWidth );

    // Clear cached bounding boxes for all text items so they're recomputed with the correct
    // default font. This is necessary because text bounding boxes may have been cached during
    // schematic loading before the render settings (and thus default font) were configured.
    SCH_SCREENS screens( sch->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items() )
            item->ClearCaches();

        for( const auto& [libItemName, libSymbol] : screen->GetLibSymbols() )
            libSymbol->ClearCaches();
    }

    std::unique_ptr<SCH_PLOTTER> schPlotter = std::make_unique<SCH_PLOTTER>( sch );

    PLOT_FORMAT format = PLOT_FORMAT::PDF;

    switch( aPlotJob->m_plotFormat )
    {
    case SCH_PLOT_FORMAT::DXF: format = PLOT_FORMAT::DXF; break;
    case SCH_PLOT_FORMAT::PDF: format = PLOT_FORMAT::PDF; break;
    case SCH_PLOT_FORMAT::SVG: format = PLOT_FORMAT::SVG; break;
    case SCH_PLOT_FORMAT::POST: format = PLOT_FORMAT::POST; break;
    case SCH_PLOT_FORMAT::PNG: format = PLOT_FORMAT::PNG; break;
    case SCH_PLOT_FORMAT::HPGL: /* no longer supported */ break;
    }

    int pageSizeSelect = PageFormatReq::PAGE_SIZE_AUTO;

    switch( aPlotJob->m_pageSizeSelect )
    {
    case JOB_PAGE_SIZE::PAGE_SIZE_A: pageSizeSelect = PageFormatReq::PAGE_SIZE_A; break;
    case JOB_PAGE_SIZE::PAGE_SIZE_A4: pageSizeSelect = PageFormatReq::PAGE_SIZE_A4; break;
    case JOB_PAGE_SIZE::PAGE_SIZE_AUTO: pageSizeSelect = PageFormatReq::PAGE_SIZE_AUTO; break;
    }

    if( !aPlotJob->GetOutputPathIsDirectory() && aPlotJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( format ) );

        aPlotJob->SetConfiguredOutputPath( fn.GetFullName() );
    }

    wxString outPath = aPlotJob->GetFullOutputPath( &sch->Project() );

    if( !PATHS::EnsurePathExists( outPath, !aPlotJob->GetOutputPathIsDirectory() ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    SCH_PLOT_OPTS plotOpts;
    plotOpts.m_blackAndWhite = aPlotJob->m_blackAndWhite;
    plotOpts.m_PDFPropertyPopups = aPlotJob->m_PDFPropertyPopups;
    plotOpts.m_PDFHierarchicalLinks = aPlotJob->m_PDFHierarchicalLinks;
    plotOpts.m_PDFMetadata = aPlotJob->m_PDFMetadata;

    if( aPlotJob->GetOutputPathIsDirectory() )
    {
        plotOpts.m_outputDirectory = outPath;
        plotOpts.m_outputFile = wxEmptyString;
    }
    else
    {
        plotOpts.m_outputDirectory = wxEmptyString;
        plotOpts.m_outputFile = outPath;
    }

    plotOpts.m_pageSizeSelect = pageSizeSelect;
    plotOpts.m_plotAll = aPlotJob->m_plotAll;
    plotOpts.m_plotDrawingSheet = aPlotJob->m_plotDrawingSheet;
    plotOpts.m_plotPages = aPlotJob->m_plotPages;

    if( !aPlotJob->m_sheetPath.IsEmpty() )
        plotOpts.m_sheetPath = sch->Hierarchy().GetSheetPathByKIIDPath( KIID_PATH( aPlotJob->m_sheetPath ) );

    plotOpts.m_theme = aPlotJob->m_theme;
    plotOpts.m_useBackgroundColor = aPlotJob->m_useBackgroundColor;
    plotOpts.m_plotHopOver = aPlotJob->m_show_hop_over;

    if( !variantName.IsEmpty() )
        plotOpts.m_variant = variantName;

    // Always export dxf in mm by kicad-cli (similar to Pcbnew)
    plotOpts.m_DXF_File_Unit = DXF_UNITS::MM;

    if( aPlotJob->m_plotFormat == SCH_PLOT_FORMAT::PNG )
    {
        JOB_EXPORT_SCH_PLOT_PNG* pngJob = static_cast<JOB_EXPORT_SCH_PLOT_PNG*>( aPlotJob );
        plotOpts.m_pngDPI = pngJob->m_dpi;
        plotOpts.m_pngAntialias = pngJob->m_antialias;
    }

    schPlotter->Plot( format, plotOpts, renderSettings.get(), m_reporter );

    if( m_reporter->HasMessageOfSeverity( RPT_SEVERITY_ERROR ) )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    for( const wxString& outputPath : schPlotter->GetOutputFilePaths() )
        aJob->AddOutput( outputPath );

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportNetlist( JOB* aJob )
{
    JOB_EXPORT_SCH_NETLIST* aNetJob = dynamic_cast<JOB_EXPORT_SCH_NETLIST*>( aJob );

    wxCHECK( aNetJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    SCHEMATIC* sch = getSchematic( aNetJob->m_filename );

    if( !sch )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( sch->RootScreen()->GetTitleBlock() );
    sch->Project().ApplyTextVars( aJob->GetVarOverrides() );

    // Apply variant if specified
    if( !aNetJob->m_variantNames.empty() )
    {
        // For netlist export, we use the first variant name from the set
        wxString variantName = *aNetJob->m_variantNames.begin();

        if( variantName != wxS( "all" ) )
            sch->SetCurrentVariant( variantName );
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->Hierarchy().GetSymbols( referenceList, SYMBOL_FILTER_NON_POWER );

    if( referenceList.GetCount() > 0 )
    {
        if( referenceList.CheckAnnotation(
                    []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* )
                    {
                        // We're only interested in the end result -- either errors or not
                    } )
            > 0 )
        {
            m_reporter->Report( _( "Warning: schematic has annotation errors, please use the "
                                   "schematic editor to fix them\n" ),
                                RPT_SEVERITY_WARNING );
        }
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( sch );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
        m_reporter->Report( _( "Warning: duplicate sheet names.\n" ), RPT_SEVERITY_WARNING );

    std::unique_ptr<NETLIST_EXPORTER_BASE> helper;
    unsigned                               netlistOption = 0;

    wxString fileExt;

    switch( aNetJob->format )
    {
    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADSEXPR:
        fileExt = FILEEXT::NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_KICAD>( sch, m_kiway );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::ORCADPCB2:
        fileExt = FILEEXT::OrCadPcb2NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_ORCADPCB2>( sch, m_kiway );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::CADSTAR:
        fileExt = FILEEXT::CadstarNetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_CADSTAR>( sch, m_kiway );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICE:
        fileExt = FILEEXT::SpiceFileExtension;
        netlistOption = NETLIST_EXPORTER_SPICE::OPTION_SIM_COMMAND;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE>( sch, m_kiway );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICEMODEL:
        fileExt = FILEEXT::SpiceFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE_MODEL>( sch, m_kiway );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADXML:
        fileExt = wxS( "xml" );
        helper = std::make_unique<NETLIST_EXPORTER_XML>( sch, m_kiway );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::PADS:
        fileExt = wxS( "asc" );
        helper = std::make_unique<NETLIST_EXPORTER_PADS>( sch, m_kiway );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::ALLEGRO:
        fileExt = wxS( "txt" );
        helper = std::make_unique<NETLIST_EXPORTER_ALLEGRO>( sch, m_kiway );
        break;

    default:
        m_reporter->Report( _( "Unknown netlist format.\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( aNetJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( fileExt );

        aNetJob->SetConfiguredOutputPath( fn.GetFullName() );
    }

    wxString outPath = aNetJob->GetFullOutputPath( &sch->Project() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    bool res = helper->WriteNetlist( outPath, netlistOption, *m_reporter );

    if( !res )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    aJob->AddOutput( outPath );

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportBom( JOB* aJob )
{
    JOB_EXPORT_BOM* aBomJob = dynamic_cast<JOB_EXPORT_BOM*>( aJob );

    wxCHECK( aBomJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    SCHEMATIC* sch = getSchematic( aBomJob->m_filename );

    if( !sch )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( sch->RootScreen()->GetTitleBlock() );
    sch->Project().ApplyTextVars( aJob->GetVarOverrides() );

    wxString currentVariant = aBomJob->GetSelectedVariant();

    if( !currentVariant.IsEmpty() && currentVariant != wxS( "all" ) )
        sch->SetCurrentVariant( currentVariant );

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->Hierarchy().GetSymbols( referenceList, SYMBOL_FILTER_NON_POWER, false );

    if( referenceList.GetCount() > 0 )
    {
        SCH_REFERENCE_LIST copy = referenceList;

        // Check annotation splits references...
        if( copy.CheckAnnotation(
                    []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* )
                    {
                        // We're only interested in the end result -- either errors or not
                    } )
            > 0 )
        {
            m_reporter->Report( _( "Warning: schematic has annotation errors, please use the schematic "
                                   "editor to fix them\n" ),
                                RPT_SEVERITY_WARNING );
        }
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( sch );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
        m_reporter->Report( _( "Warning: duplicate sheet names.\n" ), RPT_SEVERITY_WARNING );

    // Build our data model
    SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL dataModel( referenceList );
    dataModel.SetCurrentVariant( currentVariant );

    // Mandatory fields first
    for( FIELD_T fieldId : MANDATORY_FIELDS )
        dataModel.AddColumn( GetDefaultFieldName( fieldId, UNTRANSLATED ),
                             GetDefaultFieldName( fieldId, TRANSLATED ), false );

    // Generated/virtual fields (e.g. ${QUANTITY}, ${ITEM_NUMBER}) present only in the fields table
    dataModel.AddColumn( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE,
                         GetGeneratedFieldDisplayName( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE ),
                         false );
    dataModel.AddColumn( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE,
                         GetGeneratedFieldDisplayName( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE ),
                         false );

    // Attribute fields (boolean flags on symbols)
    dataModel.AddColumn( wxS( "${DNP}" ), GetGeneratedFieldDisplayName( wxS( "${DNP}" ) ), false );
    dataModel.AddColumn( wxS( "${EXCLUDE_FROM_BOM}" ), GetGeneratedFieldDisplayName( wxS( "${EXCLUDE_FROM_BOM}" ) ),
                         false );
    dataModel.AddColumn( wxS( "${EXCLUDE_FROM_BOARD}" ), GetGeneratedFieldDisplayName( wxS( "${EXCLUDE_FROM_BOARD}" ) ),
                         false );
    dataModel.AddColumn( wxS( "${EXCLUDE_FROM_SIM}" ), GetGeneratedFieldDisplayName( wxS( "${EXCLUDE_FROM_SIM}" ) ),
                         false );

    // User field names in symbols second
    std::set<wxString> userFieldNames;

    for( size_t i = 0; i < referenceList.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = referenceList[i].GetSymbol();

        for( SCH_FIELD& field : symbol->GetFields() )
        {
            if( !field.IsMandatory() && !field.IsPrivate() )
                userFieldNames.insert( field.GetName() );
        }
    }

    for( const wxString& fieldName : userFieldNames )
        dataModel.AddColumn( fieldName, GetGeneratedFieldDisplayName( fieldName ), true );

    // Add any templateFieldNames which aren't already present in the userFieldNames
    for( const TEMPLATE_FIELDNAME& templateFieldname :
         sch->Project().GetProjectFile().m_TemplateFieldNames.GetResolvedTemplateFieldNames() )
    {
        if( userFieldNames.count( templateFieldname.m_Name ) == 0 )
        {
            dataModel.AddColumn( templateFieldname.m_Name, GetGeneratedFieldDisplayName( templateFieldname.m_Name ),
                                 false );
        }
    }

    BOM_PRESET preset;

    // Load a preset if one is specified
    if( !aBomJob->m_bomPresetName.IsEmpty() )
    {
        // Find the preset
        const BOM_PRESET* schPreset = nullptr;

        for( const BOM_PRESET& p : BOM_PRESET::BuiltInPresets() )
        {
            if( p.name == aBomJob->m_bomPresetName )
            {
                schPreset = &p;
                break;
            }
        }

        for( const BOM_PRESET& p : sch->Settings().m_BomPresets )
        {
            if( p.name == aBomJob->m_bomPresetName )
            {
                schPreset = &p;
                break;
            }
        }

        if( !schPreset )
        {
            m_reporter->Report(
                    wxString::Format( _( "BOM preset '%s' not found" ) + wxS( "\n" ), aBomJob->m_bomPresetName ),
                    RPT_SEVERITY_ERROR );

            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        preset = *schPreset;
    }
    else
    {
        // Normalize field names so that bare generated-field tokens (e.g. "QUANTITY") are
        // accepted alongside the canonical "${QUANTITY}" form. Shell expansion of ${VAR}
        // inside double quotes silently produces an empty string, so this also guards against
        // that common CLI pitfall.
        auto normalizeFieldName = [&dataModel]( const wxString& aName ) -> wxString
        {
            if( aName.IsEmpty() )
                return wxEmptyString;

            if( IsGeneratedField( aName ) )
                return aName;

            wxString wrapped = wxS( "${" ) + aName + wxS( "}" );

            if( IsGeneratedField( wrapped ) && dataModel.GetFieldNameCol( wrapped ) != -1 )
                return wrapped;

            return aName;
        };

        size_t i = 0;

        for( const wxString& rawFieldName : aBomJob->m_fieldsOrdered )
        {
            wxString fieldName = normalizeFieldName( rawFieldName );

            if( fieldName.IsEmpty() )
            {
                i++;
                continue;
            }

            // Handle wildcard. We allow the wildcard anywhere in the list, but it needs to respect
            // fields that come before and after the wildcard.
            if( fieldName == wxS( "*" ) )
            {
                for( const BOM_FIELD& modelField : dataModel.GetFieldsOrdered() )
                {
                    struct BOM_FIELD field;

                    field.name = modelField.name;
                    field.show = true;
                    field.groupBy = false;
                    field.label = field.name;

                    bool fieldAlreadyPresent = false;

                    for( BOM_FIELD& presetField : preset.fieldsOrdered )
                    {
                        if( presetField.name == field.name )
                        {
                            fieldAlreadyPresent = true;
                            break;
                        }
                    }

                    bool fieldLaterInList = false;

                    for( const wxString& fieldInList : aBomJob->m_fieldsOrdered )
                    {
                        if( normalizeFieldName( fieldInList ) == field.name )
                        {
                            fieldLaterInList = true;
                            break;
                        }
                    }

                    if( !fieldAlreadyPresent && !fieldLaterInList )
                        preset.fieldsOrdered.emplace_back( field );
                }

                continue;
            }

            struct BOM_FIELD field;

            field.name = fieldName;
            field.show = !fieldName.StartsWith( wxT( "__" ), &field.name );
            field.name = normalizeFieldName( field.name );

            if( ( aBomJob->m_fieldsLabels.size() > i ) && !aBomJob->m_fieldsLabels[i].IsEmpty() )
                field.label = aBomJob->m_fieldsLabels[i];
            else if( IsGeneratedField( field.name ) )
                field.label = GetGeneratedFieldDisplayName( field.name );
            else
                field.label = field.name;

            preset.fieldsOrdered.emplace_back( field );
            i++;
        }

        for( const wxString& rawFieldName : aBomJob->m_fieldsGroupBy )
        {
            wxString fieldName = rawFieldName;
            fieldName.StartsWith( wxT( "__" ), &fieldName );
            fieldName = normalizeFieldName( fieldName );

            if( fieldName.IsEmpty() )
                continue;

            bool fieldAlreadyPresent = false;

            for( BOM_FIELD& field : preset.fieldsOrdered )
            {
                if( field.name == fieldName )
                {
                    field.groupBy = true;
                    fieldAlreadyPresent = true;
                    break;
                }
            }

            if( !fieldAlreadyPresent )
                preset.fieldsOrdered.push_back( { fieldName, GetGeneratedFieldDisplayName( fieldName ), false, true } );
        }

        preset.sortAsc = aBomJob->m_sortAsc;
        preset.sortField = normalizeFieldName( aBomJob->m_sortField );
        preset.filterString = aBomJob->m_filterString;
        preset.filterScope = aBomJob->m_filterScope;
        preset.groupSymbols = aBomJob->m_groupSymbols;
        preset.excludeDNP = aBomJob->m_excludeDNP;
    }

    BOM_FMT_PRESET fmt;

    // Load a format preset if one is specified
    if( !aBomJob->m_bomFmtPresetName.IsEmpty() )
    {
        std::optional<BOM_FMT_PRESET> schFmtPreset;

        for( const BOM_FMT_PRESET& p : BOM_FMT_PRESET::BuiltInPresets() )
        {
            if( p.name == aBomJob->m_bomFmtPresetName )
            {
                schFmtPreset = p;
                break;
            }
        }

        for( const BOM_FMT_PRESET& p : sch->Settings().m_BomFmtPresets )
        {
            if( p.name == aBomJob->m_bomFmtPresetName )
            {
                schFmtPreset = p;
                break;
            }
        }

        if( !schFmtPreset )
        {
            m_reporter->Report( wxString::Format( _( "BOM format preset '%s' not found" ) + wxS( "\n" ),
                                                  aBomJob->m_bomFmtPresetName ),
                                RPT_SEVERITY_ERROR );

            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        fmt = *schFmtPreset;
    }
    else
    {
        fmt.fieldDelimiter = aBomJob->m_fieldDelimiter;
        fmt.stringDelimiter = aBomJob->m_stringDelimiter;
        fmt.refDelimiter = aBomJob->m_refDelimiter;
        fmt.refRangeDelimiter = aBomJob->m_refRangeDelimiter;
        fmt.keepTabs = aBomJob->m_keepTabs;
        fmt.keepLineBreaks = aBomJob->m_keepLineBreaks;
        fmt.includeByteOrderMark = aBomJob->m_includeByteOrderMark;
    }

    if( aBomJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( FILEEXT::CsvFileExtension );

        aBomJob->SetConfiguredOutputPath( fn.GetFullName() );
    }

    wxString configuredPath = aBomJob->GetConfiguredOutputPath();
    bool     hasVariantPlaceholder = configuredPath.Contains( wxS( "${VARIANT}" ) );

    // Determine which variants to process
    std::vector<wxString> variantsToProcess;

    if( aBomJob->m_variantNames.size() > 1 && hasVariantPlaceholder )
    {
        variantsToProcess = aBomJob->m_variantNames;
    }
    else
    {
        variantsToProcess.push_back( currentVariant );
    }

    for( const wxString& variantName : variantsToProcess )
    {
        std::vector<wxString> singleVariant = { variantName };
        dataModel.SetVariantNames( singleVariant );
        dataModel.SetCurrentVariant( variantName );
        dataModel.UpdateReferences( dataModel.GetReferenceList() );
        dataModel.ApplyBomPreset( preset );

        wxString outPath;

        if( hasVariantPlaceholder )
        {
            wxString variantPath = configuredPath;
            variantPath.Replace( wxS( "${VARIANT}" ), variantName );
            aBomJob->SetConfiguredOutputPath( variantPath );
            outPath = aBomJob->GetFullOutputPath( &sch->Project() );
            aBomJob->SetConfiguredOutputPath( configuredPath );
        }
        else
        {
            outPath = aBomJob->GetFullOutputPath( &sch->Project() );
        }

        if( !PATHS::EnsurePathExists( outPath, true ) )
        {
            m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }

        wxFile f;

        if( !f.Open( outPath, wxFile::write ) )
        {
            m_reporter->Report( wxString::Format( _( "Unable to open destination '%s'" ), outPath ),
                                RPT_SEVERITY_ERROR );

            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        bool res = f.Write( dataModel.Export( fmt ) );

        if( !res )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        aJob->AddOutput( outPath );

        m_reporter->Report( wxString::Format( _( "Wrote bill of materials to '%s'." ), outPath ), RPT_SEVERITY_ACTION );
    }

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportPythonBom( JOB* aJob )
{
    JOB_EXPORT_SCH_PYTHONBOM* aNetJob = dynamic_cast<JOB_EXPORT_SCH_PYTHONBOM*>( aJob );

    wxCHECK( aNetJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    SCHEMATIC* sch = getSchematic( aNetJob->m_filename );

    if( !sch )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( sch->RootScreen()->GetTitleBlock() );
    sch->Project().ApplyTextVars( aJob->GetVarOverrides() );

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->Hierarchy().GetSymbols( referenceList, SYMBOL_FILTER_NON_POWER );

    if( referenceList.GetCount() > 0 )
    {
        if( referenceList.CheckAnnotation(
                    []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* )
                    {
                        // We're only interested in the end result -- either errors or not
                    } )
            > 0 )
        {
            m_reporter->Report( _( "Warning: schematic has annotation errors, please use the "
                                   "schematic editor to fix them\n" ),
                                RPT_SEVERITY_WARNING );
        }
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( sch );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
        m_reporter->Report( _( "Warning: duplicate sheet names.\n" ), RPT_SEVERITY_WARNING );

    std::unique_ptr<NETLIST_EXPORTER_XML> xmlNetlist = std::make_unique<NETLIST_EXPORTER_XML>( sch, m_kiway );

    if( aNetJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() + "-bom" );
        fn.SetExt( FILEEXT::XmlFileExtension );

        aNetJob->SetConfiguredOutputPath( fn.GetFullName() );
    }

    wxString outPath = aNetJob->GetFullOutputPath( &sch->Project() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    bool res = xmlNetlist->WriteNetlist( outPath, GNL_OPT_BOM, *m_reporter );

    if( !res )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    aJob->AddOutput( outPath );

    m_reporter->Report( wxString::Format( _( "Wrote bill of materials to '%s'." ), outPath ), RPT_SEVERITY_ACTION );

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::doSymExportSvg( JOB_SYM_EXPORT_SVG* aSvgJob, SCH_RENDER_SETTINGS* aRenderSettings,
                                           LIB_SYMBOL* symbol )
{
    wxCHECK( symbol, CLI::EXIT_CODES::ERR_UNKNOWN );

    std::shared_ptr<LIB_SYMBOL> parent;
    LIB_SYMBOL*                 symbolToPlot = symbol;

    // if the symbol is an alias, then the draw items are stored in the root symbol
    if( symbol->IsDerived() )
    {
        parent = symbol->GetRootSymbol();

        wxCHECK( parent, CLI::EXIT_CODES::ERR_UNKNOWN );

        symbolToPlot = parent.get();
    }

    // iterate from unit 1, unit 0 would be "all units" which we don't want
    for( int unit = 1; unit < symbol->GetUnitCount() + 1; unit++ )
    {
        for( int bodyStyle = 1; bodyStyle <= symbol->GetBodyStyleCount(); ++bodyStyle )
        {
            wxString   filename;
            wxFileName fn;

            fn.SetPath( aSvgJob->m_outputDirectory );
            fn.SetExt( FILEEXT::SVGFileExtension );

            filename = symbol->GetName();

            for( wxChar c : wxFileName::GetForbiddenChars( wxPATH_DOS ) )
                filename.Replace( c, ' ' );

            // Even single units get a unit number in the filename. This simplifies the
            // handling of the files as they have a uniform pattern.
            // Also avoids aliasing 'sym', unit 2 and 'sym_unit2', unit 1 to the same file.
            filename += wxString::Format( "_unit%d", unit );

            if( symbol->HasDeMorganBodyStyles() )
            {
                if( bodyStyle == 2 )
                    filename += wxS( "_demorgan" );
            }
            else if( bodyStyle <= (int) symbol->GetBodyStyleNames().size() )
            {
                filename += wxS( "_" ) + symbol->GetBodyStyleNames()[bodyStyle - 1].Lower();
            }

            fn.SetName( filename );
            m_reporter->Report( wxString::Format( _( "Plotting symbol '%s' unit %d to '%s'\n" ), symbol->GetName(),
                                                  unit, fn.GetFullPath() ),
                                RPT_SEVERITY_ACTION );

            BOX2I symbolBB = GetSymbolPlotBBox( *symbol, unit, bodyStyle, aSvgJob->m_includeHiddenFields );

            if( !PlotSymbolToSVG( *symbolToPlot, *symbol, unit, bodyStyle, symbolBB, *aRenderSettings,
                                  aSvgJob->m_blackAndWhite, fn.GetFullPath(), m_reporter ) )
            {
                return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
            }
        }
    }

    if( m_reporter->HasMessageOfSeverity( RPT_SEVERITY_ERROR ) )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobSymExportSvg( JOB* aJob )
{
    JOB_SYM_EXPORT_SVG* svgJob = dynamic_cast<JOB_SYM_EXPORT_SVG*>( aJob );

    wxCHECK( svgJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    wxFileName fn( svgJob->m_libraryPath );
    fn.MakeAbsolute();

    // When the input is a single symbol file we restrict plotting to the symbols defined in
    // that file. Stays empty (no restriction) when the input is a whole library.
    wxString singleFileFilter;

    auto schLibrary = std::make_unique<SCH_IO_KICAD_SEXPR_LIB_CACHE>( fn.GetFullPath() );

    try
    {
        schLibrary->Load();
    }
    catch( ... )
    {
        // A single file holding a derived symbol whose parent is in a sibling file cannot load
        // alone. Retry against the enclosing directory, then plot only this file's symbols.
        bool recovered = false;

        if( !fn.IsDir() && wxDir::Exists( fn.GetPath() ) )
        {
            try
            {
                schLibrary = std::make_unique<SCH_IO_KICAD_SEXPR_LIB_CACHE>( fn.GetPath() );
                schLibrary->Load();
                singleFileFilter = fn.GetFullPath();
                recovered = true;
            }
            catch( ... )
            {
                // Fall through to the generic load error below.
            }
        }

        if( !recovered )
        {
            m_reporter->Report( _( "Unable to load library\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
    }

    if( m_progressReporter )
        m_progressReporter->KeepRefreshing();

    LIB_SYMBOL* symbol = nullptr;

    if( !svgJob->m_symbol.IsEmpty() )
    {
        // See if the selected symbol exists
        symbol = schLibrary->GetSymbol( svgJob->m_symbol );

        if( !symbol )
        {
            m_reporter->Report( _( "There is no symbol selected to save." ) + wxS( "\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }
    }

    if( !svgJob->m_outputDirectory.IsEmpty() && !wxDir::Exists( svgJob->m_outputDirectory ) )
    {
        if( !wxFileName::Mkdir( svgJob->m_outputDirectory ) )
        {
            m_reporter->Report( wxString::Format( _( "Unable to create output directory '%s'." ) + wxS( "\n" ),
                                                  svgJob->m_outputDirectory ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
    }

    SCH_RENDER_SETTINGS renderSettings;
    COLOR_SETTINGS*     cs = ::GetColorSettings( svgJob->m_colorTheme );
    renderSettings.LoadColors( cs );
    renderSettings.SetDefaultPenWidth( DEFAULT_LINE_WIDTH_MILS * schIUScale.IU_PER_MILS );
    renderSettings.m_ShowHiddenPins = svgJob->m_includeHiddenPins;
    renderSettings.m_ShowHiddenFields = svgJob->m_includeHiddenFields;

    int exitCode = CLI::EXIT_CODES::OK;

    if( symbol )
    {
        exitCode = doSymExportSvg( svgJob, &renderSettings, symbol );
    }
    else
    {
        // Just plot all the symbols we can
        const LIB_SYMBOL_MAP&               libSymMap = schLibrary->GetSymbolMap();
        const std::map<wxString, wxString>& sourceFiles = schLibrary->GetSymbolSourceFiles();
        const wxFileName                    filterFile( singleFileFilter );

        for( const auto& [name, libSymbol] : libSymMap )
        {
            // When a single file was requested, skip symbols that came from sibling files.
            if( !singleFileFilter.IsEmpty() )
            {
                auto srcIt = sourceFiles.find( name );

                if( srcIt == sourceFiles.end() || !wxFileName( srcIt->second ).SameAs( filterFile ) )
                    continue;
            }

            if( m_progressReporter )
            {
                m_progressReporter->AdvancePhase( wxString::Format( _( "Exporting %s" ), name ) );
                m_progressReporter->KeepRefreshing();
            }

            exitCode = doSymExportSvg( svgJob, &renderSettings, libSymbol );

            if( exitCode != CLI::EXIT_CODES::OK )
                break;
        }
    }

    return exitCode;
}


int EESCHEMA_JOBS_HANDLER::JobSymUpgrade( JOB* aJob )
{
    JOB_SYM_UPGRADE* upgradeJob = dynamic_cast<JOB_SYM_UPGRADE*>( aJob );

    wxCHECK( upgradeJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    wxFileName fn( upgradeJob->m_libraryPath );
    fn.MakeAbsolute();

    SCH_IO_MGR::SCH_FILE_T fileType = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

    if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
    {
        if( wxFile::Exists( upgradeJob->m_outputLibraryPath ) )
        {
            m_reporter->Report( _( "Output path must not conflict with existing path\n" ), RPT_SEVERITY_ERROR );

            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }
    else if( fileType != SCH_IO_MGR::SCH_KICAD )
    {
        m_reporter->Report( _( "Output path must be specified to convert legacy and non-KiCad libraries\n" ),
                            RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    if( fileType == SCH_IO_MGR::SCH_KICAD )
    {
        SCH_IO_KICAD_SEXPR_LIB_CACHE schLibrary( fn.GetFullPath() );

        try
        {
            schLibrary.Load();
        }
        catch( ... )
        {
            m_reporter->Report( _( "Unable to load library\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        if( m_progressReporter )
            m_progressReporter->KeepRefreshing();

        bool shouldSave =
                upgradeJob->m_force || schLibrary.GetFileFormatVersionAtLoad() < SEXPR_SYMBOL_LIB_FILE_VERSION;

        if( shouldSave )
        {
            m_reporter->Report( _( "Saving symbol library in updated format\n" ), RPT_SEVERITY_ACTION );

            try
            {
                if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
                    schLibrary.SetFileName( upgradeJob->m_outputLibraryPath );

                schLibrary.SetModified();
                schLibrary.Save();
            }
            catch( ... )
            {
                m_reporter->Report( ( "Unable to save library\n" ), RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }
        }
        else
        {
            m_reporter->Report( _( "Symbol library was not updated\n" ), RPT_SEVERITY_ERROR );
        }
    }
    else
    {
        if( !SCH_IO_MGR::ConvertLibrary( nullptr, fn.GetAbsolutePath(), upgradeJob->m_outputLibraryPath, m_reporter ) )
        {
            m_reporter->Report( ( "Unable to convert library\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
    }

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobSchErc( JOB* aJob )
{
    JOB_SCH_ERC* ercJob = dynamic_cast<JOB_SCH_ERC*>( aJob );

    wxCHECK( ercJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    SCHEMATIC* sch = getSchematic( ercJob->m_filename );

    if( !sch )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( sch->RootScreen()->GetTitleBlock() );
    sch->Project().ApplyTextVars( aJob->GetVarOverrides() );

    if( ercJob->GetConfiguredOutputPath().IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() + wxS( "-erc" ) );

        if( ercJob->m_format == JOB_SCH_ERC::OUTPUT_FORMAT::JSON )
            fn.SetExt( FILEEXT::JsonFileExtension );
        else
            fn.SetExt( FILEEXT::ReportFileExtension );

        // Use a transient working path so an empty configured output filename isn't persisted
        // back into the jobset file. Mirrors the PCB DRC handler.
        ercJob->SetWorkingOutputPath( fn.GetFullName() );
    }

    wxString outPath = ercJob->GetFullOutputPath( &sch->Project() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        m_reporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    EDA_UNITS units;

    switch( ercJob->m_units )
    {
    case JOB_SCH_ERC::UNITS::INCH: units = EDA_UNITS::INCH; break;
    case JOB_SCH_ERC::UNITS::MILS: units = EDA_UNITS::MILS; break;
    case JOB_SCH_ERC::UNITS::MM: units = EDA_UNITS::MM; break;
    default: units = EDA_UNITS::MM; break;
    }

    std::shared_ptr<SHEETLIST_ERC_ITEMS_PROVIDER> markersProvider =
            std::make_shared<SHEETLIST_ERC_ITEMS_PROVIDER>( sch );

    // Running ERC requires libraries be loaded, so make sure they have been
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &sch->Project() );
    adapter->AsyncLoad();
    adapter->BlockUntilLoaded();

    ERC_TESTER ercTester( sch );

    std::unique_ptr<DS_PROXY_VIEW_ITEM> drawingSheet( getDrawingSheetProxyView( sch ) );
    SCH_EDIT_FRAME* editFrame = nullptr;

    if( Pgm().IsGUI() )
    {
        editFrame = static_cast<SCH_EDIT_FRAME*>( m_kiway->Player( FRAME_SCH, false ) );

        if( editFrame && &editFrame->Schematic() != sch )
            editFrame = nullptr;
    }

    if( editFrame )
        editFrame->ClearErcMarkers();

    ercTester.RunTests( drawingSheet.get(), editFrame, m_kiway->KiFACE( KIWAY::FACE_CVPCB ), &sch->Project(),
                        m_progressReporter );

    if( editFrame )
        editFrame->RefreshErcMarkers();

    markersProvider->SetSeverities( ercJob->m_severity );

    m_reporter->Report( wxString::Format( _( "Found %d violations\n" ), markersProvider->GetCount() ),
                        RPT_SEVERITY_INFO );

    ERC_REPORT reportWriter( sch, units, markersProvider );

    bool wroteReport = false;

    if( ercJob->m_format == JOB_SCH_ERC::OUTPUT_FORMAT::JSON )
        wroteReport = reportWriter.WriteJsonReport( outPath );
    else
        wroteReport = reportWriter.WriteTextReport( outPath );

    if( !wroteReport )
    {
        m_reporter->Report( wxString::Format( _( "Unable to save ERC report to %s\n" ), outPath ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    m_reporter->Report( wxString::Format( _( "Saved ERC Report to %s\n" ), outPath ), RPT_SEVERITY_ACTION );

    if( ercJob->m_exitCodeViolations )
    {
        if( markersProvider->GetCount() > 0 )
            return CLI::EXIT_CODES::ERR_RC_VIOLATIONS;
    }

    return CLI::EXIT_CODES::SUCCESS;
}


int EESCHEMA_JOBS_HANDLER::JobUpgrade( JOB* aJob )
{
    JOB_SCH_UPGRADE* aUpgradeJob = dynamic_cast<JOB_SCH_UPGRADE*>( aJob );

    if( aUpgradeJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = getSchematic( aUpgradeJob->m_filename, false );

    if( !sch )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    bool shouldSave = aUpgradeJob->m_force;

    if( sch->RootScreen()->GetFileFormatVersionAtLoad() < SEXPR_SCHEMATIC_FILE_VERSION )
        shouldSave = true;

    if( !shouldSave )
    {
        m_reporter->Report( _( "Schematic file was not updated\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::SUCCESS;
    }

    // needs an absolute path
    wxFileName schPath( aUpgradeJob->m_filename );
    schPath.MakeAbsolute();
    const wxString schFullPath = schPath.GetFullPath();

    try
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        SCH_SHEET*          loadedSheet = pi->LoadSchematicFile( schFullPath, sch );
        pi->SaveSchematicFile( schFullPath, loadedSheet, sch );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg =
                wxString::Format( _( "Error saving schematic file '%s'.\n%s" ), schFullPath, ioe.What().GetData() );
        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    m_reporter->Report( _( "Successfully saved schematic file using the latest format\n" ), RPT_SEVERITY_INFO );

    return CLI::EXIT_CODES::SUCCESS;
}


// Unloads a project that was only opened for the duration of a job.
struct TRANSIENT_PROJECT_GUARD
{
    SETTINGS_MANAGER& m_mgr;
    PROJECT*          m_project;
    bool              m_active;

    ~TRANSIENT_PROJECT_GUARD()
    {
        if( m_active )
            m_mgr.UnloadProject( m_project, false );
    }
};


// Classify library symbols so their losses appear in the report.
static void classifySymbolLibraries( const wxString& aDir, const DOWNGRADE_TARGET& aTarget,
                                     COMPATIBILITY_REPORT& aReport, bool aDropInsteadOfApproximate )
{
    wxArrayString files;
    wxDir::GetAllFiles( aDir, &files, wxT( "*.kicad_sym" ) );

    for( const wxString& file : files )
    {
        if( IsDowngradeWorkingArtifact( file ) )
            continue;

        try
        {
            SCH_IO_KICAD_SEXPR       pi;
            std::vector<LIB_SYMBOL*> symbols;
            pi.EnumerateSymbolLib( symbols, file );

            for( const LIB_SYMBOL* symbol : symbols )
            {
                aReport.Merge( ClassifyLibSymbolForDowngrade( symbol, aTarget, aDropInsteadOfApproximate ) );
            }
        }
        catch( const IO_ERROR& )
        {
            // A library that fails to load is caught later by the downgrade pass itself.
        }
    }
}


// Downgrade every .kicad_sym under the folder in place. Returns the first file the target could
// not open, or empty when all are safe.
static wxString downgradeSymbolLibraries( const wxString& aDir, const DOWNGRADE_TARGET& aTarget,
                                          bool                        aDropInsteadOfApproximate,
                                          DOWNGRADE_FILE_TRANSACTION* aTransaction = nullptr,
                                          wxString*                   aRecoveryError = nullptr )
{
    auto convert = [&]( const wxString& aFile, const wxString& aTmp ) -> DOWNGRADE_FILE_RESULT
    {
        SCH_IO_KICAD_SEXPR       pi;
        std::vector<LIB_SYMBOL*> symbols;
        pi.EnumerateSymbolLib( symbols, aFile );

        // The format wants parents before the symbols that inherit from them, so sort by depth.
        std::stable_sort( symbols.begin(), symbols.end(),
                          []( const LIB_SYMBOL* a, const LIB_SYMBOL* b )
                          {
                              return a->GetInheritanceDepth() < b->GetInheritanceDepth();
                          } );

        for( LIB_SYMBOL* symbol : symbols )
            DowngradeLibSymbolInPlace( symbol, aTarget, aDropInsteadOfApproximate );

        SaveSymbolLibraryForTarget( symbols, aTmp, aTarget );
        return DOWNGRADE_FILE_RESULT::CONVERTED;
    };

    return DowngradeLibraryFilesInPlace(
            aDir, wxT( "*.kicad_sym" ), aTarget.m_symLibVersion,
            [&]( const wxString& aText )
            {
                return FindUnsupportedSchToken( aText, aTarget );
            },
            convert, aTransaction, aRecoveryError );
}


int EESCHEMA_JOBS_HANDLER::JobDowngrade( JOB* aJob )
{
    JOB_SCH_DOWNGRADE* job = dynamic_cast<JOB_SCH_DOWNGRADE*>( aJob );

    if( job == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    const DOWNGRADE_TARGET* target =
            job->m_target.IsEmpty() ? &GetDowngradeTargets().front() : FindDowngradeTarget( job->m_target );

    if( target == nullptr )
    {
        m_reporter->Report( wxString::Format( _( "Unknown target version '%s'. Valid targets: %s\n" ), job->m_target,
                                              DowngradeTargetNames() ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    if( !job->m_libraryDir.IsEmpty() && !wxDirExists( job->m_libraryDir ) )
    {
        m_reporter->Report( wxString::Format( _( "The library folder '%s' does not exist.\n" ), job->m_libraryDir ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    DOWNGRADE_OUTPUT_DIRECTORIES outputDirectories;
    wxString                     libraryWorkDir = job->m_libraryDir;
    wxString                     libraryCopyDir;
    wxString                     outputError;

    if( !job->m_inPlace && !job->m_dryRun )
    {
        wxFileName output = wxFileName::DirName( job->m_outputDir );

        if( job->m_outputDir.IsEmpty() || !output.MakeAbsolute()
            || !ValidateFreshDowngradeOutput( output.GetPath(), outputError ) )
        {
            if( outputError.IsEmpty() )
                outputError = _( "An output folder is required. Use --output." );

            m_reporter->Report( outputError + wxS( "\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }

        job->m_outputDir = output.GetPath();
    }

    if( !job->m_libraryDir.IsEmpty() )
    {
        if( !ValidateDowngradeLibrarySource( job->m_libraryDir, outputError ) )
        {
            m_reporter->Report( outputError + wxS( "\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }

        if( !job->m_inPlace && !job->m_dryRun )
        {
            libraryCopyDir = DowngradeLibraryCopyPath( job->m_libraryDir, job->m_outputDir );

            if( !ValidateDowngradeLibraryCopy( job->m_libraryDir, libraryCopyDir, outputError ) )
            {
                m_reporter->Report( outputError + wxS( "\n" ), RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_ARGS;
            }
        }
    }

    auto prepareOutput = [&]() -> bool
    {
        if( job->m_inPlace )
            return true;

        if( !outputDirectories.Create( job->m_outputDir, true ) )
        {
            m_reporter->Report( _( "Could not create the schematic output folder.\n" ), RPT_SEVERITY_ERROR );
            return false;
        }

        if( !libraryCopyDir.IsEmpty() )
        {
            if( !CopyDowngradeLibraryDirectory( job->m_libraryDir, libraryCopyDir, outputDirectories, outputError ) )
            {
                m_reporter->Report( outputError + wxS( "\n" ), RPT_SEVERITY_ERROR );
                return false;
            }

            libraryWorkDir = libraryCopyDir;
        }

        return true;
    };

    auto downgradeLibraries = [&]( DOWNGRADE_FILE_TRANSACTION* aTransaction = nullptr ) -> int
    {
        if( job->m_dryRun || job->m_libraryDir.IsEmpty() )
            return CLI::EXIT_CODES::SUCCESS;

        try
        {
            wxString recoveryError;
            wxString badLib = downgradeSymbolLibraries( libraryWorkDir, *target, job->m_dropInsteadOfApproximate,
                                                        aTransaction, &recoveryError );

            if( !badLib.IsEmpty() )
            {
                m_reporter->Report( wxString::Format( _( "Cannot export: symbol library '%s' uses a feature %s "
                                                         "cannot open.\n" ),
                                                      badLib, target->m_name ),
                                    RPT_SEVERITY_ERROR );

                if( !recoveryError.IsEmpty() )
                    m_reporter->Report( recoveryError, RPT_SEVERITY_ERROR );

                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }
        }
        catch( const IO_ERROR& ioe )
        {
            m_reporter->Report(
                    wxString::Format( _( "Error downgrading symbol libraries.\n%s" ), ioe.What().GetData() ),
                    RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        return CLI::EXIT_CODES::SUCCESS;
    };

    // No schematic to process. Do the libraries and stop.
    if( job->m_filename.IsEmpty() )
    {
        if( !job->m_variant.IsEmpty() )
        {
            m_reporter->Report( _( "Variants do not apply to a library downgrade.\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }

        if( !job->m_libraryDir.IsEmpty() )
        {
            COMPATIBILITY_REPORT report;
            classifySymbolLibraries( job->m_libraryDir, *target, report, job->m_dropInsteadOfApproximate );
            job->m_report = report;
            job->m_report.Print( m_reporter );

            if( !job->m_dryRun && report.IsBlocked() )
            {
                m_reporter->Report( _( "Cannot export: the libraries use features the target cannot represent.\n" ),
                                    RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }

            if( !job->m_dryRun && report.IsLossy() && !job->m_force )
            {
                m_reporter->Report( _( "Export would lose data. Re-run with --force to export anyway.\n" ),
                                    RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }
        }

        if( job->m_dryRun )
            return CLI::EXIT_CODES::SUCCESS;

        if( !prepareOutput() )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        int result = downgradeLibraries();

        if( result == CLI::EXIT_CODES::SUCCESS )
            outputDirectories.Commit();

        return result;
    }

    // The project and schematic loaders require absolute paths.
    wxFileName schInputFn( job->m_filename );
    schInputFn.MakeAbsolute();
    job->m_filename = schInputFn.GetFullPath();

    // Load a private copy, never the live editor schematic. A transient project keeps it apart
    // from anything open in the GUI.
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    wxFileName projectFn( job->m_filename );
    projectFn.SetExt( FILEEXT::ProjectFileExtension );

    PROJECT* projectPtr = mgr.GetProject( projectFn.GetFullPath() );
    bool     createdTransientProject = false;

    if( !projectPtr )
    {
        mgr.LoadProject( projectFn.GetFullPath(), false );
        projectPtr = mgr.GetProject( projectFn.GetFullPath() );
        createdTransientProject = ( projectPtr != nullptr );
    }

    if( !projectPtr )
    {
        m_reporter->Report( _( "Could not establish a project for the schematic\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    TRANSIENT_PROJECT_GUARD transientProjectGuard{ mgr, projectPtr, createdTransientProject };

    LOCALE_IO dummy;

    // Binding a SCHEMATIC to a project reallocates that project's ERC and schematic settings. When
    // we borrowed a live project, snapshot the pointers so we can put them back. This relies on
    // ~SCHEMATIC not calling SetProject.
    PROJECT_FILE& projectFile = projectPtr->GetProjectFile();

    // The settings loader repairs missing root filenames as if this were a renamed
    // template. That is unsafe during export: check the stored list before using the
    // repaired in-memory list, or a missing sibling can be replaced by the main sheet.
    if( auto storedRoots = projectFile.GetJson( "schematic.top_level_sheets" ); storedRoots && storedRoots->is_array() )
    {
        for( const auto& info : *storedRoots )
        {
            if( !info.contains( "filename" ) || !info["filename"].is_string() )
                continue;

            const wxString filename = wxString::FromUTF8( info["filename"].get<std::string>() );
            wxFileName     rootFile( filename );
            rootFile.MakeAbsolute( projectPtr->GetProjectPath() );

            if( filename.IsEmpty() || !rootFile.FileExists() )
            {
                m_reporter->Report( wxString::Format( _( "Cannot export: top-level schematic '%s' is missing.\n" ),
                                                      rootFile.GetFullPath() ),
                                    RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
            }
        }
    }

    ERC_SETTINGS*       savedErc = createdTransientProject ? nullptr : projectFile.m_ErcSettings;
    SCHEMATIC_SETTINGS* savedSch = createdTransientProject ? nullptr : projectFile.m_SchematicSettings;

    std::unique_ptr<SCHEMATIC> owned = std::make_unique<SCHEMATIC>( projectPtr );
    owned->CreateDefaultScreens();
    SCHEMATIC* sch = owned.get();

    struct SCH_PROJECT_GUARD
    {
        SCHEMATIC*          m_sch;
        PROJECT*            m_project;
        bool                m_created;
        ERC_SETTINGS*       m_savedErc;
        SCHEMATIC_SETTINGS* m_savedSch;

        ~SCH_PROJECT_GUARD()
        {
            if( m_created )
            {
                m_sch->SetProject( nullptr );
            }
            else
            {
                // Drop the settings this schematic made and put the live project's own back.
                PROJECT_FILE& pf = m_project->GetProjectFile();
                delete pf.m_ErcSettings;
                delete pf.m_SchematicSettings;
                pf.m_ErcSettings = m_savedErc;
                pf.m_SchematicSettings = m_savedSch;
            }
        }
    } schProjectGuard{ sch, projectPtr, createdTransientProject, savedErc, savedSch };

    try
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        SCH_SHEET*          root = pi->LoadSchematicFile( job->m_filename, sch );

        if( !root )
        {
            m_reporter->Report( _( "Failed to load schematic\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        // The loader collects sub-sheet errors instead of throwing. An incomplete hierarchy must
        // not be exported, or missing sheets would be written out empty.
        if( !pi->GetError().IsEmpty() )
        {
            m_reporter->Report( wxString::Format( _( "Cannot export: the schematic did not load "
                                                     "completely.\n%s" ),
                                                  pi->GetError() ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }

        // The loader does not install the hierarchy. Without this the sheet walk below sees only
        // the empty default screens and the export silently does nothing.
        std::vector<SCH_SHEET*> topLevelSheets = sch->GetTopLevelSheets();
        bool rootIsTopLevel = std::find( topLevelSheets.begin(), topLevelSheets.end(), root ) != topLevelSheets.end();
        bool rootIsVirtualRoot = root == &sch->Root() || root->IsVirtualRootSheet();
        bool changed = false;

        if( !rootIsTopLevel && !rootIsVirtualRoot )
        {
            topLevelSheets = { root };
            changed = true;
        }

        // A flat hierarchy records its other top-level sheets only in the project file. They
        // must load too, or classification never sees them and their trees never convert.
        if( !rootIsVirtualRoot )
        {
            for( const TOP_LEVEL_SHEET_INFO& info : projectFile.GetTopLevelSheets() )
            {
                wxFileName fn( projectPtr->GetProjectPath(), info.filename );
                fn.MakeAbsolute();

                if( fn.GetFullPath() == job->m_filename )
                    continue;

                SCH_SHEET* sibling = pi->LoadSchematicFile( fn.GetFullPath(), sch );

                if( !sibling || !pi->GetError().IsEmpty() )
                {
                    m_reporter->Report( wxString::Format( _( "Cannot export: the schematic did not load "
                                                             "completely.\n%s" ),
                                                          pi->GetError() ),
                                        RPT_SEVERITY_ERROR );
                    return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
                }

                if( info.uuid != niluuid )
                    const_cast<KIID&>( sibling->m_Uuid ) = info.uuid;

                sibling->SetName( info.name );
                topLevelSheets.push_back( sibling );
                changed = true;
            }
        }

        if( changed )
            sch->SetTopLevelSheets( topLevelSheets );

        sch->LoadVariants();
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Error loading schematic: %s\n" ), ioe.What() ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // A screen can be shared by several sheets, so gather each one once.
    std::set<SCH_SCREEN*>                           seen;
    std::vector<std::pair<SCH_SHEET*, SCH_SCREEN*>> screens;

    for( const SCH_SHEET_PATH& path : sch->BuildUnorderedSheetList() )
    {
        SCH_SHEET*  sheet = path.Last();
        SCH_SCREEN* screen = sheet ? sheet->GetScreen() : nullptr;

        if( screen && seen.insert( screen ).second )
            screens.emplace_back( sheet, screen );
    }

    COMPATIBILITY_REPORT report;

    // Variants (20250922) are schematic-wide, so report them once for the whole schematic.
    int variantCount = static_cast<int>( sch->GetVariantNames().size() );

    if( target->m_schVersion < SCH_VER_VARIANTS && variantCount > 0 )
    {
        job->m_foundVariants.assign( sch->GetVariantNames().begin(), sch->GetVariantNames().end() );
    }

    if( !job->m_variant.IsEmpty() )
    {
        if( target->m_schVersion >= SCH_VER_VARIANTS )
        {
            m_reporter->Report( _( "The target supports variants, so there is nothing to flatten.\n" ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }

        if( !sch->GetVariantNames().count( job->m_variant ) )
        {
            m_reporter->Report( wxString::Format( _( "The schematic has no variant named '%s'.\n" ), job->m_variant ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }

        wxString conflict;

        if( !FlattenSchematicVariant( *sch, job->m_variant, &conflict ) )
        {
            m_reporter->Report(
                    wxString::Format( _( "Cannot export selected variant '%s': %s" ), job->m_variant, conflict )
                            + wxS( "\n" ),
                    RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
    }

    for( const auto& [sheet, screen] : screens )
    {
        report.Merge( ClassifyScreenForDowngrade( screen, *target, job->m_dropInsteadOfApproximate ) );
    }

    if( target->m_schVersion < SCH_VER_VARIANTS && variantCount > 0 )
    {
        report.Add( DOWNGRADE_BUCKET::DROP, _( "Schematic variants" ), _( "The variant registry is removed." ),
                    variantCount );
    }

    report.Merge( ClassifySchematicStructureForDowngrade( *sch, *target ) );

    // Net chains are connectivity-derived and a plain reload drops them. Detect them in the source
    // so the user is warned.
    if( target->m_schVersion < SCH_VER_NET_CHAINS )
    {
        wxString rootContent;
        wxFFile  rootFile( job->m_filename, wxT( "rb" ) );

        if( rootFile.IsOpened() && rootFile.ReadAll( &rootContent ) && rootContent.Contains( wxT( "(net_chain" ) ) )
        {
            report.Add( DOWNGRADE_BUCKET::DROP, _( "Net chains" ),
                        _( "No equivalent in the target. Net chain grouping is removed." ), 1 );
        }
    }

    // Library losses belong in the report the user approves too.
    if( !job->m_libraryDir.IsEmpty() )
        classifySymbolLibraries( job->m_libraryDir, *target, report, job->m_dropInsteadOfApproximate );

    job->m_report = report;

    report.Print( m_reporter );

    if( job->m_dryRun )
        return CLI::EXIT_CODES::SUCCESS;

    if( report.IsBlocked() )
    {
        m_reporter->Report( _( "Cannot export: the schematic uses features the target cannot represent.\n" ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( report.IsLossy() && !job->m_force )
    {
        m_reporter->Report( _( "Export would lose data. Re-run with --force to export anyway.\n" ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( !libraryCopyDir.IsEmpty() )
    {
        wxFileName base( job->m_filename );
        base.MakeAbsolute();
        const wxString        rootPrefix = wxFileName::DirName( base.GetPath() ).GetPathWithSep();
        std::vector<wxString> destinations;

        for( const auto& [sheet, screen] : screens )
        {
            wxFileName source( screen->GetFileName() );

            if( !source.IsAbsolute() )
                source.MakeAbsolute( base.GetPath() );

            source.Normalize( wxPATH_NORM_DOTS );
            const wxString sourcePath = source.GetFullPath();
            const bool     contained = wxFileName::IsCaseSensitive() ? sourcePath.StartsWith( rootPrefix )
                                                                     : sourcePath.Lower().StartsWith( rootPrefix.Lower() );

            if( !contained )
            {
                m_reporter->Report(
                        wxString::Format( _( "Cannot export: sheet '%s' lies outside the project folder.\n" ),
                                          sourcePath ),
                        RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }

            destinations.push_back( job->m_outputDir + wxFileName::GetPathSeparator()
                                    + sourcePath.Mid( rootPrefix.length() ) );
        }

        wxFileName projectOutput = projectFn;
        projectOutput.SetPath( job->m_outputDir );
        destinations.push_back( projectOutput.GetFullPath() );
        std::error_code error;
        wxString        library =
                DowngradeWxPath( std::filesystem::weakly_canonical( DowngradeNativePath( libraryCopyDir ), error ) );

        if( error )
        {
            m_reporter->Report( _( "The library output path is not accessible.\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }

        for( const wxString& destination : destinations )
        {
            wxString path =
                    DowngradeWxPath( std::filesystem::weakly_canonical( DowngradeNativePath( destination ), error ) );

            if( error )
            {
                m_reporter->Report( _( "The schematic output path is not accessible.\n" ), RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_ARGS;
            }

            wxString libraryPath = library;

            if( !wxFileName::IsCaseSensitive() )
            {
                path.MakeLower();
                libraryPath.MakeLower();
            }

            const wxString separator( wxFileName::GetPathSeparator() );

            if( path == libraryPath || path.StartsWith( libraryPath + separator )
                || libraryPath.StartsWith( path + separator ) )
            {
                m_reporter->Report( _( "The copied library folder overlaps a schematic or project output path. "
                                       "Choose a different library folder.\n" ),
                                    RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_ARGS;
            }
        }
    }

    if( !prepareOutput() )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( target->m_schVersion < SCH_VER_VARIANTS )
    {
        std::vector<wxString> variants( sch->GetVariantNames().begin(), sch->GetVariantNames().end() );

        for( const wxString& name : variants )
            sch->DeleteVariant( name );
    }

    // Keep designs, companion metadata, and libraries staged until every file is verified.
    DOWNGRADE_FILE_TRANSACTION transaction;
    wxString                   currentTemp;

    try
    {
        wxFileName base( job->m_filename );
        base.MakeAbsolute();

        for( const auto& [sheet, screen] : screens )
        {
            DowngradeScreenInPlace( screen, *target, job->m_dropInsteadOfApproximate );

            wxFileName screenFn( screen->GetFileName() );

            if( !screenFn.IsAbsolute() )
                screenFn.MakeAbsolute( base.GetPath() );

            // Resolve embedded dot segments, or an absolute path with ../ could sidestep the check.
            screenFn.Normalize( wxPATH_NORM_DOTS );

            // Never write outside the tree that holds the root schematic. A sheet with an absolute
            // or ../ path would otherwise overwrite the user's original shared sheet.
            wxFileName rootDir( base.GetPath(), wxEmptyString );

            wxString screenPath = screenFn.GetFullPath();
            wxString rootPrefix = rootDir.GetPathWithSep();

            // Case-insensitive filesystems must not let a case mismatch sidestep the check.
            bool contained = wxFileName::IsCaseSensitive() ? screenPath.StartsWith( rootPrefix )
                                                           : screenPath.Lower().StartsWith( rootPrefix.Lower() );

            if( !contained )
            {
                m_reporter->Report( wxString::Format( _( "Cannot export: sheet '%s' lies outside the project "
                                                         "folder.\n" ),
                                                      screenFn.GetFullPath() ),
                                    RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }

            wxString finalPath = screenFn.GetFullPath();

            // With an output folder, mirror the sheet's place in the tree there instead of
            // rewriting the original.
            if( !job->m_outputDir.IsEmpty() )
            {
                wxFileName destFn( job->m_outputDir + wxFileName::GetPathSeparator()
                                   + screenPath.Mid( rootPrefix.length() ) );

                if( !wxDirExists( destFn.GetPath() )
                    && !wxFileName::Mkdir( destFn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
                {
                    m_reporter->Report(
                            wxString::Format( _( "Could not create the output folder '%s'.\n" ), destFn.GetPath() ),
                            RPT_SEVERITY_ERROR );
                    return CLI::EXIT_CODES::ERR_UNKNOWN;
                }

                finalPath = destFn.GetFullPath();
            }

            if( !job->m_inPlace && !ValidateFreshDowngradeOutput( finalPath, outputError ) )
                THROW_IO_ERROR( outputError );

            currentTemp = transaction.Stage( finalPath );

            if( currentTemp.IsEmpty() )
                THROW_IO_ERROR( _( "Could not stage the schematic export." ) );

            SaveSchematicForTarget( sheet, sch, currentTemp, *target );

            wxString unsupported;

            bool good = VerifyDowngradedFile(
                    currentTemp, target->m_schVersion,
                    [&]( const wxString& aText )
                    {
                        return FindUnsupportedSchToken( aText, *target );
                    },
                    &unsupported );

            if( !good )
            {
                if( !unsupported.IsEmpty() )
                    m_reporter->Report( wxString::Format( _( "Cannot export: the result still uses '%s', which %s "
                                                             "cannot open. This feature has no downgrade rule "
                                                             "yet.\n" ),
                                                          unsupported, target->m_name ),
                                        RPT_SEVERITY_ERROR );
                else
                    m_reporter->Report(
                            wxString::Format( _( "Cannot export: could not verify '%s'.\n" ), screenFn.GetFullPath() ),
                            RPT_SEVERITY_ERROR );

                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }

            currentTemp.Clear();
        }

        if( target->m_schVersion >= SCH_VER_VARIANTS )
        {
            nlohmann::json document = nlohmann::json::object();
            wxString       content;

            if( projectFn.FileExists() )
            {
                wxFFile sourceProject( projectFn.GetFullPath(), wxT( "rb" ) );

                if( !sourceProject.IsOpened() || !sourceProject.ReadAll( &content ) )
                    THROW_IO_ERROR( _( "Could not read the schematic project settings." ) );

                try
                {
                    document = nlohmann::json::parse( content.ToStdString( wxConvUTF8 ) );
                }
                catch( const nlohmann::json::exception& )
                {
                    THROW_IO_ERROR( _( "Could not read the schematic project settings." ) );
                }
            }

            if( !document.is_object() )
                THROW_IO_ERROR( _( "The schematic project settings are invalid." ) );

            nlohmann::json aliases = nlohmann::json::object();

            for( const auto& alias : sch->GetAllBusAliases() )
            {
                nlohmann::json members = nlohmann::json::array();

                for( const wxString& member : alias->Members() )
                    members.push_back( member.ToStdString( wxConvUTF8 ) );

                aliases[alias->GetName().ToStdString( wxConvUTF8 )] = std::move( members );
            }

            if( !document.contains( "schematic" ) || !document["schematic"].is_object() )
                document["schematic"] = nlohmann::json::object();

            nlohmann::json& schematicSettings = document["schematic"];
            bool            needsProject = !job->m_outputDir.IsEmpty()
                                || schematicSettings.value( "bus_aliases", nlohmann::json::object() ) != aliases;

            if( needsProject )
            {
                schematicSettings["bus_aliases"] = std::move( aliases );
                nlohmann::json roots = nlohmann::json::array();

                for( const TOP_LEVEL_SHEET_INFO& root : projectFile.GetTopLevelSheets() )
                {
                    wxFileName filename( root.filename );
                    filename.MakeAbsolute( projectPtr->GetProjectPath() );
                    filename.MakeRelativeTo( base.GetPath() );
                    roots.push_back( { { "uuid", root.uuid.AsString().ToStdString( wxConvUTF8 ) },
                                       { "name", root.name.ToStdString( wxConvUTF8 ) },
                                       { "filename", filename.GetFullPath().ToStdString( wxConvUTF8 ) } } );
                }

                schematicSettings["top_level_sheets"] = roots;

                if( !document.contains( "meta" ) )
                    document["meta"] = { { "version", 3 } };

                wxFileName destination = projectFn;

                if( !job->m_outputDir.IsEmpty() )
                    destination.SetPath( job->m_outputDir );

                wxString finalPath = destination.GetFullPath();
                if( !job->m_inPlace && !ValidateFreshDowngradeOutput( finalPath, outputError ) )
                    THROW_IO_ERROR( outputError );

                currentTemp = transaction.Stage( finalPath );

                if( currentTemp.IsEmpty() )
                    THROW_IO_ERROR( _( "Could not stage the schematic project settings." ) );

                wxFFile outputProject( currentTemp, wxT( "wb" ) );

                if( !outputProject.IsOpened()
                    || !outputProject.Write( wxString::FromUTF8( ( document.dump( 2 ) + "\n" ).c_str() ) )
                    || !outputProject.Close() )
                {
                    THROW_IO_ERROR( _( "Could not save the schematic project settings." ) );
                }

                currentTemp.Clear();
            }
        }
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report(
                wxString::Format( _( "Error saving schematic file '%s'.\n%s" ), job->m_filename, ioe.What().GetData() ),
                RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( int libResult = downgradeLibraries( &transaction ); libResult != CLI::EXIT_CODES::SUCCESS )
        return libResult;

    wxString failedDestination = transaction.Commit();

    if( !failedDestination.IsEmpty() )
    {
        m_reporter->Report( wxString::Format( _( "Could not replace '%s'.\n" ), failedDestination ),
                            RPT_SEVERITY_ERROR );

        if( !transaction.GetRecoveryError().IsEmpty() )
            m_reporter->Report( transaction.GetRecoveryError(), RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    outputDirectories.Commit();
    m_reporter->Report( _( "Saved downgraded schematic\n" ), RPT_SEVERITY_INFO );

    return CLI::EXIT_CODES::SUCCESS;
}


int EESCHEMA_JOBS_HANDLER::JobImport( JOB* aJob )
{
    JOB_SCH_IMPORT* job = dynamic_cast<JOB_SCH_IMPORT*>( aJob );

    if( !job )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    job->m_netNameMap.clear();

    if( !wxFile::Exists( job->m_inputFile ) )
    {
        m_reporter->Report( wxString::Format( _( "Input file not found: '%s'\n" ), job->m_inputFile ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // AUTO restricts autodetect to non-KiCad plugins so a native file is not re-imported.
    SCH_IO_MGR::SCH_FILE_T fileType = SCH_IO_MGR::SCH_FILE_UNKNOWN;

    switch( job->m_format )
    {
    case JOB_SCH_IMPORT::FORMAT::AUTO:
        fileType = SCH_IO_MGR::GuessPluginTypeFromSchPath( job->m_inputFile, KICTL_NONKICAD_ONLY );
        break;
    case JOB_SCH_IMPORT::FORMAT::ALTIUM:     fileType = SCH_IO_MGR::SCH_ALTIUM;          break;
    case JOB_SCH_IMPORT::FORMAT::EAGLE:      fileType = SCH_IO_MGR::SCH_EAGLE;           break;
    case JOB_SCH_IMPORT::FORMAT::CADSTAR:    fileType = SCH_IO_MGR::SCH_CADSTAR_ARCHIVE; break;
    case JOB_SCH_IMPORT::FORMAT::EASYEDA:    fileType = SCH_IO_MGR::SCH_EASYEDA;         break;
    case JOB_SCH_IMPORT::FORMAT::EASYEDAPRO: fileType = SCH_IO_MGR::SCH_EASYEDAPRO;      break;
    case JOB_SCH_IMPORT::FORMAT::LTSPICE:    fileType = SCH_IO_MGR::SCH_LTSPICE;         break;
    case JOB_SCH_IMPORT::FORMAT::PADS:       fileType = SCH_IO_MGR::SCH_PADS;            break;
    case JOB_SCH_IMPORT::FORMAT::DIPTRACE:   fileType = SCH_IO_MGR::SCH_DIPTRACE;        break;
    case JOB_SCH_IMPORT::FORMAT::PCAD:       fileType = SCH_IO_MGR::SCH_PCAD;            break;
    case JOB_SCH_IMPORT::FORMAT::ORCAD:      fileType = SCH_IO_MGR::SCH_ORCAD;           break;
    case JOB_SCH_IMPORT::FORMAT::EASYPC:     fileType = SCH_IO_MGR::SCH_EASYPC;          break;
    }

    if( fileType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
    {
        // Quiet sentinel: lets the top-level `import` command treat the file as not-a-schematic.
        m_reporter->Report( wxString::Format( _( "No schematic importer recognizes the file format "
                                                 "of '%s'\n" ),
                                              job->m_inputFile ),
                            RPT_SEVERITY_INFO );
        return CLI::EXIT_CODES::ERR_UNKNOWN_FILE_FORMAT;
    }

    wxString outputPath = job->GetConfiguredOutputPath();

    if( outputPath.IsEmpty() )
        outputPath = DefaultImportOutputPath( job->m_inputFile, FILEEXT::KiCadSchematicFileExtension );

    wxFileName inputFn( job->m_inputFile );
    inputFn.MakeAbsolute();

    wxFileName outputFn( outputPath );
    outputFn.MakeAbsolute();

    // Foreign importers resolve their symbol library against the *active* project, so an import
    // with no project loaded needs a transient active one at the output location (never written to
    // disk; LoadProject returns false yet still registers it, hence the GetProject() check).
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    PROJECT*          projectPtr = nullptr;
    bool              createdTransientProject = false;

    if( mgr.IsProjectOpenNotDummy() )
    {
        projectPtr = &mgr.Prj();
    }
    else
    {
        wxFileName projectFn( outputFn );
        projectFn.SetExt( FILEEXT::ProjectFileExtension );

        mgr.LoadProject( projectFn.GetFullPath(), true );
        projectPtr = mgr.GetProject( projectFn.GetFullPath() );
        createdTransientProject = ( projectPtr != nullptr );
    }

    if( !projectPtr )
    {
        m_reporter->Report( _( "Could not establish a project for the import\n" ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    PROJECT& project = *projectPtr;

    // Declared before the SCHEMATIC so reverse-destruction tears the schematic (which references
    // the project) down first; unloads the transient project on every exit path.
    TRANSIENT_PROJECT_GUARD transientProjectGuard{ mgr, projectPtr, createdTransientProject };

    LOCALE_IO dummy;

    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( &project );

    wxString   formatName = SCH_IO_MGR::ShowType( fileType );
    SCH_SHEET* loadedSheet = nullptr;

    // Declared before the plugin so it outlives the plugin's pointer to it
    IMPORT_MESSAGE_COLLECTOR importMessages( *m_reporter );

    // outlives the load so the retained symbol definitions can be reconciled below
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( fileType ) );

    if( !pi )
    {
        m_reporter->Report( wxString::Format( _( "No plugin found for file type '%s'\n" ),
                                              formatName ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    // Without this the importer's warnings go to the wx log and never reach the import report
    pi->SetReporter( &importMessages );

    try
    {
        m_reporter->Report( wxString::Format( _( "Importing '%s' using %s format...\n" ),
                                              inputFn.GetFullPath(), formatName ),
                            RPT_SEVERITY_INFO );

        loadedSheet = pi->LoadSchematicFile( inputFn.GetFullPath(), schematic.get() );

        if( !loadedSheet )
        {
            m_reporter->Report( _( "Failed to load schematic\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Error during import: %s\n" ), ioe.What() ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    size_t symbolCount = 0;
    size_t sheetCount = 0;

    try
    {
        // Some importers build the top-level sheet set themselves; only collapse to the returned
        // sheet otherwise (mirrors SCH_EDIT_FRAME::importFile()).
        std::vector<SCH_SHEET*> topLevelSheets = schematic->GetTopLevelSheets();
        bool loadedIsTopLevel = std::find( topLevelSheets.begin(), topLevelSheets.end(),
                                           loadedSheet ) != topLevelSheets.end();
        bool loadedIsVirtualRoot = loadedSheet == &schematic->Root()
                                   || loadedSheet->IsVirtualRootSheet();

        if( !loadedIsTopLevel && !loadedIsVirtualRoot )
            schematic->SetTopLevelSheets( { loadedSheet } );

        // Extract a project symbol library and re-link LIB_IDs, as importFile() does; without it
        // the saved schematic references nicknames no library table row resolves.
        ReconcileImportedSymbols( *pi, *schematic, project, inputFn.GetFullPath(), nullptr,
                                  *m_reporter );

        // Recompute connectivity so instance data is valid before saving, as importFile() does.
        std::unique_ptr<TOOL_MANAGER> toolManager = std::make_unique<TOOL_MANAGER>();
        toolManager->SetEnvironment( schematic.get(), nullptr, nullptr, Kiface().KifaceSettings(),
                                     nullptr );

        {
            SCH_COMMIT dummyCommit( toolManager.get() );
            schematic->RecalculateConnections( &dummyCommit, GLOBAL_CLEANUP, toolManager.get() );
            dummyCommit.Push( _( "Schematic Cleanup" ), SKIP_UNDO | SKIP_CONNECTIVITY | DELETE_REMOVED_ITEMS );
        }

        schematic->SetSheetNumberAndCount();

        // Imported schematics preserve their source page frame as schematic graphics.
        // Match the editor import path and suppress KiCad's default frame/title block.
        schematic->Settings().m_SchDrawingSheetFileName = wxS( "empty.kicad_wks" );
        DS_DATA_MODEL::GetTheInstance().SetEmptyLayout();

        if( SCH_SHEET* topSheet = schematic->GetTopLevelSheet() )
            topSheet->SetFileName( outputFn.GetFullName() );

        schematic->RootScreen()->SetFileName( outputFn.GetFullPath() );

        SCH_SCREENS screens( schematic->Root() );

        std::unordered_map<SCH_SCREEN*, wxString> filenameMap;
        filenameMap[schematic->RootScreen()] = outputFn.GetFullPath();

        wxString errorMsg;

        if( !PrepareSaveAsFiles( *schematic, screens, inputFn, outputFn, /*aSaveCopy*/ true,
                                 /*aCopySubsheets*/ true, /*aIncludeExternSheets*/ true,
                                 filenameMap, errorMsg ) )
        {
            m_reporter->Report( errorMsg + wxS( "\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        // PrepareSaveAsFiles seeds an entry (empty for sheets it does not relocate) for every
        // screen; empty paths are skipped.
        IO_RELEASER<SCH_IO> kicadPi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

        for( size_t i = 0; i < screens.GetCount(); i++ )
        {
            SCH_SCREEN* screen = screens.GetScreen( i );
            wxString    path = filenameMap[screen];

            if( path.IsEmpty() )
                continue;

            wxFileName fn( path );
            fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

            kicadPi->SaveSchematicFile( fn.GetFullPath(), screens.GetSheet( i ), schematic.get() );
            sheetCount++;

            auto symbols = screen->Items().OfType( SCH_SYMBOL_T );
            symbolCount += std::distance( symbols.begin(), symbols.end() );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Error saving imported schematic: %s\n" ),
                                              ioe.What() ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }
    catch( const std::exception& exc )
    {
        m_reporter->Report( wxString::Format( _( "Error saving imported schematic: %s\n" ),
                                              exc.what() ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    // The board job renames its nets from this; nothing is written beside the schematic.
    if( const IMPORT_NET_MAP* map = schematic->GetImportNetMap() )
        job->m_netNameMap = GetBoardNetNameMap( *map, *m_reporter );

    m_reporter->Report( wxString::Format( _( "Successfully saved imported schematic to '%s'\n" ),
                                          outputFn.GetFullPath() ),
                        RPT_SEVERITY_INFO );

    // Linked by the top-level `import` command's subsequent SaveProject().
    if( Pgm().GetSettingsManager().IsProjectOpenNotDummy() )
    {
        std::vector<FILE_INFO_PAIR>& projectSheets = project.GetProjectFile().GetSheets();
        projectSheets.clear();

        for( const SCH_SHEET_PATH& sheetPath : schematic->Hierarchy() )
        {
            SCH_SHEET* sheet = sheetPath.Last();

            if( sheet && !sheet->IsVirtualRootSheet() )
                projectSheets.emplace_back( std::make_pair( sheet->m_Uuid, sheet->GetName() ) );
        }

        // Reload uses the top-level sheet list, not the UUID-to-name map.
        const std::vector<SCH_SHEET*>& topLevelSheets = schematic->GetTopLevelSheets();

        if( !topLevelSheets.empty() )
        {
            std::vector<TOP_LEVEL_SHEET_INFO>& infos = project.GetProjectFile().GetTopLevelSheets();
            infos.clear();

            wxString projectPath = project.GetProjectPath();

            for( SCH_SHEET* sheet : topLevelSheets )
            {
                if( !sheet || !sheet->GetScreen() )
                    continue;

                wxFileName sheetFn( sheet->GetScreen()->GetFileName() );

                if( sheetFn.IsAbsolute() )
                    sheetFn.MakeRelativeTo( projectPath );

                infos.emplace_back( sheet->m_Uuid, sheet->GetName(), sheetFn.GetFullPath() );
            }
        }
    }

    // Extra top-level sheets need a project file to remain reachable.
    if( createdTransientProject && schematic->GetTopLevelSheets().size() > 1 )
    {
        m_reporter->Report( wxString::Format( _( "%zu sheets were written, but only '%s' is "
                                                 "reachable: top-level sheets are recorded in a "
                                                 "project file. Use 'kicad-cli import' to create "
                                                 "one.\n" ),
                                              sheetCount, outputFn.GetFullName() ),
                            RPT_SEVERITY_WARNING );
    }

    IMPORT_REPORT_DATA reportData;
    importMessages.Flush( job->m_reportFormat, reportData );

    if( job->m_reportFormat != IMPORT_REPORT_FORMAT::NONE )
    {
        reportData.m_sourceFile = inputFn.GetFullName();
        reportData.m_sourceFormat = formatName;
        reportData.m_outputFile = outputFn.GetFullName();
        reportData.m_statistics = {
            { wxS( "symbols" ), symbolCount },
            { wxS( "sheets" ), sheetCount }
        };

        reportData.m_statistics.emplace_back( wxS( "renamed_board_nets" ), job->m_netNameMap.size() );

        if( !WriteImportReport( m_reporter, job->m_reportFormat, job->m_reportFile, reportData ) )
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    return CLI::EXIT_CODES::SUCCESS;
}


DS_PROXY_VIEW_ITEM* EESCHEMA_JOBS_HANDLER::getDrawingSheetProxyView( SCHEMATIC* aSch )
{
    DS_PROXY_VIEW_ITEM* drawingSheet =
            new DS_PROXY_VIEW_ITEM( schIUScale, &aSch->RootScreen()->GetPageSettings(), &aSch->Project(),
                                    &aSch->RootScreen()->GetTitleBlock(), aSch->GetProperties() );

    drawingSheet->SetPageNumber( TO_UTF8( aSch->RootScreen()->GetPageNumber() ) );
    drawingSheet->SetSheetCount( aSch->RootScreen()->GetPageCount() );
    drawingSheet->SetFileName( TO_UTF8( aSch->RootScreen()->GetFileName() ) );
    drawingSheet->SetColorLayer( LAYER_SCHEMATIC_DRAWINGSHEET );
    drawingSheet->SetPageBorderColorLayer( LAYER_SCHEMATIC_PAGE_LIMITS );
    drawingSheet->SetIsFirstPage( aSch->RootScreen()->GetVirtualPageNumber() == 1 );

    wxString currentVariant = aSch->GetCurrentVariant();
    wxString variantDesc = aSch->GetVariantDescription( currentVariant );
    drawingSheet->SetVariantName( TO_UTF8( currentVariant ) );
    drawingSheet->SetVariantDesc( TO_UTF8( variantDesc ) );

    drawingSheet->SetSheetName( "" );
    drawingSheet->SetSheetPath( "" );

    return drawingSheet;
}


// ============================================================================
// JobSchDiff: sch_diff implementation
// ============================================================================
#include <diff_merge/diff_job_output.h>
#include <diff_merge/diff_renderer_plotter.h>
#include <diff_merge/diff_scene.h>
#include <diff_merge/sch_differ.h>
#include <diff_merge/sch_geometry_extractor.h>
#include <diff_merge/project_file_patch.h>
#include <diff_merge/kicad_diff_types.h>
#include <project/project_file.h>
#include <settings/json_settings_internals.h>
#include <jobs/job_sch_diff.h>
#include <jobs/scratch_doc.h>
#include <wx/file.h>


// Load a schematic into a SCRATCH_DOC<SCHEMATIC> that keeps a dedicated scratch
// PROJECT attached for the document's lifetime. Without a per-document project,
// a second LoadProject(path, true) destroys the first project and the first
// schematic's m_project dangles. The destructor severs the link via
// SetProject( nullptr ). Shared by every SCH diff/merge job.
static SCRATCH_DOC<SCHEMATIC> loadScratchSchematic( SETTINGS_MANAGER& aMgr, const wxString& aPath )
{
    return LoadScratchDoc<SCHEMATIC>(
            aMgr, aPath,
            [aPath]( PROJECT* aProject )
            {
                return std::unique_ptr<SCHEMATIC>(
                        EESCHEMA_HELPERS::LoadSchematic( aPath,
                                                         /*aSetActive=*/false,
                                                         /*aForceDefaultProject=*/false, aProject,
                                                         /*aCalculateConnectivity=*/false ) );
            },
            []( SCHEMATIC* aSch )
            {
                aSch->SetProject( nullptr );
            } );
}


int EESCHEMA_JOBS_HANDLER::JobSchDiff( JOB* aJob )
{
    JOB_SCH_DIFF* diffJob = dynamic_cast<JOB_SCH_DIFF*>( aJob );

    if( !diffJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    // Two schematics in the same SettingsManager need scratch PROJECTs;
    // otherwise the second LoadProject(path, true) destroys the first
    // project and the first schematic's m_project dangles, crashing on
    // any per-instance bbox / field / reference resolution (e.g. inside
    // SCH_DIFFER's makeDescriptor calling SCH_SYMBOL::GetRef).
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    SCRATCH_DOC<SCHEMATIC> a = loadScratchSchematic( mgr, diffJob->m_inputA );
    SCRATCH_DOC<SCHEMATIC> b = loadScratchSchematic( mgr, diffJob->m_inputB );

    if( !a.doc )
    {
        m_reporter->Report( wxString::Format( _( "Failed to load %s\n" ), diffJob->m_inputA ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( !b.doc )
    {
        m_reporter->Report( wxString::Format( _( "Failed to load %s\n" ), diffJob->m_inputB ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    SCHEMATIC* schA = a.doc.get();
    SCHEMATIC* schB = b.doc.get();

    KICAD_DIFF::SCH_DIFFER    differ( schA, schB, diffJob->m_inputB );
    KICAD_DIFF::DOCUMENT_DIFF result = differ.Diff();

    int diffExitCode = KICAD_DIFF::DiffExitCode( result );

    if( diffJob->m_exitCodeOnly )
        return diffExitCode;

    // The schematic geometry (wires, junctions, symbol/sheet/label bbox
    // outlines) renders beneath the change rectangles for PNG/SVG, matching
    // the interactive dialog.
    KICAD_DIFF::DIFF_EMIT_OPTIONS emitOpts =
            KICAD_DIFF::MakeEmitOptions( *diffJob, diffJob->m_inputA, diffJob->m_inputB );
    emitOpts.docKind            = KICAD_DIFF::DOC_KIND::SCH;
    emitOpts.referenceGeometry  = [&]( const KIGFX::COLOR4D& aColor )
                                  { return KICAD_DIFF::ExtractSchematicGeometry( *schA, aColor ); };
    emitOpts.comparisonGeometry = [&]( const KIGFX::COLOR4D& aColor )
                                  { return KICAD_DIFF::ExtractSchematicGeometry( *schB, aColor ); };

    return KICAD_DIFF::EmitDiffResult( result, emitOpts, diffExitCode, *m_reporter );
}


// ============================================================================
// JobSymDiff: sym_diff implementation
// ============================================================================
#include <diff_merge/sym_lib_differ.h>
#include <jobs/job_sym_diff.h>


// Load one side of a symbol-library diff into its owner vector and name map.
// When aAllowEmpty is set an empty path resolves to a clean (empty) side; the
// non-interactive job path leaves it unset so a missing path is an input error.
static int loadSymbolLibrarySide( const wxString& aPath,
                                  std::vector<std::unique_ptr<LIB_SYMBOL>>& aOwners,
                                  KICAD_DIFF::SYM_LIB_DIFFER::SYMBOL_MAP& aMap, bool aAllowEmpty,
                                  REPORTER& aReporter )
{
    if( aAllowEmpty && aPath.IsEmpty() )
        return CLI::EXIT_CODES::SUCCESS;

    try
    {
        auto loaded = KICAD_DIFF::SYM_LIB_DIFFER::LoadLibrary( aPath );
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


// Flatten a symbol-library name map into a single DOCUMENT_GEOMETRY tinted with
// the supplied per-side theme colour.
static KICAD_DIFF::DOCUMENT_GEOMETRY
symbolLibraryGeometry( const KICAD_DIFF::SYM_LIB_DIFFER::SYMBOL_MAP& aMap, const KIGFX::COLOR4D& aColor )
{
    KICAD_DIFF::DOCUMENT_GEOMETRY geometry;

    for( const auto& [name, symbol] : aMap )
    {
        if( symbol )
            KICAD_DIFF::AppendGeometry( geometry, KICAD_DIFF::ExtractSymbolGeometry( *symbol, aColor ) );
    }

    return geometry;
}


int EESCHEMA_JOBS_HANDLER::JobSymDiff( JOB* aJob )
{
    JOB_SYM_DIFF* diffJob = dynamic_cast<JOB_SYM_DIFF*>( aJob );

    if( !diffJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    std::vector<std::unique_ptr<LIB_SYMBOL>> ownersA;
    std::vector<std::unique_ptr<LIB_SYMBOL>> ownersB;
    KICAD_DIFF::SYM_LIB_DIFFER::SYMBOL_MAP   mapA;
    KICAD_DIFF::SYM_LIB_DIFFER::SYMBOL_MAP   mapB;

    if( int rc = loadSymbolLibrarySide( diffJob->m_inputA, ownersA, mapA, false, *m_reporter );
        rc != CLI::EXIT_CODES::SUCCESS )
    {
        return rc;
    }

    if( int rc = loadSymbolLibrarySide( diffJob->m_inputB, ownersB, mapB, false, *m_reporter );
        rc != CLI::EXIT_CODES::SUCCESS )
    {
        return rc;
    }

    KICAD_DIFF::SYM_LIB_DIFFER differ( mapA, mapB, diffJob->m_inputB );
    KICAD_DIFF::DOCUMENT_DIFF  result = differ.Diff();

    int diffExitCode = KICAD_DIFF::DiffExitCode( result );

    if( diffJob->m_exitCodeOnly )
        return diffExitCode;

    KICAD_DIFF::DIFF_EMIT_OPTIONS emitOpts =
            KICAD_DIFF::MakeEmitOptions( *diffJob, diffJob->m_inputA, diffJob->m_inputB );
    emitOpts.docKind            = KICAD_DIFF::DOC_KIND::SYM_LIB;
    emitOpts.referenceGeometry  = [&]( const KIGFX::COLOR4D& aColor )
                                  { return symbolLibraryGeometry( mapA, aColor ); };
    emitOpts.comparisonGeometry = [&]( const KIGFX::COLOR4D& aColor )
                                  { return symbolLibraryGeometry( mapB, aColor ); };

    return KICAD_DIFF::EmitDiffResult( result, emitOpts, diffExitCode, *m_reporter );
}


// ============================================================================
// JobOpenDiffDialog: load two on-disk files and open DIALOG_KICAD_DIFF.
// Dispatched from the project manager / PR-review dialog via KIWAY.
// ============================================================================
#include <dialogs/dialog_kicad_diff.h>
#include <diff_merge/sch_diff_canvas_context.h>
#include <diff_merge/sch_geometry_extractor.h>
#include <jobs/scratch_doc.h>


int EESCHEMA_JOBS_HANDLER::OpenDiffDialog( KICAD_DIFF::DOC_KIND aKind, const wxString& aFileA,
                                           const wxString& aFileB, const wxString& aLabelA,
                                           const wxString& aLabelB, wxWindow* aParent,
                                           REPORTER* aReporter )
{
    // Restore m_reporter on scope exit so a caller's transient (often
    // stack-local) reporter doesn't outlive this call as a dangling member.
    SCOPED_SET_RESET<REPORTER*> reporterGuard( m_reporter,
                                               aReporter ? aReporter : m_reporter );

    wxWindow* parent = aParent ? aParent : ( wxTheApp ? wxTheApp->GetTopWindow() : nullptr );

    KICAD_DIFF::DOCUMENT_DIFF     result;
    KICAD_DIFF::DOCUMENT_GEOMETRY refGeometry;
    KICAD_DIFF::DOCUMENT_GEOMETRY compGeometry;

    switch( aKind )
    {
    case KICAD_DIFF::DOC_KIND::SCH:
    {
        SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

        SCRATCH_DOC<SCHEMATIC> a = loadScratchSchematic( mgr, aFileA );
        SCRATCH_DOC<SCHEMATIC> b = loadScratchSchematic( mgr, aFileB );

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

        // Synthesize empty SCHEMATICs against scratch PROJECTs for any
        // missing side so SCH_DIFFER sees a valid empty document.
        PROJECT                    scratchPrjA;
        PROJECT                    scratchPrjB;
        std::unique_ptr<SCHEMATIC> emptyA;
        std::unique_ptr<SCHEMATIC> emptyB;

        if( !a.doc )
        {
            emptyA = std::make_unique<SCHEMATIC>( &scratchPrjA );
            emptyA->CreateDefaultScreens();
        }

        if( !b.doc )
        {
            emptyB = std::make_unique<SCHEMATIC>( &scratchPrjB );
            emptyB->CreateDefaultScreens();
        }

        SCHEMATIC* schA = a.doc ? a.doc.get() : emptyA.get();
        SCHEMATIC* schB = b.doc ? b.doc.get() : emptyB.get();

        KICAD_DIFF::SCH_DIFFER differ( schA, schB, aFileB );
        result = differ.Diff();

        const KICAD_DIFF::DIFF_COLOR_THEME theme;
        refGeometry = KICAD_DIFF::ExtractSchematicGeometry( *schA, theme.reference );
        compGeometry = KICAD_DIFF::ExtractSchematicGeometry( *schB, theme.comparison );

        const wxString labelA = aLabelA.IsEmpty() ? aFileA : aLabelA;
        const wxString labelB = aLabelB.IsEmpty() ? aFileB : aLabelB;

        DIALOG_KICAD_DIFF dlg(
                parent, labelA, labelB, result, std::move( refGeometry ), std::move( compGeometry ),
                [schA, schB, color = theme.reference]( WIDGET_DIFF_CANVAS& aCanvas, const KIID_PATH& aSheetPath )
                {
                    SCH_SCREEN* refScreen = schA ? schA->RootScreen() : nullptr;
                    SCH_SCREEN* compScreen = schB ? schB->RootScreen() : nullptr;

                    if( !aSheetPath.empty() )
                    {
                        if( schA )
                        {
                            if( auto sp = schA->Hierarchy().GetSheetPathByKIIDPath( aSheetPath, true ) )
                                refScreen = sp->LastScreen();
                        }

                        if( schB )
                        {
                            if( auto sp = schB->Hierarchy().GetSheetPathByKIIDPath( aSheetPath, true ) )
                                compScreen = sp->LastScreen();
                        }
                    }

                    KICAD_DIFF::ConfigureSchDiffCanvasContext( aCanvas, schA, schB, color, {}, {}, {}, refScreen,
                                                               compScreen );
                } );
        dlg.ShowModal();

        if( emptyA )
            emptyA->SetProject( nullptr );

        if( emptyB )
            emptyB->SetProject( nullptr );

        return CLI::EXIT_CODES::SUCCESS;
    }
    case KICAD_DIFF::DOC_KIND::SYM_LIB:
    {
        std::vector<std::unique_ptr<LIB_SYMBOL>> ownersA;
        std::vector<std::unique_ptr<LIB_SYMBOL>> ownersB;
        KICAD_DIFF::SYM_LIB_DIFFER::SYMBOL_MAP   mapA;
        KICAD_DIFF::SYM_LIB_DIFFER::SYMBOL_MAP   mapB;

        if( int rc = loadSymbolLibrarySide( aFileA, ownersA, mapA, true, *m_reporter );
            rc != CLI::EXIT_CODES::SUCCESS )
        {
            return rc;
        }

        if( int rc = loadSymbolLibrarySide( aFileB, ownersB, mapB, true, *m_reporter );
            rc != CLI::EXIT_CODES::SUCCESS )
        {
            return rc;
        }

        KICAD_DIFF::SYM_LIB_DIFFER differ( mapA, mapB, aFileB );
        result = differ.Diff();

        const KICAD_DIFF::DIFF_COLOR_THEME theme;
        refGeometry = symbolLibraryGeometry( mapA, theme.reference );
        compGeometry = symbolLibraryGeometry( mapB, theme.comparison );
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
// JobSchMerge: sch_merge implementation
// ============================================================================
#include <diff_merge/sch_merge_applier.h>
#include <dialogs/dialog_kicad_merge_3way.h>
#include <jobs/scratch_doc.h>


int EESCHEMA_JOBS_HANDLER::RunMerge( KICAD_DIFF::DOC_KIND aKind, const wxString& aAncestor,
                                     const wxString& aOurs, const wxString& aTheirs,
                                     const wxString& aOutput, bool aInteractive, bool aSingleFile,
                                     REPORTER* aReporter )
{
    // Restore m_reporter on scope exit so a caller's transient (often
    // stack-local) reporter doesn't outlive this call as a dangling member.
    SCOPED_SET_RESET<REPORTER*> reporterGuard( m_reporter,
                                               aReporter ? aReporter : m_reporter );

    if( aKind == KICAD_DIFF::DOC_KIND::SYM_LIB )
        return runSymLibMerge( aAncestor, aOurs, aTheirs, aOutput );

    return runSchMerge( aAncestor, aOurs, aTheirs, aOutput, aInteractive );
}


int EESCHEMA_JOBS_HANDLER::runSchMerge( const wxString& aAncestor, const wxString& aOurs,
                                        const wxString& aTheirs, const wxString& aOutput,
                                        bool aInteractive )
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    SCRATCH_DOC<SCHEMATIC> ancestor = loadScratchSchematic( mgr, aAncestor );
    SCRATCH_DOC<SCHEMATIC> ours = loadScratchSchematic( mgr, aOurs );
    SCRATCH_DOC<SCHEMATIC> theirs = loadScratchSchematic( mgr, aTheirs );

    if( !ancestor.doc || !ours.doc || !theirs.doc )
    {
        m_reporter->Report( _( "Failed to load one or more input schematics\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Multi-sheet hierarchies are supported: each non-root sub-sheet is
    // written alongside the output root using its original basename. Top-level
    // sheets stay singular — multiple roots is an editor invariant the diff
    // engine never models, so refuse those.
    auto hasSingleRoot = []( const SCHEMATIC* aSch )
    {
        return aSch->GetTopLevelSheets().size() == 1;
    };

    if( !hasSingleRoot( ancestor.doc.get() ) || !hasSingleRoot( ours.doc.get() ) || !hasSingleRoot( theirs.doc.get() ) )
    {
        m_reporter->Report( _( "sch merge requires each input to have a single top-level sheet\n" ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    KICAD_DIFF::SCH_DIFFER ourDiff( ancestor.doc.get(), ours.doc.get() );
    KICAD_DIFF::SCH_DIFFER theirDiff( ancestor.doc.get(), theirs.doc.get() );

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

        const KICAD_DIFF::DIFF_COLOR_THEME        theme;
        DIALOG_KICAD_MERGE_3WAY::CONFLICT_CONTEXT ctx;

        if( ancestor.doc )
            ctx.ancestorGeometry = KICAD_DIFF::ExtractSchematicGeometry( *ancestor.doc, theme.reference );

        if( ours.doc )
            ctx.oursGeometry = KICAD_DIFF::ExtractSchematicGeometry( *ours.doc, theme.reference );

        if( theirs.doc )
            ctx.theirsGeometry = KICAD_DIFF::ExtractSchematicGeometry( *theirs.doc, theme.comparison );

        KICAD_DIFF::CollectChangeBBoxes( ourDocDiff, ctx.oursBBoxes );
        KICAD_DIFF::CollectChangeBBoxes( theirDocDiff, ctx.theirsBBoxes );

        DIALOG_KICAD_MERGE_3WAY dlg( wxTheApp->GetTopWindow(), plan, std::move( ctx ) );

        if( dlg.ShowModal() == wxID_APPLY )
            plan = dlg.GetResolvedPlan();
    }

    // Snapshot of the plan before the applier moves it; drives the
    // unresolved-conflict report below.
    const KICAD_DIFF::MERGE_PLAN planSnapshot = plan;

    KICAD_DIFF::SCH_MERGE_APPLIER applier( ancestor.doc.get(), ours.doc.get(), theirs.doc.get(), std::move( plan ) );

    if( !applier.Apply() )
    {
        m_reporter->Report( _( "Merge applier failed to produce a schematic\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    // Sheet add/remove/replace resolutions are explicitly skipped by
    // SCH_MERGE_APPLIER (see isSheetItem); succeeding here would silently
    // drop hierarchy edits.
    if( applier.GetReport().sheetActionsSkipped > 0 )
    {
        m_reporter->Report( _( "Merge contains hierarchical sheet structure changes that sch merge "
                               "cannot apply\n" ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    // Refusal above guarantees a single top-level sheet.
    SCH_SHEET* rootSheet = ancestor.doc->GetTopLevelSheet( 0 );

    wxFileName outFn( aOutput );
    outFn.MakeAbsolute();

    // Sub-sheets land alongside the root by basename. Preserving the original
    // relative-path subdirectory structure would force kicad-cli sch merge to
    // mkdir into user space; the basename-flat scheme is what makes the common
    // git-mergetool case work without surprises.
    const wxString outDir = outFn.GetPath();

    SCH_SCREENS screens( rootSheet );
    SCH_SCREEN* rootScreen = rootSheet->GetScreen();

    // Detect two sub-sheets sharing a basename (e.g., a/foo.kicad_sch and
    // b/foo.kicad_sch) before any I/O — the flat output layout can't honor
    // both, and silently overwriting one is the worst outcome.
    std::map<wxString, SCH_SCREEN*> basenameOwner;

    for( size_t i = 0; i < screens.GetCount(); ++i )
    {
        SCH_SCREEN* screen = screens.GetScreen( i );

        if( !screen || screen == rootScreen )
            continue;

        const wxString basename = wxFileName( screen->GetFileName() ).GetFullName();

        if( basename.IsEmpty() )
            continue;

        auto [it, inserted] = basenameOwner.emplace( basename, screen );

        if( !inserted && it->second != screen )
        {
            m_reporter->Report( wxString::Format( _( "Cannot flatten sub-sheets with duplicate "
                                                     "basename '%s'\n" ),
                                                  basename ),
                                RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    // Rewrite every SCH_SHEET symbol's filename field to its child screen's
    // basename, so the root file (and any intermediate sheet) references the
    // flattened layout we're about to write.
    for( size_t i = 0; i < screens.GetCount(); ++i )
    {
        SCH_SCREEN* parent = screens.GetScreen( i );

        if( !parent )
            continue;

        for( SCH_ITEM* item : parent->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET*  childRef = static_cast<SCH_SHEET*>( item );
            SCH_SCREEN* childScreen = childRef->GetScreen();

            if( !childScreen || childScreen == rootScreen )
                continue;

            const wxString basename = wxFileName( childScreen->GetFileName() ).GetFullName();

            if( !basename.IsEmpty() )
                childRef->SetFileName( basename );
        }
    }

    try
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        pi->SaveSchematicFile( outFn.GetFullPath(), rootSheet, ancestor.doc.get() );

        for( size_t i = 0; i < screens.GetCount(); ++i )
        {
            SCH_SCREEN* screen = screens.GetScreen( i );
            SCH_SHEET*  sheet = screens.GetSheet( i );

            if( !screen || !sheet || screen == rootScreen )
                continue;

            const wxString basename = wxFileName( screen->GetFileName() ).GetFullName();

            if( basename.IsEmpty() )
                continue;

            wxFileName outSubFn( outDir, basename );
            pi->SaveSchematicFile( outSubFn.GetFullPath(), sheet, ancestor.doc.get() );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Failed to save merged schematic: %s\n" ), ioe.What() ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    // If the applier mutated project-file-scoped state (ERC severities, etc),
    // persist it as a sibling .kicad_pro alongside the .kicad_sch output —
    // otherwise the resolution dies with the process. Only write when actually
    // needed; clobbering an existing .kicad_pro with ancestor's project
    // would lose unrelated user settings (library tables, mru paths).
    if( applier.GetReport().projectFileTouched && ancestor.project )
    {
        wxFileName proFn = outFn;
        proFn.SetExt( FILEEXT::ProjectFileExtension );

        // JSON-patch path: only the diffed DOC_PROP fields are written into
        // the output .kicad_pro, so any non-diffed user customisations
        // (library tables, last paths, layer presets, text variables) are
        // preserved.  Fall back to SaveProjectCopy on parse failure.
        PROJECT_FILE& ancProj = ancestor.project->GetProjectFile();
        ancProj.Store();

        const KICAD_DIFF::SCH_MERGE_APPLIER::REPORT& mergeReport = applier.GetReport();

        // PROJECT_FILE::Store() flushes the project file's own params but not
        // its registered NESTED_SETTINGS. Flush only the resolved nested
        // settings so Internals() reflects the merge result without touching
        // unrelated project subtrees.
        if( mergeReport.ercSeveritiesTouched && ancestor.doc )
            ancestor.doc->ErcSettings().SaveToFile( wxEmptyString, true );

        if( mergeReport.drawingSheetFileTouched && ancestor.doc )
            ancestor.doc->Settings().SaveToFile( wxEmptyString, true );

        std::set<wxString> touched;
        if( mergeReport.ercSeveritiesTouched )
            touched.insert( KICAD_DIFF::DOC_PROP_ERC_SEVERITIES );

        if( mergeReport.drawingSheetFileTouched )
            touched.insert( KICAD_DIFF::DOC_PROP_DRAWING_SHEET );

        if( !KICAD_DIFF::ApplyProjectFilePatches( proFn.GetFullPath(), *ancProj.Internals(), touched,
                                                  KICAD_DIFF::DOC_KIND::SCH ) )
        {
            if( !Pgm().GetSettingsManager().SaveProjectCopy( proFn.GetFullPath(), ancestor.project ) )
            {
                m_reporter->Report(
                        wxString::Format( _( "Failed to save merged project file: %s\n" ), proFn.GetFullPath() ),
                        RPT_SEVERITY_ERROR );
                return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
            }
        }
    }

    // Surface post-apply validator findings (refdes collisions, schema
    // mismatch, missed connectivity rebuild). Advisory — they do not change the
    // exit code, only the merge's resolved/unresolved status does.
    for( const KICAD_DIFF::VALIDATION_FAILURE& f : applier.GetReport().validation.failures )
        m_reporter->Report( wxString::Format( wxS( "%s: %s\n" ), f.validator, f.message ), f.severity );

    // The merged schematic was written to m_outputPath above, so the output is
    // always valid. Unresolved conflicts are reported and signalled via the
    // exit code; the user resolves them with the interactive mergetool.
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
// JobSymLibMerge: 3-way merge of .kicad_sym libraries.
// ============================================================================
#include <diff_merge/sym_lib_merge_applier.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_lib_cache.h>
#include <wx/ffile.h>


int EESCHEMA_JOBS_HANDLER::runSymLibMerge( const wxString& aAncestor, const wxString& aOurs,
                                           const wxString& aTheirs, const wxString& aOutput )
{
    if( aOutput.IsEmpty() )
    {
        m_reporter->Report( _( "--output is required\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    // Three sides into name -> LIB_SYMBOL maps.
    struct LIB_SIDE
    {
        std::vector<std::unique_ptr<LIB_SYMBOL>> owners;
        KICAD_DIFF::SYM_LIB_DIFFER::SYMBOL_MAP   map;
    };

    LIB_SIDE ancestor, ours, theirs;

    auto loadSide = [&]( const wxString& aPath, LIB_SIDE& aSide ) -> int
    {
        try
        {
            auto loaded = KICAD_DIFF::SYM_LIB_DIFFER::LoadLibrary( aPath );
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

    KICAD_DIFF::SYM_LIB_DIFFER ourDiff( ancestor.map, ours.map, aOurs );
    KICAD_DIFF::SYM_LIB_DIFFER theirDiff( ancestor.map, theirs.map, aTheirs );

    KICAD_DIFF::DOCUMENT_DIFF ourDocDiff = ourDiff.Diff();
    KICAD_DIFF::DOCUMENT_DIFF theirDocDiff = theirDiff.Diff();

    KICAD_DIFF::KICAD_MERGE_ENGINE engine;
    KICAD_DIFF::MERGE_PLAN         plan = engine.Plan( ourDocDiff, theirDocDiff );

    const KICAD_DIFF::MERGE_PLAN planSnapshot = plan;

    KICAD_DIFF::SYM_LIB_MERGE_APPLIER        applier( ancestor.map, ours.map, theirs.map, std::move( plan ) );
    std::vector<std::unique_ptr<LIB_SYMBOL>> merged = applier.Apply();

    // Per-property symbol merge isn't implemented; MERGE_PROPS resolutions are
    // downgraded to TAKE_OURS. Surface that as unresolved so the user sees a
    // marker instead of silent partial-merge.
    const bool hadSilentFallback = applier.GetReport().mergePropsFallback > 0;

    // Serialize via the sexpr lib cache: create at output path, add each
    // merged symbol, save. The cache owns its symbols once added.
    wxFileName outFn( aOutput );
    outFn.MakeAbsolute();

    try
    {
        SCH_IO_KICAD_SEXPR_LIB_CACHE cache( outFn.GetFullPath() );

        // SCH_IO_LIB_CACHE::AddSymbol takes ownership; the cache destructor
        // deletes the symbols it holds.
        for( auto& sym : merged )
        {
            if( sym )
                cache.AddSymbol( std::move( sym ) );
        }

        cache.SetModified( true );
        cache.Save();
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter->Report( wxString::Format( _( "Failed to save merged symbol library: %s\n" ), ioe.What() ),
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

        m_reporter->Report( wxString::Format( _( "Symbol library merge completed with %zu unresolved "
                                                 "conflict(s) in %s\n" ),
                                              conflicts.size(), aOutput ),
                            RPT_SEVERITY_WARNING );
        return CLI::EXIT_CODES::ERR_RC_VIOLATIONS;
    }

    return CLI::EXIT_CODES::SUCCESS;
}
