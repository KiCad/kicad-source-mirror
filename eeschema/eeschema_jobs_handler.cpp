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
#include <jobs/job_export_sch_bom.h>
#include <jobs/job_export_sch_pythonbom.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_sym_export_svg.h>
#include <jobs/job_sym_upgrade.h>
#include <schematic.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <memory>
#include <connection_graph.h>
#include "eeschema_helpers.h"
#include <sch_painter.h>
#include <locale_io.h>
#include <erc.h>
#include <wildcards_and_files_ext.h>
#include <plotters/plotters_pslike.h>
#include <drawing_sheet/ds_data_model.h>
#include <reporter.h>

#include <settings/settings_manager.h>

#include <sch_file_versions.h>
#include <sch_plugins/kicad/sch_sexpr_lib_plugin_cache.h>

#include <netlist.h>
#include <netlist_exporter_base.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_spice.h>
#include <netlist_exporter_spice_model.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_xml.h>

#include <fields_data_model.h>


EESCHEMA_JOBS_HANDLER::EESCHEMA_JOBS_HANDLER()
{
    Register( "bom",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportBom, this, std::placeholders::_1 ) );
    Register( "pythonbom",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPythonBom, this, std::placeholders::_1 ) );
    Register( "netlist",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportNetlist, this, std::placeholders::_1 ) );
    Register( "plot",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPlot, this, std::placeholders::_1 ) );
    Register( "symupgrade",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSymUpgrade, this, std::placeholders::_1 ) );
    Register( "symsvg",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSymExportSvg, this, std::placeholders::_1 ) );
}


void EESCHEMA_JOBS_HANDLER::InitRenderSettings( KIGFX::SCH_RENDER_SETTINGS* aRenderSettings,
                                                const wxString& aTheme, SCHEMATIC* aSch )
{
    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings( aTheme );
    aRenderSettings->LoadColors( cs );

    aRenderSettings->SetDefaultPenWidth( aSch->Settings().m_DefaultLineWidth );
    aRenderSettings->m_LabelSizeRatio = aSch->Settings().m_LabelSizeRatio;
    aRenderSettings->m_TextOffsetRatio = aSch->Settings().m_TextOffsetRatio;
    aRenderSettings->m_PinSymbolSize = aSch->Settings().m_PinSymbolSize;

    aRenderSettings->SetDashLengthRatio( aSch->Settings().m_DashedLineDashRatio );
    aRenderSettings->SetGapLengthRatio( aSch->Settings().m_DashedLineGapRatio );

    // Load the drawing sheet from the filename stored in BASE_SCREEN::m_DrawingSheetFileName.
    // If empty, or not existing, the default drawing sheet is loaded.
    wxString filename = DS_DATA_MODEL::ResolvePath( aSch->Settings().m_SchDrawingSheetFileName,
                                                    aSch->Prj().GetProjectPath() );

    if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename ) )
        m_reporter->Report( _( "Error loading drawing sheet." ), RPT_SEVERITY_ERROR );
}


int EESCHEMA_JOBS_HANDLER::JobExportPlot( JOB* aJob )
{
    JOB_EXPORT_SCH_PLOT* aPlotJob = dynamic_cast<JOB_EXPORT_SCH_PLOT*>( aJob );

    if( !aPlotJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aPlotJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    std::unique_ptr<KIGFX::SCH_RENDER_SETTINGS> renderSettings =
            std::make_unique<KIGFX::SCH_RENDER_SETTINGS>();
    InitRenderSettings( renderSettings.get(), aPlotJob->settings.m_theme, sch );

    std::unique_ptr<SCH_PLOTTER> schPlotter = std::make_unique<SCH_PLOTTER>( sch );
    schPlotter->Plot( aPlotJob->m_plotFormat, aPlotJob->settings, renderSettings.get(), m_reporter );

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportNetlist( JOB* aJob )
{
    JOB_EXPORT_SCH_NETLIST* aNetJob = dynamic_cast<JOB_EXPORT_SCH_NETLIST*>( aJob );

    if( !aNetJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aNetJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->GetSheets().GetSymbols( referenceList );

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
    {
        m_reporter->Report( _( "Warning: duplicate sheet names.\n" ), RPT_SEVERITY_WARNING );
    }


    std::unique_ptr<NETLIST_EXPORTER_BASE> helper;
    unsigned netlistOption = 0;

    wxString fileExt;

    switch( aNetJob->format )
    {
    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADSEXPR:
        fileExt = NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_KICAD>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::ORCADPCB2:
        fileExt = OrCadPcb2NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_ORCADPCB2>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::CADSTAR:
        fileExt = CadstarNetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_CADSTAR>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICE:
        fileExt = SpiceFileExtension;
        netlistOption = NETLIST_EXPORTER_SPICE::OPTION_SIM_COMMAND;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICEMODEL:
        fileExt = SpiceFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE_MODEL>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADXML:
        fileExt = wxS( "xml" );
        helper = std::make_unique<NETLIST_EXPORTER_XML>( sch );
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

    if(!res)
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportBom( JOB* aJob )
{
    JOB_EXPORT_SCH_BOM* aBomJob = dynamic_cast<JOB_EXPORT_SCH_BOM*>( aJob );

    if( !aBomJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aBomJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->GetSheets().GetSymbols( referenceList, false, false );

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
    {
        m_reporter->Report( _( "Warning: duplicate sheet names.\n" ), RPT_SEVERITY_WARNING );
    }


    // Build our data model
    FIELDS_EDITOR_GRID_DATA_MODEL dataModel( referenceList );

    // Mandatory fields + quantity virtual field first
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
        dataModel.AddColumn( TEMPLATE_FIELDNAME::GetDefaultFieldName( i ),
                             TEMPLATE_FIELDNAME::GetDefaultFieldName( i, true ), false );

    dataModel.AddColumn( wxS( "Quantity" ), _( "Qty" ), false );

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
            dataModel.AddColumn( templateFieldname.m_Name, GetTextVars( templateFieldname.m_Name ),
                                 false );
    }

    BOM_PRESET preset;

    size_t i = 0;
    for( wxString fieldName : aBomJob->m_fieldsOrdered )
    {
        struct BOM_FIELD field;

        field.name = fieldName;
        field.show = true;
        field.groupBy = std::find( aBomJob->m_fieldsGroupBy.begin(), aBomJob->m_fieldsGroupBy.end(),
                                   field.name )
                        != aBomJob->m_fieldsGroupBy.end();
        field.label =
                ( ( aBomJob->m_fieldsLabels.size() > i ) && !aBomJob->m_fieldsLabels[i].IsEmpty() )
                        ? aBomJob->m_fieldsLabels[i]
                        : field.name;

        preset.fieldsOrdered.emplace_back( field );
        i++;
    }

    preset.sortAsc = aBomJob->m_sortAsc;
    preset.sortField = aBomJob->m_sortField;
    preset.filterString = aBomJob->m_filterString;
    preset.groupSymbols = ( aBomJob->m_fieldsGroupBy.size() > 0 );
    preset.excludeDNP = aBomJob->m_excludeDNP;

    dataModel.ApplyBomPreset( preset );

    if( aBomJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( CsvFileExtension );

        aBomJob->m_outputFile = fn.GetFullName();
    }

    wxFile f;
    if( !f.Open( aBomJob->m_outputFile, wxFile::write ) )
    {
        m_reporter->Report(
                wxString::Format( _( "Unable to open destination '%s'" ), aBomJob->m_outputFile ),
                RPT_SEVERITY_ERROR
        );

        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    BOM_FMT_PRESET fmt;
    fmt.fieldDelimiter = aBomJob->m_fieldDelimiter;
    fmt.stringDelimiter = aBomJob->m_stringDelimiter;
    fmt.refDelimiter = aBomJob->m_refDelimiter;
    fmt.refRangeDelimiter = aBomJob->m_refRangeDelimiter;
    fmt.keepTabs = aBomJob->m_keepTabs;
    fmt.keepLineBreaks = aBomJob->m_keepLineBreaks;

    bool res = f.Write( dataModel.Export( fmt ) );

    if( !res )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportPythonBom( JOB* aJob )
{
    JOB_EXPORT_SCH_PYTHONBOM* aNetJob = dynamic_cast<JOB_EXPORT_SCH_PYTHONBOM*>( aJob );

    if( !aNetJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aNetJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        m_reporter->Report( _( "Failed to load schematic file\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->GetSheets().GetSymbols( referenceList );

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
        fn.SetExt( XmlFileExtension );

        aNetJob->m_outputFile = fn.GetFullName();
    }

    bool res = xmlNetlist->WriteNetlist( aNetJob->m_outputFile, GNL_OPT_BOM, *m_reporter );

    if( !res )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::doSymExportSvg( JOB_SYM_EXPORT_SVG*         aSvgJob,
                                           KIGFX::SCH_RENDER_SETTINGS* aRenderSettings,
                                           LIB_SYMBOL*                 symbol )
{
    wxASSERT( symbol != nullptr );

    if( symbol == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    LIB_SYMBOL* symbolToPlot = symbol;

    // if the symbol is an alias, then the draw items are stored in the parent
    if( symbol->IsAlias() )
    {
        LIB_SYMBOL_SPTR parent = symbol->GetParent().lock();
        symbolToPlot = parent.get();
    }

    if( aSvgJob->m_includeHiddenPins )
    {
        // horrible hack, TODO overhaul the Plot method to handle this
        for( LIB_ITEM& item : symbolToPlot->GetDrawItems() )
        {
            if( item.Type() != LIB_PIN_T )
                continue;

            LIB_PIN& pin = static_cast<LIB_PIN&>( item );
            pin.SetVisible( true );
        }
    }

    // iterate from unit 1, unit 0 would be "all units" which we don't want
    for( int unit = 1; unit < symbol->GetUnitCount() + 1; unit++ )
    {
        for( int convert = 1; convert < ( symbol->HasConversion() ? 2 : 1 ) + 1; ++convert )
        {
            wxString   filename;
            wxFileName fn;

            fn.SetPath( aSvgJob->m_outputDirectory );
            fn.SetExt( SVGFileExtension );

            //simplify the name if its single unit
            if( symbol->IsMulti() )
            {
                filename = wxString::Format( "%s_%d", symbol->GetName().Lower(), unit );

                if( convert == 2 )
                    filename += wxS( "_demorgan" );

                fn.SetName( filename );
                m_reporter->Report( wxString::Format( _( "Plotting symbol '%s' unit %d to '%s'\n" ),
                                                      symbol->GetName(), unit, fn.GetFullPath() ),
                                    RPT_SEVERITY_ACTION );
            }
            else
            {
                filename = symbol->GetName().Lower();

                if( convert == 2 )
                    filename += wxS( "_demorgan" );

                fn.SetName( filename );
                m_reporter->Report( wxString::Format( _( "Plotting symbol '%s' to '%s'\n" ),
                                                      symbol->GetName(), fn.GetFullPath() ),
                                    RPT_SEVERITY_ACTION );
            }

            // Get the symbol bounding box to fit the plot page to it
            BOX2I     symbolBB = symbol->Flatten()->GetUnitBoundingBox( unit, convert, false );
            PAGE_INFO pageInfo( PAGE_INFO::Custom );
            pageInfo.SetHeightMils( schIUScale.IUToMils( symbolBB.GetHeight() * 1.2 ) );
            pageInfo.SetWidthMils( schIUScale.IUToMils( symbolBB.GetWidth() * 1.2 ) );

            SVG_PLOTTER* plotter = new SVG_PLOTTER();
            plotter->SetRenderSettings( aRenderSettings );
            plotter->SetPageSettings( pageInfo );
            plotter->SetColorMode( !aSvgJob->m_blackAndWhite );

            VECTOR2I     plot_offset;
            const double scale = 1.0;

            // Currently, plot units are in decimil
            plotter->SetViewport( plot_offset, schIUScale.IU_PER_MILS / 10, scale, false );

            plotter->SetCreator( wxT( "Eeschema-SVG" ) );

            if( !plotter->OpenFile( fn.GetFullPath() ) )
            {
                m_reporter->Report( wxString::Format( _( "Unable to open destination '%s'" ),
                                                      fn.GetFullPath() ),
                                    RPT_SEVERITY_ERROR );

                delete plotter;
                return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
            }

            LOCALE_IO toggle;

            plotter->StartPlot( wxT( "1" ) );

            bool      background = true;
            TRANSFORM temp; // Uses default transform
            VECTOR2I  plotPos;

            plotPos.x = pageInfo.GetWidthIU( schIUScale.IU_PER_MILS ) / 2;
            plotPos.y = pageInfo.GetHeightIU( schIUScale.IU_PER_MILS ) / 2;

            // note, we want the fields from the original symbol pointer (in case of non-alias)
            symbolToPlot->Plot( plotter, unit, convert, background, plotPos, temp, false );
            symbol->PlotLibFields( plotter, unit, convert, background, plotPos, temp, false,
                                   aSvgJob->m_includeHiddenFields );

            symbolToPlot->Plot( plotter, unit, convert, !background, plotPos, temp, false );
            symbol->PlotLibFields( plotter, unit, convert, !background, plotPos, temp, false,
                                   aSvgJob->m_includeHiddenFields );

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

    SCH_SEXPR_PLUGIN_CACHE schLibrary( fn.GetFullPath() );

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
            m_reporter->Report( _( "There is no symbol selected to save." ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_ARGS;
        }
    }

    if( !svgJob->m_outputDirectory.IsEmpty() && !wxDir::Exists( svgJob->m_outputDirectory ) )
    {
        wxFileName::Mkdir( svgJob->m_outputDirectory );
    }

    KIGFX::SCH_RENDER_SETTINGS renderSettings;
    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings( svgJob->m_colorTheme );
    renderSettings.LoadColors( cs );
    renderSettings.SetDefaultPenWidth( DEFAULT_LINE_WIDTH_MILS * schIUScale.IU_PER_MILS );

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

    SCH_SEXPR_PLUGIN_CACHE schLibrary( fn.GetFullPath() );

    try
    {
        schLibrary.Load();
    }
    catch( ... )
    {
        m_reporter->Report( _( "Unable to load library\n" ), RPT_SEVERITY_ERROR );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
    {
        if( wxFile::Exists( upgradeJob->m_outputLibraryPath ) )
        {
            m_reporter->Report( _( "Output path must not conflict with existing path\n" ),
                                RPT_SEVERITY_ERROR );

            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    bool shouldSave = upgradeJob->m_force
                      || schLibrary.GetFileFormatVersionAtLoad() < SEXPR_SYMBOL_LIB_FILE_VERSION;

    if( shouldSave )
    {
        m_reporter->Report( _( "Saving symbol library in updated format\n" ), RPT_SEVERITY_ACTION );

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

    return CLI::EXIT_CODES::OK;
}
