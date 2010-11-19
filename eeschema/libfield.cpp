/*****************************************************/
/*  Component library edit field manipulation code.  */
/*****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"
#include "template_fieldnames.h"


void LIB_EDIT_FRAME::EditField( wxDC* DC, LIB_FIELD* Field )
{
    wxString Text;
    wxString title;
    wxString oldName;

    if( Field == NULL )
        return;

    title = Field->GetName();
    Text = Field->m_Text;

    wxTextEntryDialog dlg( this, title + wxT( ":" ), _( "Edit field" ), Text );

    if( dlg.ShowModal() != wxID_OK || dlg.GetValue() == Text )
        return;

    Text = dlg.GetValue();

    Text.Replace( wxT( " " ), wxT( "_" ) );

    if( ( Field->m_FieldId == REFERENCE || Field->m_FieldId == VALUE ) && Text.IsEmpty ( ) )
    {
        DisplayError( this, title + _( " field cannot be empty." ) );
        return;
    }

    wxString fieldText = Field->GetFullText( m_unit );
    LIB_COMPONENT* parent = Field->GetParent();

    /* If the value field is changed, this is equivalent to creating a new
     * component from the old one.  Check for an existing library entry of
     * this "new" component and change the value only if there is no existing
     * entry with the same name.
     */
    if( Field->m_FieldId == VALUE && Text != Field->m_Text )
    {
        wxString msg;

        /* Test for an existing name in the current components alias list and in
         * the current library.
         */
        if( ( parent->HasAlias( Text ) && !parent->GetAlias( Text )->IsRoot() )
            || ( m_library && m_library->FindEntry( Text ) != NULL ) )
        {
            msg.Printf( _( "The field name <%s> conflicts with an existing \
entry in the component library <%s>.\nPlease choose another name that does \
not conflict with any library entries." ),
                        GetChars( Text ),
                        GetChars( m_library->GetName() ) );
            DisplayError( this, msg );
            return;
        }
    }

    if( Field->m_FieldId == VALUE && Field->m_Text == m_aliasName )
        m_aliasName = Text;

    if( !Field->InEditMode() )
    {
        SaveCopyInUndoList( parent );
        ( (LIB_DRAW_ITEM*) Field )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                                          &fieldText, DefaultTransform );
    }

    Field->SetText( Text );

    if( !Field->InEditMode() )
    {
        fieldText = Field->GetFullText( m_unit );
        ( (LIB_DRAW_ITEM*) Field )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                                          &fieldText, DefaultTransform );
    }

    OnModify();
    UpdateAliasSelectList();
}


LIB_DRAW_ITEM* LIB_EDIT_FRAME::LocateItemUsingCursor()
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
