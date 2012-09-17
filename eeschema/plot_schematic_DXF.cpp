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
#include <worksheet.h>
#include <class_sch_screen.h>
#include <wxEeschemaStruct.h>
#include <sch_sheet_path.h>
#include <dialog_plot_schematic.h>


void DIALOG_PLOT_SCHEMATIC::CreateDXFFile( )
{
    SCH_EDIT_FRAME* schframe  = (SCH_EDIT_FRAME*) m_parent;
    SCH_SCREEN*     screen    = schframe->GetScreen();
    SCH_SHEET_PATH* sheetpath;
    SCH_SHEET_PATH  oldsheetpath = schframe->GetCurrentSheet();
    wxString        plotFileName;
    wxPoint         plot_offset;

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and others parameters
     * in the printed SCH_SCREEN
     *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
     *  is shared between many sheets
     */
    SCH_SHEET_LIST SheetList( NULL );

    sheetpath = SheetList.GetFirst();
    SCH_SHEET_PATH list;

    while( true )
    {
        if( m_select_PlotAll )
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

        plot_offset.x = 0;
        plot_offset.y = 0;

        plotFileName = schframe->GetUniqueFilenameForCurrentSheet() + wxT(".")
                       + DXF_PLOTTER::GetDefaultFileExtension();

        PlotOneSheetDXF( plotFileName, screen, plot_offset, 1 );

        if( !m_select_PlotAll )
            break;
    }

    schframe->SetCurrentSheet( oldsheetpath );
    schframe->GetCurrentSheet().UpdateAllScreenReferences();
    schframe->SetSheetNumberAndCount();
}


void DIALOG_PLOT_SCHEMATIC::PlotOneSheetDXF( const wxString&    FileName,
                                                 SCH_SCREEN*        screen,
                                                 wxPoint            plot_offset,
                                                 double             scale )
{


    wxString msg;
    FILE*    output_file = wxFopen( FileName, wxT( "wt" ) );

    if( output_file == NULL )
    {
        msg  = wxT( "\n** " );
        msg += _( "Unable to create " ) + FileName + wxT( " **\n" );
        m_MessagesBox->AppendText( msg );
        return;
    }

    msg.Printf( _( "Plot: %s " ), GetChars( FileName ) );
    m_MessagesBox->AppendText( msg );

    LOCALE_IO   toggle;

    DXF_PLOTTER* plotter = new DXF_PLOTTER();

    const PAGE_INFO&   pageInfo = screen->GetPageSettings();
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( getModeColor() );
    plotter->SetViewport( plot_offset, IU_PER_DECIMILS, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-DXF" ) );
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

    // finish
    plotter->EndPlot();
    delete plotter;

    m_MessagesBox->AppendText( wxT( "Ok\n" ) );
}
