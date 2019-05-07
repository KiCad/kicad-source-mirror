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
    g_LastLineWidth       = dialog.GetWidth();
    m_DrawSpecificConvert = !dialog.GetApplyToAllConversions();
    m_DrawSpecificUnit    = !dialog.GetApplyToAllUnits();

    // Save copy for undo if not in edit (edit command already handle the save copy)
    if( !DrawItem->InEditMode() )
        SaveCopyInUndoList( DrawItem->GetParent() );

    if( m_DrawSpecificUnit )
        DrawItem->SetUnit( GetUnit() );
    else
        DrawItem->SetUnit( 0 );

    if( m_DrawSpecificConvert )
        DrawItem->SetConvert( GetConvert() );
    else
        DrawItem->SetConvert( 0 );

    if( DrawItem->IsFillable() )
        DrawItem->SetFillMode( (FILL_T) dialog.GetFillStyle() );

    DrawItem->SetWidth( g_LastLineWidth );

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
    item->EndEdit( parent->GetCrossHairPosition( true ) );

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
