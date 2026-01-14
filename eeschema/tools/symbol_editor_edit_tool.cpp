/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "symbol_editor_edit_tool.h"

#include <tool/picker_tool.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_tool_utils.h>
#include <tools/symbol_editor_pin_tool.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <tools/symbol_editor_move_tool.h>
#include <clipboard.h>
#include <sch_actions.h>
#include <increment.h>
#include <pin_layout_cache.h>
#include <string_utils.h>
#include <symbol_edit_frame.h>
#include <sch_commit.h>
#include <dialogs/dialog_shape_properties.h>
#include <dialogs/dialog_text_properties.h>
#include <dialogs/dialog_field_properties.h>
#include <dialogs/dialog_lib_symbol_properties.h>
#include <dialogs/dialog_lib_edit_pin_table.h>
#include <dialogs/dialog_update_symbol_fields.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <richio.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_textbox.h>
#include <lib_symbol_library_manager.h>
#include <wx/textdlg.h>     // for wxTextEntryDialog
#include <math/util.h>      // for KiROUND
#include <io/kicad/kicad_io_utils.h>
#include <trace_helpers.h>
#include <plotters/plotters_pslike.h>
#include <sch_painter.h>
#include <sch_plotter.h>
#include <locale_io.h>
#include <gal/gal_print.h>
#include <gal/graphics_abstraction_layer.h>
#include <zoom_defines.h>
#include <wx/ffile.h>
#include <wx/mstream.h>
#include <wx/dcmemory.h>


namespace
{
constexpr double clipboardPpi = 96.0;
constexpr int    clipboardMaxBitmapSize = 4096;
constexpr double clipboardBboxInflation = 0.02;


void appendMimeData( std::vector<CLIPBOARD_MIME_DATA>& aMimeData, const wxString& aMimeType,
                     const wxMemoryBuffer& aBuffer )
{
    if( aBuffer.GetDataLen() == 0 )
        return;

    CLIPBOARD_MIME_DATA entry;
    entry.m_mimeType = aMimeType;
    entry.m_data = aBuffer;
    aMimeData.push_back( entry );
}


void appendMimeData( std::vector<CLIPBOARD_MIME_DATA>& aMimeData, const wxString& aMimeType,
                     wxImage&& aImage )
{
    if( !aImage.IsOk() )
        return;

    CLIPBOARD_MIME_DATA entry;
    entry.m_mimeType = aMimeType;
    entry.m_image = std::move( aImage );
    aMimeData.push_back( std::move( entry ) );
}


bool loadFileToBuffer( const wxString& aFileName, wxMemoryBuffer& aBuffer )
{
    wxFFile file( aFileName, wxS( "rb" ) );

    if( !file.IsOpened() )
        return false;

    wxFileOffset size = file.Length();

    if( size <= 0 )
        return false;

    void* data = aBuffer.GetWriteBuf( size );

    if( file.Read( data, size ) != static_cast<size_t>( size ) )
    {
        aBuffer.UngetWriteBuf( 0 );
        return false;
    }

    aBuffer.UngetWriteBuf( size );
    return true;
}


bool plotSymbolToSvg( SYMBOL_EDIT_FRAME* aFrame, LIB_SYMBOL* aSymbol, const BOX2I& aBBox,
                      int aUnit, int aBodyStyle, wxMemoryBuffer& aBuffer )
{
    if( !aSymbol )
        return false;

    SCH_RENDER_SETTINGS renderSettings;
    renderSettings.LoadColors( aFrame->GetColorSettings() );
    renderSettings.SetDefaultPenWidth( aFrame->GetRenderSettings()->GetDefaultPenWidth() );

    std::unique_ptr<SVG_PLOTTER> plotter = std::make_unique<SVG_PLOTTER>();
    plotter->SetRenderSettings( &renderSettings );

    PAGE_INFO pageInfo = aFrame->GetScreen()->GetPageSettings();
    pageInfo.SetWidthMils( schIUScale.IUToMils( aBBox.GetWidth() ) );
    pageInfo.SetHeightMils( schIUScale.IUToMils( aBBox.GetHeight() ) );

    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( true );

    VECTOR2I plot_offset = aBBox.GetOrigin();
    plotter->SetViewport( plot_offset, schIUScale.IU_PER_MILS / 10, 1.0, false );
    plotter->SetCreator( wxT( "Eeschema-SVG" ) );

    wxFileName tempFile( wxFileName::CreateTempFileName( wxS( "kicad_symbol_svg" ) ) );

    if( !plotter->OpenFile( tempFile.GetFullPath() ) )
    {
        wxRemoveFile( tempFile.GetFullPath() );
        return false;
    }

    LOCALE_IO     toggle;
    SCH_PLOT_OPTS plotOpts;

    plotter->StartPlot( wxT( "1" ) );

    constexpr bool background = true;
    aSymbol->Plot( plotter.get(), background, plotOpts, aUnit, aBodyStyle, VECTOR2I( 0, 0 ), false );
    aSymbol->Plot( plotter.get(), !background, plotOpts, aUnit, aBodyStyle, VECTOR2I( 0, 0 ), false );
    aSymbol->PlotFields( plotter.get(), !background, plotOpts, aUnit, aBodyStyle, VECTOR2I( 0, 0 ), false );

    plotter->EndPlot();
    plotter.reset();

    bool ok = loadFileToBuffer( tempFile.GetFullPath(), aBuffer );
    wxRemoveFile( tempFile.GetFullPath() );
    return ok;
}


wxImage renderSymbolToBitmap( SYMBOL_EDIT_FRAME* aFrame, LIB_SYMBOL* aSymbol, const BOX2I& aBBox,
                              int aUnit, int aBodyStyle, int aWidth, int aHeight,
                              double aViewScale, const wxColour& aBgColor )
{
    if( !aSymbol )
        return wxImage();

    wxBitmap bitmap( aWidth, aHeight, 24 );
    wxMemoryDC dc;
    dc.SelectObject( bitmap );
    dc.SetBackground( wxBrush( aBgColor ) );
    dc.Clear();

    KIGFX::GAL_DISPLAY_OPTIONS options;
    options.antialiasing_mode = KIGFX::GAL_ANTIALIASING_MODE::AA_HIGHQUALITY;
    std::unique_ptr<KIGFX::GAL_PRINT> galPrint = KIGFX::GAL_PRINT::Create( options, &dc );

    if( !galPrint )
        return wxImage();

    KIGFX::GAL* gal = galPrint->GetGAL();
    KIGFX::PRINT_CONTEXT* printCtx = galPrint->GetPrintCtx();
    std::unique_ptr<KIGFX::SCH_PAINTER> painter = std::make_unique<KIGFX::SCH_PAINTER>( gal );
    std::unique_ptr<KIGFX::VIEW> view = std::make_unique<KIGFX::VIEW>();

    // For symbol editor, we don't have a full schematic context
    // but SCH_PAINTER can still work for rendering individual items
    view->SetGAL( gal );
    view->SetPainter( painter.get() );
    view->SetScaleLimits( ZOOM_MAX_LIMIT_EESCHEMA, ZOOM_MIN_LIMIT_EESCHEMA );
    view->SetScale( 1.0 );
    gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    // Clone items and add to view
    std::vector<std::unique_ptr<SCH_ITEM>> clonedItems;

    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( aUnit && item.GetUnit() && item.GetUnit() != aUnit )
            continue;

        if( aBodyStyle && item.GetBodyStyle() && item.GetBodyStyle() != aBodyStyle )
            continue;

        SCH_ITEM* clone = static_cast<SCH_ITEM*>( item.Clone() );
        clonedItems.emplace_back( clone );
        view->Add( clone );
    }

    SCH_RENDER_SETTINGS* dstSettings = painter->GetSettings();
    dstSettings->LoadColors( aFrame->GetColorSettings() );
    dstSettings->SetDefaultPenWidth( aFrame->GetRenderSettings()->GetDefaultPenWidth() );
    dstSettings->SetIsPrinting( true );

    COLOR4D bgColor4D( aBgColor.Red() / 255.0, aBgColor.Green() / 255.0,
                       aBgColor.Blue() / 255.0, 1.0 );
    dstSettings->SetBackgroundColor( bgColor4D );

    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; ++i )
    {
        view->SetLayerVisible( i, true );
        view->SetLayerTarget( i, KIGFX::TARGET_NONCACHED );
    }

    view->SetLayerVisible( LAYER_DRAWINGSHEET, false );

    double ppi = clipboardPpi;
    double inch2Iu = 1000.0 * schIUScale.IU_PER_MILS;
    VECTOR2D pageSizeIn( (double) aWidth / ppi, (double) aHeight / ppi );

    galPrint->SetSheetSize( pageSizeIn );
    galPrint->SetNativePaperSize( pageSizeIn, printCtx->HasNativeLandscapeRotation() );

    double zoomFactor = aViewScale * inch2Iu / ppi;

    gal->SetLookAtPoint( aBBox.Centre() );
    gal->SetZoomFactor( zoomFactor );
    gal->SetClearColor( bgColor4D );
    gal->ClearScreen();

    view->UseDrawPriority( true );

    {
        KIGFX::GAL_DRAWING_CONTEXT ctx( gal );
        view->Redraw();
    }

    dc.SelectObject( wxNullBitmap );
    return bitmap.ConvertToImage();
}


wxImage renderSymbolToImageWithAlpha( SYMBOL_EDIT_FRAME* aFrame, LIB_SYMBOL* aSymbol,
                                      const BOX2I& aBBox, int aUnit, int aBodyStyle )
{
    if( !aSymbol )
        return wxImage();

    VECTOR2I size = aBBox.GetSize();

    if( size.x <= 0 || size.y <= 0 )
        return wxImage();

    // Use the current view scale to match what the user sees on screen
    double viewScale = aFrame->GetCanvas()->GetView()->GetScale();
    int    bitmapWidth = KiROUND( size.x * viewScale );
    int    bitmapHeight = KiROUND( size.y * viewScale );

    // Clamp to maximum size while preserving aspect ratio
    if( bitmapWidth > clipboardMaxBitmapSize || bitmapHeight > clipboardMaxBitmapSize )
    {
        double scaleDown = (double) clipboardMaxBitmapSize / std::max( bitmapWidth, bitmapHeight );
        bitmapWidth = KiROUND( bitmapWidth * scaleDown );
        bitmapHeight = KiROUND( bitmapHeight * scaleDown );
        viewScale *= scaleDown;
    }

    if( bitmapWidth <= 0 || bitmapHeight <= 0 )
        return wxImage();

    // Render twice with different backgrounds for alpha computation
    wxImage imageOnWhite = renderSymbolToBitmap( aFrame, aSymbol, aBBox, aUnit, aBodyStyle,
                                                  bitmapWidth, bitmapHeight, viewScale, *wxWHITE );
    wxImage imageOnBlack = renderSymbolToBitmap( aFrame, aSymbol, aBBox, aUnit, aBodyStyle,
                                                  bitmapWidth, bitmapHeight, viewScale, *wxBLACK );

    if( !imageOnWhite.IsOk() || !imageOnBlack.IsOk() )
        return wxImage();

    // Create output image with alpha channel
    wxImage result( bitmapWidth, bitmapHeight );
    result.InitAlpha();

    unsigned char* rgbWhite = imageOnWhite.GetData();
    unsigned char* rgbBlack = imageOnBlack.GetData();
    unsigned char* rgbResult = result.GetData();
    unsigned char* alphaResult = result.GetAlpha();

    int pixelCount = bitmapWidth * bitmapHeight;

    for( int i = 0; i < pixelCount; ++i )
    {
        int idx = i * 3;

        int rW = rgbWhite[idx], gW = rgbWhite[idx + 1], bW = rgbWhite[idx + 2];
        int rB = rgbBlack[idx], gB = rgbBlack[idx + 1], bB = rgbBlack[idx + 2];

        // Alpha computation: α = 1 - (white - black) / 255
        int diffR = rW - rB;
        int diffG = gW - gB;
        int diffB = bW - bB;
        int avgDiff = ( diffR + diffG + diffB ) / 3;

        int alpha = 255 - avgDiff;
        alpha = std::max( 0, std::min( 255, alpha ) );
        alphaResult[i] = static_cast<unsigned char>( alpha );

        if( alpha > 0 )
        {
            rgbResult[idx] = static_cast<unsigned char>( std::min( 255, rB * 255 / alpha ) );
            rgbResult[idx + 1] = static_cast<unsigned char>( std::min( 255, gB * 255 / alpha ) );
            rgbResult[idx + 2] = static_cast<unsigned char>( std::min( 255, bB * 255 / alpha ) );
        }
        else
        {
            rgbResult[idx] = 0;
            rgbResult[idx + 1] = 0;
            rgbResult[idx + 2] = 0;
        }
    }

    return result;
}

}  // namespace


SYMBOL_EDITOR_EDIT_TOOL::SYMBOL_EDITOR_EDIT_TOOL() :
        SCH_TOOL_BASE( "eeschema.SymbolEditTool" )
{
}


const std::vector<KICAD_T> SYMBOL_EDITOR_EDIT_TOOL::SwappableItems = {
    LIB_SYMBOL_T, // Allows swapping the anchor
    SCH_PIN_T,
    SCH_SHAPE_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_FIELD_T,
};


bool SYMBOL_EDITOR_EDIT_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    SYMBOL_EDITOR_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
    SYMBOL_EDITOR_MOVE_TOOL*     moveTool = m_toolMgr->GetTool<SYMBOL_EDITOR_MOVE_TOOL>();

    wxASSERT_MSG( drawingTools, "eeschema.SymbolDrawing tool is not available" );

    auto haveSymbolCondition =
            [&]( const SELECTION& sel )
            {
                return m_isSymbolEditor && m_frame->GetCurSymbol();
            };

    auto canEdit =
            [&]( const SELECTION& sel )
            {
                if( !m_frame->IsSymbolEditable() )
                    return false;

                if( m_frame->IsSymbolAlias() )
                {
                    for( EDA_ITEM* item : sel )
                    {
                        if( item->Type() != SCH_FIELD_T )
                            return false;
                    }
                }

                return true;
            };

    auto swapSelectionCondition =
            canEdit && SCH_CONDITIONS::OnlyTypes( SwappableItems ) && SELECTION_CONDITIONS::MoreThan( 1 );

    const auto canCopyText = SCH_CONDITIONS::OnlyTypes( {
            SCH_TEXT_T,
            SCH_TEXTBOX_T,
            SCH_FIELD_T,
            SCH_PIN_T,
            SCH_TABLE_T,
            SCH_TABLECELL_T,
    } );

    const auto canConvertStackedPins =
            [&]( const SELECTION& sel )
            {
                // If multiple pins are selected, check they are all at same location
                if( sel.Size() >= 2 )
                {
                    std::vector<SCH_PIN*> pins;
                    for( EDA_ITEM* item : sel )
                    {
                        if( item->Type() != SCH_PIN_T )
                            return false;
                        pins.push_back( static_cast<SCH_PIN*>( item ) );
                    }

                    // Check that all pins are at the same location
                    VECTOR2I pos = pins[0]->GetPosition();
                    for( size_t i = 1; i < pins.size(); ++i )
                    {
                        if( pins[i]->GetPosition() != pos )
                            return false;
                    }
                    return true;
                }

                // If single pin is selected, check if there are other pins at same location
                if( sel.Size() == 1 && sel.Front()->Type() == SCH_PIN_T )
                {
                    SCH_PIN* selectedPin = static_cast<SCH_PIN*>( sel.Front() );
                    VECTOR2I pos = selectedPin->GetPosition();

                    // Get the symbol and check for other pins at same location
                    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();
                    if( !symbol )
                        return false;

                    int coLocatedCount = 0;

                    for( SCH_PIN* pin : symbol->GetPins() )
                    {
                        if( pin->GetPosition() == pos )
                        {
                            coLocatedCount++;

                            if( coLocatedCount >= 2 )
                                return true;
                        }
                    }
                }

                return false;
            };

    const auto canExplodeStackedPin =
            [&]( const SELECTION& sel )
            {
                if( sel.Size() != 1 || sel.Front()->Type() != SCH_PIN_T )
                    return false;

                SCH_PIN* pin = static_cast<SCH_PIN*>( sel.Front() );
                bool isValid;
                std::vector<wxString> stackedNumbers = pin->GetStackedPinNumbers( &isValid );
                return isValid && stackedNumbers.size() > 1;
            };

    // clang-format off
    // Add edit actions to the move tool menu
    if( moveTool )
    {
        CONDITIONAL_MENU& moveMenu = moveTool->GetToolMenu().GetMenu();

        moveMenu.AddSeparator( 200 );
        moveMenu.AddItem( SCH_ACTIONS::rotateCCW,   canEdit && SCH_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( SCH_ACTIONS::rotateCW,    canEdit && SCH_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( SCH_ACTIONS::mirrorV,     canEdit && SCH_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( SCH_ACTIONS::mirrorH,     canEdit && SCH_CONDITIONS::NotEmpty, 200 );

        moveMenu.AddItem( SCH_ACTIONS::swap,        swapSelectionCondition, 200 );
        moveMenu.AddItem( SCH_ACTIONS::properties,  canEdit && SCH_CONDITIONS::Count( 1 ), 200 );

        moveMenu.AddSeparator( 300 );
        moveMenu.AddItem( ACTIONS::cut,             SCH_CONDITIONS::IdleSelection, 300 );
        moveMenu.AddItem( ACTIONS::copy,            SCH_CONDITIONS::IdleSelection, 300 );
        moveMenu.AddItem( ACTIONS::copyAsText,      canCopyText && SCH_CONDITIONS::IdleSelection, 300 );
        moveMenu.AddItem( ACTIONS::duplicate,       canEdit && SCH_CONDITIONS::NotEmpty, 300 );
        moveMenu.AddItem( ACTIONS::doDelete,        canEdit && SCH_CONDITIONS::NotEmpty, 200 );

        moveMenu.AddSeparator( 400 );
        moveMenu.AddItem( ACTIONS::selectAll,       haveSymbolCondition, 400 );
        moveMenu.AddItem( ACTIONS::unselectAll,     haveSymbolCondition, 400 );
    }

    // Add editing actions to the drawing tool menu
    CONDITIONAL_MENU& drawMenu = drawingTools->GetToolMenu().GetMenu();

    drawMenu.AddSeparator( 200 );
    drawMenu.AddItem( SCH_ACTIONS::rotateCCW,       canEdit && SCH_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( SCH_ACTIONS::rotateCW,        canEdit && SCH_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( SCH_ACTIONS::mirrorV,         canEdit && SCH_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( SCH_ACTIONS::mirrorH,         canEdit && SCH_CONDITIONS::IdleSelection, 200 );

    drawMenu.AddItem( SCH_ACTIONS::properties,      canEdit && SCH_CONDITIONS::Count( 1 ), 200 );

    // Add editing actions to the selection tool menu
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( SCH_ACTIONS::rotateCCW,    canEdit && SCH_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::rotateCW,     canEdit && SCH_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::mirrorV,      canEdit && SCH_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::mirrorH,      canEdit && SCH_CONDITIONS::NotEmpty, 200 );

    selToolMenu.AddItem( SCH_ACTIONS::swap,         swapSelectionCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::properties,   canEdit && SCH_CONDITIONS::Count( 1 ), 200 );

    selToolMenu.AddSeparator( 250 );
    selToolMenu.AddItem( SCH_ACTIONS::convertStackedPins, canEdit && canConvertStackedPins, 250 );
    selToolMenu.AddItem( SCH_ACTIONS::explodeStackedPin,  canEdit && canExplodeStackedPin, 250 );

    selToolMenu.AddSeparator( 300 );
    selToolMenu.AddItem( ACTIONS::cut,              SCH_CONDITIONS::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::copy,             SCH_CONDITIONS::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::copyAsText,       canCopyText && SCH_CONDITIONS::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::paste,            canEdit && SCH_CONDITIONS::Idle, 300 );
    selToolMenu.AddItem( ACTIONS::duplicate,        canEdit && SCH_CONDITIONS::NotEmpty, 300 );
    selToolMenu.AddItem( ACTIONS::doDelete,         canEdit && SCH_CONDITIONS::NotEmpty, 300 );

    selToolMenu.AddSeparator( 400 );
    selToolMenu.AddItem( ACTIONS::selectAll,        haveSymbolCondition, 400 );
    selToolMenu.AddItem( ACTIONS::unselectAll,      haveSymbolCondition, 400 );
    // clang-format on

    return true;
}


int SYMBOL_EDITOR_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.GetSize() == 0 )
        return 0;

    VECTOR2I    rotPoint;
    bool        ccw = ( aEvent.Matches( SCH_ACTIONS::rotateCCW.MakeEvent() ) );
    SCH_ITEM*   item = static_cast<SCH_ITEM*>( selection.Front() );
    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    if( !item->IsMoving() )
        commit->Modify( m_frame->GetCurSymbol(), m_frame->GetScreen(), RECURSE_MODE::RECURSE );

    if( selection.GetSize() == 1 )
        rotPoint = item->GetPosition();
    else
        rotPoint = m_frame->GetNearestHalfGridPosition( selection.GetCenter() );

    for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
    {
        item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );
        item->Rotate( rotPoint, ccw );
        m_frame->UpdateItem( item, false, true );
    }

    if( item->IsMoving() )
    {
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        if( !localCommit.Empty() )
            localCommit.Push( _( "Rotate" ) );
    }

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.GetSize() == 0 )
        return 0;

    VECTOR2I  mirrorPoint;
    bool      xAxis = ( aEvent.Matches( SCH_ACTIONS::mirrorV.MakeEvent() ) );
    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.Front() );

    if( !item->IsMoving() )
        saveCopyInUndoList( m_frame->GetCurSymbol(), UNDO_REDO::LIBEDIT );

    if( selection.GetSize() == 1 )
    {
        mirrorPoint = item->GetPosition();

        switch( item->Type() )
        {
        case SCH_FIELD_T:
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

            if( xAxis )
                field->SetVertJustify( GetFlippedAlignment( field->GetVertJustify() ) );
            else
                field->SetHorizJustify( GetFlippedAlignment( field->GetHorizJustify() ) );

            break;
        }

        default:
            if( xAxis )
                item->MirrorVertically( mirrorPoint.y );
            else
                item->MirrorHorizontally( mirrorPoint.x );

            break;
        }


        m_frame->UpdateItem( item, false, true );
    }
    else
    {
        mirrorPoint = m_frame->GetNearestHalfGridPosition( selection.GetCenter() );

        for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
        {
            item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

            if( xAxis )
                item->MirrorVertically( mirrorPoint.y );
            else
                item->MirrorHorizontally( mirrorPoint.x );

            m_frame->UpdateItem( item, false, true );
        }
    }

    if( item->IsMoving() )
    {
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        m_frame->OnModify();
    }

    return 0;
}
int SYMBOL_EDITOR_EDIT_TOOL::Swap( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION&         selection = m_selectionTool->RequestSelection( SwappableItems );
    std::vector<EDA_ITEM*> sorted = selection.GetItemsSortedBySelectionOrder();

    if( selection.Size() < 2 )
        return 0;

    EDA_ITEM* front = selection.Front();
    bool      isMoving = front->IsMoving();

    // Save copy for undo if not in edit (edit command already handle the save copy)
    if( front->GetEditFlags() == 0 )
        saveCopyInUndoList( front->GetParent(), UNDO_REDO::LIBEDIT );

    for( size_t i = 0; i < sorted.size() - 1; i++ )
    {
        SCH_ITEM* a = static_cast<SCH_ITEM*>( sorted[i] );
        SCH_ITEM* b = static_cast<SCH_ITEM*>( sorted[( i + 1 ) % sorted.size()] );

        VECTOR2I aPos = a->GetPosition(), bPos = b->GetPosition();
        std::swap( aPos, bPos );

        a->SetPosition( aPos );
        b->SetPosition( bPos );

        // Special case some common swaps
        if( a->Type() == b->Type() )
        {
            switch( a->Type() )
            {
            case SCH_PIN_T:
            {
                SCH_PIN* aPin = static_cast<SCH_PIN*>( a );
                SCH_PIN* bBpin = static_cast<SCH_PIN*>( b );

                PIN_ORIENTATION aOrient = aPin->GetOrientation();
                PIN_ORIENTATION bOrient = bBpin->GetOrientation();

                aPin->SetOrientation( bOrient );
                bBpin->SetOrientation( aOrient );

                break;
            }
            default: break;
            }
        }

        m_frame->UpdateItem( a, false, true );
        m_frame->UpdateItem( b, false, true );
    }

    // Update R-Tree for modified items
    for( EDA_ITEM* selected : selection )
        updateItem( selected, true );

    if( isMoving )
    {
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        m_frame->OnModify();
    }

    return 0;
}


static std::vector<KICAD_T> nonFields =
{
    LIB_SYMBOL_T,
    SCH_SHAPE_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_PIN_T
};


int SYMBOL_EDITOR_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL*           symbol = m_frame->GetCurSymbol();
    std::deque<EDA_ITEM*> items = m_selectionTool->RequestSelection().GetItems();
    SCH_COMMIT            commit( m_frame );

    if( items.empty() )
        return 0;

    // Don't leave a freed pointer in the selection
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    commit.Modify( symbol, m_frame->GetScreen() );

    std::set<SCH_ITEM*> toDelete;
    int                 fieldsHidden = 0;
    int                 fieldsAlreadyHidden = 0;

    for( EDA_ITEM* item : items )
    {
        if( item->Type() == SCH_PIN_T )
        {
            SCH_PIN*  curr_pin = static_cast<SCH_PIN*>( item );
            VECTOR2I pos = curr_pin->GetPosition();

            toDelete.insert( curr_pin );

            // when pin editing is synchronized, pins in the same position, with the same name
            // in different units are also removed.  But only one pin per unit (matching)
            if( m_frame->SynchronizePins() )
            {
                std::vector<bool> got_unit( symbol->GetUnitCount() + 1 );

                got_unit[curr_pin->GetUnit()] = true;

                for( SCH_PIN* pin : symbol->GetPins() )
                {
                    if( got_unit[pin->GetUnit()] )
                        continue;

                    if( pin->GetPosition() != pos )
                        continue;

                    if( pin->GetBodyStyle() != curr_pin->GetBodyStyle() )
                        continue;

                    if( pin->GetType() != curr_pin->GetType() )
                        continue;

                    if( pin->GetName() != curr_pin->GetName() )
                        continue;

                    toDelete.insert( pin );
                    got_unit[pin->GetUnit()] = true;
                }
            }
        }
        else if( item->Type() == SCH_FIELD_T )
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

            // Hide "deleted" fields
            if( field->IsVisible() )
            {
                field->SetVisible( false );
                fieldsHidden++;
            }
            else
            {
                fieldsAlreadyHidden++;
            }
        }
        else if( SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item ) )
        {
            toDelete.insert( schItem );
        }
    }

    for( SCH_ITEM* item : toDelete )
        symbol->RemoveDrawItem( item );

    if( toDelete.size() == 0 )
    {
        if( fieldsHidden == 1 )
            commit.Push( _( "Hide Field" ) );
        else if( fieldsHidden > 1 )
            commit.Push( _( "Hide Fields" ) );
        else if( fieldsAlreadyHidden > 0 )
            m_frame->ShowInfoBarError( _( "Use the Symbol Properties dialog to remove fields." ) );
    }
    else
    {
        commit.Push( _( "Delete" ) );
    }

    m_frame->RebuildView();
    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.Empty() || aEvent.IsAction( &SCH_ACTIONS::symbolProperties ) )
    {
        // If called from tree context menu, edit properties without loading into canvas
        if( aEvent.IsAction( &SCH_ACTIONS::symbolProperties ) )
        {
            LIB_ID treeLibId = m_frame->GetTreeLIBID();

            // Check if the selected symbol in tree is different from the currently loaded one
            if( treeLibId.IsValid() &&
                ( !m_frame->GetCurSymbol() || m_frame->GetCurSymbol()->GetLibId() != treeLibId ) )
            {
                // Edit properties directly from library buffer without loading to canvas
                editSymbolPropertiesFromLibrary( treeLibId );
                return 0;
            }
        }

        if( m_frame->GetCurSymbol() )
            editSymbolProperties();
    }
    else if( selection.Size() == 1 )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.Front() );

        // Save copy for undo if not in edit (edit command already handle the save copy)
        if( item->GetEditFlags() == 0 )
            saveCopyInUndoList( item->GetParent(), UNDO_REDO::LIBEDIT );

        switch( item->Type() )
        {
        case SCH_PIN_T:
        {
            SCH_PIN& pin = static_cast<SCH_PIN&>( *item );

            // Mouse, not cursor, as grid points may well not be under any text
            const VECTOR2I&   mousePos = m_toolMgr->GetMousePosition();
            PIN_LAYOUT_CACHE& layout = pin.GetLayoutCache();

            bool mouseOverNumber = false;
            if( OPT_BOX2I numberBox = layout.GetPinNumberBBox() )
            {
                mouseOverNumber = numberBox->Contains( mousePos );
            }

            if( SYMBOL_EDITOR_PIN_TOOL* pinTool = m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>() )
                pinTool->EditPinProperties( &pin, mouseOverNumber );

            break;
        }
        case SCH_SHAPE_T:
            editShapeProperties( static_cast<SCH_SHAPE*>( item ) );
            break;

        case SCH_TEXT_T:
            editTextProperties( item );
            break;

        case SCH_TEXTBOX_T:
            editTextBoxProperties( item );
            break;

        case SCH_FIELD_T:
            editFieldProperties( static_cast<SCH_FIELD*>( item ) );
            break;

        default:
            wxFAIL_MSG( wxT( "Unhandled item <" ) + item->GetClass() + wxT( ">" ) );
            break;
        }
    }

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


void SYMBOL_EDITOR_EDIT_TOOL::editShapeProperties( SCH_SHAPE* aShape )
{
    DIALOG_SHAPE_PROPERTIES dlg( m_frame, aShape );

    if( dlg.ShowModal() != wxID_OK )
        return;

    updateItem( aShape, true );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    SYMBOL_EDITOR_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
    drawingTools->SetDrawSpecificBodyStyle( !dlg.GetApplyToAllConversions() );
    drawingTools->SetDrawSpecificUnit( !dlg.GetApplyToAllUnits() );

    std::vector<MSG_PANEL_ITEM> items;
    aShape->GetMsgPanelInfo( m_frame, items );
    m_frame->SetMsgPanel( items );
}


void SYMBOL_EDITOR_EDIT_TOOL::editTextProperties( SCH_ITEM* aItem )
{
    if ( aItem->Type() != SCH_TEXT_T )
        return;

    DIALOG_TEXT_PROPERTIES dlg( m_frame, static_cast<SCH_TEXT*>( aItem ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    updateItem( aItem, true );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify( );
}


void SYMBOL_EDITOR_EDIT_TOOL::editTextBoxProperties( SCH_ITEM* aItem )
{
    if ( aItem->Type() != SCH_TEXTBOX_T )
        return;

    DIALOG_TEXT_PROPERTIES dlg( m_frame, static_cast<SCH_TEXTBOX*>( aItem ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    updateItem( aItem, true );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify( );
}


void SYMBOL_EDITOR_EDIT_TOOL::editFieldProperties( SCH_FIELD* aField )
{
    if( aField == nullptr )
        return;

    wxString caption;

    if( aField->IsMandatory() )
        caption.Printf( _( "Edit %s Field" ), TitleCaps( aField->GetName() ) );
    else
        caption.Printf( _( "Edit '%s' Field" ), aField->GetName() );

    DIALOG_FIELD_PROPERTIES dlg( m_frame, caption, aField );

    // The dialog may invoke a kiway player for footprint fields
    // so we must use a quasimodal dialog.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    SCH_COMMIT commit( m_toolMgr );
    commit.Modify( aField, m_frame->GetScreen() );

    dlg.UpdateField( aField );

    commit.Push( caption );

    m_frame->GetCanvas()->Refresh();
    m_frame->UpdateSymbolMsgPanelInfo();
}


void SYMBOL_EDITOR_EDIT_TOOL::editSymbolPropertiesFromLibrary( const LIB_ID& aLibId )
{
    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = m_frame->GetLibManager();
    wxString libName = aLibId.GetLibNickname();
    wxString symbolName = aLibId.GetLibItemName();

    // Get the symbol from the library buffer (without loading it into the editor)
    LIB_SYMBOL* bufferedSymbol = libMgr.GetBufferedSymbol( symbolName, libName );

    if( !bufferedSymbol )
        return;

    // Create a copy to work with
    LIB_SYMBOL tempSymbol( *bufferedSymbol );

    m_toolMgr->RunAction( ACTIONS::cancelInteractive );
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    DIALOG_LIB_SYMBOL_PROPERTIES dlg( m_frame, &tempSymbol );

    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    // Update the buffered symbol with the changes
    libMgr.UpdateSymbol( &tempSymbol, libName );

    // Mark the library as modified
    libMgr.SetSymbolModified( symbolName, libName );

    // Update the tree view
    wxDataViewItem treeItem = libMgr.GetAdapter()->FindItem( aLibId );
    m_frame->UpdateLibraryTree( treeItem, &tempSymbol );
}


void SYMBOL_EDITOR_EDIT_TOOL::editSymbolProperties()
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();
    bool        partLocked = symbol->UnitsLocked();

    m_toolMgr->RunAction( ACTIONS::cancelInteractive );
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    DIALOG_LIB_SYMBOL_PROPERTIES dlg( m_frame, symbol );

    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    m_frame->RebuildSymbolUnitAndBodyStyleLists();
    m_frame->OnModify();

    // if m_UnitSelectionLocked has changed, set some edit options or defaults
    // to the best value
    if( partLocked != symbol->UnitsLocked() )
    {
        SYMBOL_EDITOR_DRAWING_TOOLS* tools = m_toolMgr->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();

        // Enable synchronized pin edit mode for symbols with interchangeable units
        m_frame->m_SyncPinEdit = !symbol->UnitsLocked();

        // also set default edit options to the better value
        // Usually if units are locked, graphic items are specific to each unit
        // and if units are interchangeable, graphic items are common to units
        tools->SetDrawSpecificUnit( symbol->UnitsLocked() );
    }
}


int SYMBOL_EDITOR_EDIT_TOOL::PinTable( const TOOL_EVENT& aEvent )
{
    SCH_COMMIT  commit( m_frame );
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    if( !symbol )
        return 0;

    commit.Modify( symbol, m_frame->GetScreen() );

    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    wxCHECK( selTool, -1 );

    std::vector<SCH_PIN*> selectedPins;

    SCH_SELECTION& selection = selTool->GetSelection();

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_PIN_T )
        {
            SCH_PIN* pinItem = static_cast<SCH_PIN*>( item );
            selectedPins.push_back( pinItem );
        }
    }

    // And now clear the selection so if we change the pins we don't have dangling pointers
    // in the selection.
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    DIALOG_LIB_EDIT_PIN_TABLE dlg( m_frame, symbol, selectedPins );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    commit.Push( _( "Edit Pins" ) );
    m_frame->RebuildView();

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::ConvertStackedPins( const TOOL_EVENT& aEvent )
{
    SCH_COMMIT  commit( m_frame );
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    if( !symbol )
        return 0;

    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    wxCHECK( selTool, -1 );

    SCH_SELECTION& selection = selTool->GetSelection();

    // Collect pins to convert - accept pins with any number format
    std::vector<SCH_PIN*> pinsToConvert;

    if( selection.Size() == 1 && selection.Front()->Type() == SCH_PIN_T )
    {
        // Single pin selected - find all pins at the same location
        SCH_PIN* selectedPin = static_cast<SCH_PIN*>( selection.Front() );
        VECTOR2I pos = selectedPin->GetPosition();

        for( SCH_PIN* pin : symbol->GetPins() )
        {
            if( pin->GetPosition() == pos )
                pinsToConvert.push_back( pin );
        }
    }
    else
    {
        // Multiple pins selected - use them directly, accepting any pin numbers
        for( EDA_ITEM* item : selection )
        {
            if( item->Type() == SCH_PIN_T )
                pinsToConvert.push_back( static_cast<SCH_PIN*>( item ) );
        }
    }

    if( pinsToConvert.size() < 2 )
    {
        m_frame->ShowInfoBarError( _( "At least two pins are needed to convert to stacked pins" ) );
        return 0;
    }

    // Check that all pins are at the same location
    VECTOR2I pos = pinsToConvert[0]->GetPosition();
    for( size_t i = 1; i < pinsToConvert.size(); ++i )
    {
        if( pinsToConvert[i]->GetPosition() != pos )
        {
            m_frame->ShowInfoBarError( _( "All pins must be at the same location" ) );
            return 0;
        }
    }

    commit.Modify( symbol, m_frame->GetScreen() );

    // Clear selection before modifying pins, like the Delete command does
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    // Sort pins for consistent ordering - handle arbitrary pin number formats
    std::sort( pinsToConvert.begin(), pinsToConvert.end(),
        []( SCH_PIN* a, SCH_PIN* b )
        {
            wxString numA = a->GetNumber();
            wxString numB = b->GetNumber();

            // Try to convert to integers for proper numeric sorting
            long longA, longB;
            bool aIsNumeric = numA.ToLong( &longA );
            bool bIsNumeric = numB.ToLong( &longB );

            // Both are purely numeric - sort numerically
            if( aIsNumeric && bIsNumeric )
                return longA < longB;

            // Mixed numeric/non-numeric - numeric pins come first
            if( aIsNumeric && !bIsNumeric )
                return true;
            if( !aIsNumeric && bIsNumeric )
                return false;

            // Both non-numeric or mixed alphanumeric - use lexicographic sorting
            return numA < numB;
        });

    // Build the stacked notation string with range collapsing
    wxString stackedNotation = wxT("[");

    // Helper function to collapse consecutive numbers into ranges - handles arbitrary pin formats
    auto collapseRanges = [&]() -> wxString
    {
        if( pinsToConvert.empty() )
            return wxT("");

        wxString result;

        // Group pins by their alphanumeric prefix for range collapsing
        std::map<wxString, std::vector<long>> prefixGroups;
        std::vector<wxString> nonNumericPins;

        // Parse each pin number to separate prefix from numeric suffix
        for( SCH_PIN* pin : pinsToConvert )
        {
            wxString pinNumber = pin->GetNumber();

            // Skip empty pin numbers (shouldn't happen, but be defensive)
            if( pinNumber.IsEmpty() )
            {
                nonNumericPins.push_back( wxT("(empty)") );
                continue;
            }

            wxString prefix;
            wxString numericPart;

            // Find where numeric part starts (scan from end)
            size_t numStart = pinNumber.length();
            for( int i = pinNumber.length() - 1; i >= 0; i-- )
            {
                if( !wxIsdigit( pinNumber[i] ) )
                {
                    numStart = i + 1;
                    break;
                }
                if( i == 0 )  // All digits
                    numStart = 0;
            }

            if( numStart < pinNumber.length() )  // Has numeric suffix
            {
                prefix = pinNumber.Left( numStart );
                numericPart = pinNumber.Mid( numStart );

                long numValue;
                if( numericPart.ToLong( &numValue ) && numValue >= 0 )  // Valid non-negative number
                {
                    prefixGroups[prefix].push_back( numValue );
                }
                else
                {
                    // Numeric part couldn't be parsed or is negative - treat as non-numeric
                    nonNumericPins.push_back( pinNumber );
                }
            }
            else  // No numeric suffix - consolidate as individual value
            {
                nonNumericPins.push_back( pinNumber );
            }
        }

        // Process each prefix group
        for( auto& [prefix, numbers] : prefixGroups )
        {
            if( !result.IsEmpty() )
                result += wxT(",");

            // Sort numeric values for this prefix
            std::sort( numbers.begin(), numbers.end() );

            // Collapse consecutive ranges within this prefix
            size_t i = 0;
            while( i < numbers.size() )
            {
                if( i > 0 )  // Not first number in this prefix group
                    result += wxT(",");

                long start = numbers[i];
                long end = start;

                // Find the end of consecutive sequence
                while( i + 1 < numbers.size() && numbers[i + 1] == numbers[i] + 1 )
                {
                    i++;
                    end = numbers[i];
                }

                // Add range or single number with prefix
                if( end > start + 1 )  // Range of 3+ numbers
                    result += wxString::Format( wxT("%s%ld-%s%ld"), prefix, start, prefix, end );
                else if( end == start + 1 )  // Two consecutive numbers
                    result += wxString::Format( wxT("%s%ld,%s%ld"), prefix, start, prefix, end );
                else  // Single number
                    result += wxString::Format( wxT("%s%ld"), prefix, start );

                i++;
            }
        }

        // Add non-numeric pin numbers as individual comma-separated values
        for( const wxString& nonNum : nonNumericPins )
        {
            if( !result.IsEmpty() )
                result += wxT(",");
            result += nonNum;
        }

        return result;
    };

    stackedNotation += collapseRanges();
    stackedNotation += wxT("]");

    // Keep the first pin and give it the stacked notation
    SCH_PIN* masterPin = pinsToConvert[0];
    masterPin->SetNumber( stackedNotation );

    // Log information about pins being removed before we remove them
    wxLogTrace( traceStackedPins,
               wxString::Format( "Converting %zu pins to stacked notation '%s'",
                               pinsToConvert.size(), stackedNotation ) );

    // Remove all other pins from the symbol that were consolidated into the stacked notation
    // Collect pins to remove first, then remove them all at once like the Delete command
    std::vector<SCH_PIN*> pinsToRemove;
    for( size_t i = 1; i < pinsToConvert.size(); ++i )
    {
        SCH_PIN* pinToRemove = pinsToConvert[i];

        // Log the pin before removing it
    wxLogTrace( traceStackedPins,
           wxString::Format( "Will remove pin '%s' at position (%d, %d)",
                   pinToRemove->GetNumber(),
                   pinToRemove->GetPosition().x,
                   pinToRemove->GetPosition().y ) );

        pinsToRemove.push_back( pinToRemove );
    }

    // Remove all pins at once, like the Delete command does
    for( SCH_PIN* pin : pinsToRemove )
    {
        symbol->RemoveDrawItem( pin );
    }

    commit.Push( wxString::Format( _( "Convert %zu Stacked Pins to '%s'" ),
                                  pinsToConvert.size(), stackedNotation ) );
    m_frame->RebuildView();
    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::ExplodeStackedPin( const TOOL_EVENT& aEvent )
{
    SCH_COMMIT  commit( m_frame );
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    if( !symbol )
        return 0;

    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    wxCHECK( selTool, -1 );

    SCH_SELECTION& selection = selTool->GetSelection();

    if( selection.GetSize() != 1 || selection.Front()->Type() != SCH_PIN_T )
    {
        m_frame->ShowInfoBarError( _( "Select a single pin with stacked notation to explode" ) );
        return 0;
    }

    SCH_PIN* pin = static_cast<SCH_PIN*>( selection.Front() );

    // Check if the pin has stacked notation
    bool isValid;
    std::vector<wxString> stackedNumbers = pin->GetStackedPinNumbers( &isValid );

    if( !isValid || stackedNumbers.size() <= 1 )
    {
        m_frame->ShowInfoBarError( _( "Selected pin does not have valid stacked notation" ) );
        return 0;
    }

    commit.Modify( symbol, m_frame->GetScreen() );

    // Clear selection before modifying pins
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    // Sort the stacked numbers to find the smallest one
    std::sort( stackedNumbers.begin(), stackedNumbers.end(),
        []( const wxString& a, const wxString& b )
        {
            // Try to convert to integers for proper numeric sorting
            long numA, numB;
            if( a.ToLong( &numA ) && b.ToLong( &numB ) )
                return numA < numB;

            // Fall back to string comparison if not numeric
            return a < b;
        });

    // Change the original pin to use the first (smallest) number and make it visible
    pin->SetNumber( stackedNumbers[0] );
    pin->SetVisible( true );

    // Create additional pins for the remaining numbers and make them invisible
    for( size_t i = 1; i < stackedNumbers.size(); ++i )
    {
        SCH_PIN* newPin = new SCH_PIN( symbol );

        // Copy all properties from the original pin
        newPin->SetPosition( pin->GetPosition() );
        newPin->SetOrientation( pin->GetOrientation() );
        newPin->SetShape( pin->GetShape() );
        newPin->SetLength( pin->GetLength() );
        newPin->SetType( pin->GetType() );
        newPin->SetName( pin->GetName() );
        newPin->SetNumber( stackedNumbers[i] );
        newPin->SetNameTextSize( pin->GetNameTextSize() );
        newPin->SetNumberTextSize( pin->GetNumberTextSize() );
        newPin->SetUnit( pin->GetUnit() );
        newPin->SetBodyStyle( pin->GetBodyStyle() );
        newPin->SetVisible( false );  // Make all other pins invisible

        // Add the new pin to the symbol
        symbol->AddDrawItem( newPin );
    }

    commit.Push( _( "Explode Stacked Pin" ) );
    m_frame->RebuildView();
    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::UpdateSymbolFields( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    if( !symbol )
        return 0;

    if( !symbol->IsDerived() )
    {
        m_frame->ShowInfoBarError( _( "Symbol is not derived from another symbol." ) );
    }
    else
    {
        DIALOG_UPDATE_SYMBOL_FIELDS dlg( m_frame, symbol );

        if( dlg.ShowModal() == wxID_CANCEL )
            return -1;
    }

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Undo( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    // Nuke the selection for later rebuilding.  This does *not* clear the flags on any items;
    // it just clears the SELECTION's reference to them.
    selTool->GetSelection().Clear();
    {
        m_frame->GetSymbolFromUndoList();
    }
    selTool->RebuildSelection();

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Redo( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    // Nuke the selection for later rebuilding.  This does *not* clear the flags on any items;
    // it just clears the SELECTION's reference to them.
    selTool->GetSelection().Clear();
    {
        m_frame->GetSymbolFromRedoList();
    }
    selTool->RebuildSelection();

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Cut( const TOOL_EVENT& aEvent )
{
    int retVal = Copy( aEvent );

    if( retVal == 0 )
        retVal = DoDelete( aEvent );

    return retVal;
}


int SYMBOL_EDITOR_EDIT_TOOL::Copy( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL*    symbol = m_frame->GetCurSymbol();
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( nonFields );

    if( !symbol || !selection.GetSize() )
        return 0;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_FIELD_T )
            continue;

        wxASSERT( !item.HasFlag( STRUCT_DELETED ) );

        if( !item.IsSelected() )
            item.SetFlags( STRUCT_DELETED );
    }

    LIB_SYMBOL* partCopy = new LIB_SYMBOL( *symbol );

    STRING_FORMATTER  formatter;
    SCH_IO_KICAD_SEXPR::FormatLibSymbol( partCopy, formatter );

    delete partCopy;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
        item.ClearFlags( STRUCT_DELETED );

    std::string prettyData = formatter.GetString();
    KICAD_FORMAT::Prettify( prettyData, KICAD_FORMAT::FORMAT_MODE::COMPACT_TEXT_PROPERTIES );

    // Generate SVG and PNG for multi-format clipboard
    std::vector<CLIPBOARD_MIME_DATA> mimeData;

    // Get the bounding box for just the selected items
    BOX2I bbox;

    for( EDA_ITEM* item : selection )
    {
        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );
        if( bbox.GetWidth() == 0 && bbox.GetHeight() == 0 )
            bbox = schItem->GetBoundingBox();
        else
            bbox.Merge( schItem->GetBoundingBox() );
    }

    if( bbox.GetWidth() > 0 && bbox.GetHeight() > 0 )
    {
        bbox.Inflate( bbox.GetWidth() * clipboardBboxInflation,
                      bbox.GetHeight() * clipboardBboxInflation );

        // Create a temporary symbol with just the selected items for plotting
        LIB_SYMBOL* plotSymbol = new LIB_SYMBOL( *symbol );

        // Mark unselected items as deleted in the plot copy
        for( SCH_ITEM& item : plotSymbol->GetDrawItems() )
        {
            if( item.Type() == SCH_FIELD_T )
                continue;

            // Find matching item in selection by position/type
            bool found = false;

            for( EDA_ITEM* selItem : selection )
            {
                SCH_ITEM* selSchItem = static_cast<SCH_ITEM*>( selItem );

                if( selSchItem->Type() == item.Type()
                    && selSchItem->GetPosition() == item.GetPosition() )
                {
                    found = true;
                    break;
                }
            }

            if( !found )
                item.SetFlags( STRUCT_DELETED );
        }

        // Now copy only the non-deleted items to a clean symbol for plotting
        LIB_SYMBOL* cleanSymbol = new LIB_SYMBOL( *plotSymbol );
        delete plotSymbol;

        int unit = m_frame->GetUnit();
        int bodyStyle = m_frame->GetBodyStyle();

        wxMemoryBuffer svgBuffer;

        if( plotSymbolToSvg( m_frame, cleanSymbol, bbox, unit, bodyStyle, svgBuffer ) )
            appendMimeData( mimeData, wxS( "image/svg+xml" ), svgBuffer );

        wxImage pngImage = renderSymbolToImageWithAlpha( m_frame, cleanSymbol, bbox, unit, bodyStyle );

        if( pngImage.IsOk() )
            appendMimeData( mimeData, wxS( "image/png" ), std::move( pngImage ) );

        delete cleanSymbol;
    }

    if( SaveClipboard( prettyData, mimeData ) )
        return 0;
    else
        return -1;
}


int SYMBOL_EDITOR_EDIT_TOOL::CopyAsText( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selTool->RequestSelection();

    if( selection.Empty() )
        return 0;

    wxString itemsAsText = GetSelectedItemsAsText( selection );

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return SaveClipboard( itemsAsText.ToStdString() );
}


int SYMBOL_EDITOR_EDIT_TOOL::Paste( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol  = m_frame->GetCurSymbol();
    LIB_SYMBOL* newPart = nullptr;

    if( !symbol || symbol->IsDerived() )
        return 0;

    std::string clipboardData = GetClipboardUTF8();

    try
    {
        std::vector<LIB_SYMBOL*> newParts = SCH_IO_KICAD_SEXPR::ParseLibSymbols( clipboardData, "Clipboard" );

        if( newParts.empty() || !newParts[0] )
            return -1;

        newPart = newParts[0];
    }
    catch( IO_ERROR& )
    {
        // If it's not a symbol then paste as text
        newPart = new LIB_SYMBOL( "dummy_part" );

        wxString pasteText( clipboardData );

        // Limit of 5000 is totally arbitrary.  Without a limit, pasting a bitmap image from
        // eeschema makes KiCad appear to hang.
        if( pasteText.Length() > 5000 )
            pasteText = pasteText.Left( 5000 ) + wxT( "..." );

        SCH_TEXT* newText = new SCH_TEXT( { 0, 0 }, pasteText, LAYER_DEVICE );
        newPart->AddDrawItem( newText );
    }

    SCH_COMMIT commit( m_toolMgr );

    commit.Modify( symbol, m_frame->GetScreen() );
    m_selectionTool->ClearSelection();

    for( SCH_ITEM& item : symbol->GetDrawItems() )
        item.ClearFlags( IS_NEW | IS_PASTED | SELECTED );

    for( SCH_ITEM& item : newPart->GetDrawItems() )
    {
        if( item.Type() == SCH_FIELD_T )
            continue;

        SCH_ITEM* newItem = item.Duplicate( true, &commit );
        newItem->SetParent( symbol );
        newItem->SetFlags( IS_NEW | IS_PASTED | SELECTED );

        newItem->SetUnit( newItem->GetUnit() ? m_frame->GetUnit() : 0 );
        newItem->SetBodyStyle( newItem->GetBodyStyle() ? m_frame->GetBodyStyle() : 0 );

        symbol->AddDrawItem( newItem );
        getView()->Add( newItem );
    }

    delete newPart;

    m_selectionTool->RebuildSelection();

    SCH_SELECTION& selection = m_selectionTool->GetSelection();

    if( !selection.Empty() )
    {
        selection.SetReferencePoint( getViewControls()->GetCursorPosition( true ) );

        if( m_toolMgr->RunSynchronousAction( SCH_ACTIONS::move, &commit ) )
            commit.Push( _( "Paste" ) );
        else
            commit.Revert();
    }

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL*    symbol = m_frame->GetCurSymbol();
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( nonFields );
    SCH_COMMIT     commit( m_toolMgr );

    if( selection.GetSize() == 0 )
        return 0;

    commit.Modify( symbol, m_frame->GetScreen() );

    std::vector<EDA_ITEM*> oldItems;
    std::vector<EDA_ITEM*> newItems;

    std::copy( selection.begin(), selection.end(), std::back_inserter( oldItems ) );
    std::sort( oldItems.begin(), oldItems.end(), []( EDA_ITEM* a, EDA_ITEM* b )
    {
        int cmp;

        if( a->Type() != b->Type() )
            return a->Type() < b->Type();

        // Create the new pins in the same order as the old pins
        if( a->Type() == SCH_PIN_T )
        {
            const wxString& aNum = static_cast<SCH_PIN*>( a )->GetNumber();
            const wxString& bNum = static_cast<SCH_PIN*>( b )->GetNumber();

            cmp = StrNumCmp( aNum, bNum );

            // If the pin numbers are not numeric, then just number them by their position
            // on the screen.
            if( aNum.IsNumber() && bNum.IsNumber() && cmp != 0 )
                return cmp < 0;
        }

        cmp = LexicographicalCompare( a->GetPosition(), b->GetPosition() );

        if( cmp != 0 )
            return cmp < 0;

        return a->m_Uuid < b->m_Uuid;
    } );

    for( EDA_ITEM* item : oldItems )
    {
        SCH_ITEM* oldItem = static_cast<SCH_ITEM*>( item );
        SCH_ITEM* newItem = oldItem->Duplicate( true, &commit );

        if( newItem->Type() == SCH_PIN_T )
        {
            SCH_PIN* newPin = static_cast<SCH_PIN*>( newItem );

            if( !newPin->GetNumber().IsEmpty() )
                newPin->SetNumber( wxString::Format( wxT( "%i" ), symbol->GetMaxPinNumber() + 1 ) );
        }

        oldItem->ClearFlags( IS_NEW | IS_PASTED | SELECTED );
        newItem->SetFlags( IS_NEW | IS_PASTED | SELECTED );
        newItem->SetParent( symbol );
        newItems.push_back( newItem );

        symbol->AddDrawItem( newItem );
        getView()->Add( newItem );
    }

    m_toolMgr->RunAction( ACTIONS::selectionClear );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &newItems );

    selection.SetReferencePoint( getViewControls()->GetCursorPosition( true ) );

    if( m_toolMgr->RunSynchronousAction( SCH_ACTIONS::move, &commit ) )
        commit.Push( _( "Duplicate" ) );
    else
        commit.Revert();

    return 0;
}


void SYMBOL_EDITOR_EDIT_TOOL::setTransitions()
{
    // clang-format off
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Undo,               ACTIONS::undo.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Redo,               ACTIONS::redo.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Cut,                ACTIONS::cut.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Copy,               ACTIONS::copy.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::CopyAsText,         ACTIONS::copyAsText.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Paste,              ACTIONS::paste.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Duplicate,          ACTIONS::duplicate.MakeEvent() );

    Go( &SYMBOL_EDITOR_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCW.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCCW.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorV.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorH.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Swap,               SCH_ACTIONS::swap.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::DoDelete,           ACTIONS::doDelete.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::InteractiveDelete,  ACTIONS::deleteTool.MakeEvent() );

    Go( &SYMBOL_EDITOR_EDIT_TOOL::Increment,          ACTIONS::increment.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Increment,          ACTIONS::incrementPrimary.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Increment,          ACTIONS::decrementPrimary.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Increment,          ACTIONS::incrementSecondary.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Increment,          ACTIONS::decrementSecondary.MakeEvent() );

    Go( &SYMBOL_EDITOR_EDIT_TOOL::Properties,         SCH_ACTIONS::properties.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Properties,         SCH_ACTIONS::symbolProperties.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::PinTable,           SCH_ACTIONS::pinTable.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::ConvertStackedPins, SCH_ACTIONS::convertStackedPins.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::ExplodeStackedPin,  SCH_ACTIONS::explodeStackedPin.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::UpdateSymbolFields, SCH_ACTIONS::updateSymbolFields.MakeEvent() );
    // clang-format on
}
