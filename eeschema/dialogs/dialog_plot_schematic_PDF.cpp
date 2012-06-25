/** @file dialog_plot_schematic_PDF.cpp
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
#include <gr_basic.h>
#include <confirm.h>
#include <worksheet.h>
#include <plot_common.h>
#include <class_sch_screen.h>
#include <wxEeschemaStruct.h>
#include <base_units.h>

#include <general.h>
#include <protos.h>
#include <sch_sheet_path.h>


enum PageFormatReq {
    PAGE_SIZE_AUTO,
    PAGE_SIZE_A4,
    PAGE_SIZE_A
};

#include <dialog_plot_schematic_PDF_base.h>

class DIALOG_PLOT_SCHEMATIC_PDF : public DIALOG_PLOT_SCHEMATIC_PDF_BASE
{
private:
    SCH_EDIT_FRAME* m_Parent;

public:

    /// Constructors
    DIALOG_PLOT_SCHEMATIC_PDF( SCH_EDIT_FRAME* parent );

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
    void createPDFFile();
    void plotOneSheet( PDF_PLOTTER *plotter, SCH_SCREEN* screen );
    void plotSetupPage( PDF_PLOTTER *plotter, SCH_SCREEN* screen );
};

// static members (static to remember last state):
bool DIALOG_PLOT_SCHEMATIC_PDF::m_plotColorOpt   = false;
int  DIALOG_PLOT_SCHEMATIC_PDF::m_pageSizeSelect = PAGE_SIZE_AUTO;
bool DIALOG_PLOT_SCHEMATIC_PDF::m_plot_Sheet_Ref = true;


void SCH_EDIT_FRAME::ToPlot_PDF( wxCommandEvent& event )
{
    DIALOG_PLOT_SCHEMATIC_PDF dlg( this );

    dlg.ShowModal();
}


DIALOG_PLOT_SCHEMATIC_PDF::DIALOG_PLOT_SCHEMATIC_PDF( SCH_EDIT_FRAME* parent ) :
    DIALOG_PLOT_SCHEMATIC_PDF_BASE( parent )
{
    m_Parent = parent;
    m_select_PlotAll = false;
    initDlg();

    GetSizer()->SetSizeHints( this );
    Centre();
    m_buttonPlotAll->SetDefault();
}


/*!
 * Control creation for DIALOG_PLOT_SCHEMATIC_PDF
 */

void DIALOG_PLOT_SCHEMATIC_PDF::initDlg()
{
    SetFocus(); // make the ESC work

    // Set options
    m_SizeOption->SetSelection( m_pageSizeSelect );
    m_PlotPDFColorOption->SetSelection( m_plotColorOpt ? 1 : 0 );
    m_Plot_Sheet_Ref_Ctrl->SetValue( m_plot_Sheet_Ref );

    AddUnitSymbol( *m_defaultLineWidthTitle, g_UserUnit );
    PutValueInLocalUnits( *m_DefaultLineSizeCtrl, g_DrawDefaultLineThickness );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void DIALOG_PLOT_SCHEMATIC_PDF::OnPlotCurrent( wxCommandEvent& event )
{
    m_select_PlotAll = false;

    initOptVars();
    createPDFFile();
    m_MsgBox->AppendText( wxT( "*****\n" ) );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
 */

void DIALOG_PLOT_SCHEMATIC_PDF::OnPlotAll( wxCommandEvent& event )
{
    m_select_PlotAll = true;

    initOptVars();
    createPDFFile();
    m_MsgBox->AppendText( wxT( "*****\n" ) );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_PLOT_SCHEMATIC_PDF::OnCancelClick( wxCommandEvent& event )
{
    initOptVars();
    EndModal( 0 );
}


void DIALOG_PLOT_SCHEMATIC_PDF::initOptVars()
{
    m_plot_Sheet_Ref = m_Plot_Sheet_Ref_Ctrl->GetValue();
    m_plotColorOpt   = m_PlotPDFColorOption->GetSelection();
    m_pageSizeSelect = m_SizeOption->GetSelection();
    g_DrawDefaultLineThickness = ReturnValueFromTextCtrl( *m_DefaultLineSizeCtrl );

    if( g_DrawDefaultLineThickness < 1 )
        g_DrawDefaultLineThickness = 1;
}


void DIALOG_PLOT_SCHEMATIC_PDF::createPDFFile()
{
    SCH_SCREEN*         screen    = m_Parent->GetScreen();
    SCH_SHEET_PATH*     sheetpath;
    SCH_SHEET_PATH      oldsheetpath = m_Parent->GetCurrentSheet(); // sheetpath is saved here
    wxPoint             plot_offset;

    /* When printing all pages, the printed page is not the current page.  In
     * complex hierarchies, we must update component references and others
     * parameters in the given printed SCH_SCREEN, accordint to the sheet path
     * because in complex hierarchies a SCH_SCREEN (a drawing ) is shared
     * between many sheets and component references depend on the actual sheet
     * path used
     */
    SCH_SHEET_LIST SheetList( NULL );

    sheetpath = SheetList.GetFirst();

    // Allocate the plotter and set the job level parameter
    PDF_PLOTTER* plotter = new PDF_PLOTTER();
    plotter->SetDefaultLineWidth( g_DrawDefaultLineThickness );
    plotter->SetColorMode( m_plotColorOpt );
    plotter->SetCreator( wxT( "Eeschema-PDF" ) );
    plotter->SetPsTextMode( PSTEXTMODE_PHANTOM );


    // First page handling is different
    bool first_page = true;

    do
    {
	// Step over the schematic hierarchy
        if( m_select_PlotAll )
        {
	    SCH_SHEET_PATH list;
            if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
            {
                m_Parent->SetCurrentSheet( list );
                m_Parent->GetCurrentSheet().UpdateAllScreenReferences();
                m_Parent->SetSheetNumberAndCount();
                screen = m_Parent->GetCurrentSheet().LastScreen();
            }
            else  // Should not happen
                wxASSERT( 0 );

            sheetpath = SheetList.GetNext();
        }

	if( first_page ) {
	    wxString msg;
	    wxString plotFileName = m_Parent->GetUniqueFilenameForCurrentSheet()
		                    + wxT( ".pdf" );
	    msg.Printf( _( "Plot: %s " ), GetChars( plotFileName ) );
	    m_MsgBox->AppendText( msg );

	    FILE* output_file = wxFopen( plotFileName, wxT( "wb" ) );

	    if( output_file == NULL )
	    {
		msg = wxT( "\n** " );
		msg += _( "Unable to create " ) + plotFileName + wxT( " **\n" );
		m_MsgBox->AppendText( msg );
		wxBell();
		return;
	    }
	    // Open the plotter and do the first page
	    SetLocaleTo_C_standard();
	    plotter->SetFilename( plotFileName );
	    plotSetupPage( plotter, screen );
	    plotter->StartPlot( output_file );
	    first_page = false;
	}
	else
	{
	    /* For the following pages you need to close the (finished) page,
	       reconfigure, and then start a new one */
	    plotter->ClosePage();
	    plotSetupPage( plotter, screen );
	    plotter->StartPage();
	}
	plotOneSheet( plotter, screen );
    } while  (m_select_PlotAll && sheetpath );

    // Everything done, close the plot and restore the environment
    plotter->EndPlot();
    delete plotter;
    SetLocaleTo_Default();

    // Restore the previous sheet
    m_Parent->SetCurrentSheet( oldsheetpath );
    m_Parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_Parent->SetSheetNumberAndCount();
}

void DIALOG_PLOT_SCHEMATIC_PDF::plotSetupPage( PDF_PLOTTER*     plotter,
                                               SCH_SCREEN*      screen)
{
    PAGE_INFO plotPage; // page size selected to plot
    // Considerations on page size and scaling requests
    PAGE_INFO actualPage = screen->GetPageSettings(); // page size selected in schematic
    switch( m_pageSizeSelect )
    {
    case PAGE_SIZE_A:
	plotPage.SetType( wxT( "A" ) );
	plotPage.SetPortrait( actualPage.IsPortrait() );
	break;

    case PAGE_SIZE_A4:
	plotPage.SetType( wxT( "A4" ) );
	plotPage.SetPortrait( actualPage.IsPortrait() );
	break;

    case PAGE_SIZE_AUTO:
    default:
	plotPage = actualPage;
	break;
    }

    double scalex = (double) plotPage.GetWidthMils()  / actualPage.GetWidthMils();
    double scaley = (double) plotPage.GetHeightMils() / actualPage.GetHeightMils();
    double scale  = MIN( scalex, scaley );
    plotter->SetViewport( wxPoint( 0, 0 ), IU_PER_DECIMILS, scale, 0 );
    plotter->SetPageSettings( plotPage );
}

void DIALOG_PLOT_SCHEMATIC_PDF::plotOneSheet( PDF_PLOTTER*   plotter,
                                              SCH_SCREEN*    screen )
{
    if( m_plot_Sheet_Ref )
    {
	plotter->SetColor( BLACK );
	m_Parent->PlotWorkSheet( plotter, screen, g_DrawDefaultLineThickness );
    }

    screen->Plot( plotter );
}

