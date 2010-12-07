/***************************/
/*  EESchema - PinEdit.cpp */
/***************************/

#include "fctsys.h"
#include "common.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"

#include "libeditframe.h"
#include "eeschema_id.h"
#include "class_libentry.h"
#include "lib_pin.h"
#include "general.h"
#include "protos.h"

#include "../common/dialogs/dialog_display_info_HTML_base.h"
#include "dialog_lib_edit_pin.h"


static void CreateImagePins( LIB_PIN* Pin, int unit, int convert,
                             bool asDeMorgan );
static void AbortPinMove( WinEDA_DrawPanel* Panel, wxDC* DC );
static void DrawMovePin( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


static wxPoint OldPos;
static wxPoint PinPreviousPos;
static int     LastPinType          = PIN_INPUT;
static int     LastPinOrient        = PIN_RIGHT;
static int     LastPinShape         = NONE;
static int     LastPinLength        = 300;
static int     LastPinNameSize      = 50;
static int     LastPinNumSize       = 50;
static bool    LastPinCommonConvert = false;
static bool    LastPinCommonUnit    = false;
static bool    LastPinVisible       = true;


void LIB_EDIT_FRAME::OnRotatePin( wxCommandEvent& event )
{

	// Check, if the item is a pin, else return
	if( m_drawItem == NULL || m_drawItem->Type() != COMPONENT_PIN_DRAW_TYPE )
		return;

	// save flags to restore them after rotating
	int item_flags = m_drawItem->m_Flags;
	LIB_PIN* pin = (LIB_PIN*) m_drawItem;

	// Save old pin orientation
	LastPinOrient = pin->GetOrientation();

    if( !pin->InEditMode() )
        SaveCopyInUndoList( pin->GetParent() );

	// Get the actual pin orientation index
	int orientationIndex = pin->GetOrientationCodeIndex( pin->GetOrientation() );

	// Compute the next orientation, swap lower two bits for the right order
	orientationIndex = ((orientationIndex & 2) >> 1) | ((orientationIndex & 1) << 1);
	orientationIndex = orientationIndex + 1;
	orientationIndex = ((orientationIndex & 2) >> 1) | ((orientationIndex & 1) << 1);

	// Set the new orientation
	pin->SetOrientation( pin->GetOrientationCode( orientationIndex ) );

	OnModify( );
	pin->DisplayInfo( this );
	DrawPanel->Refresh();

	// Restore pin flags
	pin->m_Flags = item_flags;
}

void LIB_EDIT_FRAME::OnEditPin( wxCommandEvent& event )
{
    if( m_drawItem == NULL || m_drawItem->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    int item_flags = m_drawItem->m_Flags;       // save flags to restore them after editing
    LIB_PIN* pin = (LIB_PIN*) m_drawItem;

    DIALOG_LIB_EDIT_PIN dlg( this );

    wxString units = GetUnitsLabel( g_UserUnit );
    dlg.SetOrientationList( LIB_PIN::GetOrientationNames(), LIB_PIN::GetOrientationSymbols() );
    dlg.SetOrientation( LIB_PIN::GetOrientationCodeIndex( pin->GetOrientation() ) );
    dlg.SetStyleList( LIB_PIN::GetStyleNames(), LIB_PIN::GetStyleSymbols() );
    dlg.SetStyle( LIB_PIN::GetStyleCodeIndex( pin->GetShape() ) );
    dlg.SetElectricalTypeList( LIB_PIN::GetElectricalTypeNames(),
                               LIB_PIN::GetElectricalTypeSymbols() );
    dlg.SetElectricalType( pin->GetType() );
    dlg.SetName( pin->GetName() );
    dlg.SetNameTextSize( ReturnStringFromValue( g_UserUnit,
                                                pin->m_PinNameSize,
                                                m_InternalUnits ) );
    dlg.SetNameTextSizeUnits( units );
    dlg.SetNumber( pin->GetNumberString() );
    dlg.SetNumberTextSize( ReturnStringFromValue( g_UserUnit,
                                                  pin->m_PinNumSize,
                                                  m_InternalUnits ) );
    dlg.SetNumberTextSizeUnits( units );
    dlg.SetLength( ReturnStringFromValue( g_UserUnit, pin->GetLength(), m_InternalUnits ) );
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
            pin->m_Flags |= IS_CANCELLED;
            DrawPanel->UnManageCursor();
        }
        return;
    }

    /* Save the pin properties to use for the next new pin. */
    LastPinNameSize = ReturnValueFromString( g_UserUnit,
                                             dlg.GetNameTextSize(),
                                             m_InternalUnits );
    LastPinNumSize = ReturnValueFromString( g_UserUnit,
                                            dlg.GetNumberTextSize(),
                                            m_InternalUnits );
    LastPinOrient = LIB_PIN::GetOrientationCode( dlg.GetOrientation() );
    LastPinLength = ReturnValueFromString( g_UserUnit, dlg.GetLength(), m_InternalUnits );
    LastPinShape = LIB_PIN::GetStyleCode( dlg.GetStyle() );
    LastPinType = dlg.GetElectricalType();
    LastPinCommonConvert = dlg.GetAddToAllBodyStyles();
    LastPinCommonUnit = dlg.GetAddToAllParts();
    LastPinVisible = dlg.GetVisible();

    pin->EnableEditMode( true, g_EditPinByPinIsOn );
    pin->SetName( dlg.GetName() );
    pin->SetNameTextSize( LastPinNameSize );
    pin->SetNumber( dlg.GetNumber() );
    pin->SetNumberTextSize( LastPinNumSize );
    pin->SetOrientation( LastPinOrient );
    pin->SetLength( LastPinLength );
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
        pin->DisplayInfo( this );
        DrawPanel->Refresh();
    }

    pin->EnableEditMode( false, g_EditPinByPinIsOn );

    // Restore pin flags, that can be changed by the dialog editor
    pin->m_Flags = item_flags;
}


/**
 * Clean up after aborting a move pin command.
 */
static void AbortPinMove( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) Panel->GetParent();

    if( parent == NULL )
        return;

    LIB_PIN* CurrentPin = (LIB_PIN*) parent->GetDrawItem();

    if( CurrentPin == NULL || CurrentPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    if( CurrentPin->m_Flags & IS_NEW )
        delete CurrentPin;
    else
        parent->RestoreComponent();

    /* clear edit flags */
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    parent->SetDrawItem( NULL );
    parent->SetLastDrawItem( NULL );
    Panel->Refresh( true );
}


/**
 * Managed cursor callback for placing component pins.
 */
void LIB_EDIT_FRAME::PlacePin( wxDC* DC )
{
    LIB_PIN* Pin;
    LIB_PIN* CurrentPin  = (LIB_PIN*) m_drawItem;
    bool     ask_for_pin = true;
    wxPoint  newpos;
    bool     status;

    // Some tests
    if( (CurrentPin == NULL) || (CurrentPin->Type() != COMPONENT_PIN_DRAW_TYPE) )
    {
        wxMessageBox( wxT("LIB_EDIT_FRAME::PlacePin() error") );
        return;
    }

    newpos.x = GetScreen()->m_Curseur.x;
    newpos.y = -GetScreen()->m_Curseur.y;

    // Tst for an other pin in same new position:
    for( Pin = m_component->GetNextPin(); Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin == CurrentPin || newpos != Pin->GetPosition() || Pin->m_Flags )
            continue;

        if( ask_for_pin && !g_EditPinByPinIsOn )
        {
            DrawPanel->m_IgnoreMouseEvents = true;
            status =
                IsOK( this, _( "This position is already occupied by \
another pin. Continue?" ) );
            DrawPanel->MouseToCursorSchema();
            DrawPanel->m_IgnoreMouseEvents = false;

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

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    OnModify();
    CurrentPin->SetPosition( newpos );

    if( CurrentPin->IsNew() )
    {
        LastPinOrient = CurrentPin->GetOrientation();
        LastPinType   = CurrentPin->GetType();
        LastPinShape  = CurrentPin->GetShape();

        if( !g_EditPinByPinIsOn )
            CreateImagePins( CurrentPin, m_unit, m_convert, m_showDeMorgan );

        m_lastDrawItem = CurrentPin;
        m_component->AddDrawItem( m_drawItem );
    }

    /* Put linked pins in new position, and clear flags */
    for( Pin = m_component->GetNextPin(); Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin->m_Flags == 0 )
            continue;

        Pin->SetPosition( CurrentPin->GetPosition() );
        Pin->m_Flags = 0;
    }

    DrawPanel->CursorOff( DC );
    bool showPinText = true;
    CurrentPin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE,
                      &showPinText, DefaultTransform );
    DrawPanel->CursorOn( DC );

    m_drawItem = NULL;
}


/**
 * Prepare the displacement of a pin
 *
 * Locate the pin pointed to by the cursor, and set the cursor management
 * function move the pin.
 */
void LIB_EDIT_FRAME::StartMovePin( wxDC* DC )
{
    LIB_PIN* Pin;
    LIB_PIN* CurrentPin = (LIB_PIN*) m_drawItem;
    wxPoint  startPos;

    TempCopyComponent();

    /* Mark pins for moving. */
    Pin = m_component->GetNextPin();
    for( ; Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        Pin->m_Flags = 0;
        if( Pin == CurrentPin )
            continue;
        if( ( Pin->GetPosition() == CurrentPin->GetPosition() )
            && ( Pin->GetOrientation() == CurrentPin->GetOrientation() )
            && ( g_EditPinByPinIsOn == false ) )
            Pin->m_Flags |= IS_LINKED | IS_MOVED;
    }

    CurrentPin->m_Flags |= IS_LINKED | IS_MOVED;
    PinPreviousPos = OldPos = CurrentPin->GetPosition();

    startPos.x = OldPos.x;
    startPos.y = -OldPos.y;
    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = startPos;
    DrawPanel->MouseToCursorSchema();

    CurrentPin->DisplayInfo( this );
    DrawPanel->ManageCurseur = DrawMovePin;
    DrawPanel->ForceCloseManageCurseur = AbortPinMove;

    DrawPanel->CursorOn( DC );
}


/* Move pin to the current mouse position.  This function is called by the
 * cursor management code. */
static void DrawMovePin( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) panel->GetParent();

    if( parent == NULL )
        return;

    LIB_PIN* CurrentPin = (LIB_PIN*) parent->GetDrawItem();

    if( CurrentPin == NULL || CurrentPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    wxPoint pinpos = CurrentPin->GetPosition();
    bool    showPinText = true;

    /* Erase pin in old position */
    if( erase )
    {
        CurrentPin->SetPosition( PinPreviousPos );
        CurrentPin->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                          &showPinText, DefaultTransform );
    }

    /* Redraw pin in new position */
    CurrentPin->SetPosition( panel->GetScreen()->GetCursorDrawPosition() );
    CurrentPin->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &showPinText, DefaultTransform );

    PinPreviousPos = CurrentPin->GetPosition();

    /* Keep the original position for existing pin (for Undo command)
     * and the current position for a new pin */
    if( !CurrentPin->IsNew() )
        CurrentPin->SetPosition( pinpos );
}


/*
 * Delete pin at the current mouse position.
 * If g_EditPinByPinIsOn == false:
 *    All pins at the same position will be erased.
 *    Otherwise only the pin of the current unit and convert will be  erased.
 */
void LIB_EDIT_FRAME::DeletePin( wxDC* DC, LIB_COMPONENT* LibEntry, LIB_PIN* Pin )
{
    LIB_PIN* tmp;
    wxPoint  PinPos;

    if( LibEntry == NULL || Pin == NULL )
        return;

    PinPos = Pin->GetPosition();
    LibEntry->RemoveDrawItem( (LIB_DRAW_ITEM*) Pin, DrawPanel, DC );

    if( g_EditPinByPinIsOn == false )
    {
        tmp = LibEntry->GetNextPin();

        while( tmp != NULL )
        {
            Pin = tmp;
            tmp = LibEntry->GetNextPin( Pin );

            if( Pin->GetPosition() != PinPos )
                continue;

            LibEntry->RemoveDrawItem( (LIB_DRAW_ITEM*) Pin );
        }
    }

    OnModify( );
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

    pin->m_Flags = IS_NEW;
    pin->SetUnit( m_unit );
    pin->SetConvert( m_convert );

    /* Flag pins to consider */
    if( g_EditPinByPinIsOn == false )
        pin->m_Flags |= IS_LINKED;

    pin->SetPosition( GetScreen()->GetCursorDrawPosition() );
    pin->SetLength( LastPinLength );
    pin->SetOrientation( LastPinOrient );
    pin->SetType( LastPinType );
    pin->SetShape( LastPinShape );
    pin->m_PinNameSize = LastPinNameSize;
    pin->m_PinNumSize  = LastPinNumSize;
    pin->SetConvert( LastPinCommonConvert ? 0 : m_convert );
    pin->SetUnit( LastPinCommonUnit ? 0 : m_unit );
    pin->SetVisible( LastPinVisible );

    PinPreviousPos = pin->GetPosition();
    DrawPanel->m_IgnoreMouseEvents = true;
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetId( ID_LIBEDIT_EDIT_PIN );
    GetEventHandler()->ProcessEvent( cmd );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = false;

    if( pin->m_Flags & IS_CANCELLED )
    {
        DeletePin( NULL, m_component, pin );
        m_drawItem = NULL;
    }
    else
    {
        ClearTempCopyComponent();
        DrawPanel->ManageCurseur = DrawMovePin;
        DrawPanel->ForceCloseManageCurseur = AbortPinMove;

        if( DC )
            pin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, wxCOPY, &showPinText,
                       DefaultTransform );

    }
}


static void CreateImagePins( LIB_PIN* Pin, int unit, int convert, bool asDeMorgan )
{
    int      ii;
    LIB_PIN* NewPin;
    bool     CreateConv = false;


    if( g_EditPinByPinIsOn )
        return;

    if( asDeMorgan && ( Pin->GetConvert() != 0 ) )
        CreateConv = true;

    /* Create "convert" pin at the current position. */
    if( CreateConv == true )
    {
        NewPin = (LIB_PIN*) Pin->GenCopy();

        if( Pin->GetConvert() > 1 )
            NewPin->SetConvert( 1 );
        else
            NewPin->SetConvert( 2 );

        Pin->GetParent()->AddDrawItem( NewPin );
    }

    for( ii = 1; ii <= Pin->GetParent()->GetPartCount(); ii++ )
    {
        if( ii == unit || Pin->GetUnit() == 0 )
            continue;                       /* Pin common to all units. */

        NewPin = (LIB_PIN*) Pin->GenCopy();

        if( convert != 0 )
            NewPin->SetConvert( 1 );

        NewPin->SetUnit( ii );
        Pin->GetParent()->AddDrawItem( NewPin );

        if( CreateConv == false )
            continue;

        NewPin = (LIB_PIN*) Pin->GenCopy();
        NewPin->SetConvert( 2 );

        if( Pin->GetUnit() != 0 )
            NewPin->SetUnit( ii );

        Pin->GetParent()->AddDrawItem( NewPin );
    }
}


/*  Depending on "id":
 * - Change pin text size (name or num) (range 10 .. 1000 mil)
 * - Change pin lenght.
 *
 * If Pin is selected ( .m_flag == IS_SELECTED ) only the other selected
 * pins are modified
 */
void LIB_EDIT_FRAME::GlobalSetPins( wxDC* DC, LIB_PIN* MasterPin, int id )

{
    LIB_PIN* Pin;
    bool     selected    = ( MasterPin->m_Selected & IS_SELECTED ) != 0;
    bool     showPinText = true;

    if( ( m_component == NULL ) || ( MasterPin == NULL ) )
        return;

    if( MasterPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    OnModify( );

    Pin = m_component->GetNextPin();

    for( ; Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        if( ( Pin->GetConvert() ) && ( Pin->GetConvert() != m_convert ) )
            continue;

        // Is it the "selected mode" ?
        if( selected && ( Pin->m_Selected & IS_SELECTED ) == 0 )
            continue;

        Pin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &showPinText, DefaultTransform );

        switch( id )
        {
        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
            Pin->m_PinNumSize = MasterPin->m_PinNumSize;
            break;

        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
            Pin->m_PinNameSize = MasterPin->m_PinNameSize;
            break;

        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
            Pin->SetLength( MasterPin->GetLength() );
            break;
        }

        ( ( LIB_DRAW_ITEM* )Pin )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE,
                                         &showPinText, DefaultTransform );
    }
}


/* Create a new pin based on the previous pin with an incremented pin number. */
void LIB_EDIT_FRAME::RepeatPinItem( wxDC* DC, LIB_PIN* SourcePin )
{
    LIB_PIN* Pin;
    wxString msg;

    if( m_component == NULL || SourcePin == NULL || SourcePin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    Pin = (LIB_PIN*) SourcePin->GenCopy();
    Pin->m_Flags = IS_NEW;
    Pin->SetPosition( Pin->GetPosition() + wxPoint( g_RepeatStep.x, -g_RepeatStep.y ) );
    wxString nextName = Pin->GetName();
    IncrementLabelMember( nextName );
    Pin->SetName( nextName );

    Pin->ReturnPinStringNum( msg );
    IncrementLabelMember( msg );
    Pin->SetPinNumFromString( msg );

    m_drawItem = Pin;

    if( g_EditPinByPinIsOn == false )
        Pin->m_Flags |= IS_LINKED;

    wxPoint savepos = GetScreen()->m_Curseur;
    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur.x = Pin->GetPosition().x;
    GetScreen()->m_Curseur.y = -Pin->GetPosition().y;

    // Add this new pin in list, and creates pins for others parts if needed
    m_drawItem = Pin;
    ClearTempCopyComponent();
    PlacePin( DC );
    m_lastDrawItem = Pin;

    GetScreen()->m_Curseur = savepos;
    DrawPanel->CursorOn( DC );

    Pin->DisplayInfo( this );
    OnModify( );
}


/* helper function to sort pins by pin num */
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
    int          dup_error;
    int          offgrid_error;
    LIB_PIN*     Pin;
    LIB_PIN_LIST PinList;
    wxString     msg;
    wxString     aux_msg;

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
        Pin->ReturnPinStringNum( stringPinNum );
        curr_pin->ReturnPinStringNum( stringCurrPinNum );
        msg.Printf( _( "<b>Duplicate pin %s</b> \"%s\" at location <b>(%.3f, \
%.3f)</b> conflicts with pin %s \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                    GetChars( stringCurrPinNum ),
                    GetChars( curr_pin->GetName() ),
                    (float) curr_pin->GetPosition().x / 1000.0,
                    (float) -curr_pin->GetPosition().y / 1000.0,
                    GetChars( stringPinNum ),
                    GetChars( Pin->GetName() ),
                    (float) Pin->GetPosition().x / 1000.0,
                    (float) -Pin->GetPosition().y / 1000.0 );

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

        // A pin is foun here off grid
        offgrid_error++;
        wxString stringPinNum;
        Pin->ReturnPinStringNum( stringPinNum );
        msg.Printf( _( "<b>Off grid pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                    GetChars( stringPinNum ),
                    GetChars( Pin->GetName() ),
                    (float) Pin->GetPosition().x / 1000.0,
                    (float) -Pin->GetPosition().y / 1000.0 );

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
