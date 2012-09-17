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
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <class_sch_screen.h>
#include <wxEeschemaStruct.h>
#include <dcsvg.h>
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
            msg = _( "Create file " ) + fn.GetFullPath();

            if( !success )
                msg += _( " error" );

            msg += wxT( "\n" );
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
        msg = _( "Create file " ) + fn.GetFullPath();

        if( !success )
            msg += _( " error" );

        msg += wxT( "\n" );
        m_MessagesBox->AppendText( msg );
    }
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetSVG( EDA_DRAW_FRAME*    frame,
                                             const wxString&    FullFileName,
                                             SCH_SCREEN*        aScreen,
                                             bool               aPrintBlackAndWhite,
                                             bool               aPrintFrameRef )
{
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxSize  sheetSize;          // Sheet size in internal units
    wxPoint old_org;
    bool    success = true;

    tmp_startvisu = aScreen->m_StartVisu;
    tmpzoom = aScreen->GetZoom();
    old_org = aScreen->m_DrawOrg;
    aScreen->m_DrawOrg.x     = aScreen->m_DrawOrg.y = 0;
    aScreen->m_StartVisu.x   = aScreen->m_StartVisu.y = 0;

    sheetSize = aScreen->GetPageSettings().GetSizeIU();
    aScreen->SetScalingFactor( 1.0 );
    EDA_DRAW_PANEL* panel = frame->GetCanvas();

    LOCALE_IO       toggle;

    double          dpi = 1000.0 * IU_PER_MILS;
    wxPoint         origin;
    KicadSVGFileDC  dc( FullFileName, origin, sheetSize, dpi );

    EDA_RECT        tmp = *panel->GetClipBox();
    GRResetPenAndBrush( &dc );
    GRForceBlackPen( aPrintBlackAndWhite );


    panel->SetClipBox( EDA_RECT( wxPoint( -0x3FFFFF0, -0x3FFFFF0 ),
                                 wxSize( 0x7FFFFF0, 0x7FFFFF0 ) ) );

    aScreen->m_IsPrinting = true;

    if( frame->IsType( SCHEMATIC_FRAME_TYPE ) )
        aScreen->Draw( panel, &dc, GR_COPY );

    if( frame->IsType( LIBEDITOR_FRAME_TYPE ) )
        ( (LIB_EDIT_FRAME*) frame )->RedrawComponent( &dc,
                                                      wxPoint( sheetSize.x / 2,
                                                               sheetSize.y / 2 ) );

    if( aPrintFrameRef )
        frame->TraceWorkSheet( &dc, aScreen, g_DrawDefaultLineThickness,
                               IU_PER_MILS, frame->GetScreenDesc() );

    aScreen->m_IsPrinting = false;
    panel->SetClipBox( tmp );

    GRForceBlackPen( false );

    aScreen->m_StartVisu = tmp_startvisu;
    aScreen->m_DrawOrg   = old_org;
    aScreen->SetZoom( tmpzoom );

    return success;
}
