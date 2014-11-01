/** @file plot_schematic_HPGL.cpp
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
#include <project.h>

#include <dialog_plot_schematic.h>


enum HPGL_PAGEZ_T {
    PAGE_DEFAULT = 0,
    HPGL_PAGE_SIZE_A4,
    HPGL_PAGE_SIZE_A3,
    HPGL_PAGE_SIZE_A2,
    HPGL_PAGE_SIZE_A1,
    HPGL_PAGE_SIZE_A0,
    HPGL_PAGE_SIZE_A,
    HPGL_PAGE_SIZE_B,
    HPGL_PAGE_SIZE_C,
    HPGL_PAGE_SIZE_D,
    HPGL_PAGE_SIZE_E,
};


static const wxChar* plot_sheet_list( int aSize )
{
    const wxChar* ret;

    switch( aSize )
    {
    default:
    case PAGE_DEFAULT:
        ret = NULL;         break;

    case HPGL_PAGE_SIZE_A4:
        ret = wxT( "A4" );  break;

    case HPGL_PAGE_SIZE_A3:
        ret = wxT( "A3" );  break;

    case HPGL_PAGE_SIZE_A2:
        ret = wxT( "A2" );  break;

    case HPGL_PAGE_SIZE_A1:
        ret = wxT( "A1" );  break;

    case HPGL_PAGE_SIZE_A0:
        ret = wxT( "A0" );  break;

    case HPGL_PAGE_SIZE_A:
        ret = wxT( "A" );   break;

    case HPGL_PAGE_SIZE_B:
        ret = wxT( "B" );   break;

    case HPGL_PAGE_SIZE_C:
        ret = wxT( "C" );   break;

    case HPGL_PAGE_SIZE_D:
        ret = wxT( "D" );   break;

    case HPGL_PAGE_SIZE_E:
        ret = wxT( "E" );   break;
    }

    return ret;
};


void DIALOG_PLOT_SCHEMATIC::SetHPGLPenWidth()
{
    m_HPGLPenSize = ValueFromTextCtrl( *m_penHPGLWidthCtrl );

    if( m_HPGLPenSize > Millimeter2iu( 2 ) )
        m_HPGLPenSize = Millimeter2iu( 2 );

    if( m_HPGLPenSize < Millimeter2iu( 0.01 ) )
        m_HPGLPenSize = Millimeter2iu( 0.01 );
}


void DIALOG_PLOT_SCHEMATIC::createHPGLFile( bool aPlotAll, bool aPlotFrameRef )
{
    SCH_SCREEN*     screen = m_parent->GetScreen();
    SCH_SHEET_PATH* sheetpath;
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and other parameters
     *  in the printed SCH_SCREEN
     *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
     *  is shared between many sheets
     */
    SCH_SHEET_LIST  SheetList( NULL );

    sheetpath = SheetList.GetFirst();
    SCH_SHEET_PATH  list;
    WX_TEXT_CTRL_REPORTER reporter(m_MessagesBox);

    SetHPGLPenWidth();

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

                if( !screen ) // LastScreen() may return NULL
                    screen = m_parent->GetScreen();
            }
            else // Should not happen
                return;

            sheetpath = SheetList.GetNext();
        }

        const PAGE_INFO&    curPage = screen->GetPageSettings();

        PAGE_INFO           plotPage = curPage;

        // if plotting on a page size other than curPage
        if( m_HPGLPaperSizeOption->GetSelection() != PAGE_DEFAULT )
            plotPage.SetType( plot_sheet_list( m_HPGLPaperSizeOption->GetSelection() ) );

        // Calculation of conversion scales.
        double  plot_scale = (double) plotPage.GetWidthMils() / curPage.GetWidthMils();

        // Calculate offsets
        wxPoint plotOffset;
        wxString msg;

        if( GetPlotOriginCenter() )
        {
            plotOffset.x    = plotPage.GetWidthIU() / 2;
            plotOffset.y    = -plotPage.GetHeightIU() / 2;
        }

        try
        {
            wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();
            wxString ext = HPGL_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( m_outputDirectoryName, fname,
                                                          ext, &reporter );

            LOCALE_IO toggle;

            if( Plot_1_Page_HPGL( plotFileName.GetFullPath(), screen, plotPage, plotOffset,
                                plot_scale, aPlotFrameRef ) )
                msg.Printf( _( "Plot: '%s' OK\n" ), GetChars( plotFileName.GetFullPath() ) );
            else    // Error
                msg.Printf( _( "Unable to create '%s'\n" ), GetChars( plotFileName.GetFullPath() ) );

            m_MessagesBox->AppendText( msg );

            if( !aPlotAll )
                break;
        }
        catch( IO_ERROR& e )
        {
            msg.Printf( wxT( "HPGL Plotter Exception : '%s'"), GetChars( e.errorText ) );
            m_MessagesBox->AppendText( msg );
        }

    }

    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::Plot_1_Page_HPGL( const wxString&   aFileName,
                                              SCH_SCREEN*       aScreen,
                                              const PAGE_INFO&  aPageInfo,
                                              wxPoint           aPlot0ffset,
                                              double            aScale,
                                              bool              aPlotFrameRef )
{
    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();

    plotter->SetPageSettings( aPageInfo );
    plotter->SetViewport( aPlot0ffset, IU_PER_DECIMILS, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-HPGL" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO toggle;

    // Pen num and pen speed are not initialized here.
    // Default HPGL driver values are used
    plotter->SetPenDiameter( m_HPGLPenSize );
    plotter->SetPenOverlap( m_HPGLPenSize / 4 );
    plotter->StartPlot();

    plotter->SetColor( BLACK );

    if( getPlotFrameRef() )
        PlotWorkSheet( plotter, m_parent->GetTitleBlock(),
                       m_parent->GetPageSettings(),
                       aScreen->m_ScreenNumber, aScreen->m_NumberOfScreens,
                       m_parent->GetScreenDesc(),
                       aScreen->GetFileName() );

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;

    return true;
}
