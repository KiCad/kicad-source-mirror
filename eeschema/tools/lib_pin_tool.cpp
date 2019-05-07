/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <lib_edit_frame.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <sch_view.h>
#include <dialogs/dialog_display_info_HTML_base.h>
#include <dialogs/dialog_lib_edit_pin.h>
#include "lib_pin_tool.h"


static ELECTRICAL_PINTYPE g_LastPinType          = PIN_INPUT;
static int                g_LastPinOrient        = PIN_RIGHT;
static GRAPHIC_PINSHAPE   g_LastPinShape         = PINSHAPE_LINE;
static bool               g_LastPinCommonConvert = false;
static bool               g_LastPinCommonUnit    = false;
static bool               g_LastPinVisible       = true;

// The -1 is a non-valid value to trigger delayed initialization
static int                g_LastPinLength        = -1;
static int                g_LastPinNameSize      = -1;
static int                g_LastPinNumSize       = -1;

static int GetLastPinLength()
{
    if( g_LastPinLength == -1 )
        g_LastPinLength = LIB_EDIT_FRAME::GetDefaultPinLength();

    return g_LastPinLength;
}

static int GetLastPinNameSize()
{
    if( g_LastPinNameSize == -1 )
        g_LastPinNameSize = LIB_EDIT_FRAME::GetPinNameDefaultSize();

    return g_LastPinNameSize;
}

static int GetLastPinNumSize()
{
    if( g_LastPinNumSize == -1 )
        g_LastPinNumSize = LIB_EDIT_FRAME::GetPinNumDefaultSize();

    return g_LastPinNumSize;
}


extern void IncrementLabelMember( wxString& name, int aIncrement );


LIB_PIN_TOOL::LIB_PIN_TOOL() :
        TOOL_INTERACTIVE( "eeschema.PinEditing" ),
        m_selectionTool( nullptr ),
        m_frame( nullptr )
{
}


LIB_PIN_TOOL::~LIB_PIN_TOOL()
{
}


bool LIB_PIN_TOOL::Init()
{
    m_frame = getEditFrame<LIB_EDIT_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, "eeshema.InteractiveSelection tool is not available" );

    return true;
}


void LIB_PIN_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
    {
        // Init variables used by every drawing tool
        m_frame = getEditFrame<LIB_EDIT_FRAME>();
    }
}


void LIB_EDIT_FRAME::OnEditPin( wxCommandEvent& event )
{
    if( GetDrawItem() == NULL || GetDrawItem()->Type() != LIB_PIN_T )
        return;

    STATUS_FLAGS item_flags = GetDrawItem()->GetFlags(); // save flags to restore them after editing
    LIB_PIN* pin = (LIB_PIN*) GetDrawItem();

    pin->EnableEditMode( true, !SynchronizePins() );

    DIALOG_LIB_EDIT_PIN dlg( this, pin );

    if( dlg.ShowModal() == wxID_CANCEL )
    {
        if( pin->IsNew() )
        {
            pin->SetFlags( IS_CANCELLED );
            m_canvas->EndMouseCapture();
        }
        return;
    }

    if( pin->IsModified() || pin->IsNew() )
    {
        GetCanvas()->GetView()->Update( pin );
        GetCanvas()->Refresh();
        OnModify( );

        MSG_PANEL_ITEMS items;
        pin->GetMsgPanelInfo( m_UserUnits, items );
        SetMsgPanel( items );
    }

    pin->EnableEditMode( false );

    // Restore pin flags, that can be changed by the dialog editor
    pin->ClearFlags();
    pin->SetFlags( item_flags );

    // Save the pin properties to use for the next new pin.
    g_LastPinNameSize = pin->GetNameTextSize();
    g_LastPinNumSize = pin->GetNumberTextSize();
    g_LastPinOrient = pin->GetOrientation();
    g_LastPinLength = pin->GetLength();
    g_LastPinShape = pin->GetShape();
    g_LastPinType = pin->GetType();
    g_LastPinCommonConvert = pin->GetConvert() == 0;
    g_LastPinCommonUnit = pin->GetUnit() == 0;
    g_LastPinVisible = pin->IsVisible();
}


/**
 * Clean up after aborting a move pin command.
 */
static void AbortPinMove( EDA_DRAW_PANEL* aPanel, wxDC* DC )
{
    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) aPanel->GetParent();
    auto panel = static_cast<SCH_DRAW_PANEL*>( aPanel );

    if( parent == NULL )
        return;

    LIB_PIN* pin = (LIB_PIN*) parent->GetDrawItem();

    if( pin == NULL || pin->Type() != LIB_PIN_T )
        return;

    pin->ClearFlags();

    if( pin->IsNew() )
        delete pin;
    else
        parent->RestoreComponent();

    panel->GetView()->ClearPreview();
    panel->GetView()->ClearHiddenFlags();

    // clear edit flags
    parent->SetDrawItem( NULL );
    parent->SetLastDrawItem( NULL );
}


/**
 * Managed cursor callback for placing component pins.
 */
void LIB_EDIT_FRAME::PlacePin()
{
    LIB_PIN* cur_pin  = (LIB_PIN*) GetDrawItem();

    DBG(printf("PlacePin!\n");)

    // Some tests
    if( !cur_pin || cur_pin->Type() != LIB_PIN_T )
    {
        wxMessageBox( wxT( "LIB_EDIT_FRAME::PlacePin() error" ) );
        return;
    }

    wxPoint  newpos;
    newpos = GetCrossHairPosition( true );

    LIB_PART*      part = GetCurPart();

    // Test for another pin in same new position in other units:
    bool     ask_for_pin = true;

    for( LIB_PIN* pin = part->GetNextPin(); pin; pin = part->GetNextPin( pin ) )
    {
        if( pin == cur_pin || newpos != pin->GetPosition() || pin->GetEditFlags() )
            continue;

        // test for same body style
        if( pin->GetConvert() && pin->GetConvert() != cur_pin->GetConvert() )
            continue;

        if( ask_for_pin && SynchronizePins() )
        {
            m_canvas->SetIgnoreMouseEvents( true );
            wxString msg;
            msg.Printf( _( "This position is already occupied by another pin, in unit %d." ),
                        pin->GetUnit() );

            KIDIALOG dlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            dlg.SetOKLabel( _( "Create Pin Anyway" ) );
            dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            bool status = dlg.ShowModal() == wxID_OK;

            m_canvas->MoveCursorToCrossHair();
            m_canvas->SetIgnoreMouseEvents( false );

            if( !status )
            {
                RebuildView();
                return;
            }
            else
                ask_for_pin = false;
        }
    }

    // Create Undo from GetTempCopyComponent() if exists ( i.e. after a pin move)
    // or from m_component (pin add ...)
    if( GetTempCopyComponent() )
        SaveCopyInUndoList( GetTempCopyComponent() );
    else
        SaveCopyInUndoList( part );

    m_canvas->SetMouseCapture( NULL, NULL );
    cur_pin->Move( newpos );

    if( cur_pin->IsNew() )
    {
        g_LastPinOrient = cur_pin->GetOrientation();
        g_LastPinType   = cur_pin->GetType();
        g_LastPinShape  = cur_pin->GetShape();

        if( SynchronizePins() )
            CreateImagePins( cur_pin );

        m_lastDrawItem = cur_pin;
        part->AddDrawItem( GetDrawItem() );
    }

    // Put linked pins in new position, and clear flags
    for( LIB_PIN* pin = part->GetNextPin();  pin;  pin = part->GetNextPin( pin ) )
    {
        if( pin->GetEditFlags() == 0 )
            continue;

        pin->Move( cur_pin->GetPosition() );
        pin->ClearFlags();
    }

    SetDrawItem( NULL );

    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
}


/* Move pin to the current mouse position.  This function is called by the
 * cursor management code. */
static void DrawMovePin( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                         bool aErase )
{
    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) aPanel->GetParent();

    if( parent == NULL )
        return;

    LIB_PIN* cur_pin = (LIB_PIN*) parent->GetDrawItem();

    if( cur_pin == NULL || cur_pin->Type() != LIB_PIN_T )
        return;

    auto p =  aPanel->GetParent()->GetCrossHairPosition( true );

    // Redraw pin in new position
    cur_pin->Move(p);

    KIGFX::SCH_VIEW* view = parent->GetCanvas()->GetView();

    view->Hide( cur_pin );
    view->ClearPreview();
    view->AddToPreview( cur_pin->Clone() );
}


void LIB_EDIT_FRAME::StartMovePin( LIB_ITEM* aItem )
{
    LIB_PIN* cur_pin = (LIB_PIN*) aItem;

    TempCopyComponent();

    LIB_PART*      part = GetCurPart();

    // Clear pin flags and mark pins for moving. All pins having the same location
    // orientation, and body style are flagged.
    for( LIB_PIN* pin = part->GetNextPin(); pin; pin = part->GetNextPin( pin ) )
    {
        pin->ClearFlags();

        if( !SynchronizePins() )
            continue;

        if( pin == cur_pin )
            continue;

        if( pin->GetPosition() == cur_pin->GetPosition() &&
            pin->GetOrientation() == cur_pin->GetOrientation() &&
            pin->GetConvert() == cur_pin->GetConvert() )
        {
            pin->SetFlags( IS_LINKED | IS_MOVED );
        }
    }

    cur_pin->SetFlags( IS_LINKED | IS_MOVED );

    MSG_PANEL_ITEMS items;

    cur_pin->GetMsgPanelInfo( m_UserUnits, items );
    SetMsgPanel( items );

    m_canvas->SetMouseCapture( DrawMovePin, AbortPinMove );
}


/*
 * Create a new pin.
 */
LIB_PIN* LIB_PIN_TOOL::CreatePin( const VECTOR2I& aPosition, LIB_PART* aPart )
{
    aPart->ClearStatus();

    LIB_PIN* pin = new LIB_PIN( aPart );

    pin->SetFlags( IS_NEW );

    // Flag pins to consider
    if( m_frame->SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    pin->Move( (wxPoint) aPosition );
    pin->SetLength( GetLastPinLength() );
    pin->SetOrientation( g_LastPinOrient );
    pin->SetType( g_LastPinType );
    pin->SetShape( g_LastPinShape );
    pin->SetNameTextSize( GetLastPinNameSize() );
    pin->SetNumberTextSize( GetLastPinNumSize() );
    pin->SetConvert( g_LastPinCommonConvert ? 0 : m_frame->GetConvert() );
    pin->SetUnit( g_LastPinCommonUnit ? 0 : m_frame->GetUnit() );
    pin->SetVisible( g_LastPinVisible );

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    m_frame->OnEditPin( cmd );

    if( pin->GetFlags() & IS_CANCELLED )
    {
        delete pin;
        pin = nullptr;
    }

    return pin;
}


void LIB_EDIT_FRAME::CreateImagePins( LIB_PIN* aPin )
{
    int      ii;
    LIB_PIN* newPin;

    // if "synchronize pins editing" option is off, do not create any similar pin for other
    // units and/or shapes: each unit is edited regardless other units or body
    if( !SynchronizePins() )
        return;

    if( aPin->GetUnit() == 0 )  // Pin common to all units: no need to create similar pins.
        return;

    // When units are interchangeable, all units are expected to have similar pins
    // at the same position
    // to facilitate pin editing, create pins for all other units for the current body style
    // at the same position as aPin

    for( ii = 1; ii <= aPin->GetParent()->GetUnitCount(); ii++ )
    {
        if( ii == aPin->GetUnit() )
            continue;

        newPin = (LIB_PIN*) aPin->Clone();

        // To avoid mistakes, gives this pin a new pin number because
        // it does no have the save pin number as the master pin
        // Because we do not know the actual number, give it a temporary number
        wxString unknownNum;
        unknownNum.Printf( "%s-U%c", aPin->GetNumber(), wxChar( 'A' + ii - 1 ) );
        newPin->SetNumber( unknownNum );

        newPin->SetUnit( ii );
        aPin->GetParent()->AddDrawItem( newPin );
    }
}


/* aMasterPin is the "template" pin
 * aId is a param to select what should be mofified:
 * - aId = ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
 *          Change pins text name size
 * - aId = ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
 *          Change pins text num size
 * - aId = ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
 *          Change pins length.
 *
 * If aMasterPin is selected ( .m_flag == IS_SELECTED ),
 * only the other selected pins are modified
 */
void LIB_EDIT_FRAME::GlobalSetPins( LIB_PIN* aMasterPin, int aId )

{
    LIB_PART*      part = GetCurPart();

    if( !part || !aMasterPin )
        return;

    if( aMasterPin->Type() != LIB_PIN_T )
        return;

    bool selected = aMasterPin->IsSelected();

    for( LIB_PIN* pin = part->GetNextPin();  pin;  pin = part->GetNextPin( pin ) )
    {
        if( pin->GetConvert() && pin->GetConvert() != m_convert )
            continue;

        // Is it the "selected mode" ?
        if( selected && !pin->IsSelected() )
            continue;

        switch( aId )
        {
        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
            pin->SetNumberTextSize( aMasterPin->GetNumberTextSize() );
            break;

        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
            pin->SetNameTextSize( aMasterPin->GetNameTextSize() );
            break;

        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
            pin->SetLength( aMasterPin->GetLength() );
            break;
        }

        // Clear the flag IS_CHANGED, which was set by previous changes (if any)
        // but not used here.
        pin->ClearFlags( IS_CHANGED );
    }

    // Now changes are made, call OnModify() to validate thes changes and set
    // the global change for UI
    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
}


// Create a new pin based on the previous pin with an incremented pin number.
void LIB_EDIT_FRAME::RepeatPinItem( wxDC* DC, LIB_PIN* SourcePin )
{
    wxString msg;

    LIB_PART*      part = GetCurPart();

    if( !part || !SourcePin || SourcePin->Type() != LIB_PIN_T )
        return;

    LIB_PIN* pin = (LIB_PIN*) SourcePin->Clone();

    pin->ClearFlags();
    pin->SetFlags( IS_NEW );
    wxPoint step;

    switch( pin->GetOrientation() )
    {
    case PIN_UP:    step.x = GetRepeatPinStep();   break;
    case PIN_DOWN:  step.x = GetRepeatPinStep();   break;
    case PIN_LEFT:  step.y = -GetRepeatPinStep();  break;
    case PIN_RIGHT: step.y = -GetRepeatPinStep();  break;
    }

    pin->Move( pin->GetPosition() + step );
    wxString nextName = pin->GetName();
    IncrementLabelMember( nextName, GetRepeatDeltaLabel() );
    pin->SetName( nextName );

    msg = pin->GetNumber();
    IncrementLabelMember( msg, GetRepeatDeltaLabel() );
    pin->SetNumber( msg );

    SetDrawItem( pin );

    if( SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    wxPoint savepos = GetCrossHairPosition();
    m_canvas->CrossHairOff( DC );

    SetCrossHairPosition( wxPoint( pin->GetPosition().x, -pin->GetPosition().y ) );

    // Add this new pin in list, and creates pins for others parts if needed
    SetDrawItem( pin );
    ClearTempCopyComponent();
    PlacePin();
    m_lastDrawItem = pin;

    SetCrossHairPosition( savepos );
    m_canvas->CrossHairOn( DC );

    MSG_PANEL_ITEMS items;
    pin->GetMsgPanelInfo( m_UserUnits, items );
    SetMsgPanel( items );

    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
}


