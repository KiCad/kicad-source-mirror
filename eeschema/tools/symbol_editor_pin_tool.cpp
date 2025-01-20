/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tools/ee_selection_tool.h>
#include <symbol_edit_frame.h>
#include <sch_commit.h>
#include <confirm.h>
#include <ee_actions.h>
#include <dialogs/dialog_pin_properties.h>
#include <settings/settings_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <pgm_base.h>
#include <wx/log.h>
#include "symbol_editor_pin_tool.h"


static ELECTRICAL_PINTYPE g_LastPinType            = ELECTRICAL_PINTYPE::PT_INPUT;
static PIN_ORIENTATION    g_LastPinOrient          = PIN_ORIENTATION::PIN_RIGHT;
static GRAPHIC_PINSHAPE   g_LastPinShape           = GRAPHIC_PINSHAPE::LINE;
static bool               g_LastPinCommonBodyStyle = false;
static bool               g_LastPinCommonUnit      = false;
static bool               g_LastPinVisible         = true;

// The -1 is a non-valid value to trigger delayed initialization
static int                g_LastPinLength          = -1;
static int                g_LastPinNameSize        = -1;
static int                g_LastPinNumSize         = -1;

static int GetLastPinLength()
{
    if( g_LastPinLength == -1 )
    {
        auto* settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
        g_LastPinLength = schIUScale.MilsToIU( settings->m_Defaults.pin_length );
    }

    return g_LastPinLength;
}

static int GetLastPinNameSize()
{
    if( g_LastPinNameSize == -1 )
    {
        auto* settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
        g_LastPinNameSize = schIUScale.MilsToIU( settings->m_Defaults.pin_name_size );
    }

    return g_LastPinNameSize;
}

static int GetLastPinNumSize()
{
    if( g_LastPinNumSize == -1 )
    {
        auto* settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
        g_LastPinNumSize = schIUScale.MilsToIU( settings->m_Defaults.pin_num_size );
    }

    return g_LastPinNumSize;
}


extern bool IncrementLabelMember( wxString& name, int aIncrement );


SYMBOL_EDITOR_PIN_TOOL::SYMBOL_EDITOR_PIN_TOOL() :
        EE_TOOL_BASE<SYMBOL_EDIT_FRAME>( "eeschema.PinEditing" )
{
}


bool SYMBOL_EDITOR_PIN_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto canEdit =
            [&]( const SELECTION& sel )
            {
                SYMBOL_EDIT_FRAME* editor = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
                wxCHECK( editor, false );

                return editor->IsSymbolEditable() && !editor->IsSymbolAlias();
            };

    auto singlePinCondition = EE_CONDITIONS::Count( 1 ) && EE_CONDITIONS::OnlyTypes( { LIB_PIN_T } );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddSeparator( 250 );
    selToolMenu.AddItem( EE_ACTIONS::pushPinLength,    canEdit && singlePinCondition, 250 );
    selToolMenu.AddItem( EE_ACTIONS::pushPinNameSize,  canEdit && singlePinCondition, 250 );
    selToolMenu.AddItem( EE_ACTIONS::pushPinNumSize,   canEdit && singlePinCondition, 250 );

    return true;
}


bool SYMBOL_EDITOR_PIN_TOOL::EditPinProperties( LIB_PIN* aPin )
{
    LIB_PIN               original_pin( *aPin );
    DIALOG_PIN_PROPERTIES dlg( m_frame, aPin );
    SCH_COMMIT            commit( m_frame );

    if( aPin->GetEditFlags() == 0 )
        commit.Modify( aPin->GetParent() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    if( !aPin->IsNew() && m_frame->SynchronizePins() && aPin->GetParent() )
    {
        LIB_PINS pinList;
        aPin->GetParent()->GetPins( pinList );

        // a pin can have a unit id = 0 (common to all units) to unit count
        // So we need a buffer size = GetUnitCount()+1 to store a value in a vector
        // when using the unit id of a pin as index
        std::vector<bool> got_unit( aPin->GetParent()->GetUnitCount() + 1 );

        got_unit[static_cast<size_t>(aPin->GetUnit())] = true;

        for( LIB_PIN* other : pinList )
        {
            if( other == aPin )
                continue;

            /// Only change one pin per unit to allow stacking pins
            /// If you change all units on the position, then pins are not
            /// uniquely editable
            if( got_unit[static_cast<size_t>( other->GetUnit() )] )
                continue;

            if( other->GetPosition() == original_pin.GetPosition()
                && other->GetOrientation() == original_pin.GetOrientation()
                && other->GetType() == original_pin.GetType()
                && other->IsVisible() == original_pin.IsVisible()
                && other->GetName() == original_pin.GetName() )
            {
                if( aPin->GetBodyStyle() == 0 )
                {
                    if( !aPin->GetUnit() || other->GetUnit() == aPin->GetUnit() )
                        aPin->GetParent()->RemoveDrawItem( other );
                }

                if( other->GetBodyStyle() == aPin->GetBodyStyle() )
                {
                    other->ChangeLength( aPin->GetLength() );

                    // Must be done after ChangeLenght(), which can alter the position
                    other->SetPosition( aPin->GetPosition() );

                    other->SetShape( aPin->GetShape() );
                }

                if( aPin->GetUnit() == 0 )
                {
                    if( !aPin->GetBodyStyle() || other->GetBodyStyle() == aPin->GetBodyStyle() )
                        aPin->GetParent()->RemoveDrawItem( other );
                }

                other->SetOrientation( aPin->GetOrientation() );
                other->SetType( aPin->GetType() );
                other->SetVisible( aPin->IsVisible() );
                other->SetName( aPin->GetName() );
                other->SetNameTextSize( aPin->GetNameTextSize() );
                other->SetNumberTextSize( aPin->GetNumberTextSize() );

                got_unit[static_cast<size_t>( other->GetUnit() )] = true;
            }
        }
    }

    commit.Push( _( "Edit Pin Properties" ) );

    std::vector<MSG_PANEL_ITEM> items;
    aPin->GetMsgPanelInfo( m_frame, items );
    m_frame->SetMsgPanel( items );

    // Save the pin properties to use for the next new pin.
    g_LastPinNameSize = aPin->GetNameTextSize();
    g_LastPinNumSize = aPin->GetNumberTextSize();
    g_LastPinOrient = aPin->GetOrientation();
    g_LastPinLength = aPin->GetLength();
    g_LastPinShape = aPin->GetShape();
    g_LastPinType = aPin->GetType();
    g_LastPinCommonBodyStyle = aPin->GetBodyStyle() == 0;
    g_LastPinCommonUnit = aPin->GetUnit() == 0;
    g_LastPinVisible = aPin->IsVisible();

    return true;
}


bool SYMBOL_EDITOR_PIN_TOOL::PlacePin( LIB_PIN* aPin )
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();
    bool        ask_for_pin = true;   // Test for another pin in same position in other units

    std::vector<LIB_PIN*> pins = symbol->GetAllLibPins();

    for( LIB_PIN* test : pins )
    {
        if( test == aPin || aPin->GetPosition() != test->GetPosition() || test->GetEditFlags() )
            continue;

        // test for same body style
        if( test->GetBodyStyle() && test->GetBodyStyle() != aPin->GetBodyStyle() )
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
                if( aPin->IsNew() && !aPin->HasFlag( IS_PASTED ) )
                    delete aPin;

                return false;
            }
            else
            {
                ask_for_pin = false;
            }
        }
    }

    if( aPin->IsNew() && !aPin->HasFlag( IS_PASTED ) )
    {
        g_LastPinOrient = aPin->GetOrientation();
        g_LastPinType   = aPin->GetType();
        g_LastPinShape  = aPin->GetShape();

        if( m_frame->SynchronizePins() )
            CreateImagePins( aPin );

        symbol->AddDrawItem( aPin );
        aPin->ClearFlags( IS_NEW );
    }

    // Put linked pins in new position, and clear flags
    for( LIB_PIN* pin : pins )
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
LIB_PIN* SYMBOL_EDITOR_PIN_TOOL::CreatePin( const VECTOR2I& aPosition, LIB_SYMBOL* aSymbol )
{
    aSymbol->ClearTempFlags();

    LIB_PIN* pin = new LIB_PIN( aSymbol );

    pin->SetFlags( IS_NEW );

    // Flag pins to consider
    if( m_frame->SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    pin->MoveTo( aPosition );
    pin->SetLength( GetLastPinLength() );
    pin->SetOrientation( g_LastPinOrient );
    pin->SetType( g_LastPinType );
    pin->SetShape( g_LastPinShape );
    pin->SetNameTextSize( GetLastPinNameSize() );
    pin->SetNumberTextSize( GetLastPinNumSize() );
    pin->SetBodyStyle( g_LastPinCommonBodyStyle ? 0 : m_frame->GetBodyStyle() );
    pin->SetUnit( g_LastPinCommonUnit ? 0 : m_frame->GetUnit() );
    pin->SetVisible( g_LastPinVisible );

    if( !EditPinProperties( pin ) )
    {
        delete pin;
        pin = nullptr;
    }

    return pin;
}


void SYMBOL_EDITOR_PIN_TOOL::CreateImagePins( LIB_PIN* aPin )
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

        newPin = (LIB_PIN*) aPin->Duplicate();

        // To avoid mistakes, gives this pin a new pin number because
        // it does no have the save pin number as the master pin
        // Because we do not know the actual number, give it a temporary number
        wxString unknownNum;
        unknownNum.Printf( "%s-U%c", aPin->GetNumber(), wxChar( 'A' + ii - 1 ) );
        newPin->SetNumber( unknownNum );

        newPin->SetUnit( ii );

        try
        {
            aPin->GetParent()->AddDrawItem( newPin );
        }
        catch( const boost::bad_pointer& e )
        {
            wxLogError( "Cannot add new pin to symbol.  Boost pointer error %s occurred.",
                        e.what() );
            delete newPin;
            return;
        }

        newPin->ClearFlags( IS_NEW );
    }
}


int SYMBOL_EDITOR_PIN_TOOL::PushPinProperties( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL*   symbol = m_frame->GetCurSymbol();
    EE_SELECTION& selection = m_selectionTool->GetSelection();
    LIB_PIN*      sourcePin = dynamic_cast<LIB_PIN*>( selection.Front() );

    if( !sourcePin )
        return 0;

    saveCopyInUndoList( symbol, UNDO_REDO::LIBEDIT );
    std::vector<LIB_PIN*> pins = symbol->GetAllLibPins();

    for( LIB_PIN* pin : pins )
    {
        if( pin == sourcePin )
            continue;

        if( aEvent.IsAction( &EE_ACTIONS::pushPinLength ) )
        {
            if( !pin->GetBodyStyle() || pin->GetBodyStyle() == m_frame->GetBodyStyle() )
                pin->ChangeLength( sourcePin->GetLength() );
        }
        else if( aEvent.IsAction( &EE_ACTIONS::pushPinNameSize ) )
        {
            pin->SetNameTextSize( sourcePin->GetNameTextSize() );
        }
        else if( aEvent.IsAction( &EE_ACTIONS::pushPinNumSize ) )
        {
            pin->SetNumberTextSize( sourcePin->GetNumberTextSize() );
        }
    }

    m_frame->RebuildView();
    m_frame->OnModify();

    return 0;
}


// Create a new pin based on the previous pin with an incremented pin number.
LIB_PIN* SYMBOL_EDITOR_PIN_TOOL::RepeatPin( const LIB_PIN* aSourcePin )
{
    SCH_COMMIT  commit( m_frame );
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    commit.Modify( symbol );

    LIB_PIN* pin = (LIB_PIN*) aSourcePin->Duplicate();
    VECTOR2I step;

    pin->ClearFlags();
    pin->SetFlags( IS_NEW );

    auto* settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();

    switch( pin->GetOrientation() )
    {
    case PIN_ORIENTATION::PIN_UP:    step.x = schIUScale.MilsToIU(settings->m_Repeat.pin_step);   break;
    case PIN_ORIENTATION::PIN_DOWN:  step.x = schIUScale.MilsToIU(settings->m_Repeat.pin_step);   break;
    case PIN_ORIENTATION::PIN_LEFT:  step.y = schIUScale.MilsToIU(-settings->m_Repeat.pin_step);  break;
    case PIN_ORIENTATION::PIN_RIGHT: step.y = schIUScale.MilsToIU(-settings->m_Repeat.pin_step);  break;
    }

    pin->Offset( step );

    wxString nextName = pin->GetName();
    IncrementLabelMember( nextName, settings->m_Repeat.label_delta );
    pin->SetName( nextName );

    wxString nextNumber = pin->GetNumber();
    IncrementLabelMember( nextNumber, settings->m_Repeat.label_delta );
    pin->SetNumber( nextNumber );

    if( m_frame->SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    if( PlacePin( pin ) )
    {
        commit.Push( _( "Repeat Pin" ) );
        return pin;
    }

    return nullptr;
}


void SYMBOL_EDITOR_PIN_TOOL::setTransitions()
{
    Go( &SYMBOL_EDITOR_PIN_TOOL::PushPinProperties,    EE_ACTIONS::pushPinLength.MakeEvent() );
    Go( &SYMBOL_EDITOR_PIN_TOOL::PushPinProperties,    EE_ACTIONS::pushPinNameSize.MakeEvent() );
    Go( &SYMBOL_EDITOR_PIN_TOOL::PushPinProperties,    EE_ACTIONS::pushPinNumSize.MakeEvent() );
}
