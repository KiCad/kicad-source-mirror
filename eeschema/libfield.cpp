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


void LIB_EDIT_FRAME::EditField( wxDC* DC, LIB_FIELD* aField )
{
    wxString text;
    wxString title;
    wxString caption;
    wxString oldName;

    if( aField == NULL )
        return;

    LIB_COMPONENT* parent = aField->GetParent();

    // Editing the component value field is equivalent to creating a new component based
    // on the current component.  Set the dialog message to inform the user.
    if( aField->GetId() == VALUE )
    {
        caption = _( "Component Name" );
        title = _( "Enter a name to create a new component based on this one." );
    }
    else
    {
        caption = _( "Edit Field" );
        title.Printf( _( "Enter a new value for the %s field." ),
                      GetChars( aField->GetName().Lower() ) );
    }

    wxTextEntryDialog dlg( this, title, caption, aField->m_Text );

    if( dlg.ShowModal() != wxID_OK || dlg.GetValue() == aField->m_Text )
        return;

    text = dlg.GetValue();

    text.Replace( wxT( " " ), wxT( "_" ) );

    if( ( aField->GetId() == REFERENCE || aField->GetId() == VALUE ) && text.IsEmpty ( ) )
    {
        title.Printf( _( "A %s field cannot be empty." ), GetChars(aField->GetName().Lower() ) );
        DisplayError( this, title );
        return;
    }

    wxString fieldText = aField->GetFullText( m_unit );

    /* If the value field is changed, this is equivalent to creating a new component from
     * the old one.  Rename the component and remove any conflicting aliases to prevent name
     * errors when updating the library.
     */
    if( aField->GetId() == VALUE )
    {
        wxString msg;

        // Test the current library for name conflicts.
        if( m_library->FindEntry( text ) != NULL )
        {
            msg.Printf( _( "The name <%s> conflicts with an existing entry in the component \
library <%s>.\n\nDo you wish to replace the current component in library with this one?" ),
                        GetChars( text ),
                        GetChars( m_library->GetName() ) );

            int rsp = wxMessageBox( msg, _( "Confirm" ),
                                    wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT, this );

            if( rsp == wxNO )
                return;
        }

        // Test the current component for name conflicts.
        if( parent->HasAlias( text ) )
        {
            msg.Printf( _( "The current component already has an alias named <%s>.\n\nDo you \
wish to remove this alias from the component?" ),
                        GetChars( text ),
                        GetChars( m_library->GetName() ) );

            int rsp = wxMessageBox( msg, _( "Confirm" ), wxYES_NO | wxICON_QUESTION, this );

            if( rsp == wxNO )
                return;

            parent->RemoveAlias( text );
        }

        parent->SetName( text );

        // Test the library for any conflicts with the any aliases in the current component.
        if( parent->GetAliasCount() > 1 && m_library->Conflicts( parent ) )
        {
            msg.Printf( _( "The new component contains alias names that conflict with entries \
in the component library <%s>.\n\nDo you wish to remove all of the conflicting aliases from \
this component?" ),
                        GetChars( m_library->GetName() ) );

            int rsp = wxMessageBox( msg, _( "Confirm" ), wxYES_NO | wxICON_QUESTION, this );

            if( rsp == wxNO )
            {
                parent->SetName( fieldText );
                return;
            }

            wxArrayString aliases = parent->GetAliasNames( false );

            for( size_t i = 0;  i < aliases.GetCount();  i++ )
            {
                if( m_library->FindEntry( aliases[ i ] ) != NULL )
                    parent->RemoveAlias( aliases[ i ] );
            }
        }

        if( !parent->HasAlias( m_aliasName ) )
            m_aliasName = text;
    }
    else
    {
        aField->SetText( text );
    }

    if( !aField->InEditMode() )
    {
        SaveCopyInUndoList( parent );
        ( (LIB_DRAW_ITEM*) aField )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                                           &fieldText, DefaultTransform );
    }


    if( !aField->InEditMode() )
    {
        fieldText = aField->GetFullText( m_unit );
        ( (LIB_DRAW_ITEM*) aField )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
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
