/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_struct.h>
#include <gr_text.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <general.h>
#include <sch_text.h>
#include <sch_view.h>

#include <wx/tokenzr.h>

#include "invoke_sch_dialog.h"

void SCH_EDIT_FRAME::ConvertTextType( SCH_TEXT* aText, KICAD_T aNewType )
{
    KICAD_T oldType = aText->Type();
    bool    selected = aText->IsSelected();

    if( oldType == aNewType )
        return;

    SCH_TEXT*        newtext     = nullptr;
    const wxPoint&   position    = aText->GetPosition();
    LABEL_SPIN_STYLE orientation = aText->GetLabelSpinStyle();
    wxString         txt         = UnescapeString( aText->GetText() );

    // There can be characters in a SCH_TEXT object that can break labels so we have to
    // fix them here.
    if( oldType == SCH_TEXT_T )
    {
        txt.Replace( "\n", "_" );
        txt.Replace( "\r", "_" );
        txt.Replace( "\t", "_" );
        txt.Replace( " ", "_" );
    }

    // label strings are "escaped" i.e. a '/' is replaced by "{slash}"
    if( aNewType != SCH_TEXT_T )
        txt = EscapeString( txt, CTX_NETNAME );

    switch( aNewType )
    {
    case SCH_LABEL_T:        newtext = new SCH_LABEL( position, txt );        break;
    case SCH_GLOBAL_LABEL_T: newtext = new SCH_GLOBALLABEL( position, txt );  break;
    case SCH_HIER_LABEL_T:   newtext = new SCH_HIERLABEL( position, txt );    break;
    case SCH_TEXT_T:         newtext = new SCH_TEXT( position, txt );         break;

    default:
        wxFAIL_MSG( wxString::Format( "Invalid text type: %d.", aNewType ) );
        return;
    }

    // Copy the old text item settings to the new one.  Justifications are not copied
    // because they are not used in labels.  Justifications will be set to default value
    // in the new text item type.
    //
    newtext->SetFlags( aText->GetEditFlags() );
    newtext->SetShape( aText->GetShape() );
    newtext->SetLabelSpinStyle( orientation );
    newtext->SetTextSize( aText->GetTextSize() );
    newtext->SetTextThickness( aText->GetTextThickness() );
    newtext->SetItalic( aText->IsItalic() );
    newtext->SetBold( aText->IsBold() );
    newtext->SetIsDangling( aText->IsDangling() );

    if( selected )
        m_toolManager->RunAction( EE_ACTIONS::removeItemFromSel, true, aText );

    if( !aText->IsNew() )
    {
        SaveCopyInUndoList( aText, UR_DELETED );
        SaveCopyInUndoList( newtext, UR_NEW, true );

        RemoveFromScreen( aText );
        AddToScreen( newtext );
    }

    if( selected )
        m_toolManager->RunAction( EE_ACTIONS::addItemToSel, true, newtext );

    // Otherwise, pointer is owned by the undo stack
    if( aText->IsNew() )
        delete aText;

    if( aNewType == SCH_TEXT_T )
    {
        if( newtext->IsDangling() )
        {
            newtext->SetIsDangling( false );
            GetCanvas()->GetView()->Update( newtext, KIGFX::REPAINT );
        }
    }
    else
        TestDanglingEnds();

    OnModify();
}


/*
 * Function to increment bus label numbers.  Adds aIncrement to labels which end in numbers.
 */
void IncrementLabelMember( wxString& name, int aIncrement )
{
    int  ii, nn;
    long number = 0;

    ii = name.Len() - 1; nn = 0;

    if( !wxIsdigit( name.GetChar( ii ) ) )
        return;

    while( (ii >= 0) && wxIsdigit( name.GetChar( ii ) ) )
    {
        ii--; nn++;
    }

    ii++;   /* digits are starting at ii position */
    wxString litt_number = name.Right( nn );

    if( litt_number.ToLong( &number ) )
    {
        number += aIncrement;
        name.Remove( ii ); name << number;
    }
}
