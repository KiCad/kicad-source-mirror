/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
    dlg.SetPinName( pin->GetName() );
    dlg.SetPinNameTextSize( StringFromValue( g_UserUnit, pin->GetNameTextSize() ) );
    dlg.SetPinNameTextSizeUnits( units );
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
    LastPinNameSize = ValueFromString( g_UserUnit, dlg.GetPinNameTextSize() );
    LastPinNumSize = ValueFromString( g_UserUnit, dlg.GetPadNameTextSize() );
    LastPinOrient = LIB_PIN::GetOrientationCode( dlg.GetOrientation() );
    LastPinLength = ValueFromString( g_UserUnit, dlg.GetLength() );
    LastPinShape = LIB_PIN::GetStyleCode( dlg.GetStyle() );
    LastPinType = dlg.GetElectricalType();
    LastPinCommonConvert = dlg.GetAddToAllBodyStyles();
    LastPinCommonUnit = dlg.GetAddToAllParts();
    LastPinVisible = dlg.GetVisible();

    pin->EnableEditMode( true, m_editPinsPerPartOrConvert );
    pin->SetName( dlg.GetPinName() );
    pin->SetNameTextSize( GetLastPinNameSize() );
    pin->SetNumber( dlg.GetPadName() );
    pin->SetNumberTextSize( GetLastPinNumSize() );
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
    LIB_PIN* cur_pin  = (LIB_PIN*) m_drawItem;
    bool     ask_for_pin = true;
    wxPoint  newpos;
    bool     status;

    // Some tests
    if( !cur_pin || cur_pin->Type() != LIB_PIN_T )
    {
        wxMessageBox( wxT( "LIB_EDIT_FRAME::PlacePin() error" ) );
        return;
    }

    newpos = GetCrossHairPosition( true );

    LIB_PART*      part = GetCurPart();

    // Test for an other pin in same new position:
    for( LIB_PIN* pin = part->GetNextPin();  pin;  pin = part->GetNextPin( pin ) )
    {
        if( pin == cur_pin || newpos != pin->GetPosition() || pin->GetFlags() )
            continue;

        if( ask_for_pin && SynchronizePins() )
        {
            m_canvas->SetIgnoreMouseEvents( true );

            status = IsOK( this, _( "This position is already occupied by another pin. Continue?" ) );

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
        SaveCopyInUndoList( part );

    m_canvas->SetMouseCapture( NULL, NULL );
    OnModify();
    cur_pin->Move( newpos );

    if( cur_pin->IsNew() )
    {
        LastPinOrient = cur_pin->GetOrientation();
        LastPinType   = cur_pin->GetType();
        LastPinShape  = cur_pin->GetShape();

        if( SynchronizePins() )
            CreateImagePins( cur_pin, m_unit, m_convert, m_showDeMorgan );

        m_lastDrawItem = cur_pin;
        part->AddDrawItem( m_drawItem );
    }

    // Put linked pins in new position, and clear flags
    for( LIB_PIN* pin = part->GetNextPin();  pin;  pin = part->GetNextPin( pin ) )
    {
        if( pin->GetFlags() == 0 )
            continue;

        pin->Move( cur_pin->GetPosition() );
        pin->ClearFlags();
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
    LIB_PIN* cur_pin = (LIB_PIN*) m_drawItem;
    wxPoint  startPos;

    TempCopyComponent();

    LIB_PART*      part = GetCurPart();

    // Mark pins for moving.
    for( LIB_PIN* pin = part->GetNextPin();  pin;  pin = part->GetNextPin( pin ) )
    {
        pin->ClearFlags();

        if( pin == cur_pin )
            continue;

        if( pin->GetPosition() == cur_pin->GetPosition() &&
            pin->GetOrientation() == cur_pin->GetOrientation() && SynchronizePins() )
        {
            pin->SetFlags( IS_LINKED | IS_MOVED );
        }
    }

    cur_pin->SetFlags( IS_LINKED | IS_MOVED );

    PinPreviousPos = OldPos = cur_pin->GetPosition();
    startPos.x = OldPos.x;
    startPos.y = -OldPos.y;

//    m_canvas->CrossHairOff( DC );
    SetCrossHairPosition( startPos );
    m_canvas->MoveCursorToCrossHair();

    MSG_PANEL_ITEMS items;

    cur_pin->GetMsgPanelInfo( items );
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

    LIB_PIN* cur_pin = (LIB_PIN*) parent->GetDrawItem();

    if( cur_pin == NULL || cur_pin->Type() != LIB_PIN_T )
        return;

    wxPoint pinpos = cur_pin->GetPosition();
    bool    showPinText = true;

    // Erase pin in old position
    if( aErase )
    {
        cur_pin->Move( PinPreviousPos );
        cur_pin->Draw( aPanel, aDC, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, g_XorMode,
                          &showPinText, DefaultTransform );
    }

    // Redraw pin in new position
    cur_pin->Move( aPanel->GetParent()->GetCrossHairPosition( true ) );
    cur_pin->Draw( aPanel, aDC, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, g_XorMode,
                      &showPinText, DefaultTransform );

    PinPreviousPos = cur_pin->GetPosition();

    /* Keep the original position for existing pin (for Undo command)
     * and the current position for a new pin */
    if( !cur_pin->IsNew() )
        cur_pin->Move( pinpos );
}


/*
 * Create a new pin.
 */
void LIB_EDIT_FRAME::CreatePin( wxDC* DC )
{
    bool     showPinText = true;

    LIB_PART*      part = GetCurPart();

    if( !part )
        return;

    part->ClearStatus();

    LIB_PIN* pin = new LIB_PIN( part );

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
    pin->SetNameTextSize( GetLastPinNameSize() );
    pin->SetNumberTextSize( GetLastPinNumSize() );
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

    for( ii = 1; ii <= aPin->GetParent()->GetUnitCount(); ii++ )
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

    LIB_PART*      part = GetCurPart();

    if( !part || !aMasterPin )
        return;

    if( aMasterPin->Type() != LIB_PIN_T )
        return;

    OnModify( );

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
    pin->Move( pin->GetPosition() + wxPoint( g_RepeatStep.x, -g_RepeatStep.y ) );
    wxString nextName = pin->GetName();
    IncrementLabelMember( nextName );
    pin->SetName( nextName );

    pin->PinStringNum( msg );
    IncrementLabelMember( msg );
    pin->SetPinNumFromString( msg );

    m_drawItem = pin;

    if( SynchronizePins() )
        pin->SetFlags( IS_LINKED );

    wxPoint savepos = GetCrossHairPosition();
    m_canvas->CrossHairOff( DC );

    SetCrossHairPosition( wxPoint( pin->GetPosition().x, -pin->GetPosition().y ) );

    // Add this new pin in list, and creates pins for others parts if needed
    m_drawItem = pin;
    ClearTempCopyComponent();
    PlacePin();
    m_lastDrawItem = pin;

    SetCrossHairPosition( savepos );
    m_canvas->CrossHairOn( DC );

    MSG_PANEL_ITEMS items;
    pin->GetMsgPanelInfo( items );
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


void LIB_EDIT_FRAME::OnCheckComponent( wxCommandEvent& event )
{
    LIB_PART*      part = GetCurPart();

    if( !part )
        return;

    const int MIN_GRID_SIZE = 25;

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
        wxString stringPinNum, stringCurrPinNum;

        LIB_PIN* curr_pin = pinList[ii];
        LIB_PIN* pin      = pinList[ii - 1];

        if( pin->GetNumber() != curr_pin->GetNumber()
            || pin->GetConvert() != curr_pin->GetConvert()
            || pin->GetUnit() != curr_pin->GetUnit() )
            continue;

        dup_error++;
        pin->PinStringNum( stringPinNum );

        /* TODO I dare someone to find a way to make happy translators on
           this thing! Lorenzo */
        curr_pin->PinStringNum( stringCurrPinNum );

        wxString msg = wxString::Format( _(
            "<b>Duplicate pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>"
            " conflicts with pin %s \"%s\" at location <b>(%.3f, %.3f)</b>" ),
            GetChars( stringCurrPinNum ),
            GetChars( curr_pin->GetName() ),
            curr_pin->GetPosition().x / 1000.0,
            -curr_pin->GetPosition().y / 1000.0,
            GetChars( stringPinNum ),
            GetChars( pin->GetName() ),
            pin->GetPosition().x / 1000.0,
            -pin->GetPosition().y / 1000.0
            );

        if( part->GetUnitCount() > 1 )
        {
            msg += wxString::Format( _( " in part %c" ), 'A' + curr_pin->GetUnit() - 1 );
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

        if( ( (pin->GetPosition().x % MIN_GRID_SIZE) == 0 ) &&
            ( (pin->GetPosition().y % MIN_GRID_SIZE) == 0 ) )
            continue;

        // "pin" is off grid here.
        offgrid_error++;
        wxString stringPinNum;
        pin->PinStringNum( stringPinNum );

        wxString msg = wxString::Format( _(
            "<b>Off grid pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>" ),
            GetChars( stringPinNum ),
            GetChars( pin->GetName() ),
            pin->GetPosition().x / 1000.0,
            -pin->GetPosition().y / 1000.0
            );

        if( part->GetUnitCount() > 1 )
        {
            msg += wxString::Format( _( " in part %c" ), 'A' + pin->GetUnit() - 1 );
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
