/*****************************************************/
/*  Component library edit field manipulation code.  */
/*****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"


static void ShowMoveField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


extern int     m_unit;
static wxPoint s_InitialPosition, s_LastPosition;


static void AbortMoveField( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;

    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) Panel->GetParent();

    if( parent == NULL )
        return;

    LIB_DRAW_ITEM* item = parent->GetDrawItem();

    if( item == NULL )
        return;

    wxPoint             curpos = Panel->GetScreen()->m_Curseur;

    Panel->GetScreen()->m_Curseur = s_InitialPosition;
    ShowMoveField( Panel, DC, true );
    Panel->GetScreen()->m_Curseur = curpos;
    item->m_Flags = 0;
    parent->SetDrawItem( NULL );
}


void WinEDA_LibeditFrame::StartMoveField( wxDC* DC, LIB_FIELD* field )
{
    wxPoint startPos;

    if( ( m_component == NULL ) || ( field == NULL ) )
        return;

    m_drawItem = field;
    s_InitialPosition = field->m_Pos;
    NEGATE( s_InitialPosition.y );
    m_drawItem->m_Flags |= IS_MOVED;

    DrawPanel->CursorOff( DC );
    s_LastPosition = s_InitialPosition;
    GetScreen()->m_Curseur = s_InitialPosition;
    DrawPanel->MouseToCursorSchema();

    DrawPanel->ManageCurseur = ShowMoveField;
    DrawPanel->ForceCloseManageCurseur = AbortMoveField;
    DrawPanel->ManageCurseur( DrawPanel, DC, true );
    s_InitialPosition = GetScreen()->m_Curseur;

    DrawPanel->CursorOn( DC );
}


/*
 * Routine to display text 'Field' on the move.
 * Normally called by cursor management code.
 */
static void ShowMoveField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) panel->GetParent();

    if( parent == NULL )
        return;

    LIB_FIELD* Field = (LIB_FIELD*) parent->GetDrawItem();

    if( Field == NULL )
        return;

    wxString text = Field->GetFullText( parent->GetUnit() );
    wxPoint  offset;
    offset.x = s_LastPosition.x - Field->m_Pos.x;
    offset.y = s_LastPosition.y + Field->m_Pos.y;

    if( erase )
        Field->Draw( panel, DC, offset, -1, g_XorMode, &text,
                     DefaultTransformMatrix );

    s_LastPosition = panel->GetScreen()->m_Curseur;
    offset.x = s_LastPosition.x - Field->m_Pos.x;
    offset.y = s_LastPosition.y + Field->m_Pos.y;

    Field->Draw( panel, DC, offset, -1, g_XorMode, &text,
                 DefaultTransformMatrix );
}


void WinEDA_LibeditFrame::PlaceField( wxDC* DC, LIB_FIELD* Field )
{
    if( Field == NULL )
        return;

    Field->m_Flags = 0;
    Field->m_Pos.x = GetScreen()->m_Curseur.x;
    Field->m_Pos.y = -GetScreen()->m_Curseur.y;
    DrawPanel->CursorOff( DC );

    wxString fieldText = Field->GetFullText( m_unit );

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE,
                 &fieldText, DefaultTransformMatrix );

    DrawPanel->CursorOn( DC );
    OnModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    m_drawItem = NULL;
}


void WinEDA_LibeditFrame::EditField( wxDC* DC, LIB_FIELD* Field )
{
    wxString Text;
    wxString title;
    bool     save = true;

    if( Field == NULL )
        return;

    switch( Field->m_FieldId )
    {
    case REFERENCE:
        title = _( "Reference:" );
        break;

    case VALUE:
        title = _( "Component Name / Value:" );
        break;

    default:
        break;
    }

    Text = Field->m_Text;

    {
        wxTextEntryDialog dlg( this, title, _( "Edit field" ), Text );
        if( dlg.ShowModal() != wxID_OK )
            return; // cancelled by user
        Text = dlg.GetValue( );
    }

    Text.Replace( wxT( " " ), wxT( "_" ) );

    if( Field->m_FieldId == REFERENCE || Field->m_FieldId == VALUE )
    {
        if( Text.IsEmpty ( ) )
        {
            DisplayError( this, _( "Value or Reference cannot be void. Aborted" ) );
            return;
        }
    }


    wxString fieldText = Field->GetFullText( m_unit );

    /* If the value field is changed, this is equivalent to creating a new
     * component from the old one.  Check for an existing library entry of
     * this "new" component and change the value only if there is no existing
     * entry with the same name.
     */
    if( Field->m_FieldId == VALUE && Text != Field->m_Text )
    {
        wxString msg;

        /* Test for an existing name in the current components alias list. */
        if( Field->GetParent()->GetAliasList().Index( Text, false ) != wxNOT_FOUND )
        {
            msg.Printf( _( "The field name <%s> is an existing alias of the \
component <%s>.\nPlease choose another name that does not conflict with any \
names in the alias list." ),
                        GetChars( Text ),
                        GetChars( Field->GetParent()->GetName() ) );
            DisplayError( this, msg );
            return;
        }

        /* Test for an existing entry in the library to prevent duplicate
         * entry names.
         */
        if( m_library && m_library->FindEntry( Text ) != NULL )
        {
            msg.Printf( _(
                           "The field name <%s> conflicts with an existing \
entry in the component library <%s>.\nPlease choose another name that does \
not conflict with any library entries."                                                                                                                                                           ),
                       GetChars( Text ),
                       GetChars( m_library->GetName() ) );
            DisplayError( this, msg );
            return;
        }

        SaveCopyInUndoList( Field->GetParent() );
        save = false;
        Field->GetParent()->SetName( Text );
    }

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &fieldText,
                 DefaultTransformMatrix );

    if( !Text.IsEmpty() )
    {
        if( save )
            SaveCopyInUndoList( Field->GetParent() );
        Field->m_Text = Text;
    }
    else
    {
        DisplayError( this, _( "No new text: no change" ) );
    }

    fieldText = Field->GetFullText( m_unit );
    int drawMode = g_XorMode;

    if( Field->m_Flags == 0 )
        drawMode = GR_DEFAULT_DRAWMODE;

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, drawMode, &fieldText,
                 DefaultTransformMatrix );

    OnModify();
    UpdateAliasSelectList();
}


/*
 * Rotate a field horizontally or vertically.
 *
 * If a field is being edited, rotate.
 * Otherwise rotate the field at the current cursor position.
 */
void WinEDA_LibeditFrame::RotateField( wxDC* DC, LIB_FIELD* Field )
{
    if( Field == NULL )
        return;

    OnModify();
    DrawPanel->CursorOff( DC );
    GRSetDrawMode( DC, g_XorMode );

    wxString fieldText = Field->GetFullText( m_unit );

    if( (Field->m_Flags & IS_MOVED) )
        ShowMoveField( DrawPanel, DC, false );
    else
        Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &fieldText,
                     DefaultTransformMatrix );

    if( Field->m_Orient ==  TEXT_ORIENT_VERT )
        Field->m_Orient = TEXT_ORIENT_HORIZ;
    else
        Field->m_Orient = TEXT_ORIENT_VERT;

    int drawMode = g_XorMode;

    if( Field->m_Flags == 0 )
        drawMode = GR_DEFAULT_DRAWMODE;

    if( (Field->m_Flags & IS_MOVED) )
        ShowMoveField( DrawPanel, DC, false );
    else
        Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, drawMode, &fieldText,
                     DefaultTransformMatrix );

    DrawPanel->CursorOn( DC );
}


LIB_DRAW_ITEM* WinEDA_LibeditFrame::LocateItemUsingCursor()
{
    if( m_component == NULL )
        return NULL;

    if( ( m_drawItem == NULL ) || ( m_drawItem->m_Flags == 0 ) )
    {
        m_drawItem = m_component->LocateDrawItem( m_unit, m_convert,
                                                  TYPE_NOT_INIT,
                                                  GetScreen()->m_MousePosition );

        if( m_drawItem == NULL )
            m_drawItem = m_component->LocateDrawItem( m_unit, m_convert,
                                                      TYPE_NOT_INIT,
                                                      GetScreen()->m_Curseur );
    }

    return m_drawItem;
}
