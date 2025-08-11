/*
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_print_generic.h>

#include <eda_draw_frame.h>
#include <printout.h>
#include <pgm_base.h>
#include <confirm.h>

#include <wx/print.h>
#include <wx/printdlg.h>

#include <dialogs/panel_printer_list.h>

// Define min and max reasonable values for print scale
static constexpr double MIN_SCALE = 0.01;
static constexpr double MAX_SCALE = 100.0;

wxPrintData*           DIALOG_PRINT_GENERIC::s_printData = nullptr;
wxPageSetupDialogData* DIALOG_PRINT_GENERIC::s_pageSetupData = nullptr;


/**
 * Custom print preview frame.
 *
 * This derived preview frame remembers its size and position during a session.
 */
class KI_PREVIEW_FRAME : public wxPreviewFrame
{
public:
    KI_PREVIEW_FRAME( wxPrintPreview* aPreview, wxWindow* aParent, const wxString& aTitle,
                      const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize ) :
            wxPreviewFrame( aPreview, aParent, aTitle, aPos, aSize )
    {
    }

    bool Show( bool show ) override
    {
        bool ret;

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


wxPoint KI_PREVIEW_FRAME::s_pos;
wxSize  KI_PREVIEW_FRAME::s_size;


DIALOG_PRINT_GENERIC::DIALOG_PRINT_GENERIC( EDA_DRAW_FRAME* aParent, PRINTOUT_SETTINGS* aSettings ) :
        DIALOG_PRINT_GENERIC_BASE( aParent ),
        m_settings( aSettings )
{
    // Show m_panelPrinters only if there are printers to list:
    m_panelPrinters->Show( m_panelPrinters->AsPrintersAvailable() );

    SetupStandardButtons( { { wxID_OK,     _( "Print" )         },
                            { wxID_APPLY,  _( "Print Preview" ) },
                            { wxID_CANCEL, _( "Close" )         } } );

#if defined(__WXMAC__) or defined(__WXGTK__)
    // Preview does not work well on GTK or Mac,
    // but these platforms provide native print preview
    m_sdbSizer1Apply->Hide();
#endif

    finishDialogSettings();
    Layout();
    initPrintData();
}


void DIALOG_PRINT_GENERIC::ForcePrintBorder( bool aValue )
{
    m_titleBlock->SetValue( aValue );
    m_titleBlock->Hide();
}


void DIALOG_PRINT_GENERIC::saveSettings()
{
    m_settings->m_scale = getScaleValue();
    m_settings->m_titleBlock = m_titleBlock->GetValue();
    m_settings->m_blackWhite = m_outputMode->GetSelection();
}


double DIALOG_PRINT_GENERIC::getScaleValue()
{
    if( m_scale1->GetValue() )
    {
        return 1.0;
    }
    else if( m_scaleFit->GetValue() )
    {
        return 0.0;
    }
    else if( m_scaleCustom->GetValue() )
    {
        double scale = 1.0;;

        if( !m_scaleCustomText->GetValue().ToDouble( &scale ) )
        {
            DisplayInfoMessage( nullptr, _( "Warning: custom scale is not a number." ) );
            setScaleValue( 1.0 );
            scale = 1.0;
        }

        if( scale > MAX_SCALE )
        {
            scale = MAX_SCALE;
            setScaleValue( scale );
            DisplayInfoMessage( nullptr, wxString::Format( _( "Warning: custom scale is too large.\n"
                                                              "It will be clamped to %f." ), scale ) );
        }
        else if( scale < MIN_SCALE )
        {
            scale = MIN_SCALE;
            setScaleValue( scale );
            DisplayInfoMessage( nullptr, wxString::Format( _( "Warning: custom scale is too small.\n"
                                                              "It will be clamped to %f." ), scale ) );
        }

        return scale;
    }

    wxFAIL_MSG( wxT( "No scale option selected." ) );
    return 1.0;
}


void DIALOG_PRINT_GENERIC::setScaleValue( double aValue )
{
    wxASSERT( aValue >= 0.0 );

    if( aValue == 0.0 )     // fit to page
    {
        m_scaleFit->SetValue( true );
    }
    else if( aValue == 1.0 )
    {
        m_scale1->SetValue( true );
    }
    else
    {
        // Silently clamp the value (it comes from the config file).
        if( aValue > MAX_SCALE )
            aValue = MAX_SCALE;
        else if( aValue < MIN_SCALE )
            aValue = MIN_SCALE;

        m_scaleCustom->SetValue( true );
        m_scaleCustomText->SetValue( wxString::Format( wxT( "%f" ), aValue ) );
    }
}


bool DIALOG_PRINT_GENERIC::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    setScaleValue( m_settings->m_scale );
    m_titleBlock->SetValue( m_settings->m_titleBlock );
    m_outputMode->SetSelection( m_settings->m_blackWhite ? 1 : 0 );

    return true;
}


void DIALOG_PRINT_GENERIC::onPageSetup( wxCommandEvent& event )
{
    wxPageSetupDialog pageSetupDialog( this, s_pageSetupData );
    pageSetupDialog.ShowModal();

    (*s_printData ) = pageSetupDialog.GetPageSetupDialogData().GetPrintData();
    (*s_pageSetupData) = pageSetupDialog.GetPageSetupDialogData();
}


void DIALOG_PRINT_GENERIC::onPrintPreview( wxCommandEvent& event )
{
    m_settings->m_pageCount = 0;    // it needs to be set by a derived dialog
    saveSettings();

    if( m_settings->m_pageCount == 0 )
    {
        DisplayError( this, _( "Nothing to print" ) );
        return;
    }

    wxString selectedPrinterName;

    if( m_panelPrinters )
        selectedPrinterName = m_panelPrinters->GetSelectedPrinterName();

    s_printData->SetPrinterName( selectedPrinterName );

    // Pass two printout objects: for preview, and possible printing.
    wxString title = _( "Print Preview" );
    wxPrintPreview* preview = new wxPrintPreview( createPrintout( title ), createPrintout( title ), s_printData );

    preview->SetZoom( 100 );

    KI_PREVIEW_FRAME* frame = new KI_PREVIEW_FRAME( preview, this, title );

    // On wxGTK, set the flag wxTOPLEVEL_EX_DIALOG is mandatory, if we want
    // close the frame using the X box in caption, when the preview frame is run
    // from a dialog
    frame->SetExtraStyle( frame->GetExtraStyle() | wxTOPLEVEL_EX_DIALOG );

    // We use here wxPreviewFrame_WindowModal option to make the wxPrintPreview frame
    // modal for its caller only.
    // An other reason is the fact when closing the frame without this option,
    // all top level frames are reenabled.
    // With this option, only the parent is reenabled.
    // Reenabling all top level frames should be made by the parent dialog.
    frame->InitializeWithModality( wxPreviewFrame_WindowModal );

    // on first invocation in this runtime session, set to 3/4 size of parent,
    // but will be changed in Show() if not first time as will position.
    // Must be called after InitializeWithModality because otherwise in some wxWidget
    // versions it is not always taken in account
    frame->SetMinSize( wxSize( 650, 500 ) );
    frame->SetSize( ( m_parent->GetSize() * 3 ) / 4 );

    frame->Raise(); // Needed on Ubuntu/Unity to display the frame
    frame->Show( true );
}


void DIALOG_PRINT_GENERIC::onPrintButtonClick( wxCommandEvent& event )
{
    if( Pgm().m_Printing )
    {
        DisplayError( this, _( "Previous print job not yet complete." ) );
        return;
    }

    m_settings->m_pageCount = 0;    // it needs to be set by a derived dialog
    saveSettings();

    if( m_settings->m_pageCount == 0 )
    {
        DisplayError( this, _( "Nothing to print" ) );
        return;
    }

    wxString selectedPrinterName;

    if( m_panelPrinters )
        selectedPrinterName = m_panelPrinters->GetSelectedPrinterName();

    s_printData->SetPrinterName( selectedPrinterName );

    wxPrintDialogData printDialogData( *s_printData );
    printDialogData.SetMaxPage( m_settings->m_pageCount );

    wxPrinter printer( &printDialogData );
    auto printout = std::unique_ptr<wxPrintout>( createPrintout( _( "Print" ) ) );

    Pgm().m_Printing = true;

    {
        if( !printer.Print( this, printout.get(), true ) )
        {
            if( wxPrinter::GetLastError() == wxPRINTER_ERROR )
                DisplayError( this, _( "There was a problem printing." ) );
        }
        else
        {
            *s_printData = printer.GetPrintDialogData().GetPrintData();
        }
    }

    Pgm().m_Printing = false;
}


void DIALOG_PRINT_GENERIC::onCancelButtonClick( wxCommandEvent& event )
{
    saveSettings();

    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL ) );
}


void DIALOG_PRINT_GENERIC::onClose( wxCloseEvent& event )
{
    saveSettings();
    event.Skip();
}


void DIALOG_PRINT_GENERIC::onSetCustomScale( wxCommandEvent& event )
{
    // Select 'custom scale' radio button when user types in a value in the
    // custom scale text box
    m_scaleCustom->SetValue( true );
}


void DIALOG_PRINT_GENERIC::initPrintData()
{
    if( !s_printData )  // First print
    {
        s_printData = new wxPrintData();

        if( !s_printData->Ok() )
            DisplayError( this, _( "An error occurred initializing the printer information." ) );

        s_printData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGH;
    }

    if( !s_pageSetupData )
    {
        const PAGE_INFO& pageInfo = m_settings->m_pageInfo;

        s_pageSetupData = new wxPageSetupDialogData( *s_printData );
        s_pageSetupData->SetPaperId( pageInfo.GetPaperId() );
        s_pageSetupData->GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

        if( pageInfo.IsCustom() )
        {
            if( pageInfo.IsPortrait() )
            {
                s_pageSetupData->SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ),
                                                       EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ) ) );
            }
            else
            {
                s_pageSetupData->SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ),
                                                       EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ) ) );
            }
        }

        *s_printData = s_pageSetupData->GetPrintData();
    }
}

