/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


wxFileName SCH_PLOTTER::getOutputFilenameSingle( const SCH_PLOT_OPTS& aPlotOpts,
                                                 REPORTER* aReporter, const wxString& aExt )
{
    if( !aPlotOpts.m_outputFile.empty() )
    {
        return aPlotOpts.m_outputFile;
    }
    else
    {
        wxString fname = m_schematic->GetUniqueFilenameForCurrentSheet();

        // The sub sheet can be in a sub_hierarchy, but we plot the file in the main
        // project folder (or the folder specified by the caller), so replace separators
        // to create a unique filename:
        fname.Replace( "/", "_" );
        fname.Replace( "\\", "_" );

        return createPlotFileName( aPlotOpts, fname, aExt, aReporter );
    }

}


void SCH_PLOTTER::createPDFFile( const SCH_PLOT_OPTS& aPlotOpts,
                                 SCH_RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    SCH_SHEET_PATH oldsheetpath = m_schematic->CurrentSheet(); // sheetpath is saved here

    /* When printing all pages, the printed page is not the current page.  In complex hierarchies,
     * we must update symbol references and other parameters in the given printed SCH_SCREEN,
     * according to the sheet path because in complex hierarchies a SCH_SCREEN (a drawing ) is
     * shared between many sheets and symbol references depend on the actual sheet path used.
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotOpts.m_plotAll || aPlotOpts.m_plotPages.size() > 0 )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByHierarchicalPageNumbers();

        // remove the non-selected pages if we are in plot pages mode
        if( aPlotOpts.m_plotPages.size() > 0 )
            sheetList.TrimToPageNumbers( aPlotOpts.m_plotPages );
    }
    else
    {
        // in Eeschema, this prints the current page
        sheetList.push_back( m_schematic->CurrentSheet() );
    }

    if( sheetList.empty() )
    {
        if( aReporter )
            aReporter->Report( _( "No sheets to plot." ), RPT_SEVERITY_ERROR );

        return;
    }

    wxCHECK( m_schematic, /* void */ );

    // Allocate the plotter and set the job level parameter
    PDF_PLOTTER* plotter = new PDF_PLOTTER( &m_schematic->Project() );
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetColorMode( !aPlotOpts.m_blackAndWhite );
    plotter->SetCreator( wxT( "Eeschema-PDF" ) );
    plotter->SetTitle( ExpandTextVars( m_schematic->RootScreen()->GetTitleBlock().GetTitle(),
                                       &m_schematic->Project() ) );

    wxString   msg;
    wxFileName plotFileName;

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_schematic->SetCurrentSheet( sheetList[i] );
        m_schematic->CurrentSheet().UpdateAllScreenReferences();
        m_schematic->SetSheetNumberAndCount();

        SCH_SCREEN* screen = m_schematic->CurrentSheet().LastScreen();
        wxString    sheetName = sheetList[i].Last()->GetField( FIELD_T::SHEET_NAME )->GetShownText( false );

        if( aPlotOpts.m_PDFMetadata )
        {
            msg = wxS( "AUTHOR" );

            if( m_schematic->ResolveTextVar( &sheetList[i], &msg, 0 ) )
                plotter->SetAuthor( msg );

            msg = wxS( "SUBJECT" );

            if( m_schematic->ResolveTextVar( &sheetList[i], &msg, 0 ) )
                plotter->SetSubject( msg );
        }

        if( i == 0 )
        {
            try
            {
                wxString ext = PDF_PLOTTER::GetDefaultFileExtension();
                plotFileName = getOutputFilenameSingle( aPlotOpts, aReporter, ext );

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
                setupPlotPagePDF( plotter, screen, aPlotOpts );

                plotter->StartPlot( sheetList[i].GetPageNumber(), sheetName );
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
            setupPlotPagePDF( plotter, screen, aPlotOpts );
            SCH_SHEET_PATH parentSheet = sheetList[i];

            if( parentSheet.size() > 1 )
            {
                // The sheet path is the full path to the sheet, so we need to remove the last
                // sheet name to get the parent sheet path
                parentSheet.pop_back();
            }

            wxString parentSheetName =
                    parentSheet.Last()->GetField( FIELD_T::SHEET_NAME )->GetShownText( false );

            plotter->StartPage( sheetList[i].GetPageNumber(), sheetName,
                                parentSheet.GetPageNumber(), parentSheetName );
        }

        plotOneSheetPDF( plotter, screen, aPlotOpts );
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
                                   const SCH_PLOT_OPTS& aPlotOpts )
{
    if( aPlotOpts.m_useBackgroundColor && aPlotter->GetColorMode() )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetBackgroundColor() );

        // Use page size selected in schematic to know the schematic bg area
        const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic
        VECTOR2I end( actualPage.GetWidthIU( schIUScale.IU_PER_MILS ),
                      actualPage.GetHeightIU( schIUScale.IU_PER_MILS ) );

        aPlotter->Rect( VECTOR2I( 0, 0 ), end, FILL_T::FILLED_SHAPE, 1.0 );
    }

    if( aPlotOpts.m_plotDrawingSheet )
    {
        COLOR4D color = COLOR4D::BLACK;

        if( aPlotter->GetColorMode() )
            color = aPlotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();
        const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic

        PlotDrawingSheet( aPlotter, &aScreen->Schematic()->Project(),
                          aScreen->GetTitleBlock(),
                          actualPage,
                          aScreen->Schematic()->GetProperties(),
                          aScreen->GetPageNumber(), aScreen->GetPageCount(), sheetName, sheetPath,
                          aScreen->GetFileName(), color, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( aPlotter, aPlotOpts );
}


void SCH_PLOTTER::setupPlotPagePDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen,
                                    const SCH_PLOT_OPTS& aPlotOpts )
{
    PAGE_INFO   plotPage;                               // page size selected to plot

    // Considerations on page size and scaling requests
    const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic

    switch( aPlotOpts.m_pageSizeSelect )
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


void SCH_PLOTTER::createPSFiles( const SCH_PLOT_OPTS& aPlotOpts,
                                 SCH_RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
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

    if( aPlotOpts.m_plotAll )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByPageNumbers();

        // remove the non-selected pages if we are in plot pages mode
        if( aPlotOpts.m_plotPages.size() > 0 )
        {
            sheetList.TrimToPageNumbers( aPlotOpts.m_plotPages );
        }
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

        switch( aPlotOpts.m_pageSizeSelect )
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
            wxFileName plotFileName = createPlotFileName( aPlotOpts, fname, ext, aReporter );

            m_lastOutputFilePath = plotFileName.GetFullPath();

            if( !plotFileName.IsOk() )
            {
                if( aReporter )
                {
                    // Error
                    msg.Printf( _( "Failed to create file '%s'." ), plotFileName.GetFullPath() );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
            else if( plotOneSheetPS( plotFileName.GetFullPath(), screen, aRenderSettings,
                                     actualPage, plot_offset, scale, aPlotOpts ) )
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
        aReporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );

    restoreEnvironment( nullptr, oldsheetpath );
}


bool SCH_PLOTTER::plotOneSheetPS( const wxString& aFileName, SCH_SCREEN* aScreen,
                                  RENDER_SETTINGS* aRenderSettings, const PAGE_INFO& aPageInfo,
                                  const VECTOR2I& aPlot0ffset, double aScale,
                                  const SCH_PLOT_OPTS& aPlotOpts )
{
    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( aPageInfo );
    plotter->SetColorMode( !aPlotOpts.m_blackAndWhite );

    // Currently, plot units are in decimil
    plotter->SetViewport( aPlot0ffset, schIUScale.IU_PER_MILS / 10, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-PS" ) );

    if( !plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    plotter->StartPlot( m_schematic->CurrentSheet().GetPageNumber() );

    if( aPlotOpts.m_useBackgroundColor && plotter->GetColorMode() )
    {
        plotter->SetColor( plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );

        // Use page size selected in schematic to know the schematic bg area
        const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic
        VECTOR2I end( actualPage.GetWidthIU( schIUScale.IU_PER_MILS ),
                      actualPage.GetHeightIU( schIUScale.IU_PER_MILS ) );

        plotter->Rect( VECTOR2I( 0, 0 ), end, FILL_T::FILLED_SHAPE, 1.0, 0 );
    }

    if( aPlotOpts.m_plotDrawingSheet )
    {
        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();
        COLOR4D  color = plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        PlotDrawingSheet( plotter, &aScreen->Schematic()->Project(),
                          aScreen->GetTitleBlock(),
                          aPageInfo, aScreen->Schematic()->GetProperties(),
                          aScreen->GetPageNumber(), aScreen->GetPageCount(), sheetName, sheetPath,
                          aScreen->GetFileName(), plotter->GetColorMode() ? color : COLOR4D::BLACK,
                          aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter, aPlotOpts );

    plotter->EndPlot();
    delete plotter;

    return true;
}


void SCH_PLOTTER::createSVGFiles( const SCH_PLOT_OPTS& aPlotOpts,
                                  SCH_RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    wxString       msg;
    SCH_SHEET_PATH oldsheetpath = m_schematic->CurrentSheet();
    SCH_SHEET_LIST sheetList;

    if( aPlotOpts.m_plotAll )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByPageNumbers();

        // remove the non-selected pages if we are in plot pages mode
        if( aPlotOpts.m_plotPages.size() > 0 )
        {
            sheetList.TrimToPageNumbers( aPlotOpts.m_plotPages );
        }
    }
    else
    {
        // in Eeschema, this prints the current page
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
            wxFileName plotFileName = createPlotFileName( aPlotOpts, fname, ext, aReporter );

            m_lastOutputFilePath = plotFileName.GetFullPath();

            if( !plotFileName.IsOk() )
                return;

            bool success = plotOneSheetSVG( plotFileName.GetFullPath(), screen, aRenderSettings,
                                            aPlotOpts );

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
        aReporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );
    }

    restoreEnvironment( nullptr, oldsheetpath );
}


bool SCH_PLOTTER::plotOneSheetSVG( const wxString& aFileName, SCH_SCREEN* aScreen,
                                   RENDER_SETTINGS* aRenderSettings,
                                   const SCH_PLOT_OPTS& aPlotOpts )
{
    PAGE_INFO plotPage;

    // Adjust page size and scaling requests
    const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic

    switch( aPlotOpts.m_pageSizeSelect )
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
    plotter->SetColorMode( aPlotOpts.m_blackAndWhite ? false : true );
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

    plotter->StartPlot( m_schematic->CurrentSheet().GetPageNumber() );

    if( aPlotOpts.m_useBackgroundColor && plotter->GetColorMode() )
    {
        plotter->SetColor( plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );

        // Use page size selected in schematic to know the schematic bg area
        VECTOR2I end( actualPage.GetWidthIU( schIUScale.IU_PER_MILS ),
                      actualPage.GetHeightIU( schIUScale.IU_PER_MILS ) );

        plotter->Rect( VECTOR2I( 0, 0 ), end, FILL_T::FILLED_SHAPE, 1.0, 0 );
    }

    if( aPlotOpts.m_plotDrawingSheet )
    {
        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();
        COLOR4D  color = plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        PlotDrawingSheet( plotter, &aScreen->Schematic()->Project(),
                          aScreen->GetTitleBlock(),
                          actualPage, aScreen->Schematic()->GetProperties(),
                          aScreen->GetPageNumber(),
                          aScreen->GetPageCount(), sheetName, sheetPath, aScreen->GetFileName(),
                          plotter->GetColorMode() ? color : COLOR4D::BLACK,
                          aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter, aPlotOpts );

    plotter->EndPlot();
    delete plotter;

    return true;
}


void SCH_PLOTTER::createDXFFiles( const SCH_PLOT_OPTS& aPlotOpts,
                                  SCH_RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    SCH_SHEET_PATH  oldsheetpath = m_schematic->CurrentSheet();

    /* When printing all pages, the printed page is not the current page.  In complex hierarchies,
     * we must update symbol references and other parameters in the given printed SCH_SCREEN,
     * according to the sheet path because in complex hierarchies a SCH_SCREEN (a drawing ) is
     * shared between many sheets and symbol references depend on the actual sheet path used.
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotOpts.m_plotAll )
    {
        sheetList.BuildSheetList( &m_schematic->Root(), true );
        sheetList.SortByPageNumbers();

        // remove the non-selected pages if we are in plot pages mode
        if( aPlotOpts.m_plotPages.size() > 0 )
            sheetList.TrimToPageNumbers( aPlotOpts.m_plotPages );
    }
    else
    {
        // in Eeschema, this prints the current page
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
            wxFileName plotFileName = createPlotFileName( aPlotOpts, fname, ext, aReporter );

            m_lastOutputFilePath = plotFileName.GetFullPath();

            if( !plotFileName.IsOk() )
                return;

            if( plotOneSheetDXF( plotFileName.GetFullPath(), screen, aRenderSettings, plot_offset,
                                 1.0, aPlotOpts ) )
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
                                   double aScale, const SCH_PLOT_OPTS& aPlotOpts )
{
    aRenderSettings->LoadColors( m_colorSettings );
    aRenderSettings->SetDefaultPenWidth( 0 );

    const PAGE_INFO& pageInfo = aScreen->GetPageSettings();
    DXF_PLOTTER*     plotter = new DXF_PLOTTER();

    plotter->SetUnits( aPlotOpts.m_DXF_File_Unit );

    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( !aPlotOpts.m_blackAndWhite );

    // Currently, plot units are in decimil
    plotter->SetViewport( aPlotOffset, schIUScale.IU_PER_MILS / 10, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-DXF" ) );

    if( !plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    plotter->StartPlot( m_schematic->CurrentSheet().GetPageNumber() );

    if( aPlotOpts.m_plotDrawingSheet )
    {
        wxString sheetName = m_schematic->CurrentSheet().Last()->GetName();
        wxString sheetPath = m_schematic->CurrentSheet().PathHumanReadable();
        COLOR4D  color = plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        PlotDrawingSheet( plotter, &m_schematic->Project(),
                          aScreen->GetTitleBlock(),
                          pageInfo,
                          aScreen->Schematic()->GetProperties(), aScreen->GetPageNumber(),
                          aScreen->GetPageCount(), sheetName, sheetPath, aScreen->GetFileName(),
                          plotter->GetColorMode() ? color : COLOR4D::BLACK,
                          aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter, aPlotOpts );

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


wxFileName SCH_PLOTTER::createPlotFileName( const SCH_PLOT_OPTS& aPlotOpts,
                                            const wxString& aPlotFileName,
                                            const wxString& aExtension, REPORTER* aReporter )
{
    wxFileName retv;
    wxFileName tmp;

    tmp.SetPath( aPlotOpts.m_outputDirectory );
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


void SCH_PLOTTER::Plot( PLOT_FORMAT aPlotFormat, const SCH_PLOT_OPTS& aPlotOpts,
                        SCH_RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter )
{
    wxString oldVariant = m_schematic->GetCurrentVariant();
    m_schematic->SetCurrentVariant( aPlotOpts.m_variant );
    m_colorSettings = ::GetColorSettings( aPlotOpts.m_theme );

    switch( aPlotFormat )
    {
    default:
    case PLOT_FORMAT::POST: createPSFiles( aPlotOpts, aRenderSettings, aReporter );   break;
    case PLOT_FORMAT::DXF:  createDXFFiles( aPlotOpts, aRenderSettings, aReporter );  break;
    case PLOT_FORMAT::PDF:  createPDFFile( aPlotOpts, aRenderSettings, aReporter );   break;
    case PLOT_FORMAT::SVG:  createSVGFiles( aPlotOpts, aRenderSettings, aReporter );  break;
    case PLOT_FORMAT::HPGL: /* no longer supported */                                 break;
    }

    m_schematic->SetCurrentVariant( oldVariant );
}
