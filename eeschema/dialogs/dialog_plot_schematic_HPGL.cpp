/** @file dialog_plot_schematic_HPGL.cpp
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
#include "plot_common.h"
#include "worksheet.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_sheet_path.h"

#include "dialog_plot_schematic_HPGL_base.h"

enum PageFormatReq
{
    PAGE_DEFAULT = 0,
    PAGE_SIZE_A4,
    PAGE_SIZE_A3,
    PAGE_SIZE_A2,
    PAGE_SIZE_A1,
    PAGE_SIZE_A0,
    PAGE_SIZE_A,
    PAGE_SIZE_B,
    PAGE_SIZE_C,
    PAGE_SIZE_D,
    PAGE_SIZE_E
};

static Ki_PageDescr* Plot_sheet_list[] =
{
    NULL,
    &g_Sheet_A4,
    &g_Sheet_A3,
    &g_Sheet_A2,
    &g_Sheet_A1,
    &g_Sheet_A0,
    &g_Sheet_A,
    &g_Sheet_B,
    &g_Sheet_C,
    &g_Sheet_D,
    &g_Sheet_E,
    &g_Sheet_GERBER,
    &g_Sheet_user
};

class DIALOG_PLOT_SCHEMATIC_HPGL : public DIALOG_PLOT_SCHEMATIC_HPGL_BASE
{
private:
    SCH_EDIT_FRAME* m_Parent;

public:
   DIALOG_PLOT_SCHEMATIC_HPGL( SCH_EDIT_FRAME* parent );

private:
    static PageFormatReq  m_pageSizeSelect;
    static bool m_plot_Sheet_Ref;
    bool        m_select_PlotAll;

private:
    void OnPageSelected( wxCommandEvent& event );
    void OnPlotCurrent( wxCommandEvent& event );
    void OnPlotAll( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void AcceptPlotOffset( wxCommandEvent& event );

    void initDlg();
    void SetPenSpeed();
    void SetPenNum();
    void SetPenWidth();
    void SetPageOffsetValue();
    void HPGL_Plot( bool aPlotAll );
    void Plot_Schematic_HPGL( bool aPlotAll, int HPGL_SheetSize );
    void Plot_1_Page_HPGL( const wxString& FileName,
                           SCH_SCREEN* screen, Ki_PageDescr* sheet,
                           wxPoint& offset, double plot_scale );
    void ReturnSheetDims( SCH_SCREEN* screen, wxSize& SheetSize, wxPoint& SheetOffset );
};
/* static members (static to remember last state): */
PageFormatReq DIALOG_PLOT_SCHEMATIC_HPGL:: m_pageSizeSelect = PAGE_DEFAULT;
bool DIALOG_PLOT_SCHEMATIC_HPGL::m_plot_Sheet_Ref = true;


void SCH_EDIT_FRAME::ToPlot_HPGL( wxCommandEvent& event )
{
    DIALOG_PLOT_SCHEMATIC_HPGL dlg( this );
    dlg.ShowModal();
}


DIALOG_PLOT_SCHEMATIC_HPGL::DIALOG_PLOT_SCHEMATIC_HPGL( SCH_EDIT_FRAME* parent )
    :DIALOG_PLOT_SCHEMATIC_HPGL_BASE(parent)
{
    m_Parent = parent;
    initDlg();
    SetPageOffsetValue();

    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_PLOT_SCHEMATIC_HPGL::initDlg()
{

    SetFocus(); // Make ESC key working

    // Set validators
    m_SizeOption->SetSelection( m_pageSizeSelect );
    AddUnitSymbol( *m_penWidthTitle, g_UserUnit );
    PutValueInLocalUnits( *m_penWidthCtrl, g_HPGL_Pen_Descr. m_Pen_Diam, EESCHEMA_INTERNAL_UNIT );
    m_penSpeedCtrl->SetValue( g_HPGL_Pen_Descr. m_Pen_Speed );
    m_penNumCtrl->SetValue( g_HPGL_Pen_Descr. m_Pen_Num );

}


void DIALOG_PLOT_SCHEMATIC_HPGL::OnPlotCurrent( wxCommandEvent& event )
{
    HPGL_Plot( false );
}

void DIALOG_PLOT_SCHEMATIC_HPGL::OnPlotAll( wxCommandEvent& event )
{
    HPGL_Plot( true );
}

void DIALOG_PLOT_SCHEMATIC_HPGL::OnCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}

void DIALOG_PLOT_SCHEMATIC_HPGL::SetPageOffsetValue()
{
    wxString msg;

    m_pageSizeSelect = (PageFormatReq) m_SizeOption->GetSelection();
    if( m_pageSizeSelect != PAGE_DEFAULT )
    {
        msg = ReturnStringFromValue( g_UserUnit,
                                     Plot_sheet_list[m_pageSizeSelect]->m_Offset.x,
                                     EESCHEMA_INTERNAL_UNIT );
        m_PlotOrgPosition_X->SetValue( msg );
        msg = ReturnStringFromValue( g_UserUnit,
                                     Plot_sheet_list[m_pageSizeSelect]-> m_Offset.y,
                                     EESCHEMA_INTERNAL_UNIT );
        m_PlotOrgPosition_Y->SetValue( msg );

        m_PlotOrgPosition_X->Enable( TRUE );
        m_PlotOrgPosition_Y->Enable( TRUE );
    }
    else
    {
        m_PlotOrgPosition_X->Enable( FALSE );
        m_PlotOrgPosition_Y->Enable( FALSE );
    }
}


void DIALOG_PLOT_SCHEMATIC_HPGL::AcceptPlotOffset( wxCommandEvent& event )
{
    m_pageSizeSelect = (PageFormatReq) m_SizeOption->GetSelection();

    if( m_pageSizeSelect != PAGE_DEFAULT )
    {
        wxString msg = m_PlotOrgPosition_X->GetValue();
        Plot_sheet_list[m_pageSizeSelect]->m_Offset.x =
            ReturnValueFromString( g_UserUnit, msg, EESCHEMA_INTERNAL_UNIT );
        msg = m_PlotOrgPosition_Y->GetValue();
        Plot_sheet_list[m_pageSizeSelect]->m_Offset.y =
            ReturnValueFromString( g_UserUnit, msg, EESCHEMA_INTERNAL_UNIT );
    }
}


void DIALOG_PLOT_SCHEMATIC_HPGL::SetPenWidth( )
{
    g_HPGL_Pen_Descr.m_Pen_Diam = ReturnValueFromTextCtrl( *m_penWidthCtrl,
                                                           EESCHEMA_INTERNAL_UNIT);
    if( g_HPGL_Pen_Descr.m_Pen_Diam > 100 )
        g_HPGL_Pen_Descr.m_Pen_Diam = 100;

    if( g_HPGL_Pen_Descr.m_Pen_Diam < 1 )
        g_HPGL_Pen_Descr.m_Pen_Diam = 1;
}


void DIALOG_PLOT_SCHEMATIC_HPGL::SetPenSpeed(  )
{
    g_HPGL_Pen_Descr.m_Pen_Speed = m_penSpeedCtrl->GetValue();

    if( g_HPGL_Pen_Descr.m_Pen_Speed > 40 )
        g_HPGL_Pen_Descr.m_Pen_Speed = 40;

    if( g_HPGL_Pen_Descr.m_Pen_Speed < 1 )
        g_HPGL_Pen_Descr.m_Pen_Speed = 1;
}


void DIALOG_PLOT_SCHEMATIC_HPGL::SetPenNum(  )
{
    g_HPGL_Pen_Descr.m_Pen_Num = m_penNumCtrl->GetValue();

    if( g_HPGL_Pen_Descr.m_Pen_Num > 8 )
        g_HPGL_Pen_Descr.m_Pen_Num = 8;

    if( g_HPGL_Pen_Descr.m_Pen_Num < 1 )
        g_HPGL_Pen_Descr.m_Pen_Num = 1;
}


void DIALOG_PLOT_SCHEMATIC_HPGL::HPGL_Plot( bool aPlotAll )
{
    SetPenWidth( );
    SetPenNum( );
    SetPenSpeed( );

    if( m_pageSizeSelect != PAGE_DEFAULT )
    {
        Ki_PageDescr* plot_sheet = Plot_sheet_list[m_pageSizeSelect];
        wxString msg = m_PlotOrgPosition_X->GetValue();
        plot_sheet->m_Offset.x =
            ReturnValueFromString( g_UserUnit, msg, EESCHEMA_INTERNAL_UNIT );
        msg = m_PlotOrgPosition_Y->GetValue();
        plot_sheet->m_Offset.y =
            ReturnValueFromString( g_UserUnit, msg, EESCHEMA_INTERNAL_UNIT );
    }

    Plot_Schematic_HPGL( aPlotAll, m_pageSizeSelect );
}


/* Function calculates the offsets and dimensions of any trace of the
 * selected sheet
 */
void DIALOG_PLOT_SCHEMATIC_HPGL::ReturnSheetDims( SCH_SCREEN* screen,
                                                  wxSize&     SheetSize,
                                                  wxPoint&    SheetOffset )
{
    Ki_PageDescr* PlotSheet;

    if( screen == NULL )
        screen = m_Parent->GetScreen();

    PlotSheet = screen->m_CurrentSheetDesc;

    SheetSize   = PlotSheet->m_Size;
    SheetOffset = PlotSheet->m_Offset;
}


void DIALOG_PLOT_SCHEMATIC_HPGL::Plot_Schematic_HPGL( bool aPlotAll, int HPGL_SheetSize )
{
    wxString               PlotFileName;
    SCH_SCREEN*            screen    = m_Parent->GetScreen();
    SCH_SHEET_PATH*        sheetpath, * oldsheetpath = m_Parent->GetSheet();
    Ki_PageDescr*          PlotSheet;
    wxSize                 SheetSize;
    wxPoint                SheetOffset, PlotOffset;

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and others parameters
     *  in the printed SCH_SCREEN
     *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
     *  is shared between many sheets
     */
    SCH_SHEET_LIST SheetList( NULL );

    sheetpath = SheetList.GetFirst();
    SCH_SHEET_PATH list;

    while( true )
    {
        if( aPlotAll )
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
            }
            else  // Should not happen
                return;

            sheetpath = SheetList.GetNext();
        }

        ReturnSheetDims( screen, SheetSize, SheetOffset );

        /* Calculation of conversion scales. */
        if( HPGL_SheetSize )
            PlotSheet = Plot_sheet_list[HPGL_SheetSize];
        else
            PlotSheet = screen->m_CurrentSheetDesc;

        /* 10x because eeschema works in mils, not decimals */
        double plot_scale = 10 * (double) PlotSheet->m_Size.x / (double) SheetSize.x;

        /* Calculate offsets */
        PlotOffset.x = -SheetOffset.x;
        PlotOffset.y = -SheetOffset.y;

        PlotFileName = m_Parent->GetUniqueFilenameForCurrentSheet() + wxT( ".plt" );

        SetLocaleTo_C_standard();
        Plot_1_Page_HPGL( PlotFileName, screen, PlotSheet, PlotOffset, plot_scale );
        SetLocaleTo_Default();

        if( !aPlotAll )
            break;
    }

    m_Parent->m_CurrentSheet = oldsheetpath;
    m_Parent->m_CurrentSheet->UpdateAllScreenReferences();
    m_Parent->SetSheetNumberAndCount();
}


void DIALOG_PLOT_SCHEMATIC_HPGL::Plot_1_Page_HPGL( const wxString& FileName,
                                                   SCH_SCREEN*     screen,
                                                   Ki_PageDescr*   sheet,
                                                   wxPoint&        offset,
                                                   double          plot_scale )
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
    msg.Printf( _( "Plot: %s " ), FileName.GetData() );
    m_MsgBox->AppendText( msg );

    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();
    plotter->set_paper_size( sheet );
    plotter->set_viewport( offset, plot_scale, 0 );
    plotter->set_default_line_width( g_DrawDefaultLineThickness );
    /* Init : */
    plotter->set_creator( wxT( "EESchema-HPGL" ) );
    plotter->set_filename( FileName );
    plotter->set_pen_speed( g_HPGL_Pen_Descr.m_Pen_Speed );
    plotter->set_pen_number( g_HPGL_Pen_Descr.m_Pen_Num );
    plotter->set_pen_diameter( g_HPGL_Pen_Descr.m_Pen_Diam );
    plotter->set_pen_overlap( g_HPGL_Pen_Descr.m_Pen_Diam / 2 );
    plotter->start_plot( output_file );

    plotter->set_color( BLACK );

    if( m_plot_Sheet_Ref )
        m_Parent->PlotWorkSheet( plotter, screen );

    PlotDrawlist( plotter, screen->GetDrawItems() );

    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();

    m_MsgBox->AppendText( wxT( "Ok\n" ) );
}


/* Event handler for page size option
 */
void DIALOG_PLOT_SCHEMATIC_HPGL::OnPageSelected( wxCommandEvent& event )
{
    m_pageSizeSelect = (PageFormatReq) m_SizeOption->GetSelection();
    SetPageOffsetValue();
}


