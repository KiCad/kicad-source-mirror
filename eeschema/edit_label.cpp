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
#include <eeschema_id.h>
#include <sch_view.h>


static PINSHEETLABEL_SHAPE  lastGlobalLabelShape = NET_INPUT;
static int                  lastTextOrientation = 0;
static bool                 lastTextBold = false;
static bool                 lastTextItalic = false;


SCH_TEXT* SCH_EDIT_FRAME::CreateNewText( int aType )
{
    SCH_TEXT* textItem = NULL;

    switch( aType )
    {
    case LAYER_NOTES:
        textItem = new SCH_TEXT( GetCrossHairPosition() );
        break;

    case LAYER_LOCLABEL:
        textItem = new SCH_LABEL( GetCrossHairPosition() );
        break;

    case LAYER_HIERLABEL:
        textItem = new SCH_HIERLABEL( GetCrossHairPosition() );
        textItem->SetShape( lastGlobalLabelShape );
        break;

    case LAYER_GLOBLABEL:
        textItem = new SCH_GLOBALLABEL( GetCrossHairPosition() );
        textItem->SetShape( lastGlobalLabelShape );
        break;

    default:
        DisplayError( this, wxT( "SCH_EDIT_FRAME::CreateNewText() Internal error" ) );
        return NULL;
    }

    textItem->SetBold( lastTextBold );
    textItem->SetItalic( lastTextItalic );
    textItem->SetLabelSpinStyle( lastTextOrientation );
    textItem->SetTextSize( wxSize( GetDefaultTextSize(), GetDefaultTextSize() ) );
    textItem->SetFlags( IS_NEW | IS_MOVED );

    EditSchematicText( textItem );

    if( textItem->GetText().IsEmpty() )
    {
        delete textItem;
        return NULL;
    }

    lastTextBold = textItem->IsBold();
    lastTextItalic = textItem->IsItalic();
    lastTextOrientation = textItem->GetLabelSpinStyle();

    if( textItem->Type() == SCH_GLOBAL_LABEL_T || textItem->Type() == SCH_HIER_LABEL_T )
        lastGlobalLabelShape = textItem->GetShape();

    return textItem;
}


void SCH_EDIT_FRAME::ConvertTextType( SCH_TEXT* aText, KICAD_T aNewType )
{
    bool selected = aText->IsSelected();

    wxCHECK_RET( aText->CanIncrementLabel(), "Cannot convert text type." );

    if( aText->Type() == aNewType )
        return;

    SCH_TEXT* newtext = nullptr;
    const wxPoint& position = aText->GetPosition();
    wxString txt = UnescapeString( aText->GetText() );

    // There can be characters in a SCH_TEXT object that can break labels so we have to
    // fix them here.
    if( aText->Type() == SCH_TEXT_T )
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
        wxASSERT_MSG( false, wxString::Format( "Invalid text type: %d.", aNewType ) );
        return;
    }

    // Copy the old text item settings to the new one.  Justifications are not copied
    // because they are not used in labels.  Justifications will be set to default value
    // in the new text item type.
    //
    newtext->SetFlags( aText->GetEditFlags() );
    newtext->SetShape( aText->GetShape() );
    newtext->SetLabelSpinStyle( aText->GetLabelSpinStyle() );
    newtext->SetTextSize( aText->GetTextSize() );
    newtext->SetThickness( aText->GetThickness() );
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
