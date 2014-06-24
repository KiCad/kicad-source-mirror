/****************************************/
/* File: dialog_print_using_printer.cpp */
/****************************************/

// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1

#include <fctsys.h>
//#include <pgm_base.h>
#include <kiface_i.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>

#include <dialog_print_using_printer_base.h>
#include <printout_controler.h>

#include <gerbview.h>
#include <pcbplot.h>

static long   s_SelectedLayers;
static double s_ScaleList[] =
{ 0, 0.5, 0.7, 0.999, 1.0, 1.4, 2.0, 3.0, 4.0 };

// Define min et max reasonnable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

// static print data and page setup data, to remember settings during the session
static wxPrintData* g_PrintData;
static wxPageSetupDialogData* g_pageSetupData = (wxPageSetupDialogData*) NULL;

// Variables locales
static PRINT_PARAMETERS  s_Parameters;


/* Dialog to print schematic. Class derived from DIALOG_PRINT_USING_PRINTER_BASE
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_BASE
{
private:
    GERBVIEW_FRAME* m_Parent;
    wxConfigBase*         m_Config;
    wxCheckBox*       m_BoxSelectLayer[32];

public:
    DIALOG_PRINT_USING_PRINTER( GERBVIEW_FRAME* parent );
    ~DIALOG_PRINT_USING_PRINTER() {};

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnInitDialog( wxInitDialogEvent& event );
    void OnPageSetup( wxCommandEvent& event );
    void OnPrintPreview( wxCommandEvent& event );
    void OnPrintButtonClick( wxCommandEvent& event );
    void OnScaleSelectionClick( wxCommandEvent& event );

    void OnButtonCancelClick( wxCommandEvent& event ) { Close(); }
    void SetPrintParameters( );
    void InitValues( );

public:
    bool IsMirrored() { return m_Print_Mirror->IsChecked(); }
    bool PrintUsingSinglePage() { return true; }
    int SetLayerSetFromListSelection();
};


/*******************************************************/
void GERBVIEW_FRAME::ToPrinter( wxCommandEvent& event )
/*******************************************************/

/* Virtual function:
 * Display the print dialog
 */
{
    if( g_PrintData == NULL )  // First print
        g_PrintData = new wxPrintData();

    if( !g_PrintData->Ok() )
    {
        DisplayError( this, _( "Error Init Printer info" ) );
        return;
    }

    g_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );
    g_PrintData->SetOrientation( GetPageSettings().IsPortrait() ?
                                 wxPORTRAIT : wxLANDSCAPE );

    DIALOG_PRINT_USING_PRINTER* frame = new DIALOG_PRINT_USING_PRINTER( this );

    frame->ShowModal();
    frame->Destroy();
}


/*************************************************************************************/
DIALOG_PRINT_USING_PRINTER::DIALOG_PRINT_USING_PRINTER( GERBVIEW_FRAME* parent ) :
    DIALOG_PRINT_USING_PRINTER_BASE( parent )
/*************************************************************************************/
{
    m_Parent = parent;
    m_Config = Kiface().KifaceSettings();

    InitValues( );
    GetSizer()->SetSizeHints( this );

#ifdef __WXMAC__
    /* Problems with modal on wx-2.9 - Anyway preview is standard for OSX */
   m_buttonPreview->Hide();
#endif
}


/************************************************************************/
void DIALOG_PRINT_USING_PRINTER::InitValues( )
/************************************************************************/
{
    SetFocus();
    LAYER_NUM layer_max = NB_GERBER_LAYERS;
    wxString msg;

    if( g_pageSetupData == NULL )
    {
        g_pageSetupData = new wxPageSetupDialogData;
        // Set initial page margins.
        // Margins are already set in Pcbnew, so we can use 0
        g_pageSetupData->SetMarginTopLeft(wxPoint(0, 0));
        g_pageSetupData->SetMarginBottomRight(wxPoint(0, 0));
    }

    s_Parameters.m_PageSetupData = g_pageSetupData;

    layer_max = NB_LAYERS;
    // Create layer list
    for( LAYER_NUM ii = FIRST_LAYER; ii < layer_max; ++ii )
    {
        LSET mask = GetLayerSet( ii );
        msg = _( "Layer" );
        msg << wxT( " " ) << ii + 1;
        m_BoxSelectLayer[ii] = new wxCheckBox( this, -1, msg );

        if( mask & s_SelectedLayers )
            m_BoxSelectLayer[ii]->SetValue( true );
        if( ii < 16 )
            m_leftLayersBoxSizer->Add( m_BoxSelectLayer[ii],
                                         wxGROW | wxLEFT | wxRIGHT | wxTOP );
        else
            m_rightLayersBoxSizer->Add( m_BoxSelectLayer[ii],
                                            wxGROW | wxLEFT | wxRIGHT | wxTOP );
    }


    // Read the scale adjust option
    int scale_idx = 4; // default selected scale = ScaleList[4] = 1.000

    if( m_Config )
    {
        m_Config->Read( OPTKEY_PRINT_X_FINESCALE_ADJ, &s_Parameters.m_XScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_Y_FINESCALE_ADJ, &s_Parameters.m_YScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_SCALE, &scale_idx );
        m_Config->Read( OPTKEY_PRINT_PAGE_FRAME, &s_Parameters.m_Print_Sheet_Ref, 1);
        m_Config->Read( OPTKEY_PRINT_MONOCHROME_MODE, &s_Parameters.m_Print_Black_and_White, 1);

        // Test for a reasonnable scale value. Set to 1 if problem
        if( s_Parameters.m_XScaleAdjust < MIN_SCALE ||
            s_Parameters.m_YScaleAdjust < MIN_SCALE ||
            s_Parameters.m_XScaleAdjust > MAX_SCALE ||
            s_Parameters.m_YScaleAdjust > MAX_SCALE )
            s_Parameters.m_XScaleAdjust = s_Parameters.m_YScaleAdjust = 1.0;

        s_SelectedLayers = 0;
        for( LAYER_NUM layer = FIRST_LAYER; layer < layer_max; ++layer )
        {
            wxString layerKey;
            bool     option;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            option = false;
            if( m_Config->Read( layerKey, &option ) )
            {
                m_BoxSelectLayer[layer]->SetValue( option );
                if( option )
                    s_SelectedLayers |= GetLayerSet( layer );
            }
        }
    }

    m_ScaleOption->SetSelection( scale_idx );
    scale_idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale =  s_ScaleList[scale_idx];
    m_Print_Mirror->SetValue(s_Parameters.m_PrintMirror);


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
    if( m_FineAdjustXscaleOpt )
        m_FineAdjustXscaleOpt->Enable(enable);
    if( m_FineAdjustYscaleOpt )
        m_FineAdjustYscaleOpt->Enable(enable);
}

/**************************************************************/
int DIALOG_PRINT_USING_PRINTER::SetLayerSetFromListSelection()
/**************************************************************/
{
    int page_count = 0;
    s_Parameters.m_PrintMaskLayer = NO_LAYERS;
    for( LAYER_NUM ii = FIRST_LAYER; ii < NB_GERBER_LAYERS; ++ii )
    {
        if( m_BoxSelectLayer[ii]->IsChecked() )
        {
            page_count++;
            s_Parameters.m_PrintMaskLayer |= GetLayerSet( ii );
        }
    }

    s_Parameters.m_PageCount = page_count;

    return page_count;
}


/********************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnCloseWindow( wxCloseEvent& event )
/********************************************************************/
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
        for( LAYER_NUM layer = FIRST_LAYER; layer < NB_GERBER_LAYERS; ++layer )
        {
            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            m_Config->Write( layerKey, m_BoxSelectLayer[layer]->IsChecked() );
        }
    }
    EndModal( 0 );
}


/******************************************************************/
void DIALOG_PRINT_USING_PRINTER::SetPrintParameters( )
/******************************************************************/
{
    s_Parameters.m_PrintMirror = m_Print_Mirror->GetValue();
    s_Parameters.m_Print_Black_and_White =
        m_ModeColorOption->GetSelection() != 0;

    // Due to negative objects in gerber objects, always use one page per image,
    // because these objects create artefact when they are printed on an existing image.
    s_Parameters.m_OptionPrintPage = false;

    SetLayerSetFromListSelection();

    int idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale =  s_ScaleList[idx];

    if( m_FineAdjustXscaleOpt )
    {
        if( s_Parameters.m_XScaleAdjust > MAX_SCALE ||
            s_Parameters.m_YScaleAdjust > MAX_SCALE )
            DisplayInfoMessage( NULL, _( "Warning: Scale option set to a very large value" ) );
        m_FineAdjustXscaleOpt->GetValue().ToDouble( &s_Parameters.m_XScaleAdjust );
    }
    if( m_FineAdjustYscaleOpt )
    {
        // Test for a reasonnable scale value
        if( s_Parameters.m_XScaleAdjust < MIN_SCALE ||
            s_Parameters.m_YScaleAdjust < MIN_SCALE )
            DisplayInfoMessage( NULL, _( "Warning: Scale option set to a very small value" ) );
        m_FineAdjustYscaleOpt->GetValue().ToDouble( &s_Parameters.m_YScaleAdjust );
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

/**********************************************************/
void DIALOG_PRINT_USING_PRINTER::OnPageSetup( wxCommandEvent& event )
/**********************************************************/

/* Open a dialog box for printer setup (printer options, page size ...)
 */
{
    *g_pageSetupData = *g_PrintData;

    wxPageSetupDialog pageSetupDialog(this, g_pageSetupData);
    pageSetupDialog.ShowModal();

    (*g_PrintData) = pageSetupDialog.GetPageSetupDialogData().GetPrintData();
    (*g_pageSetupData) = pageSetupDialog.GetPageSetupDialogData();
}


/************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnPrintPreview( wxCommandEvent& event )
/************************************************************/

/* Open and display a previewer frame for printing
 */
{
    SetPrintParameters( );

    // Pass two printout objects: for preview, and possible printing.
    wxString        title   = _( "Print Preview" );
    wxPrintPreview* preview =
        new wxPrintPreview( new BOARD_PRINTOUT_CONTROLLER( s_Parameters, m_Parent, title ),
                            new BOARD_PRINTOUT_CONTROLLER( s_Parameters, m_Parent, title ),
                            g_PrintData );

    if( preview == NULL )
    {
        DisplayError( this, wxT( "OnPrintPreview() problem" ) );
        return;
    }

    SetLayerSetFromListSelection();

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in Pcbnew:
    if( s_Parameters.m_PrintMaskLayer == 0 )
    {
        DisplayError( this, _( "No layer selected" ) );
        return;
    }


    // Uses the parent position and size.
    // @todo uses last position and size ans store them when exit in m_Config
    wxPoint         WPos  = m_Parent->GetPosition();
    wxSize          WSize = m_Parent->GetSize();

    wxPreviewFrame* frame = new wxPreviewFrame( preview, this, title, WPos, WSize );

    frame->Initialize();
    frame->Show( true );
}


/***************************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnPrintButtonClick( wxCommandEvent& event )
/***************************************************************************/

/* Called on activate Print button
 */
{
    SetPrintParameters( );

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in Pcbnew:
    if( s_Parameters.m_PrintMaskLayer == 0 )
    {
        DisplayError( this, _( "No layer selected" ) );
        return;
    }

    wxPrintDialogData printDialogData( *g_PrintData );

    wxPrinter         printer( &printDialogData );

    wxString          title = _( "Print" );
    BOARD_PRINTOUT_CONTROLLER      printout( s_Parameters, m_Parent, title );

#if !defined(__WINDOWS__) && !wxCHECK_VERSION(2,9,0)
    wxDC*             dc = printout.GetDC();
    ( (wxPostScriptDC*) dc )->SetResolution( 600 );  // Postscript DC resolution is 600 ppi
#endif

    if( !printer.Print( this, &printout, true ) )
    {
        if( wxPrinter::GetLastError() == wxPRINTER_ERROR )
            DisplayError( this, _( "There was a problem printing" ) );
        return;
    }
    else
    {
        *g_PrintData = printer.GetPrintDialogData().GetPrintData();
    }
}

