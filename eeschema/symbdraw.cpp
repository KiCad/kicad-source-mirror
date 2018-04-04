/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009-2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2017 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file symbdraw.cpp
 * @brief Create, move .. graphic shapes used to build and draw a symbol (lines, arcs ..)
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <base_units.h>
#include <msgpanel.h>

#include <eeschema_id.h>
#include <lib_edit_frame.h>
#include <class_libentry.h>
#include <lib_arc.h>
#include <lib_circle.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include <lib_text.h>

#include <dialogs/dialog_lib_edit_draw_item.h>


static void SymbolDisplayDraw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                               bool aErase );
static void RedrawWhileMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase );


void LIB_EDIT_FRAME::EditGraphicSymbol( wxDC* DC, LIB_ITEM* DrawItem )
{
    if( DrawItem == NULL )
        return;

    LIB_PART*      symbol = DrawItem->GetParent();

    DIALOG_LIB_EDIT_DRAW_ITEM dialog( this, DrawItem->GetTypeName() );

    dialog.SetWidthUnits( ReturnUnitSymbol( g_UserUnit ) );

    wxString val = StringFromValue( g_UserUnit, DrawItem->GetWidth() );
    dialog.SetWidth( val );
    dialog.SetApplyToAllUnits( DrawItem->GetUnit() == 0 );
    dialog.EnableApplyToAllUnits( symbol && symbol->GetUnitCount() > 1 );
    dialog.SetApplyToAllConversions( DrawItem->GetConvert() == 0 );
    bool enblConvOptStyle = symbol && symbol->HasConversion();
    // if a symbol contains no graphic items, symbol->HasConversion() returns false.
    // but when creating a new symbol, with DeMorgan option set, the ApplyToAllConversions
    // must be enabled even if symbol->HasConversion() returns false in order to be able
    // to create graphic items shared by all body styles
    if( GetShowDeMorgan() )
        enblConvOptStyle = true;

    dialog.EnableApplyToAllConversions( enblConvOptStyle );
    dialog.SetFillStyle( DrawItem->GetFillMode() );
    dialog.EnableFillStyle( DrawItem->IsFillable() );

    if( dialog.ShowModal() == wxID_CANCEL )
        return;

    // Init default values (used to create a new draw item)
    val = dialog.GetWidth();
    m_drawLineWidth = ValueFromString( g_UserUnit, val );
    m_drawSpecificConvert = !dialog.GetApplyToAllConversions();
    m_drawSpecificUnit    = !dialog.GetApplyToAllUnits();

#if 0
    /* TODO: see if m_drawFillStyle must retain the last fill option or not.
     * if the last is Filled, having next new graphic items created
     * with filled body is often bad.
     * currently m_drawFillStyle is left with the default value (not filled)
     */
    if( DrawItem->IsFillable() )
        m_drawFillStyle = (FILL_T) dialog.GetFillStyle();
#endif

    // Save copy for undo if not in edit (edit command already handle the save copy)
    if( !DrawItem->InEditMode() )
        SaveCopyInUndoList( DrawItem->GetParent() );

    if( m_drawSpecificUnit )
        DrawItem->SetUnit( GetUnit() );
    else
        DrawItem->SetUnit( 0 );

    if( m_drawSpecificConvert )
        DrawItem->SetConvert( GetConvert() );
    else
        DrawItem->SetConvert( 0 );

    if( DrawItem->IsFillable() )
        DrawItem->SetFillMode( (FILL_T) dialog.GetFillStyle() );

    DrawItem->SetWidth( m_drawLineWidth );

    OnModify( );

    MSG_PANEL_ITEMS items;
    DrawItem->GetMsgPanelInfo( items );
    SetMsgPanel( items );
    m_canvas->Refresh();
}


static void AbortSymbolTraceOn( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) Panel->GetParent();
    LIB_ITEM* item = parent->GetDrawItem();

    if( item == NULL )
        return;

    bool newItem = item->IsNew();
    item->EndEdit( parent->GetCrossHairPosition( true ), true );

    if( newItem )
    {
        delete item;
    }
    else
        parent->RestoreComponent();

    parent->SetDrawItem( NULL );
    Panel->Refresh();
}


LIB_ITEM* LIB_EDIT_FRAME::CreateGraphicItem( LIB_PART* LibEntry, wxDC* DC )
{
    LIB_ITEM* item = GetDrawItem();
    m_canvas->SetMouseCapture( SymbolDisplayDraw, AbortSymbolTraceOn );
    wxPoint drawPos = GetCrossHairPosition( true );

    // no temp copy -> the current version of symbol will be used for Undo
    // This is normal when adding new items to the current symbol
    ClearTempCopyComponent();

    switch( GetToolId() )
    {
    case ID_LIBEDIT_BODY_ARC_BUTT:
        item = new LIB_ARC( LibEntry );
        break;

    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
        item = new LIB_CIRCLE( LibEntry );
        break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
        item = new LIB_RECTANGLE( LibEntry );
        break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
        item = new LIB_POLYLINE( LibEntry );
        break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
        {
            LIB_TEXT* text = new LIB_TEXT( LibEntry );
            text->SetTextSize( wxSize( m_textSize, m_textSize ) );
            text->SetTextAngle( m_current_text_angle );

            // Enter the graphic text info
            m_canvas->SetIgnoreMouseEvents( true );
            EditSymbolText( NULL, text );

            m_canvas->SetIgnoreMouseEvents( false );
            m_canvas->MoveCursorToCrossHair();

            if( text->GetText().IsEmpty() )
            {
                delete text;
                item = NULL;
            }
            else
            {
                item = text;
            }
        }
        break;

    default:
        DisplayError( this, wxT( "LIB_EDIT_FRAME::CreateGraphicItem() error" ) );
        return NULL;
    }

    if( item )
    {
        item->BeginEdit( IS_NEW, drawPos );

        // Don't set line parameters for text objects.
        if( item->Type() != LIB_TEXT_T )
        {
            item->SetWidth( m_drawLineWidth );
            item->SetFillMode( m_drawFillStyle );
        }

        if( m_drawSpecificUnit )
            item->SetUnit( m_unit );

        if( m_drawSpecificConvert )
            item->SetConvert( m_convert );

        // Draw initial symbol:
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
    }
    else
    {
        m_canvas->EndMouseCapture();
        return NULL;
    }

    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
    SetDrawItem( item );

    return item;
}


void LIB_EDIT_FRAME::GraphicItemBeginDraw( wxDC* DC )
{
    if( GetDrawItem() == NULL )
        return;

    wxPoint pos = GetCrossHairPosition( true );

    if( GetDrawItem()->ContinueEdit( pos ) )
    {
        GetDrawItem()->Draw( m_canvas, DC, pos, COLOR4D::UNSPECIFIED, g_XorMode, NULL,
                          DefaultTransform );
        return;
    }

    EndDrawGraphicItem( DC );
}


/*
 * Redraw the graphic shape while moving
 */
static void RedrawWhileMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase )
{
    LIB_ITEM* item;

    item = ( (LIB_EDIT_FRAME*) aPanel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    item->SetEraseLastDrawItem( aErase );

    // if item is the reference field, we must add the current unit id
    if( item->Type() == LIB_FIELD_T )
    {
        int         unit = ((LIB_EDIT_FRAME*)aPanel->GetParent())->GetUnit();
        wxString    text = ((LIB_FIELD*)item)->GetFullText( unit );

        item->Draw( aPanel, aDC, aPanel->GetParent()->GetCrossHairPosition( true ),
                    COLOR4D::UNSPECIFIED, g_XorMode, &text,
                    DefaultTransform );
    }
    else
        item->Draw( aPanel, aDC, aPanel->GetParent()->GetCrossHairPosition( true ),
                    COLOR4D::UNSPECIFIED, g_XorMode, NULL,
                    DefaultTransform );
}


void LIB_EDIT_FRAME::StartMoveDrawSymbol( wxDC* DC, LIB_ITEM* aItem )
{
    if( aItem == NULL )
        return;

    SetCursor( wxCURSOR_HAND );

    TempCopyComponent();

    // For fields only, move the anchor point of the field
    // to the cursor position to allow user to see the text justification
    if( aItem->Type() == LIB_FIELD_T )
        aItem->BeginEdit( IS_MOVED, aItem->GetPosition() );
    else
        aItem->BeginEdit( IS_MOVED, GetCrossHairPosition( true ) );

    m_canvas->SetMouseCapture( RedrawWhileMovingCursor, AbortSymbolTraceOn );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, true );
}


void LIB_EDIT_FRAME::StartModifyDrawSymbol( wxDC* DC, LIB_ITEM* aItem )
{
    if( aItem == NULL )
        return;

    TempCopyComponent();
    aItem->BeginEdit( IS_RESIZED, GetCrossHairPosition( true ) );
    m_canvas->SetMouseCapture( SymbolDisplayDraw, AbortSymbolTraceOn );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, true );
}


//! @brief Manage mouse events when creating new graphic object or modifying an graphic object.
static void SymbolDisplayDraw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                               bool aErase )
{
    LIB_ITEM* item = ( (LIB_EDIT_FRAME*) aPanel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    item->SetEraseLastDrawItem( aErase );
    item->Draw( aPanel, aDC, aPanel->GetParent()->GetCrossHairPosition( true ),
                COLOR4D::UNSPECIFIED, g_XorMode, NULL, DefaultTransform );
}


void LIB_EDIT_FRAME::EndDrawGraphicItem( wxDC* DC )
{
    LIB_ITEM* item = GetDrawItem();

    if( item == NULL )
        return;

    if( LIB_PART* part = GetCurPart() )
    {
        if( GetToolId() != ID_NO_TOOL_SELECTED )
            SetCursor( wxCURSOR_PENCIL );
        else
            SetCursor( (wxStockCursor) m_canvas->GetDefaultCursor() );

        if( GetTempCopyComponent() )    // used when editing an existing item
            SaveCopyInUndoList( GetTempCopyComponent() );
        else
        {
            // When creating a new item, there is still no change for the
            // current symbol. So save it.
            SaveCopyInUndoList( part );
        }

        if( item->IsNew() )
            part->AddDrawItem( item );

        item->EndEdit( GetCrossHairPosition( true ) );

        SetDrawItem( NULL );

        OnModify();

        m_canvas->SetMouseCapture( NULL, NULL );
        m_canvas->Refresh();
    }
}
