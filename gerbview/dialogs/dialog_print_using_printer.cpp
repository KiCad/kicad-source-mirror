/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2016 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiface_i.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>

#include <dialog_print_using_printer_base.h>
#include <printout_controler.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>

///@{
/// \ingroup config

#define OPTKEY_LAYERBASE             wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_X_FINESCALE_ADJ wxT( "PrintXFineScaleAdj" )
#define OPTKEY_PRINT_Y_FINESCALE_ADJ wxT( "PrintYFineScaleAdj" )
#define OPTKEY_PRINT_SCALE           wxT( "PrintScale" )
#define OPTKEY_PRINT_PAGE_FRAME      wxT( "PrintPageFrame" )
#define OPTKEY_PRINT_MONOCHROME_MODE wxT( "PrintMonochrome" )

///@}

static double s_ScaleList[] =
{ 0, 0.5, 0.7, 0.999, 1.0, 1.4, 2.0, 3.0, 4.0 };

// Define min et max reasonnable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

// static print data and page setup data, to remember settings during the session
static wxPrintData* s_printData;
static wxPageSetupDialogData* s_pageSetupData = (wxPageSetupDialogData*) NULL;

// Variables locales
static PRINT_PARAMETERS  s_Parameters;


/* Dialog to print schematic. Class derived from DIALOG_PRINT_USING_PRINTER_BASE
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_BASE
{
private:
    GERBVIEW_FRAME* m_Parent;
    wxConfigBase*   m_Config;
    wxCheckBox*     m_BoxSelectLayer[32];

public:
    DIALOG_PRINT_USING_PRINTER( GERBVIEW_FRAME* parent );
    ~DIALOG_PRINT_USING_PRINTER() {};

private:
    void OnCloseWindow( wxCloseEvent& event ) override;
    void OnInitDialog( wxInitDialogEvent& event );
    void OnPageSetup( wxCommandEvent& event ) override;
    void OnPrintPreview( wxCommandEvent& event ) override;
    void OnPrintButtonClick( wxCommandEvent& event ) override;
    void OnScaleSelectionClick( wxCommandEvent& event ) override;

    void OnButtonCloseClick( wxCommandEvent& event ) override { Close(); }
    void SetPrintParameters();
    void InitValues();

public:
    bool IsMirrored() { return m_Print_Mirror->IsChecked(); }
    bool PrintUsingSinglePage() { return true; }
    int  SetLayerSetFromListSelection();
    // Prepare print parameters. return true if OK,
    // false if there is an issue (mainly no printable layers)
    bool PreparePrintPrms();
};


void GERBVIEW_FRAME::ToPrinter( wxCommandEvent& event )
{
    if( s_printData == NULL )  // First print
        s_printData = new wxPrintData();

    if( !s_printData->Ok() )
    {
        DisplayError( this, _( "Error Init Printer info" ) );
        return;
    }

    s_printData->SetQuality( wxPRINT_QUALITY_HIGH );
    s_printData->SetOrientation( GetPageSettings().IsPortrait() ?
                                 wxPORTRAIT : wxLANDSCAPE );

    DIALOG_PRINT_USING_PRINTER* frame = new DIALOG_PRINT_USING_PRINTER( this );

    frame->ShowModal();
    frame->Destroy();
}


DIALOG_PRINT_USING_PRINTER::DIALOG_PRINT_USING_PRINTER( GERBVIEW_FRAME* parent ) :
    DIALOG_PRINT_USING_PRINTER_BASE( parent )
{
    m_Parent = parent;
    m_Config = Kiface().KifaceSettings();

    InitValues( );
    GetSizer()->SetSizeHints( this );

#ifdef __WXMAC__
    /* Problems with modal on wx-2.9 - Anyway preview is standard for OSX */
   m_buttonPreview->Hide();
#endif

    GetSizer()->Fit( this );
}


void DIALOG_PRINT_USING_PRINTER::InitValues( )
{
    SetFocus();
    wxString msg;

    if( s_pageSetupData == NULL )
    {
        s_pageSetupData = new wxPageSetupDialogData;
        // Set initial page margins.
        // Margins are already set in Gerbview, so we can use 0
        s_pageSetupData->SetMarginTopLeft( wxPoint( 0, 0 ) );
        s_pageSetupData->SetMarginBottomRight( wxPoint( 0, 0 ) );
    }

    s_Parameters.m_PageSetupData = s_pageSetupData;
    GERBER_FILE_IMAGE_LIST* images = m_Parent->GetGerberLayout()->GetImagesList();

    // Create layer list
    for( unsigned ii = 0; ii < images->ImagesMaxCount(); ++ii )
    {
        msg = _( "Layer" );
        msg << wxT( " " ) << ii + 1;

        wxStaticBoxSizer* boxSizer = ( ii < 16 ) ? m_leftLayersBoxSizer
                                                 : m_rightLayersBoxSizer;

        m_BoxSelectLayer[ii] = new wxCheckBox( boxSizer->GetStaticBox(),
                                               wxID_ANY, msg );
        boxSizer->Add( m_BoxSelectLayer[ii], wxGROW | wxLEFT | wxRIGHT | wxTOP );

        if( images->GetGbrImage( ii ) == NULL )     // Nothing loaded on this draw layer
            m_BoxSelectLayer[ii]->Enable( false );
    }

    // Read the scale adjust option
    int scale_idx = 4; // default selected scale = ScaleList[4] = 1.000

    if( m_Config )
    {
        m_Config->Read( OPTKEY_PRINT_X_FINESCALE_ADJ, &s_Parameters.m_XScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_Y_FINESCALE_ADJ, &s_Parameters.m_YScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_SCALE, &scale_idx );
        m_Config->Read( OPTKEY_PRINT_PAGE_FRAME, &s_Parameters.m_Print_Sheet_Ref, 1 );
        m_Config->Read( OPTKEY_PRINT_MONOCHROME_MODE, &s_Parameters.m_Print_Black_and_White, 1 );

        // Test for a reasonnable scale value. Set to 1 if problem
        if( s_Parameters.m_XScaleAdjust < MIN_SCALE ||
            s_Parameters.m_YScaleAdjust < MIN_SCALE ||
            s_Parameters.m_XScaleAdjust > MAX_SCALE ||
            s_Parameters.m_YScaleAdjust > MAX_SCALE )
            s_Parameters.m_XScaleAdjust = s_Parameters.m_YScaleAdjust = 1.0;

        for( int layer = 0; layer < GERBER_DRAWLAYERS_COUNT; ++layer )
        {
            wxString layerKey;
            bool     option;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            m_Config->Read( layerKey, &option, false );
            m_BoxSelectLayer[layer]->SetValue( option );
        }
    }

    m_ScaleOption->SetSelection( scale_idx );
    scale_idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale = s_ScaleList[scale_idx];
    m_Print_Mirror->SetValue( s_Parameters.m_PrintMirror );


    if( s_Parameters.m_Print_Black_and_White )
        m_ModeColorOption->SetSelection( 1 );
    else
        m_ModeColorOption->SetSelection( 0 );

    s_Parameters.m_PenDefaultSize = 0;

    // Create scale adjust option
    msg.Printf( wxT( "%f" ), s_Parameters.m_XScaleAdjust );
    m_FineAdjustXscaleOpt->SetValue( msg );
    msg.Printf( wxT( "%f" ), s_Parameters.m_YScaleAdjust );
    m_FineAdjustYscaleOpt->SetValue( msg );

    bool enable = (s_Parameters.m_PrintScale == 1.0);

    m_FineAdjustXscaleOpt->Enable(enable);
    m_FineAdjustYscaleOpt->Enable(enable);
}


int DIALOG_PRINT_USING_PRINTER::SetLayerSetFromListSelection()
{
    std::vector<int> layerList;

    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        if( m_BoxSelectLayer[ii]->IsChecked() && m_BoxSelectLayer[ii]->IsEnabled() )
            layerList.push_back( ii );
    }

    m_Parent->GetGerberLayout()->SetPrintableLayers( layerList );
    s_Parameters.m_PageCount = layerList.size();

    return s_Parameters.m_PageCount;
}


void DIALOG_PRINT_USING_PRINTER::OnCloseWindow( wxCloseEvent& event )
{
    SetPrintParameters();

    if( m_Config )
    {
        m_Config->Write( OPTKEY_PRINT_X_FINESCALE_ADJ, s_Parameters.m_XScaleAdjust );
        m_Config->Write( OPTKEY_PRINT_Y_FINESCALE_ADJ, s_Parameters.m_YScaleAdjust );
        m_Config->Write( OPTKEY_PRINT_SCALE, m_ScaleOption->GetSelection() );
        m_Config->Write( OPTKEY_PRINT_PAGE_FRAME, s_Parameters.m_Print_Sheet_Ref);
        m_Config->Write( OPTKEY_PRINT_MONOCHROME_MODE, s_Parameters.m_Print_Black_and_White);
        wxString layerKey;

        for( int layer = 0; layer < GERBER_DRAWLAYERS_COUNT; ++layer )
        {
            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            m_Config->Write( layerKey, m_BoxSelectLayer[layer]->IsChecked() );
        }
    }

    EndModal( 0 );
}


void DIALOG_PRINT_USING_PRINTER::SetPrintParameters()
{
    s_Parameters.m_PrintMirror = m_Print_Mirror->GetValue();
    s_Parameters.m_Print_Black_and_White =
        m_ModeColorOption->GetSelection() != 0;

    // Due to negative objects in gerber objects, always use one page per image,
    // because these objects create artefact when they are printed on an existing image.
    s_Parameters.m_OptionPrintPage = false;

    SetLayerSetFromListSelection();

    int idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale = s_ScaleList[idx];

    // Test for a reasonnable scale value
    bool ok = true;
    double scaleX;
    m_FineAdjustXscaleOpt->GetValue().ToDouble( &scaleX );

    double scaleY;
    m_FineAdjustYscaleOpt->GetValue().ToDouble( &scaleY );

    if( scaleX > MAX_SCALE || scaleY > MAX_SCALE )
    {
        DisplayInfoMessage( NULL, _( "Warning: Scale option set to a very large value" ) );
        ok = false;
    }

    if( scaleX < MIN_SCALE || scaleY < MIN_SCALE )
    {
        DisplayInfoMessage( NULL, _( "Warning: Scale option set to a very small value" ) );
        ok = false;
    }

    if( ok )
    {
        s_Parameters.m_XScaleAdjust = scaleX;
        s_Parameters.m_YScaleAdjust = scaleY;
    }
    else
    {
        // Update actual fine scale value
        m_FineAdjustXscaleOpt->SetValue( wxString::Format( "%f", s_Parameters.m_XScaleAdjust ) );
        m_FineAdjustYscaleOpt->SetValue( wxString::Format( "%f", s_Parameters.m_YScaleAdjust ) );
    }
}

void DIALOG_PRINT_USING_PRINTER::OnScaleSelectionClick( wxCommandEvent& event )
{
    double scale = s_ScaleList[m_ScaleOption->GetSelection()];
    bool enable = (scale == 1.0);

    if( m_FineAdjustXscaleOpt )
        m_FineAdjustXscaleOpt->Enable(enable);

    if( m_FineAdjustYscaleOpt )
        m_FineAdjustYscaleOpt->Enable(enable);
}

// Open a dialog box for printer setup (printer options, page size ...)
void DIALOG_PRINT_USING_PRINTER::OnPageSetup( wxCommandEvent& event )
{
    *s_pageSetupData = *s_printData;

    wxPageSetupDialog pageSetupDialog(this, s_pageSetupData);
    pageSetupDialog.ShowModal();

    (*s_printData) = pageSetupDialog.GetPageSetupDialogData().GetPrintData();
    (*s_pageSetupData) = pageSetupDialog.GetPageSetupDialogData();
}

bool DIALOG_PRINT_USING_PRINTER::PreparePrintPrms()
{
    SetPrintParameters();

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in Pcbnew:
    if( m_Parent->GetGerberLayout()->GetPrintableLayers().size() == 0 )
    {
        DisplayError( this, _( "No layer selected" ) );
        return false;
    }

    return true;
}

// Open and display a previewer frame for printing
void DIALOG_PRINT_USING_PRINTER::OnPrintPreview( wxCommandEvent& event )
{
    if( !PreparePrintPrms() )
        return;

    // Pass two printout objects: for preview, and possible printing.
    wxString        title   = _( "Print Preview" );
    wxPrintPreview* preview =
        new wxPrintPreview( new BOARD_PRINTOUT_CONTROLLER( s_Parameters, m_Parent, title ),
                            new BOARD_PRINTOUT_CONTROLLER( s_Parameters, m_Parent, title ),
                            s_printData );

    if( preview == NULL )
    {
        DisplayError( this, wxT( "OnPrintPreview() problem" ) );
        return;
    }


    // Uses the parent position and size.
    // @todo uses last position and size ans store them when exit in m_Config
    wxPoint         WPos  = m_Parent->GetPosition();
    wxSize          WSize = m_Parent->GetSize();

    wxPreviewFrame* frame = new wxPreviewFrame( preview, this, title, WPos, WSize );
    frame->SetMinSize( wxSize( 550, 350 ) );

    frame->Initialize();

    frame->Raise(); // Needed on Ubuntu/Unity to display the frame
    frame->Show( true );
}


void DIALOG_PRINT_USING_PRINTER::OnPrintButtonClick( wxCommandEvent& event )
{
    if( !PreparePrintPrms() )
        return;

    wxPrintDialogData printDialogData( *s_printData );

    wxPrinter printer( &printDialogData );
    wxString title = _( "Print" );
    BOARD_PRINTOUT_CONTROLLER printout( s_Parameters, m_Parent, title );

    if( !printer.Print( this, &printout, true ) )
    {
        if( wxPrinter::GetLastError() == wxPRINTER_ERROR )
            DisplayError( this, _( "There was a problem printing" ) );
        return;
    }
    else
    {
        *s_printData = printer.GetPrintDialogData().GetPrintData();
    }
}

