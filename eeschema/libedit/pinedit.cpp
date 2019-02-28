/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pinedit.cpp
 * @brief Eeschema pin edit code.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <base_units.h>
#include <msgpanel.h>

#include <lib_edit_frame.h>
#include <eeschema_id.h>
#include <class_libentry.h>
#include <lib_pin.h>
#include <general.h>
#include <confirm.h>

#include <../common/dialogs/dialog_display_info_HTML_base.h>
#include <dialog_lib_edit_pin.h>

#include <sch_view.h>

extern void IncrementLabelMember( wxString& name, int aIncrement );


static void AbortPinMove( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void DrawMovePin( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPositon, bool aErase );


static wxPoint OldPos;
static ELECTRICAL_PINTYPE LastPinType   = PIN_INPUT;
static int     LastPinOrient        = PIN_RIGHT;
static GRAPHIC_PINSHAPE LastPinShape = PINSHAPE_LINE;
static bool    LastPinCommonConvert = false;
static bool    LastPinCommonUnit    = false;
static bool    LastPinVisible       = true;

// The -1 is a non-valid value to trigger delayed initialization
static int     LastPinLength        = -1;
static int     LastPinNameSize      = -1;
static int     LastPinNumSize       = -1;

static int GetLastPinLength()
{
    if( LastPinLength == -1 )
        LastPinLength = LIB_EDIT_FRAME::GetDefaultPinLength();

    return LastPinLength;
}

static int GetLastPinNameSize()
{
    if( LastPinNameSize == -1 )
        LastPinNameSize = LIB_EDIT_FRAME::GetPinNameDefaultSize();

    return LastPinNameSize;
}

static int GetLastPinNumSize()
{
    if( LastPinNumSize == -1 )
        LastPinNumSize = LIB_EDIT_FRAME::GetPinNumDefaultSize();

    return LastPinNumSize;
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
    LastPinNameSize = pin->GetNameTextSize();
    LastPinNumSize = pin->GetNumberTextSize();
    LastPinOrient = pin->GetOrientation();
    LastPinLength = pin->GetLength();
    LastPinShape = pin->GetShape();
    LastPinType = pin->GetType();
    LastPinCommonConvert = pin->GetConvert() == 0;
    LastPinCommonUnit = pin->GetUnit() == 0;
    LastPinVisible = pin->IsVisible();
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
        if( pin == cur_pin || newpos != pin->GetPosition() || pin->GetFlags() )
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
        LastPinOrient = cur_pin->GetOrientation();
        LastPinType   = cur_pin->GetType();
        LastPinShape  = cur_pin->GetShape();

        if( SynchronizePins() )
            CreateImagePins( cur_pin );

        m_lastDrawItem = cur_pin;
        part->AddDrawItem( GetDrawItem() );
    }

    // Put linked pins in new position, and clear flags
    for( LIB_PIN* pin = part->GetNextPin();  pin;  pin = part->GetNextPin( pin ) )
    {
        if( pin->GetFlags() == 0 )
            continue;

        pin->Move( cur_pin->GetPosition() );
        pin->ClearFlags();
    }

    SetDrawItem( NULL );

    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
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

    auto view = parent->GetCanvas()->GetView();

    view->Hide( cur_pin );
    view->ClearPreview();
    view->AddToPreview( cur_pin->Clone() );
}


/*
 * Create a new pin.
 */
void LIB_EDIT_FRAME::CreatePin( wxDC* DC )
{
    LIB_PART*      part = GetCurPart();

    if( !part )
        return;

    part->ClearStatus();

    LIB_PIN* pin = new LIB_PIN( part );

    SetDrawItem( pin );

    pin->SetFlags( IS_NEW );
    pin->SetUnit( m_unit );
    pin->SetConvert( m_convert );

    // Flag pins to consider
    if( SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    pin->Move( GetCrossHairPosition( ) );
    pin->SetLength( GetLastPinLength() );
    pin->SetOrientation( LastPinOrient );
    pin->SetType( LastPinType );
    pin->SetShape( LastPinShape );
    pin->SetNameTextSize( GetLastPinNameSize() );
    pin->SetNumberTextSize( GetLastPinNumSize() );
    pin->SetConvert( LastPinCommonConvert ? 0 : m_convert );
    pin->SetUnit( LastPinCommonUnit ? 0 : m_unit );
    pin->SetVisible( LastPinVisible );
    m_canvas->SetIgnoreMouseEvents( true );
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetId( ID_LIBEDIT_EDIT_PIN );
    GetEventHandler()->ProcessEvent( cmd );
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );

    if( pin->GetFlags() & IS_CANCELLED )
    {
        deleteItem( DC, pin );
    }
    else
    {
        ClearTempCopyComponent();
        m_canvas->SetMouseCapture( DrawMovePin, AbortPinMove );
    }
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
    case PIN_UP:
        step.x = GetRepeatPinStep();
        break;

    case PIN_DOWN:
        step.x = GetRepeatPinStep();
        break;

    case PIN_LEFT:
        step.y = - GetRepeatPinStep();
        break;

    case PIN_RIGHT:
        step.y = - GetRepeatPinStep();
        break;
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


// helper function to sort pins by pin num
bool sort_by_pin_number( const LIB_PIN* ref, const LIB_PIN* tst )
{
    int test = ref->GetNumber().Cmp( tst->GetNumber() );

    if( test == 0 )
    {
        test = ref->GetConvert() - tst->GetConvert();
    }

    if( test == 0 )
    {
        test = ref->GetUnit() - tst->GetUnit();
    }

    return test < 0;
}


void LIB_EDIT_FRAME::OnCheckComponent( wxCommandEvent& event )
{
    LIB_PART*      part = GetCurPart();

    if( !part )
        return;

    wxRealPoint curr_grid_size = GetScreen()->GetGridSize();
    const int min_grid_size = 25;
    const int grid_size = KiROUND( curr_grid_size.x );
    const int clamped_grid_size = ( grid_size < min_grid_size ) ? min_grid_size : grid_size;

    LIB_PINS pinList;

    part->GetPins( pinList );

    if( pinList.size() == 0 )
    {
        DisplayInfoMessage( this, _( "No pins!" ) );
        return;
    }

    // Sort pins by pin num, so 2 duplicate pins
    // (pins with the same number) will be consecutive in list
    sort( pinList.begin(), pinList.end(), sort_by_pin_number );

    // Test for duplicates:
    DIALOG_DISPLAY_HTML_TEXT_BASE error_display( this, wxID_ANY,
                                                 _( "Marker Information" ),
                                                 wxDefaultPosition,
                                                 wxSize( 750, 600 ) );

    int dup_error = 0;

    for( unsigned ii = 1; ii < pinList.size(); ii++ )
    {
        LIB_PIN* curr_pin = pinList[ii];
        LIB_PIN* pin      = pinList[ii - 1];

        if( pin->GetNumber() != curr_pin->GetNumber()
            || pin->GetConvert() != curr_pin->GetConvert() )
            continue;

        dup_error++;

        /* TODO I dare someone to find a way to make happy translators on
           this thing! Lorenzo */

        wxString msg = wxString::Format( _(
            "<b>Duplicate pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>"
            " conflicts with pin %s \"%s\" at location <b>(%.3f, %.3f)</b>" ),
            GetChars( curr_pin->GetNumber() ),
            GetChars( curr_pin->GetName() ),
            curr_pin->GetPosition().x / 1000.0,
            -curr_pin->GetPosition().y / 1000.0,
            GetChars( pin->GetNumber() ),
            GetChars( pin->GetName() ),
            pin->GetPosition().x / 1000.0,
            -pin->GetPosition().y / 1000.0
            );

        if( part->GetUnitCount() > 1 )
        {
            msg += wxString::Format( _( " in units %c and %c" ),
                                     'A' + curr_pin->GetUnit() - 1,
                                     'A' + pin->GetUnit() - 1 );
        }

        if( m_showDeMorgan )
        {
            if( curr_pin->GetConvert() )
                msg += _( "  of converted" );
            else
                msg += _( "  of normal" );
        }

        msg += wxT( ".<br>" );

        error_display.m_htmlWindow->AppendToPage( msg );
    }

    // Test for off grid pins:
    int offgrid_error = 0;

    for( unsigned ii = 0; ii < pinList.size(); ii++ )
    {
        LIB_PIN* pin = pinList[ii];

        if( ( (pin->GetPosition().x % clamped_grid_size) == 0 ) &&
            ( (pin->GetPosition().y % clamped_grid_size) == 0 ) )
            continue;

        // "pin" is off grid here.
        offgrid_error++;

        wxString msg = wxString::Format( _(
            "<b>Off grid pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>" ),
            GetChars( pin->GetNumber() ),
            GetChars( pin->GetName() ),
            pin->GetPosition().x / 1000.0,
            -pin->GetPosition().y / 1000.0
            );

        if( part->GetUnitCount() > 1 )
        {
            msg += wxString::Format( _( " in symbol %c" ), 'A' + pin->GetUnit() - 1 );
        }

        if( m_showDeMorgan )
        {
            if( pin->GetConvert() )
                msg += _( "  of converted" );
            else
                msg += _( "  of normal" );
        }

        msg += wxT( ".<br>" );

        error_display.m_htmlWindow->AppendToPage( msg );
    }

    if( !dup_error && !offgrid_error )
        DisplayInfoMessage( this, _( "No off grid or duplicate pins were found." ) );
    else
        error_display.ShowModal();
}
