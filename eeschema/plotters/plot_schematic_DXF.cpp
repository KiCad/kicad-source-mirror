/** @file plot_schematic_DXF.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
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

#include <locale_io.h>
#include <plotters_specific.h>
#include <sch_edit_frame.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <project.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <sch_painter.h>
#include <dialog_plot_schematic.h>
#include <wx_html_report_panel.h>


void DIALOG_PLOT_SCHEMATIC::CreateDXFFile( bool aPlotAll, bool aPlotDrawingSheet,
                                           RENDER_SETTINGS*  aRenderSettings )
{
    SCH_EDIT_FRAME* schframe  = m_parent;
    SCH_SHEET_PATH  oldsheetpath = schframe->GetCurrentSheet();

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and others parameters
     * in the printed SCH_SCREEN
     *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
     *  is shared between many sheets
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotAll )
    {
        sheetList.BuildSheetList( &schframe->Schematic().Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( schframe->GetCurrentSheet() );
    }

    REPORTER& reporter = m_MessagesBox->Reporter();

    for( unsigned i = 0; i < sheetList.size();  i++ )
    {
        schframe->SetCurrentSheet( sheetList[i] );
        schframe->GetCurrentSheet().UpdateAllScreenReferences();
        schframe->SetSheetNumberAndCount();

        SCH_SCREEN* screen = schframe->GetCurrentSheet().LastScreen();
        wxPoint     plot_offset;
        wxString    msg;

        try
        {
            wxString fname = schframe->GetUniqueFilenameForCurrentSheet();
            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace("/", "_" );
            fname.Replace("\\", "_" );
            wxString ext = DXF_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( fname, ext, &reporter );

            if( PlotOneSheetDXF( plotFileName.GetFullPath(), screen, aRenderSettings,
                                 plot_offset, 1.0, aPlotDrawingSheet ) )
            {
                msg.Printf( _( "Plot: \"%s\" OK.\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ACTION );
            }
            else    // Error
            {
                msg.Printf( _( "Unable to create file \"%s\".\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ERROR );
            }
        }
        catch( IO_ERROR& e )
        {
            msg.Printf( wxT( "DXF Plotter exception: %s"), e.What() );
            reporter.Report( msg, RPT_SEVERITY_ERROR );
            schframe->SetCurrentSheet( oldsheetpath );
            schframe->GetCurrentSheet().UpdateAllScreenReferences();
            schframe->SetSheetNumberAndCount();
            return;
        }
    }

    schframe->SetCurrentSheet( oldsheetpath );
    schframe->GetCurrentSheet().UpdateAllScreenReferences();
    schframe->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::PlotOneSheetDXF( const wxString&  aFileName,
                                             SCH_SCREEN*      aScreen,
                                             RENDER_SETTINGS* aRenderSettings,
                                             wxPoint          aPlotOffset,
                                             double           aScale,
                                             bool             aPlotFrameRef )
{
    aRenderSettings->LoadColors( getColorSettings() );
    aRenderSettings->SetDefaultPenWidth( 0 );

    const PAGE_INFO& pageInfo = aScreen->GetPageSettings();
    DXF_PLOTTER*     plotter = new DXF_PLOTTER();

    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( getModeColor() );
    // Currently, plot units are in decimil
    plotter->SetViewport( aPlotOffset, IU_PER_MILS/10, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-DXF" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO   toggle;

    plotter->StartPlot();

    if( aPlotFrameRef )
    {
        PlotDrawingSheet( plotter, &m_parent->Prj(), m_parent->GetTitleBlock(), pageInfo,
                          aScreen->GetPageNumber(), aScreen->GetPageCount(),
                          m_parent->GetScreenDesc(), aScreen->GetFileName(),
                          plotter->GetColorMode() ?
                          plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) :
                          COLOR4D::BLACK, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    // finish
    plotter->EndPlot();
    delete plotter;

    return true;
}
