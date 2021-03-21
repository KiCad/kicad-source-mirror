/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <pgm_base.h>
#include <confirm.h>
#include <sch_screen.h>
#include <sch_edit_frame.h>
#include <base_units.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <sch_sheet_path.h>
#include <dialog_print_using_printer_base.h>
#include <sch_painter.h>

class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_BASE
{
public:
    DIALOG_PRINT_USING_PRINTER( SCH_EDIT_FRAME* aParent );
    ~DIALOG_PRINT_USING_PRINTER() override;

protected:
    void OnMonochromeChecked( wxCommandEvent& event ) override;
    void OnUseColorThemeChecked( wxCommandEvent& event ) override;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnPageSetup( wxCommandEvent& event ) override;
    void OnPrintPreview( wxCommandEvent& event ) override;

    void SavePrintOptions();

    SCH_EDIT_FRAME* m_parent;
};



/**
 * Custom print out for printing schematics.
 */
class SCH_PRINTOUT : public wxPrintout
{
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
    void PrintPage( SCH_SCREEN* aScreen );

private:
    SCH_EDIT_FRAME* m_parent;
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
    DIALOG_PRINT_USING_PRINTER_BASE( aParent ),
    m_parent( aParent )
{
    wxASSERT( aParent );

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

    finishDialogSettings();
}


DIALOG_PRINT_USING_PRINTER::~DIALOG_PRINT_USING_PRINTER()
{
    SavePrintOptions();
}


bool DIALOG_PRINT_USING_PRINTER::TransferDataToWindow()
{
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    m_checkReference->SetValue( cfg->m_Printing.title_block );
    m_checkMonochrome->SetValue( cfg->m_Printing.monochrome );
    m_checkBackgroundColor->SetValue( cfg->m_Printing.background );
    m_checkUseColorTheme->SetValue( cfg->m_Printing.use_theme );

    m_colorTheme->Clear();

    int width    = 0;
    int height   = 0;
    int minwidth = width;

    wxString target = cfg->m_Printing.use_theme ? cfg->m_Printing.color_theme : cfg->m_ColorTheme;

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        int pos = m_colorTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings->GetFilename() == target )
            m_colorTheme->SetSelection( pos );

        m_colorTheme->GetTextExtent( settings->GetName(), &width, &height );
        minwidth = std::max( minwidth, width );
    }

    m_colorTheme->SetMinSize( wxSize( minwidth + 50, -1 ) );

    m_colorTheme->Enable( cfg->m_Printing.use_theme );

    // Initialize page specific print setup dialog settings.
    const PAGE_INFO& pageInfo = m_parent->GetScreen()->GetPageSettings();
    wxPageSetupDialogData& pageSetupDialogData = m_parent->GetPageSetupData();

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

    Layout();

    return true;
}


void DIALOG_PRINT_USING_PRINTER::OnUseColorThemeChecked( wxCommandEvent& event )
{
    m_colorTheme->Enable( m_checkUseColorTheme->GetValue() );
}


void DIALOG_PRINT_USING_PRINTER::OnMonochromeChecked( wxCommandEvent& event )
{
    m_checkBackgroundColor->Enable( !m_checkMonochrome->GetValue() );

    if( m_checkMonochrome->GetValue() )
        m_checkBackgroundColor->SetValue( false );
    else
        m_checkBackgroundColor->SetValue( m_parent->eeconfig()->m_Printing.background );
}


void DIALOG_PRINT_USING_PRINTER::SavePrintOptions()
{
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    cfg->m_Printing.monochrome  = m_checkMonochrome->IsChecked();
    cfg->m_Printing.title_block = m_checkReference->IsChecked();
    cfg->m_Printing.background  = m_checkBackgroundColor->IsChecked();
    cfg->m_Printing.use_theme   = m_checkUseColorTheme->IsChecked();

    COLOR_SETTINGS* theme = static_cast<COLOR_SETTINGS*>(
            m_colorTheme->GetClientData( m_colorTheme->GetSelection() ) );

    if( theme && m_checkUseColorTheme->IsChecked() )
        cfg->m_Printing.color_theme = theme->GetFilename();
}


void DIALOG_PRINT_USING_PRINTER::OnPageSetup( wxCommandEvent& event )
{
    wxPageSetupDialog pageSetupDialog( this, &m_parent->GetPageSetupData() );
    pageSetupDialog.ShowModal();

    m_parent->GetPageSetupData() = pageSetupDialog.GetPageSetupDialogData();
}


void DIALOG_PRINT_USING_PRINTER::OnPrintPreview( wxCommandEvent& event )
{
    SavePrintOptions();

    // Pass two printout objects: for preview, and possible printing.
    wxString        title   = _( "Preview" );
    wxPrintPreview* preview = new wxPrintPreview( new SCH_PRINTOUT( m_parent, title ),
                                                  new SCH_PRINTOUT( m_parent, title ),
                                                  &m_parent->GetPageSetupData().GetPrintData() );

    preview->SetZoom( 100 );

    SCH_PREVIEW_FRAME* frame = new SCH_PREVIEW_FRAME( preview, this, title );
    frame->SetMinSize( wxSize( 550, 350 ) );

    // on first invocation in this runtime session, set to 2/3 size of my parent,
    // but will be changed in Show() if not first time as will position.
    frame->SetSize( (m_parent->GetSize() * 2) / 3 );
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
    if( Pgm().m_Printing )
    {
        DisplayError( this, _( "Previous print job not yet complete." ) );
        return false;
    }

    SavePrintOptions();

    int sheet_count = m_parent->Schematic().Root().CountSheets();

    wxPrintDialogData printDialogData( m_parent->GetPageSetupData().GetPrintData() );
    printDialogData.SetMaxPage( sheet_count );

    if( sheet_count > 1 )
        printDialogData.EnablePageNumbers( true );

    wxPrinter printer( &printDialogData );
    SCH_PRINTOUT printout( m_parent, _( "Print Schematic" ) );

    Pgm().m_Printing = true;
    {
        if( !printer.Print( this, &printout, true ) )
        {
            if( wxPrinter::GetLastError() == wxPRINTER_ERROR )
                DisplayError( this, _( "An error occurred attempting to print the schematic." ) );
        }
        else
        {
            m_parent->GetPageSetupData() = printer.GetPrintDialogData().GetPrintData();
        }
    }
    Pgm().m_Printing = false;

    return true;
}


bool SCH_PRINTOUT::OnPrintPage( int page )
{
    SCH_SHEET_LIST sheetList = m_parent->Schematic().GetSheets();

    wxCHECK_MSG( page >= 1 && page <= (int)sheetList.size(), false,
                 wxT( "Cannot print invalid page number." ) );

    wxCHECK_MSG( sheetList[ page - 1].LastScreen() != NULL, false,
                 wxT( "Cannot print page with NULL screen." ) );

    wxString msg;
    msg.Printf( _( "Print page %d" ), page );
    m_parent->SetMsgPanel( msg, wxEmptyString );

    SCH_SCREEN*     screen       = m_parent->GetScreen();
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();
    m_parent->SetCurrentSheet( sheetList[ page - 1 ] );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
    screen = m_parent->GetCurrentSheet().LastScreen();
    PrintPage( screen );
    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();

    return true;
}


void SCH_PRINTOUT::GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo )
{
    *minPage = *selPageFrom = 1;
    *maxPage = *selPageTo   = m_parent->Schematic().Root().CountSheets();
}


bool SCH_PRINTOUT::HasPage( int pageNum )
{
    return m_parent->Schematic().Root().CountSheets() >= pageNum;
}


bool SCH_PRINTOUT::OnBeginDocument( int startPage, int endPage )
{
    if( !wxPrintout::OnBeginDocument( startPage, endPage ) )
        return false;

    return true;
}


/*
 * This is the real print function: print the active screen
 */
void SCH_PRINTOUT::PrintPage( SCH_SCREEN* aScreen )
{
    wxPoint  tmp_startvisu;
    wxSize   pageSizeIU;             // Page size in internal units
    wxPoint  old_org;
    wxRect   fitRect;
    wxDC*    dc = GetDC();

    wxBusyCursor dummy;

    // Save current offsets and clip box.
    tmp_startvisu = aScreen->m_StartVisu;
    old_org = aScreen->m_DrawOrg;

    SETTINGS_MANAGER&  mgr   = Pgm().GetSettingsManager();
    EESCHEMA_SETTINGS* cfg   = m_parent->eeconfig();
    COLOR_SETTINGS*    theme = mgr.GetColorSettings( cfg->m_Printing.color_theme );

    // Change scale factor and offset to print the whole page.
    bool printReference = cfg->m_Printing.title_block;

    pageSizeIU = aScreen->GetPageSettings().GetSizeIU();
    FitThisSizeToPaper( pageSizeIU );

    fitRect = GetLogicalPaperRect();

    // When is the actual paper size does not match the schematic page size, the drawing will
    // not be centered on X or Y axis.  Give a draw offset to center the schematic page on the
    // paper draw area.
    int xoffset = ( fitRect.width - pageSizeIU.x ) / 2;
    int yoffset = ( fitRect.height - pageSizeIU.y ) / 2;

    if( dc->CanUseTransformMatrix() )
    {
        wxAffineMatrix2D matrix = dc->GetTransformMatrix();

        // Check for portrait/landscape mismatch:
        if( ( fitRect.width > fitRect.height ) != ( pageSizeIU.x > pageSizeIU.y ) )
        {
            matrix.Rotate( M_PI_2 );
            xoffset = ( fitRect.height - pageSizeIU.x ) / 2;
            yoffset = ( fitRect.width - pageSizeIU.y ) / 2;
        }

        matrix.Translate( xoffset, yoffset );
        dc->SetTransformMatrix( matrix );
    }
    else
    {
        // wxWidgets appears to have a bug when OffsetLogicalOrigin()'s yoffset changes from
        // page to page.
        // NB: this is a workaround, not a fix.  The Y centering will be off, but this is less
        // annoying than a blank page.  See https://bugs.launchpad.net/kicad/+bug/1464773.
        yoffset = 0;

        OffsetLogicalOrigin( xoffset, yoffset );
    }

    dc->SetLogicalFunction( wxCOPY );
    GRResetPenAndBrush( dc );

    COLOR4D savedBgColor = m_parent->GetDrawBgColor();
    COLOR4D bgColor      = m_parent->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    if( cfg->m_Printing.background )
    {
        if( cfg->m_Printing.use_theme && theme )
            bgColor = theme->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    }
    else
    {
        bgColor = COLOR4D::WHITE;
    }

    m_parent->SetDrawBgColor( bgColor );

    GRSFilledRect( nullptr, dc, fitRect.GetX(), fitRect.GetY(), fitRect.GetRight(),
                   fitRect.GetBottom(), 0, bgColor, bgColor );

    if( cfg->m_Printing.monochrome )
        GRForceBlackPen( true );

    KIGFX::SCH_RENDER_SETTINGS renderSettings( *m_parent->GetRenderSettings() );
    renderSettings.SetPrintDC( dc );

    if( cfg->m_Printing.use_theme && theme )
        renderSettings.LoadColors( theme );

    // The drawing-sheet-item print code is shared between PCBNew and Eeschema, so it's easier
    // if they just use the PCB layer.
    renderSettings.SetLayerColor( LAYER_DRAWINGSHEET,
                                  renderSettings.GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) );

    if( printReference )
    {
        m_parent->PrintDrawingSheet( &renderSettings, aScreen, IU_PER_MILS, aScreen->GetFileName(),
                                     wxEmptyString );
    }

    renderSettings.SetIsPrinting( true );

    aScreen->Print( &renderSettings );

    m_parent->SetDrawBgColor( savedBgColor );

    GRForceBlackPen( false );

    aScreen->m_StartVisu = tmp_startvisu;
    aScreen->m_DrawOrg   = old_org;
}


int InvokeDialogPrintUsingPrinter( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_PRINT_USING_PRINTER dlg( aCaller );

    return dlg.ShowModal();
}
