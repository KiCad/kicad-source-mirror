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


void DIALOG_PLOT_SCHEMATIC::createPSFile()
{
    SCH_SCREEN*     screen = m_parent->GetScreen();
    SCH_SHEET_PATH* sheetpath;
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();     // sheetpath is saved here
    wxString        plotFileName;
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
        if( m_select_PlotAll )
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

        double  scale = MIN( scalex, scaley );

        wxPoint plot_offset;
        plotFileName = m_parent->GetUniqueFilenameForCurrentSheet() + wxT( "." )
                       + PS_PLOTTER::GetDefaultFileExtension();

        plotOneSheetPS( plotFileName, screen, plotPage, plot_offset, scale );

        if( !m_select_PlotAll )
            break;
    }

    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


void DIALOG_PLOT_SCHEMATIC::plotOneSheetPS( const wxString&     FileName,
                                            SCH_SCREEN*         screen,
                                            const PAGE_INFO&    pageInfo,
                                            wxPoint             plot_offset,
                                            double              scale )
{
    wxString    msg;

    FILE*       output_file = wxFopen( FileName, wxT( "wt" ) );

    if( output_file == NULL )
    {
        msg = wxT( "\n** " );
        msg += _( "Unable to create " ) + FileName + wxT( " **\n" );
        m_MessagesBox->AppendText( msg );
        return;
    }

    msg.Printf( _( "Plot: %s " ), GetChars( FileName ) );
    m_MessagesBox->AppendText( msg );

    SetLocaleTo_C_standard();
    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->SetPageSettings( pageInfo );
    plotter->SetDefaultLineWidth( g_DrawDefaultLineThickness );
    plotter->SetColorMode( getModeColor() );
    plotter->SetViewport( plot_offset, IU_PER_DECIMILS, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-PS" ) );
    plotter->SetFilename( FileName );
    plotter->StartPlot( output_file );

    if( getPlotFrameRef() )
    {
        plotter->SetColor( BLACK );
        PlotWorkSheet( plotter, m_parent->GetTitleBlock(),
                       m_parent->GetPageSettings(),
                       screen->m_ScreenNumber, screen->m_NumberOfScreens,
                       m_parent->GetScreenDesc(),
                       screen->GetFileName() );
    }

    screen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;
    SetLocaleTo_Default();

    m_MessagesBox->AppendText( wxT( "Ok\n" ) );
}
