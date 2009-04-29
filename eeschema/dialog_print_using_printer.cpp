/****************************************/
/* File: dialog_print_using_printer.cpp */
/****************************************/

// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "general.h"

#include <wx/dcps.h>
#include "dialog_print_using_printer_base.h"


#define DEFAULT_ORIENTATION_PAPER wxLANDSCAPE   // other option is wxPORTRAIT
#define WIDTH_MAX_VALUE           100
#define WIDTH_MIN_VALUE           1

#define PRINTMODECOLOR_KEY  wxT("PrintModeColor")

// static print data and page setup data, to remember settings during the session
static wxPrintData* g_PrintData;

// Variables locales
static int          s_OptionPrintPage = 0;
static int          s_Print_Black_and_White = true;
static bool         s_Print_Frame_Ref = true;


/* Dialog to print schematic. Class derived from DIALOG_PRINT_USING_PRINTER_base
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_base
{
private:
    WinEDA_DrawFrame* m_Parent;
    wxConfig*         m_Config;

public:
    DIALOG_PRINT_USING_PRINTER( WinEDA_DrawFrame* parent );
    ~DIALOG_PRINT_USING_PRINTER() {};

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnInitDialog( wxInitDialogEvent& event );
    void OnPrintSetup( wxCommandEvent& event );
    void OnPrintPreview( wxCommandEvent& event );
    void OnPrintButtonClick( wxCommandEvent& event );
	void OnButtonCancelClick( wxCommandEvent& event ){ Close(); }
    void SetPenWidth();
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

    EDA_Printout( DIALOG_PRINT_USING_PRINTER* aPrint_frame,
                  WinEDA_DrawFrame*           aParent,
                  const wxString&             aTitle,
                  bool                        aPrint_ref ) :
        wxPrintout( aTitle )
    {
        m_PrintFrame      = aPrint_frame;
        m_Parent          = aParent;
        m_Print_Sheet_Ref = aPrint_ref;
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

/* Virtual function
 * Calls the print dialog for Eeschema
 */
{
    if( g_PrintData == NULL )   // First call. creates print handlers
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

    if( m_Config )
    {
        m_Config->Read( PRINTMODECOLOR_KEY, &s_Print_Black_and_White );
    }

    AddUnitSymbol(* m_TextPenWidth, g_UnitMetric );
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue(g_UnitMetric, g_PlotLine_Width, m_Parent->m_InternalUnits ) );
    m_Print_Sheet_Ref->SetValue( s_Print_Frame_Ref );

    m_ModeColorOption->SetSelection( s_Print_Black_and_White ? 1 : 0);
    m_Print_Sheet_Ref->SetValue(s_Print_Frame_Ref);

    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
}


/*********************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnCloseWindow( wxCloseEvent& event )
/*********************************************************************/
{
    s_Print_Black_and_White = m_ModeColorOption->GetSelection();
    s_Print_Frame_Ref = m_Print_Sheet_Ref->GetValue();
    SetPenWidth();

    if( m_Config )
    {
        m_Config->Write( PRINTMODECOLOR_KEY, s_Print_Black_and_White );
    }

    EndModal( 0 );
}


/****************************************/
void DIALOG_PRINT_USING_PRINTER::SetPenWidth()
/****************************************/

/* Get the new pen width value, and verify min et max value
 * NOTE: g_PlotLine_Width is in internal units
 */
{
    g_PlotLine_Width = ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->m_InternalUnits );

    if( g_PlotLine_Width > WIDTH_MAX_VALUE )
    {
        g_PlotLine_Width = WIDTH_MAX_VALUE;
    }

    if( g_PlotLine_Width < WIDTH_MIN_VALUE )
    {
        g_PlotLine_Width = WIDTH_MIN_VALUE;
    }

    m_DialogPenWidth->SetValue(
        ReturnStringFromValue(g_UnitMetric, g_PlotLine_Width, m_Parent->m_InternalUnits ) );
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


/***********************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnPrintPreview( wxCommandEvent& event )
/***********************************************************************/

/* Open and display a previewer frame for printing
 */
{
    wxSize  WSize;
    wxPoint WPos;
    bool    print_ref = m_Print_Sheet_Ref->GetValue();

    s_Print_Black_and_White = m_ModeColorOption->GetSelection();
    SetPenWidth();

    s_OptionPrintPage = m_PagesOption->GetSelection();

    // Pass two printout objects: for preview, and possible printing.
    wxString        title   = _("Preview");
    wxPrintPreview* preview =
        new wxPrintPreview( new EDA_Printout( this, m_Parent, title, print_ref ),
                            new EDA_Printout( this, m_Parent, title, print_ref ), g_PrintData );

    if( preview == NULL )
    {
        DisplayError( this, wxT( "OnPrintPreview() problem" ) );
        return;
    }

    WPos = m_Parent->GetPosition( );
    WSize    = m_Parent->GetSize();

    wxPreviewFrame* frame = new wxPreviewFrame( preview, this,
                                                title, WPos, WSize );

    frame->Initialize();
    frame->Show( true );
}


/**************************************************************************/
void DIALOG_PRINT_USING_PRINTER::OnPrintButtonClick( wxCommandEvent& event )
/**************************************************************************/
{
    bool print_ref = m_Print_Sheet_Ref->GetValue();

    s_Print_Black_and_White = m_ModeColorOption->GetSelection();

    s_OptionPrintPage = 0;
    if( m_PagesOption )
        s_OptionPrintPage = m_PagesOption->GetSelection();

    SetPenWidth();

    wxPrintDialogData printDialogData( *g_PrintData );

    wxPrinter         printer( &printDialogData );

    wxString          title = _("Preview");
    EDA_Printout      printout( this, m_Parent, title, print_ref );

#ifndef __WINDOWS__
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


/***************************************/
bool EDA_Printout::OnPrintPage( int page )
/***************************************/
{
    wxString msg;

    msg.Printf( _( "Print page %d" ), page );
    m_Parent->Affiche_Message( msg );

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
        return false;
    ActiveScreen = screen;
    DrawPage();
    ActiveScreen = oldscreen;
    schframe->m_CurrentSheet = oldsheetpath;
    schframe->m_CurrentSheet->UpdateAllScreenReferences();
    schframe->SetSheetNumberAndCount();

    return true;
}


/*********************************************************/
void EDA_Printout::GetPageInfo( int* minPage, int* maxPage,
                                int* selPageFrom, int* selPageTo )
/*********************************************************/
{
    int ii = 1;

    *minPage     = 1;
    *selPageFrom = 1;

    if( s_OptionPrintPage == 1 )
    {
        ii = g_RootSheet->CountSheets();
    }

    *maxPage   = ii;
    *selPageTo = ii;
}


/**************************************/
bool EDA_Printout::HasPage( int pageNum )
/**************************************/
{
    int pageCount;

    pageCount = g_RootSheet->CountSheets();
    if( pageCount >= pageNum )
        return true;

    return false;
}


/*************************************************************/
bool EDA_Printout::OnBeginDocument( int startPage, int endPage )
/*************************************************************/
{
    if( !wxPrintout::OnBeginDocument( startPage, endPage ) )
        return false;

    return true;
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
    double     DrawZoom = 1;
    wxDC*   dc = GetDC();

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

    SheetSize    = ActiveScreen->m_CurrentSheetDesc->m_Size;    // size in 1/1000 inch
    SheetSize.x *= m_Parent->m_InternalUnits / 1000;
    SheetSize.y *= m_Parent->m_InternalUnits / 1000;            // size in pixels

    // Get the size of the DC in pixels
    dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );

    // Calculate a suitable scaling factor
    scaleX = (double) SheetSize.x / PlotAreaSize.x;
    scaleY = (double) SheetSize.y / PlotAreaSize.y;
    scale  = wxMax( scaleX, scaleY ); // Use x or y scaling factor, whichever fits on the DC

    // ajust the real draw scale
    dc->SetUserScale( DrawZoom / scale, DrawZoom / scale );

    ActiveScreen->m_DrawOrg = DrawOffset;

    GRResetPenAndBrush( dc );
    if( s_Print_Black_and_White )
        GRForceBlackPen( true );


    /* set Pen min width */
    double ftmp, xdcscale, ydcscale;

    // g_PlotLine_Width is in internal units ( 1/1000 inch), and must be converted in pixels
    ftmp  = (float) g_PlotLine_Width * 25.4 / EESCHEMA_INTERNAL_UNIT;   // ftmp est en mm
    ftmp *= (float) PlotAreaSize.x / PageSize_in_mm.x;                  /* ftmp is in  pixels */

    /* because the pen size will be scaled by the dc scale, we modify the size
     * in order to keep the requested value */
    dc->GetUserScale( &xdcscale, &ydcscale );
    ftmp /= xdcscale;
    SetPenMinWidth( wxRound( ftmp ) );

    WinEDA_DrawPanel* panel = m_Parent->DrawPanel;
    BASE_SCREEN*      screen = panel->GetScreen();
    EDA_Rect          tmp   = panel->m_ClipBox;

    panel->m_ClipBox.SetOrigin( wxPoint( 0, 0 ) );
    panel->m_ClipBox.SetSize( wxSize( 0x7FFFFF0, 0x7FFFFF0 ) );

    screen->m_IsPrinting = true;
    int bg_color = g_DrawBgColor;

    panel->PrintPage( dc, m_Print_Sheet_Ref, 0xFFFFFFFF, false );

    g_DrawBgColor    = bg_color;
    screen->m_IsPrinting = false;
    panel->m_ClipBox = tmp;

    SetPenMinWidth( 1 );
    GRForceBlackPen( false );

    ActiveScreen->m_StartVisu = tmp_startvisu;
    ActiveScreen->m_DrawOrg   = old_org;
    ActiveScreen->SetZoom( tmpzoom );
}
