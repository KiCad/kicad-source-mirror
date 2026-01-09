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

#include "eeschema_jobs_handler.h"
#include <common.h>
#include <pgm_base.h>
#include <cli/exit_codes.h>
#include <sch_plotter.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <font/kicad_font_name.h>
#include <jobs/job_export_sch_bom.h>
#include <jobs/job_export_sch_pythonbom.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_sch_erc.h>
#include <jobs/job_sch_upgrade.h>
#include <jobs/job_sym_export_svg.h>
#include <jobs/job_sym_upgrade.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <sch_screen.h>
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
#include <wildcards_and_files_ext.h>
#include <plotters/plotters_pslike.h>
#include <drawing_sheet/ds_data_model.h>
#include <paths.h>
#include <reporter.h>
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

#include <fields_data_model.h>

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
    Register( "bom",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportBom, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
	              JOB_EXPORT_SCH_BOM* bomJob = dynamic_cast<JOB_EXPORT_SCH_BOM*>( job );

                  SCH_EDIT_FRAME* editFrame =
                          static_cast<SCH_EDIT_FRAME*>( aKiway->Player( FRAME_SCH, false ) );

                  wxCHECK( bomJob && editFrame, false );

                  DIALOG_SYMBOL_FIELDS_TABLE dlg( editFrame, bomJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "pythonbom",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPythonBom, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "netlist",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportNetlist, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_SCH_NETLIST* netJob = dynamic_cast<JOB_EXPORT_SCH_NETLIST*>( job );

                  SCH_EDIT_FRAME* editFrame =
                          static_cast<SCH_EDIT_FRAME*>( aKiway->Player( FRAME_SCH, false ) );

                  wxCHECK( netJob && editFrame, false );

                  DIALOG_EXPORT_NETLIST dlg( editFrame, aParent, netJob );
                  return dlg.ShowModal() == wxID_OK;
              } );
    Register( "plot",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPlot, this, std::placeholders::_1 ),
              [aKiway]( JOB* job, wxWindow* aParent ) -> bool
              {
                  JOB_EXPORT_SCH_PLOT* plotJob = dynamic_cast<JOB_EXPORT_SCH_PLOT*>( job );

                  SCH_EDIT_FRAME* editFrame =
                          static_cast<SCH_EDIT_FRAME*>( aKiway->Player( FRAME_SCH, false ) );

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
    Register( "symupgrade",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSymUpgrade, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
    Register( "symsvg",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSymExportSvg, this, std::placeholders::_1 ),
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
}


SCHEMATIC* EESCHEMA_JOBS_HANDLER::getSchematic( const wxString& aPath )
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

        if( !m_cliSchematic )
            m_cliSchematic = EESCHEMA_HELPERS::LoadSchematic( schPath, true, false, &project );

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
        sch = EESCHEMA_HELPERS::LoadSchematic( aPath, true, false );
    }

    if( !sch )
        m_reporter->Report( _( "Failed to load schematic\n" ), RPT_SEVERITY_ERROR );

    return sch;
}

void EESCHEMA_JOBS_HANDLER::InitRenderSettings( SCH_RENDER_SETTINGS* aRenderSettings,
                                                const wxString& aTheme, SCHEMATIC* aSch,
                                                const wxString& aDrawingSheetOverride )
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

    aRenderSettings->SetDashLengthRatio( aSch->Settings().m_DashedLineDashRatio );
    aRenderSettings->SetGapLengthRatio( aSch->Settings().m_DashedLineGapRatio );

    // Load the drawing sheet from the filename stored in BASE_SCREEN::m_DrawingSheetFileName.
    // If empty, or not existing, the default drawing sheet is loaded.

    auto loadSheet =
            [&]( const wxString& path ) -> bool
            {
                wxString msg;
                FILENAME_RESOLVER resolve;
                resolve.SetProject( &aSch->Project() );
                resolve.SetProgramBase( &Pgm() );

                wxString absolutePath = resolve.ResolvePath( path, wxGetCwd(),
                                                             { aSch->GetEmbeddedFiles() } );

                if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( absolutePath, &msg ) )
                {
                    m_reporter->Report( wxString::Format( _( "Error loading drawing sheet '%s'." ), path )
                                            + wxS( "\n" ) + msg + wxS( "\n" ),
                                        RPT_SEVERITY_ERROR );
                    return false;
                }

                return true;
            };

    // try to load the override first
    if( !aDrawingSheetOverride.IsEmpty() && loadSheet( aDrawingSheetOverride ) )
        return;

    // no override or failed override continues here
    loadSheet( aSch->Settings().m_SchDrawingSheetFileName );
}


int EESCHEMA_JOBS_HANDLER::JobExportPlot( JOB* aJob )
{
    JOB_EXPORT_SCH_PLOT* aPlotJob = dynamic_cast<JOB_EXPORT_SCH_PLOT*>( aJob );

    wxCHECK( aPlotJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    if( aPlotJob->m_plotFormat == SCH_PLOT_FORMAT::HPGL )
    {
        m_reporter->Report( _( "Plotting to HPGL is no longer supported as of KiCad 10.0.\n" ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    SCHEMATIC* sch = getSchematic( aPlotJob->m_filename );

    if( !sch )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( sch->RootScreen()->GetTitleBlock() );
    sch->Project().ApplyTextVars( aJob->GetVarOverrides() );

    // Apply variant if specified
    if( !aPlotJob->m_variantNames.empty() )
    {
        // For plot export, we use the first variant name from the set
        wxString variantName = *aPlotJob->m_variantNames.begin();

        if( variantName != wxS( "all" ) )
            sch->SetCurrentVariant( variantName );
    }

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
    case SCH_PLOT_FORMAT::DXF:    format = PLOT_FORMAT::DXF;    break;
    case SCH_PLOT_FORMAT::PDF:    format = PLOT_FORMAT::PDF;    break;
    case SCH_PLOT_FORMAT::SVG:    format = PLOT_FORMAT::SVG;    break;
    case SCH_PLOT_FORMAT::POST:   format = PLOT_FORMAT::POST;   break;
    case SCH_PLOT_FORMAT::HPGL:   /* no longer supported */     break;
    }

    int pageSizeSelect = PageFormatReq::PAGE_SIZE_AUTO;

    switch( aPlotJob->m_pageSizeSelect )
    {
    case JOB_PAGE_SIZE::PAGE_SIZE_A:    pageSizeSelect = PageFormatReq::PAGE_SIZE_A;    break;
    case JOB_PAGE_SIZE::PAGE_SIZE_A4:   pageSizeSelect = PageFormatReq::PAGE_SIZE_A4;   break;
    case JOB_PAGE_SIZE::PAGE_SIZE_AUTO: pageSizeSelect = PageFormatReq::PAGE_SIZE_AUTO; break;
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
    plotOpts.m_theme = aPlotJob->m_theme;
    plotOpts.m_useBackgroundColor = aPlotJob->m_useBackgroundColor;
    plotOpts.m_plotHopOver = aPlotJob->m_show_hop_over;

    // Use variant from m_variantNames if specified, otherwise use the schematic's current variant
    if( !aPlotJob->m_variantNames.empty() )
        plotOpts.m_variant = aPlotJob->m_variantNames.front();

    // Always export dxf in mm by kicad-cli (similar to Pcbnew)
    plotOpts.m_DXF_File_Unit = DXF_UNITS::MM;

    schPlotter->Plot( format, plotOpts, renderSettings.get(), m_reporter );

    if( m_reporter->HasMessageOfSeverity( RPT_SEVERITY_ERROR ) )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

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
    sch->Hierarchy().GetSymbols( referenceList );

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
    unsigned netlistOption = 0;

    wxString fileExt;

    switch( aNetJob->format )
    {
    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADSEXPR:
        fileExt = FILEEXT::NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_KICAD>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::ORCADPCB2:
        fileExt = FILEEXT::OrCadPcb2NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_ORCADPCB2>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::CADSTAR:
        fileExt = FILEEXT::CadstarNetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_CADSTAR>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICE:
        fileExt = FILEEXT::SpiceFileExtension;
        netlistOption = NETLIST_EXPORTER_SPICE::OPTION_SIM_COMMAND;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICEMODEL:
        fileExt = FILEEXT::SpiceFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE_MODEL>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADXML:
        fileExt = wxS( "xml" );
        helper = std::make_unique<NETLIST_EXPORTER_XML>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::PADS:
        fileExt = wxS( "asc" );
        helper = std::make_unique<NETLIST_EXPORTER_PADS>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::ALLEGRO:
        fileExt = wxS( "txt" );
        helper = std::make_unique<NETLIST_EXPORTER_ALLEGRO>( sch );
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

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportBom( JOB* aJob )
{
    JOB_EXPORT_SCH_BOM* aBomJob = dynamic_cast<JOB_EXPORT_SCH_BOM*>( aJob );

    wxCHECK( aBomJob, CLI::EXIT_CODES::ERR_UNKNOWN );

    SCHEMATIC* sch = getSchematic( aBomJob->m_filename );

    if( !sch )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    aJob->SetTitleBlock( sch->RootScreen()->GetTitleBlock() );
    sch->Project().ApplyTextVars( aJob->GetVarOverrides() );

    wxString currentVariant;

    if( !aBomJob->m_variantNames.empty() )
    {
        currentVariant = aBomJob->m_variantNames.front();

        if( currentVariant != wxS( "all" ) )
            sch->SetCurrentVariant( currentVariant );
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->Hierarchy().GetSymbols( referenceList, false, false );

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
            m_reporter->Report(
                    _( "Warning: schematic has annotation errors, please use the schematic "
                       "editor to fix them\n" ),
                    RPT_SEVERITY_WARNING );
        }
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( sch );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
        m_reporter->Report( _( "Warning: duplicate sheet names.\n" ), RPT_SEVERITY_WARNING );

    // Build our data model
    FIELDS_EDITOR_GRID_DATA_MODEL dataModel( referenceList, nullptr );

    // Mandatory fields first
    for( FIELD_T fieldId : MANDATORY_FIELDS )
    {
        dataModel.AddColumn( GetCanonicalFieldName( fieldId ),
                             GetDefaultFieldName( fieldId, DO_TRANSLATE ), false, currentVariant );
    }

    // Generated/virtual fields (e.g. ${QUANTITY}, ${ITEM_NUMBER}) present only in the fields table
    dataModel.AddColumn( FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE,
                         GetGeneratedFieldDisplayName( FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE ),
                         false, currentVariant );
    dataModel.AddColumn( FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE,
                         GetGeneratedFieldDisplayName( FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE ),
                         false, currentVariant );

    // Attribute fields (boolean flags on symbols)
    dataModel.AddColumn( wxS( "${DNP}" ), GetGeneratedFieldDisplayName( wxS( "${DNP}" ) ),
                         false, currentVariant );
    dataModel.AddColumn( wxS( "${EXCLUDE_FROM_BOM}" ), GetGeneratedFieldDisplayName( wxS( "${EXCLUDE_FROM_BOM}" ) ),
                         false, currentVariant );
    dataModel.AddColumn( wxS( "${EXCLUDE_FROM_BOARD}" ), GetGeneratedFieldDisplayName( wxS( "${EXCLUDE_FROM_BOARD}" ) ),
                         false, currentVariant );
    dataModel.AddColumn( wxS( "${EXCLUDE_FROM_SIM}" ), GetGeneratedFieldDisplayName( wxS( "${EXCLUDE_FROM_SIM}" ) ),
                         false, currentVariant );

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
        dataModel.AddColumn( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, currentVariant );

    // Add any templateFieldNames which aren't already present in the userFieldNames
    for( const TEMPLATE_FIELDNAME& templateFieldname :
         sch->Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( userFieldNames.count( templateFieldname.m_Name ) == 0 )
        {
            dataModel.AddColumn( templateFieldname.m_Name, GetGeneratedFieldDisplayName( templateFieldname.m_Name ),
                                 false, currentVariant );
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
            m_reporter->Report( wxString::Format( _( "BOM preset '%s' not found" ) + wxS( "\n" ),
                                                  aBomJob->m_bomPresetName ),
                                RPT_SEVERITY_ERROR );

            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        preset = *schPreset;
    }
    else
    {
        size_t i = 0;

        for( const wxString& fieldName : aBomJob->m_fieldsOrdered )
        {
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
                        if( fieldInList == field.name )
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
            field.groupBy = alg::contains( aBomJob->m_fieldsGroupBy, field.name );

            if( ( aBomJob->m_fieldsLabels.size() > i ) && !aBomJob->m_fieldsLabels[i].IsEmpty() )
                field.label = aBomJob->m_fieldsLabels[i];
            else if( IsGeneratedField( field.name ) )
                field.label = GetGeneratedFieldDisplayName( field.name );
            else
                field.label = field.name;

            preset.fieldsOrdered.emplace_back( field );
            i++;
        }

        preset.sortAsc = aBomJob->m_sortAsc;
        preset.sortField = aBomJob->m_sortField;
        preset.filterString = aBomJob->m_filterString;
        preset.groupSymbols = ( aBomJob->m_fieldsGroupBy.size() > 0 );
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
        dataModel.ApplyBomPreset( preset, variantName );

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

        m_reporter->Report( wxString::Format( _( "Wrote bill of materials to '%s'." ), outPath ),
                            RPT_SEVERITY_ACTION );
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
    sch->Hierarchy().GetSymbols( referenceList );

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

    std::unique_ptr<NETLIST_EXPORTER_XML> xmlNetlist =
            std::make_unique<NETLIST_EXPORTER_XML>( sch );

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

    m_reporter->Report( wxString::Format( _( "Wrote bill of materials to '%s'." ), outPath ),
                        RPT_SEVERITY_ACTION );

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
                filename += wxS( "_" ) + symbol->GetBodyStyleNames()[bodyStyle-1].Lower();
            }

            fn.SetName( filename );
            m_reporter->Report( wxString::Format( _( "Plotting symbol '%s' unit %d to '%s'\n" ),
                                                  symbol->GetName(),
                                                  unit,
                                                  fn.GetFullPath() ),
                                RPT_SEVERITY_ACTION );

            // Get the symbol bounding box to fit the plot page to it
            BOX2I symbolBB = symbol->Flatten()->GetUnitBoundingBox( unit, bodyStyle,
                                                                    !aSvgJob->m_includeHiddenFields );
            PAGE_INFO pageInfo( PAGE_SIZE_TYPE::User );
            pageInfo.SetHeightMils( schIUScale.IUToMils( symbolBB.GetHeight() * 1.2 ) );
            pageInfo.SetWidthMils( schIUScale.IUToMils( symbolBB.GetWidth() * 1.2 ) );

            SVG_PLOTTER* plotter = new SVG_PLOTTER();
            plotter->SetRenderSettings( aRenderSettings );
            plotter->SetPageSettings( pageInfo );
            plotter->SetColorMode( !aSvgJob->m_blackAndWhite );

            VECTOR2I     plot_offset = symbolBB.GetCenter();
            const double scale = 1.0;

            // Currently, plot units are in decimal
            plotter->SetViewport( plot_offset, schIUScale.IU_PER_MILS / 10, scale, false );

            plotter->SetCreator( wxT( "Eeschema-SVG" ) );

            if( !plotter->OpenFile( fn.GetFullPath() ) )
            {
                m_reporter->Report( wxString::Format( _( "Unable to open destination '%s'" ) + wxS( "\n" ),
                                                      fn.GetFullPath() ),
                                    RPT_SEVERITY_ERROR );

                delete plotter;
                return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
            }

            LOCALE_IO     toggle;
            SCH_PLOT_OPTS plotOpts;

            plotter->StartPlot( wxT( "1" ) );

            bool     background = true;
            VECTOR2I offset( pageInfo.GetWidthIU( schIUScale.IU_PER_MILS ) / 2,
                             pageInfo.GetHeightIU( schIUScale.IU_PER_MILS ) / 2 );

            // note, we want the fields from the original symbol pointer (in case of non-alias)
            symbolToPlot->Plot( plotter, background, plotOpts, unit, bodyStyle, offset, false );
            symbol->PlotFields( plotter, background, plotOpts, unit, bodyStyle, offset, false );

            symbolToPlot->Plot( plotter, !background, plotOpts, unit, bodyStyle, offset, false );
            symbol->PlotFields( plotter, !background, plotOpts, unit, bodyStyle, offset, false );

            plotter->EndPlot();
            delete plotter;
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

    LIB_SYMBOL* symbol = nullptr;

    if( !svgJob->m_symbol.IsEmpty() )
    {
        // See if the selected symbol exists
        symbol = schLibrary.GetSymbol( svgJob->m_symbol );

        if( !symbol )
        {
            m_reporter->Report( _( "There is no symbol selected to save." ) + wxS( "\n" ),
                                RPT_SEVERITY_ERROR );
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
    COLOR_SETTINGS* cs = ::GetColorSettings( svgJob->m_colorTheme );
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
        const LIB_SYMBOL_MAP& libSymMap = schLibrary.GetSymbolMap();

        for( const auto& [name, libSymbol] : libSymMap )
        {
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
            m_reporter->Report( _( "Output path must not conflict with existing path\n" ),
                                RPT_SEVERITY_ERROR );

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
        if( !SCH_IO_MGR::ConvertLibrary( nullptr, fn.GetAbsolutePath(), upgradeJob->m_outputLibraryPath ) )
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

        ercJob->SetConfiguredOutputPath( fn.GetFullName() );
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
    case JOB_SCH_ERC::UNITS::MM:   units = EDA_UNITS::MM;   break;
    default:                       units = EDA_UNITS::MM;   break;
    }

    std::shared_ptr<SHEETLIST_ERC_ITEMS_PROVIDER> markersProvider =
            std::make_shared<SHEETLIST_ERC_ITEMS_PROVIDER>( sch );

    // Running ERC requires libraries be loaded, so make sure they have been
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &sch->Project() );
    adapter->AsyncLoad();
    adapter->BlockUntilLoaded();

    ERC_TESTER ercTester( sch );

    std::unique_ptr<DS_PROXY_VIEW_ITEM> drawingSheet( getDrawingSheetProxyView( sch ) );
    ercTester.RunTests( drawingSheet.get(), nullptr, m_kiway->KiFACE( KIWAY::FACE_CVPCB ),
                        &sch->Project(), m_progressReporter );

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
        m_reporter->Report( wxString::Format( _( "Unable to save ERC report to %s\n" ), outPath ),
                            RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    m_reporter->Report( wxString::Format( _( "Saved ERC Report to %s\n" ), outPath ),
                        RPT_SEVERITY_ACTION );

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

    SCHEMATIC* sch = getSchematic( aUpgradeJob->m_filename );

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


DS_PROXY_VIEW_ITEM* EESCHEMA_JOBS_HANDLER::getDrawingSheetProxyView( SCHEMATIC* aSch )
{
    DS_PROXY_VIEW_ITEM* drawingSheet =
            new DS_PROXY_VIEW_ITEM( schIUScale, &aSch->RootScreen()->GetPageSettings(),
                                    &aSch->Project(), &aSch->RootScreen()->GetTitleBlock(),
                                    aSch->GetProperties() );

    drawingSheet->SetPageNumber( TO_UTF8( aSch->RootScreen()->GetPageNumber() ) );
    drawingSheet->SetSheetCount( aSch->RootScreen()->GetPageCount() );
    drawingSheet->SetFileName( TO_UTF8( aSch->RootScreen()->GetFileName() ) );
    drawingSheet->SetColorLayer( LAYER_SCHEMATIC_DRAWINGSHEET );
    drawingSheet->SetPageBorderColorLayer( LAYER_SCHEMATIC_PAGE_LIMITS );
    drawingSheet->SetIsFirstPage( aSch->RootScreen()->GetVirtualPageNumber() == 1 );

    drawingSheet->SetSheetName( "" );
    drawingSheet->SetSheetPath( "" );

    return drawingSheet;
}
