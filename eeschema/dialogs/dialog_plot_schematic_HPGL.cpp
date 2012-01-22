/** @file dialog_plot_schematic_HPGL.cpp
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

#include "fctsys.h"
#include "gr_basic.h"
#include "confirm.h"
#include "plot_common.h"
#include "worksheet.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_sheet_path.h"

#include "dialog_plot_schematic_HPGL_base.h"


enum HPGL_PAGEZ_T
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
    PAGE_SIZE_E,
};


static const wxChar* plot_sheet_list( HPGL_PAGEZ_T aSize )
{
    const wxChar* ret;

    switch( aSize )
    {
    default:
    case PAGE_DEFAULT:  ret = NULL;         break;
    case PAGE_SIZE_A4:  ret = wxT( "A4" );  break;
    case PAGE_SIZE_A3:  ret = wxT( "A3" );  break;
    case PAGE_SIZE_A2:  ret = wxT( "A2" );  break;
    case PAGE_SIZE_A1:  ret = wxT( "A1" );  break;
    case PAGE_SIZE_A0:  ret = wxT( "A0" );  break;
    case PAGE_SIZE_A:   ret = wxT( "A" );   break;
    case PAGE_SIZE_B:   ret = wxT( "B" );   break;
    case PAGE_SIZE_C:   ret = wxT( "C" );   break;
    case PAGE_SIZE_D:   ret = wxT( "D" );   break;
    case PAGE_SIZE_E:   ret = wxT( "E" );   break;
    }

    return ret;
};


class DIALOG_PLOT_SCHEMATIC_HPGL : public DIALOG_PLOT_SCHEMATIC_HPGL_BASE
{
private:
    SCH_EDIT_FRAME* m_Parent;

public:
    DIALOG_PLOT_SCHEMATIC_HPGL( SCH_EDIT_FRAME* parent );

private:
    static HPGL_PAGEZ_T s_pageSizeSelect;
    static bool         s_plot_Sheet_Ref;
    static wxSize       s_Offset;

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
    void Plot_Schematic_HPGL( bool aPlotAll );

    void Plot_1_Page_HPGL( const wxString& FileName,
                           SCH_SCREEN* screen, const PAGE_INFO& aPageInfo,
                           wxPoint& offset, double plot_scale );
};


// static members (static to remember last state):
HPGL_PAGEZ_T DIALOG_PLOT_SCHEMATIC_HPGL:: s_pageSizeSelect = PAGE_DEFAULT;
bool DIALOG_PLOT_SCHEMATIC_HPGL::s_plot_Sheet_Ref = true;
wxSize DIALOG_PLOT_SCHEMATIC_HPGL::s_Offset;

void SCH_EDIT_FRAME::ToPlot_HPGL( wxCommandEvent& event )
{
    DIALOG_PLOT_SCHEMATIC_HPGL dlg( this );
    dlg.ShowModal();
}


DIALOG_PLOT_SCHEMATIC_HPGL::DIALOG_PLOT_SCHEMATIC_HPGL( SCH_EDIT_FRAME* parent ) :
    DIALOG_PLOT_SCHEMATIC_HPGL_BASE( parent )
{
    m_Parent = parent;
    initDlg();
    SetPageOffsetValue();

    GetSizer()->SetSizeHints( this );
    Centre();
    m_buttonPlotAll->SetDefault();
}


void DIALOG_PLOT_SCHEMATIC_HPGL::initDlg()
{
    SetFocus();     // Make ESC key work

    // Set validators
    m_SizeOption->SetSelection( s_pageSizeSelect );
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

    s_pageSizeSelect = (HPGL_PAGEZ_T) m_SizeOption->GetSelection();

    if( s_pageSizeSelect != PAGE_DEFAULT )
    {
        msg = ReturnStringFromValue( g_UserUnit,
                                     s_Offset.x,
                                     EESCHEMA_INTERNAL_UNIT );

        m_PlotOrgPosition_X->SetValue( msg );

        msg = ReturnStringFromValue( g_UserUnit,
                                     s_Offset.y,
                                     EESCHEMA_INTERNAL_UNIT );

        m_PlotOrgPosition_Y->SetValue( msg );

        m_PlotOrgPosition_X->Enable( true );
        m_PlotOrgPosition_Y->Enable( true );
    }
    else
    {
        m_PlotOrgPosition_X->Enable( false );
        m_PlotOrgPosition_Y->Enable( false );
    }
}


void DIALOG_PLOT_SCHEMATIC_HPGL::AcceptPlotOffset( wxCommandEvent& event )
{
    s_pageSizeSelect = (HPGL_PAGEZ_T) m_SizeOption->GetSelection();

    if( s_pageSizeSelect != PAGE_DEFAULT )
    {
        wxString msg = m_PlotOrgPosition_X->GetValue();

        s_Offset.x = ReturnValueFromString( g_UserUnit, msg, EESCHEMA_INTERNAL_UNIT );

        msg = m_PlotOrgPosition_Y->GetValue();

        s_Offset.y = ReturnValueFromString( g_UserUnit, msg, EESCHEMA_INTERNAL_UNIT );
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

    if( s_pageSizeSelect != PAGE_DEFAULT )
    {
        wxString msg = m_PlotOrgPosition_X->GetValue();

        s_Offset.x = ReturnValueFromString( g_UserUnit, msg, EESCHEMA_INTERNAL_UNIT );

        msg = m_PlotOrgPosition_Y->GetValue();

        s_Offset.y = ReturnValueFromString( g_UserUnit, msg, EESCHEMA_INTERNAL_UNIT );
    }

    Plot_Schematic_HPGL( aPlotAll );
}


void DIALOG_PLOT_SCHEMATIC_HPGL::Plot_Schematic_HPGL( bool aPlotAll )
{
    wxString        plotFileName;
    SCH_SCREEN*     screen = m_Parent->GetScreen();
    SCH_SHEET_PATH* sheetpath;
    SCH_SHEET_PATH  oldsheetpath = m_Parent->GetCurrentSheet();

    wxPoint         plotOffset;

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and other parameters
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
                m_Parent->SetCurrentSheet( list );
                m_Parent->GetCurrentSheet().UpdateAllScreenReferences();
                m_Parent->SetSheetNumberAndCount();

                screen = m_Parent->GetCurrentSheet().LastScreen();

                if( !screen )       // LastScreen() may return NULL
                    screen = m_Parent->GetScreen();
            }
            else  // Should not happen
                return;

            sheetpath = SheetList.GetNext();
        }

        const PAGE_INFO& curPage = screen->GetPageSettings();

        PAGE_INFO plotPage = curPage;

        // if plotting on a page size other than curPage
        if( s_pageSizeSelect != PAGE_DEFAULT )
            plotPage.SetType( plot_sheet_list( s_pageSizeSelect ) );

        // Calculation of conversion scales.

        // 10x because Eeschema works in mils, not deci-mils
        double plot_scale = 10 * (double) plotPage.GetWidthMils() / curPage.GetWidthMils();

        // Calculate offsets
        plotOffset.x = -s_Offset.x;
        plotOffset.y = -s_Offset.y;

        plotFileName = m_Parent->GetUniqueFilenameForCurrentSheet() + wxT( ".plt" );

        LOCALE_IO   toggle;

        Plot_1_Page_HPGL( plotFileName, screen, plotPage, plotOffset, plot_scale );

        if( !aPlotAll )
            break;
    }

    m_Parent->SetCurrentSheet( oldsheetpath );
    m_Parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_Parent->SetSheetNumberAndCount();
}


void DIALOG_PLOT_SCHEMATIC_HPGL::Plot_1_Page_HPGL( const wxString&  FileName,
                                                   SCH_SCREEN*      screen,
                                                   const PAGE_INFO& pageInfo,
                                                   wxPoint&         offset,
                                                   double           plot_scale )
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

    LOCALE_IO   toggle;

    msg.Printf( _( "Plot: %s " ), FileName.GetData() );
    m_MsgBox->AppendText( msg );

    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();

    plotter->SetPageSettings( pageInfo );

    plotter->set_viewport( offset, plot_scale, 0 );
    plotter->set_default_line_width( g_DrawDefaultLineThickness );

    // Init :
    plotter->set_creator( wxT( "Eeschema-HPGL" ) );
    plotter->set_filename( FileName );
    plotter->set_pen_speed( g_HPGL_Pen_Descr.m_Pen_Speed );
    plotter->set_pen_number( g_HPGL_Pen_Descr.m_Pen_Num );
    plotter->set_pen_diameter( g_HPGL_Pen_Descr.m_Pen_Diam );
    plotter->set_pen_overlap( g_HPGL_Pen_Descr.m_Pen_Diam / 2 );
    plotter->start_plot( output_file );

    plotter->set_color( BLACK );

    if( s_plot_Sheet_Ref )
        m_Parent->PlotWorkSheet( plotter, screen );

    screen->Plot( plotter );

    plotter->end_plot();
    delete plotter;

    m_MsgBox->AppendText( wxT( "Ok\n" ) );
}


/* Event handler for page size option
 */
void DIALOG_PLOT_SCHEMATIC_HPGL::OnPageSelected( wxCommandEvent& event )
{
    s_pageSizeSelect = (HPGL_PAGEZ_T) m_SizeOption->GetSelection();
    SetPageOffsetValue();
}


