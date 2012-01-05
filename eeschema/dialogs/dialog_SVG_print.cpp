/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file eeschema/dialogs/dialog_SVG_print.cpp
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"
#include "dcsvg.h"
#include "general.h"
#include "libeditframe.h"
#include "sch_sheet_path.h"

#include "dialog_SVG_print.h"


// Keys for configuration
#define PLOTSVGMODECOLOR_KEY wxT( "PlotSVGModeColor" )

#define WIDTH_MAX_VALUE 100
#define WIDTH_MIN_VALUE 1

// Variables locales
static bool s_Print_Frame_Ref      = true;
static int  s_PlotBlackAndWhite    = 0;


/*!
 * DIALOG_SVG_PRINT functions
 */
DIALOG_SVG_PRINT::DIALOG_SVG_PRINT( EDA_DRAW_FRAME* parent ) :
    DIALOG_SVG_PRINT_base( parent )
{
    m_Parent   = parent;
    m_Config   = wxGetApp().GetSettings();
}


void DIALOG_SVG_PRINT::OnInitDialog( wxInitDialogEvent& event )
{
    SetFocus();     // Make ESC key working

    if( m_Config )
    {
        m_Config->Read( PLOTSVGMODECOLOR_KEY, &s_PlotBlackAndWhite );
    }

    m_ModeColorOption->SetSelection( s_PlotBlackAndWhite );

    AddUnitSymbol( *m_TextPenWidth, g_UserUnit );
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UserUnit, g_DrawDefaultLineThickness,
                               m_Parent->GetInternalUnits() ) );
    m_Print_Sheet_Ref->SetValue( s_Print_Frame_Ref );

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


void DIALOG_SVG_PRINT::SetPenWidth()
{
    g_DrawDefaultLineThickness =
    ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->GetInternalUnits() );

    if( g_DrawDefaultLineThickness > WIDTH_MAX_VALUE )
    {
        g_DrawDefaultLineThickness = WIDTH_MAX_VALUE;
    }

    if( g_DrawDefaultLineThickness < WIDTH_MIN_VALUE )
    {
        g_DrawDefaultLineThickness = WIDTH_MIN_VALUE;
    }

    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UserUnit, g_DrawDefaultLineThickness,
                               m_Parent->GetInternalUnits() ) );
}


void DIALOG_SVG_PRINT::PrintSVGDoc( bool aPrintAll, bool aPrint_Sheet_Ref )
{
    wxString    msg;
    wxFileName  fn;

    SetPenWidth();

    g_DrawDefaultLineThickness =
    ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->GetInternalUnits() );

    SCH_SCREEN* screen = (SCH_SCREEN*) m_Parent->GetScreen();

    if( aPrintAll && m_Parent->IsType( SCHEMATIC_FRAME ) )
    {
        SCH_EDIT_FRAME*  schframe = (SCH_EDIT_FRAME*) m_Parent;
        SCH_SHEET_PATH*  sheetpath;
        SCH_SHEET_PATH   oldsheetpath = schframe->GetCurrentSheet();
        SCH_SCREEN*      schscreen = schframe->GetScreen();
        SCH_SHEET_LIST   SheetList( NULL );
        sheetpath = SheetList.GetFirst();
        SCH_SHEET_PATH   list;

        for( ; ; )
        {
            if( sheetpath == NULL )
                break;

            list.Clear();

            if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
            {
                schframe->SetCurrentSheet( list );
                schframe->GetCurrentSheet().UpdateAllScreenReferences();
                schframe->SetSheetNumberAndCount();
                schscreen = schframe->GetCurrentSheet().LastScreen();
            }
            else  // Should not happen
                return;

            sheetpath = SheetList.GetNext();

            fn = schframe->GetUniqueFilenameForCurrentSheet() + wxT( ".svg" );

            bool success = DrawSVGPage( m_Parent, fn.GetFullPath(), ( SCH_SCREEN* ) schscreen,
                                        m_ModeColorOption->GetSelection() == 0 ? false : true,
                                        aPrint_Sheet_Ref );
            msg = _( "Create file " ) + fn.GetFullPath();
            if( !success )
                msg += _( " error" );
            msg += wxT( "\n" );
            m_MessagesBox->AppendText( msg );
        }

        schframe->SetCurrentSheet( oldsheetpath );
        schframe->GetCurrentSheet().UpdateAllScreenReferences();
        schframe->SetSheetNumberAndCount();
    }
    else
    {
        fn = m_FileNameCtrl->GetValue();

        if( !fn.IsOk() )
            fn = screen->GetFileName();

        fn.SetExt( wxT( "svg" ) );
        fn.MakeAbsolute();

        bool success = DrawSVGPage( m_Parent, fn.GetFullPath(), ( SCH_SCREEN* ) screen,
                                    m_ModeColorOption->GetSelection() == 0 ? false : true,
                                    aPrint_Sheet_Ref );
        msg = _( "Create file " ) + fn.GetFullPath();

        if( !success )
            msg += _( " error" );

        msg += wxT( "\n" );
        m_MessagesBox->AppendText( msg );
    }
}


bool DIALOG_SVG_PRINT::DrawSVGPage( EDA_DRAW_FRAME* frame,
                                    const wxString& FullFileName,
                                    SCH_SCREEN*     screen,
                                    bool            aPrintBlackAndWhite,
                                    bool            aPrint_Sheet_Ref )
{
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxSize  sheetSize;          // Sheet size in internal units
    wxPoint old_org;
    bool    success = true;

    tmp_startvisu = screen->m_StartVisu;
    tmpzoom    = screen->GetZoom();
    old_org    = screen->m_DrawOrg;
    screen->m_DrawOrg.x    = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x  = screen->m_StartVisu.y = 0;

    sheetSize = screen->GetPageSettings().GetSizeIU();  // page size in 1/1000 inch, ie in internal units

    screen->SetScalingFactor( 1.0 );
    EDA_DRAW_PANEL* panel = frame->GetCanvas();

    LOCALE_IO   toggle;

    float       dpi = (float) frame->GetInternalUnits();
    wxSVGFileDC dc( FullFileName, sheetSize.x, sheetSize.y, dpi );

    EDA_RECT    tmp = *panel->GetClipBox();
    GRResetPenAndBrush( &dc );
    GRForceBlackPen( aPrintBlackAndWhite );


    panel->SetClipBox( EDA_RECT( wxPoint( -0x3FFFFF0, -0x3FFFFF0 ),
                                 wxSize( 0x7FFFFF0, 0x7FFFFF0 ) ) );

    screen->m_IsPrinting = true;
    screen->Draw( panel, &dc, GR_COPY );

    if( aPrint_Sheet_Ref )
        frame->TraceWorkSheet( &dc, screen, g_DrawDefaultLineThickness );

    screen->m_IsPrinting   = false;
    panel->SetClipBox( tmp );

    GRForceBlackPen( false );

    screen->m_StartVisu    = tmp_startvisu;
    screen->m_DrawOrg      = old_org;
    screen->SetZoom( tmpzoom );

    return success;
}


void DIALOG_SVG_PRINT::OnButtonPlotAllClick( wxCommandEvent& event )
{
    PrintSVGDoc( true, m_Print_Sheet_Ref->GetValue() );
}


void DIALOG_SVG_PRINT::OnButtonPlotCurrentClick( wxCommandEvent& event )
{
    PrintSVGDoc( false, m_Print_Sheet_Ref->GetValue() );
}


void DIALOG_SVG_PRINT::OnButtonCancelClick( wxCommandEvent& event )
{
    Close();
}


void DIALOG_SVG_PRINT::OnCloseWindow( wxCloseEvent& event )
{
    if( m_Config )
    {
        s_PlotBlackAndWhite = m_ModeColorOption->GetSelection();
        m_Config->Write( PLOTSVGMODECOLOR_KEY, s_PlotBlackAndWhite );
    }
    EndModal( 0 );
}


/* called on radiobox color/black and white selection
 */
void DIALOG_SVG_PRINT::OnSetColorModeSelected( wxCommandEvent& event )
{
    s_PlotBlackAndWhite = m_ModeColorOption->GetSelection();
}
