/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <class_drawpanel.h>
#include <confirm.h>
#include <class_sch_screen.h>
#include <base_units.h>
#include <msgpanel.h>

#include <libeditframe.h>
#include <eeschema_id.h>
#include <class_libentry.h>
#include <lib_pin.h>
#include <general.h>
#include <protos.h>

#include <../common/dialogs/dialog_display_info_HTML_base.h>
#include <dialog_lib_edit_pin.h>


extern void IncrementLabelMember( wxString& name );


static void AbortPinMove( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void DrawMovePin( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPositon, bool aErase );


static wxPoint OldPos;
static wxPoint PinPreviousPos;
static int     LastPinType          = PIN_INPUT;
static int     LastPinOrient        = PIN_RIGHT;
static int     LastPinShape         = NONE;
static int     LastPinNameSize      = DEFAULT_TEXT_SIZE;
static int     LastPinNumSize       = DEFAULT_TEXT_SIZE;
static bool    LastPinCommonConvert = false;
static bool    LastPinCommonUnit    = false;
static bool    LastPinVisible       = true;

// The -1 is a non-valid value to trigger delayed initialization
static int     LastPinLength        = -1;

static int GetLastPinLength()
{
    if( LastPinLength == -1 )
        LastPinLength = GetDefaultPinLength();

    return LastPinLength;
}

void LIB_EDIT_FRAME::OnEditPin( wxCommandEvent& event )
{
    if( m_drawItem == NULL || m_drawItem->Type() != LIB_PIN_T )
        return;

    STATUS_FLAGS item_flags = m_drawItem->GetFlags(); // save flags to restore them after editing
    LIB_PIN* pin = (LIB_PIN*) m_drawItem;

    DIALOG_LIB_EDIT_PIN dlg( this, pin );

    wxString units = GetUnitsLabel( g_UserUnit );
    dlg.SetOrientationList( LIB_PIN::GetOrientationNames(), LIB_PIN::GetOrientationSymbols() );
    dlg.SetOrientation( LIB_PIN::GetOrientationCodeIndex( pin->GetOrientation() ) );
    dlg.SetStyleList( LIB_PIN::GetStyleNames(), LIB_PIN::GetStyleSymbols() );
    dlg.SetStyle( LIB_PIN::GetStyleCodeIndex( pin->GetShape() ) );
    dlg.SetElectricalTypeList( LIB_PIN::GetElectricalTypeNames(),
                               LIB_PIN::GetElectricalTypeSymbols() );
    dlg.SetElectricalType( pin->GetType() );
    dlg.SetName( pin->GetName() );
    dlg.SetNameTextSize( StringFromValue( g_UserUnit, pin->GetNameTextSize() ) );
    dlg.SetNameTextSizeUnits( units );
    dlg.SetPadName( pin->GetNumberString() );
    dlg.SetPadNameTextSize( StringFromValue( g_UserUnit, pin->GetNumberTextSize() ) );

    dlg.SetPadNameTextSizeUnits( units );
    dlg.SetLength( StringFromValue( g_UserUnit, pin->GetLength() ) );
    dlg.SetLengthUnits( units );
    dlg.SetAddToAllParts( pin->GetUnit() == 0 );
    dlg.SetAddToAllBodyStyles( pin->GetConvert() == 0 );
    dlg.SetVisible( pin->IsVisible() );

    /* This ugly hack fixes a bug in wxWidgets 2.8.7 and likely earlier
     * versions for the flex grid sizer in wxGTK that prevents the last
     * column from being sized correctly.  It doesn't cause any problems
     * on win32 so it doesn't need to wrapped in ugly #ifdef __WXGTK__
     * #endif.
     */
    dlg.Layout();
    dlg.Fit();
    dlg.SetMinSize( dlg.GetSize() );
    // dlg.SetLastSizeAndPosition();    // done in DIALOG_SHIM::Show()

    if( dlg.ShowModal() == wxID_CANCEL )
    {
        if( pin->IsNew() )
        {
            pin->SetFlags( IS_CANCELLED );
            m_canvas->EndMouseCapture();
        }
        return;
    }

    // Save the pin properties to use for the next new pin.
    LastPinNameSize = ValueFromString( g_UserUnit, dlg.GetNameTextSize() );
    LastPinNumSize = ValueFromString( g_UserUnit, dlg.GetPadNameTextSize() );
    LastPinOrient = LIB_PIN::GetOrientationCode( dlg.GetOrientation() );
    LastPinLength = ValueFromString( g_UserUnit, dlg.GetLength() );
    LastPinShape = LIB_PIN::GetStyleCode( dlg.GetStyle() );
    LastPinType = dlg.GetElectricalType();
    LastPinCommonConvert = dlg.GetAddToAllBodyStyles();
    LastPinCommonUnit = dlg.GetAddToAllParts();
    LastPinVisible = dlg.GetVisible();

    pin->EnableEditMode( true, m_editPinsPerPartOrConvert );
    pin->SetName( dlg.GetName() );
    pin->SetNameTextSize( LastPinNameSize );
    pin->SetNumber( dlg.GetPadName() );
    pin->SetNumberTextSize( LastPinNumSize );
    pin->SetOrientation( LastPinOrient );
    pin->SetLength( GetLastPinLength() );
    pin->SetType( LastPinType );
    pin->SetShape( LastPinShape );
    pin->SetConversion( ( LastPinCommonConvert ) ? 0 : m_convert );
    pin->SetPartNumber( ( LastPinCommonUnit ) ? 0 : m_unit );
    pin->SetVisible( LastPinVisible );

    if( pin->IsModified() || pin->IsNew() )
    {
        if( !pin->InEditMode() )
            SaveCopyInUndoList( pin->GetParent() );

        OnModify( );

        MSG_PANEL_ITEMS items;
        pin->GetMsgPanelInfo( items );
        SetMsgPanel( items );
        m_canvas->Refresh();
    }

    pin->EnableEditMode( false, m_editPinsPerPartOrConvert );

    // Restore pin flags, that can be changed by the dialog editor
    pin->ClearFlags();
    pin->SetFlags( item_flags );
}

/**
 * Clean up after aborting a move pin command.
 */
static void AbortPinMove( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) Panel->GetParent();

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

    // clear edit flags
    parent->SetDrawItem( NULL );
    parent->SetLastDrawItem( NULL );
    Panel->Refresh( true );
}


/**
 * Managed cursor callback for placing component pins.
 */
void LIB_EDIT_FRAME::PlacePin()
{
    LIB_PIN* Pin;
    LIB_PIN* CurrentPin  = (LIB_PIN*) m_drawItem;
    bool     ask_for_pin = true;
    wxPoint  newpos;
    bool     status;

    // Some tests
    if( (CurrentPin == NULL) || (CurrentPin->Type() != LIB_PIN_T) )
    {
        wxMessageBox( wxT( "LIB_EDIT_FRAME::PlacePin() error" ) );
        return;
    }

    newpos = GetCrossHairPosition( true );

    // Test for an other pin in same new position:
    for( Pin = m_component->GetNextPin(); Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin == CurrentPin || newpos != Pin->GetPosition() || Pin->GetFlags() )
            continue;

        if( ask_for_pin && SynchronizePins() )
        {
            m_canvas->SetIgnoreMouseEvents( true );
            status =
                IsOK( this, _( "This position is already occupied by \
another pin. Continue?" ) );
            m_canvas->MoveCursorToCrossHair();
            m_canvas->SetIgnoreMouseEvents( false );

            if( !status )
                return;
            else
                ask_for_pin = false;
        }
    }

    // Create Undo from GetTempCopyComponent() if exists ( i.e. after a pin move)
    // or from m_component (pin add ...)
    if( GetTempCopyComponent() )
        SaveCopyInUndoList( GetTempCopyComponent() );
    else
        SaveCopyInUndoList( m_component );

    m_canvas->SetMouseCapture( NULL, NULL );
    OnModify();
    CurrentPin->Move( newpos );

    if( CurrentPin->IsNew() )
    {
        LastPinOrient = CurrentPin->GetOrientation();
        LastPinType   = CurrentPin->GetType();
        LastPinShape  = CurrentPin->GetShape();

        if( SynchronizePins() )
            CreateImagePins( CurrentPin, m_unit, m_convert, m_showDeMorgan );

        m_lastDrawItem = CurrentPin;
        m_component->AddDrawItem( m_drawItem );
    }

    // Put linked pins in new position, and clear flags
    for( Pin = m_component->GetNextPin(); Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin->GetFlags() == 0 )
            continue;

        Pin->Move( CurrentPin->GetPosition() );
        Pin->ClearFlags();
    }

    m_drawItem = NULL;

    m_canvas->Refresh();
}


/**
 * Prepare the displacement of a pin
 *
 * Locate the pin pointed to by the cursor, and set the cursor management
 * function move the pin.
 */
void LIB_EDIT_FRAME::StartMovePin( wxDC* DC )
{
    LIB_PIN* currentPin = (LIB_PIN*) m_drawItem;
    wxPoint  startPos;

    TempCopyComponent();

    // Mark pins for moving.
    LIB_PIN* pin = m_component->GetNextPin();

    for( ; pin != NULL; pin = m_component->GetNextPin( pin ) )
    {
        pin->ClearFlags();

        if( pin == currentPin )
            continue;

        if( ( pin->GetPosition() == currentPin->GetPosition() )
            && ( pin->GetOrientation() == currentPin->GetOrientation() )
            && SynchronizePins() )
            pin->SetFlags( IS_LINKED | IS_MOVED );
    }

    currentPin->SetFlags( IS_LINKED | IS_MOVED );
    PinPreviousPos = OldPos = currentPin->GetPosition();
    startPos.x = OldPos.x;
    startPos.y = -OldPos.y;
//    m_canvas->CrossHairOff( DC );
    SetCrossHairPosition( startPos );
    m_canvas->MoveCursorToCrossHair();

    MSG_PANEL_ITEMS items;
    currentPin->GetMsgPanelInfo( items );
    SetMsgPanel( items );
    m_canvas->SetMouseCapture( DrawMovePin, AbortPinMove );
//    m_canvas->CrossHairOn( DC );

    // Refresh the screen to avoid color artifacts when drawing
    // the pin in Edit mode and moving it from its start position
    m_canvas->Refresh();
}


/* Move pin to the current mouse position.  This function is called by the
 * cursor management code. */
static void DrawMovePin( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                         bool aErase )
{
    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) aPanel->GetParent();

    if( parent == NULL )
        return;

    LIB_PIN* CurrentPin = (LIB_PIN*) parent->GetDrawItem();

    if( CurrentPin == NULL || CurrentPin->Type() != LIB_PIN_T )
        return;

    wxPoint pinpos = CurrentPin->GetPosition();
    bool    showPinText = true;

    // Erase pin in old position
    if( aErase )
    {
        CurrentPin->Move( PinPreviousPos );
        CurrentPin->Draw( aPanel, aDC, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, g_XorMode,
                          &showPinText, DefaultTransform );
    }

    // Redraw pin in new position
    CurrentPin->Move( aPanel->GetParent()->GetCrossHairPosition( true ) );
    CurrentPin->Draw( aPanel, aDC, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, g_XorMode,
                      &showPinText, DefaultTransform );

    PinPreviousPos = CurrentPin->GetPosition();

    /* Keep the original position for existing pin (for Undo command)
     * and the current position for a new pin */
    if( !CurrentPin->IsNew() )
        CurrentPin->Move( pinpos );
}


/*
 * Create a new pin.
 */
void LIB_EDIT_FRAME::CreatePin( wxDC* DC )
{
    LIB_PIN* pin;
    bool     showPinText = true;

    if( m_component == NULL )
        return;

    m_component->ClearStatus();

    pin = new LIB_PIN( m_component );

    m_drawItem = pin;

    pin->SetFlags( IS_NEW );
    pin->SetUnit( m_unit );
    pin->SetConvert( m_convert );

    // Flag pins to consider
    if( SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    pin->Move( GetCrossHairPosition( true ) );
    pin->SetLength( GetLastPinLength() );
    pin->SetOrientation( LastPinOrient );
    pin->SetType( LastPinType );
    pin->SetShape( LastPinShape );
    pin->SetNameTextSize( LastPinNameSize );
    pin->SetNumberTextSize( LastPinNumSize );
    pin->SetConvert( LastPinCommonConvert ? 0 : m_convert );
    pin->SetUnit( LastPinCommonUnit ? 0 : m_unit );
    pin->SetVisible( LastPinVisible );
    PinPreviousPos = pin->GetPosition();
    m_canvas->SetIgnoreMouseEvents( true );
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetId( ID_LIBEDIT_EDIT_PIN );
    GetEventHandler()->ProcessEvent( cmd );
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );

    if( pin->GetFlags() & IS_CANCELLED )
    {
        deleteItem( DC );
    }
    else
    {
        ClearTempCopyComponent();
        m_canvas->SetMouseCapture( DrawMovePin, AbortPinMove );

        if( DC )
            pin->Draw( m_canvas, DC, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, GR_COPY, &showPinText,
                       DefaultTransform );

    }
}


void LIB_EDIT_FRAME::CreateImagePins( LIB_PIN* aPin, int aUnit, int aConvert, bool aDeMorgan )
{
    int      ii;
    LIB_PIN* NewPin;

    if( !SynchronizePins() )
        return;

    // Create "convert" pin at the current position.
    if( aDeMorgan && ( aPin->GetConvert() != 0 ) )
    {
        NewPin = (LIB_PIN*) aPin->Clone();

        if( aPin->GetConvert() > 1 )
            NewPin->SetConvert( 1 );
        else
            NewPin->SetConvert( 2 );

        aPin->GetParent()->AddDrawItem( NewPin );
    }

    for( ii = 1; ii <= aPin->GetParent()->GetPartCount(); ii++ )
    {
        if( ii == aUnit || aPin->GetUnit() == 0 )
            continue;                       // Pin common to all units.

        NewPin = (LIB_PIN*) aPin->Clone();

        if( aConvert != 0 )
            NewPin->SetConvert( 1 );

        NewPin->SetUnit( ii );
        aPin->GetParent()->AddDrawItem( NewPin );

        if( !( aDeMorgan && ( aPin->GetConvert() != 0 ) ) )
            continue;

        NewPin = (LIB_PIN*) aPin->Clone();
        NewPin->SetConvert( 2 );

        if( aPin->GetUnit() != 0 )
            NewPin->SetUnit( ii );

        aPin->GetParent()->AddDrawItem( NewPin );
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
    bool selected = aMasterPin->IsSelected();

    if( ( m_component == NULL ) || ( aMasterPin == NULL ) )
        return;

    if( aMasterPin->Type() != LIB_PIN_T )
        return;

    OnModify( );

    LIB_PIN* pin = m_component->GetNextPin();

    for( ; pin != NULL; pin = m_component->GetNextPin( pin ) )
    {
        if( ( pin->GetConvert() ) && ( pin->GetConvert() != m_convert ) )
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
}


// Create a new pin based on the previous pin with an incremented pin number.
void LIB_EDIT_FRAME::RepeatPinItem( wxDC* DC, LIB_PIN* SourcePin )
{
    LIB_PIN* Pin;
    wxString msg;

    if( m_component == NULL || SourcePin == NULL || SourcePin->Type() != LIB_PIN_T )
        return;

    Pin = (LIB_PIN*) SourcePin->Clone();
    Pin->ClearFlags();
    Pin->SetFlags( IS_NEW );
    Pin->Move( Pin->GetPosition() + wxPoint( g_RepeatStep.x, -g_RepeatStep.y ) );
    wxString nextName = Pin->GetName();
    IncrementLabelMember( nextName );
    Pin->SetName( nextName );

    Pin->PinStringNum( msg );
    IncrementLabelMember( msg );
    Pin->SetPinNumFromString( msg );

    m_drawItem = Pin;

    if( SynchronizePins() )
        Pin->SetFlags( IS_LINKED );

    wxPoint savepos = GetCrossHairPosition();
    m_canvas->CrossHairOff( DC );

    SetCrossHairPosition( wxPoint( Pin->GetPosition().x, -Pin->GetPosition().y ) );

    // Add this new pin in list, and creates pins for others parts if needed
    m_drawItem = Pin;
    ClearTempCopyComponent();
    PlacePin();
    m_lastDrawItem = Pin;

    SetCrossHairPosition( savepos );
    m_canvas->CrossHairOn( DC );

    MSG_PANEL_ITEMS items;
    Pin->GetMsgPanelInfo( items );
    SetMsgPanel( items );
    OnModify( );
}


// helper function to sort pins by pin num
bool sort_by_pin_number( const LIB_PIN* ref, const LIB_PIN* tst )
{
    int test = ref->GetNumber() - tst->GetNumber();

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


/* Test for duplicate pins and off grid pins:
 * Pins are considered off grid when they are not on the 25 mils grid
 * A grid smaller than 25 mils must be used only to build graphic shapes.
 */
void LIB_EDIT_FRAME::OnCheckComponent( wxCommandEvent& event )
{
    #define MIN_GRID_SIZE 25
    int      dup_error;
    int      offgrid_error;
    LIB_PIN* Pin;
    LIB_PINS PinList;
    wxString msg;
    wxString aux_msg;

    if( m_component == NULL )
        return;

    m_component->GetPins( PinList );

    if( PinList.size() == 0 )
    {
        DisplayInfoMessage( this, _( "No pins!" ) );
        return;
    }

    // Sort pins by pin num, so 2 duplicate pins
    // (pins with the same number) will be consecutive in list
    sort( PinList.begin(), PinList.end(), sort_by_pin_number );

    // Test for duplicates:
    dup_error = 0;
    DIALOG_DISPLAY_HTML_TEXT_BASE error_display( this, wxID_ANY,
                                                 _( "Marker Information" ),
                                                 wxDefaultPosition,
                                                 wxSize( 750, 600 ) );

    for( unsigned ii = 1; ii < PinList.size(); ii++ )
    {
        wxString stringPinNum, stringCurrPinNum;

        LIB_PIN* curr_pin = PinList[ii];
        Pin = PinList[ii - 1];

        if( Pin->GetNumber() != curr_pin->GetNumber()
            || Pin->GetConvert() != curr_pin->GetConvert()
            || Pin->GetUnit() != curr_pin->GetUnit() )
            continue;

        dup_error++;
        Pin->PinStringNum( stringPinNum );

        /* TODO I dare someone to find a way to make happy translators on
           this thing! Lorenzo */
        curr_pin->PinStringNum( stringCurrPinNum );
        msg.Printf( _( "<b>Duplicate pin %s</b> \"%s\" at location <b>(%.3f, \
%.3f)</b> conflicts with pin %s \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                    GetChars( stringCurrPinNum ),
                    GetChars( curr_pin->GetName() ),
                    curr_pin->GetPosition().x / 1000.0,
                    -curr_pin->GetPosition().y / 1000.0,
                    GetChars( stringPinNum ),
                    GetChars( Pin->GetName() ),
                    Pin->GetPosition().x / 1000.0,
                    -Pin->GetPosition().y / 1000.0 );

        if( m_component->GetPartCount() > 1 )
        {
            aux_msg.Printf( _( " in part %c" ), 'A' + curr_pin->GetUnit() );
            msg += aux_msg;
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
    offgrid_error = 0;

    for( unsigned ii = 0; ii < PinList.size(); ii++ )
    {
        Pin = PinList[ii];

        if( ( (Pin->GetPosition().x % MIN_GRID_SIZE) == 0 ) &&
            ( (Pin->GetPosition().y % MIN_GRID_SIZE) == 0 ) )
            continue;

        // A pin is found here off grid
        offgrid_error++;
        wxString stringPinNum;
        Pin->PinStringNum( stringPinNum );

        msg.Printf( _( "<b>Off grid pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                    GetChars( stringPinNum ),
                    GetChars( Pin->GetName() ),
                    Pin->GetPosition().x / 1000.0,
                    -Pin->GetPosition().y / 1000.0 );

        if( m_component->GetPartCount() > 1 )
        {
            aux_msg.Printf( _( " in part %c" ), 'A' + Pin->GetUnit() );
            msg += aux_msg;
        }

        if( m_showDeMorgan )
        {
            if( Pin->GetConvert() )
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
