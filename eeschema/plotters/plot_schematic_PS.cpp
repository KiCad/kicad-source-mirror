/** @file plot_schematic_PS.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see change_log.txt for contributors.
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

#include <plotters_specific.h>
#include <sch_edit_frame.h>
#include <base_units.h>
#include <sch_sheet_path.h>
#include <locale_io.h>
#include <pgm_base.h>
#include <project.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <sch_painter.h>
#include <schematic.h>
#include <dialog_plot_schematic.h>
#include <wx_html_report_panel.h>


void DIALOG_PLOT_SCHEMATIC::createPSFile( bool aPlotAll, bool aPlotFrameRef,
                                          RENDER_SETTINGS* aRenderSettings )
{
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();  // sheetpath is saved here
    PAGE_INFO       plotPage;                                    // page size selected to plot

    /* When printing all pages, the printed page is not the current page.
     * In complex hierarchies, we must update component references
     *  and others parameters in the given printed SCH_SCREEN, accordant to the sheet path
     *  because in complex hierarchies a SCH_SCREEN (a drawing )
     *  is shared between many sheets and component references depend on the actual sheet path used
     */
    SCH_SHEET_LIST  sheetList;

    if( aPlotAll )
    {
        sheetList.BuildSheetList( &m_parent->Schematic().Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_parent->GetCurrentSheet() );
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_parent->SetCurrentSheet( sheetList[i] );
        m_parent->GetCurrentSheet().UpdateAllScreenReferences();
        m_parent->SetSheetNumberAndCount();

        SCH_SCREEN* screen = m_parent->GetCurrentSheet().LastScreen();
        PAGE_INFO   actualPage = screen->GetPageSettings();

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

        wxString msg;
        REPORTER& reporter = m_MessagesBox->Reporter();

        try
        {
            wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();
            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace("/", "_" );
            fname.Replace("\\", "_" );
            wxString ext = PS_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( fname, ext, &reporter );

            if( plotOneSheetPS( plotFileName.GetFullPath(), screen, aRenderSettings, plotPage,
                                plot_offset, scale, aPlotFrameRef ) )
            {
                msg.Printf( _( "Plot: \"%s\" OK.\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ACTION );
            }
            else
            {
                // Error
                msg.Printf( _( "Unable to create file \"%s\".\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ERROR );
            }

        }
        catch( IO_ERROR& e )
        {
            msg.Printf( wxT( "PS Plotter exception: %s"), e.What() );
            reporter.Report( msg, RPT_SEVERITY_ERROR );
        }
    }

    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetPS( const wxString&     aFileName,
                                            SCH_SCREEN*         aScreen,
                                            RENDER_SETTINGS*    aRenderSettings,
                                            const PAGE_INFO&    aPageInfo,
                                            wxPoint             aPlot0ffset,
                                            double              aScale,
                                            bool                aPlotFrameRef )
{
    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( aPageInfo );
    plotter->SetColorMode( getModeColor() );
    // Currently, plot units are in decimil
    plotter->SetViewport( aPlot0ffset, IU_PER_MILS/10, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-PS" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO toggle;       // Switch the locale to standard C

    plotter->StartPlot();

    if( m_plotBackgroundColor->GetValue() )
    {
        plotter->SetColor( plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
        wxPoint end( plotter->PageSettings().GetWidthIU(),
                     plotter->PageSettings().GetHeightIU() );
        plotter->Rect( wxPoint( 0, 0 ), end, FILL_TYPE::FILLED_SHAPE, 1.0 );
    }

    if( aPlotFrameRef )
    {
        PlotDrawingSheet( plotter, &aScreen->Schematic()->Prj(), m_parent->GetTitleBlock(),
                          aPageInfo, aScreen->GetPageNumber(), aScreen->GetPageCount(),
                          m_parent->GetScreenDesc(), aScreen->GetFileName(),
                          plotter->GetColorMode() ?
                          plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) :
                          COLOR4D::BLACK, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;

    return true;
}
