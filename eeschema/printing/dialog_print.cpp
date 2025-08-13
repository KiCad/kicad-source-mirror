/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <pgm_base.h>
#include <confirm.h>
#include <sch_screen.h>
#include <sch_edit_frame.h>
#include <math/vector2wx.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/filename.h>
#include "dialog_print.h"


#include <dialogs/panel_printer_list.h>

#include <advanced_config.h>
#include <printing.h>
#include <sch_plotter.h>

#include "sch_printout.h"


/**
 * Custom schematic print preview frame.
 * This derived preview frame remembers its size and position during a session
 */
class SCH_PREVIEW_FRAME : public wxPreviewFrame
{
public:
    SCH_PREVIEW_FRAME( wxPrintPreview* aPreview, wxWindow* aParent,
                       const wxString& aTitle, const wxPoint& aPos = wxDefaultPosition,
                       const wxSize& aSize = wxDefaultSize ) :
            wxPreviewFrame( aPreview, aParent, aTitle, aPos, aSize )
    {
    }

    bool Show( bool show ) override
    {
        bool        ret;

        // Show or hide the window.  If hiding, save current position and size.
        // If showing, use previous position and size.
        if( show )
        {
            ret = wxPreviewFrame::Show( show );

            if( s_size.x != 0 && s_size.y != 0 )
                SetSize( s_pos.x, s_pos.y, s_size.x, s_size.y, 0 );
        }
        else
        {
            // Save the dialog's position & size before hiding
            s_size = GetSize();
            s_pos  = GetPosition();

            ret = wxPreviewFrame::Show( show );
        }

        return ret;
    }

private:
    static wxPoint  s_pos;
    static wxSize   s_size;
};


wxPoint SCH_PREVIEW_FRAME::s_pos;
wxSize  SCH_PREVIEW_FRAME::s_size;


DIALOG_PRINT::DIALOG_PRINT( SCH_EDIT_FRAME* aParent ) :
        DIALOG_PRINT_BASE( aParent ),
        m_parent( aParent )
{
    wxASSERT( aParent );

    // Show m_panelPrinters only if there are printers to list:
    m_panelPrinters->Show( m_panelPrinters->AsPrintersAvailable() );

    SetupStandardButtons( { { wxID_OK,     _( "Print" )         },
                            { wxID_APPLY,  _( "Print Preview" ) },
                            { wxID_CANCEL, _( "Close" )         } } );

#ifdef __WXMAC__
    // Problems with modal on wx-2.9 - Anyway preview is standard for OSX
    m_sdbSizerApply->Hide();
#endif
#if defined(__WXGTK__)
    // Preview using Cairo does not work on GTK,
    // but this platform provide native print preview
    m_sdbSizerApply->Hide();
#endif

    // New printing subsystem has print preview on all platforms
#if defined( _MSC_VER )
    if( ADVANCED_CFG::GetCfg().m_UsePdfPrint )
    {
        m_sdbSizerApply->Hide();
    }
#endif

    m_sdbSizerOK->SetFocus();

    Layout();

    finishDialogSettings();
}


DIALOG_PRINT::~DIALOG_PRINT()
{
    SavePrintOptions();
}


bool DIALOG_PRINT::TransferDataToWindow()
{
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    if( cfg->m_Printing.monochrome )
    {
        m_checkBackgroundColor->SetValue( false );
        m_checkBackgroundColor->Enable( false );
    }

    m_checkReference->SetValue( cfg->m_Printing.title_block );
    m_colorPrint->SetSelection( cfg->m_Printing.monochrome ? 1 : 0 );
    m_checkBackgroundColor->SetValue( cfg->m_Printing.background );
    m_checkUseColorTheme->SetValue( cfg->m_Printing.use_theme );

    m_colorTheme->Clear();

    int width    = 0;
    int height   = 0;
    int minwidth = width;

    wxString target = cfg->m_Printing.use_theme ? cfg->m_Printing.color_theme : cfg->m_ColorTheme;

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        int pos = m_colorTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings->GetFilename() == target )
            m_colorTheme->SetSelection( pos );

        m_colorTheme->GetTextExtent( settings->GetName(), &width, &height );
        minwidth = std::max( minwidth, width );
    }

    m_colorTheme->SetMinSize( wxSize( minwidth + 50, -1 ) );

    m_colorTheme->Enable( cfg->m_Printing.use_theme );

    // Initialize page specific print setup dialog settings.
    const PAGE_INFO& pageInfo = m_parent->GetScreen()->GetPageSettings();
    wxPageSetupDialogData& pageSetupDialogData = m_parent->GetPageSetupData();

    pageSetupDialogData.SetPaperId( pageInfo.GetPaperId() );

    if( pageInfo.IsCustom() )
    {
        if( pageInfo.IsPortrait() )
        {
            pageSetupDialogData.SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ),
                                                      EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ) ) );
        }
        else
        {
            pageSetupDialogData.SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ),
                                                      EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ) ) );
        }
    }

    pageSetupDialogData.GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

    Layout();

    return true;
}


void DIALOG_PRINT::OnUseColorThemeChecked( wxCommandEvent& event )
{
    m_colorTheme->Enable( m_checkUseColorTheme->GetValue() );
}


void DIALOG_PRINT::OnOutputChoice( wxCommandEvent& event )
{
    long sel = event.GetSelection();
    m_checkBackgroundColor->Enable( sel == 0 );

    if( sel )
        m_checkBackgroundColor->SetValue( false );
    else
        m_checkBackgroundColor->SetValue( m_parent->eeconfig()->m_Printing.background );
}


void DIALOG_PRINT::SavePrintOptions()
{
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    cfg->m_Printing.monochrome  = !!m_colorPrint->GetSelection();
    cfg->m_Printing.title_block = m_checkReference->IsChecked();

    if( m_checkBackgroundColor->IsEnabled() )
        cfg->m_Printing.background = m_checkBackgroundColor->IsChecked();
    else
        cfg->m_Printing.background = false;

    cfg->m_Printing.use_theme   = m_checkUseColorTheme->IsChecked();

    COLOR_SETTINGS* theme = static_cast<COLOR_SETTINGS*>(
            m_colorTheme->GetClientData( m_colorTheme->GetSelection() ) );

    if( theme && m_checkUseColorTheme->IsChecked() )
        cfg->m_Printing.color_theme = theme->GetFilename();
}


void DIALOG_PRINT::OnPageSetup( wxCommandEvent& event )
{
    wxPageSetupDialog pageSetupDialog( this, &m_parent->GetPageSetupData() );
    pageSetupDialog.ShowModal();

    m_parent->GetPageSetupData() = pageSetupDialog.GetPageSetupDialogData();
}


void DIALOG_PRINT::OnPrintPreview( wxCommandEvent& event )
{
    SavePrintOptions();
    wxPrintData& prn_data = m_parent->GetPageSetupData().GetPrintData();

    wxString selectedPrinterName;

    if( m_panelPrinters )
        selectedPrinterName = m_panelPrinters->GetSelectedPrinterName();

    prn_data.SetPrinterName( selectedPrinterName );

    // Pass two printout objects: for preview, and possible printing.
    wxString        title   = _( "Preview" );
    wxPrintPreview* preview = new wxPrintPreview( new SCH_PRINTOUT( m_parent, title ),
                                                  new SCH_PRINTOUT( m_parent, title ), &prn_data );

    preview->SetZoom( 100 );

    SCH_PREVIEW_FRAME* frame = new SCH_PREVIEW_FRAME( preview, this, title );

    // On wxGTK, set the flag wxTOPLEVEL_EX_DIALOG is mandatory, if we want
    // close the frame using the X box in caption, when the preview frame is run
    // from a dialog
    frame->SetExtraStyle( frame->GetExtraStyle() | wxTOPLEVEL_EX_DIALOG );

    // We use here wxPreviewFrame_WindowModal option to make the wxPrintPreview frame
    // modal for its caller only.
    // another reason is the fact when closing the frame without this option,
    // all top level frames are reenabled.
    // With this option, only the parent is reenabled.
    // Reenabling all top level frames should be made by the parent dialog.
    frame->InitializeWithModality( wxPreviewFrame_WindowModal );

    // on first invocation in this runtime session, set to 3/4 size of parent,
    // but will be changed in Show() if not first time as will position.
    // Must be called after InitializeWithModality because otherwise in some wxWidget
    // versions it is not always taken in account
    frame->SetMinSize( wxSize( 650, 500 ) );
    frame->SetSize( (m_parent->GetSize() * 3) / 4 );

    frame->Raise(); // Needed on Ubuntu/Unity to display the frame
    frame->Show( true );
}


bool DIALOG_PRINT::TransferDataFromWindow()
{
    if( Pgm().m_Printing )
    {
        DisplayError( this, _( "Previous print job not yet complete." ) );
        return false;
    }

    SavePrintOptions();

#if defined( _MSC_VER )
    if( ADVANCED_CFG::GetCfg().m_UsePdfPrint )
    {
        EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

        SCH_RENDER_SETTINGS renderSettings( *m_parent->GetRenderSettings() );
        renderSettings.m_ShowHiddenPins = false;
        renderSettings.m_ShowHiddenFields = false;

        COLOR_SETTINGS* cs = ::GetColorSettings( cfg->m_Printing.use_theme
                                                ? cfg->m_Printing.color_theme
                                                : cfg->m_ColorTheme );
        renderSettings.LoadColors( cs );

        SCH_PLOT_OPTS plotOpts;
        plotOpts.m_plotDrawingSheet = cfg->m_Printing.title_block;
        plotOpts.m_blackAndWhite = cfg->m_Printing.monochrome;
        plotOpts.m_useBackgroundColor = cfg->m_Printing.background;
        plotOpts.m_theme = cfg->m_Printing.use_theme ? cfg->m_Printing.color_theme
                                                     : cfg->m_ColorTheme;

        wxFileName tmp = wxFileName::CreateTempFileName( wxS( "eeschema_print" ) );
        wxRemoveFile( tmp.GetFullPath() );
        tmp.SetExt( wxS( "pdf" ) );
        plotOpts.m_outputFile = tmp.GetFullPath();

        SCH_PLOTTER plotter( m_parent );

        Pgm().m_Printing = true;
        plotter.Plot( PLOT_FORMAT::PDF, plotOpts, &renderSettings, nullptr );
        Pgm().m_Printing = false;

        KIPLATFORM::PRINTING::PRINT_RESULT result =
                KIPLATFORM::PRINTING::PrintPDF( TO_UTF8( plotter.GetLastOutputFilePath() ) );

        if( result != KIPLATFORM::PRINTING::PRINT_RESULT::OK &&
            result != KIPLATFORM::PRINTING::PRINT_RESULT::CANCELLED )
        {
            DisplayError( this, KIPLATFORM::PRINTING::PrintResultToString( result ) );
        }

        return true;
    }
#endif

    int sheet_count = m_parent->Schematic().Root().CountSheets();

    wxPrintData& data = m_parent->GetPageSetupData().GetPrintData();

#if defined( __WXGTK__ ) && !wxCHECK_VERSION( 3, 2, 3 )
    // In GTK, the default bottom margin is bigger by 0.31 inches for
    // Letter, Legal, A4 paper sizes (see gtk_paper_size_get_default_bottom_margin).
    //
    // wxWidgets doesn't handle this properly when paper is in
    // landscape orientation.
    //
    // Using custom page size avoids the problematic
    // gtk_page_setup_set_paper_size_and_default_margins call in wxWidgets.

    wxPaperSize   paperId = data.GetPaperId();
    const wxChar* paperType = nullptr;

    // clang-format off
    std::set<wxPaperSize> letterSizes = {
        // na_letter
        wxPAPER_LETTER,
        wxPAPER_LETTERSMALL,
        wxPAPER_NOTE,
        wxPAPER_LETTER_TRANSVERSE,
        wxPAPER_LETTER_ROTATED
    };

    std::set<wxPaperSize> legalSizes = {
        // na_legal
        wxPAPER_LEGAL
    };

    std::set<wxPaperSize> a4Sizes = {
        // iso_a4
        wxPAPER_A4,
        wxPAPER_A4SMALL,
        wxPAPER_A4_TRANSVERSE,
        wxPAPER_A4_ROTATED
    };
    // clang-format on

    if( letterSizes.count( paperId ) )
        paperType = PAGE_INFO::USLetter;
    else if( legalSizes.count( paperId ) )
        paperType = PAGE_INFO::USLegal;
    else if( a4Sizes.count( paperId ) )
        paperType = PAGE_INFO::A4;

    if( paperType )
    {
        PAGE_INFO pageInfo( paperType, data.GetOrientation() == wxPORTRAIT );

        if( pageInfo.IsPortrait() )
            data.SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ),
                                       EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ) ) );
        else
            data.SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ),
                                       EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ) ) );

        data.SetOrientation( pageInfo.GetWxOrientation() );
        data.SetPaperId( wxPAPER_NONE );
    }
#endif

    wxString selectedPrinterName;

    if( m_panelPrinters )
        selectedPrinterName = m_panelPrinters->GetSelectedPrinterName();
    data.SetPrinterName( selectedPrinterName );

    wxPrintDialogData printDialogData( data );
    printDialogData.SetMaxPage( sheet_count );

    if( sheet_count > 1 )
        printDialogData.EnablePageNumbers( true );

    wxPrinter printer( &printDialogData );
    SCH_PRINTOUT printout( m_parent, _( "Print Schematic" ) );

    Pgm().m_Printing = true;
    {
        if( !printer.Print( this, &printout, true ) )
        {
            if( wxPrinter::GetLastError() == wxPRINTER_ERROR )
                DisplayError( this, _( "An error occurred attempting to print the schematic." ) );
        }
        else
        {
            m_parent->GetPageSetupData() = printer.GetPrintDialogData().GetPrintData();
        }
    }

    Pgm().m_Printing = false;

    return true;
}
