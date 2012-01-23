/****************************************/
/* File: dialog_print_using_printer.cpp */
/****************************************/

// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>

#include <printout_controler.h>
#include <pcbnew.h>
#include <pcbplot.h>

#include <class_board.h>

#include <dialog_print_using_printer_base.h>


#define WIDTH_MAX_VALUE           1000
#define WIDTH_MIN_VALUE           1


extern int g_DrawDefaultLineThickness;

// Local variables
static long   s_SelectedLayers;
static double s_ScaleList[] =
{ 0, 0.5, 0.7, 0.999, 1.0, 1.4, 2.0, 3.0, 4.0 };

// Define min et max reasonnable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

// static print data and page setup data, to remember settings during the session
static wxPrintData* s_PrintData;
static wxPageSetupDialogData* s_pageSetupData = (wxPageSetupDialogData*) NULL;

static PRINT_PARAMETERS  s_Parameters;


/* Dialog to print schematic. Class derived from DIALOG_PRINT_USING_PRINTER_base
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_base
{
private:
    PCB_EDIT_FRAME* m_Parent;
    wxConfig*       m_Config;
    wxCheckBox*     m_BoxSelectLayer[32];
    static bool     m_ExcludeEdgeLayer;
    static wxPoint  s_LastPos;
    static wxSize   s_LastSize;

public:
    DIALOG_PRINT_USING_PRINTER( PCB_EDIT_FRAME* parent );
    ~DIALOG_PRINT_USING_PRINTER() {};

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnPageSetup( wxCommandEvent& event );
    void OnPrintPreview( wxCommandEvent& event );
    void OnPrintButtonClick( wxCommandEvent& event );
    void OnScaleSelectionClick( wxCommandEvent& event );

    void OnButtonCancelClick( wxCommandEvent& event ) { Close(); }
    void SetPrintParameters( );
    void SetPenWidth();
    void InitValues( );

    bool Show( bool show );     // overload stock function

public:
    bool IsMirrored() { return m_Print_Mirror->IsChecked(); }
    bool ExcludeEdges() { return m_Exclude_Edges_Pcb->IsChecked(); }
    bool PrintUsingSinglePage() { return m_PagesOption->GetSelection(); }
    int SetLayerMaskFromListSelection();
};

bool DIALOG_PRINT_USING_PRINTER::m_ExcludeEdgeLayer;
// We want our dialog to remember its previous screen position
wxPoint DIALOG_PRINT_USING_PRINTER::s_LastPos( -1, -1 );
wxSize  DIALOG_PRINT_USING_PRINTER::s_LastSize;




/*******************************************************/
void PCB_EDIT_FRAME::ToPrinter( wxCommandEvent& event )
/*******************************************************/

/* Virtual function:
 * Display the print dialog
 */
{
    if( s_PrintData == NULL )  // First print
    {
        s_PrintData = new wxPrintData();

        if( !s_PrintData->Ok() )
        {
            DisplayError( this, _( "Error Init Printer info" ) );
        }
        s_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGHT;
    }

    s_PrintData->SetOrientation( GetPageSettings().IsPortrait() ? wxPORTRAIT : wxLANDSCAPE );

    DIALOG_PRINT_USING_PRINTER* frame = new DIALOG_PRINT_USING_PRINTER( this );

    frame->ShowModal(); frame->Destroy();
}


/*************************************************************************************/
DIALOG_PRINT_USING_PRINTER::DIALOG_PRINT_USING_PRINTER( PCB_EDIT_FRAME* parent ) :
    DIALOG_PRINT_USING_PRINTER_base( parent )
/*************************************************************************************/
{
    m_Parent = parent;
    m_Config = wxGetApp().GetSettings();

    InitValues( );

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }

    Center();
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
    int      layer_max = NB_LAYERS;
    wxString msg;
    BOARD*   board = m_Parent->GetBoard();
    if( s_pageSetupData == NULL )
    {
        s_pageSetupData = new wxPageSetupDialogData;
        // Set initial page margins.
        // Margins are already set in Pcbnew, so we cans use 0
        s_pageSetupData->SetMarginTopLeft(wxPoint(0, 0));
        s_pageSetupData->SetMarginBottomRight(wxPoint(0, 0));
    }

    s_Parameters.m_PageSetupData = s_pageSetupData;

     // Create layer list.
    int      layer;
    wxString layerKey;
    for( layer = 0; layer < NB_LAYERS; ++layer )
    {
        if( !board->IsLayerEnabled( layer ) )
            m_BoxSelectLayer[layer] = NULL;
        else
        m_BoxSelectLayer[layer] =
            new wxCheckBox( this, -1, board->GetLayerName( layer ) );
    }

    // Add wxCheckBoxes in layers lists dialog
    //  List layers in same order than in setup layers dialog
    // (Front or Top to Back or Bottom)
    DECLARE_LAYERS_ORDER_LIST(layersOrder);
    for( int layer_idx = 0; layer_idx < NB_LAYERS; ++layer_idx )
    {
        layer = layersOrder[layer_idx];

        wxASSERT(layer < NB_LAYERS);

        if( m_BoxSelectLayer[layer] == NULL )
            continue;

        if( layer < NB_COPPER_LAYERS )
            m_CopperLayersBoxSizer->Add( m_BoxSelectLayer[layer],
                                     0, wxGROW | wxALL, 1 );
        else
            m_TechnicalLayersBoxSizer->Add( m_BoxSelectLayer[layer],
                                     0, wxGROW | wxALL, 1 );


        layerKey.Printf( OPTKEY_LAYERBASE, layer );
        bool option;
        if( m_Config->Read( layerKey, &option ) )
            m_BoxSelectLayer[layer]->SetValue( option );
        else
        {
            long mask = 1 << layer;
            if( mask & s_SelectedLayers )
                m_BoxSelectLayer[layer]->SetValue( true );
        }
    }


    // Option for excluding contents of "Edges Pcb" layer
    m_Exclude_Edges_Pcb->Show( true );

    // Read the scale adjust option
    int scale_idx = 4; // default selected scale = ScaleList[4] = 1.000

    if( m_Config )
    {
        m_Config->Read( OPTKEY_PRINT_X_FINESCALE_ADJ, &s_Parameters.m_XScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_Y_FINESCALE_ADJ, &s_Parameters.m_YScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_SCALE, &scale_idx );
        m_Config->Read( OPTKEY_PRINT_PAGE_FRAME, &s_Parameters.m_Print_Sheet_Ref, 1);
        m_Config->Read( OPTKEY_PRINT_MONOCHROME_MODE, &s_Parameters.m_Print_Black_and_White, 1);
        int tmp;
        m_Config->Read( OPTKEY_PRINT_PADS_DRILL,  &tmp, PRINT_PARAMETERS::SMALL_DRILL_SHAPE );
        s_Parameters.m_DrillShapeOpt = (PRINT_PARAMETERS::DrillShapeOptT) tmp;

        // Test for a reasonnable scale value. Set to 1 if problem
        if( s_Parameters.m_XScaleAdjust < MIN_SCALE ||
            s_Parameters.m_YScaleAdjust < MIN_SCALE ||
            s_Parameters.m_XScaleAdjust > MAX_SCALE ||
            s_Parameters.m_YScaleAdjust > MAX_SCALE )
            s_Parameters.m_XScaleAdjust = s_Parameters.m_YScaleAdjust = 1.0;

        s_SelectedLayers = 0;
        for( int layer = 0;  layer<layer_max;  ++layer )
        {
            if( m_BoxSelectLayer[layer] == NULL )
                continue;

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

    m_ScaleOption->SetSelection( scale_idx );
    scale_idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale =  s_ScaleList[scale_idx];
    m_Print_Mirror->SetValue(s_Parameters.m_PrintMirror);
    m_Exclude_Edges_Pcb->SetValue(m_ExcludeEdgeLayer);
    m_Print_Sheet_Ref->SetValue( s_Parameters.m_Print_Sheet_Ref );

    // Options to plot pads and vias holes
    m_Drill_Shape_Opt->SetSelection( s_Parameters.m_DrillShapeOpt );

    if( s_Parameters.m_Print_Black_and_White )
        m_ModeColorOption->SetSelection( 1 );
    else
        m_ModeColorOption->SetSelection( 0 );

    s_Parameters.m_PenDefaultSize = g_DrawDefaultLineThickness;
    AddUnitSymbol( *m_TextPenWidth, g_UserUnit );
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UserUnit, s_Parameters.m_PenDefaultSize, m_Parent->GetInternalUnits() ) );


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
    int layers_count = NB_LAYERS;

    s_Parameters.m_PrintMaskLayer = 0;
    int ii;
    for( ii = 0, page_count = 0; ii < layers_count; ii++ )
    {
        if( m_BoxSelectLayer[ii] == NULL )
            continue;
        if( m_BoxSelectLayer[ii]->IsChecked() )
        {
            page_count++;
            s_Parameters.m_PrintMaskLayer |= 1 << ii;
        }
    }

    // In Pcbnew force the EDGE layer to be printed or not with the other layers
    m_ExcludeEdgeLayer = m_Exclude_Edges_Pcb->IsChecked();
    if( m_ExcludeEdgeLayer )
        s_Parameters.m_Flags = 0;
    else
        s_Parameters.m_Flags = 1;

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
        m_Config->Write( OPTKEY_PRINT_PADS_DRILL, (long) s_Parameters.m_DrillShapeOpt );
        wxString layerKey;
        for( int layer = 0; layer < NB_LAYERS;  ++layer )
        {
            if( m_BoxSelectLayer[layer] == NULL )
                continue;
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
    s_Parameters.m_Print_Sheet_Ref = m_Print_Sheet_Ref->GetValue();
    s_Parameters.m_Print_Black_and_White =
        m_ModeColorOption->GetSelection() != 0;

    s_Parameters.m_DrillShapeOpt =
        (PRINT_PARAMETERS::DrillShapeOptT) m_Drill_Shape_Opt->GetSelection();

    if( m_PagesOption )
        s_Parameters.m_OptionPrintPage = m_PagesOption->GetSelection() != 0;


    SetLayerMaskFromListSelection();

    int idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale =  s_ScaleList[idx];
    g_PcbPlotOptions.m_PlotScale =  s_Parameters.m_PrintScale;

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
    g_PcbPlotOptions.m_FineScaleAdjustX = s_Parameters.m_XScaleAdjust;
    g_PcbPlotOptions.m_FineScaleAdjustY = s_Parameters.m_YScaleAdjust;
    SetPenWidth();
}


/**********************************************/
void DIALOG_PRINT_USING_PRINTER::SetPenWidth()
/***********************************************/

/* Get the new pen width value, and verify min et max value
 * NOTE: s_Parameters.m_PenDefaultSize is in internal units
 */
{
    s_Parameters.m_PenDefaultSize = ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->GetInternalUnits() );

    if( s_Parameters.m_PenDefaultSize > WIDTH_MAX_VALUE )
    {
        s_Parameters.m_PenDefaultSize = WIDTH_MAX_VALUE;
    }

    if( s_Parameters.m_PenDefaultSize < WIDTH_MIN_VALUE )
    {
        s_Parameters.m_PenDefaultSize = WIDTH_MIN_VALUE;
    }

    g_DrawDefaultLineThickness = s_Parameters.m_PenDefaultSize;

    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UserUnit, s_Parameters.m_PenDefaultSize, m_Parent->GetInternalUnits() ) );
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
    *s_pageSetupData = *s_PrintData;

    wxPageSetupDialog pageSetupDialog(this, s_pageSetupData);
    pageSetupDialog.ShowModal();

    (*s_PrintData) = pageSetupDialog.GetPageSetupDialogData().GetPrintData();
    (*s_pageSetupData) = pageSetupDialog.GetPageSetupDialogData();
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
                            s_PrintData );

    if( preview == NULL )
    {
        DisplayError( this, wxT( "OnPrintPreview() problem" ) );
        return;
    }

    SetLayerMaskFromListSelection();

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

    wxPrintDialogData printDialogData( *s_PrintData );

    wxPrinter         printer( &printDialogData );

    wxString          title = _( "Print" );
    BOARD_PRINTOUT_CONTROLER      printout( s_Parameters, m_Parent, title );

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
        *s_PrintData = printer.GetPrintDialogData().GetPrintData();
    }
}

