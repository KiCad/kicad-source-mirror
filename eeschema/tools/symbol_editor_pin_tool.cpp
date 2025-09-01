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

#include "symbol_editor_pin_tool.h"

#include <sch_commit.h>
#include <kidialog.h>
#include <sch_actions.h>
#include <dialogs/dialog_pin_properties.h>
#include <increment.h>
#include <settings/settings_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <pgm_base.h>
#include <wx/debug.h>


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
        if( SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
            g_LastPinLength = schIUScale.MilsToIU( cfg->m_Defaults.pin_length );
    }

    return g_LastPinLength;
}

static int GetLastPinNameSize()
{
    if( g_LastPinNameSize == -1 )
    {
        if( SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
            g_LastPinNameSize = schIUScale.MilsToIU( cfg->m_Defaults.pin_name_size );
    }

    return g_LastPinNameSize;
}

static int GetLastPinNumSize()
{
    if( g_LastPinNumSize == -1 )
    {
        if( SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
            g_LastPinNumSize = schIUScale.MilsToIU( cfg->m_Defaults.pin_num_size );
    }

    return g_LastPinNumSize;
}


SYMBOL_EDITOR_PIN_TOOL::SYMBOL_EDITOR_PIN_TOOL() :
        SCH_TOOL_BASE<SYMBOL_EDIT_FRAME>( "eeschema.PinEditing" )
{
}


bool SYMBOL_EDITOR_PIN_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    auto canEdit =
            [this]( const SELECTION& sel )
            {
                SYMBOL_EDIT_FRAME* editor = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );

                return editor && editor->IsSymbolEditable() && !editor->IsSymbolAlias();
            };

    static const std::vector<KICAD_T> pinTypes = { SCH_PIN_T };

    auto singlePinCondition = SCH_CONDITIONS::Count( 1 ) && SCH_CONDITIONS::OnlyTypes( pinTypes );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddSeparator( 250 );
    selToolMenu.AddItem( SCH_ACTIONS::pushPinLength,    canEdit && singlePinCondition, 250 );
    selToolMenu.AddItem( SCH_ACTIONS::pushPinNameSize,  canEdit && singlePinCondition, 250 );
    selToolMenu.AddItem( SCH_ACTIONS::pushPinNumSize,   canEdit && singlePinCondition, 250 );

    return true;
}


bool SYMBOL_EDITOR_PIN_TOOL::EditPinProperties( SCH_PIN* aPin, bool aFocusPinNumber )
{
    SCH_PIN               original_pin( *aPin );
    DIALOG_PIN_PROPERTIES dlg( m_frame, aPin, aFocusPinNumber );
    SCH_COMMIT            commit( m_frame );
    LIB_SYMBOL*           parentSymbol = static_cast<LIB_SYMBOL*>( aPin->GetParentSymbol() );

    if( aPin->GetEditFlags() == 0 )
        commit.Modify( parentSymbol, m_frame->GetScreen() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    if( !aPin->IsNew() && m_frame->SynchronizePins() && parentSymbol )
    {
        // a pin can have a unit id = 0 (common to all units) to unit count
        // So we need a buffer size = GetUnitCount()+1 to store a value in a vector
        // when using the unit id of a pin as index
        std::vector<bool> got_unit( parentSymbol->GetUnitCount() + 1 );

        got_unit[static_cast<size_t>(aPin->GetUnit())] = true;

        for( SCH_PIN* other : parentSymbol->GetPins() )
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
                        parentSymbol->RemoveDrawItem( other );
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
                        parentSymbol->RemoveDrawItem( other );
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


bool SYMBOL_EDITOR_PIN_TOOL::PlacePin( SCH_COMMIT* aCommit, SCH_PIN* aPin )
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();
    bool        ask_for_pin = true;   // Test for another pin in same position in other units

    std::vector<SCH_PIN*> pins = symbol->GetPins();

    for( SCH_PIN* test : pins )
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
            dlg.SetExtendedMessage( _( "Disable the 'Synchronized Pins Mode' option to avoid this message." ) );
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
            CreateImagePins( aCommit, aPin );

        symbol->AddDrawItem( aPin );
        aPin->ClearFlags( IS_NEW );
    }

    // Put linked pins in new position, and clear flags
    for( SCH_PIN* pin : pins )
    {
        if( ( pin->GetEditFlags() & IS_LINKED ) == 0 )
            continue;

        pin->SetPosition( aPin->GetPosition() );
        pin->ClearFlags();
    }

    m_frame->RebuildView();
    m_frame->OnModify();

    return true;
}


/*
 * Create a new pin.
 */
SCH_PIN* SYMBOL_EDITOR_PIN_TOOL::CreatePin( const VECTOR2I& aPosition, LIB_SYMBOL* aSymbol )
{
    aSymbol->ClearTempFlags();

    SCH_PIN* pin = new SCH_PIN( aSymbol );

    pin->SetFlags( IS_NEW );

    // Flag pins to consider
    if( m_frame->SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    pin->SetPosition( aPosition );
    pin->SetLength( GetLastPinLength() );
    pin->SetOrientation( g_LastPinOrient );
    pin->SetType( g_LastPinType );
    pin->SetShape( g_LastPinShape );
    pin->SetNameTextSize( GetLastPinNameSize() );
    pin->SetNumberTextSize( GetLastPinNumSize() );
    pin->SetBodyStyle( g_LastPinCommonBodyStyle ? 0 : m_frame->GetBodyStyle() );
    pin->SetUnit( g_LastPinCommonUnit ? 0 : m_frame->GetUnit() );
    pin->SetVisible( g_LastPinVisible );

    if( !EditPinProperties( pin, false ) )
    {
        delete pin;
        pin = nullptr;
    }

    return pin;
}


void SYMBOL_EDITOR_PIN_TOOL::CreateImagePins( SCH_COMMIT* aCommit, SCH_PIN* aPin )
{
    int      ii;
    SCH_PIN* newPin;

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

    for( ii = 1; ii <= aPin->GetParentSymbol()->GetUnitCount(); ii++ )
    {
        if( ii == aPin->GetUnit() )
            continue;

        // Already called Modify() on parent symbol; no need for Modify() calls on individual items
        SCH_COMMIT dummy( m_toolMgr );
        newPin = static_cast<SCH_PIN*>( aPin->Duplicate( true, &dummy ) );

        // To avoid mistakes, gives this pin a new pin number because
        // it does no have the save pin number as the master pin
        // Because we do not know the actual number, give it a temporary number
        wxString unknownNum;
        unknownNum.Printf( "%s-U%c", aPin->GetNumber(), wxChar( 'A' + ii - 1 ) );
        newPin->SetNumber( unknownNum );

        newPin->SetUnit( ii );

        try
        {
            LIB_SYMBOL* symbol = static_cast<LIB_SYMBOL*>( aPin->GetParentSymbol() );
            symbol->AddDrawItem( newPin );
        }
        catch( const boost::bad_pointer& e )
        {
            wxFAIL_MSG( wxString::Format( wxT( "Boost pointer exception occurred: %s" ), e.what() ));
            delete newPin;
            return;
        }

        newPin->ClearFlags( IS_NEW );
    }
}


int SYMBOL_EDITOR_PIN_TOOL::PushPinProperties( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL*    symbol = m_frame->GetCurSymbol();
    SCH_SELECTION& selection = m_selectionTool->GetSelection();
    SCH_PIN*       sourcePin = dynamic_cast<SCH_PIN*>( selection.Front() );

    if( !sourcePin )
        return 0;

    saveCopyInUndoList( symbol, UNDO_REDO::LIBEDIT );

    for( SCH_PIN* pin : symbol->GetPins() )
    {
        if( pin == sourcePin )
            continue;

        if( aEvent.IsAction( &SCH_ACTIONS::pushPinLength ) )
        {
            if( !pin->GetBodyStyle() || pin->GetBodyStyle() == m_frame->GetBodyStyle() )
                pin->ChangeLength( sourcePin->GetLength() );
        }
        else if( aEvent.IsAction( &SCH_ACTIONS::pushPinNameSize ) )
        {
            pin->SetNameTextSize( sourcePin->GetNameTextSize() );
        }
        else if( aEvent.IsAction( &SCH_ACTIONS::pushPinNumSize ) )
        {
            pin->SetNumberTextSize( sourcePin->GetNumberTextSize() );
        }
    }

    m_frame->RebuildView();
    m_frame->OnModify();

    return 0;
}


// Create a new pin based on the previous pin with an incremented pin number.
SCH_PIN* SYMBOL_EDITOR_PIN_TOOL::RepeatPin( const SCH_PIN* aSourcePin )
{
    SCH_COMMIT  commit( m_frame );
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    commit.Modify( symbol, m_frame->GetScreen() );

    SCH_PIN* pin = static_cast<SCH_PIN*>( aSourcePin->Duplicate( true, &commit ) );

    pin->ClearFlags();
    pin->SetFlags( IS_NEW );

    if( SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
    {
        VECTOR2I step;

        switch( pin->GetOrientation() )
        {
        default:
        case PIN_ORIENTATION::PIN_RIGHT: step.y = schIUScale.MilsToIU( cfg->m_Repeat.pin_step ); break;
        case PIN_ORIENTATION::PIN_UP:    step.x = schIUScale.MilsToIU( cfg->m_Repeat.pin_step ); break;
        case PIN_ORIENTATION::PIN_DOWN:  step.x = schIUScale.MilsToIU( cfg->m_Repeat.pin_step) ; break;
        case PIN_ORIENTATION::PIN_LEFT:  step.y = schIUScale.MilsToIU( cfg->m_Repeat.pin_step ); break;
        }

        pin->Move( step );

        wxString nextName = pin->GetName();
        IncrementString( nextName, cfg->m_Repeat.label_delta );
        pin->SetName( nextName );

        wxString nextNumber = pin->GetNumber();
        IncrementString( nextNumber, cfg->m_Repeat.label_delta );
        pin->SetNumber( nextNumber );
    }

    if( m_frame->SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    if( PlacePin( &commit, pin ) )
    {
        commit.Push( _( "Repeat Pin" ) );
        return pin;
    }

    return nullptr;
}


void SYMBOL_EDITOR_PIN_TOOL::setTransitions()
{
    Go( &SYMBOL_EDITOR_PIN_TOOL::PushPinProperties,    SCH_ACTIONS::pushPinLength.MakeEvent() );
    Go( &SYMBOL_EDITOR_PIN_TOOL::PushPinProperties,    SCH_ACTIONS::pushPinNameSize.MakeEvent() );
    Go( &SYMBOL_EDITOR_PIN_TOOL::PushPinProperties,    SCH_ACTIONS::pushPinNumSize.MakeEvent() );
}
