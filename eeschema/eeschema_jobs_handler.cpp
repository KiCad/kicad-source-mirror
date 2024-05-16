/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <jobs/job_export_sch_bom.h>
#include <jobs/job_export_sch_pythonbom.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_sch_erc.h>
#include <jobs/job_sym_export_svg.h>
#include <jobs/job_sym_upgrade.h>
#include <schematic.h>
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
#include <reporter.h>
#include <string_utils.h>

#include <settings/settings_manager.h>

#include <sch_file_versions.h>
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


EESCHEMA_JOBS_HANDLER::EESCHEMA_JOBS_HANDLER( KIWAY* aKiway ) :
        JOB_DISPATCHER( aKiway )
{
    Register( "bom",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportBom, this, std::placeholders::_1 ) );
    Register( "pythonbom",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPythonBom, this,
                         std::placeholders::_1 ) );
    Register( "netlist",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportNetlist, this, std::placeholders::_1 ) );
    Register( "plot",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPlot, this, std::placeholders::_1 ) );
    Register( "symupgrade",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSymUpgrade, this, std::placeholders::_1 ) );
    Register( "symsvg",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSymExportSvg, this, std::placeholders::_1 ) );
    Register( "erc",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSchErc, this, std::placeholders::_1 ) );
}


void EESCHEMA_JOBS_HANDLER::InitRenderSettings( SCH_RENDER_SETTINGS* aRenderSettings,
                                                const wxString& aTheme, SCHEMATIC* aSch,
                                                const wxString& aDrawingSheetOverride )
{
    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings( aTheme );
    aRenderSettings->LoadColors( cs );
    aRenderSettings->m_ShowHiddenPins = false;
    aRenderSettings->m_ShowHiddenFields = false;

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
                resolve.SetProject( &aSch->Prj() );
                resolve.SetProgramBase( &Pgm() );

                wxString absolutePath = resolve.ResolvePath( path,
                                                            wxGetCwd(),
                                                            aSch->GetEmbeddedFiles() );

                if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( absolutePath, &msg ) )
                {
                    m_reporter->Report( wxString::Format( _( "Error loading drawing sheet '%s'." ),
                                                          path )
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

    if( !aPlotJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aPlotJob->m_filename, SCH_IO_MGR::SCH_KICAD, true );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    sch->Prj().ApplyTextVars( aJob->GetVarOverrides() );

    std::unique_ptr<SCH_RENDER_SETTINGS> renderSettings = std::make_unique<SCH_RENDER_SETTINGS>();
    InitRenderSettings( renderSettings.get(), aPlotJob->m_theme, sch, aPlotJob->m_drawingSheet );

    std::unique_ptr<SCH_PLOTTER> schPlotter = std::make_unique<SCH_PLOTTER>( sch );

    PLOT_FORMAT format = PLOT_FORMAT::PDF;
    switch( aPlotJob->m_plotFormat )
    {
    case SCH_PLOT_FORMAT::DXF:    format = PLOT_FORMAT::DXF;    break;
    case SCH_PLOT_FORMAT::PDF:    format = PLOT_FORMAT::PDF;    break;
    case SCH_PLOT_FORMAT::SVG:    format = PLOT_FORMAT::SVG;    break;
    case SCH_PLOT_FORMAT::POST:   format = PLOT_FORMAT::POST;   break;
    case SCH_PLOT_FORMAT::HPGL:   format = PLOT_FORMAT::HPGL;   break;
    case SCH_PLOT_FORMAT::GERBER: format = PLOT_FORMAT::GERBER; break;
    }

    HPGL_PAGE_SIZE hpglPageSize = HPGL_PAGE_SIZE::DEFAULT;
    switch( aPlotJob->m_HPGLPaperSizeSelect )
    {
    case JOB_HPGL_PAGE_SIZE::DEFAULT: hpglPageSize = HPGL_PAGE_SIZE::DEFAULT; break;
    case JOB_HPGL_PAGE_SIZE::SIZE_A:  hpglPageSize = HPGL_PAGE_SIZE::SIZE_A;  break;
    case JOB_HPGL_PAGE_SIZE::SIZE_A0: hpglPageSize = HPGL_PAGE_SIZE::SIZE_A0; break;
    case JOB_HPGL_PAGE_SIZE::SIZE_A1: hpglPageSize = HPGL_PAGE_SIZE::SIZE_A1; break;
    case JOB_HPGL_PAGE_SIZE::SIZE_A2: hpglPageSize = HPGL_PAGE_SIZE::SIZE_A2; break;
    case JOB_HPGL_PAGE_SIZE::SIZE_A3: hpglPageSize = HPGL_PAGE_SIZE::SIZE_A3; break;
    case JOB_HPGL_PAGE_SIZE::SIZE_A4: hpglPageSize = HPGL_PAGE_SIZE::SIZE_A4; break;
    case JOB_HPGL_PAGE_SIZE::SIZE_A5: hpglPageSize = HPGL_PAGE_SIZE::SIZE_A5; break;
    case JOB_HPGL_PAGE_SIZE::SIZE_B:  hpglPageSize = HPGL_PAGE_SIZE::SIZE_B;  break;
    case JOB_HPGL_PAGE_SIZE::SIZE_C:  hpglPageSize = HPGL_PAGE_SIZE::SIZE_C;  break;
    case JOB_HPGL_PAGE_SIZE::SIZE_D:  hpglPageSize = HPGL_PAGE_SIZE::SIZE_D;  break;
    case JOB_HPGL_PAGE_SIZE::SIZE_E:  hpglPageSize = HPGL_PAGE_SIZE::SIZE_E;  break;
    }

    HPGL_PLOT_ORIGIN_AND_UNITS hpglOrigin = HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE;
    switch( aPlotJob->m_HPGLPlotOrigin )
    {
    case JOB_HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT:
        hpglOrigin = HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT;
        break;
    case JOB_HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER:
        hpglOrigin = HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER;
        break;
    case JOB_HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT:
        hpglOrigin = HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT;
        break;
    case JOB_HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE:
        hpglOrigin = HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE;
        break;
    }

    int pageSizeSelect = PageFormatReq::PAGE_SIZE_AUTO;

    switch( aPlotJob->m_pageSizeSelect )
    {
    case JOB_PAGE_SIZE::PAGE_SIZE_A:    pageSizeSelect = PageFormatReq::PAGE_SIZE_A;    break;
    case JOB_PAGE_SIZE::PAGE_SIZE_A4:   pageSizeSelect = PageFormatReq::PAGE_SIZE_A4;   break;
    case JOB_PAGE_SIZE::PAGE_SIZE_AUTO: pageSizeSelect = PageFormatReq::PAGE_SIZE_AUTO; break;
    }

    SCH_PLOT_OPTS plotOpts;
    plotOpts.m_blackAndWhite = aPlotJob->m_blackAndWhite;
    plotOpts.m_HPGLPaperSizeSelect = hpglPageSize;
    plotOpts.m_HPGLPenSize = aPlotJob->m_HPGLPenSize;
    plotOpts.m_HPGLPlotOrigin = hpglOrigin;
    plotOpts.m_PDFPropertyPopups = aPlotJob->m_PDFPropertyPopups;
    plotOpts.m_PDFMetadata = aPlotJob->m_PDFMetadata;
    plotOpts.m_outputDirectory = aPlotJob->m_outputDirectory;
    plotOpts.m_outputFile = aPlotJob->m_outputFile;
    plotOpts.m_pageSizeSelect = pageSizeSelect;
    plotOpts.m_plotAll = aPlotJob->m_plotAll;
    plotOpts.m_plotDrawingSheet = aPlotJob->m_plotDrawingSheet;
    plotOpts.m_plotPages = aPlotJob->m_plotPages;
    plotOpts.m_theme = aPlotJob->m_theme;
    plotOpts.m_useBackgroundColor = aPlotJob->m_useBackgroundColor;

    schPlotter->Plot( format, plotOpts, renderSettings.get(), m_reporter );

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportNetlist( JOB* aJob )
{
    JOB_EXPORT_SCH_NETLIST* aNetJob = dynamic_cast<JOB_EXPORT_SCH_NETLIST*>( aJob );

    if( !aNetJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aNetJob->m_filename, SCH_IO_MGR::SCH_KICAD, true );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->BuildUnorderedSheetList().GetSymbols( referenceList );

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

    if( aNetJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( fileExt );

        aNetJob->m_outputFile = fn.GetFullName();
    }

    bool res = helper->WriteNetlist( aNetJob->m_outputFile, netlistOption, *m_reporter );

    if( !res )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportBom( JOB* aJob )
{
    JOB_EXPORT_SCH_BOM* aBomJob = dynamic_cast<JOB_EXPORT_SCH_BOM*>( aJob );

    if( !aBomJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aBomJob->m_filename, SCH_IO_MGR::SCH_KICAD, true );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    sch->Prj().ApplyTextVars( aJob->GetVarOverrides() );

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->BuildUnorderedSheetList().GetSymbols( referenceList, false, false );

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
    FIELDS_EDITOR_GRID_DATA_MODEL dataModel( referenceList );

    // Mandatory fields + quantity virtual field first
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
        dataModel.AddColumn( TEMPLATE_FIELDNAME::GetDefaultFieldName( i ),
                             TEMPLATE_FIELDNAME::GetDefaultFieldName( i, true ), false );

    // User field names in symbols second
    std::set<wxString> userFieldNames;

    for( size_t i = 0; i < referenceList.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = referenceList[i].GetSymbol();

        for( int j = MANDATORY_FIELDS; j < symbol->GetFieldCount(); ++j )
            userFieldNames.insert( symbol->GetFields()[j].GetName() );
    }

    for( const wxString& fieldName : userFieldNames )
        dataModel.AddColumn( fieldName, GetTextVars( fieldName ), true );

    // Add any templateFieldNames which aren't already present in the userFieldNames
    for( const TEMPLATE_FIELDNAME& templateFieldname :
         sch->Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( userFieldNames.count( templateFieldname.m_Name ) == 0 )
        {
            dataModel.AddColumn( templateFieldname.m_Name, GetTextVars( templateFieldname.m_Name ),
                                 false );
        }
    }

    BOM_PRESET preset;

    // Load a preset if one is specified
    if( !aBomJob->m_bomPresetName.IsEmpty() )
    {
        // Make sure the built-in presets are loaded
        for( const BOM_PRESET& p : BOM_PRESET::BuiltInPresets() )
            sch->Settings().m_BomPresets.emplace_back( p );

        // Find the preset
        BOM_PRESET* schPreset = nullptr;

        for( BOM_PRESET& p : sch->Settings().m_BomPresets )
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

        for( wxString fieldName : aBomJob->m_fieldsOrdered )
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
            field.show = true;
            field.groupBy = std::find( aBomJob->m_fieldsGroupBy.begin(),
                                       aBomJob->m_fieldsGroupBy.end(), field.name )
                            != aBomJob->m_fieldsGroupBy.end();

            if( ( aBomJob->m_fieldsLabels.size() > i ) && !aBomJob->m_fieldsLabels[i].IsEmpty() )
                field.label = aBomJob->m_fieldsLabels[i];
            else if( IsTextVar( field.name ) )
                field.label = GetTextVars( field.name );
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
        preset.includeExcludedFromBOM = aBomJob->m_includeExcludedFromBOM;
    }

    dataModel.ApplyBomPreset( preset );

    if( aBomJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( FILEEXT::CsvFileExtension );

        aBomJob->m_outputFile = fn.GetFullName();
    }

    wxFile f;

    if( !f.Open( aBomJob->m_outputFile, wxFile::write ) )
    {
        m_reporter->Report( wxString::Format( _( "Unable to open destination '%s'" ),
                                              aBomJob->m_outputFile ),
                            RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    BOM_FMT_PRESET fmt;

    // Load a format preset if one is specified
    if( !aBomJob->m_bomFmtPresetName.IsEmpty() )
    {
        // Make sure the built-in presets are loaded
        for( const BOM_FMT_PRESET& p : BOM_FMT_PRESET::BuiltInPresets() )
            sch->Settings().m_BomFmtPresets.emplace_back( p );

        // Find the preset
        BOM_FMT_PRESET* schFmtPreset = nullptr;

        for( BOM_FMT_PRESET& p : sch->Settings().m_BomFmtPresets )
        {
            if( p.name == aBomJob->m_bomFmtPresetName )
            {
                schFmtPreset = &p;
                break;
            }
        }

        if( !schFmtPreset )
        {
            m_reporter->Report(
                    wxString::Format( _( "BOM format preset '%s' not found" ) + wxS( "\n" ),
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

    bool res = f.Write( dataModel.Export( fmt ) );

    if( !res )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportPythonBom( JOB* aJob )
{
    JOB_EXPORT_SCH_PYTHONBOM* aNetJob = dynamic_cast<JOB_EXPORT_SCH_PYTHONBOM*>( aJob );

    if( !aNetJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aNetJob->m_filename, SCH_IO_MGR::SCH_KICAD, true );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->BuildUnorderedSheetList().GetSymbols( referenceList );

    if( referenceList.GetCount() > 0 )
    {
        if( referenceList.CheckAnnotation(
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

    std::unique_ptr<NETLIST_EXPORTER_XML> xmlNetlist =
            std::make_unique<NETLIST_EXPORTER_XML>( sch );

    if( aNetJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() + "-bom" );
        fn.SetExt( FILEEXT::XmlFileExtension );

        aNetJob->m_outputFile = fn.GetFullName();
    }

    bool res = xmlNetlist->WriteNetlist( aNetJob->m_outputFile, GNL_OPT_BOM, *m_reporter );

    if( !res )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::doSymExportSvg( JOB_SYM_EXPORT_SVG*  aSvgJob,
                                           SCH_RENDER_SETTINGS* aRenderSettings,
                                           LIB_SYMBOL*          symbol )
{
    wxASSERT( symbol != nullptr );

    if( symbol == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    LIB_SYMBOL* symbolToPlot = symbol;

    // if the symbol is an alias, then the draw items are stored in the root symbol
    if( symbol->IsAlias() )
    {
        if(  LIB_SYMBOL_SPTR parent = symbol->GetRootSymbol() )
        {
            symbolToPlot = parent.get();
        }
        else
        {
            wxCHECK( false, CLI::EXIT_CODES::ERR_UNKNOWN );
        }
    }

    // iterate from unit 1, unit 0 would be "all units" which we don't want
    for( int unit = 1; unit < symbol->GetUnitCount() + 1; unit++ )
    {
        for( int bodyStyle = 1; bodyStyle < ( symbol->HasAlternateBodyStyle() ? 2 : 1 ) + 1; ++bodyStyle )
        {
            wxString   filename;
            wxFileName fn;
            size_t     forbidden_char;

            fn.SetPath( aSvgJob->m_outputDirectory );
            fn.SetExt( FILEEXT::SVGFileExtension );

            filename = symbol->GetName().Lower();

            while( wxString::npos
                   != ( forbidden_char = filename.find_first_of(
                                wxFileName::GetForbiddenChars( wxPATH_DOS ) ) ) )
            {
                filename = filename.replace( forbidden_char, 1, wxS( '_' ) );
            }

            //simplify the name if its single unit
            if( symbol->GetUnitCount() > 1 )
            {
                filename += wxString::Format( "_%d", unit );

                if( bodyStyle == 2 )
                    filename += wxS( "_demorgan" );

                fn.SetName( filename );
                m_reporter->Report( wxString::Format( _( "Plotting symbol '%s' unit %d to '%s'\n" ),
                                                      symbol->GetName(), unit, fn.GetFullPath() ),
                                    RPT_SEVERITY_ACTION );
            }
            else
            {
                if( bodyStyle == 2 )
                    filename += wxS( "_demorgan" );

                fn.SetName( filename );
                m_reporter->Report( wxString::Format( _( "Plotting symbol '%s' to '%s'\n" ),
                                                      symbol->GetName(), fn.GetFullPath() ),
                                    RPT_SEVERITY_ACTION );
            }

            // Get the symbol bounding box to fit the plot page to it
            BOX2I     symbolBB = symbol->Flatten()->GetUnitBoundingBox( unit, bodyStyle, !aSvgJob->m_includeHiddenFields );
            PAGE_INFO pageInfo( PAGE_INFO::Custom );
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
                m_reporter->Report(
                        wxString::Format( _( "Unable to open destination '%s'" ) + wxS( "\n" ),
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

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobSymExportSvg( JOB* aJob )
{
    JOB_SYM_EXPORT_SVG* svgJob = dynamic_cast<JOB_SYM_EXPORT_SVG*>( aJob );

    if( !svgJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

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
        wxFileName::Mkdir( svgJob->m_outputDirectory );
    }

    SCH_RENDER_SETTINGS renderSettings;
    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings( svgJob->m_colorTheme );
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

        for( const std::pair<const wxString, LIB_SYMBOL*>& entry : libSymMap )
        {
            exitCode = doSymExportSvg( svgJob, &renderSettings, entry.second );

            if( exitCode != CLI::EXIT_CODES::OK )
                break;
        }
    }

    return exitCode;
}


int EESCHEMA_JOBS_HANDLER::JobSymUpgrade( JOB* aJob )
{
    JOB_SYM_UPGRADE* upgradeJob = dynamic_cast<JOB_SYM_UPGRADE*>( aJob );

    if( !upgradeJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

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

        bool shouldSave =
                upgradeJob->m_force
                || schLibrary.GetFileFormatVersionAtLoad() < SEXPR_SYMBOL_LIB_FILE_VERSION;

        if( shouldSave )
        {
            m_reporter->Report( _( "Saving symbol library in updated format\n" ),
                                RPT_SEVERITY_ACTION );

            try
            {
                if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
                {
                    schLibrary.SetFileName( upgradeJob->m_outputLibraryPath );
                }

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
            m_reporter->Report( _( "Symbol library was not updated\n" ), RPT_SEVERITY_INFO );
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

    if( !ercJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( ercJob->m_filename, SCH_IO_MGR::SCH_KICAD, true );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    sch->Prj().ApplyTextVars( aJob->GetVarOverrides() );

    if( ercJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );

        if( ercJob->m_format == JOB_SCH_ERC::OUTPUT_FORMAT::JSON )
            fn.SetExt( FILEEXT::JsonFileExtension );
        else
            fn.SetExt( FILEEXT::ReportFileExtension );

        ercJob->m_outputFile = fn.GetFullName();
    }

    EDA_UNITS units;

    switch( ercJob->m_units )
    {
    case JOB_SCH_ERC::UNITS::INCHES:      units = EDA_UNITS::INCHES;      break;
    case JOB_SCH_ERC::UNITS::MILS:        units = EDA_UNITS::MILS;        break;
    case JOB_SCH_ERC::UNITS::MILLIMETERS: units = EDA_UNITS::MILLIMETRES; break;
    default:                              units = EDA_UNITS::MILLIMETRES; break;
    }

    std::shared_ptr<SHEETLIST_ERC_ITEMS_PROVIDER> markersProvider =
            std::make_shared<SHEETLIST_ERC_ITEMS_PROVIDER>( sch );

    ERC_TESTER ercTester( sch );

    m_reporter->Report( _( "Running ERC...\n" ), RPT_SEVERITY_INFO );

    std::unique_ptr<DS_PROXY_VIEW_ITEM> drawingSheet( getDrawingSheetProxyView( sch ) );
    ercTester.RunTests( drawingSheet.get(), nullptr, m_kiway->KiFACE( KIWAY::FACE_CVPCB ),
                        &sch->Prj(), m_progressReporter );

    markersProvider->SetSeverities( ercJob->m_severity );

    m_reporter->Report( wxString::Format( _( "Found %d violations\n" ),
                                          markersProvider->GetCount() ),
                        RPT_SEVERITY_INFO );

    ERC_REPORT reportWriter( sch, units );

    bool wroteReport = false;

    if( ercJob->m_format == JOB_SCH_ERC::OUTPUT_FORMAT::JSON )
        wroteReport = reportWriter.WriteJsonReport( ercJob->m_outputFile );
    else
        wroteReport = reportWriter.WriteTextReport( ercJob->m_outputFile );

    if( !wroteReport )
    {
        m_reporter->Report( wxString::Format( _( "Unable to save ERC report to %s\n" ),
                                              ercJob->m_outputFile ),
                            RPT_SEVERITY_INFO );
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    m_reporter->Report( wxString::Format( _( "Saved ERC Report to %s\n" ), ercJob->m_outputFile ),
                        RPT_SEVERITY_INFO );

    if( ercJob->m_exitCodeViolations )
    {
        if( markersProvider->GetCount() > 0 )
            return CLI::EXIT_CODES::ERR_RC_VIOLATIONS;
    }

    return CLI::EXIT_CODES::SUCCESS;
}


DS_PROXY_VIEW_ITEM* EESCHEMA_JOBS_HANDLER::getDrawingSheetProxyView( SCHEMATIC* aSch )
{
    DS_PROXY_VIEW_ITEM* drawingSheet =
            new DS_PROXY_VIEW_ITEM( schIUScale, &aSch->RootScreen()->GetPageSettings(),
                                    &aSch->Prj(), &aSch->RootScreen()->GetTitleBlock(),
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
