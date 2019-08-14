/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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
#include <tools/ee_selection_tool.h>
#include <lib_edit_frame.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <ee_actions.h>
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
        EE_TOOL_BASE<LIB_EDIT_FRAME>( "eeschema.PinEditing" )
{
}


bool LIB_PIN_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto singlePinCondition = EE_CONDITIONS::Count( 1 ) && EE_CONDITIONS::OnlyType( LIB_PIN_T );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddSeparator( 400 );
    selToolMenu.AddItem( EE_ACTIONS::pushPinLength,    singlePinCondition, 400 );
    selToolMenu.AddItem( EE_ACTIONS::pushPinNameSize,  singlePinCondition, 400 );
    selToolMenu.AddItem( EE_ACTIONS::pushPinNumSize,   singlePinCondition, 400 );

    return true;
}


bool LIB_PIN_TOOL::EditPinProperties( LIB_PIN* aPin )
{
    aPin->EnableEditMode( true, !m_frame->SynchronizePins() );

    DIALOG_LIB_EDIT_PIN dlg( m_frame, aPin );

    if( dlg.ShowModal() == wxID_CANCEL )
    {
        return false;
    }

    m_frame->RefreshItem( aPin );
    m_frame->OnModify( );

    MSG_PANEL_ITEMS items;
    aPin->GetMsgPanelInfo( m_frame->GetUserUnits(), items );
    m_frame->SetMsgPanel( items );

    aPin->EnableEditMode( false );

    // Save the pin properties to use for the next new pin.
    g_LastPinNameSize = aPin->GetNameTextSize();
    g_LastPinNumSize = aPin->GetNumberTextSize();
    g_LastPinOrient = aPin->GetOrientation();
    g_LastPinLength = aPin->GetLength();
    g_LastPinShape = aPin->GetShape();
    g_LastPinType = aPin->GetType();
    g_LastPinCommonConvert = aPin->GetConvert() == 0;
    g_LastPinCommonUnit = aPin->GetUnit() == 0;
    g_LastPinVisible = aPin->IsVisible();

    return true;
}


bool LIB_PIN_TOOL::PlacePin( LIB_PIN* aPin )
{
    LIB_PART* part = m_frame->GetCurPart();
    bool      ask_for_pin = true;   // Test for another pin in same position in other units

    for( LIB_PIN* test = part->GetNextPin(); test; test = part->GetNextPin( test ) )
    {
        if( test == aPin || aPin->GetPosition() != test->GetPosition() || test->GetEditFlags() )
            continue;

        // test for same body style
        if( test->GetConvert() && test->GetConvert() != aPin->GetConvert() )
            continue;

        if( ask_for_pin && m_frame->SynchronizePins() )
        {
            wxString msg;
            msg.Printf( _( "This position is already occupied by another pin, in unit %d." ),
                        test->GetUnit() );

            KIDIALOG dlg( m_frame, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            dlg.SetOKLabel( _( "Place Pin Anyway" ) );
            dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            bool status = dlg.ShowModal() == wxID_OK;

            if( !status )
            {
                if( aPin->IsNew() )
                    delete aPin;

                return false;
            }
            else
            {
                ask_for_pin = false;
            }
        }
    }

    if( aPin->IsNew() && !( aPin->GetFlags() & IS_PASTED ) )
    {
        g_LastPinOrient = aPin->GetOrientation();
        g_LastPinType   = aPin->GetType();
        g_LastPinShape  = aPin->GetShape();

        if( m_frame->SynchronizePins() )
            CreateImagePins( aPin );

        part->AddDrawItem( aPin );
        aPin->ClearFlags( IS_NEW );
    }

    // Put linked pins in new position, and clear flags
    for( LIB_PIN* pin = part->GetNextPin();  pin;  pin = part->GetNextPin( pin ) )
    {
        if( ( pin->GetEditFlags() & IS_LINKED ) == 0 )
            continue;

        pin->MoveTo( aPin->GetPosition() );
        pin->ClearFlags();
    }

    m_frame->RebuildView();
    m_frame->OnModify();

    return true;
}


/*
 * Create a new pin.
 */
LIB_PIN* LIB_PIN_TOOL::CreatePin( const VECTOR2I& aPosition, LIB_PART* aPart )
{
    aPart->ClearTempFlags();

    LIB_PIN* pin = new LIB_PIN( aPart );

    pin->SetFlags( IS_NEW );

    // Flag pins to consider
    if( m_frame->SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    pin->MoveTo((wxPoint) aPosition );
    pin->SetLength( GetLastPinLength() );
    pin->SetOrientation( g_LastPinOrient );
    pin->SetType( g_LastPinType );
    pin->SetShape( g_LastPinShape );
    pin->SetNameTextSize( GetLastPinNameSize() );
    pin->SetNumberTextSize( GetLastPinNumSize() );
    pin->SetConvert( g_LastPinCommonConvert ? 0 : m_frame->GetConvert() );
    pin->SetUnit( g_LastPinCommonUnit ? 0 : m_frame->GetUnit() );
    pin->SetVisible( g_LastPinVisible );

    if( !EditPinProperties( pin ) )
    {
        delete pin;
        pin = nullptr;
    }

    return pin;
}


void LIB_PIN_TOOL::CreateImagePins( LIB_PIN* aPin )
{
    int      ii;
    LIB_PIN* newPin;

    // if "synchronize pins editing" option is off, do not create any similar pin for other
    // units and/or shapes: each unit is edited regardless other units or body
    if( !m_frame->SynchronizePins() )
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


int LIB_PIN_TOOL::PushPinProperties( const TOOL_EVENT& aEvent )
{
    LIB_PART*     part = m_frame->GetCurPart();
    EE_SELECTION& selection = m_selectionTool->GetSelection();
    LIB_PIN*      sourcePin = dynamic_cast<LIB_PIN*>( selection.Front() );

    if( !sourcePin )
        return 0;

    saveCopyInUndoList( part, UR_LIBEDIT );

    for( LIB_PIN* pin = part->GetNextPin();  pin;  pin = part->GetNextPin( pin ) )
    {
        if( pin->GetConvert() && pin->GetConvert() != m_frame->GetConvert() )
            continue;

        if( pin == sourcePin )
            continue;

        if( aEvent.IsAction( &EE_ACTIONS::pushPinLength ) )
            pin->SetLength( sourcePin->GetLength() );
        else if( aEvent.IsAction( &EE_ACTIONS::pushPinNameSize ) )
            pin->SetNameTextSize( sourcePin->GetNameTextSize() );
        else if( aEvent.IsAction( &EE_ACTIONS::pushPinNumSize ) )
            pin->SetNumberTextSize( sourcePin->GetNumberTextSize() );
    }

    m_frame->RebuildView();
    m_frame->OnModify();

    return 0;
}


// Create a new pin based on the previous pin with an incremented pin number.
LIB_PIN* LIB_PIN_TOOL::RepeatPin( const LIB_PIN* aSourcePin )
{
    LIB_PIN* pin = (LIB_PIN*) aSourcePin->Clone();
    wxPoint  step;

    pin->ClearFlags();
    pin->SetFlags( IS_NEW );

    switch( pin->GetOrientation() )
    {
    case PIN_UP:    step.x = m_frame->GetRepeatPinStep();   break;
    case PIN_DOWN:  step.x = m_frame->GetRepeatPinStep();   break;
    case PIN_LEFT:  step.y = -m_frame->GetRepeatPinStep();  break;
    case PIN_RIGHT: step.y = -m_frame->GetRepeatPinStep();  break;
    }

    pin->Offset( step );

    wxString nextName = pin->GetName();
    IncrementLabelMember( nextName, m_frame->GetRepeatDeltaLabel() );
    pin->SetName( nextName );

    wxString nextNumber = pin->GetNumber();
    IncrementLabelMember( nextNumber, m_frame->GetRepeatDeltaLabel() );
    pin->SetNumber( nextNumber );

    if( m_frame->SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    PlacePin( pin );

    return pin;
}


void LIB_PIN_TOOL::setTransitions()
{
    Go( &LIB_PIN_TOOL::PushPinProperties,    EE_ACTIONS::pushPinLength.MakeEvent() );
    Go( &LIB_PIN_TOOL::PushPinProperties,    EE_ACTIONS::pushPinNameSize.MakeEvent() );
    Go( &LIB_PIN_TOOL::PushPinProperties,    EE_ACTIONS::pushPinNumSize.MakeEvent() );
}

