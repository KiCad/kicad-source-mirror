/*
 * Copyright (C) 2018 CERN
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

#include "dialog_print_generic.h"

#include <confirm.h>
#include <eda_draw_frame.h>
#include <printout.h>
#include <pgm_base.h>

// Define min and max reasonable values for print scale
static constexpr double MIN_SCALE = 0.01;
static constexpr double MAX_SCALE = 100.0;

DIALOG_PRINT_GENERIC::DIALOG_PRINT_GENERIC( EDA_DRAW_FRAME* aParent, PRINTOUT_SETTINGS* aSettings )
    : DIALOG_PRINT_GENERIC_BASE( aParent ), m_config( nullptr ), m_settings( aSettings )
{
    // Note: for the validator, min value is 0.0, to allow typing values like 0.5
    // that start by 0
    m_scaleValidator.SetRange( 0.0, MAX_SCALE );
    m_scaleCustomText->SetValidator( m_scaleValidator );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Print" ) );
    m_sdbSizer1Apply->SetLabel( _( "Print Preview" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    m_sdbSizer1OK->SetDefault();

#if defined(__WXMAC__) or defined(__WXGTK__)
    // Preview does not work well on GTK or Mac,
    // but these platforms provide native print preview
    m_sdbSizer1Apply->Hide();
#endif

    FinishDialogSettings();
    Layout();
    initPrintData();
}


DIALOG_PRINT_GENERIC::~DIALOG_PRINT_GENERIC()
{
}


void DIALOG_PRINT_GENERIC::ForcePrintBorder( bool aValue )
{
    m_titleBlock->SetValue( aValue );
    m_titleBlock->Hide();

    if( m_config )
    {
        m_settings->Load( m_config );
        m_settings->m_titleBlock = aValue;
        m_settings->Save( m_config );
    }
}


void DIALOG_PRINT_GENERIC::saveSettings()
{
    m_settings->m_scale = getScaleValue();
    m_settings->m_titleBlock = m_titleBlock->GetValue();
    m_settings->m_blackWhite = m_outputMode->GetSelection();

    if( m_config )
        m_settings->Save( m_config );
}


double DIALOG_PRINT_GENERIC::getScaleValue()
{
    if( m_scale1->GetValue() )
        return 1.0;

    if( m_scaleFit->GetValue() )
        return 0.0;

    if( m_scaleCustom->GetValue() )
    {
        double scale = 1.0;;

        if( !m_scaleCustomText->GetValue().ToDouble( &scale ) )
        {
            DisplayInfoMessage( nullptr, _( "Warning: Bad scale number" ) );
            scale = 1.0;
        }

        if( scale > MAX_SCALE )
        {
            scale = MAX_SCALE;
            setScaleValue( scale );
            DisplayInfoMessage( nullptr,
                wxString::Format( _( "Warning: Scale option set to a very large value.\n"
                                     " Clamped to %f" ), scale ) );
        }
        else if( scale < MIN_SCALE )
        {
            scale = MIN_SCALE;
            setScaleValue( scale );
            DisplayInfoMessage( nullptr,
                wxString::Format( _( "Warning: Scale option set to a very small value.\n"
                                     " Clamped to %f" ), scale ) );
        }
        return scale;
    }

    wxCHECK( false, 1.0 );
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

    if( m_config )
        m_settings->Load( m_config );

    setScaleValue( m_settings->m_scale );
    m_titleBlock->SetValue( m_settings->m_titleBlock );
    m_outputMode->SetSelection( m_settings->m_blackWhite ? 1 : 0 );

    return true;
}


void DIALOG_PRINT_GENERIC::onPageSetup( wxCommandEvent& event )
{
    wxPageSetupDialog pageSetupDialog( this, s_pageSetupData );
    pageSetupDialog.ShowModal();

    (*s_PrintData) = pageSetupDialog.GetPageSetupDialogData().GetPrintData();
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

    // Pass two printout objects: for preview, and possible printing.
    wxString title = _( "Print Preview" );
    wxPrintPreview* preview =
            new wxPrintPreview( createPrintout( title ), createPrintout( title ), s_PrintData );

    preview->SetZoom( 100 );

    wxPreviewFrame* frame = new wxPreviewFrame( preview, this, title, m_parent->GetPosition(),
                                                m_parent->GetSize() );
    frame->SetMinSize( wxSize( 550, 350 ) );
    frame->Center();

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

    wxPrintDialogData printDialogData( *s_PrintData );
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
            *s_PrintData = printer.GetPrintDialogData().GetPrintData();
        }
    }
    Pgm().m_Printing = false;
}


void DIALOG_PRINT_GENERIC::onCloseButton( wxCommandEvent& event )
{
    saveSettings();

    if( IsQuasiModal() )
        EndQuasiModal( wxID_CANCEL );

    if( IsModal() )
        EndModal( wxID_CANCEL );

    Close();
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
    if( !s_PrintData )  // First print
    {
        s_PrintData = new wxPrintData();

        if( !s_PrintData->Ok() )
            DisplayError( this, _( "An error occurred initializing the printer information." ) );

        s_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGH;
    }

    if( !s_pageSetupData )
    {
        const PAGE_INFO& pageInfo = m_settings->m_pageInfo;

        s_pageSetupData = new wxPageSetupDialogData( *s_PrintData );
        s_pageSetupData->SetPaperId( pageInfo.GetPaperId() );
        s_pageSetupData->GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

        if( pageInfo.IsCustom() )
        {
            if( pageInfo.IsPortrait() )
                s_pageSetupData->SetPaperSize( wxSize( Mils2mm( pageInfo.GetWidthMils() ),
                                                    Mils2mm( pageInfo.GetHeightMils() ) ) );
            else
                s_pageSetupData->SetPaperSize( wxSize( Mils2mm( pageInfo.GetHeightMils() ),
                                                    Mils2mm( pageInfo.GetWidthMils() ) ) );
        }

        *s_PrintData = s_pageSetupData->GetPrintData();
    }
}


wxPrintData* DIALOG_PRINT_GENERIC::s_PrintData = nullptr;
wxPageSetupDialogData* DIALOG_PRINT_GENERIC::s_pageSetupData = nullptr;
