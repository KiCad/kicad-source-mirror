/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_view.h>
#include <sch_component.h>
#include <lib_edit_frame.h>
#include <class_library.h>
#include <symbol_lib_table.h>
#include <template_fieldnames.h>
#include <dialog_edit_one_field.h>

#include <lib_manager.h>
#include <widgets/symbol_tree_pane.h>
#include <widgets/lib_tree.h>


void LIB_EDIT_FRAME::EditField( LIB_FIELD* aField )
{
    wxString newFieldValue;
    wxString caption;

    if( aField == NULL )
        return;

    LIB_PART* parent = aField->GetParent();
    wxCHECK( parent, /* void */ );

    // Editing the component value field is equivalent to creating a new component based
    // on the current component.  Set the dialog message to inform the user.
    if( aField->GetId() == VALUE )
        caption = _( "Edit Component Name" );
    else
        caption.Printf( _( "Edit %s Field" ), GetChars( aField->GetName() ) );

    DIALOG_LIB_EDIT_ONE_FIELD dlg( this, caption, aField );

    // The dialog may invoke a kiway player for footprint fields
    // so we must use a quasimodal dialog.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    newFieldValue = LIB_ID::FixIllegalChars( dlg.GetText(), LIB_ID::ID_SCH );
    wxString oldFieldValue = aField->GetFullText( m_unit );
    bool renamed = aField->GetId() == VALUE && newFieldValue != oldFieldValue;

    if( renamed )
    {
        wxString msg;
        wxString lib = GetCurLib();

        // Test the current library for name conflicts
        if( !lib.empty() && m_libMgr->PartExists( newFieldValue, lib ) )
        {
            msg.Printf( _(
                "The name \"%s\" conflicts with an existing entry in the symbol library \"%s\"." ),
                newFieldValue, lib );

            DisplayErrorMessage( this, msg );
            return;
        }

        SaveCopyInUndoList( parent, UR_LIB_RENAME );
        parent->SetName( newFieldValue );

        m_libMgr->UpdatePartAfterRename( parent, oldFieldValue, lib );

        // Reselect the renamed part
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, newFieldValue ) );
    }

    if( !aField->InEditMode() && !renamed )
        SaveCopyInUndoList( parent );

    dlg.UpdateField( aField );

    GetCanvas()->GetView()->Update( aField );
    GetCanvas()->Refresh();
    OnModify();
}
