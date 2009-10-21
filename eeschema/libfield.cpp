/*********************************************************************/
/*  EESchema - edition des librairies: Edition des champs ( Fields ) */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditfrm.h"
#include "class_library.h"


/* Routines locales */
static void ShowMoveField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


/* Variables locales */
extern int     m_unit;
static wxPoint StartCursor, LastTextPosition;


static void ExitMoveField( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;

    WinEDA_LibeditFrame* parent = ( WinEDA_LibeditFrame* ) Panel->GetParent();

    if( parent == NULL )
        return;

    LIB_DRAW_ITEM* item = parent->GetDrawItem();

    if( item == NULL )
        return;

    wxPoint curpos = Panel->GetScreen()->m_Curseur;

    Panel->GetScreen()->m_Curseur = StartCursor;
    ShowMoveField( Panel, DC, TRUE );
    Panel->GetScreen()->m_Curseur = curpos;
    item->m_Flags = 0;
    parent->SetDrawItem( NULL );
}


/*
 * Initialise le deplacement d'un champ ( ref ou Name )
 */
void WinEDA_LibeditFrame::StartMoveField( wxDC* DC, LIB_FIELD* field )
{
    wxPoint startPos;

    if( ( m_component == NULL ) || ( field == NULL ) )
        return;

    m_drawItem  = field;
    LastTextPosition = field->m_Pos;
    m_drawItem->m_Flags |= IS_MOVED;

    startPos.x = LastTextPosition.x;
    startPos.y = -LastTextPosition.y;
    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = startPos;
    DrawPanel->MouseToCursorSchema();

    DrawPanel->ManageCurseur = ShowMoveField;
    DrawPanel->ForceCloseManageCurseur = ExitMoveField;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
    StartCursor = GetScreen()->m_Curseur;

    DrawPanel->CursorOn( DC );
}


/*****************************************************************/
/* Routine d'affichage du texte 'Field' en cours de deplacement. */
/*  Routine normalement attachee au curseur                     */
/*****************************************************************/
static void ShowMoveField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) panel->GetParent();

    if( parent == NULL )
        return;

    LIB_FIELD* Field = (LIB_FIELD*) parent->GetDrawItem();

    if( Field == NULL )
        return;

    wxString text = Field->GetFullText( parent->GetUnit() );

    if( erase )
        Field->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &text,
                     DefaultTransformMatrix );

    LastTextPosition.x = panel->GetScreen()->m_Curseur.x;
    LastTextPosition.y = -panel->GetScreen()->m_Curseur.y;

    Field->m_Pos = LastTextPosition;
    Field->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &text,
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
    GetScreen()->SetModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    m_drawItem = NULL;
}


void WinEDA_LibeditFrame::EditField( wxDC* DC, LIB_FIELD* Field )
{
    wxString   Text;
    wxString   title;

    if( Field == NULL )
        return;

    switch( Field->m_FieldId )
    {
    case REFERENCE:
        title = wxT( "Reference:" );
        break;

    case VALUE:
        title = wxT( "Component Name / Value:" );
        break;

    default:
        break;
    }

    Text = Field->m_Text;

    /* FIXME: Use wxTextEntry dialog here and check for cancel button. */
    Get_Message( title, _( "Edit field" ), Text, this );
    Text.Replace( wxT( " " ), wxT( "_" ) );

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
        if( Field->GetParent()->m_AliasList.Index( Text, false ) != wxNOT_FOUND )
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
            msg.Printf( _( "The field name <%s> conflicts with an existing \
entry in the component library <%s>.\nPlease choose another name that does \
not conflict with any library entries." ),
                        GetChars( Text ),
                        GetChars( m_library->GetName() ) );
            DisplayError( this, msg );
            return;
        }

        Field->GetParent()->SetName( Text );
    }

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &fieldText,
                 DefaultTransformMatrix );

    if( !Text.IsEmpty() )
    {
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

    GetScreen()->SetModify();
    UpdateAliasSelectList();
}


/*
 * Routine de modification de l'orientation ( Horiz ou Vert. ) du champ.
 * si un champ est en cours d'edition, modif de celui ci.
 * sinon Modif du champ pointe par la souris
 */
void WinEDA_LibeditFrame::RotateField( wxDC* DC, LIB_FIELD* Field )
{

    if( Field == NULL )
        return;

    GetScreen()->SetModify();
    DrawPanel->CursorOff( DC );
    GRSetDrawMode( DC, g_XorMode );

    wxString fieldText = Field->GetFullText( m_unit );

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &fieldText,
                 DefaultTransformMatrix );

    if( Field->m_Orient )
        Field->m_Orient = 0;
    else
        Field->m_Orient = 900;

    int drawMode = g_XorMode;

    if( Field->m_Flags == 0 )
        drawMode = GR_DEFAULT_DRAWMODE;

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
