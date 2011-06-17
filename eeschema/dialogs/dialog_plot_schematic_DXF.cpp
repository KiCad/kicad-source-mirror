/** @file dialog_plot_schematic_DXF.cpp
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
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
#include "macros.h"
#include "plot_common.h"
#include "confirm.h"
#include "worksheet.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_sheet_path.h"

#include "dialog_plot_schematic_DXF_base.h"

class DIALOG_PLOT_SCHEMATIC_DXF : public DIALOG_PLOT_SCHEMATIC_DXF_BASE
{
private:
    SCH_EDIT_FRAME* m_Parent;

public:

    /// Constructors
    DIALOG_PLOT_SCHEMATIC_DXF( SCH_EDIT_FRAME* parent );

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
    void CreateDXFFile();
    void PlotOneSheetDXF( const wxString& FileName,
                         SCH_SCREEN* screen, Ki_PageDescr* sheet,
                         wxPoint plot_offset, double scale );
};
/* static members (static to remember last state): */
bool DIALOG_PLOT_SCHEMATIC_DXF::m_plotColorOpt   = false;
bool DIALOG_PLOT_SCHEMATIC_DXF::m_plot_Sheet_Ref = true;



void SCH_EDIT_FRAME::ToPlot_DXF( wxCommandEvent& event )
{
    DIALOG_PLOT_SCHEMATIC_DXF DXF_frame( this );
    DXF_frame.ShowModal();
}


DIALOG_PLOT_SCHEMATIC_DXF::DIALOG_PLOT_SCHEMATIC_DXF( SCH_EDIT_FRAME* parent )
    : DIALOG_PLOT_SCHEMATIC_DXF_BASE( parent )
{
    m_Parent = parent;
    m_select_PlotAll = false;
    initDlg();

    GetSizer()->SetSizeHints( this );
    Centre();
    m_buttonPlotAll->SetDefault();
}

void DIALOG_PLOT_SCHEMATIC_DXF::initDlg()
{
    SetFocus(); // make the ESC work
    // Set options
    m_PlotColorCtrl->SetSelection( m_plotColorOpt ? 1 : 0 );
    m_Plot_Sheet_Ref_Ctrl->SetValue( m_plot_Sheet_Ref );
}


/* event handler for Plot Current button
 */
void DIALOG_PLOT_SCHEMATIC_DXF::OnPlotCurrent( wxCommandEvent& event )
{
    m_select_PlotAll = false;

    initOptVars();
    CreateDXFFile( );
    m_MsgBox->AppendText( wxT( "*****\n" ) );
}

/* event handler for Plot ALL button
 */
void DIALOG_PLOT_SCHEMATIC_DXF::OnPlotAll( wxCommandEvent& event )
{
    m_select_PlotAll = true;

    initOptVars();
    CreateDXFFile( );
    m_MsgBox->AppendText( wxT( "*****\n" ) );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */
void DIALOG_PLOT_SCHEMATIC_DXF::OnCancelClick( wxCommandEvent& event )
{
    initOptVars();
    EndModal( 0 );
}


void DIALOG_PLOT_SCHEMATIC_DXF::initOptVars()
{
    m_plot_Sheet_Ref  = m_Plot_Sheet_Ref_Ctrl->GetValue();
    m_plotColorOpt = m_PlotColorCtrl->GetSelection() == 1 ? true : false;
}


void DIALOG_PLOT_SCHEMATIC_DXF::CreateDXFFile( )
{
    SCH_EDIT_FRAME* schframe  = (SCH_EDIT_FRAME*) m_Parent;
    SCH_SCREEN*     screen    = schframe->GetScreen();
    SCH_SHEET_PATH* sheetpath, * oldsheetpath = schframe->GetSheet();
    wxString        PlotFileName;
    Ki_PageDescr*   PlotSheet;
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
                schframe->m_CurrentSheet = &list;
                schframe->m_CurrentSheet->UpdateAllScreenReferences();
                schframe->SetSheetNumberAndCount();
                screen = schframe->m_CurrentSheet->LastScreen();
            }
            else  // Should not happen
                return;

            sheetpath = SheetList.GetNext();
        }

        PlotSheet = screen->m_CurrentSheetDesc;
        double scale = 10;

        plot_offset.x = 0;
        plot_offset.y = 0;

        PlotFileName = schframe->GetUniqueFilenameForCurrentSheet() + wxT( ".dxf" );

        PlotOneSheetDXF( PlotFileName, screen, PlotSheet, plot_offset, scale );

        if( !m_select_PlotAll )
            break;
    }

    schframe->m_CurrentSheet = oldsheetpath;
    schframe->m_CurrentSheet->UpdateAllScreenReferences();
    schframe->SetSheetNumberAndCount();
}


void DIALOG_PLOT_SCHEMATIC_DXF::PlotOneSheetDXF( const wxString& FileName,
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
        return;
    }

    msg.Printf( _( "Plot: %s " ), GetChars( FileName ) );
    m_MsgBox->AppendText( msg );

    SetLocaleTo_C_standard();
    DXF_PLOTTER* plotter = new DXF_PLOTTER();
    plotter->set_paper_size( sheet );
    plotter->set_viewport( plot_offset, scale, 0 );
    plotter->set_color_mode( m_plotColorOpt );

    /* Init : */
    plotter->set_creator( wxT( "EESchema-DXF" ) );
    plotter->set_filename( FileName );
    plotter->start_plot( output_file );

    if( m_plot_Sheet_Ref )
    {
        plotter->set_color( BLACK );
        m_Parent->PlotWorkSheet( plotter, screen );
    }

    screen->Plot( plotter );

    /* fin */
    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();

    m_MsgBox->AppendText( wxT( "Ok\n" ) );
}
