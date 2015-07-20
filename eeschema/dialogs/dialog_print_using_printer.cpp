/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <pgm_base.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <class_sch_screen.h>
#include <schframe.h>
#include <base_units.h>

#include <general.h>
#include <eeschema_config.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>

#include <invoke_sch_dialog.h>
#include <dialog_print_using_printer_base.h>



/**
 * Class DIALOG_PRINT_USING_PRINTER
 * offers to print a schematic dialog.
 *
 * Derived from DIALOG_PRINT_USING_PRINTER_base created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_BASE
{
public:
    DIALOG_PRINT_USING_PRINTER( SCH_EDIT_FRAME* aParent );

    SCH_EDIT_FRAME* GetParent() const
    {
        return ( SCH_EDIT_FRAME* ) wxWindow::GetParent();
    }

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnPageSetup( wxCommandEvent& event );
    void OnPrintPreview( wxCommandEvent& event );
    void OnPrintButtonClick( wxCommandEvent& event );
    void OnButtonCancelClick( wxCommandEvent& event ) { Close(); }

    void initDialog();
    void GetPrintOptions();
};



/**
 * Custom print out for printing schematics.
 */
class SCH_PRINTOUT : public wxPrintout
{
private:
    DIALOG_PRINT_USING_PRINTER* m_parent;

public:
    SCH_PRINTOUT( DIALOG_PRINT_USING_PRINTER* aParent, const wxString& aTitle ) :
        wxPrintout( aTitle )
    {
        wxASSERT( aParent != NULL );

        m_parent = aParent;
    }
    SCH_EDIT_FRAME* GetSchFrameParent() { return m_parent->GetParent(); }
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

    bool Show( bool show )      // overload
    {
        bool        ret;

        // Show or hide the window.  If hiding, save current position and size.
        // If showing, use previous position and size.
        if( show )
        {
            ret = wxPreviewFrame::Show( show );

            if( s_size.x != 0 && s_size.y != 0 )
                SetSize( s_pos.x, s_pos.y, s_size.x, s_size.y, 0 );
        }
        else
        {
            // Save the dialog's position & size before hiding
            s_size = GetSize();
            s_pos  = GetPosition();

            ret = wxPreviewFrame::Show( show );
        }
        return ret;
    }

private:
    static wxPoint  s_pos;
    static wxSize   s_size;

    DECLARE_CLASS( SCH_PREVIEW_FRAME )
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS( SCH_PREVIEW_FRAME )
};

wxPoint SCH_PREVIEW_FRAME::s_pos;
wxSize  SCH_PREVIEW_FRAME::s_size;

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
    initDialog();

#ifdef __WXMAC__
    // Problems with modal on wx-2.9 - Anyway preview is standard for OSX
   m_buttonPreview->Hide();
#endif

    GetSizer()->Fit( this );
}


void DIALOG_PRINT_USING_PRINTER::initDialog()
{
    SCH_EDIT_FRAME* parent = GetParent();

    // Initialize page specific print setup dialog settings.
    const PAGE_INFO& pageInfo = parent->GetScreen()->GetPageSettings();
    wxPageSetupDialogData& pageSetupDialogData = parent->GetPageSetupData();

    pageSetupDialogData.SetPaperId( pageInfo.GetPaperId() );

    if( pageInfo.IsCustom() )
    {
        if( pageInfo.IsPortrait() )
            pageSetupDialogData.SetPaperSize( wxSize( Mils2mm( pageInfo.GetWidthMils() ),
                                                      Mils2mm( pageInfo.GetHeightMils() ) ) );
        else
            pageSetupDialogData.SetPaperSize( wxSize( Mils2mm( pageInfo.GetHeightMils() ),
                                                      Mils2mm( pageInfo.GetWidthMils() ) ) );
    }

    pageSetupDialogData.GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

    if ( GetSizer() )
        GetSizer()->SetSizeHints( this );

    // Rely on the policy in class DIALOG_SHIM, which centers the dialog
    // initially during a runtime session but gives user the ability to move it in
    // that session.
    // This dialog may get moved and resized in Show(), but in case this is
    // the first time, center it for starters.
    Center();

    m_buttonPrint->SetDefault();    // on linux, this is inadequate to determine
                                    // what ENTER does.  Must also SetFocus().
    m_buttonPrint->SetFocus();
}


void DIALOG_PRINT_USING_PRINTER::GetPrintOptions()
{
    SCH_EDIT_FRAME* parent = GetParent();

    parent->SetPrintMonochrome( m_checkMonochrome->IsChecked() );
    parent->SetPrintSheetReference( m_checkReference->IsChecked() );
}


void DIALOG_PRINT_USING_PRINTER::OnCloseWindow( wxCloseEvent& event )
{
    SCH_EDIT_FRAME* parent = GetParent();

    if( !IsIconized() )
    {
        parent->SetPrintDialogPosition( GetPosition() );
        parent->SetPrintDialogSize( GetSize() );
    }

    GetPrintOptions();

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

    GetPrintOptions();

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

    SCH_PREVIEW_FRAME* frame = new SCH_PREVIEW_FRAME( preview, this, title );
    frame->SetMinSize( wxSize( 550, 350 ) );

    // on first invocation in this runtime session, set to 2/3 size of my parent,
    // but will be changed in Show() if not first time as will position.
    frame->SetSize( (parent->GetSize() * 2) / 3 );
    frame->Center();

    frame->Initialize();

    frame->Raise(); // Needed on Ubuntu/Unity to display the frame
    frame->Show( true );
}


void DIALOG_PRINT_USING_PRINTER::OnPrintButtonClick( wxCommandEvent& event )
{
    SCH_EDIT_FRAME* parent = GetParent();

    GetPrintOptions();

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
    SCH_EDIT_FRAME* parent = m_parent->GetParent();
    msg.Printf( _( "Print page %d" ), page );
    parent->ClearMsgPanel();
    parent->AppendMsgPanel( msg, wxEmptyString, CYAN );

    SCH_SCREEN*     screen       = parent->GetScreen();
    SCH_SHEET_PATH  oldsheetpath = parent->GetCurrentSheet();
    SCH_SHEET_PATH  list;
    SCH_SHEET_LIST  SheetList( NULL );
    SCH_SHEET_PATH* sheetpath = SheetList.GetSheet( page - 1 );

    if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
    {
        parent->SetCurrentSheet( list );
        parent->GetCurrentSheet().UpdateAllScreenReferences();
        parent->SetSheetNumberAndCount();
        screen = parent->GetCurrentSheet().LastScreen();
    }
    else
    {
        screen = NULL;
    }

    if( screen == NULL )
        return false;

    DrawPage( screen );
    parent->SetCurrentSheet( oldsheetpath );
    parent->GetCurrentSheet().UpdateAllScreenReferences();
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
    SCH_EDIT_FRAME* parent = m_parent->GetParent();
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
    wxSize   pageSizeIU;             // Page size in internal units
    wxPoint  old_org;
    EDA_RECT oldClipBox;
    wxRect   fitRect;
    wxDC*    dc = GetDC();
    SCH_EDIT_FRAME* parent = m_parent->GetParent();
    EDA_DRAW_PANEL* panel = parent->GetCanvas();

    wxBusyCursor dummy;

    // Save current scale factor, offsets, and clip box.
    tmp_startvisu = aScreen->m_StartVisu;
    oldZoom = aScreen->GetZoom();
    old_org = aScreen->m_DrawOrg;

    oldClipBox = *panel->GetClipBox();

    // Change clip box to print the whole page.
    #define MAX_VALUE (INT_MAX/2)   // MAX_VALUE is the max we can use in an integer
                                    // and that allows calculations without overflow
    panel->SetClipBox( EDA_RECT( wxPoint( 0, 0 ), wxSize( MAX_VALUE, MAX_VALUE ) ) );

    // Change scale factor and offset to print the whole page.
    bool printReference = parent->GetPrintSheetReference();

    pageSizeIU = aScreen->GetPageSettings().GetSizeIU();
    FitThisSizeToPaper( pageSizeIU );
    fitRect = GetLogicalPaperRect();

    wxLogDebug( wxT( "Fit rectangle: x = %d, y = %d, w = %d, h = %d" ),
                  fitRect.x, fitRect.y, fitRect.width, fitRect.height );

    // When is the actual paper size does not match the schematic page
    // size, the drawing is not perfectly centered on X or Y axis.
    // Give a draw offset centers the schematic page on the paper draw area
    // Because the sizes are fitted, only an Y or X offset is needed
    // and both are 0 when sizes are identical.
    // Y or Y offset is not null when the X/Y size ratio differs between
    // the actual paper size and the schematic page
    int xoffset = ( fitRect.width - pageSizeIU.x ) / 2;

    // For an obscure reason, OffsetLogicalOrigin creates issues,
    // under some circumstances, when yoffset is not always null
    // and changes from a page to an other page
    // This is only a workaround, not a fix
    // see https://bugs.launchpad.net/kicad/+bug/1464773
    // xoffset does not create issues.
#if 0   // FIX ME
    int yoffset = ( fitRect.height - pageSizeIU.y ) / 2;
#else
    // the Y centering will be not perfect, but this is less annoying
    // than a blank page or a buggy centering
    int yoffset = 0;
#endif
    OffsetLogicalOrigin( xoffset, yoffset );

    GRResetPenAndBrush( dc );

    if( parent->GetPrintMonochrome() )
        GRForceBlackPen( true );

    aScreen->m_IsPrinting = true;

    EDA_COLOR_T bg_color = GetSchFrameParent()->GetDrawBgColor();

    aScreen->Draw( panel, dc, (GR_DRAWMODE) 0 );

    if( printReference )
        parent->DrawWorkSheet( dc, aScreen, GetDefaultLineThickness(),
                IU_PER_MILS, aScreen->GetFileName() );

    GetSchFrameParent()->SetDrawBgColor( bg_color );
    aScreen->m_IsPrinting = false;
    panel->SetClipBox( oldClipBox );

    GRForceBlackPen( false );

    aScreen->m_StartVisu = tmp_startvisu;
    aScreen->m_DrawOrg   = old_org;
    aScreen->SetZoom( oldZoom );
}


int InvokeDialogPrintUsingPrinter( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_PRINT_USING_PRINTER dlg( aCaller );

    return dlg.ShowModal();
}
