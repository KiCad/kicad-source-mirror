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

#include <wx/dcps.h>

#include "dialog_print_using_printer_base.h"

#include "pcbnew.h"
#include "protos.h"
#include "pcbplot.h"

#define DEFAULT_ORIENTATION_PAPER wxLANDSCAPE   // other option is wxPORTRAIT
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
static int          s_PrintPenMinWidth = 50;    // A reasonnable value to draw items that do dot have aspecifed line width
static int          s_PrintMaskLayer;
static int          s_OptionPrintPage = 0;
static int          s_Print_Black_and_White = TRUE;
static int          s_Scale_Select = 3; // default selected scale = ScaleList[3] = 1
static bool         s_PrintMirror;
static bool         s_Print_Sheet_Ref = TRUE;


/* Dialog to print schematic. Class derived from DIALOG_PRINT_USING_PRINTER_base
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_base
{
private:
    WinEDA_DrawFrame* m_Parent;
    wxConfig*         m_Config;
    wxCheckBox*       m_BoxSelectLayer[32];
public:
    double            m_XScaleAdjust, m_YScaleAdjust;

public:
    DIALOG_PRINT_USING_PRINTER( WinEDA_DrawFrame* parent );
    ~DIALOG_PRINT_USING_PRINTER() {};

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnInitDialog( wxInitDialogEvent& event );
    void OnPrintSetup( wxCommandEvent& event );
    void OnPrintPreview( wxCommandEvent& event );
    void OnPrintButtonClick( wxCommandEvent& event );

    void OnButtonCancelClick( wxCommandEvent& event ) { Close(); }
    void SetScale( wxCommandEvent& event );
    void SetPenWidth();

public:
    bool IsMirrored() { return m_Print_Mirror->IsChecked(); }
    bool ExcludeEdges() { return m_Exclude_Edges_Pcb->IsChecked(); }
    bool PrintUsingSinglePage() { return m_PagesOption->GetSelection(); }
    int SetLayerMaskFromListSelection();
};

/***************************/
/* Gestion de l'impression */
/***************************/

class EDA_Printout : public wxPrintout
{
public:
    bool m_Print_Sheet_Ref;

public:
    WinEDA_DrawFrame*           m_Parent;
    DIALOG_PRINT_USING_PRINTER* m_PrintFrame;

    EDA_Printout( DIALOG_PRINT_USING_PRINTER* print_frame,
                  WinEDA_DrawFrame*           parent,
                  const wxString&             title,
                  bool                        print_ref ) :
        wxPrintout( title )
    {
        m_PrintFrame      = print_frame;
        m_Parent          = parent;
        s_PrintMaskLayer  = 0xFFFFFFFF;
        m_Print_Sheet_Ref = print_ref;
    }


    bool OnPrintPage( int page );
    bool HasPage( int page );
    bool OnBeginDocument( int startPage, int endPage );
    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo );

    void DrawPage();
};

/*******************************************************/
void WinEDA_DrawFrame::ToPrinter( wxCommandEvent& event )
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
DIALOG_PRINT_USING_PRINTER::DIALOG_PRINT_USING_PRINTER( WinEDA_DrawFrame* parent ) :
    DIALOG_PRINT_USING_PRINTER_base( parent )
/*************************************************************************************/
{
    m_Parent = parent;
    m_Config = wxGetApp().m_EDA_Config;
}


/************************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnInitDialog( wxInitDialogEvent& event )
/************************************************************************/
{
    SetFocus();
    int      layer_max = NB_LAYERS;
    wxString msg;

    #ifdef GERBVIEW
    layer_max = 32;
    m_Exclude_Edges_Pcb->SetValue( true );    // no meaning in gerbview
    m_Exclude_Edges_Pcb->Show( false );
    msg = _( "Layers:" );

    // Set wxRadioBox title to "Layers:" for copper layers and thechincal layers
    // Because in Gerbview , al layers are only graphic layers (layer id has no meaning)
    m_CopperLayersBoxSizer->GetStaticBox()->SetLabel( msg );
    m_TechnicalLayersBoxSizer->GetStaticBox()->SetLabel( msg );
    #endif

    /* Create layer list */
    int mask = 1, ii;
    for( ii = 0; ii < layer_max; ii++, mask <<= 1 )
    {
#ifdef GERBVIEW
        msg = _( "Layer" );
        msg << wxT( " " ) << ii + 1;
#else
        msg = ( (WinEDA_PcbFrame*) m_Parent )->GetBoard()->GetLayerName( ii );
#endif
        m_BoxSelectLayer[ii] = new wxCheckBox( this, -1, msg );

        if( mask & s_SelectedLayers )
            m_BoxSelectLayer[ii]->SetValue( TRUE );
        if( ii < 16 )
            m_CopperLayersBoxSizer->Add( m_BoxSelectLayer[ii],
                                         wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE );
        else
            m_TechnicalLayersBoxSizer->Add( m_BoxSelectLayer[ii],
                                            wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE );
    }

    // Option for excluding contents of "Edges Pcb" layer
#ifndef GERBVIEW
    m_Exclude_Edges_Pcb->Show( true );
#endif

    m_XScaleAdjust = m_YScaleAdjust = 1.0;      // Default value for scale
    // Read the scale adjust option
    if( m_Config )
    {
        m_Config->Read( OPTKEY_PLOT_LINEWIDTH_VALUE, &s_PrintPenMinWidth );
        m_Config->Read( OPTKEY_PRINT_X_FINESCALE_ADJ, &m_XScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_Y_FINESCALE_ADJ, &m_YScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_SCALE, &s_Scale_Select );

        // Test for a reasonnable scale value. Set to 1 if problem
        if( m_XScaleAdjust < MIN_SCALE || m_YScaleAdjust < MIN_SCALE || m_XScaleAdjust >
            MAX_SCALE || m_YScaleAdjust > MAX_SCALE )
            m_XScaleAdjust = m_YScaleAdjust = 1.0;

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

    m_ScaleOption->SetSelection( s_Scale_Select );

    if( s_Print_Black_and_White )
        m_ModeColorOption->SetSelection( 1 );

    AddUnitSymbol( *m_TextPenWidth, g_UnitMetric );
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UnitMetric, s_PrintPenMinWidth, m_Parent->m_InternalUnits ) );


    // Create scale adjust option
    msg.Printf( wxT( "%f" ), m_XScaleAdjust );
    m_FineAdjustXscaleOpt->SetValue( msg );
    msg.Printf( wxT( "%f" ), m_YScaleAdjust );
    m_FineAdjustYscaleOpt->SetValue( msg );

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


/**************************************************************/
int DIALOG_PRINT_USING_PRINTER::SetLayerMaskFromListSelection()
/**************************************************************/
{
    int page_count;
    int layers_count = NB_LAYERS;

    if( m_Parent->m_Ident == GERBER_FRAME )
        layers_count = 32;

    s_PrintMaskLayer = 0;
    int ii;
    for( ii = 0, page_count = 0; ii < layers_count; ii++ )
    {
        if( m_BoxSelectLayer[ii]->IsChecked() )
        {
            page_count++;
            s_PrintMaskLayer |= 1 << ii;
        }
    }

    return page_count;
}


/********************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnCloseWindow( wxCloseEvent& event )
/********************************************************************/
{
    s_Print_Black_and_White = m_ModeColorOption->GetSelection();
    m_Print_Sheet_Ref->SetValue( s_Print_Sheet_Ref );
    SetPenWidth();

    if( m_Config )
    {
        m_Config->Write( OPTKEY_PLOT_LINEWIDTH_VALUE, s_PrintPenMinWidth );
    }

    if( m_FineAdjustXscaleOpt )
        m_FineAdjustXscaleOpt->GetValue().ToDouble( &m_XScaleAdjust );
    if( m_FineAdjustYscaleOpt )
        m_FineAdjustYscaleOpt->GetValue().ToDouble( &m_YScaleAdjust );
    SetPenWidth();

    if( m_Config )
    {
        m_Config->Write( OPTKEY_PRINT_X_FINESCALE_ADJ, m_XScaleAdjust );
        m_Config->Write( OPTKEY_PRINT_Y_FINESCALE_ADJ, m_YScaleAdjust );
        m_Config->Write( OPTKEY_PRINT_SCALE, s_Scale_Select );
        wxString layerKey;
        int      layers_count = NB_LAYERS;
        if( m_Parent->m_Ident == GERBER_FRAME )
            layers_count = 32;
        for( int layer = 0;  layer<layers_count;  ++layer )
        {
            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            m_Config->Write( layerKey, m_BoxSelectLayer[layer]->IsChecked() );
        }
    }
    EndModal( 0 );
}


/******************************************************************/
void DIALOG_PRINT_USING_PRINTER::SetScale( wxCommandEvent& event )
/******************************************************************/
{
    s_Scale_Select = m_ScaleOption->GetSelection();
    g_pcb_plot_options.Scale =  s_ScaleList[s_Scale_Select];
    if( m_FineAdjustXscaleOpt )
        m_FineAdjustXscaleOpt->GetValue().ToDouble( &m_XScaleAdjust );
    if( m_FineAdjustYscaleOpt )
        m_FineAdjustYscaleOpt->GetValue().ToDouble( &m_YScaleAdjust );
    g_pcb_plot_options.ScaleAdjX = m_XScaleAdjust;
    g_pcb_plot_options.ScaleAdjX = m_YScaleAdjust;
}


/**********************************************/
void DIALOG_PRINT_USING_PRINTER::SetPenWidth()
/***********************************************/

/* Get the new pen width value, and verify min et max value
 * NOTE: s_PrintPenMinWidth is in internal units
 */
{
    s_PrintPenMinWidth = ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->m_InternalUnits );

    if( s_PrintPenMinWidth > WIDTH_MAX_VALUE )
    {
        s_PrintPenMinWidth = WIDTH_MAX_VALUE;
    }

    if( s_PrintPenMinWidth < WIDTH_MIN_VALUE )
    {
        s_PrintPenMinWidth = WIDTH_MIN_VALUE;
    }

    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UnitMetric, s_PrintPenMinWidth, m_Parent->m_InternalUnits ) );
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
    SetScale( event );
    SetPenWidth();

    s_Print_Black_and_White = m_ModeColorOption->GetSelection();

    if( m_PagesOption )
        s_OptionPrintPage = m_PagesOption->GetSelection();

    s_Print_Sheet_Ref = m_Print_Sheet_Ref->GetValue();

    // Pass two printout objects: for preview, and possible printing.
    wxString        title   = _( "Print Preview" );
    wxPrintPreview* preview =
        new wxPrintPreview( new EDA_Printout( this, m_Parent, title, s_Print_Sheet_Ref ),
                            new EDA_Printout( this, m_Parent, title, s_Print_Sheet_Ref ),
                            g_PrintData );

    if( preview == NULL )
    {
        DisplayError( this, wxT( "OnPrintPreview() problem" ) );
        return;
    }

    SetLayerMaskFromListSelection();

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in pcbnew:
    if( s_PrintMaskLayer == 0 )
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
    SetScale( event );

    s_Print_Black_and_White = m_ModeColorOption->GetSelection();

    s_OptionPrintPage = 0;
    if( m_PagesOption )
        s_OptionPrintPage = m_PagesOption->GetSelection();

    s_Print_Sheet_Ref = m_Print_Sheet_Ref->GetValue();

    SetLayerMaskFromListSelection();

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in pcbnew:
    if( s_PrintMaskLayer == 0 )
    {
        DisplayError( this, _( "No layer selected" ) );
        return;
    }


    SetPenWidth();

    wxPrintDialogData printDialogData( *g_PrintData );

    wxPrinter         printer( &printDialogData );

    wxString          title = _( "Print" );
    EDA_Printout      printout( this, m_Parent, title, s_Print_Sheet_Ref );

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


/***************************************/
bool EDA_Printout::OnPrintPage( int page )
/***************************************/
{
    wxString msg;

    msg.Printf( _( "Print page %d" ), page );
    m_Parent->Affiche_Message( msg );

    int layers_count = NB_LAYERS;
    if( m_Parent->m_Ident == GERBER_FRAME )
        layers_count = 32;

    m_PrintFrame->SetLayerMaskFromListSelection();
    if( s_OptionPrintPage == 0 )    // compute layer mask from page number
    {
        int ii, jj, mask = 1;
        for( ii = 0, jj = 0; ii < layers_count; ii++ )
        {
            if( s_PrintMaskLayer & mask )
                jj++;
            if( jj == page )
            {
                s_PrintMaskLayer = mask;
                break;
            }
            mask <<= 1;
        }
    }

    if( s_PrintMaskLayer == 0 )
        return false;

    DrawPage();

    return TRUE;
}


/*********************************************************/
void EDA_Printout::GetPageInfo( int* minPage, int* maxPage,
                                int* selPageFrom, int* selPageTo )
/*********************************************************/
{
    int ii = 1;

    *minPage     = 1;
    *selPageFrom = 1;

    switch( s_OptionPrintPage )
    {
    case 0:
        ii = m_PrintFrame->SetLayerMaskFromListSelection();
        break;

    case 1:
        ii = 1;
        break;
    }

    *maxPage   = ii;
    *selPageTo = ii;
}


/**************************************/
bool EDA_Printout::HasPage( int pageNum )
/**************************************/
{
    return TRUE;
}


/*************************************************************/
bool EDA_Printout::OnBeginDocument( int startPage, int endPage )
/*************************************************************/
{
    if( !wxPrintout::OnBeginDocument( startPage, endPage ) )
        return false;

    return TRUE;
}


/********************************/
void EDA_Printout::DrawPage()
/********************************/

/*
 * This is the real print function: print the active screen
 */
{
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxSize  PageSize_in_mm;
    wxSize  SheetSize;      // Page size in internal units
    wxSize  PlotAreaSize;   // plot area size in pixels
    double  scaleX, scaleY, scale;
    wxPoint old_org;
    wxPoint DrawOffset; // Offset de trace
    double  userscale;
    double  DrawZoom = 1;
    wxDC*   dc = GetDC();

    s_PrintMirror = m_PrintFrame->IsMirrored();

    wxBusyCursor dummy;

    GetPageSizeMM( &PageSize_in_mm.x, &PageSize_in_mm.y );

    /* Save old draw scale and draw offset */
    tmp_startvisu = ActiveScreen->m_StartVisu;
    tmpzoom = ActiveScreen->GetZoom();
    old_org = ActiveScreen->m_DrawOrg;
    /* Change draw scale and offset to draw the whole page */
    ActiveScreen->SetScalingFactor( DrawZoom );
    ActiveScreen->m_DrawOrg.x   = ActiveScreen->m_DrawOrg.y = 0;
    ActiveScreen->m_StartVisu.x = ActiveScreen->m_StartVisu.y = 0;

    // Gerbview uses a very large sheet (called "World" in gerber language)
    // to print a sheet, uses A4 is better
    SheetSize = ActiveScreen->m_CurrentSheetDesc->m_Size;       // size in 1/1000 inch
    if( m_Parent->m_Ident == GERBER_FRAME )
    {
        SheetSize = g_Sheet_A4.m_Size;    // size in 1/1000 inch
    }
    SheetSize.x *= m_Parent->m_InternalUnits / 1000;
    SheetSize.y *= m_Parent->m_InternalUnits / 1000;            // size in pixels

    // Get the size of the DC in pixels
    dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );

    WinEDA_BasePcbFrame* pcbframe = (WinEDA_BasePcbFrame*) m_Parent;
    pcbframe->GetBoard()->ComputeBoundaryBox();
    /* Compute the PCB size in internal units*/
    userscale = s_ScaleList[s_Scale_Select];
    if( userscale == 0 )            //  fit in page
    {
        int extra_margin = 8000;    // Margin = 8000/2 units pcb = 0,4 inch
        SheetSize.x = pcbframe->GetBoard()->m_BoundaryBox.GetWidth() + extra_margin;
        SheetSize.y = pcbframe->GetBoard()->m_BoundaryBox.GetHeight() + extra_margin;
        userscale   = 0.99;
    }

    if( (s_ScaleList[s_Scale_Select] > 1.0)         //  scale > 1 -> Recadrage
       || (s_ScaleList[s_Scale_Select] == 0) )      //  fit in page
    {
        DrawOffset.x += pcbframe->GetBoard()->m_BoundaryBox.Centre().x;
        DrawOffset.y += pcbframe->GetBoard()->m_BoundaryBox.Centre().y;
    }

    // Calculate a suitable scaling factor
    scaleX = (double) SheetSize.x / PlotAreaSize.x;
    scaleY = (double) SheetSize.y / PlotAreaSize.y;
    scale  = wxMax( scaleX, scaleY ) / userscale; // Use x or y scaling factor, whichever fits on the DC

    if( m_PrintFrame->m_XScaleAdjust > MAX_SCALE || m_PrintFrame->m_YScaleAdjust > MAX_SCALE )
        DisplayInfoMessage( NULL, _( "Warning: Scale option set to a very large value" ) );

    // Test for a reasonnable scale value
    if( m_PrintFrame->m_XScaleAdjust < MIN_SCALE || m_PrintFrame->m_YScaleAdjust < MIN_SCALE )
        DisplayInfoMessage( NULL, _( "Warning: Scale option set to a very small value" ) );

    // ajust the real draw scale
    double accurate_Xscale, accurate_Yscale;
    dc->SetUserScale( DrawZoom / scale * m_PrintFrame->m_XScaleAdjust,
                      DrawZoom / scale * m_PrintFrame->m_YScaleAdjust );

    // Compute Accurate scale 1
    {
        int w, h;
        GetPPIPrinter( &w, &h );
        accurate_Xscale = ( (double) ( DrawZoom * w ) ) / PCB_INTERNAL_UNIT;
        accurate_Yscale = ( (double) ( DrawZoom * h ) ) / PCB_INTERNAL_UNIT;
        if( IsPreview() )  // Scale must take in account the DC size in Preview
        {
            // Get the size of the DC in pixels
            dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );
            GetPageSizePixels( &w, &h );
            accurate_Xscale *= PlotAreaSize.x; accurate_Xscale /= w;
            accurate_Yscale *= PlotAreaSize.y; accurate_Yscale /= h;
        }
        accurate_Xscale *= m_PrintFrame->m_XScaleAdjust;
        accurate_Yscale *= m_PrintFrame->m_YScaleAdjust;
    }

    if( (s_ScaleList[s_Scale_Select] > 1.0)         //  scale > 1 -> Recadrage
       || (s_ScaleList[s_Scale_Select] == 0) )      //  fit in page
    {
        DrawOffset.x -= (int) ( (PlotAreaSize.x / 2) * scale );
        DrawOffset.y -= (int) ( (PlotAreaSize.y / 3) * scale );
    }
    DrawOffset.x += (int) ( (SheetSize.x / 2) * (m_PrintFrame->m_XScaleAdjust - 1.0) );
    DrawOffset.y += (int) ( (SheetSize.y / 2) * (m_PrintFrame->m_YScaleAdjust - 1.0) );

    ActiveScreen->m_DrawOrg = DrawOffset;

    GRResetPenAndBrush( dc );
    if( s_Print_Black_and_White )
        GRForceBlackPen( TRUE );


    SetPenMinWidth( 1 );  // min width = 1 pixel

    WinEDA_DrawPanel* panel = m_Parent->DrawPanel;
    EDA_Rect          tmp   = panel->m_ClipBox;

    panel->m_ClipBox.SetOrigin( wxPoint( 0, 0 ) );
    panel->m_ClipBox.SetSize( wxSize( 0x7FFFFF0, 0x7FFFFF0 ) );

    m_Parent->GetBaseScreen()->m_IsPrinting = true;
    int bg_color = g_DrawBgColor;

    if( m_Print_Sheet_Ref )
        m_Parent->TraceWorkSheet( dc, ActiveScreen, 0 );

    if( userscale == 1.0 ) // Draw the Sheet refs at optimum scale, and board at 1.0 scale
    {
        dc->SetUserScale( accurate_Xscale, accurate_Yscale );
    }

    if( s_PrintMirror )
    {
        // To plot mirror, we reverse the y axis, and modify the plot y origin
        double sx, sy;

        dc->GetUserScale( &sx, &sy );
        dc->SetAxisOrientation( TRUE, TRUE );
        if( userscale < 1.0 )
            sy /= userscale;

        /* Plot offset y is moved by the y plot area size in order to have
         * the old draw area in the new draw area, because the draw origin has not moved
         * (this is the upper left corner) but the Y axis is reversed, therefore the plotting area
         * is the y coordinate values from  - PlotAreaSize.y to 0 */
        int ysize = (int) ( PlotAreaSize.y / sy );
        DrawOffset.y += ysize;

        /* in order to keep the board position in the sheet
         * (when user scale <= 1) the y offset in moved by the distance between
         * the middle of the page and the middle of the board
         * This is equivalent to put the mirror axis to the board centre
         * for scales > 1, the DrawOffset was already computed to have the board centre
         * to the middle of the page.
         */
        wxPoint pcb_centre = pcbframe->GetBoard()->m_BoundaryBox.Centre();
        if( userscale <= 1.0 )
            DrawOffset.y += pcb_centre.y - (ysize / 2);
        ActiveScreen->m_DrawOrg = DrawOffset;
        panel->m_ClipBox.SetOrigin( wxPoint( -0x7FFFFF, -0x7FFFFF ) );
    }

#ifndef GERBVIEW
    if( !m_PrintFrame->ExcludeEdges() )
        s_PrintMaskLayer |= EDGE_LAYER;
#endif

    g_DrawBgColor = WHITE;

    /* when printing in color mode, we use the graphic OR mode that gives the same look as the screen
    * But because the backgroud is white when printing, we must use a trick:
    * In order to plot on a white background in OR mode we must:
    * 1 - Plot all items in black, this creates a local black backgroud
    * 2 - Plot in OR mode on black "local" background
    */
    if( ! s_Print_Black_and_White )
    {
        GRForceBlackPen( true );
        panel->PrintPage( dc, 0, s_PrintMaskLayer, s_PrintMirror );
        GRForceBlackPen( false );
    }

     panel->PrintPage( dc, 0, s_PrintMaskLayer, s_PrintMirror );

    g_DrawBgColor    = bg_color;
    m_Parent->GetBaseScreen()->m_IsPrinting = false;
    panel->m_ClipBox = tmp;

    SetPenMinWidth( 1 );
    GRForceBlackPen( false );

    ActiveScreen->m_StartVisu = tmp_startvisu;
    ActiveScreen->m_DrawOrg   = old_org;
    ActiveScreen->SetZoom( tmpzoom );
}
