/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_screen.h>
#include <sch_edit_frame.h>
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
    ~DIALOG_PRINT_USING_PRINTER() override;

    SCH_EDIT_FRAME* GetParent() const
    {
        return ( SCH_EDIT_FRAME* ) wxWindow::GetParent();
    }

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnPageSetup( wxCommandEvent& event ) override;
    void OnPrintPreview( wxCommandEvent& event ) override;

    void GetPrintOptions();
};



/**
 * Custom print out for printing schematics.
 */
class SCH_PRINTOUT : public wxPrintout
{
private:
    SCH_EDIT_FRAME* m_parent;

public:
    SCH_PRINTOUT( SCH_EDIT_FRAME* aParent, const wxString& aTitle ) :
        wxPrintout( aTitle )
    {
        wxASSERT( aParent != NULL );
        m_parent = aParent;
    }
    bool OnPrintPage( int page ) override;
    bool HasPage( int page ) override;
    bool OnBeginDocument( int startPage, int endPage ) override;
    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo ) override;
    void DrawPage( SCH_SCREEN* aScreen );
};


/**
 * Custom schematic print preview frame.
 * This derived preview frame remembers its size and position during a session
 */
class SCH_PREVIEW_FRAME : public wxPreviewFrame
{
public:
    SCH_PREVIEW_FRAME( wxPrintPreview* aPreview, wxWindow* aParent,
                       const wxString& aTitle, const wxPoint& aPos = wxDefaultPosition,
                       const wxSize& aSize = wxDefaultSize ) :
        wxPreviewFrame( aPreview, aParent, aTitle, aPos, aSize )
    {
    }

    bool Show( bool show ) override
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
};


wxPoint SCH_PREVIEW_FRAME::s_pos;
wxSize  SCH_PREVIEW_FRAME::s_size;


DIALOG_PRINT_USING_PRINTER::DIALOG_PRINT_USING_PRINTER( SCH_EDIT_FRAME* aParent ) :
    DIALOG_PRINT_USING_PRINTER_BASE( aParent )
{
    wxASSERT( aParent != NULL );

    m_checkReference->SetValue( aParent->GetPrintSheetReference() );
    m_checkMonochrome->SetValue( aParent->GetPrintMonochrome() );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Print" ) );
    m_sdbSizer1Apply->SetLabel( _( "Preview" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

#ifdef __WXMAC__
    // Problems with modal on wx-2.9 - Anyway preview is standard for OSX
    m_sdbSizer1Apply->Hide();
#endif

    m_sdbSizer1OK->SetDefault();    // on linux, this is inadequate to determine
                                    // what ENTER does.  Must also SetFocus().
    m_sdbSizer1OK->SetFocus();

    FinishDialogSettings();
}


DIALOG_PRINT_USING_PRINTER::~DIALOG_PRINT_USING_PRINTER()
{
    GetPrintOptions();
}


bool DIALOG_PRINT_USING_PRINTER::TransferDataToWindow()
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

    return true;
}


void DIALOG_PRINT_USING_PRINTER::GetPrintOptions()
{
    SCH_EDIT_FRAME* parent = GetParent();

    parent->SetPrintMonochrome( m_checkMonochrome->IsChecked() );
    parent->SetPrintSheetReference( m_checkReference->IsChecked() );
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
    wxPrintPreview* preview = new wxPrintPreview( new SCH_PRINTOUT( parent, title ),
                                                  new SCH_PRINTOUT( parent, title ),
                                                  &parent->GetPageSetupData().GetPrintData() );

    preview->SetZoom( 100 );

    SCH_PREVIEW_FRAME* frame = new SCH_PREVIEW_FRAME( preview, this, title );
    frame->SetMinSize( wxSize( 550, 350 ) );

    // on first invocation in this runtime session, set to 2/3 size of my parent,
    // but will be changed in Show() if not first time as will position.
    frame->SetSize( (parent->GetSize() * 2) / 3 );
    frame->Center();

    // On wxGTK, set the flag wxTOPLEVEL_EX_DIALOG is mandatory, if we want
    // close the frame using the X box in caption, when the preview frame is run
    // from a dialog
    frame->SetExtraStyle( frame->GetExtraStyle() | wxTOPLEVEL_EX_DIALOG );

    // We use here wxPreviewFrame_WindowModal option to make the wxPrintPreview frame
    // modal for its caller only.
    // another reason is the fact when closing the frame without this option,
    // all top level frames are reenabled.
    // With this option, only the parent is reenabled.
    // Reenabling all top level frames should be made by the parent dialog.
    frame->InitializeWithModality( wxPreviewFrame_WindowModal );

    frame->Raise(); // Needed on Ubuntu/Unity to display the frame
    frame->Show( true );
}


bool DIALOG_PRINT_USING_PRINTER::TransferDataFromWindow()
{
    SCH_EDIT_FRAME* parent = GetParent();

    GetPrintOptions();

    wxPrintDialogData printDialogData( parent->GetPageSetupData().GetPrintData() );
    printDialogData.SetMaxPage( g_RootSheet->CountSheets() );

    if( g_RootSheet->CountSheets() > 1 )
        printDialogData.EnablePageNumbers( true );

    wxPrinter printer( &printDialogData );
    SCH_PRINTOUT printout( parent, _( "Print Schematic" ) );

    // Disable 'Print' button to prevent issuing another print
    // command before the previous one is finished (causes problems on Windows)
    m_sdbSizer1OK->Enable( false );

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

    return true;
}


bool SCH_PRINTOUT::OnPrintPage( int page )
{
    SCH_SHEET_LIST sheetList( g_RootSheet );

    wxCHECK_MSG( page >= 1 && page <= (int)sheetList.size(), false,
                 wxT( "Cannot print invalid page number." ) );

    wxCHECK_MSG( sheetList[ page - 1].LastScreen() != NULL, false,
                 wxT( "Cannot print page with NULL screen." ) );

    wxString msg;
    msg.Printf( _( "Print page %d" ), page );
    m_parent->ClearMsgPanel();
    m_parent->AppendMsgPanel( msg, wxEmptyString, CYAN );

    SCH_SCREEN*     screen       = m_parent->GetScreen();
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();
    m_parent->SetCurrentSheet( sheetList[ page - 1 ] );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
    screen = m_parent->GetCurrentSheet().LastScreen();
    DrawPage( screen );
    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();

    return true;
}


void SCH_PRINTOUT::GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo )
{
    *minPage = *selPageFrom = 1;
    *maxPage = *selPageTo   = g_RootSheet->CountSheets();
}


bool SCH_PRINTOUT::HasPage( int pageNum )
{
    return g_RootSheet->CountSheets() >= pageNum;
}


bool SCH_PRINTOUT::OnBeginDocument( int startPage, int endPage )
{
    if( !wxPrintout::OnBeginDocument( startPage, endPage ) )
        return false;

#ifdef __WXDEBUG__
    wxLogDebug( wxT( "Printer name: " ) +
                m_parent->GetPageSetupData().GetPrintData().GetPrinterName() );
    wxLogDebug( wxT( "Paper ID: %d" ),
                m_parent->GetPageSetupData().GetPrintData().GetPaperId() );
    wxLogDebug( wxT( "Color: %d" ),
                (int)m_parent->GetPageSetupData().GetPrintData().GetColour() );
    wxLogDebug( wxT( "Monochrome: %d" ), m_parent->GetPrintMonochrome() );
    wxLogDebug( wxT( "Orientation: %d:" ),
                m_parent->GetPageSetupData().GetPrintData().GetOrientation() );
    wxLogDebug( wxT( "Quality: %d"),
                m_parent->GetPageSetupData().GetPrintData().GetQuality() );
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
    auto panel = m_parent->GetCanvas();

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
    bool printReference = m_parent->GetPrintSheetReference();

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
    // and changes from a page to another page
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

    aScreen->m_IsPrinting = true;

    COLOR4D bgColor = m_parent->GetDrawBgColor();
    m_parent->SetDrawBgColor( COLOR4D::WHITE );

    GRSetDrawMode( dc, GR_COPY );
    GRSFilledRect( nullptr, dc, fitRect.GetX(), fitRect.GetY(),
                   fitRect.GetRight(), fitRect.GetBottom(),
                   0, COLOR4D::WHITE, COLOR4D::WHITE );

    if( m_parent->GetPrintMonochrome() )
        GRForceBlackPen( true );

    if( printReference )
        m_parent->DrawWorkSheet( dc, aScreen, GetDefaultLineThickness(),
                IU_PER_MILS, aScreen->GetFileName(), wxEmptyString,
                GetLayerColor( ( SCH_LAYER_ID )LAYER_WORKSHEET ) );


    aScreen->Draw( panel, dc, (GR_DRAWMODE) 0 );
    m_parent->SetDrawBgColor( bgColor );
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
