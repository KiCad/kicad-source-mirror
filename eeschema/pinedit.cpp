/***************************/
/*  EESchema - PinEdit.cpp */
/***************************/

#include "fctsys.h"
#include "common.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "libeditfrm.h"
#include "eeschema_id.h"
#include "class_libentry.h"
#include "general.h"
#include "protos.h"

#include "dialog_display_info_HTML_base.h"
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


void WinEDA_LibeditFrame::OnEditPin( wxCommandEvent& event )
{
    if( m_drawItem == NULL || m_drawItem->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    int item_flags = m_drawItem->m_Flags;       // save flags to restore them after editing
    LIB_PIN* pin = (LIB_PIN*) m_drawItem;

    DIALOG_LIB_EDIT_PIN dlg( this );

    wxString units = GetUnitsLabel( g_UnitMetric );
    dlg.SetOrientationList( LIB_PIN::GetOrientationNames() );
    dlg.SetOrientation( LIB_PIN::GetOrientationCodeIndex( pin->m_Orient ) );
    dlg.SetStyleList( LIB_PIN::GetStyleNames() );
    dlg.SetStyle( LIB_PIN::GetStyleCodeIndex( pin->m_PinShape ) );
    dlg.SetElectricalTypeList( LIB_PIN::GetElectricalTypeNames() );
    dlg.SetElectricalType( pin->m_PinType );
    dlg.SetName( pin->m_PinName );
    dlg.SetNameTextSize( ReturnStringFromValue( g_UnitMetric,
                                                pin->m_PinNameSize,
                                                m_InternalUnits ) );
    dlg.SetNameTextSizeUnits( units );
    dlg.SetNumber( pin->GetNumber() );
    dlg.SetNumberTextSize( ReturnStringFromValue( g_UnitMetric,
                                                  pin->m_PinNumSize,
                                                  m_InternalUnits ) );
    dlg.SetNumberTextSizeUnits( units );
    dlg.SetLength( ReturnStringFromValue( g_UnitMetric, pin->m_PinLen,
                                          m_InternalUnits ) );
    dlg.SetLengthUnits( units );
    dlg.SetAddToAllParts( pin->m_Unit == 0 );
    dlg.SetAddToAllBodyStyles( pin->m_Convert == 0 );
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
    LastPinNameSize = ReturnValueFromString( g_UnitMetric,
                                             dlg.GetNameTextSize(),
                                             m_InternalUnits );
    LastPinNumSize = ReturnValueFromString( g_UnitMetric,
                                            dlg.GetNumberTextSize(),
                                            m_InternalUnits );
    LastPinOrient = LIB_PIN::GetOrientationCode( dlg.GetOrientation() );
    LastPinLength = ReturnValueFromString( g_UnitMetric, dlg.GetLength(),
                                           m_InternalUnits );
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
    pin->SetElectricalType( LastPinType );
    pin->SetDrawStyle( LastPinShape );
    pin->SetConversion( ( LastPinCommonConvert ) ? 0 : m_convert );
    pin->SetPartNumber( ( LastPinCommonUnit ) ? 0 : m_unit );
    pin->SetVisible( LastPinVisible );

    if( pin->IsModified() || pin->IsNew() )
    {
        if( !pin->IsNew() )
            SaveCopyInUndoList( pin->GetParent() );

        GetScreen()->SetModify();
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
    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) Panel->GetParent();

    if( parent == NULL )
        return;

    LIB_PIN* CurrentPin = (LIB_PIN*) parent->GetDrawItem();

    if( CurrentPin == NULL || CurrentPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    if( CurrentPin->m_Flags & IS_NEW )
        delete CurrentPin;
    else
        CurrentPin->m_Flags = 0;

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
void WinEDA_LibeditFrame::PlacePin( wxDC* DC )
{
    LIB_PIN* Pin;
    LIB_PIN* CurrentPin  = (LIB_PIN*) m_drawItem;
    bool     ask_for_pin = true;
    wxPoint  newpos;
    bool     status;

    if( CurrentPin == NULL )
        return;

    newpos.x = GetScreen()->m_Curseur.x;
    newpos.y = -GetScreen()->m_Curseur.y;

    // Tst for an other pin in same new position:
    for( Pin = m_component->GetNextPin(); Pin != NULL;
         Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin == CurrentPin || newpos != Pin->m_Pos || Pin->m_Flags )
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

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->SetModify();
    CurrentPin->m_Pos = newpos;

    if( CurrentPin->IsNew() )
    {
        LastPinOrient = CurrentPin->m_Orient;
        LastPinType   = CurrentPin->m_PinType;
        LastPinShape  = CurrentPin->m_PinShape;
        CreateImagePins( CurrentPin, m_unit, m_convert, m_showDeMorgan );
        m_lastDrawItem = CurrentPin;
        m_component->AddDrawItem( m_drawItem );
    }

    /* Put linked pins in new position, and clear flags */
    for( Pin = m_component->GetNextPin(); Pin != NULL;
         Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin->m_Flags == 0 )
            continue;
        Pin->m_Pos   = CurrentPin->m_Pos;
        Pin->m_Flags = 0;
    }

    DrawPanel->CursorOff( DC );
    bool showPinText = true;
    CurrentPin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE,
                      &showPinText, DefaultTransformMatrix );
    DrawPanel->CursorOn( DC );

    m_drawItem = NULL;
};


/**
 * Prepare the displacement of a pin
 *
 * Locate the pin pointed to by the cursor, and set the cursor management
 * function move the pin.
 */
void WinEDA_LibeditFrame::StartMovePin( wxDC* DC )
{
    LIB_PIN* Pin;
    LIB_PIN* CurrentPin = (LIB_PIN*) m_drawItem;
    wxPoint  startPos;

    /* Mark pins for moving. */
    Pin = m_component->GetNextPin();
    for( ; Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        Pin->m_Flags = 0;
        if( Pin == CurrentPin )
            continue;
        if( ( Pin->m_Pos == CurrentPin->m_Pos )
           && ( Pin->m_Orient == CurrentPin->m_Orient )
           && ( g_EditPinByPinIsOn == false ) )
            Pin->m_Flags |= IS_LINKED | IS_MOVED;
    }

    CurrentPin->m_Flags |= IS_LINKED | IS_MOVED;
    PinPreviousPos = OldPos = CurrentPin->m_Pos;

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
    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) panel->GetParent();

    if( parent == NULL )
        return;

    LIB_PIN* CurrentPin = (LIB_PIN*) parent->GetDrawItem();

    if( CurrentPin == NULL || CurrentPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    wxPoint pinpos = CurrentPin->m_Pos;
    bool    showPinText = true;

    /* Erase pin in old position */
    if( erase || CurrentPin->IsNew() )
    {
        wxLogDebug( wxT( "Initial pin position (%d, %d)" ),
                    PinPreviousPos.x, PinPreviousPos.y );
        CurrentPin->m_Pos = PinPreviousPos;
        CurrentPin->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                          &showPinText, DefaultTransformMatrix );
    }

    /* Redraw pin in new position */
    CurrentPin->m_Pos.x = panel->GetScreen()->m_Curseur.x;
    CurrentPin->m_Pos.y = -panel->GetScreen()->m_Curseur.y;
    CurrentPin->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                      &showPinText, DefaultTransformMatrix );

    PinPreviousPos = CurrentPin->m_Pos;

    /* Keep the original position for existing pin (for Undo command)
     * and the current position for a new pin */
    if( !CurrentPin->IsNew() )
        CurrentPin->m_Pos = pinpos;
}


/*
 * Delete pin at the current mouse position.
 * If g_EditPinByPinIsOn == false:
 *    All pins at the same position will be erased.
 *    Otherwise only the pin of the current unit and convert will be  erased.
 */
void WinEDA_LibeditFrame::DeletePin( wxDC*          DC,
                                     LIB_COMPONENT* LibEntry,
                                     LIB_PIN*       Pin )
{
    LIB_PIN* tmp;
    wxPoint  PinPos;

    if( LibEntry == NULL || Pin == NULL )
        return;

    PinPos = Pin->m_Pos;
    LibEntry->RemoveDrawItem( (LIB_DRAW_ITEM*) Pin, DrawPanel, DC );

    if( g_EditPinByPinIsOn == false )
    {
        tmp = LibEntry->GetNextPin();

        while( tmp != NULL )
        {
            Pin = tmp;
            tmp = LibEntry->GetNextPin( Pin );

            if( Pin->m_Pos != PinPos )
                continue;

            LibEntry->RemoveDrawItem( (LIB_DRAW_ITEM*) Pin );
        }
    }

    GetScreen()->SetModify();
}


/*
 * Create a new pin.
 */
void WinEDA_LibeditFrame::CreatePin( wxDC* DC )
{
    LIB_PIN* pin;
    bool     showPinText = true;

    if( m_component == NULL )
        return;

    m_component->ClearStatus();

    pin = new LIB_PIN( m_component );

    if( pin == NULL )
        return;

    m_drawItem = pin;

    pin->m_Flags   = IS_NEW;
    pin->m_Unit    = m_unit;
    pin->m_Convert = m_convert;

    /* Flag pins to consider */
    if( g_EditPinByPinIsOn == false )
        pin->m_Flags |= IS_LINKED;

    pin->m_Pos.x       = GetScreen()->m_Curseur.x;
    pin->m_Pos.y       = -GetScreen()->m_Curseur.y;
    pin->m_PinLen      = LastPinLength;
    pin->m_Orient      = LastPinOrient;
    pin->m_PinType     = LastPinType;
    pin->m_PinShape    = LastPinShape;
    pin->m_PinNameSize = LastPinNameSize;
    pin->m_PinNumSize  = LastPinNumSize;

    if( LastPinCommonConvert )
        pin->m_Convert = 0;
    else
        pin->m_Convert = m_convert;

    if( LastPinCommonUnit )
        pin->m_Unit = 0;
    else
        pin->m_Unit = m_unit;

    if( LastPinVisible )
        pin->m_Attributs &= ~PINNOTDRAW;
    else
        pin->m_Attributs |= PINNOTDRAW;

    if( DC )
        pin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, wxCOPY, &showPinText,
                   DefaultTransformMatrix );

    PinPreviousPos = pin->m_Pos;
    wxLogDebug( wxT( "Initial pin position (%d, %d)" ),
                PinPreviousPos.x, PinPreviousPos.y );
    DrawPanel->m_IgnoreMouseEvents = true;
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetId( ID_LIBEDIT_EDIT_PIN );
    GetEventHandler()->ProcessEvent( cmd );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = false;

    if (pin->m_Flags & IS_CANCELLED)
    {
        DeletePin(DC, m_component, pin);
        m_drawItem = NULL;
    }
    else
    {
        DrawPanel->ManageCurseur = DrawMovePin;
        DrawPanel->ForceCloseManageCurseur = AbortPinMove;
    }
}


static void CreateImagePins( LIB_PIN* Pin, int unit, int convert,
                             bool asDeMorgan )
{
    int      ii;
    LIB_PIN* NewPin;
    bool     CreateConv = false;


    if( g_EditPinByPinIsOn )
        return;

    if( asDeMorgan && ( Pin->m_Convert != 0 ) )
        CreateConv = true;

    /* Create "convert" pin at the current position. */
    if( CreateConv == true )
    {
        NewPin = (LIB_PIN*) Pin->GenCopy();
        if( Pin->m_Convert > 1 )
            NewPin->m_Convert = 1;
        else
            NewPin->m_Convert = 2;
        Pin->GetParent()->AddDrawItem( NewPin );
    }

    for( ii = 1; ii <= Pin->GetParent()->GetPartCount(); ii++ )
    {
        if( ii == unit || Pin->m_Unit == 0 )
            continue;                       /* Pin common to all units. */

        NewPin = (LIB_PIN*) Pin->GenCopy();
        if( convert != 0 )
            NewPin->m_Convert = 1;
        NewPin->m_Unit = ii;
        Pin->GetParent()->AddDrawItem( NewPin );

        if( CreateConv == false )
            continue;

        NewPin = (LIB_PIN*) Pin->GenCopy();
        NewPin->m_Convert = 2;
        if( Pin->m_Unit != 0 )
            NewPin->m_Unit = ii;
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
void WinEDA_LibeditFrame::GlobalSetPins( wxDC* DC, LIB_PIN* MasterPin, int id )

{
    LIB_PIN* Pin;
    bool     selected    = ( MasterPin->m_Selected & IS_SELECTED ) != 0;
    bool     showPinText = true;

    if( ( m_component == NULL ) || ( MasterPin == NULL ) )
        return;
    if( MasterPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    GetScreen()->SetModify();

    Pin = m_component->GetNextPin();
    for( ; Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        if( ( Pin->m_Convert ) && ( Pin->m_Convert != m_convert ) )
            continue;

        // Is it the "selected mode" ?
        if( selected && ( Pin->m_Selected & IS_SELECTED ) == 0 )
            continue;

        Pin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                   &showPinText, DefaultTransformMatrix );

        switch( id )
        {
        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
            Pin->m_PinNumSize = MasterPin->m_PinNumSize;
            break;

        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
            Pin->m_PinNameSize = MasterPin->m_PinNameSize;
            break;

        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
            Pin->m_PinLen = MasterPin->m_PinLen;
            break;
        }

        Pin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE,
                   &showPinText, DefaultTransformMatrix );
    }
}


/* Create a new pin based on the previous pin with an incremented pin number. */
void WinEDA_LibeditFrame::RepeatPinItem( wxDC* DC, LIB_PIN* SourcePin )
{
    LIB_PIN* Pin;
    wxString msg;
    int      ox = 0, oy = 0;

    if( m_component == NULL || SourcePin == NULL
        || SourcePin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    Pin = (LIB_PIN*) SourcePin->GenCopy();
    m_component->AddDrawItem( Pin );

    Pin->m_Flags  = IS_NEW;
    Pin->m_Pos.x += g_RepeatStep.x;
    ox = Pin->m_Pos.x;
    Pin->m_Pos.y += -g_RepeatStep.y;
    oy = Pin->m_Pos.y;
    IncrementLabelMember( Pin->m_PinName );

    Pin->ReturnPinStringNum( msg );
    IncrementLabelMember( msg );
    Pin->SetPinNumFromString( msg );

    m_drawItem = Pin;

    if( g_EditPinByPinIsOn == false )
        Pin->m_Flags |= IS_LINKED;

    wxPoint savepos = GetScreen()->m_Curseur;
    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur.x = Pin->m_Pos.x;
    GetScreen()->m_Curseur.y = -Pin->m_Pos.y;
    PlacePin( DC );
    GetScreen()->m_Curseur = savepos;
    DrawPanel->CursorOn( DC );

    Pin->DisplayInfo( this );
    GetScreen()->SetModify();
}


/* helper function to sort pins by pin num */
bool sort_by_pin_number( const LIB_PIN* ref, const LIB_PIN* tst )
{
    int test = ref->m_PinNum - tst->m_PinNum;

    if( test == 0 )
    {
        test = ref->m_Convert - tst->m_Convert;
    }
    if( test == 0 )
    {
        test = ref->m_Unit - tst->m_Unit;
    }
    return test < 0;
}


/* Test for duplicate pins and off grid pins:
 * Pins are considered off grid when they are not on the 25 mils grid
 * A grid smaller than 25 mils must be used only to build graphic shapes.
 */
void WinEDA_LibeditFrame::OnCheckComponent( wxCommandEvent& event )
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

        if( Pin->m_PinNum != curr_pin->m_PinNum
            || Pin->m_Convert != curr_pin->m_Convert
            || Pin->m_Unit != curr_pin->m_Unit )
            continue;

        dup_error++;
        Pin->ReturnPinStringNum( stringPinNum );
        curr_pin->ReturnPinStringNum( stringCurrPinNum );
        msg.Printf( _( "<b>Duplicate pin %s</b> \"%s\" at location <b>(%.3f, \
%.3f)</b> conflicts with pin %s \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                    GetChars( stringCurrPinNum ),
                    GetChars( curr_pin->m_PinName ),
                    (float) curr_pin->m_Pos.x / 1000.0,
                    (float) -curr_pin->m_Pos.y / 1000.0,
                    GetChars( stringPinNum ),
                    GetChars( Pin->m_PinName ),
                    (float) Pin->m_Pos.x / 1000.0,
                    (float) -Pin->m_Pos.y / 1000.0 );

        if( m_component->GetPartCount() > 1 )
        {
            aux_msg.Printf( _( " in part %c" ), 'A' + curr_pin->m_Unit );
            msg += aux_msg;
        }

        if( m_showDeMorgan )
        {
            if( curr_pin->m_Convert )
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

        if( ( (Pin->m_Pos.x % MIN_GRID_SIZE) == 0 ) &&
            ( (Pin->m_Pos.y % MIN_GRID_SIZE) == 0 ) )
            continue;

        // A pin is foun here off grid
        offgrid_error++;
        wxString stringPinNum;
        Pin->ReturnPinStringNum( stringPinNum );
        msg.Printf( _( "<b>Off grid pin %s</b> \"%s\" at location <b>(%.3f, \
%.3f)</b>" ),
                    GetChars( stringPinNum ),
                    GetChars( Pin->m_PinName ),
                    (float) Pin->m_Pos.x / 1000.0,
                    (float) -Pin->m_Pos.y / 1000.0 );

        if( m_component->GetPartCount() > 1 )
        {
            aux_msg.Printf( _( " in part %c" ), 'A' + Pin->m_Unit );
            msg += aux_msg;
        }

        if( m_showDeMorgan )
        {
            if( Pin->m_Convert )
                msg += _( "  of converted" );
            else
                msg += _( "  of normal" );
        }

        msg += wxT( ".<br>" );
        error_display.m_htmlWindow->AppendToPage( msg );
    }

    if( !dup_error && !offgrid_error )
        DisplayInfoMessage( this,
                            _( "No off grid or duplicate pins were found." ) );

    else
        error_display.ShowModal();
}
