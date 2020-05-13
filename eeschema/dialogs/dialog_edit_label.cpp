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

#include <fctsys.h>
#include <sch_edit_frame.h>
#include <base_units.h>
#include <sch_validators.h>
#include <tool/tool_manager.h>
#include <general.h>
#include <gr_text.h>
#include <confirm.h>
#include <sch_text.h>
#include <sch_component.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <widgets/unit_binder.h>
#include <dialog_edit_label_base.h>
#include <kicad_string.h>
#include <tool/actions.h>
#include <html_messagebox.h>

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
        case SCH_HIER_LABEL_T:
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
    void OnEnterKey( wxCommandEvent& aEvent ) override;
    void OnCharHook( wxKeyEvent& aEvent );
    void OnFormattingHelp( wxHyperlinkEvent& aEvent ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    wxString convertKIIDsToReferences( const wxString& aSource ) const;

    wxString convertReferencesToKIIDs( const wxString& aSource ) const;

    SCH_EDIT_FRAME* m_Parent;
    SCH_TEXT*       m_CurrentText;
    wxWindow*       m_activeTextCtrl;
    wxTextEntry*    m_activeTextEntry;
    UNIT_BINDER     m_textSize;
    SCH_NETNAME_VALIDATOR m_netNameValidator;
};


int InvokeDialogLabelEditor( SCH_EDIT_FRAME* aCaller, SCH_TEXT* aTextItem )
{
    DIALOG_LABEL_EDITOR dialog( aCaller, aTextItem );

    return dialog.ShowModal();
}


// Don't allow text to disappear; it can be difficult to correct if you can't select it
const int MIN_TEXTSIZE = (int)( 0.01 * IU_PER_MM );
const int MAX_TEXTSIZE = INT_MAX;


DIALOG_LABEL_EDITOR::DIALOG_LABEL_EDITOR( SCH_EDIT_FRAME* aParent, SCH_TEXT* aTextItem ) :
    DIALOG_LABEL_EDITOR_BASE( aParent ),
    m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, false ),
    m_netNameValidator( true )
{
    m_Parent = aParent;
    m_CurrentText = aTextItem;

    switch( m_CurrentText->Type() )
    {
    case SCH_GLOBAL_LABEL_T:       SetTitle( _( "Global Label Properties" ) );           break;
    case SCH_HIER_LABEL_T:         SetTitle( _( "Hierarchical Label Properties" ) );     break;
    case SCH_LABEL_T:              SetTitle( _( "Label Properties" ) );                  break;
    case SCH_SHEET_PIN_T:          SetTitle( _( "Hierarchical Sheet Pin Properties" ) ); break;
    default:                       SetTitle( _( "Text Properties" ) );                   break;
    }

    m_valueMultiLine->SetEOLMode( wxSTC_EOL_LF );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_valueMultiLine->SetScrollWidth( 1 );
    m_valueMultiLine->SetScrollWidthTracking( true );

    if( m_CurrentText->IsMultilineAllowed() )
    {
        m_activeTextCtrl = m_valueMultiLine;
        m_activeTextEntry = nullptr;

        m_labelSingleLine->Show( false );
        m_valueSingleLine->Show( false );
        m_labelCombo->Show( false );
        m_valueCombo->Show( false );

        m_textEntrySizer->AddGrowableRow( 0 );
    }
    else if( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T || m_CurrentText->Type() == SCH_LABEL_T )
    {
        m_activeTextCtrl = m_valueCombo;
        m_activeTextEntry = m_valueCombo;

        m_labelSingleLine->Show( false );  m_valueSingleLine->Show( false );
        m_labelMultiLine->Show( false );   m_valueMultiLine->Show( false );

        m_valueCombo->SetValidator( m_netNameValidator );
    }
    else
    {
        m_activeTextCtrl = m_valueSingleLine;
        m_activeTextEntry = m_valueSingleLine;

        m_labelCombo->Show( false );
        m_valueCombo->Show( false );
        m_labelMultiLine->Show( false );
        m_valueMultiLine->Show( false );

        if( m_CurrentText->Type() != SCH_TEXT_T )
            m_valueSingleLine->SetValidator( m_netNameValidator );

        m_valueCombo->SetValidator( m_netNameValidator );
    }

    SetInitialFocus( m_activeTextCtrl );

    m_TextShape->Show( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T ||
                       m_CurrentText->Type() == SCH_HIER_LABEL_T );

    if( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T )
    {
        wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
        infoFont.SetSymbolicSize( wxFONTSIZE_X_SMALL );
        m_note1->SetFont( infoFont );
        m_note2->SetFont( infoFont );
    }
    else
    {
        m_note1->Show( false );
        m_note2->Show( false );
    }

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


wxString DIALOG_LABEL_EDITOR::convertKIIDsToReferences( const wxString& aSource ) const
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                SCH_SHEET_LIST sheetList = m_Parent->Schematic().GetSheets();
                wxString       remainder;
                wxString       ref = token.BeforeFirst( ':', &remainder );

                SCH_SHEET_PATH refSheetPath;
                SCH_ITEM*      refItem = sheetList.GetItem( KIID( ref ), &refSheetPath );

                if( refItem && refItem->Type() == SCH_COMPONENT_T )
                {
                    SCH_COMPONENT* refComponent = static_cast<SCH_COMPONENT*>( refItem );
                    token = refComponent->GetRef( &refSheetPath, true ) + ":" + remainder;
                }
            }

            newbuf.append( "${" + token + "}" );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


wxString DIALOG_LABEL_EDITOR::convertReferencesToKIIDs( const wxString& aSource ) const
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                SCH_SHEET_LIST     sheets = m_Parent->Schematic().GetSheets();
                wxString           remainder;
                wxString           ref = token.BeforeFirst( ':', &remainder );
                SCH_REFERENCE_LIST references;

                sheets.GetComponents( references );

                for( size_t jj = 0; jj < references.GetCount(); jj++ )
                {
                    SCH_COMPONENT* refComponent = references[ jj ].GetComp();

                    if( ref == refComponent->GetRef( &references[ jj ].GetSheetPath(), true ) )
                    {
                        wxString test( remainder );

                        if( refComponent->ResolveTextVar( &test ) )
                            token = refComponent->m_Uuid.AsString() + ":" + remainder;

                        break;
                    }
                }
            }

            newbuf.append( "${" + token + "}" );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


bool DIALOG_LABEL_EDITOR::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( m_CurrentText->Type() == SCH_TEXT_T )
    {
        // show text variable cross-references in a human-readable format
        m_valueMultiLine->SetValue( convertKIIDsToReferences( m_CurrentText->GetText() ) );
    }
    else
    {
        // show control characters in a human-readable format
        m_activeTextEntry->SetValue( UnescapeString( m_CurrentText->GetText() ) );
    }

    if( m_valueCombo->IsShown() )
    {
        // Load the combobox with the existing labels of the same type
        std::set<wxString> existingLabels;
        SCH_SCREENS        allScreens( m_Parent->Schematic().Root() );

        for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
        {
            for( auto item : screen->Items().OfType( m_CurrentText->Type() ) )
            {
                auto textItem = static_cast<const SCH_TEXT*>( item );
                existingLabels.insert( UnescapeString( textItem->GetText() ) );
            }
        }

        wxArrayString existingLabelArray;

        for( const wxString& label : existingLabels )
            existingLabelArray.push_back( label );

        // existingLabelArray.Sort();
        m_valueCombo->Append( existingLabelArray );
    }

    // Set text options:
    m_TextOrient->SetSelection( static_cast<int>( m_CurrentText->GetLabelSpinStyle() ) );

    m_TextShape->SetSelection( static_cast<int>( m_CurrentText->GetShape() ) );

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
void DIALOG_LABEL_EDITOR::OnCharHook( wxKeyEvent& aEvt )
{
    if( aEvt.GetKeyCode() == WXK_TAB )
    {
        if( aEvt.ControlDown() )
        {
            int flags = 0;

            if( !aEvt.ShiftDown() )
                flags |= wxNavigationKeyEvent::IsForward;

            NavigateIn( flags );
        }
        else
        {
            m_valueMultiLine->Tab();
        }
    }
    else if( m_valueMultiLine->IsShown() && IsCtrl( 'Z', aEvt ) )
    {
        m_valueMultiLine->Undo();
    }
    else if( m_valueMultiLine->IsShown() && ( IsShiftCtrl( 'Z', aEvt ) || IsCtrl( 'Y', aEvt ) ) )
    {
        m_valueMultiLine->Redo();
    }
    else if( IsCtrl( 'X', aEvt ) )
    {
        m_valueMultiLine->Cut();
    }
    else if( IsCtrl( 'C', aEvt ) )
    {
        m_valueMultiLine->Copy();
    }
    else if( IsCtrl( 'V', aEvt ) )
    {
        m_valueMultiLine->Paste();
    }
    else
    {
        aEvt.Skip();
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
    if( m_CurrentText->GetEditFlags() == 0 )
        m_Parent->SaveCopyInUndoList( m_CurrentText, UR_CHANGED );

    m_Parent->GetCanvas()->Refresh();

    if( m_CurrentText->Type() == SCH_TEXT_T )
    {
        // convert any text variable cross-references to their UUIDs
        text = convertReferencesToKIIDs( m_valueMultiLine->GetValue() );
    }
    else
    {
        // labels need escaping
        text = EscapeString( m_activeTextEntry->GetValue(), CTX_NETNAME );
    }

    if( !text.IsEmpty() )
        m_CurrentText->SetText( text );
    else if( !m_CurrentText->IsNew() )
    {
        DisplayError( this, _( "Empty Text!" ) );
        return false;
    }

    m_CurrentText->SetLabelSpinStyle( (LABEL_SPIN_STYLE::SPIN) m_TextOrient->GetSelection() );

    m_CurrentText->SetTextSize( wxSize( m_textSize.GetValue(), m_textSize.GetValue() ) );

    if( m_TextShape )
        m_CurrentText->SetShape( (PINSHEETLABEL_SHAPE) m_TextShape->GetSelection() );

    int style = m_TextStyle->GetSelection();

    m_CurrentText->SetItalic( ( style & 1 ) );

    if( ( style & 2 ) )
    {
        m_CurrentText->SetBold( true );
        m_CurrentText->SetTextThickness( GetPenSizeForBold( m_CurrentText->GetTextWidth() ) );
    }
    else
    {
        m_CurrentText->SetBold( false );
        m_CurrentText->SetTextThickness( 0 );    // Use default pen width
    }

    m_Parent->RefreshItem( m_CurrentText );
    m_Parent->GetCanvas()->Refresh();
    m_Parent->OnModify();

    return true;
}


void DIALOG_LABEL_EDITOR::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    SCH_TEXT::ShowSyntaxHelp( this );
}
