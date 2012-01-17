/****************************************/
/* File: dialog_print_for_modedit.cpp */
/****************************************/

// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "pcbplot.h"

#include "dialog_print_for_modedit_base.h"
#include "printout_controler.h"

#define WIDTH_MAX_VALUE           1000
#define WIDTH_MIN_VALUE           1

static double s_ScaleList[] =
{ 0, 0.5, 0.7, 1.0, 1.4, 2.0, 3.0, 4.0, 8.0, 16.0 };


// static print data and page setup data, to remember settings during the session
static PRINT_PARAMETERS  s_Parameters;
static wxPrintData* s_PrintData;
static wxPageSetupDialogData* s_pageSetupData = (wxPageSetupDialogData*) NULL;


/* Dialog to print schematic. Class derived from DIALOG_PRINT_FOR_MODEDIT_BASE
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_FOR_MODEDIT : public DIALOG_PRINT_FOR_MODEDIT_BASE
{
private:
    EDA_DRAW_FRAME* m_Parent;
    wxConfig*       m_Config;

public:
    DIALOG_PRINT_FOR_MODEDIT( EDA_DRAW_FRAME* parent );
    ~DIALOG_PRINT_FOR_MODEDIT() {};

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnPrintSetup( wxCommandEvent& event );
    void OnPrintPreview( wxCommandEvent& event );
    void OnPrintButtonClick( wxCommandEvent& event );

    void OnButtonCancelClick( wxCommandEvent& event ) { Close(); }
    void InitValues( );
};


/*************************************************************/
void FOOTPRINT_EDIT_FRAME::ToPrinter( wxCommandEvent& event )
/*************************************************************/
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

    DIALOG_PRINT_FOR_MODEDIT* frame = new DIALOG_PRINT_FOR_MODEDIT( this );

    frame->ShowModal();
    frame->Destroy();
}

/*************************************************************************************/
DIALOG_PRINT_FOR_MODEDIT::DIALOG_PRINT_FOR_MODEDIT( EDA_DRAW_FRAME* parent ) :
    DIALOG_PRINT_FOR_MODEDIT_BASE( parent )
/*************************************************************************************/
{
    m_Parent = parent;
    s_Parameters.m_ForceCentered = true;
    m_Config = wxGetApp().GetSettings();
    InitValues();

    m_buttonPrint->SetDefault();
    GetSizer()->SetSizeHints( this );
    Center();
}


/************************************************************************/
void DIALOG_PRINT_FOR_MODEDIT::InitValues( )
/************************************************************************/
{
    SetFocus();

    if( s_pageSetupData == NULL )
    {
        s_pageSetupData = new wxPageSetupDialogData;
        // Set initial page margins.
        // Margins are already set in Pcbnew, so we cans use 0
        s_pageSetupData->SetMarginTopLeft(wxPoint(0, 0));
        s_pageSetupData->SetMarginBottomRight(wxPoint(0, 0));
    }

    s_Parameters.m_PageSetupData = s_pageSetupData;

    // Read the scale adjust option
    int scale_Select = 3; // default selected scale = ScaleList[3] = 1
    if( m_Config )
    {
        m_Config->Read( OPTKEY_PRINT_MODULE_SCALE, &scale_Select );
        m_Config->Read( OPTKEY_PRINT_MONOCHROME_MODE, &s_Parameters.m_Print_Black_and_White, 1);
    }

    extern int g_DrawDefaultLineThickness;
    s_Parameters.m_PenDefaultSize = g_DrawDefaultLineThickness;
    m_ScaleOption->SetSelection( scale_Select );

    if( s_Parameters.m_Print_Black_and_White )
        m_ModeColorOption->SetSelection( 1 );
}


/********************************************************************/
void DIALOG_PRINT_FOR_MODEDIT::OnCloseWindow( wxCloseEvent& event )
/********************************************************************/
{
    if( m_Config )
    {
        m_Config->Write( OPTKEY_PRINT_MODULE_SCALE, m_ScaleOption->GetSelection() );
        m_Config->Write( OPTKEY_PRINT_MONOCHROME_MODE, s_Parameters.m_Print_Black_and_White);
    }
    EndModal( 0 );
}


/**********************************************************/
void DIALOG_PRINT_FOR_MODEDIT::OnPrintSetup( wxCommandEvent& event )
/**********************************************************/

/* Open a dialog box for printer setup (printer options, page size ...)
 */
{
    wxPrintDialogData printDialogData( *s_PrintData );

    if( printDialogData.Ok() )
    {
        wxPrintDialog printerDialog( this, &printDialogData );
        printerDialog.ShowModal();
        *s_PrintData = printerDialog.GetPrintDialogData().GetPrintData();
    }
    else
        DisplayError( this, _( "Printer Problem!" ) );
}


/************************************************************/
void DIALOG_PRINT_FOR_MODEDIT::OnPrintPreview( wxCommandEvent& event )
/************************************************************/

/* Open and display a previewer frame for printing
 */
{
    s_Parameters.m_Print_Black_and_White = m_ModeColorOption->GetSelection();
    s_Parameters.m_PrintScale = s_ScaleList[m_ScaleOption->GetSelection()];

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

     // Uses the parent position and size.
    // @todo uses last position and size ans store them when exit in m_Config
    wxPoint         WPos  = m_Parent->GetPosition();
    wxSize          WSize = m_Parent->GetSize();

    wxPreviewFrame* frame = new wxPreviewFrame( preview, this, title, WPos, WSize );

    frame->Initialize();
    frame->Show( TRUE );
}


/***************************************************************************/
void DIALOG_PRINT_FOR_MODEDIT::OnPrintButtonClick( wxCommandEvent& event )
/***************************************************************************/

/* Called on activate Print button
 */
{
    s_Parameters.m_Print_Black_and_White = m_ModeColorOption->GetSelection();
    s_Parameters.m_PrintScale = s_ScaleList[m_ScaleOption->GetSelection()];

    g_PcbPlotOptions.m_FineScaleAdjustX = s_Parameters.m_XScaleAdjust;
    g_PcbPlotOptions.m_FineScaleAdjustY = s_Parameters.m_YScaleAdjust;
    g_PcbPlotOptions.m_PlotScale = s_Parameters.m_PrintScale;

    wxPrintDialogData printDialogData( *s_PrintData );
    wxPrinter         printer( &printDialogData );

    BOARD_PRINTOUT_CONTROLER      printout( s_Parameters, m_Parent, _( "Print Footprint" ) );

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
        *s_PrintData = printer.GetPrintDialogData().GetPrintData();
    }
}
