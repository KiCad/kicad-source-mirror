/** @file plot_schematic_PS.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
#include <reporter.h>


void DIALOG_PLOT_SCHEMATIC::createPSFile( bool aPlotAll, bool aPlotFrameRef )
{
    SCH_SCREEN*     screen = m_parent->GetScreen();
    SCH_SHEET_PATH* sheetpath;
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();     // sheetpath is saved here
    PAGE_INFO       actualPage;                                     // page size selected in schematic
    PAGE_INFO       plotPage;                                       // page size selected to plot

    /* When printing all pages, the printed page is not the current page.
     * In complex hierarchies, we must update component references
     *  and others parameters in the given printed SCH_SCREEN, accordint to the sheet path
     *  because in complex hierarchies a SCH_SCREEN (a drawing )
     *  is shared between many sheets and component references depend on the actual sheet path used
     */
    SCH_SHEET_LIST  SheetList( NULL );

    sheetpath = SheetList.GetFirst();
    SCH_SHEET_PATH  list;

    while( true )
    {
        if( aPlotAll )
        {
            if( sheetpath == NULL )
                break;

            list.Clear();

            if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
            {
                m_parent->SetCurrentSheet( list );
                m_parent->GetCurrentSheet().UpdateAllScreenReferences();
                m_parent->SetSheetNumberAndCount();
                screen = m_parent->GetCurrentSheet().LastScreen();
            }
            else // Should not happen
                return;

            sheetpath = SheetList.GetNext();
        }

        actualPage = screen->GetPageSettings();

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

        double  scale = std::min( scalex, scaley );

        wxPoint plot_offset;

        wxString outputDirName = m_outputDirectoryName->GetValue();
        wxFileName outputDir = wxFileName::DirName( outputDirName );

        wxString msg;
        WX_TEXT_CTRL_REPORTER reporter(m_MessagesBox);

        try
        {
            wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();
            wxString ext = PS_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( m_outputDirectoryName,
                                                          fname, ext, &reporter );

            if( plotOneSheetPS( plotFileName.GetFullPath(), screen, plotPage, plot_offset,
                                scale, aPlotFrameRef ) )
            {
                msg.Printf( _( "Plot: '%s' OK\n" ), GetChars( plotFileName.GetFullPath() ) );
            }
            else
            {
                // Error
                msg.Printf( _( "Unable to create '%s'\n" ), GetChars( plotFileName.GetFullPath() ) );
            }

            m_MessagesBox->AppendText( msg );
        }
        catch (IO_ERROR& e)
        {
            msg.Printf( _( "PS Plotter Exception : '%s'"), wxString(e.errorText ) );
            m_MessagesBox->AppendText( msg );
        }

        if( !aPlotAll )
            break;
    }

    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetPS( const wxString&     aFileName,
                                            SCH_SCREEN*         aScreen,
                                            const PAGE_INFO&    aPageInfo,
                                            wxPoint             aPlot0ffset,
                                            double              aScale,
                                            bool                aPlotFrameRef )
{
    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->SetPageSettings( aPageInfo );
    plotter->SetDefaultLineWidth( GetDefaultLineThickness() );
    plotter->SetColorMode( getModeColor() );
    plotter->SetViewport( aPlot0ffset, IU_PER_DECIMILS, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-PS" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    SetLocaleTo_C_standard();
    plotter->StartPlot();

    if( aPlotFrameRef )
    {
        plotter->SetColor( BLACK );
        PlotWorkSheet( plotter, m_parent->GetTitleBlock(),
                       m_parent->GetPageSettings(),
                       aScreen->m_ScreenNumber, aScreen->m_NumberOfScreens,
                       m_parent->GetScreenDesc(),
                       aScreen->GetFileName() );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;
    SetLocaleTo_Default();

    return true;
}
