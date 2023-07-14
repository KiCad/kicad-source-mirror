/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/log.h>
#include <common.h>
#include <sch_plotter.h>
#include <locale_io.h>
#include <plotters/plotter_hpgl.h>
#include <plotters/plotter_dxf.h>
#include <plotters/plotters_pslike.h>

#include <pgm_base.h>
#include <trace_helpers.h>

#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <schematic.h>
#include <sch_screen.h>
#include <settings/settings_manager.h>

// Note:
// We need to switch between sheets to plot a hierarchy and update references and sheet number
// Use SCHEMATIC::SetCurrentSheet( xxx ) to switch to a sheet.
// Do not use SCH_EDIT_FRAME::SetCurrentSheet( xxx ) to switch to a sheet, because the new sheet
// is not displayed, but SCH_EDIT_FRAME::SetCurrentSheet() has side effects to the current VIEW
// (clear some data used to show the sheet on screen) and does not fully restore the "old" screen


static const wxChar* plot_sheet_list( HPGL_PAGE_SIZE aSize )
{
    switch( aSize )
    {
    default:
    case HPGL_PAGE_SIZE::DEFAULT: return nullptr;
    case HPGL_PAGE_SIZE::SIZE_A5: return wxT( "A5" );
    case HPGL_PAGE_SIZE::SIZE_A4: return wxT( "A4" );
    case HPGL_PAGE_SIZE::SIZE_A3: return wxT( "A3" );
    case HPGL_PAGE_SIZE::SIZE_A2: return wxT( "A2" );
    case HPGL_PAGE_SIZE::SIZE_A1: return wxT( "A1" );
    case HPGL_PAGE_SIZE::SIZE_A0: return wxT( "A0" );
    case HPGL_PAGE_SIZE::SIZE_A: return wxT( "A" );
    case HPGL_PAGE_SIZE::SIZE_B: return wxT( "B" );
    case HPGL_PAGE_SIZE::SIZE_C: return wxT( "C" );
    case HPGL_PAGE_SIZE::SIZE_D: return wxT( "D" );
    case HPGL_PAGE_SIZE::SIZE_E: return wxT( "E" );
    }
}


SCH_PLOTTER::SCH_PLOTTER( SCHEMATIC* aSchematic ) :
        m_schematic( aSchematic )
{
    m_colorSettings = nullptr;
}


SCH_PLOTTER::SCH_PLOTTER( SCH_EDIT_FRAME* aFrame ) :
        m_schematic( &aFrame->Schematic() )
{
    m_colorSettings = nullptr;
}


wxFileName SCH_PLOTTER::getOutputFilenameSingle( const SCH_PLOT_SETTINGS& aPlotSettings,
                                            REPORTER* aReporter, const wxString& aExt )
{
    if( !aPlotSettings.m_outputFile.empty() )
        return aPlotSettings.m_outputFile;
    else
    {
        wxString fname = m_schematic->GetUniqueFilenameForCurrentSheet();

        // The sub sheet can be in a sub_hierarchy, but we plot the file in the main
        // project folder (or the folder specified by the caller), so replace separators
        // to create a unique filename:
        fname.Replace( "/", "_" );
        fname.Replace( "\\", "_" );

        return createPlotFileName( aPlotSettings, fname, aExt, aReporter );
    }

}


void SCH_PLOTTER::createPDFFile( const SCH_PLOT_SETTINGS& aPlotSettings,
                                 RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    SCH_SHEET_PATH oldsheetpath = m_schematic->CurrentSheet(); // sheetpath is saved here

    /* When printing all pages, the printed page is not the current page.  In complex hierarchies,
     * we must update symbol references and other parameters in the given printed SCH_SCREEN,
     * according to the sheet path because in complex hierarchies a SCH_SCREEN (a drawing ) is
     * shared between many sheets and symbol references depend on the actual sheet path used.
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotSettings.m_plotAll )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_schematic->CurrentSheet() );
    }

    // Allocate the plotter and set the job level parameter
    PDF_PLOTTER* plotter = new PDF_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetColorMode( !aPlotSettings.m_blackAndWhite );
    plotter->SetCreator( wxT( "Eeschema-PDF" ) );
    plotter->SetTitle( ExpandTextVars( m_schematic->RootScreen()->GetTitleBlock().GetTitle(),
                                       &m_schematic->Prj() ) );

    wxString   msg;
    wxFileName plotFileName;
    LOCALE_IO  toggle; // Switch the locale to standard C

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_schematic->SetCurrentSheet( sheetList[i] );
        m_schematic->CurrentSheet().UpdateAllScreenReferences();
        m_schematic->SetSheetNumberAndCount();

        SCH_SCREEN* screen = m_schematic->CurrentSheet().LastScreen();

        if( i == 0 )
        {
            try
            {
                wxString ext = PDF_PLOTTER::GetDefaultFileExtension();
                plotFileName = getOutputFilenameSingle( aPlotSettings, aReporter, ext );

                m_lastOutputFilePath = plotFileName.GetFullPath();

                if( !plotFileName.IsOk() )
                    return;

                if( !plotter->OpenFile( plotFileName.GetFullPath() ) )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Failed to create file '%s'." ),
                                    plotFileName.GetFullPath() );
                        aReporter->Report( msg, RPT_SEVERITY_ERROR );
                    }
                    delete plotter;
                    return;
                }

                // Open the plotter and do the first page
                setupPlotPagePDF( plotter, screen, aPlotSettings );
                plotter->StartPlot( sheetList[i].GetPageNumber(), _( "Root" ) );
            }
            catch( const IO_ERROR& e )
            {
                // Cannot plot PDF file
                if( aReporter )
                {
                    msg.Printf( wxT( "PDF Plotter exception: %s" ), e.What() );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }

                restoreEnvironment( plotter, oldsheetpath );
                return;
            }
        }
        else
        {
            /* For the following pages you need to close the (finished) page,
             *  reconfigure, and then start a new one */
            plotter->ClosePage();
            setupPlotPagePDF( plotter, screen, aPlotSettings );
            plotter->StartPage( sheetList[i].GetPageNumber(),
                                sheetList[i].Last()->GetFields()[SHEETNAME].GetShownText( false ) );
        }

        plotOneSheetPDF( plotter, screen, aPlotSettings );
    }

    // Everything done, close the plot and restore the environment
    if( aReporter )
    {
        msg.Printf( _( "Plotted to '%s'.\n" ), plotFileName.GetFullPath() );
        aReporter->Report( msg, RPT_SEVERITY_ACTION );
        aReporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );
    }

    restoreEnvironment( plotter, oldsheetpath );
}


void SCH_PLOTTER::plotOneSheetPDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen,
                                   const SCH_PLOT_SETTINGS& aPlotSettings )
{
    if( aPlotSettings.m_useBackgroundColor && aPlotter->GetColorMode() )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetBackgroundColor() );
        VECTOR2I end( aPlotter->PageSettings().GetWidthIU( schIUScale.IU_PER_MILS ),
                      aPlotter->PageSettings().GetHeightIU( schIUScale.IU_PER_MILS ) );
        aPlotter->Rect( VECTOR2I( 0, 0 ), end, FILL_T::FILLED_SHAPE, 1.0 );
    }

    if( aPlotSettings.m_plotDrawingSheet )
    {
        COLOR4D color = COLOR4D::BLACK;

        if( aPlotter->GetColorMode() )
            color = aPlotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();
        const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic

        PlotDrawingSheet( aPlotter, &aScreen->Schematic()->Prj(),
                          aScreen->GetTitleBlock(),
                          actualPage,
                          aScreen->Schematic()->GetProperties(),
                          aScreen->GetPageNumber(), aScreen->GetPageCount(), sheetName, sheetPath,
                          aScreen->GetFileName(), color, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( aPlotter );
}


void SCH_PLOTTER::setupPlotPagePDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen,
                                    const SCH_PLOT_SETTINGS& aPlotSettings )
{
    PAGE_INFO   plotPage;                               // page size selected to plot

    // Considerations on page size and scaling requests
    const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic

    switch( aPlotSettings.m_pageSizeSelect )
    {
    case PAGE_SIZE_A:
        plotPage.SetType( wxT( "A" ) );
        plotPage.SetPortrait( actualPage.IsPortrait() );
        break;

    case PAGE_SIZE_A4:
        plotPage.SetType( wxT( "A4" ) );
        plotPage.SetPortrait( actualPage.IsPortrait() );
        break;

    case PAGE_SIZE_AUTO:
    default:
        plotPage = actualPage;
        break;
    }

    double  scalex  = (double) plotPage.GetWidthMils() / actualPage.GetWidthMils();
    double  scaley  = (double) plotPage.GetHeightMils() / actualPage.GetHeightMils();
    double  scale   = std::min( scalex, scaley );
    aPlotter->SetPageSettings( plotPage );

    // Currently, plot units are in decimil
    aPlotter->SetViewport( VECTOR2I( 0, 0 ), schIUScale.IU_PER_MILS / 10, scale, false );
}


void SCH_PLOTTER::createPSFiles( const SCH_PLOT_SETTINGS& aPlotSettings,
                                 RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    SCH_SHEET_PATH oldsheetpath = m_schematic->CurrentSheet(); // sheetpath is saved here
    PAGE_INFO      plotPage;                                   // page size selected to plot
    wxString       msg;

    /* When printing all pages, the printed page is not the current page.
     * In complex hierarchies, we must update symbol references and other parameters in the
     * given printed SCH_SCREEN, accordant to the sheet path because in complex hierarchies
     * a SCH_SCREEN (a drawing ) is shared between many sheets and symbol references
     * depend on the actual sheet path used.
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotSettings.m_plotAll )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_schematic->CurrentSheet() );
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_schematic->SetCurrentSheet( sheetList[i] );
        m_schematic->CurrentSheet().UpdateAllScreenReferences();
        m_schematic->SetSheetNumberAndCount();

        SCH_SCREEN* screen = m_schematic->CurrentSheet().LastScreen();
        PAGE_INFO   actualPage = screen->GetPageSettings();

        switch( aPlotSettings.m_pageSizeSelect )
        {
        case PAGE_SIZE_A:
            plotPage.SetType( wxT( "A" ) );
            plotPage.SetPortrait( actualPage.IsPortrait() );
            break;

        case PAGE_SIZE_A4:
            plotPage.SetType( wxT( "A4" ) );
            plotPage.SetPortrait( actualPage.IsPortrait() );
            break;

        case PAGE_SIZE_AUTO:
        default: plotPage = actualPage; break;
        }

        double  scalex = (double) plotPage.GetWidthMils() / actualPage.GetWidthMils();
        double  scaley = (double) plotPage.GetHeightMils() / actualPage.GetHeightMils();
        double  scale = std::min( scalex, scaley );
        VECTOR2I plot_offset;

        try
        {
            wxString fname = m_schematic->GetUniqueFilenameForCurrentSheet();

            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace( "/", "_" );
            fname.Replace( "\\", "_" );
            wxString   ext = PS_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( aPlotSettings, fname, ext, aReporter );

            m_lastOutputFilePath = plotFileName.GetFullPath();

            if( !plotFileName.IsOk() )
                return;

            if( plotOneSheetPS( plotFileName.GetFullPath(), screen, aRenderSettings, actualPage,
                                plot_offset, scale, aPlotSettings ) )
            {
                if( aReporter )
                {
                    msg.Printf( _( "Plotted to '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ACTION );
                }
            }
            else
            {
                if( aReporter )
                {
                    // Error
                    msg.Printf( _( "Failed to create file '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
        }
        catch( IO_ERROR& e )
        {
            if( aReporter )
            {
                msg.Printf( wxT( "PS Plotter exception: %s" ), e.What() );
                aReporter->Report( msg, RPT_SEVERITY_ERROR );
            }
        }
    }

    if( aReporter )
    {
        aReporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );
    }

    restoreEnvironment( nullptr, oldsheetpath );
}


bool SCH_PLOTTER::plotOneSheetPS( const wxString& aFileName, SCH_SCREEN* aScreen,
                                            RENDER_SETTINGS* aRenderSettings,
                                            const PAGE_INFO& aPageInfo, const VECTOR2I& aPlot0ffset,
                                            double aScale, const SCH_PLOT_SETTINGS& aPlotSettings )
{
    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( aPageInfo );
    plotter->SetColorMode( !aPlotSettings.m_blackAndWhite );

    // Currently, plot units are in decimil
    plotter->SetViewport( aPlot0ffset, schIUScale.IU_PER_MILS / 10, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-PS" ) );

    if( !plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO toggle; // Switch the locale to standard C

    plotter->StartPlot( m_schematic->CurrentSheet().GetPageNumber() );

    if( aPlotSettings.m_useBackgroundColor && plotter->GetColorMode() )
    {
        plotter->SetColor( plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );

        VECTOR2I end( plotter->PageSettings().GetWidthIU( schIUScale.IU_PER_MILS ),
                      plotter->PageSettings().GetHeightIU( schIUScale.IU_PER_MILS ) );
        plotter->Rect( VECTOR2I( 0, 0 ), end, FILL_T::FILLED_SHAPE, 1.0 );
    }

    if( aPlotSettings.m_plotDrawingSheet )
    {
        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();
        COLOR4D  color = plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        PlotDrawingSheet( plotter, &aScreen->Schematic()->Prj(),
                          aScreen->GetTitleBlock(),
                          aPageInfo, aScreen->Schematic()->GetProperties(),
                          aScreen->GetPageNumber(), aScreen->GetPageCount(), sheetName, sheetPath,
                          aScreen->GetFileName(), plotter->GetColorMode() ? color : COLOR4D::BLACK,
                          aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;

    return true;
}


void SCH_PLOTTER::createSVGFiles( const SCH_PLOT_SETTINGS& aPlotSettings,
                                  RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    wxString       msg;
    SCH_SHEET_PATH oldsheetpath = m_schematic->CurrentSheet();
    SCH_SHEET_LIST sheetList;

    if( aPlotSettings.m_plotAll )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_schematic->CurrentSheet() );
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        SCH_SCREEN* screen;

        m_schematic->SetCurrentSheet( sheetList[i] );
        m_schematic->CurrentSheet().UpdateAllScreenReferences();
        m_schematic->SetSheetNumberAndCount();

        screen = m_schematic->CurrentSheet().LastScreen();

        try
        {
            wxString fname = m_schematic->GetUniqueFilenameForCurrentSheet();

            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace( "/", "_" );
            fname.Replace( "\\", "_" );
            wxString   ext = SVG_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( aPlotSettings, fname, ext, aReporter );

            m_lastOutputFilePath = plotFileName.GetFullPath();

            if( !plotFileName.IsOk() )
                return;

            bool success = plotOneSheetSVG( plotFileName.GetFullPath(), screen, aRenderSettings,
                                            aPlotSettings );

            if( !success )
            {
                if( aReporter )
                {
                    msg.Printf( _( "Failed to create file '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
            else
            {
                if( aReporter )
                {
                    msg.Printf( _( "Plotted to '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ACTION );
                }
            }
        }
        catch( const IO_ERROR& e )
        {
            if( aReporter )
            {
                // Cannot plot SVG file
                msg.Printf( wxT( "SVG Plotter exception: %s" ), e.What() );
                aReporter->Report( msg, RPT_SEVERITY_ERROR );
            }
            break;
        }
    }

    if( aReporter )
    {
        aReporter->ReportTail( _( "Done" ), RPT_SEVERITY_INFO );
    }

    restoreEnvironment( nullptr, oldsheetpath );
}


bool SCH_PLOTTER::plotOneSheetSVG( const wxString& aFileName, SCH_SCREEN* aScreen,
                                   RENDER_SETTINGS*         aRenderSettings,
                                   const SCH_PLOT_SETTINGS& aPlotSettings )
{
    PAGE_INFO plotPage;
    // Adjust page size and scaling requests
    const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic

    switch( aPlotSettings.m_pageSizeSelect )
    {
    case PAGE_SIZE_A:
        plotPage.SetType( wxT( "A" ) );
        plotPage.SetPortrait( actualPage.IsPortrait() );
        break;

    case PAGE_SIZE_A4:
        plotPage.SetType( wxT( "A4" ) );
        plotPage.SetPortrait( actualPage.IsPortrait() );
        break;

    case PAGE_SIZE_AUTO:
    default:
        plotPage = actualPage;
        break;
    }

    SVG_PLOTTER* plotter = new SVG_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( plotPage );
    plotter->SetColorMode( aPlotSettings.m_blackAndWhite ? false : true );
    VECTOR2I plot_offset;

    double  scalex = (double) plotPage.GetWidthMils() / actualPage.GetWidthMils();
    double  scaley = (double) plotPage.GetHeightMils() / actualPage.GetHeightMils();
    double  scale = std::min( scalex, scaley );

    // Currently, plot units are in decimil
    plotter->SetViewport( plot_offset, schIUScale.IU_PER_MILS / 10, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-SVG" ) );

    if( !plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO toggle;

    plotter->StartPlot( m_schematic->CurrentSheet().GetPageNumber() );

    if( aPlotSettings.m_useBackgroundColor && plotter->GetColorMode() )
    {
        plotter->SetColor( plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
        VECTOR2I end( plotter->PageSettings().GetWidthIU( schIUScale.IU_PER_MILS ),
                      plotter->PageSettings().GetHeightIU( schIUScale.IU_PER_MILS ) );
        plotter->Rect( VECTOR2I( 0, 0 ), end, FILL_T::FILLED_SHAPE, 1.0 );
    }

    if( aPlotSettings.m_plotDrawingSheet )
    {
        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();
        COLOR4D  color = plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        PlotDrawingSheet( plotter, &aScreen->Schematic()->Prj(),
                          aScreen->GetTitleBlock(),
                          actualPage, aScreen->Schematic()->GetProperties(), aScreen->GetPageNumber(),
                          aScreen->GetPageCount(), sheetName, sheetPath, aScreen->GetFileName(),
                          plotter->GetColorMode() ? color : COLOR4D::BLACK,
                          aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;

    return true;
}


void SCH_PLOTTER::createHPGLFiles( const SCH_PLOT_SETTINGS& aPlotSettings,
                                   RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    SCH_SCREEN*    screen = m_schematic->RootScreen();
    SCH_SHEET_PATH oldsheetpath = m_schematic->CurrentSheet();

    /* When printing all pages, the printed page is not the current page.  In complex hierarchies,
     * we must update symbol references and other parameters in the given printed SCH_SCREEN,
     * according to the sheet path because in complex hierarchies a SCH_SCREEN (a drawing ) is
     * shared between many sheets and symbol references depend on the actual sheet path used.
     */
    SCH_SHEET_LIST  sheetList;

    if( aPlotSettings.m_plotAll )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_schematic->CurrentSheet() );
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_schematic->SetCurrentSheet( sheetList[i] );
        m_schematic->CurrentSheet().UpdateAllScreenReferences();
        m_schematic->SetSheetNumberAndCount();

        screen = m_schematic->CurrentSheet().LastScreen();

        if( !screen ) // LastScreen() may return NULL
            screen = m_schematic->RootScreen();

        const PAGE_INFO&    curPage = screen->GetPageSettings();

        PAGE_INFO           plotPage = curPage;

        // if plotting on a page size other than curPage
        plotPage.SetType( plot_sheet_list( aPlotSettings.m_HPGLPaperSizeSelect ) );

        // Calculation of conversion scales.
        double  plot_scale = (double) plotPage.GetWidthMils() / curPage.GetWidthMils();

        // Calculate offsets
        VECTOR2I plotOffset;
        wxString msg;

        if( aPlotSettings.m_HPGLPlotOrigin == HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER )
        {
            plotOffset.x = plotPage.GetWidthIU( schIUScale.IU_PER_MILS ) / 2;
            plotOffset.y = -plotPage.GetHeightIU( schIUScale.IU_PER_MILS ) / 2;
        }

        try
        {
            wxString fname = m_schematic->GetUniqueFilenameForCurrentSheet();
            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace( "/", "_" );
            fname.Replace( "\\", "_" );
            wxString ext = HPGL_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( aPlotSettings, fname, ext, aReporter );

            if( !plotFileName.IsOk() )
                return;

            LOCALE_IO toggle;

            if( plotOneSheetHpgl( plotFileName.GetFullPath(), screen, curPage, aRenderSettings,
                                  plotOffset, plot_scale, aPlotSettings ) )
            {
                if( aReporter )
                {
                    msg.Printf( _( "Plotted to '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ACTION );
                }
            }
            else
            {
                if( aReporter )
                {
                    msg.Printf( _( "Failed to create file '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
        }
        catch( IO_ERROR& e )
        {
            if( aReporter )
            {
                msg.Printf( wxT( "HPGL Plotter exception: %s" ), e.What() );
                aReporter->Report( msg, RPT_SEVERITY_ERROR );
            }
        }
    }

    if( aReporter )
    {
        aReporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );
    }

    restoreEnvironment( nullptr, oldsheetpath );
}


bool SCH_PLOTTER::plotOneSheetHpgl( const wxString&   aFileName,
                                    SCH_SCREEN*       aScreen,
                                    const PAGE_INFO&  aPageInfo,
                                    RENDER_SETTINGS*  aRenderSettings,
                                    const VECTOR2I&   aPlot0ffset,
                                    double            aScale,
                                    const SCH_PLOT_SETTINGS&   aPlotSettings )
{
    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();
    // Currently, plot units are in decimil

    plotter->SetPageSettings( aPageInfo );
    plotter->SetRenderSettings( aRenderSettings );
    plotter->RenderSettings()->LoadColors( m_colorSettings );
    plotter->SetColorMode( !aPlotSettings.m_blackAndWhite );
    plotter->SetViewport( aPlot0ffset, schIUScale.IU_PER_MILS/10, aScale, false );

    // TODO this could be configurable
    plotter->SetTargetChordLength( schIUScale.mmToIU( 0.6 ) );

    switch( aPlotSettings.m_HPGLPlotOrigin )
    {
    case HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT:
    case HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER:
    default:
        plotter->SetUserCoords( false );
        break;
    case HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE:
        plotter->SetUserCoords( true );
        plotter->SetUserCoordsFit( false );
        break;
    case HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT:
        plotter->SetUserCoords( true );
        plotter->SetUserCoordsFit( true );
        break;
    }

    // Init :
    plotter->SetCreator( wxT( "Eeschema-HPGL" ) );

    if( !plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO toggle;

    // Pen num and pen speed are not initialized here.
    // Default HPGL driver values are used
    plotter->SetPenDiameter( aPlotSettings.m_HPGLPenSize );
    plotter->StartPlot( m_schematic->CurrentSheet().GetPageNumber() );

    if( aPlotSettings.m_plotDrawingSheet )
    {
        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();

        PlotDrawingSheet( plotter, &m_schematic->Prj(),
                          aScreen->GetTitleBlock(),
                          aPageInfo,
                          aScreen->Schematic()->GetProperties(), aScreen->GetPageNumber(),
                          aScreen->GetPageCount(), sheetName, sheetPath, aScreen->GetFileName(),
                          COLOR4D::BLACK, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();

    delete plotter;

    return true;
}


void SCH_PLOTTER::createDXFFiles( const SCH_PLOT_SETTINGS& aPlotSettings,
                                  RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    SCH_SHEET_PATH  oldsheetpath = m_schematic->CurrentSheet();

    /* When printing all pages, the printed page is not the current page.  In complex hierarchies,
     * we must update symbol references and other parameters in the given printed SCH_SCREEN,
     * according to the sheet path because in complex hierarchies a SCH_SCREEN (a drawing ) is
     * shared between many sheets and symbol references depend on the actual sheet path used.
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotSettings.m_plotAll )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_schematic->CurrentSheet() );
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_schematic->SetCurrentSheet( sheetList[i] );
        m_schematic->CurrentSheet().UpdateAllScreenReferences();
        m_schematic->SetSheetNumberAndCount();

        SCH_SCREEN* screen = m_schematic->CurrentSheet().LastScreen();
        VECTOR2I    plot_offset;
        wxString    msg;

        try
        {
            wxString fname = m_schematic->GetUniqueFilenameForCurrentSheet();

            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace( "/", "_" );
            fname.Replace( "\\", "_" );
            wxString   ext = DXF_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( aPlotSettings, fname, ext, aReporter );

            m_lastOutputFilePath = plotFileName.GetFullPath();

            if( !plotFileName.IsOk() )
                return;

            if( plotOneSheetDXF( plotFileName.GetFullPath(), screen, aRenderSettings, plot_offset,
                                 1.0, aPlotSettings ) )
            {
                if( aReporter )
                {
                    msg.Printf( _( "Plotted to '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ACTION );
                }
            }
            else // Error
            {
                if( aReporter )
                {
                    msg.Printf( _( "Failed to create file '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
        }
        catch( IO_ERROR& e )
        {
            if( aReporter )
            {
                msg.Printf( wxT( "DXF Plotter exception: %s" ), e.What() );
                aReporter->Report( msg, RPT_SEVERITY_ERROR );
            }

            m_schematic->SetCurrentSheet( oldsheetpath );
            m_schematic->CurrentSheet().UpdateAllScreenReferences();
            m_schematic->SetSheetNumberAndCount();
            return;
        }
    }

    if( aReporter )
        aReporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );

    restoreEnvironment( nullptr, oldsheetpath );
}


bool SCH_PLOTTER::plotOneSheetDXF( const wxString& aFileName, SCH_SCREEN* aScreen,
                                   RENDER_SETTINGS* aRenderSettings, const VECTOR2I& aPlotOffset,
                                   double aScale, const SCH_PLOT_SETTINGS& aPlotSettings )
{
    aRenderSettings->LoadColors( m_colorSettings );
    aRenderSettings->SetDefaultPenWidth( 0 );

    const PAGE_INFO& pageInfo = aScreen->GetPageSettings();
    DXF_PLOTTER*     plotter = new DXF_PLOTTER();

    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( !aPlotSettings.m_blackAndWhite );

    // Currently, plot units are in decimil
    plotter->SetViewport( aPlotOffset, schIUScale.IU_PER_MILS / 10, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-DXF" ) );

    if( !plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO toggle;

    plotter->StartPlot( m_schematic->CurrentSheet().GetPageNumber() );

    if( aPlotSettings.m_plotDrawingSheet )
    {
        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();
        COLOR4D  color = plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        PlotDrawingSheet( plotter, &m_schematic->Prj(),
                          aScreen->GetTitleBlock(),
                          pageInfo,
                          aScreen->Schematic()->GetProperties(), aScreen->GetPageNumber(),
                          aScreen->GetPageCount(), sheetName, sheetPath, aScreen->GetFileName(),
                          plotter->GetColorMode() ? color : COLOR4D::BLACK,
                          aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    // finish
    plotter->EndPlot();
    delete plotter;

    return true;
}


void SCH_PLOTTER::restoreEnvironment( PDF_PLOTTER* aPlotter, SCH_SHEET_PATH& aOldsheetpath )
{
    if( aPlotter )
    {
        aPlotter->EndPlot();
        delete aPlotter;
    }

    // Restore the initial sheet
    m_schematic->SetCurrentSheet( aOldsheetpath );
    m_schematic->CurrentSheet().UpdateAllScreenReferences();
    m_schematic->SetSheetNumberAndCount();
}


wxFileName SCH_PLOTTER::createPlotFileName( const SCH_PLOT_SETTINGS& aPlotSettings,
                                            const wxString&          aPlotFileName,
                                            const wxString& aExtension, REPORTER* aReporter )
{
    wxFileName retv;
    wxFileName tmp;

    tmp.SetPath( aPlotSettings.m_outputDirectory );
    retv.SetPath( tmp.GetPath() );

    if( !aPlotFileName.IsEmpty() )
        retv.SetName( aPlotFileName );
    else
        retv.SetName( _( "Schematic" ) );

    retv.SetExt( aExtension );

    if( !EnsureFileDirectoryExists( &tmp, retv.GetFullName(), aReporter ) || !tmp.IsDirWritable() )
    {
        if( aReporter )
        {
            wxString msg = wxString::Format( _( "Failed to write plot files to folder '%s'." ),
                                             tmp.GetPath() );
            aReporter->Report( msg, RPT_SEVERITY_ERROR );
        }
        retv.Clear();

        SCHEMATIC_SETTINGS& settings = m_schematic->Settings();
        settings.m_PlotDirectoryName.Clear();
    }
    else
    {
        retv.SetPath( tmp.GetPath() );
    }

    wxLogTrace( tracePathsAndFiles, "Writing plot file '%s'.", retv.GetFullPath() );

    return retv;
}


void SCH_PLOTTER::Plot( PLOT_FORMAT aPlotFormat, const SCH_PLOT_SETTINGS& aPlotSettings,
                        RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    SETTINGS_MANAGER& settingsMgr = Pgm().GetSettingsManager();

    m_colorSettings = settingsMgr.GetColorSettings( aPlotSettings.m_theme );

    switch( aPlotFormat )
    {
    default:
    case PLOT_FORMAT::POST: createPSFiles( aPlotSettings, aRenderSettings, aReporter ); break;
    case PLOT_FORMAT::DXF: createDXFFiles( aPlotSettings, aRenderSettings, aReporter ); break;
    case PLOT_FORMAT::PDF: createPDFFile( aPlotSettings, aRenderSettings, aReporter ); break;
    case PLOT_FORMAT::SVG: createSVGFiles( aPlotSettings, aRenderSettings, aReporter ); break;
    case PLOT_FORMAT::HPGL: createHPGLFiles( aPlotSettings, aRenderSettings, aReporter ); break;
    }
}
