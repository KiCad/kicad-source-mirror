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
#include <sch_draw_panel.h>
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

#include <sch_view.h>
#include <dialogs/dialog_lib_edit_draw_item.h>


static void RedrawWhileMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase );


void LIB_EDIT_FRAME::EditGraphicSymbol( wxDC* DC, LIB_ITEM* DrawItem )
{
    if( DrawItem == NULL )
        return;

    DIALOG_LIB_EDIT_DRAW_ITEM dialog( this, DrawItem );

    if( dialog.ShowModal() == wxID_CANCEL )
        return;

    // Init default values (used to create a new draw item)
    m_drawLineWidth       = dialog.GetWidth();
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

    GetCanvas()->GetView()->Update( DrawItem );
    GetCanvas()->Refresh();
    OnModify( );

    MSG_PANEL_ITEMS items;
    DrawItem->GetMsgPanelInfo( m_UserUnits, items );
    SetMsgPanel( items );
}


static void AbortSymbolTraceOn( EDA_DRAW_PANEL* aPanel, wxDC* DC )
{
    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) aPanel->GetParent();
    LIB_ITEM* item = parent->GetDrawItem();

    if( item == NULL )
        return;

    bool newItem = item->IsNew();
    item->EndEdit( parent->GetCrossHairPosition( true ), true );

    if( newItem )
        delete item;
    else
        parent->RestoreComponent();

    parent->SetDrawItem( NULL );

    auto view = static_cast<SCH_DRAW_PANEL*>(aPanel)->GetView();
    view->ClearPreview();
    view->ShowPreview( false );
    view->ClearHiddenFlags();
    parent->RebuildView();
}


LIB_ITEM* LIB_EDIT_FRAME::CreateGraphicItem( LIB_PART* LibEntry, wxDC* DC )
{
    LIB_ITEM* item = GetDrawItem();
    m_canvas->SetMouseCapture( RedrawWhileMovingCursor, AbortSymbolTraceOn );
    wxPoint drawPos = GetCrossHairPosition( true );

    // no temp copy -> the current version of symbol will be used for Undo
    // This is normal when adding new items to the current symbol
    ClearTempCopyComponent();

    auto view = static_cast<SCH_DRAW_PANEL*>(m_canvas)->GetView();
    view->ShowPreview( true );

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

    auto view = static_cast<SCH_DRAW_PANEL*>(m_canvas)->GetView();
    view->ShowPreview( true );


    if( GetDrawItem()->ContinueEdit( pos ) )
        return;

    EndDrawGraphicItem( DC );
}


/*
 * Redraw the graphic shape while moving
 */
static void RedrawWhileMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase )
{
    LIB_ITEM* item = ( (LIB_EDIT_FRAME*) aPanel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    auto view = static_cast<SCH_DRAW_PANEL*>(aPanel)->GetView();
    auto p = aPanel->GetParent()->GetCrossHairPosition( true );

    item->CalcEdit( p );

    view->Hide( item );
    view->ClearPreview();
    view->AddToPreview( item->Clone() );
}


void LIB_EDIT_FRAME::StartMoveDrawSymbol( wxDC* DC, LIB_ITEM* aItem )
{
    if( aItem == NULL )
        return;

    SetCursor( wxCURSOR_HAND );

    GetGalCanvas()->GetView()->Hide ( aItem );

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

    DBG(printf("startmdifyraw\n");)

    TempCopyComponent();
    aItem->BeginEdit( IS_RESIZED, GetCrossHairPosition( true ) );
    m_canvas->SetMouseCapture( RedrawWhileMovingCursor, AbortSymbolTraceOn );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, true );
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
            SetCursor( (wxStockCursor) GetGalCanvas()->GetDefaultCursor() );

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

        m_canvas->SetMouseCapture( NULL, NULL );

        auto view = static_cast<SCH_DRAW_PANEL*>(m_canvas)->GetView();

        DBG(printf("end: pos %d %d\n", item->GetPosition().x, item->GetPosition().y );)

        view->ClearHiddenFlags();
        view->ClearPreview();

        OnModify();
    }

    RebuildView();
    GetCanvas()->Refresh();
}
