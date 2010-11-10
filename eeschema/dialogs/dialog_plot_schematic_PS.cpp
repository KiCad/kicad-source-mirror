/** @file dialog_plot_schematic_PS.cpp
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "worksheet.h"
#include "plot_common.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "class_drawsheetpath.h"


enum PageFormatReq {
    PAGE_SIZE_AUTO,
    PAGE_SIZE_A4,
    PAGE_SIZE_A
};

#include "dialog_plot_schematic_PS_base.h"

class DIALOG_PLOT_SCHEMATIC_PS : public DIALOG_PLOT_SCHEMATIC_PS_BASE
{
private:
    WinEDA_SchematicFrame* m_Parent;

public:

    /// Constructors
    DIALOG_PLOT_SCHEMATIC_PS( WinEDA_SchematicFrame* parent );

private:
    static bool m_plotColorOpt;
    static int  m_pageSizeSelect;
    static bool m_plot_Sheet_Ref;
    bool        m_select_PlotAll;
private:
    void OnPlotCurrent( wxCommandEvent& event );
    void OnPlotAll( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    void initDlg();
    void initOptVars();
    void createPSFile();
    void plotOneSheetPS( const wxString& FileName,
                         SCH_SCREEN* screen, Ki_PageDescr* sheet,
                         wxPoint plot_offset, double scale );
};
/* static members (static to remember last state): */
bool DIALOG_PLOT_SCHEMATIC_PS::m_plotColorOpt   = false;
int DIALOG_PLOT_SCHEMATIC_PS:: m_pageSizeSelect = PAGE_SIZE_AUTO;
bool DIALOG_PLOT_SCHEMATIC_PS::m_plot_Sheet_Ref = true;


void WinEDA_SchematicFrame::ToPlot_PS( wxCommandEvent& event )
{
    DIALOG_PLOT_SCHEMATIC_PS dlg( this );

    dlg.ShowModal();
}


DIALOG_PLOT_SCHEMATIC_PS::DIALOG_PLOT_SCHEMATIC_PS( WinEDA_SchematicFrame* parent ) :
    DIALOG_PLOT_SCHEMATIC_PS_BASE( parent )
{
    m_Parent = parent;
    m_select_PlotAll = false;
    initDlg();

    GetSizer()->SetSizeHints( this );
    Centre();
}


/*!
 * Control creation for DIALOG_PLOT_SCHEMATIC_PS
 */

void DIALOG_PLOT_SCHEMATIC_PS::initDlg()
{
    SetFocus(); // make the ESC work

    // Set options
    m_SizeOption->SetSelection( m_pageSizeSelect );
    m_PlotPSColorOption->SetSelection( m_plotColorOpt ? 1 : 0 );
    m_Plot_Sheet_Ref_Ctrl->SetValue( m_plot_Sheet_Ref );

    AddUnitSymbol( *m_defaultLineWidthTitle, g_UserUnit );
    PutValueInLocalUnits( *m_DefaultLineSizeCtrl,
                          g_DrawDefaultLineThickness, EESCHEMA_INTERNAL_UNIT );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void DIALOG_PLOT_SCHEMATIC_PS::OnPlotCurrent( wxCommandEvent& event )
{
    m_select_PlotAll = false;

    initOptVars();
    createPSFile();
    m_MsgBox->AppendText( wxT( "*****\n" ) );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
 */

void DIALOG_PLOT_SCHEMATIC_PS::OnPlotAll( wxCommandEvent& event )
{
    m_select_PlotAll = TRUE;

    initOptVars();
    createPSFile();
    m_MsgBox->AppendText( wxT( "*****\n" ) );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_PLOT_SCHEMATIC_PS::OnCancelClick( wxCommandEvent& event )
{
    initOptVars();
    EndModal( 0 );
}


void DIALOG_PLOT_SCHEMATIC_PS::initOptVars()
{
    m_plot_Sheet_Ref = m_Plot_Sheet_Ref_Ctrl->GetValue();
    m_plotColorOpt   = m_PlotPSColorOption->GetSelection();
    m_pageSizeSelect = m_SizeOption->GetSelection();
    g_DrawDefaultLineThickness = ReturnValueFromTextCtrl( *m_DefaultLineSizeCtrl,
                                                          EESCHEMA_INTERNAL_UNIT );
    if( g_DrawDefaultLineThickness < 1 )
        g_DrawDefaultLineThickness = 1;
}


void DIALOG_PLOT_SCHEMATIC_PS::createPSFile()
{
    SCH_SCREEN*     screen    = m_Parent->GetScreen();
    SCH_SCREEN*     oldscreen = screen;
    SCH_SHEET_PATH* sheetpath;
    SCH_SHEET_PATH* oldsheetpath = m_Parent->GetSheet();        // sheetpath is saved here
    wxString        plotFileName;
    Ki_PageDescr*   actualPage;                                 // page size selected in schematic
    Ki_PageDescr*   plotPage;                                   // page size selected to plot
    wxPoint         plot_offset;

    /* When printing all pages, the printed page is not the current page.
     * In complex hierarchies, we must update component references
     *  and others parameters in the given printed SCH_SCREEN, accordint to the sheet path
     *  because in complex hierarchies a SCH_SCREEN (a drawing )
     *  is shared between many sheets and component references depend on the actual sheet path used
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
                m_Parent->m_CurrentSheet = &list;
                m_Parent->m_CurrentSheet->UpdateAllScreenReferences();
                m_Parent->SetSheetNumberAndCount();
                screen = m_Parent->m_CurrentSheet->LastScreen();
                ActiveScreen = screen;
            }
            else  // Should not happen
                return;
            sheetpath = SheetList.GetNext();
        }
        actualPage = screen->m_CurrentSheetDesc;
        switch( m_pageSizeSelect )
        {
        case PAGE_SIZE_A:
            plotPage = &g_Sheet_A;
            break;

        case PAGE_SIZE_A4:
            plotPage = &g_Sheet_A4;
            break;

        case PAGE_SIZE_AUTO:
        default:
            plotPage = actualPage;
            break;
        }

        double scalex = (double) plotPage->m_Size.x / actualPage->m_Size.x;
        double scaley = (double) plotPage->m_Size.y / actualPage->m_Size.y;
        double scale  = 10 * MIN( scalex, scaley );

        plot_offset.x = 0;
        plot_offset.y = 0;

        plotFileName = m_Parent->GetUniqueFilenameForCurrentSheet() + wxT( ".ps" );

        plotOneSheetPS( plotFileName, screen, plotPage, plot_offset, scale );

        if( !m_select_PlotAll )
            break;
    }

    ActiveScreen = oldscreen;
    m_Parent->m_CurrentSheet = oldsheetpath;
    m_Parent->m_CurrentSheet->UpdateAllScreenReferences();
    m_Parent->SetSheetNumberAndCount();
}


void DIALOG_PLOT_SCHEMATIC_PS::plotOneSheetPS( const wxString& FileName,
                                               SCH_SCREEN*     screen,
                                               Ki_PageDescr*   sheet,
                                               wxPoint         plot_offset,
                                               double          scale )
{
    wxString msg;

    FILE*    output_file = wxFopen( FileName, wxT( "wt" ) );

    if( output_file == NULL )
    {
        msg  = wxT( "\n** " );
        msg += _( "Unable to create " ) + FileName + wxT( " **\n" );
        m_MsgBox->AppendText( msg );
        wxBell();
        return;
    }

    SetLocaleTo_C_standard();
    msg.Printf( _( "Plot: %s " ), GetChars( FileName ) );
    m_MsgBox->AppendText( msg );

    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->set_paper_size( sheet );
    plotter->set_viewport( plot_offset, scale, 0 );
    plotter->set_default_line_width( g_DrawDefaultLineThickness );
    plotter->set_color_mode( m_plotColorOpt );

    /* Init : */
    plotter->set_creator( wxT( "EESchema-PS" ) );
    plotter->set_filename( FileName );
    plotter->start_plot( output_file );

    if( m_plot_Sheet_Ref )
    {
        plotter->set_color( BLACK );
        m_Parent->PlotWorkSheet( plotter, screen );
    }

    PlotDrawlist( plotter, screen->EEDrawList );

    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();

    m_MsgBox->AppendText( wxT( "Ok\n" ) );
}
