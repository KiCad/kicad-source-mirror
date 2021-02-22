/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file plot_schematic_SVG.cpp
 */

#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <base_units.h>
#include <locale_io.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <project.h>
#include <reporter.h>
#include <settings/settings_manager.h>

#include <dialog_plot_schematic.h>
#include <wx_html_report_panel.h>
#include "sch_painter.h"
#include <plotters_specific.h>


void DIALOG_PLOT_SCHEMATIC::createSVGFile( bool aPrintAll, bool aPrintFrameRef,
                                           RENDER_SETTINGS* aRenderSettings )
{
    wxString        msg;
    REPORTER&       reporter = m_MessagesBox->Reporter();
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();
    SCH_SHEET_LIST  sheetList;

    if( aPrintAll )
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
        SCH_SCREEN*  screen;
        m_parent->SetCurrentSheet( sheetList[i] );
        m_parent->GetCurrentSheet().UpdateAllScreenReferences();
        m_parent->SetSheetNumberAndCount();
        screen = m_parent->GetCurrentSheet().LastScreen();

        try
        {
            wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();
            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace("/", "_" );
            fname.Replace("\\", "_" );
            wxString ext = SVG_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( fname, ext, &reporter );

            bool success = plotOneSheetSVG( plotFileName.GetFullPath(), screen, aRenderSettings,
                                            getModeColor() ? false : true, aPrintFrameRef );

            if( !success )
            {
                msg.Printf( _( "Cannot create file \"%s\".\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ERROR );
            }
            else
            {
                msg.Printf( _( "Plot: \"%s\" OK.\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ACTION );
            }
        }
        catch( const IO_ERROR& e )
        {
            // Cannot plot SVG file
            msg.Printf( wxT( "SVG Plotter exception: %s" ), e.What() );
            reporter.Report( msg, RPT_SEVERITY_ERROR );
            break;
        }
    }

    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetSVG( const wxString&  aFileName,
                                             SCH_SCREEN*      aScreen,
                                             RENDER_SETTINGS* aRenderSettings,
                                             bool             aPlotBlackAndWhite,
                                             bool             aPlotFrameRef )
{
    const PAGE_INFO& pageInfo = aScreen->GetPageSettings();

    SVG_PLOTTER* plotter = new SVG_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( aPlotBlackAndWhite ? false : true );
    wxPoint plot_offset;
    double scale = 1.0;
    // Currently, plot units are in decimil
    plotter->SetViewport( plot_offset, IU_PER_MILS/10, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-SVG" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO   toggle;

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
                          pageInfo, aScreen->GetPageNumber(), aScreen->GetPageCount(),
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
