/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <class_sch_screen.h>

#include <general.h>
#include <sch_component.h>
#include <libeditframe.h>
#include <class_library.h>
#include <template_fieldnames.h>
#include <dialog_edit_one_field.h>


void LIB_EDIT_FRAME::EditField( LIB_FIELD* aField )
{
    wxString newFieldValue;
    wxString title;
    wxString caption;
    wxString oldName;

    if( aField == NULL )
        return;

    LIB_PART* parent = aField->GetParent();

    wxASSERT( parent );

    // Editing the component value field is equivalent to creating a new component based
    // on the current component.  Set the dialog message to inform the user.
    if( aField->GetId() == VALUE )
    {
        caption = _( "Component Name" );
        title = _( "Enter a name to create a new component based on this one." );
    }
    else
    {
        caption.Printf( _( "Edit Field %s" ), GetChars( aField->GetName() ) );
        title.Printf( _( "Enter a new value for the %s field." ),
                      GetChars( aField->GetName().Lower() ) );
    }

    DIALOG_LIB_EDIT_ONE_FIELD dlg( this, caption, aField );

    // The dialog may invoke a kiway player for footprint fields
    // so we must use a quasimodal dialog.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    newFieldValue = dlg.GetText();
    wxString fieldText = aField->GetFullText( m_unit );

    /* If the value field is changed, this is equivalent to creating a new component from
     * the old one.  Rename the component and remove any conflicting aliases to prevent name
     * errors when updating the library.
     */
    if( aField->GetId() == VALUE && newFieldValue != aField->GetText() )
    {
        wxString msg;

        PART_LIB* lib = GetCurLib();

        // Test the current library for name conflicts.
        if( lib && lib->FindAlias( newFieldValue ) )
        {
            msg.Printf( _(
                "The name '%s' conflicts with an existing entry in the component library '%s'.\n\n"
                "Do you wish to replace the current component in the library with this one?" ),
                GetChars( newFieldValue ),
                GetChars( lib->GetName() )
                );

            int rsp = wxMessageBox( msg, _( "Confirm" ),
                                    wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT, this );

            if( rsp == wxNO )
                return;
        }

        // Test the current component for name conflicts.
        if( parent->HasAlias( newFieldValue ) )
        {
            msg.Printf( _( "The current component already has an alias named '%s'.\n\n"
                           "Do you wish to remove this alias from the component?" ),
                        GetChars( newFieldValue ) );

            int rsp = wxMessageBox( msg, _( "Confirm" ), wxYES_NO | wxICON_QUESTION, this );

            if( rsp == wxNO )
                return;

            parent->RemoveAlias( newFieldValue );
        }

        parent->SetName( newFieldValue );

        // Test the library for any conflicts with the any aliases in the current component.
        if( parent->GetAliasCount() > 1 && lib )
        {
            bool conflicts = false;
            wxArrayString libAliasNames, symbolAliasNames;

            lib->GetAliasNames( libAliasNames );
            symbolAliasNames = parent->GetAliasNames();

            for( size_t i = 0; i < symbolAliasNames.GetCount(); i++ )
            {
                if( libAliasNames.Index( symbolAliasNames[i] ) != wxNOT_FOUND )
                {
                    conflicts = true;
                    break;
                }
            }

            if( conflicts )
            {
                msg.Printf( _( "The new component contains alias names that conflict with "
                               "entries in the component library '%s'.\n\n"
                               "Do you wish to remove all of the conflicting aliases from "
                               "this component?" ),
                            GetChars( lib->GetName() )
                    );

                int rsp = wxMessageBox( msg, _( "Confirm" ), wxYES_NO | wxICON_QUESTION, this );

                if( rsp == wxNO )
                {
                    parent->SetName( fieldText );
                    return;
                }

                wxArrayString aliases = parent->GetAliasNames( false );

                for( size_t i = 0;  i < aliases.GetCount();  i++ )
                {
                    if( lib->FindAlias( aliases[ i ] ) != NULL )
                        parent->RemoveAlias( aliases[ i ] );
                }
            }
        }

        if( !parent->HasAlias( m_aliasName ) )
            m_aliasName = newFieldValue;
    }

    dlg.UpdateField( aField );

    if( !aField->InEditMode() )
        SaveCopyInUndoList( parent );

    m_canvas->Refresh();

    OnModify();
    UpdateAliasSelectList();
}
