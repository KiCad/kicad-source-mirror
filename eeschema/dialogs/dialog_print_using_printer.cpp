/****************************************/
/* File: dialog_print_using_printer.cpp */
/****************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "eeschema_config.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"

#include "dialog_print_using_printer.h"


/**
 * Custom print out for printing schematics.
 */
class SCH_PRINTOUT : public wxPrintout
{
private:
    DIALOG_PRINT_USING_PRINTER* m_Parent;

public:
    SCH_PRINTOUT( DIALOG_PRINT_USING_PRINTER* aParent, const wxString& aTitle ) :
        wxPrintout( aTitle )
    {
        wxASSERT( aParent != NULL );

        m_Parent = aParent;
    }

    bool OnPrintPage( int page );
    bool HasPage( int page );
    bool OnBeginDocument( int startPage, int endPage );
    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo );
    void DrawPage( SCH_SCREEN* aScreen );
};


/**
 * Custom schematic print preview frame.
 */
class SCH_PREVIEW_FRAME : public wxPreviewFrame
{
public:
    SCH_PREVIEW_FRAME( wxPrintPreview* aPreview, DIALOG_PRINT_USING_PRINTER* aParent,
                       const wxString& aTitle, const wxPoint& aPos = wxDefaultPosition,
                       const wxSize& aSize = wxDefaultSize ) :
        wxPreviewFrame( aPreview, aParent, aTitle, aPos, aSize )
    {
    }

    DIALOG_PRINT_USING_PRINTER* GetParent()
    {
        return ( DIALOG_PRINT_USING_PRINTER* )wxWindow::GetParent();
    }

    void OnCloseWindow( wxCloseEvent& event )
    {
        if( !IsIconized() )
        {
            GetParent()->GetParent()->SetPreviewPosition( GetPosition() );
            GetParent()->GetParent()->SetPreviewSize( GetSize() );
        }

        wxPreviewFrame::OnCloseWindow( event );
    }

private:
    DECLARE_CLASS( SCH_PREVIEW_FRAME )
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS( SCH_PREVIEW_FRAME )
};


IMPLEMENT_CLASS( SCH_PREVIEW_FRAME, wxPreviewFrame )

BEGIN_EVENT_TABLE( SCH_PREVIEW_FRAME, wxPreviewFrame )
    EVT_CLOSE( SCH_PREVIEW_FRAME::OnCloseWindow )
END_EVENT_TABLE()


DIALOG_PRINT_USING_PRINTER::DIALOG_PRINT_USING_PRINTER( SCH_EDIT_FRAME* aParent ) :
    DIALOG_PRINT_USING_PRINTER_BASE( aParent )
{
    wxASSERT( aParent != NULL );

    m_checkReference->SetValue( aParent->GetPrintSheetReference() );
    m_checkMonochrome->SetValue( aParent->GetPrintMonochrome() );

#ifdef __WXMAC__
    /* Problems with modal on wx-2.9 - Anyway preview is standard for OSX */
   m_buttonPreview->Hide();
#endif
}


SCH_EDIT_FRAME* DIALOG_PRINT_USING_PRINTER::GetParent() const
{
    return ( SCH_EDIT_FRAME* ) wxWindow::GetParent();
}


void DIALOG_PRINT_USING_PRINTER::OnInitDialog( wxInitDialogEvent& event )
{
    SCH_EDIT_FRAME* parent = GetParent();

    if ( GetSizer() )
        GetSizer()->SetSizeHints( this );

    if( parent->GetPrintDialogPosition() == wxDefaultPosition &&
        parent->GetPrintDialogSize() == wxDefaultSize )
    {
        Center();
    }
    else
    {
        SetPosition( parent->GetPrintDialogPosition() );
        SetSize( parent->GetPrintDialogSize() );
    }

    SetFocus();

    m_buttonPrint->SetDefault();
}


void DIALOG_PRINT_USING_PRINTER::OnCloseWindow( wxCloseEvent& event )
{
    SCH_EDIT_FRAME* parent = GetParent();

    if( !IsIconized() )
    {
        parent->SetPrintDialogPosition( GetPosition() );
        parent->SetPrintDialogSize( GetSize() );
    }

    parent->SetPrintMonochrome( m_checkMonochrome->IsChecked() );
    parent->SetPrintSheetReference( m_checkReference->IsChecked() );

    EndDialog( wxID_CANCEL );
}


/* Open a dialog box for printer setup (printer options, page size ...)
 */
void DIALOG_PRINT_USING_PRINTER::OnPageSetup( wxCommandEvent& event )
{
    SCH_EDIT_FRAME* parent = GetParent();

    wxPageSetupDialog pageSetupDialog( this, &parent->GetPageSetupData() );

    pageSetupDialog.ShowModal();

    parent->GetPageSetupData() = pageSetupDialog.GetPageSetupDialogData();
}


/* Open and display a previewer frame for printing
 */
void DIALOG_PRINT_USING_PRINTER::OnPrintPreview( wxCommandEvent& event )
{
    SCH_EDIT_FRAME* parent = GetParent();

    parent->SetPrintMonochrome( m_checkMonochrome->IsChecked() );
    parent->SetPrintSheetReference( m_checkReference->IsChecked() );

    // Pass two printout objects: for preview, and possible printing.
    wxString        title   = _( "Preview" );
    wxPrintPreview* preview = new wxPrintPreview( new SCH_PRINTOUT( this, title ),
                                                  new SCH_PRINTOUT( this, title ),
                                                  &parent->GetPageSetupData().GetPrintData() );

    if( preview == NULL )
    {
        DisplayError( this, wxT( "Print preview error!" ) );
        return;
    }

    preview->SetZoom( 100 );

    SCH_PREVIEW_FRAME* frame = new SCH_PREVIEW_FRAME( preview, this, title,
                                                      parent->GetPreviewPosition(),
                                                      parent->GetPreviewSize() );
    frame->Initialize();
    frame->Show( true );
}


void DIALOG_PRINT_USING_PRINTER::OnPrintButtonClick( wxCommandEvent& event )
{
    SCH_EDIT_FRAME* parent = GetParent();

    parent->SetPrintMonochrome( m_checkMonochrome->IsChecked() );
    parent->SetPrintSheetReference( m_checkReference->IsChecked() );

    wxPrintDialogData printDialogData( parent->GetPageSetupData().GetPrintData() );
    printDialogData.SetMaxPage( g_RootSheet->CountSheets() );

    if( g_RootSheet->CountSheets() > 1 )
        printDialogData.EnablePageNumbers( true );

    wxPrinter printer( &printDialogData );
    SCH_PRINTOUT printout( this, _( "Print Schematic" ) );

    if( !printer.Print( this, &printout, true ) )
    {
        if( wxPrinter::GetLastError() == wxPRINTER_ERROR )
            wxMessageBox( _( "An error occurred attempting to print the schematic." ),
                          _( "Printing" ), wxOK );
    }
    else
    {
        parent->GetPageSetupData() = printer.GetPrintDialogData().GetPrintData();
    }
}


bool SCH_PRINTOUT::OnPrintPage( int page )
{
    wxString msg;
    SCH_EDIT_FRAME* parent = m_Parent->GetParent();
    msg.Printf( _( "Print page %d" ), page );
    parent->ClearMsgPanel();
    parent->AppendMsgPanel( msg, wxEmptyString, CYAN );

    SCH_SCREEN*     screen       = parent->GetScreen();
    SCH_SHEET_PATH* oldsheetpath = parent->GetSheet();
    SCH_SHEET_PATH  list;
    SCH_SHEET_LIST  SheetList( NULL );
    SCH_SHEET_PATH* sheetpath = SheetList.GetSheet( page - 1 );

    if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
    {
        parent->m_CurrentSheet = &list;
        parent->m_CurrentSheet->UpdateAllScreenReferences();
        parent->SetSheetNumberAndCount();
        screen = parent->m_CurrentSheet->LastScreen();
    }
    else
    {
        screen = NULL;
    }

    if( screen == NULL )
        return false;

    DrawPage( screen );
    parent->m_CurrentSheet = oldsheetpath;
    parent->m_CurrentSheet->UpdateAllScreenReferences();
    parent->SetSheetNumberAndCount();

    return true;
}


void SCH_PRINTOUT::GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo )
{
    *minPage = *selPageFrom = 1;
    *maxPage = *selPageTo   = g_RootSheet->CountSheets();
}


bool SCH_PRINTOUT::HasPage( int pageNum )
{
    int pageCount;

    pageCount = g_RootSheet->CountSheets();
    if( pageCount >= pageNum )
        return true;

    return false;
}


bool SCH_PRINTOUT::OnBeginDocument( int startPage, int endPage )
{
    if( !wxPrintout::OnBeginDocument( startPage, endPage ) )
        return false;

#ifdef __WXDEBUG__
    SCH_EDIT_FRAME* parent = m_Parent->GetParent();
    wxLogDebug( wxT( "Printer name: " ) +
                parent->GetPageSetupData().GetPrintData().GetPrinterName() );
    wxLogDebug( wxT( "Paper ID: %d" ),
                parent->GetPageSetupData().GetPrintData().GetPaperId() );
    wxLogDebug( wxT( "Color: %d" ),
                (int) parent->GetPageSetupData().GetPrintData().GetColour() );
    wxLogDebug( wxT( "Monochrome: %d" ), parent->GetPrintMonochrome() );
    wxLogDebug( wxT( "Orientation: %d:" ),
                parent->GetPageSetupData().GetPrintData().GetOrientation() );
    wxLogDebug( wxT( "Quality: %d"),
                parent->GetPageSetupData().GetPrintData().GetQuality() );
#endif

    return true;
}


/*
 * This is the real print function: print the active screen
 */
void SCH_PRINTOUT::DrawPage( SCH_SCREEN* aScreen )
{
    int      oldZoom;
    wxPoint  tmp_startvisu;
    wxSize   SheetSize;      // Page size in internal units
    wxPoint  old_org;
    EDA_RECT oldClipBox;
    wxRect   fitRect;
    wxDC*    dc = GetDC();
    SCH_EDIT_FRAME* parent = m_Parent->GetParent();
    EDA_DRAW_PANEL* panel = parent->DrawPanel;

    wxBusyCursor dummy;

    /* Save current scale factor, offsets, and clip box. */
    tmp_startvisu = aScreen->m_StartVisu;
    oldZoom = aScreen->GetZoom();
    old_org = aScreen->m_DrawOrg;
    oldClipBox = panel->m_ClipBox;

    /* Change scale factor, offsets, and clip box to print the whole page. */
    panel->m_ClipBox.SetOrigin( wxPoint( 0, 0 ) );
    panel->m_ClipBox.SetSize( wxSize( 0x7FFFFF0, 0x7FFFFF0 ) );

    bool printReference = parent->GetPrintSheetReference();

    if( printReference )
    {
        /* Draw the page to a memory and let the dc calculate the drawing
         * limits.
         */
        wxBitmap psuedoBitmap( 1, 1 );
        wxMemoryDC memDC;
        memDC.SelectObject( psuedoBitmap );
        aScreen->Draw( panel, &memDC, GR_DEFAULT_DRAWMODE );
        parent->TraceWorkSheet( &memDC, aScreen, g_DrawDefaultLineThickness );

        wxLogDebug( wxT( "MinX = %d, MaxX = %d, MinY = %d, MaxY = %d" ),
                    memDC.MinX(), memDC.MaxX(), memDC.MinY(), memDC.MaxY() );

        SheetSize.x = memDC.MaxX() - memDC.MinX();
        SheetSize.y = memDC.MaxY() - memDC.MinY();

        FitThisSizeToPageMargins( SheetSize, parent->GetPageSetupData() );
        fitRect = GetLogicalPageMarginsRect( parent->GetPageSetupData() );
    }
    else
    {
        SheetSize = aScreen->m_CurrentSheetDesc->m_Size;
        FitThisSizeToPaper( SheetSize );
        fitRect = GetLogicalPaperRect();
    }

    wxLogDebug( wxT( "Fit rectangle: %d, %d, %d, %d" ),
                fitRect.x, fitRect.y, fitRect.width, fitRect.height );

    GRResetPenAndBrush( dc );

    if( parent->GetPrintMonochrome() )
        GRForceBlackPen( true );

    aScreen->m_IsPrinting = true;

    int bg_color = g_DrawBgColor;

    aScreen->Draw( panel, dc, GR_DEFAULT_DRAWMODE );

    if( printReference )
        parent->TraceWorkSheet( dc, aScreen, g_DrawDefaultLineThickness );

    g_DrawBgColor = bg_color;
    aScreen->m_IsPrinting = false;
    panel->m_ClipBox = oldClipBox;

    GRForceBlackPen( false );

    aScreen->m_StartVisu = tmp_startvisu;
    aScreen->m_DrawOrg   = old_org;
    aScreen->SetZoom( oldZoom );
}
