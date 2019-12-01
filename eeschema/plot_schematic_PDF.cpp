/** @file plot_schematic_PDF.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <plotter.h>
#include <sch_edit_frame.h>
#include <base_units.h>
#include <sch_sheet_path.h>
#include <project.h>
#include <general.h>

#include <reporter.h>

#include <dialog_plot_schematic.h>
#include <plot.h>
#include <wx_html_report_panel.h>

void DIALOG_PLOT_SCHEMATIC::createPDFFile( bool aPlotAll, bool aPlotFrameRef )
{
    REPORTER& reporter = m_MessagesBox->Reporter();

    /* When printing all pages, the printed page is not the current page.  In
     * complex hierarchies, we must update component references and others
     * parameters in the given printed SCH_SCREEN, accordint to the sheet path
     * because in complex hierarchies a SCH_SCREEN (a drawing ) is shared
     * between many sheets and component references depend on the actual sheet
     * path used
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotAll )
        sheetList.BuildSheetList( g_RootSheet );
    else
        sheetList.push_back( m_parent->GetCurrentSheet() );

    wxString fname = sheetList.front().GetUniqueFilename();
    wxString ext = PDF_PLOTTER::GetDefaultFileExtension();
    wxFileName plotFileName = createPlotFileName( m_outputDirectoryName, fname, ext, &reporter );

    wxString pageSize;
    switch( m_pageSizeSelect )
    {
    case PAGE_SIZE_A:    pageSize = wxT( "A" ); break;
    case PAGE_SIZE_A4:   pageSize = wxT( "A4" ); break;
    default:
    case PAGE_SIZE_AUTO: pageSize = wxT( "" ); break;
    }

    PlotSheetListPDF( sheetList, aPlotFrameRef, pageSize, getModeColor(), plotFileName.GetFullPath(), reporter );
}

bool PlotSheetListPDF( SCH_SHEET_LIST& aSheetList, bool aPlotFrameRef, wxString aPageSize, bool aModeColor, wxString aFileName, REPORTER& aReporter )
{
    auto titleBlock = aSheetList.front().LastScreen()->GetTitleBlock();
    PDF_PLOTTER plotter{};
    plotter.SetDefaultLineWidth( GetDefaultLineThickness() );
    plotter.SetColorMode( aModeColor );
    plotter.SetCreator( wxT( "Eeschema-PDF" ) );
    plotter.SetTitle( titleBlock.GetTitle() );

    wxString msg;
    LOCALE_IO toggle;       // Switch the locale to standard C

    try
    {
        if( !plotter.OpenFile( aFileName ) )
        {
            msg.Printf( _( "Unable to create file \"%s\".\n" ),
                        GetChars( aFileName ) );
            aReporter.Report( msg, REPORTER::RPT_ERROR );
            return false;
        }

        bool first = true;
        for( auto& sheet : aSheetList )
        {
            sheet.UpdateAllScreenReferences();
            sheet.UpdatePageNumber();
            auto screen = sheet.LastScreen();

            if( !first )
                plotter.ClosePage();

            PAGE_INFO   plotPage;                               // page size selected to plot
            const PAGE_INFO& actualPage = screen->GetPageSettings(); // page size selected in schematic

            plotPage.SetPortrait( actualPage.IsPortrait() );
            if( aPageSize.empty() )
                plotPage = actualPage;
            else
                plotPage.SetType( aPageSize );

            double  scalex  = (double) plotPage.GetWidthMils() / actualPage.GetWidthMils();
            double  scaley  = (double) plotPage.GetHeightMils() / actualPage.GetHeightMils();
            double  scale   = std::min( scalex, scaley );
            plotter.SetPageSettings( plotPage );
            // Currently, plot units are in decimil
            plotter.SetViewport( wxPoint( 0, 0 ), IU_PER_MILS/10, scale, false );

            if( first )
                plotter.StartPlot();
            else
                plotter.StartPage();

            if( aPlotFrameRef )
            {
                plotter.SetColor( BLACK );
                PlotWorkSheet( &plotter, titleBlock,
                               screen->GetPageSettings(),
                               screen->m_ScreenNumber, screen->m_NumberOfScreens,
                               sheet.PathHumanReadable(),
                               screen->GetFileName(),
                               GetLayerColor( ( SCH_LAYER_ID )LAYER_WORKSHEET ) );
            }

            screen->Plot( &plotter );
            first = false;
        }

        plotter.EndPlot();
    }
    catch( const IO_ERROR& e )
    {
        // Cannot plot PDF file
        msg.Printf( wxT( "PDF Plotter exception: %s" ), GetChars( e.What() ) );
        aReporter.Report( msg, REPORTER::RPT_ERROR );

        plotter.EndPlot();
        return false;
    }

    // Everything done, close the plot and restore the environment
    msg.Printf( _( "Plot: \"%s\" OK.\n" ), GetChars( aFileName ) );
    aReporter.Report( msg, REPORTER::RPT_ACTION );

    return true;
}
