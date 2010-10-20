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


extern int     m_unit;

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
        if( Field->GetParent()->HasAlias( Text ) )
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

        SaveCopyInUndoList( Field->GetParent() );
        save = false;
        Field->GetParent()->SetName( Text );
    }

    ( ( LIB_DRAW_ITEM* )Field )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                                       &fieldText, DefaultTransform );

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

    ( ( LIB_DRAW_ITEM* )Field )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, drawMode,
                                       &fieldText, DefaultTransform );

    OnModify();
    UpdateAliasSelectList();
}


LIB_DRAW_ITEM* WinEDA_LibeditFrame::LocateItemUsingCursor()
{
    if( m_component == NULL )
        return NULL;

    if( ( m_drawItem == NULL ) || ( m_drawItem->m_Flags == 0 ) )
    {
        m_drawItem = m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                                  GetScreen()->m_MousePosition );

        if( m_drawItem == NULL )
            m_drawItem = m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                                      GetScreen()->m_Curseur );
    }

    return m_drawItem;
}
