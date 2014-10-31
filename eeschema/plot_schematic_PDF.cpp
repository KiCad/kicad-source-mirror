/** @file plot_schematic_PDF.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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
#include <plot_common.h>
#include <class_sch_screen.h>
#include <wxEeschemaStruct.h>
#include <base_units.h>
#include <sch_sheet_path.h>
#include <dialog_plot_schematic.h>
#include <project.h>


void DIALOG_PLOT_SCHEMATIC::createPDFFile( bool aPlotAll, bool aPlotFrameRef )
{
    SCH_SCREEN*     screen = m_parent->GetScreen();
    SCH_SHEET_PATH* sheetpath;
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();     // sheetpath is saved here
    wxPoint         plot_offset;

    /* When printing all pages, the printed page is not the current page.  In
     * complex hierarchies, we must update component references and others
     * parameters in the given printed SCH_SCREEN, accordint to the sheet path
     * because in complex hierarchies a SCH_SCREEN (a drawing ) is shared
     * between many sheets and component references depend on the actual sheet
     * path used
     */
    SCH_SHEET_LIST SheetList( NULL );

    sheetpath = SheetList.GetFirst();

    // Allocate the plotter and set the job level parameter
    PDF_PLOTTER* plotter = new PDF_PLOTTER();
    plotter->SetDefaultLineWidth( GetDefaultLineThickness() );
    plotter->SetColorMode( getModeColor() );
    plotter->SetCreator( wxT( "Eeschema-PDF" ) );

    wxString msg;
    wxFileName plotFileName;
    WX_TEXT_CTRL_REPORTER reporter(m_MessagesBox);

    // First page handling is different
    bool first_page = true;
    do
    {
        // Step over the schematic hierarchy
        if( aPlotAll )
        {
            SCH_SHEET_PATH list;

            if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
            {
                m_parent->SetCurrentSheet( list );
                m_parent->GetCurrentSheet().UpdateAllScreenReferences();
                m_parent->SetSheetNumberAndCount();
                screen = m_parent->GetCurrentSheet().LastScreen();
            }
            else // Should not happen
                wxASSERT( 0 );

            sheetpath = SheetList.GetNext();
        }

        if( first_page )
        {

            try
            {
                wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();
                wxString ext = PDF_PLOTTER::GetDefaultFileExtension();
                plotFileName = createPlotFileName( m_outputDirectoryName,
                                                   fname, ext, &reporter );

                if( !plotter->OpenFile( plotFileName.GetFullPath() ) )
                {
                    msg.Printf( _( "Unable to create '%s'\n" ), GetChars( plotFileName.GetFullPath() ) );
                    m_MessagesBox->AppendText( msg );
                    delete plotter;
                    return;
                }

                // Open the plotter and do the first page
                SetLocaleTo_C_standard();
                setupPlotPagePDF( plotter, screen );
                plotter->StartPlot();
                first_page = false;

            }
            catch( const IO_ERROR& e )
            {
                // Cannot plot PDF file
                msg.Printf( wxT( "PDF Plotter Exception : <%s>"), GetChars( e.errorText ) );
                restoreEnvironment(plotter, oldsheetpath, msg);
                return;
            }

        }
        else
        {
            /* For the following pages you need to close the (finished) page,
             *  reconfigure, and then start a new one */
            plotter->ClosePage();
            setupPlotPagePDF( plotter, screen );
            plotter->StartPage();
        }

        plotOneSheetPDF( plotter, screen, aPlotFrameRef );
    } while( aPlotAll && sheetpath );

    // Everything done, close the plot and restore the environment
    msg.Printf( _( "Plot: <%s> OK\n" ), GetChars( plotFileName.GetFullPath() ) );
    restoreEnvironment(plotter, oldsheetpath, msg);

}


void DIALOG_PLOT_SCHEMATIC::restoreEnvironment( PDF_PLOTTER* aPlotter,
                            SCH_SHEET_PATH aOldsheetpath, const wxString& aMsg )
{
    aPlotter->EndPlot();
    delete aPlotter;
    SetLocaleTo_Default();

    // Restore the previous sheet
    m_parent->SetCurrentSheet( aOldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();

    m_MessagesBox->AppendText( aMsg );
}


void DIALOG_PLOT_SCHEMATIC::plotOneSheetPDF( PLOTTER* aPlotter,
                                             SCH_SCREEN* aScreen,
                                             bool aPlotFrameRef )
{
    if( aPlotFrameRef )
    {
        aPlotter->SetColor( BLACK );
        PlotWorkSheet( aPlotter, m_parent->GetTitleBlock(),
                       m_parent->GetPageSettings(),
                       aScreen->m_ScreenNumber, aScreen->m_NumberOfScreens,
                       m_parent->GetScreenDesc(),
                       aScreen->GetFileName() );
    }

    aScreen->Plot( aPlotter );
}


void DIALOG_PLOT_SCHEMATIC::setupPlotPagePDF( PLOTTER * aPlotter, SCH_SCREEN* aScreen )
{
    PAGE_INFO   plotPage;                               // page size selected to plot
    // Considerations on page size and scaling requests
    PAGE_INFO   actualPage = aScreen->GetPageSettings(); // page size selected in schematic

    switch( m_pageSizeSelect )
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
    aPlotter->SetViewport( wxPoint( 0, 0 ), IU_PER_DECIMILS, scale, false );
}

