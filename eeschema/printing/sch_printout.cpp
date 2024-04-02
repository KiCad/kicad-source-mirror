/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/ee_selection_tool.h>
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


SCH_PRINTOUT::SCH_PRINTOUT( SCH_EDIT_FRAME* aParent, const wxString& aTitle, bool aUseCairo ) :
        wxPrintout( aTitle )
{
    wxASSERT( aParent != nullptr );
    m_parent = aParent;
    m_useCairo = aUseCairo;
    m_view = nullptr;
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


int SCH_PRINTOUT::milsToIU( int aMils )
{
    return KiROUND( aMils * schIUScale.IU_PER_MILS );
}

/*
 * This is the real print function: print the active screen
 */
void SCH_PRINTOUT::PrintPage( SCH_SCREEN* aScreen )
{
    if( !m_useCairo )
    {
        // Version using print to a wxDC
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

        SCH_RENDER_SETTINGS renderSettings( *m_parent->GetRenderSettings() );
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
    else
    {
        wxDC* dc = GetDC();
        m_view = m_parent->GetCanvas()->GetView();
        KIGFX::GAL_DISPLAY_OPTIONS options;
        options.cairo_antialiasing_mode = KIGFX::CAIRO_ANTIALIASING_MODE::GOOD;
        std::unique_ptr<KIGFX::GAL_PRINT> galPrint = KIGFX::GAL_PRINT::Create( options, dc );
        KIGFX::GAL* gal = galPrint->GetGAL();
        KIGFX::PRINT_CONTEXT* printCtx = galPrint->GetPrintCtx();
        std::unique_ptr<KIGFX::SCH_PAINTER> painter = std::make_unique<KIGFX::SCH_PAINTER>( gal );
        std::unique_ptr<KIGFX::VIEW> view( m_view->DataReference() );

        painter->SetSchematic( &m_parent->Schematic() );

        SETTINGS_MANAGER&  mgr   = Pgm().GetSettingsManager();
        EESCHEMA_SETTINGS* cfg   = m_parent->eeconfig();
        COLOR_SETTINGS*    theme = mgr.GetColorSettings( cfg->m_Printing.color_theme );
        EE_SELECTION_TOOL* selTool = m_parent->GetToolManager()->GetTool<EE_SELECTION_TOOL>();

        // Target paper size
        wxRect         pageSizePx = GetLogicalPageRect();
        const VECTOR2D pageSizeIn( (double) pageSizePx.width / dc->GetPPI().x,
                                   (double) pageSizePx.height / dc->GetPPI().y );
        const VECTOR2D pageSizeIU( milsToIU( pageSizeIn.x * 1000 ), milsToIU( pageSizeIn.y * 1000 ) );

        galPrint->SetSheetSize( pageSizeIn );

        view->SetGAL( gal );
        view->SetPainter( painter.get() );
        view->SetScaleLimits( ZOOM_MAX_LIMIT_EESCHEMA, ZOOM_MIN_LIMIT_EESCHEMA );
        view->SetScale( 1.0 );
        gal->SetWorldUnitLength( SCH_WORLD_UNIT );

        // Init the SCH_RENDER_SETTINGS used by the painter used to print schematic
        SCH_RENDER_SETTINGS* dstSettings = painter->GetSettings();

        dstSettings->m_ShowPinsElectricalType = false;

        // Set the color scheme
        dstSettings->LoadColors( m_parent->GetColorSettings( false ) );

        if( cfg->m_Printing.use_theme && theme )
            dstSettings->LoadColors( theme );

        bool printDrawingSheet = cfg->m_Printing.title_block;

        COLOR4D bgColor = m_parent->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );

        if( cfg->m_Printing.background )
        {
            if( cfg->m_Printing.use_theme && theme )
                bgColor = theme->GetColor( LAYER_SCHEMATIC_BACKGROUND );
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

        dstSettings->SetDefaultFont( cfg->m_Appearance.default_font );

        if( cfg->m_Printing.monochrome )
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
        gal->ClearScreen();

        // Needed to use the same order for printing as for screen redraw
        view->UseDrawPriority( true );

        {
            KIGFX::GAL_DRAWING_CONTEXT ctx( gal );
            view->Redraw();
        }
    }
}
