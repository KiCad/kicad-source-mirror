/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <class_sch_screen.h>
#include <wxEeschemaStruct.h>
#include <base_units.h>
#include <libeditframe.h>
#include <sch_sheet_path.h>

#include <dialog_plot_schematic.h>


void DIALOG_PLOT_SCHEMATIC::createSVGFile( bool aPrintAll, bool aPrintFrameRef )
{
    wxString    msg;
    wxFileName  fn;

    if( aPrintAll )
    {
        SCH_SHEET_PATH* sheetpath;
        SCH_SHEET_PATH  oldsheetpath    = m_parent->GetCurrentSheet();
        SCH_SHEET_LIST  SheetList( NULL );
        sheetpath = SheetList.GetFirst();
        SCH_SHEET_PATH  list;

        for( ; ; )
        {
            if( sheetpath == NULL )
                break;

            SCH_SCREEN*  screen;
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

            fn = m_parent->GetUniqueFilenameForCurrentSheet() + wxT( ".svg" );

            bool success = plotOneSheetSVG( m_parent, fn.GetFullPath(), screen,
                                            getModeColor() ? false : true,
                                            aPrintFrameRef );

            if( !success )
            {
                msg.Printf( _( "Error creating file <%s>\n" ),
                            GetChars( fn.GetFullPath() ) );
            }
            else
            {
                msg.Printf( _( "File <%s> OK\n" ),
                            GetChars( fn.GetFullPath() ) );
            }

            m_MessagesBox->AppendText( msg );
        }

        m_parent->SetCurrentSheet( oldsheetpath );
        m_parent->GetCurrentSheet().UpdateAllScreenReferences();
        m_parent->SetSheetNumberAndCount();
    }
    else    // Print current sheet
    {
        SCH_SCREEN* screen = (SCH_SCREEN*) m_parent->GetScreen();

        fn = screen->GetFileName();
        fn.SetExt( wxT( "svg" ) );
        fn.MakeAbsolute();

        bool success = plotOneSheetSVG( m_parent, fn.GetFullPath(), screen,
                                        getModeColor() ? false : true,
                                        aPrintFrameRef );
        if( success )
            msg.Printf( _( "Plot: <%s> OK\n" ),
                        GetChars( fn.GetFullPath() ) );
        else    // Error
             msg.Printf( _( "Unable to create <%s>\n" ),
                        GetChars( fn.GetFullPath() ) );

        m_MessagesBox->AppendText( msg );
    }
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetSVG( EDA_DRAW_FRAME*    aFrame,
                                             const wxString&    aFileName,
                                             SCH_SCREEN*        aScreen,
                                             bool               aPlotBlackAndWhite,
                                             bool               aPlotFrameRef )
{
    SVG_PLOTTER* plotter = new SVG_PLOTTER();

    const PAGE_INFO&   pageInfo = aScreen->GetPageSettings();
    plotter->SetPageSettings( pageInfo );
    plotter->SetDefaultLineWidth( GetDefaultLineThickness() );
    plotter->SetColorMode( aPlotBlackAndWhite ? false : true );
    wxPoint plot_offset;
    double scale = 1.0;
    plotter->SetViewport( plot_offset, IU_PER_DECIMILS, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-SVG" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO   toggle;

    plotter->StartPlot();

    if( aPlotFrameRef )
    {
        plotter->SetColor( BLACK );
        PlotWorkSheet( plotter, aFrame->GetTitleBlock(),
                       aFrame->GetPageSettings(),
                       aScreen->m_ScreenNumber, aScreen->m_NumberOfScreens,
                       aFrame->GetScreenDesc(),
                       aScreen->GetFileName() );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;

    return true;
}
