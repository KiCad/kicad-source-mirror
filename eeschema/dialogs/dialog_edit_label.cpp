/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_text.h
 * @brief Implementation of the label properties dialog.
 */

#include <fctsys.h>
#include <wx/valgen.h>
#include <wx/valnum.h>
#include <sch_edit_frame.h>
#include <base_units.h>

#include <sch_draw_panel.h>
#include <general.h>
#include <draw_graphic_text.h>
#include <confirm.h>
#include <sch_text.h>
#include <typeinfo>
#include <widgets/unit_binder.h>

#include <dialog_edit_label_base.h>

class SCH_EDIT_FRAME;
class SCH_TEXT;


class DIALOG_LABEL_EDITOR : public DIALOG_LABEL_EDITOR_BASE
{
public:
    DIALOG_LABEL_EDITOR( SCH_EDIT_FRAME* parent, SCH_TEXT* aTextItem );
    ~DIALOG_LABEL_EDITOR();

    void SetTitle( const wxString& aTitle ) override
    {
        // This class is shared for numerous tasks: a couple of single line labels and
        // multi-line text fields.  Since the desired size of the multi-line text field editor
        // is often larger, we retain separate sizes based on the dialog titles.
        switch( m_CurrentText->Type() )
        {
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_LABEL_T:
            // labels can share retained settings probably.
            break;

        default:
            m_hash_key = TO_UTF8( aTitle );
            m_hash_key += typeid(*this).name();
        }

        DIALOG_LABEL_EDITOR_BASE::SetTitle( aTitle );
    }

private:
    virtual void OnEnterKey( wxCommandEvent& aEvent ) override;
    void OnCharHook( wxKeyEvent& aEvent );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    SCH_EDIT_FRAME* m_Parent;
    SCH_TEXT*       m_CurrentText;
    wxWindow*       m_activeTextCtrl;
    wxTextEntry*    m_activeTextEntry;
    UNIT_BINDER     m_textSize;
};


void SCH_EDIT_FRAME::EditSchematicText( SCH_TEXT* aTextItem )
{
    if( aTextItem == NULL )
        return;

    DIALOG_LABEL_EDITOR dialog( this, aTextItem );

    dialog.ShowModal();
}


// Don't allow text to disappear; it can be difficult to correct if you can't select it
const int MIN_TEXTSIZE = (int)( 0.01 * IU_PER_MM );
const int MAX_TEXTSIZE = INT_MAX;


DIALOG_LABEL_EDITOR::DIALOG_LABEL_EDITOR( SCH_EDIT_FRAME* aParent, SCH_TEXT* aTextItem ) :
    DIALOG_LABEL_EDITOR_BASE( aParent ),
    m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, false )
{
    m_Parent = aParent;
    m_CurrentText = aTextItem;

    switch( m_CurrentText->Type() )
    {
    case SCH_GLOBAL_LABEL_T:       SetTitle( _( "Global Label Properties" ) );           break;
    case SCH_HIERARCHICAL_LABEL_T: SetTitle( _( "Hierarchical Label Properties" ) );     break;
    case SCH_LABEL_T:              SetTitle( _( "Label Properties" ) );                  break;
    case SCH_SHEET_PIN_T:          SetTitle( _( "Hierarchical Sheet Pin Properties" ) ); break;
    default:                       SetTitle( _( "Text Properties" ) );                   break;
    }

    if( m_CurrentText->IsMultilineAllowed() )
    {
        m_activeTextCtrl = m_valueMultiLine;
        m_activeTextEntry = m_valueMultiLine;

        m_labelSingleLine->Show( false );  m_valueSingleLine->Show( false );
        m_labelCombo->Show( false );       m_valueCombo->Show( false );

        m_textEntrySizer->AddGrowableRow( 0 );
    }
    else if( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T || m_CurrentText->Type() == SCH_LABEL_T )
    {
        m_activeTextCtrl = m_valueCombo;
        m_activeTextEntry = m_valueCombo;

        m_labelSingleLine->Show( false );  m_valueSingleLine->Show( false );
        m_labelMultiLine->Show( false );   m_valueMultiLine->Show( false );
    }
    else
    {
        m_activeTextCtrl = m_valueSingleLine;
        m_activeTextEntry = m_valueSingleLine;

        m_labelCombo->Show( false );       m_valueCombo->Show( false );
        m_labelMultiLine->Show( false );   m_valueMultiLine->Show( false );
    }

    SetInitialFocus( m_activeTextCtrl );

    if( m_CurrentText->Type() != SCH_TEXT_T )
        ( (wxTextValidator*) m_activeTextCtrl->GetValidator() )->SetCharExcludes( wxT( " /" ) );

    m_TextShape->Show( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T ||
                       m_CurrentText->Type() == SCH_HIERARCHICAL_LABEL_T );

    m_sdbSizer1OK->SetDefault();
    Layout();

    // wxTextCtrls fail to generate wxEVT_CHAR events when the wxTE_MULTILINE flag is set,
    // so we have to listen to wxEVT_CHAR_HOOK events instead.
    m_valueMultiLine->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_EDITOR::OnCharHook ), nullptr, this );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient because the
    // various versions have different controls so we want to store sizes for each version.
    m_hash_key = TO_UTF8( GetTitle() );


    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_LABEL_EDITOR::~DIALOG_LABEL_EDITOR()
{
    m_valueMultiLine->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_EDITOR::OnCharHook ), nullptr, this );
}


// Sadly we store the orientation of hierarchical and global labels using a different
// int encoding than that for local labels:
//                   Global      Local
// Left justified      0           2
// Up                  1           3
// Right justified     2           0
// Down                3           1
static int mapOrientation( KICAD_T labelType, int aOrientation )
{
    if( labelType == SCH_LABEL_T )
        return aOrientation;

    switch( aOrientation )
    {
    case 0: return 2;
    case 2: return 0;
    default: return aOrientation;
    }
}


bool DIALOG_LABEL_EDITOR::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    m_activeTextEntry->SetValue( m_CurrentText->GetText() );

    if( m_valueCombo->IsShown() )
    {
        // Load the combobox with the existing labels of the same type
        std::set<wxString> existingLabels;
        SCH_SCREENS        allScreens;

        for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
            for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
                if( item->Type() == m_CurrentText->Type() )
                    existingLabels.insert( static_cast<SCH_TEXT*>( item )->GetText() );

        wxArrayString existingLabelArray;

        for( wxString label : existingLabels )
            existingLabelArray.push_back( label );

        // existingLabelArray.Sort();
        m_valueCombo->Append( existingLabelArray );
    }

    // Set text options:
    int orientation = mapOrientation( m_CurrentText->Type(), m_CurrentText->GetLabelSpinStyle() );
    m_TextOrient->SetSelection( orientation );

    m_TextShape->SetSelection( m_CurrentText->GetShape() );

    int style = 0;

    if( m_CurrentText->IsItalic() )
        style = 1;

    if( m_CurrentText->IsBold() )
        style += 2;

    m_TextStyle->SetSelection( style );

    m_textSize.SetValue( m_CurrentText->GetTextWidth() );

    return true;
}


/*!
 * wxEVT_COMMAND_ENTER event handler for single-line control
 */

void DIALOG_LABEL_EDITOR::OnEnterKey( wxCommandEvent& aEvent )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


/*!
 * wxEVT_CHAR_HOOK event handler for multi-line control
 */

void DIALOG_LABEL_EDITOR::OnCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_TAB )
    {
        int flags = 0;
        if( !aEvent.ShiftDown() )
            flags |= wxNavigationKeyEvent::IsForward;
        if( aEvent.ControlDown() )
            flags |= wxNavigationKeyEvent::WinChange;
        NavigateIn( flags );
    }
    else if( aEvent.GetKeyCode() == WXK_RETURN && aEvent.ShiftDown() )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
    }
    else
    {
        aEvent.Skip();
    }
}


bool DIALOG_LABEL_EDITOR::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( !m_textSize.Validate( MIN_TEXTSIZE, MAX_TEXTSIZE ) )
        return false;

    wxString text;

    /* save old text in undo list if not already in edit */
    /* or the label to be edited is part of a block */
    if( m_CurrentText->GetFlags() == 0 ||
        m_Parent->GetScreen()->m_BlockLocate.GetState() != STATE_NO_BLOCK )
        m_Parent->SaveCopyInUndoList( m_CurrentText, UR_CHANGED );

    m_Parent->GetCanvas()->Refresh();

    text = m_activeTextEntry->GetValue();

    if( !text.IsEmpty() )
        m_CurrentText->SetText( text );
    else if( !m_CurrentText->IsNew() )
    {
        DisplayError( this, _( "Empty Text!" ) );
        return false;
    }

    int orientation = m_TextOrient->GetSelection();
    m_CurrentText->SetLabelSpinStyle( mapOrientation( m_CurrentText->Type(), orientation ) );

    m_CurrentText->SetTextSize( wxSize( m_textSize.GetValue(), m_textSize.GetValue() ) );

    if( m_TextShape )
        m_CurrentText->SetShape( static_cast<PINSHEETLABEL_SHAPE>( m_TextShape->GetSelection() ) );

    int style = m_TextStyle->GetSelection();

    m_CurrentText->SetItalic( ( style & 1 ) );

    if( ( style & 2 ) )
    {
        m_CurrentText->SetBold( true );
        m_CurrentText->SetThickness( GetPenSizeForBold( m_CurrentText->GetTextWidth() ) );
    }
    else
    {
        m_CurrentText->SetBold( false );
        m_CurrentText->SetThickness( 0 );
    }

    m_Parent->RefreshItem( m_CurrentText );
    m_Parent->GetCanvas()->Refresh();
    m_Parent->OnModify();

    // Make the text size the new default size ( if it is a new text ):
    if( m_CurrentText->IsNew() )
        SetDefaultTextSize( m_CurrentText->GetTextWidth() );

    m_Parent->GetCanvas()->MoveCursorToCrossHair();

    return true;
}
