/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2022 KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <math/vector2wx.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <sch_sheet_path.h>
#include <dialog_print_using_printer_base.h>
#include <sch_painter.h>
#include <wx/print.h>
#include <wx/printdlg.h>


class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_BASE
{
public:
    DIALOG_PRINT_USING_PRINTER( SCH_EDIT_FRAME* aParent );
    ~DIALOG_PRINT_USING_PRINTER() override;

protected:
    void OnOutputChoice( wxCommandEvent& event ) override;
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
        wxASSERT( aParent != nullptr );
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

    SetupStandardButtons( { { wxID_OK,     _( "Print" )         },
                            { wxID_APPLY,  _( "Print Preview" ) },
                            { wxID_CANCEL, _( "Close" )         } } );

#ifdef __WXMAC__
    // Problems with modal on wx-2.9 - Anyway preview is standard for OSX
    m_sdbSizer1Apply->Hide();
#endif

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

    if( cfg->m_Printing.monochrome )
    {
        m_checkBackgroundColor->SetValue( false );
        m_checkBackgroundColor->Enable( false );
    }

    m_checkReference->SetValue( cfg->m_Printing.title_block );
    m_colorPrint->SetSelection( cfg->m_Printing.monochrome ? 1 : 0 );
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
            pageSetupDialogData.SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ),
                                                      EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ) ) );
        else
            pageSetupDialogData.SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ),
                                                      EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ) ) );
    }

    pageSetupDialogData.GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

    Layout();

    return true;
}


void DIALOG_PRINT_USING_PRINTER::OnUseColorThemeChecked( wxCommandEvent& event )
{
    m_colorTheme->Enable( m_checkUseColorTheme->GetValue() );
}


void DIALOG_PRINT_USING_PRINTER::OnOutputChoice( wxCommandEvent& event )
{
    long sel = event.GetSelection();
    m_checkBackgroundColor->Enable( sel == 0 );

    if( sel )
        m_checkBackgroundColor->SetValue( false );
    else
        m_checkBackgroundColor->SetValue( m_parent->eeconfig()->m_Printing.background );
}


void DIALOG_PRINT_USING_PRINTER::SavePrintOptions()
{
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    cfg->m_Printing.monochrome  = !!m_colorPrint->GetSelection();
    cfg->m_Printing.title_block = m_checkReference->IsChecked();

    if( m_checkBackgroundColor->IsEnabled() )
        cfg->m_Printing.background = m_checkBackgroundColor->IsChecked();
    else
        cfg->m_Printing.background = false;

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

    // on first invocation in this runtime session, set to 3/4 size of parent,
    // but will be changed in Show() if not first time as will position.
    // Must be called after InitializeWithModality because otherwise in some wxWidget
    // versions it is not always taken in account
    frame->SetMinSize( wxSize( 650, 500 ) );
    frame->SetSize( (m_parent->GetSize() * 3) / 4 );

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

    wxCHECK_MSG( sheetList[ page - 1].LastScreen() != nullptr, false,
                 wxT( "Cannot print page with NULL screen." ) );

    wxString msg;
    msg.Printf( _( "Print page %d" ), page );
    m_parent->SetMsgPanel( msg, wxEmptyString );

    SCH_SCREEN*     screen       = m_parent->GetScreen();
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();
    m_parent->SetCurrentSheet( sheetList[ page - 1 ] );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
    m_parent->RecomputeIntersheetRefs();
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
    // Warning:
    // When printing many pages, changes in the current wxDC will affect all next printings
    // because all prints are using the same wxPrinterDC after creation
    // So be careful and reinit parameters, especially when using offsets.

    VECTOR2I tmp_startvisu;
    wxSize   pageSizeIU;             // Page size in internal units
    VECTOR2I old_org;
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
    bool printDrawingSheet = cfg->m_Printing.title_block;

    pageSizeIU = ToWxSize( aScreen->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS ) );
    FitThisSizeToPaper( pageSizeIU );

    fitRect = GetLogicalPaperRect();

    // When is the actual paper size does not match the schematic page size, the drawing will
    // not be centered on X or Y axis.  Give a draw offset to center the schematic page on the
    // paper draw area.
    int xoffset = ( fitRect.width - pageSizeIU.x ) / 2;
    int yoffset = ( fitRect.height - pageSizeIU.y ) / 2;

    // Using a wxAffineMatrix2D has a big advantage: it handles different pages orientations
    //(PORTRAIT/LANDSCAPE), but the affine matrix is not always supported
    if( dc->CanUseTransformMatrix() )
    {
        wxAffineMatrix2D matrix;    // starts from a unity matrix (the current wxDC default)

        // Check for portrait/landscape mismatch:
        if( ( fitRect.width > fitRect.height ) != ( pageSizeIU.x > pageSizeIU.y ) )
        {
            // Rotate the coordinates, and keep the draw coordinates inside the page
            matrix.Rotate( M_PI_2 );
            matrix.Translate( 0, -pageSizeIU.y );

            // Recalculate the offsets and page sizes according to the page rotation
            std::swap( pageSizeIU.x, pageSizeIU.y );
            FitThisSizeToPaper( pageSizeIU );
            fitRect = GetLogicalPaperRect();

            xoffset = ( fitRect.width - pageSizeIU.x ) / 2;
            yoffset = ( fitRect.height - pageSizeIU.y ) / 2;

            // All the coordinates will be rotated 90 deg when printing,
            // so the X,Y offset vector must be rotated -90 deg before printing
            std::swap( xoffset, yoffset );
            std::swap( fitRect.width, fitRect.height );
            yoffset = -yoffset;
        }

        matrix.Translate( xoffset, yoffset );
        dc->SetTransformMatrix( matrix );

        fitRect.x -= xoffset;
        fitRect.y -= yoffset;
    }
    else
    {
        SetLogicalOrigin( 0, 0 );   // Reset all offset settings made previously.
                                    // When printing previous pages (all prints are using the same wxDC)
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

    GRSFilledRect( dc, fitRect.GetX(), fitRect.GetY(), fitRect.GetRight(), fitRect.GetBottom(), 0,
                   bgColor, bgColor );

    if( cfg->m_Printing.monochrome )
        GRForceBlackPen( true );

    KIGFX::SCH_RENDER_SETTINGS renderSettings( *m_parent->GetRenderSettings() );
    renderSettings.SetPrintDC( dc );

    if( cfg->m_Printing.use_theme && theme )
        renderSettings.LoadColors( theme );

    renderSettings.SetBackgroundColor( bgColor );

    // The drawing-sheet-item print code is shared between PCBNew and Eeschema, so it's easier
    // if they just use the PCB layer.
    renderSettings.SetLayerColor( LAYER_DRAWINGSHEET,
                                  renderSettings.GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) );

    renderSettings.SetDefaultFont( cfg->m_Appearance.default_font );

    if( printDrawingSheet )
    {
        m_parent->PrintDrawingSheet( &renderSettings, aScreen, aScreen->Schematic()->GetProperties(),
                                     schIUScale.IU_PER_MILS, aScreen->GetFileName(), wxEmptyString );
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
