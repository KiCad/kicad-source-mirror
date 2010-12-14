/********************************/
/* Scehematic component edition */
/********************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "sch_component.h"


static void AbortMoveCmpField( WinEDA_DrawPanel* Panel, wxDC* DC );
static void MoveCmpField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


/******************************************************************************/
/* Prepare the displacement of the text being edited.
 */
/******************************************************************************/
void SCH_EDIT_FRAME::StartMoveCmpField( SCH_FIELD* aField, wxDC* DC )
{
    LIB_COMPONENT* Entry;

    SetCurrentField( aField );
    if( aField == NULL )
        return;

    if( aField->m_Text == wxEmptyString )
    {
        DisplayError( this, _( "No Field to move" ), 10 );
        return;
    }

    wxPoint        pos, newpos;
    SCH_COMPONENT* comp = (SCH_COMPONENT*) aField->GetParent();

    SAFE_DELETE( g_ItemToUndoCopy );
    g_ItemToUndoCopy = comp->GenCopy();

    pos = comp->m_Pos;

    /* Positions are computed by the transpose matrix.  Rotating mirror. */
    newpos = aField->m_Pos - pos;

    // Empirically this is necessary.  The Y coordinate appears to be inverted
    // under some circumstances, but that inversion is not preserved by all
    // combinations of mirroring and rotation.  The following clause is true
    // when the number of rotations and the number of mirrorings are both odd.
    if( comp->GetTransform().x2 * comp->GetTransform().y1 < 0 )
        NEGATE( newpos.y );

    newpos = comp->GetTransform().TransformCoordinate( newpos ) + pos;

    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = newpos;
    DrawPanel->MouseToCursorSchema();

    m_OldPos    = aField->m_Pos;
    m_Multiflag = 0;
    if( aField->m_FieldId == REFERENCE )
    {
        Entry = CMP_LIBRARY::FindLibraryComponent( comp->GetLibName() );

        if( Entry  != NULL )
        {
            if( Entry->GetPartCount() > 1 )
                m_Multiflag = 1;
        }
    }

    DrawPanel->ForceCloseManageCurseur = AbortMoveCmpField;
    DrawPanel->ManageCurseur = MoveCmpField;
    aField->m_Flags = IS_MOVED;

    DrawPanel->CursorOn( DC );
}


/******************************************************************************/
/* Edit the field Field (text, size)  */
/******************************************************************************/
void SCH_EDIT_FRAME::EditCmpFieldText( SCH_FIELD* Field, wxDC* DC )
{
    int            fieldNdx, flag;
    LIB_COMPONENT* Entry;

    if( Field == NULL )
    {
        DisplayError( this, _( "No Field To Edit" ), 10 );
        return;
    }

    SCH_COMPONENT* Cmp = (SCH_COMPONENT*) Field->GetParent();

    fieldNdx = Field->m_FieldId;

    if( fieldNdx == VALUE )
    {
        Entry = CMP_LIBRARY::FindLibraryComponent( Cmp->GetLibName() );

        if( Entry && Entry->IsPower() )
        {
            DisplayInfoMessage( this, _( "Part is a POWER, value cannot be \
modified!\nYou must create a new power"  ) );
            return;
        }
    }

    flag = 0;
    if( fieldNdx == REFERENCE )
    {
        Entry = CMP_LIBRARY::FindLibraryComponent( Cmp->GetLibName() );

        if( Entry != NULL )
        {
            if( Entry->GetPartCount() > 1 )
                flag = 1;
        }
    }

    /* save old cmp in undo list if not already in edit, or moving ... */
    if( Field->m_Flags == 0 )
        SaveCopyInUndoList( Cmp, UR_CHANGED );

    wxString newtext = Field->m_Text;
    DrawPanel->m_IgnoreMouseEvents = TRUE;

    wxTextEntryDialog dlg( this, Field->m_Name, _( "Component field text" ), newtext );
    int diag = dlg.ShowModal();
    newtext = dlg.GetValue( );
    newtext.Trim( true );
    newtext.Trim( false );

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
    if ( diag != wxID_OK )
        return;  // cancelled by user

    Field->m_AddExtraText = flag;
    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    if( !newtext.IsEmpty() )
    {
        if( Field->m_Text.IsEmpty() )
        {
            Field->m_Pos    = Cmp->m_Pos;
            Field->m_Size.x = Field->m_Size.y = m_TextFieldSize;
        }
        Field->m_Text = newtext;
        if( fieldNdx == REFERENCE )
        {
            Cmp->SetRef( GetSheet(), newtext );
        }
    }
    else
    {
        if( fieldNdx == REFERENCE )
        {
            DisplayError( this, _( "Reference needed !, No change" ) );
        }
        else if( fieldNdx == VALUE )
        {
            DisplayError( this, _( "Value needed !, No change" ) );
        }
        else
        {
            Field->m_Text = wxT( "~" );
        }
    }

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
    Cmp->DisplayInfo( this );
    OnModify();
}


/*
 * Move standard text field.  This routine is normally attached to the cursor.
 */
static void MoveCmpField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    wxPoint pos;
    int fieldNdx;

    SCH_EDIT_FRAME* frame = (SCH_EDIT_FRAME*) panel->GetParent();
    SCH_FIELD*      currentField = frame->GetCurrentField();

    if( currentField == NULL )
        return;

    SCH_COMPONENT* component = (SCH_COMPONENT*) currentField->GetParent();
    fieldNdx = currentField->m_FieldId;

    currentField->m_AddExtraText = frame->m_Multiflag;
    if( erase )
    {
        currentField->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode );
    }

    pos = ( (SCH_COMPONENT*) currentField->GetParent() )->m_Pos;

    /* Positions are calculated by the transpose matrix,  Rotating mirror. */
    wxPoint pt( panel->GetScreen()->m_Curseur - pos );

    currentField->m_Pos = pos + component->GetTransform().TransformCoordinate( pt );

    currentField->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode );
}


static void AbortMoveCmpField( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    Panel->ForceCloseManageCurseur = NULL;
    Panel->ManageCurseur = NULL;

    SCH_EDIT_FRAME* frame = (SCH_EDIT_FRAME*) Panel->GetParent();
    SCH_FIELD*      currentField = frame->GetCurrentField();

    if( currentField )
    {
        currentField->m_AddExtraText = frame->m_Multiflag;
        currentField->Draw( Panel, DC, wxPoint( 0, 0 ), g_XorMode );
        currentField->m_Flags = 0;
        currentField->m_Pos   = frame->m_OldPos;
        currentField->Draw( Panel, DC, wxPoint( 0, 0 ), g_XorMode );
    }

    frame->SetCurrentField( NULL );

    SAFE_DELETE( g_ItemToUndoCopy );
}


void SCH_EDIT_FRAME::RotateCmpField( SCH_FIELD* Field, wxDC* DC )
{
    int            fieldNdx, flag;
    LIB_COMPONENT* Entry;

    if( Field == NULL )
        return;
    if( Field->m_Text == wxEmptyString )
        return;

    SCH_COMPONENT* Cmp = (SCH_COMPONENT*) Field->GetParent();

    fieldNdx = Field->m_FieldId;
    flag     = 0;
    if( fieldNdx == REFERENCE )
    {
        Entry = CMP_LIBRARY::FindLibraryComponent(
            ( (SCH_COMPONENT*) Field->GetParent() )->GetLibName() );

        if( Entry != NULL )
        {
            if( Entry->GetPartCount() > 1 )
                flag = 1;
        }
    }

    /* save old cmp in undo list if not already in edit, or moving ... */
    if( Field->m_Flags == 0 )
        SaveCopyInUndoList( Cmp, UR_CHANGED );

    Field->m_AddExtraText = flag;
    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    if( Field->m_Orient == TEXT_ORIENT_HORIZ )
        Field->m_Orient = TEXT_ORIENT_VERT;
    else
        Field->m_Orient = TEXT_ORIENT_HORIZ;
    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    OnModify();
}


/****************************************************************************/
/* Edit the component text reference*/
/****************************************************************************/
void SCH_EDIT_FRAME::EditComponentReference( SCH_COMPONENT* Cmp, wxDC* DC )
{
    LIB_COMPONENT* Entry;
    int            flag = 0;

    if( Cmp == NULL )
        return;

    Entry = CMP_LIBRARY::FindLibraryComponent( Cmp->GetLibName() );

    if( Entry == NULL )
        return;

    if( Entry->GetPartCount() > 1 )
        flag = 1;

    wxString ref = Cmp->GetRef( GetSheet() );
    wxTextEntryDialog dlg( this, _( "Reference" ), _( "Component reference" ), ref );
    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    ref = dlg.GetValue( );
    ref.Trim( true );
    ref.Trim( false );

    if( !ref.IsEmpty() ) // New text entered
    {
        /* save old cmp in undo list if not already in edit, or moving ... */
        if( Cmp->m_Flags == 0 )
            SaveCopyInUndoList( Cmp, UR_CHANGED );
        Cmp->SetRef( GetSheet(), ref );

        Cmp->GetField( REFERENCE )->m_AddExtraText = flag;
        Cmp->GetField( REFERENCE )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
        Cmp->SetRef( GetSheet(), ref );
        Cmp->GetField( REFERENCE )->Draw( DrawPanel, DC, wxPoint( 0, 0 ),
                                          Cmp->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
        OnModify();
    }

    Cmp->DisplayInfo( this );
}


/*****************************************************************************/
/* Routine to change the selected text */
/*****************************************************************************/
void SCH_EDIT_FRAME::EditComponentValue( SCH_COMPONENT* Cmp, wxDC* DC )
{
    wxString       message;
    LIB_COMPONENT* Entry;

    if( Cmp == NULL )
        return;

    Entry = CMP_LIBRARY::FindLibraryComponent( Cmp->GetLibName() );

    if( Entry == NULL )
        return;

    SCH_FIELD* TextField = Cmp->GetField( VALUE );

    message = TextField->m_Text;

    wxTextEntryDialog dlg( this,  _( "Value" ), _( "Component value" ), message );
    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    message = dlg.GetValue( );
    message.Trim( true );
    message.Trim( false );

    if( !message.IsEmpty() )
    {
        /* save old cmp in undo list if not already in edit, or moving ... */
        if( Cmp->m_Flags == 0 )
            SaveCopyInUndoList( Cmp, UR_CHANGED );

        TextField->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
        TextField->m_Text = message;
        TextField->Draw( DrawPanel, DC, wxPoint( 0, 0 ),
                         Cmp->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
        OnModify();
    }

    Cmp->DisplayInfo( this );
}


void SCH_EDIT_FRAME::EditComponentFootprint( SCH_COMPONENT* Cmp, wxDC* DC )
{
    wxString       message;
    LIB_COMPONENT* Entry;

    if( Cmp == NULL )
        return;

    Entry = CMP_LIBRARY::FindLibraryComponent( Cmp->GetLibName() );

    if( Entry == NULL )
        return;

    SCH_FIELD* TextField = Cmp->GetField( FOOTPRINT );
    message = TextField->m_Text;

    wxTextEntryDialog dlg( this, _( "Footprint" ), _( "Component footprint" ), message );
    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    message = dlg.GetValue( );
    message.Trim( true );
    message.Trim( false );

    bool wasEmpty = false;
    if( TextField->m_Text.IsEmpty() )
        wasEmpty = true;

    // save old cmp in undo list if not already in edit, or moving ...
    if( Cmp->m_Flags == 0 )
        SaveCopyInUndoList( Cmp, UR_CHANGED );
    Cmp->GetField( FOOTPRINT )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    // Give a suitable position to the field if it was new,
    // and therefore has no position already given.
    if( wasEmpty && !message.IsEmpty() )
    {
        Cmp->GetField( FOOTPRINT )->m_Pos = Cmp->GetField( REFERENCE )->m_Pos;

        // add offset here - ? suitable heuristic below?
        Cmp->GetField( FOOTPRINT )->m_Pos.x +=
            ( Cmp->GetField( REFERENCE )->m_Pos.x - Cmp->m_Pos.x ) > 0 ?
            ( Cmp->GetField( REFERENCE )->m_Size.x ) :
            ( -1 * Cmp->GetField( REFERENCE )->m_Size.x );

        Cmp->GetField( FOOTPRINT )->m_Pos.y +=
            ( Cmp->GetField( REFERENCE )->m_Pos.y - Cmp->m_Pos.y ) > 0 ?
            ( Cmp->GetField( REFERENCE )->m_Size.y ) :
            ( -1 * Cmp->GetField( REFERENCE )->m_Size.y );

        Cmp->GetField( FOOTPRINT )->m_Orient = Cmp->GetField( REFERENCE )->m_Orient;
    }

    TextField->m_Text = message;
    Cmp->GetField( FOOTPRINT )->Draw( DrawPanel, DC, wxPoint( 0, 0 ),
                                      Cmp->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
    OnModify();

    Cmp->DisplayInfo( this );
}
