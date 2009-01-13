/*********************/
/* File: wxprint.cpp */
/*********************/

// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1

#include <ctype.h>
#include "wx/metafile.h"
#include "wx/print.h"
#include "wx/printdlg.h"
#include "wx/dcmirror.h"

#if wxTEST_POSTSCRIPT_IN_MSW
#include "wx/generic/printps.h"
#include "wx/generic/prntdlgg.h"
#endif

#include "fctsys.h"

#include "common.h"

#ifdef EESCHEMA
#include "program.h"
#include "general.h"
#endif

#ifdef PCBNEW
#include "pcbnew.h"

#include "protos.h"

// For pcbnew:
#include "pcbplot.h"
#endif

#define DEFAULT_ORIENTATION_PAPER wxLANDSCAPE   // other option is wxPORTRAIT
#ifdef EESCHEMA
#define WIDTH_MAX_VALUE 100
#else
#define WIDTH_MAX_VALUE 1000
#endif
#define WIDTH_MIN_VALUE 1

#ifdef PCBNEW
static long   s_SelectedLayers;
static double s_ScaleList[] =
{ 0, 0.5, 0.7, 0.999, 1.0, 1.4, 2.0, 3.0, 4.0 };
#endif


// static print data and page setup data, to remember settings during the session
static wxPrintData* g_PrintData;

// Variables locales
static int          s_PrintMaskLayer;
static int          s_OptionPrintPage = 0;
static int          s_Print_Black_and_White = TRUE;
static int          s_Scale_Select = 3; // default selected scale = ScaleList[3] = 1
static bool         s_PrintMirror;
static bool         s_Print_Sheet_Ref = TRUE;

#include "dialog_print.cpp"

/***************************/
/* Gestion de l'impression */
/***************************/

class EDA_Printout : public wxPrintout
{
public:
    bool m_Print_Sheet_Ref;

public:
    WinEDA_DrawFrame*  m_Parent;
    WinEDA_PrintFrame* m_PrintFrame;

    EDA_Printout( WinEDA_PrintFrame* print_frame,
                  WinEDA_DrawFrame*  parent,
                  const wxString&    title,
                  bool               print_ref ) :
        wxPrintout( title )
    {
        m_PrintFrame = print_frame;
        m_Parent = parent;
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

/* Prepare les structures de donn�es de gestion de l'impression
 * et affiche la fenetre de dialogue de gestion de l'impression des feuilles
 */
{
    wxPoint pos = GetPosition();
    bool    PrinterError = FALSE;

    // Stop the pending comand (if any...)
    if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
    {
        wxClientDC dc( DrawPanel );

        DrawPanel->PrepareDC( dc );
        DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
    }
    SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );

    if( g_PrintData == NULL )  // First print
    {
        g_PrintData = new wxPrintData();

        if( !g_PrintData->Ok() )
        {
            PrinterError = TRUE;
            DisplayError( this, _( "Error Init Printer info" ) );
        }
        g_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGHT;
        g_PrintData->SetOrientation( DEFAULT_ORIENTATION_PAPER );
    }


    pos.x += 10; pos.y += 10;
    WinEDA_PrintFrame* frame = new WinEDA_PrintFrame( this );

    frame->ShowModal(); frame->Destroy();
}


/*******************************************/
void WinEDA_PrintFrame::SetOthersDatas()
/*******************************************/
{
#ifndef PCBNEW
    m_Print_Mirror->Enable( false );
#endif

    m_FineAdjustXscaleOpt->SetToolTip( _( "Set X scale adjust for exact scale plotting" ) );
    m_FineAdjustYscaleOpt->SetToolTip( _( "Set Y scale adjust for exact scale plotting" ) );
    if( s_Print_Black_and_White )
        m_ColorOption->SetSelection( 1 );
#ifdef PCBNEW
    m_PagesOptionEeschema->Show( false );
    m_PagesOption = m_PagesOptionPcb;

    /* Create layer list */
    int mask = 1, ii;
    for( ii = 0; ii < NB_LAYERS; ii++, mask <<= 1 )
    {
        m_BoxSelecLayer[ii] = new wxCheckBox( this, -1,
#if defined (PCBNEW)
                                             ( (WinEDA_PcbFrame*) m_Parent )->GetBoard()->
                                             GetLayerName(
                                                 ii ) );
         #else
                                             ReturnLayerName( ii ) );
#endif

        if( mask & s_SelectedLayers )
            m_BoxSelecLayer[ii]->SetValue( TRUE );
        if( ii < 16 )
            m_CopperLayersBoxSizer->Add( m_BoxSelecLayer[ii],
                                         wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE );
        else
            m_TechLayersBoxSizer->Add( m_BoxSelecLayer[ii],
                                       wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE );
    }

    // Option for excluding contents of "Edges Pcb" layer
#ifndef GERBVIEW
    m_Exclude_Edges_Pcb->Show( true );
#endif

    // Read the scale adjust option
    if( m_Config )
    {
        m_Config->Read( OPTKEY_PRINT_X_FINESCALE_ADJ, &m_XScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_Y_FINESCALE_ADJ, &m_YScaleAdjust );
        m_Config->Read( OPTKEY_PRINT_SCALE, &s_Scale_Select );

        s_SelectedLayers = 0;
        for( int layer = 0;  layer<NB_LAYERS;  ++layer )
        {
            wxString layerKey;
            bool     option;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            option = false;
            if( m_Config->Read( layerKey, &option ) )
            {
                m_BoxSelecLayer[layer]->SetValue( option );
                if( option )
                    s_SelectedLayers |= 1 << layer;
            }
        }
    }

    m_ScaleOption->SetSelection( s_Scale_Select );

    // Create scale adjust option
    wxString msg;
    msg.Printf( wxT( "%lf" ), m_XScaleAdjust );
    m_FineAdjustXscaleOpt->SetValue( msg );
    msg.Printf( wxT( "%lf" ), m_YScaleAdjust );
    m_FineAdjustYscaleOpt->SetValue( msg );

#else
    m_PagesOption = m_PagesOptionEeschema;
    m_PagesOptionPcb->Show( false );
    m_ScaleOption->Show( false );
    m_FineAdjustXscaleTitle->Show( false );
    m_FineAdjustXscaleOpt->Show( false );
    m_FineAdjustYscaleTitle->Show( false );
    m_FineAdjustYscaleOpt->Show( false );
#endif
}


/**********************************************************/
int WinEDA_PrintFrame::SetLayerMaskFromListSelection()
/**********************************************************/
{
    int page_count;

    s_PrintMaskLayer = 0;
#ifdef PCBNEW
    int ii;
    for( ii = 0, page_count = 0; ii < NB_LAYERS; ii++ )
    {
        if( m_BoxSelecLayer[ii]->IsChecked() )
        {
            page_count++;
            s_PrintMaskLayer |= 1 << ii;
        }
    }

#else
    page_count = 1;
#endif
    return page_count;
}


/************************************************************/
void WinEDA_PrintFrame::SetColorOrBlack( wxCommandEvent& event )
/************************************************************/
{
    s_Print_Black_and_White = m_ColorOption->GetSelection();
}


/****************************************************/
void WinEDA_PrintFrame::OnClosePrintDialog()
/****************************************************/

/* called when WinEDA_PrintFrame is closed
 */
{
    wxConfig* Config = wxGetApp().m_EDA_Config;

    if( Config )
    {
        Config->Write( OPTKEY_PLOT_LINEWIDTH_VALUE, g_PlotLine_Width );
    }

    if( m_FineAdjustXscaleOpt )
        m_FineAdjustXscaleOpt->GetValue().ToDouble( &m_XScaleAdjust );
    if( m_FineAdjustYscaleOpt )
        m_FineAdjustYscaleOpt->GetValue().ToDouble( &m_YScaleAdjust );
    SetPenWidth();

#ifdef PCBNEW
    if( Config )
    {
        Config->Write( OPTKEY_PRINT_X_FINESCALE_ADJ, m_XScaleAdjust );
        Config->Write( OPTKEY_PRINT_Y_FINESCALE_ADJ, m_YScaleAdjust );
        Config->Write( OPTKEY_PRINT_SCALE, s_Scale_Select );
        wxString layerKey;
        for( int layer = 0;  layer<NB_LAYERS;  ++layer )
        {
            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            Config->Write( layerKey, m_BoxSelecLayer[layer]->IsChecked() );
        }
    }
#endif
    EndModal( 0 );
}


/************************************************/
wxString WinEDA_PrintFrame::BuildPrintTitle()
/************************************************/

/* return a valid filename to create a print file
 */
{
    wxString name, ext;

    wxFileName::SplitPath( m_Parent->GetBaseScreen()->m_FileName,
                           (wxString*) NULL, &name, &ext );
    name += wxT( "-" ) + ext;
    return name;
}


/******************************************************/
void WinEDA_PrintFrame::SetScale( wxCommandEvent& event )
/******************************************************/
{
#ifdef PCBNEW
    s_Scale_Select = m_ScaleOption->GetSelection();
    Scale_X = Scale_Y = s_ScaleList[s_Scale_Select];
    if( m_FineAdjustXscaleOpt )
        m_FineAdjustXscaleOpt->GetValue().ToDouble( &m_XScaleAdjust );
    if( m_FineAdjustYscaleOpt )
        m_FineAdjustYscaleOpt->GetValue().ToDouble( &m_YScaleAdjust );
    Scale_X *= m_XScaleAdjust;
    Scale_Y *= m_YScaleAdjust;
#endif
}


/****************************************/
void WinEDA_PrintFrame::SetPenWidth()
/****************************************/

/* Get the new pen width value, and verify min et max value
 * NOTE: g_PlotLine_Width is in internal units
 */
{
    g_PlotLine_Width = m_DialogPenWidth->GetValue();
    if( g_PlotLine_Width > WIDTH_MAX_VALUE )
    {
        g_PlotLine_Width = WIDTH_MAX_VALUE;
    }
    if( g_PlotLine_Width < WIDTH_MIN_VALUE )
    {
        g_PlotLine_Width = WIDTH_MIN_VALUE;
    }
    m_DialogPenWidth->SetValue( g_PlotLine_Width );
}


/**********************************************************/
void WinEDA_PrintFrame::OnPrintSetup( wxCommandEvent& event )
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
void WinEDA_PrintFrame::OnPrintPreview( wxCommandEvent& event )
/************************************************************/

/* Open and display a previewer frame for printing
 */
{
    wxSize  WSize;
    wxPoint WPos;
    int     x, y;
    bool    print_ref = TRUE;

    SetScale( event );
    SetPenWidth();

    if( m_PagesOption )
        s_OptionPrintPage = m_PagesOption->GetSelection();

    if( (m_Print_Sheet_Ref == NULL) || (m_Print_Sheet_Ref->GetValue() == FALSE) )
        print_ref = FALSE;

    // Pass two printout objects: for preview, and possible printing.
    wxString        title   = BuildPrintTitle();
    wxPrintPreview* preview =
        new wxPrintPreview( new EDA_Printout( this, m_Parent, title, print_ref ),
                            new EDA_Printout( this, m_Parent, title, print_ref ), g_PrintData );

    if( preview == NULL )
    {
        DisplayError( this, _( "There was a problem previewing" ) );
        return;
    }
#ifdef PCBNEW
    if( s_OptionPrintPage )
        SetLayerMaskFromListSelection();
#endif

    m_Parent->GetPosition( &x, &y );
    WPos.x = x + 4;
    WPos.y = y + 25;

    WSize    = m_Parent->GetSize();
    WSize.x -= 3;
    WSize.y += 6;

    wxPreviewFrame* frame = new wxPreviewFrame( preview, this,
                                                title, WPos, WSize );

    frame->Initialize();
    frame->Show( TRUE );
}


/**********************************************************/
void WinEDA_PrintFrame::EDA_PrintPage( wxCommandEvent& event )
/**********************************************************/

/* Called on activate "Print CURRENT" button
 */
{
    bool print_ref = TRUE;

    SetScale( event );

    s_OptionPrintPage = 0;
    if( m_PagesOption )
        s_OptionPrintPage = m_PagesOption->GetSelection();

    if( (m_Print_Sheet_Ref == NULL) || (m_Print_Sheet_Ref->GetValue() == FALSE) )
        print_ref = FALSE;

#ifdef PCBNEW
    if( s_OptionPrintPage )
        SetLayerMaskFromListSelection();
#endif

    SetPenWidth();

    wxPrintDialogData printDialogData( *g_PrintData );

    wxPrinter         printer( &printDialogData );

    wxString          title = BuildPrintTitle();
    EDA_Printout      printout( this, m_Parent, title, print_ref );

#ifndef __WINDOWS__
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


#ifdef EESCHEMA
    WinEDA_SchematicFrame* schframe     = (WinEDA_SchematicFrame*) m_Parent;
    SCH_SCREEN*            screen       = schframe->GetScreen();
    SCH_SCREEN*            oldscreen    = screen;
    DrawSheetPath*         oldsheetpath = schframe->GetSheet();


    DrawSheetPath          list;
    if( s_OptionPrintPage == 1 )
    {
        /* Print all pages, so when called, the page is not the current page.
         *  We must select it and setup references and others parameters
         *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
         *  is shared between many sheets
         */
        EDA_SheetList  SheetList( NULL );
        DrawSheetPath* sheetpath = SheetList.GetSheet( page - 1 );
        if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
        {
            schframe->m_CurrentSheet = &list;
            schframe->m_CurrentSheet->UpdateAllScreenReferences();
            schframe->SetSheetNumberAndCount();
            screen = schframe->m_CurrentSheet->LastScreen();
        }
        else
            screen = NULL;
    }

    if( screen == NULL )
        return FALSE;
    ActiveScreen = screen;
    DrawPage();
    ActiveScreen = oldscreen;
    schframe->m_CurrentSheet = oldsheetpath;
    schframe->m_CurrentSheet->UpdateAllScreenReferences();
    schframe->SetSheetNumberAndCount();
#endif


#ifdef PCBNEW
    if( (m_Parent->m_Ident == PCB_FRAME) || (m_Parent->m_Ident == GERBER_FRAME) )
    {
        m_PrintFrame->SetLayerMaskFromListSelection();
        if( s_OptionPrintPage == 0 )
        {
            // compute layer mask from page number
            int ii, jj, mask = 1;
            for( ii = 0, jj = 0; ii < NB_LAYERS; ii++ )
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

            if( ii == NB_LAYERS )
                return FALSE;
        }
    }
    DrawPage();
#endif

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

#ifdef EESCHEMA
    if( s_OptionPrintPage == 1 )
    {
        ii = g_RootSheet->CountSheets();
    }
#endif

#ifdef PCBNEW

    switch( s_OptionPrintPage )
    {
    case 0:
        ii = m_PrintFrame->SetLayerMaskFromListSelection();
        break;

    case 1:
        ii = 1;
        break;
    }

#endif

    *maxPage   = ii;
    *selPageTo = ii;
}


/**************************************/
bool EDA_Printout::HasPage( int pageNum )
/**************************************/
{
#ifdef EESCHEMA
    int PageCount;

    PageCount = g_RootSheet->CountSheets();
    if( PageCount >= pageNum )
        return TRUE;

    return FALSE;
#endif

#ifdef PCBNEW
    return TRUE;
#endif
}


/*************************************************************/
bool EDA_Printout::OnBeginDocument( int startPage, int endPage )
/*************************************************************/
{
    if( !wxPrintout::OnBeginDocument( startPage, endPage ) )
        return FALSE;

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
    int     DrawZoom = 1;
    wxDC*   dc = GetDC();

    s_PrintMirror = m_PrintFrame->m_Print_Mirror->GetValue();

    wxBusyCursor dummy;

    GetPageSizeMM( &PageSize_in_mm.x, &PageSize_in_mm.y );

    /* Save old draw scale and draw offset */
    tmp_startvisu = ActiveScreen->m_StartVisu;
    tmpzoom = ActiveScreen->GetZoom();
    old_org = ActiveScreen->m_DrawOrg;
    /* Change draw scale and offset to draw the whole page */
    ActiveScreen->SetZoom( DrawZoom );
    ActiveScreen->m_DrawOrg.x   = ActiveScreen->m_DrawOrg.y = 0;
    ActiveScreen->m_StartVisu.x = ActiveScreen->m_StartVisu.y = 0;

    SheetSize    = ActiveScreen->m_CurrentSheetDesc->m_Size;    // size in 1/1000 inch
    SheetSize.x *= m_Parent->m_InternalUnits / 1000;
    SheetSize.y *= m_Parent->m_InternalUnits / 1000;            // size in pixels

    // Get the size of the DC in pixels
    dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );

#ifdef PCBNEW
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
#else
    userscale = 1;
#endif

    // Calculate a suitable scaling factor
    scaleX = (double) SheetSize.x / PlotAreaSize.x;
    scaleY = (double) SheetSize.y / PlotAreaSize.y;
    scale  = wxMax( scaleX, scaleY ) / userscale; // Use x or y scaling factor, whichever fits on the DC

    // ajust the real draw scale
#ifdef PCBNEW
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
#else
    dc->SetUserScale( DrawZoom / scale, DrawZoom / scale );
#endif

#ifdef PCBNEW
    if( (s_ScaleList[s_Scale_Select] > 1.0)         //  scale > 1 -> Recadrage
       || (s_ScaleList[s_Scale_Select] == 0) )      //  fit in page
    {
        DrawOffset.x -= (int) ( (PlotAreaSize.x / 2) * scale );
        DrawOffset.y -= (int) ( (PlotAreaSize.y / 3) * scale );
    }
    DrawOffset.x += (int) ( (SheetSize.x / 2) * (m_PrintFrame->m_XScaleAdjust - 1.0) );
    DrawOffset.y += (int) ( (SheetSize.y / 2) * (m_PrintFrame->m_YScaleAdjust - 1.0) );
#endif

    ActiveScreen->m_DrawOrg = DrawOffset;

    GRResetPenAndBrush( dc );
    if( s_Print_Black_and_White )
        GRForceBlackPen( TRUE );


#ifdef EESCHEMA
    /* set Pen min width */
    double ftmp, xdcscale, ydcscale;

    // g_PlotLine_Width is in internal units ( 1/1000 inch), and must be converted in pixels
    ftmp  = (float) g_PlotLine_Width * 25.4 / EESCHEMA_INTERNAL_UNIT; // ftmp est en mm
    ftmp *= (float) PlotAreaSize.x / PageSize_in_mm.x;                  /* ftmp is in  pixels */

    /* because the pen size will be scaled by the dc scale, we modify the size
     * in order to keep the requested value */
    dc->GetUserScale( &xdcscale, &ydcscale );
    ftmp /= xdcscale;
    SetPenMinWidth( (int) round( ftmp ) );
#else
    SetPenMinWidth( 1 );  // min width = 1 pixel
#endif

    WinEDA_DrawPanel* panel = m_Parent->DrawPanel;
    EDA_Rect          tmp   = panel->m_ClipBox;

    panel->m_ClipBox.SetOrigin( wxPoint( 0, 0 ) );
    panel->m_ClipBox.SetSize( wxSize( 0x7FFFFF0, 0x7FFFFF0 ) );

    g_IsPrinting = TRUE;

#ifdef PCBNEW
    if( m_Print_Sheet_Ref )
        m_Parent->TraceWorkSheet( dc, ActiveScreen, 0 );

    if( userscale == 1.0 ) // Draw the Sheet refs at optimum scale, and board at 1.0 scale
    {
        dc->SetUserScale( accurate_Yscale, accurate_Yscale );
    }

    if( s_PrintMirror )
    {   // To plot mirror, we reverse the y axis, and modify the plot y origin
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
    if( !m_PrintFrame->m_Exclude_Edges_Pcb->GetValue() )
        s_PrintMaskLayer |= EDGE_LAYER;
#endif

    panel->PrintPage( dc, 0, s_PrintMaskLayer, s_PrintMirror );

#else
    panel->PrintPage( dc, m_Print_Sheet_Ref, s_PrintMaskLayer, s_PrintMirror );
#endif

    g_IsPrinting     = FALSE;
    panel->m_ClipBox = tmp;

    SetPenMinWidth( 1 );
    GRForceBlackPen( FALSE );

    ActiveScreen->m_StartVisu = tmp_startvisu;
    ActiveScreen->m_DrawOrg   = old_org;
    ActiveScreen->SetZoom( tmpzoom );
}
