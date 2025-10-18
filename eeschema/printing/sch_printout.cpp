/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sch_printout.h"
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <sch_edit_frame.h>
#include <math/vector2wx.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <sch_painter.h>

#include <view/view.h>
#include <gal/gal_print.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <zoom_defines.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <string_utils.h>
#include <wx/dcprint.h>
#include <wx/log.h>
#include <wx/dcmemory.h>
#include <wx/log.h>


SCH_PRINTOUT::SCH_PRINTOUT( SCH_EDIT_FRAME* aParent, const wxString& aTitle ) :
        wxPrintout( aTitle )
{
    wxASSERT( aParent != nullptr );
    m_parent = aParent;
    m_view = nullptr;
}


void SCH_PRINTOUT::GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo )
{
    *minPage = *selPageFrom = 1;
    *maxPage = *selPageTo   = m_parent->Schematic().Root().CountActiveSheets();
}


bool SCH_PRINTOUT::HasPage( int pageNum )
{
    return m_parent->Schematic().Root().CountActiveSheets() >= pageNum;
}


bool SCH_PRINTOUT::OnBeginDocument( int startPage, int endPage )
{
    if( !wxPrintout::OnBeginDocument( startPage, endPage ) )
        return false;

    return true;
}


bool SCH_PRINTOUT::OnPrintPage( int page )
{
    SCH_SHEET_LIST sheetList = m_parent->Schematic().Hierarchy();
    sheetList.SortByPageNumbers( false );

    wxCHECK_MSG( page >= 1 && page <= (int)sheetList.size(), false,
                 wxT( "Cannot print invalid page number." ) );

    wxCHECK_MSG( sheetList[ page - 1].LastScreen() != nullptr, false,
                 wxT( "Cannot print page with NULL screen." ) );

    wxString msg;
    msg.Printf( _( "Print page %d" ), page );
    m_parent->SetMsgPanel( msg, wxEmptyString );

    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();

    // Switch to the new current sheet
    m_parent->SetCurrentSheet( sheetList[ page - 1 ] );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
    m_parent->RecomputeIntersheetRefs();
    SCH_SCREEN* screen = m_parent->GetCurrentSheet().LastScreen();
    // Ensure the displayed page number is updated:
    KIGFX::SCH_VIEW* sch_view = m_parent->GetCanvas()->GetView();
    sch_view->GetDrawingSheet()->SetPageNumber( TO_UTF8( screen->GetPageNumber() ) );
    sch_view->GetDrawingSheet()->SetIsFirstPage( screen->GetVirtualPageNumber() == 1 );

    // Print page using the current wxPrinterDC
    PrintPage( screen, GetDC(), true );

    // Restore the initial current sheet
    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
    screen = m_parent->GetCurrentSheet().LastScreen();
    sch_view->GetDrawingSheet()->SetPageNumber( TO_UTF8( screen->GetPageNumber() ) );
    sch_view->GetDrawingSheet()->SetIsFirstPage( screen->GetVirtualPageNumber() == 1 );

    return true;
}


int SCH_PRINTOUT::milsToIU( int aMils )
{
    return KiROUND( aMils * schIUScale.IU_PER_MILS );
}


/*
 * This is the real print function: print the active screen
 */
bool SCH_PRINTOUT::PrintPage( SCH_SCREEN* aScreen, wxDC* aDC, bool aForPrinting )
{
    // Note: some data (like paper size) is available only when printing
    wxDC* dc = aDC;
    m_view = m_parent->GetCanvas()->GetView();
    KIGFX::GAL_DISPLAY_OPTIONS options;
    options.antialiasing_mode = KIGFX::GAL_ANTIALIASING_MODE::AA_HIGHQUALITY;
    std::unique_ptr<KIGFX::GAL_PRINT> galPrint = KIGFX::GAL_PRINT::Create( options, dc );
    KIGFX::GAL* gal = galPrint->GetGAL();
    KIGFX::PRINT_CONTEXT* printCtx = galPrint->GetPrintCtx();
    std::unique_ptr<KIGFX::SCH_PAINTER> painter = std::make_unique<KIGFX::SCH_PAINTER>( gal );
    std::unique_ptr<KIGFX::VIEW> view( m_view->DataReference() );

    painter->SetSchematic( &m_parent->Schematic() );

    EESCHEMA_SETTINGS*  cfg = m_parent->eeconfig();
    COLOR_SETTINGS*     cs = ::GetColorSettings( cfg ? cfg->m_Printing.color_theme : DEFAULT_THEME );
    SCH_SELECTION_TOOL* selTool = m_parent->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();

    // Target paper size
    wxRect pageSizePix;
    wxSize dcPPI = dc->GetPPI();

    if( aForPrinting )
    {
        pageSizePix = GetLogicalPageRect();
    }
    else
    {
        dc->SetUserScale( 1, 1 );

        if( wxMemoryDC* memdc = dynamic_cast<wxMemoryDC*>( dc ) )
        {
            wxBitmap& bm = memdc->GetSelectedBitmap();
            pageSizePix = wxRect( 0, 0, bm.GetWidth(), bm.GetHeight() );
        }
        else
        {
            return false;
        }
    }

    const VECTOR2D pageSizeIn( (double) pageSizePix.width / dcPPI.x,
                               (double) pageSizePix.height / dcPPI.y );
    const VECTOR2D pageSizeIU( milsToIU( pageSizeIn.x * 1000 ), milsToIU( pageSizeIn.y * 1000 ) );

    galPrint->SetSheetSize( pageSizeIn );

    view->SetGAL( gal );
    view->SetPainter( painter.get() );
    view->SetScaleLimits( ZOOM_MAX_LIMIT_EESCHEMA, ZOOM_MIN_LIMIT_EESCHEMA );
    view->SetScale( 1.0 );
    gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    // Init the SCH_RENDER_SETTINGS used by the painter used to print schematic
    SCH_RENDER_SETTINGS* dstSettings = painter->GetSettings();

    if( aForPrinting )
        *dstSettings = *m_parent->GetRenderSettings();

    dstSettings->m_ShowPinsElectricalType = false;

    // Set the color scheme
    dstSettings->LoadColors( m_parent->GetColorSettings( false ) );

    if( cfg && cfg->m_Printing.use_theme )
        dstSettings->LoadColors( cs );

    bool printDrawingSheet = cfg ? cfg->m_Printing.title_block : true;

    COLOR4D bgColor = m_parent->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    if( cfg && cfg->m_Printing.background )
    {
        if( cfg->m_Printing.use_theme )
            bgColor = cs->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    }
    else
    {
        bgColor = COLOR4D::WHITE;
    }

    dstSettings->SetBackgroundColor( bgColor );

    // The drawing-sheet-item print code is shared between PCBNew and Eeschema, so it's easier
    // if they just use the PCB layer.
    dstSettings->SetLayerColor( LAYER_DRAWINGSHEET,
                               dstSettings->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) );

    dstSettings->SetDefaultFont( cfg ? cfg->m_Appearance.default_font : wxString( "" ) );

    if( cfg && cfg->m_Printing.monochrome )
    {
        for( int i = 0; i < LAYER_ID_COUNT; ++i )
            dstSettings->SetLayerColor( i, COLOR4D::BLACK );

        // In B&W mode, draw the background only in white, because any other color
        // will be replaced by a black background
        dstSettings->SetBackgroundColor( COLOR4D::WHITE );
        dstSettings->m_OverrideItemColors = true;

        // Disable print some backgrounds
        dstSettings->SetPrintBlackAndWhite( true );
    }
    else // color enabled
    {
        for( int i = 0; i < LAYER_ID_COUNT; ++i )
        {
            // Cairo does not support translucent colors on PostScript surfaces
            // see 'Features support by the PostScript surface' on
            // https://www.cairographics.org/documentation/using_the_postscript_surface/
            dstSettings->SetLayerColor( i, dstSettings->GetLayerColor( i ).WithAlpha( 1.0 ) );
        }
    }

    dstSettings->SetIsPrinting( true );

    VECTOR2I sheetSizeIU = aScreen->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS );
    BOX2I    drawingAreaBBox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( sheetSizeIU ) );

    // Enable all layers and use KIGFX::TARGET_NONCACHED to force update drawings
    // for printing with current GAL instance
    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; ++i )
    {
        view->SetLayerVisible( i, true );
        view->SetLayerTarget( i, KIGFX::TARGET_NONCACHED );
    }

    view->SetLayerVisible( LAYER_DRAWINGSHEET, printDrawingSheet );

    // Don't draw the selection if it's not from the current screen
    for( EDA_ITEM* item : selTool->GetSelection() )
    {
        if( SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item ) )
        {
            if( !m_parent->GetScreen()->CheckIfOnDrawList( schItem ) )
                view->SetLayerVisible( LAYER_SELECT_OVERLAY, false );

            break;
        }
    }

    // When is the actual paper size does not match the schematic page size,
    // we need to adjust the print scale to fit the selected paper size (pageSizeIU)
    double scaleX = (double) pageSizeIU.x / drawingAreaBBox.GetWidth();
    double scaleY = (double) pageSizeIU.y / drawingAreaBBox.GetHeight();

    double print_scale = std::min( scaleX, scaleY );

    galPrint->SetNativePaperSize( pageSizeIn, printCtx->HasNativeLandscapeRotation() );
    gal->SetLookAtPoint( drawingAreaBBox.Centre() );
    gal->SetZoomFactor( print_scale );
    gal->SetClearColor( dstSettings->GetBackgroundColor() );

// Clearing the screen for the background color needs the screen set to the page size
// in pixels.  This can ?somehow? prevent some but not all foreground elements from being printed
// TODO: figure out what's going on here and fix printing.  See also board_printout
    VECTOR2I size = gal->GetScreenPixelSize();
    gal->ResizeScreen( pageSizePix.GetWidth(),pageSizePix.GetHeight() );
    gal->ClearScreen();
    gal->ResizeScreen( size.x, size.y );

    // Needed to use the same order for printing as for screen redraw
    view->UseDrawPriority( true );

    {
        KIGFX::GAL_DRAWING_CONTEXT ctx( gal );
        view->Redraw();
    }

    return true;
}
