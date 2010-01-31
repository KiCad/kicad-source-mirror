/****************************************/
/* File: dialog_print_using_printer.cpp */
/****************************************/

// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "dialog_print_using_printer_base.h"
#include "printout_controler.h"

#include "gerbview.h"
#include "wxGerberFrame.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbplot.h"
#include "class_board_design_settings.h"

#define WIDTH_MAX_VALUE           1000
#define WIDTH_MIN_VALUE           1

static long   s_SelectedLayers;
static double s_ScaleList[] =
{ 0, 0.5, 0.7, 0.999, 1.0, 1.4, 2.0, 3.0, 4.0 };

// Define min et max reasonnable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

// static print data and page setup data, to remember settings during the session
static wxPrintData* g_PrintData;

// Variables locales
static PRINT_PARAMETERS  s_Parameters;


/* Dialog to print schematic. Class derived from DIALOG_PRINT_USING_PRINTER_base
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_base
{
private:
    WinEDA_GerberFrame* m_Parent;
    wxConfig*         m_Config;
    wxCheckBox*       m_BoxSelectLayer[32];
    static wxPoint      s_LastPos;
    static wxSize       s_LastSize;

public:
    DIALOG_PRINT_USING_PRINTER( WinEDA_GerberFrame* parent );
    ~DIALOG_PRINT_USING_PRINTER() {};

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnInitDialog( wxInitDialogEvent& event );
    void OnPrintSetup( wxCommandEvent& event );
    void OnPrintPreview( wxCommandEvent& event );
    void OnPrintButtonClick( wxCommandEvent& event );

    void OnButtonCancelClick( wxCommandEvent& event ) { Close(); }
    void SetPrintParameters( );
    void InitValues( );

    bool Show( bool show );     // overload stock function

public:
    bool IsMirrored() { return m_Print_Mirror->IsChecked(); }
    bool PrintUsingSinglePage() { return m_PagesOption->GetSelection(); }
    int SetLayerMaskFromListSelection();
};

// We want our dialog to remember its previous screen position
wxPoint DIALOG_PRINT_USING_PRINTER::s_LastPos( -1, -1 );
wxSize  DIALOG_PRINT_USING_PRINTER::s_LastSize;


/*******************************************************/
void WinEDA_GerberFrame::ToPrinter( wxCommandEvent& event )
/*******************************************************/

/* Virtual function:
 * Display the print dialog
 */
{
    if( g_PrintData == NULL )  // First print
    {
        g_PrintData = new wxPrintData();

        if( !g_PrintData->Ok() )
        {
            DisplayError( this, _( "Error Init Printer info" ) );
        }
        g_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGHT;
        g_PrintData->SetOrientation( DEFAULT_ORIENTATION_PAPER );
    }

    DIALOG_PRINT_USING_PRINTER* frame = new DIALOG_PRINT_USING_PRINTER( this );

    frame->ShowModal(); frame->Destroy();
}


/*************************************************************************************/
DIALOG_PRINT_USING_PRINTER::DIALOG_PRINT_USING_PRINTER( WinEDA_GerberFrame* parent ) :
    DIALOG_PRINT_USING_PRINTER_base( parent )
/*************************************************************************************/
{
    m_Parent = parent;
    m_Config = wxGetApp().m_EDA_Config;

    InitValues( );

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


/************************************************************************/
void DIALOG_PRINT_USING_PRINTER::InitValues( )
/************************************************************************/
{
    SetFocus();
    int      layer_max = NB_LAYERS;
    wxString msg;

    layer_max = 32;

    /* Create layer list */
    int mask = 1, ii;
    for( ii = 0; ii < layer_max; ii++, mask <<= 1 )
    {
        msg = _( "Layer" );
        msg << wxT( " " ) << ii + 1;
        m_BoxSelectLayer[ii] = new wxCheckBox( this, -1, msg );

        if( mask & s_SelectedLayers )
            m_BoxSelectLayer[ii]->SetValue( TRUE );
        if( ii < 16 )
            m_leftLayersBoxSizer->Add( m_BoxSelectLayer[ii],
                                         wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE );
        else
            m_rightLayersBoxSizer->Add( m_BoxSelectLayer[ii],
                                            wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE );
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
        for( int layer = 0;  layer<layer_max;  ++layer )
        {
            wxString layerKey;
            bool     option;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            option = false;
            if( m_Config->Read( layerKey, &option ) )
            {
                m_BoxSelectLayer[layer]->SetValue( option );
                if( option )
                    s_SelectedLayers |= 1 << layer;
            }
        }
    }

    // Disable checkboxes if the corresponding layer is not enabled
    BOARD* board = ((WinEDA_BasePcbFrame*)m_Parent)->GetBoard();
    for( int layer = 0; layer<NB_LAYERS; layer++, mask <<= 1 )
    {
       if( ! board->IsLayerEnabled( layer ) )
        {
            m_BoxSelectLayer[layer]->Enable( false );
            m_BoxSelectLayer[layer]->SetValue( false );
        }
    }

    m_ScaleOption->SetSelection( scale_idx );
    m_Print_Mirror->SetValue(s_Parameters.m_PrintMirror);


    if( s_Parameters.m_Print_Black_and_White )
        m_ModeColorOption->SetSelection( 1 );
    else
        m_ModeColorOption->SetSelection( 0 );

    s_Parameters.m_PenMinSize = 0;

    // Create scale adjust option
    msg.Printf( wxT( "%f" ), s_Parameters.m_XScaleAdjust );
    m_FineAdjustXscaleOpt->SetValue( msg );
    msg.Printf( wxT( "%f" ), s_Parameters.m_YScaleAdjust );
    m_FineAdjustYscaleOpt->SetValue( msg );
}

/*************************************************/
bool DIALOG_PRINT_USING_PRINTER::Show( bool show )
/*************************************************/
{
    bool ret;

    if( show )
    {
        if( s_LastPos.x != -1 )
        {
            SetSize( s_LastPos.x, s_LastPos.y, s_LastSize.x, s_LastSize.y, 0 );
        }
        ret = DIALOG_PRINT_USING_PRINTER_base::Show( show );
    }
    else
    {
        // Save the dialog's position before hiding
        s_LastPos  = GetPosition();
        s_LastSize = GetSize();

        ret = DIALOG_PRINT_USING_PRINTER_base::Show( show );
    }

    return ret;
}

/**************************************************************/
int DIALOG_PRINT_USING_PRINTER::SetLayerMaskFromListSelection()
/**************************************************************/
{
    int page_count;
    s_Parameters.m_PrintMaskLayer = 0;
    int ii;
    for( ii = 0, page_count = 0; ii < LAYER_COUNT; ii++ )
    {
        if( m_BoxSelectLayer[ii]->IsChecked() )
        {
            page_count++;
            s_Parameters.m_PrintMaskLayer |= 1 << ii;
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
        for( int layer = 0;  layer < LAYER_COUNT;  ++layer )
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

    if( m_PagesOption )
        s_Parameters.m_OptionPrintPage = m_PagesOption->GetSelection() != 0;


    SetLayerMaskFromListSelection();

    int idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale =  s_ScaleList[idx];
    g_pcb_plot_options.Scale =  s_Parameters.m_PrintScale;

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
    g_pcb_plot_options.ScaleAdjX = s_Parameters.m_XScaleAdjust;
    g_pcb_plot_options.ScaleAdjX = s_Parameters.m_YScaleAdjust;
}

/**********************************************************/
void DIALOG_PRINT_USING_PRINTER::OnPrintSetup( wxCommandEvent& event )
/**********************************************************/

/* Open a dialog box for printer setup (printer options, page size ...)
 */
{
    wxPrintDialogData printDialogData( *g_PrintData );

    if( printDialogData.Ok() )
    {
        wxPrintDialog printerDialog( this, &printDialogData );

        printerDialog.ShowModal();

        *g_PrintData = printerDialog.GetPrintDialogData().GetPrintData();
    }
    else
        DisplayError( this, _( "Printer Problem!" ) );
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
        new wxPrintPreview( new BOARD_PRINTOUT_CONTROLER( s_Parameters, m_Parent, title ),
                            new BOARD_PRINTOUT_CONTROLER( s_Parameters, m_Parent, title ),
                            g_PrintData );

    if( preview == NULL )
    {
        DisplayError( this, wxT( "OnPrintPreview() problem" ) );
        return;
    }

    SetLayerMaskFromListSelection();

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in pcbnew:
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
    frame->Show( TRUE );
}


/***************************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnPrintButtonClick( wxCommandEvent& event )
/***************************************************************************/

/* Called on activate Print button
 */
{
    SetPrintParameters( );

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in pcbnew:
    if( s_Parameters.m_PrintMaskLayer == 0 )
    {
        DisplayError( this, _( "No layer selected" ) );
        return;
    }

    wxPrintDialogData printDialogData( *g_PrintData );

    wxPrinter         printer( &printDialogData );

    wxString          title = _( "Print" );
    BOARD_PRINTOUT_CONTROLER      printout( s_Parameters, m_Parent, title );

#if !defined(__WINDOWS__) && !wxCHECK_VERSION(2,9,0)
    wxDC*             dc = printout.GetDC();
    ( (wxPostScriptDC*) dc )->SetResolution( 600 );  // Postscript DC resolution is 600 ppi
#endif

    if( !printer.Print( this, &printout, TRUE ) )
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

