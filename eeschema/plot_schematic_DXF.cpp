/** @file plot_schematic_DXF.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
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
#include <sch_sheet_path.h>
#include <dialog_plot_schematic.h>
#include <project.h>


void DIALOG_PLOT_SCHEMATIC::CreateDXFFile( bool aPlotAll, bool aPlotFrameRef )
{
    SCH_EDIT_FRAME* schframe  = (SCH_EDIT_FRAME*) m_parent;
    SCH_SCREEN*     screen    = schframe->GetScreen();
    SCH_SHEET_PATH* sheetpath;
    SCH_SHEET_PATH  oldsheetpath = schframe->GetCurrentSheet();

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and others parameters
     * in the printed SCH_SCREEN
     *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
     *  is shared between many sheets
     */
    SCH_SHEET_LIST SheetList( NULL );

    sheetpath = SheetList.GetFirst();
    SCH_SHEET_PATH list;
    WX_TEXT_CTRL_REPORTER reporter(m_MessagesBox);

    while( true )
    {
        if( aPlotAll )
        {
            if( sheetpath == NULL )
                break;

            list.Clear();

            if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
            {
                schframe->SetCurrentSheet( list );
                schframe->GetCurrentSheet().UpdateAllScreenReferences();
                schframe->SetSheetNumberAndCount();
                screen = schframe->GetCurrentSheet().LastScreen();
            }
            else  // Should not happen
            {
                return;
            }

            sheetpath = SheetList.GetNext();
        }

        wxPoint plot_offset;
        wxString msg;

        try
        {
            wxString fname = schframe->GetUniqueFilenameForCurrentSheet();
            wxString ext = DXF_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( m_outputDirectoryName, fname,
                                                          ext, &reporter );

            if( PlotOneSheetDXF( plotFileName.GetFullPath(), screen, plot_offset, 1.0, aPlotFrameRef ) )
            {
                msg.Printf( _( "Plot: '%s' OK\n" ), GetChars( plotFileName.GetFullPath() ) );
            }
            else    // Error
            {
                msg.Printf( _( "Unable to create '%s'\n" ), GetChars( plotFileName.GetFullPath() ) );
            }
            m_MessagesBox->AppendText( msg );

        }
        catch (IO_ERROR& e)
        {
            msg.Printf( _( "DXF Plotter Exception : '%s'"), wxString(e.errorText ) );
            m_MessagesBox->AppendText( msg );
            schframe->SetCurrentSheet( oldsheetpath );
            schframe->GetCurrentSheet().UpdateAllScreenReferences();
            schframe->SetSheetNumberAndCount();
            return;
        }
        if( !aPlotAll )
        {
            break;
        }
    }

    schframe->SetCurrentSheet( oldsheetpath );
    schframe->GetCurrentSheet().UpdateAllScreenReferences();
    schframe->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::PlotOneSheetDXF( const wxString&    aFileName,
                                             SCH_SCREEN*        aScreen,
                                             wxPoint            aPlotOffset,
                                             double             aScale,
                                             bool aPlotFrameRef )
{
    DXF_PLOTTER* plotter = new DXF_PLOTTER();

    const PAGE_INFO&   pageInfo = aScreen->GetPageSettings();
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( getModeColor() );
    plotter->SetViewport( aPlotOffset, IU_PER_DECIMILS, aScale, false );

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
        PlotWorkSheet( plotter, m_parent->GetTitleBlock(),
                       m_parent->GetPageSettings(),
                       aScreen->m_ScreenNumber, aScreen->m_NumberOfScreens,
                       m_parent->GetScreenDesc(),
                       aScreen->GetFileName() );
    }

    aScreen->Plot( plotter );

    // finish
    plotter->EndPlot();
    delete plotter;

    return true;
}
