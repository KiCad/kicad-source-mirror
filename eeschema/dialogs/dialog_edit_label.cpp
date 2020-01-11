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
#include <widgets/unit_binder.h>
#include <dialog_edit_label_base.h>
#include <kicad_string.h>
#include <tool/actions.h>
#include <bitmaps.h>

class SCH_EDIT_FRAME;
class SCH_TEXT;


class DIALOG_LABEL_EDITOR : public DIALOG_LABEL_EDITOR_BASE
{
public:
    DIALOG_LABEL_EDITOR( SCH_EDIT_FRAME* parent, std::deque<SCH_TEXT*> aTextItems );
    ~DIALOG_LABEL_EDITOR();

    void SetTitle( const wxString& aTitle ) override
    {
        // This class is shared for numerous tasks: a couple of single line labels and
        // multi-line text fields.  Since the desired size of the multi-line text field editor
        // is often larger, we retain separate sizes based on the dialog titles.
        switch( m_textItems.front()->Type() )
        {
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_LABEL_T:
            // labels can share retained settings probably.
            break;

        default:
            m_hash_key = TO_UTF8( aTitle );
            m_hash_key += typeid( *this ).name();
        }

        DIALOG_LABEL_EDITOR_BASE::SetTitle( aTitle );
    }

private:
    virtual void OnEnterKey( wxCommandEvent& aEvent ) override;
    void         OnCharHook( wxKeyEvent& aEvt );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    std::deque<SCH_TEXT*> m_textItems;
    SCH_EDIT_FRAME*       m_Parent;
    wxWindow*             m_activeTextCtrl;
    wxTextEntry*          m_activeTextEntry;
    UNIT_BINDER           m_textSize;
    SCH_NETNAME_VALIDATOR m_netNameValidator;
};


int InvokeDialogLabelEditor( SCH_EDIT_FRAME* aCaller, std::deque<SCH_TEXT*> aTextItems )
{
    DIALOG_LABEL_EDITOR dialog( aCaller, aTextItems);

    return dialog.ShowModal();
}

int InvokeDialogLabelEditor(SCH_EDIT_FRAME* aCaller, SCH_TEXT* aTextItem)
{
    std::deque<SCH_TEXT*> list;
    list.push_back( aTextItem );
    DIALOG_LABEL_EDITOR dialog( aCaller, list );

    return dialog.ShowModal();
}


struct shapeTypeStruct
{
    wxString             name;
    const BITMAP_OPAQUE* bitmap;
};

/*
 * Conversion map between PLOT_DASH_TYPE values and style names displayed
 */
// clang-format off
const std::map<PINSHEETLABEL_SHAPE, struct shapeTypeStruct> shapeTypeNames = {
    { PINSHEETLABEL_SHAPE::PS_INPUT,        { _( "Input" ),         pintype_input_xpm } },
    { PINSHEETLABEL_SHAPE::PS_OUTPUT,       { _( "Output" ),        pintype_output_xpm } },
    { PINSHEETLABEL_SHAPE::PS_BIDI,         { _( "Bidirectional" ), pintype_bidi_xpm } },
    { PINSHEETLABEL_SHAPE::PS_TRISTATE,     { _( "Tri-state" ),     pintype_3states_xpm } },
    { PINSHEETLABEL_SHAPE::PS_UNSPECIFIED,  { _( "Passive" ),       pintype_passive_xpm } },
};
// clang-format on


struct orientationTypeStruct
{
    wxString             name;
    const BITMAP_OPAQUE* bitmap;
};

/*
 * Conversion map between PLOT_DASH_TYPE values and style names displayed
 */
// clang-format off
const std::map<LABEL_SPIN_STYLE, struct orientationTypeStruct> orientationOptionsMap = {
    { LABEL_SPIN_STYLE::LEFT,   { _("Left"),    align_items_left_sm_xpm } },
    { LABEL_SPIN_STYLE::UP,     { _("Up"),      align_items_top_sm_xpm } },
    { LABEL_SPIN_STYLE::RIGHT,  { _("Right"),   align_items_right_sm_xpm } },
    { LABEL_SPIN_STYLE::BOTTOM, { _("Down"),    align_items_bottom_sm_xpm } },
};
// clang-format on

struct textStyleStruct
{
    wxString             name;
    const BITMAP_OPAQUE* bitmap;
};

/*
 * Conversion map between PLOT_DASH_TYPE values and style names displayed
 */
// clang-format off
const std::map<int, struct textStyleStruct> textStylesMap = {
    { 0, { _("Normal"),             font_normal_xpm } },
    { 1, { _("Italic"),             font_italic_xpm } },
    { 2, { _("Bold"),               font_bold_xpm } },
    { 3, { _("Bold and italic"),    font_bolditalic_xpm } },
};
// clang-format on


// Don't allow text to disappear; it can be difficult to correct if you can't select it
const int MIN_TEXTSIZE = (int)( 0.01 * IU_PER_MM );
const int MAX_TEXTSIZE = INT_MAX;


DIALOG_LABEL_EDITOR::DIALOG_LABEL_EDITOR(
        SCH_EDIT_FRAME* aParent, std::deque<SCH_TEXT*> aTextItems )
        : DIALOG_LABEL_EDITOR_BASE( aParent ),
          m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, false ),
          m_netNameValidator( true )
{
    m_Parent    = aParent;
    m_textItems = aTextItems;

    wxASSERT( m_textItems.size() > 0 );

    SCH_TEXT* firstItem = m_textItems.front();
    switch( firstItem->Type() )
    {
    case SCH_GLOBAL_LABEL_T:
        SetTitle( _( "Global Label Properties" ) );
        break;
    case SCH_HIER_LABEL_T:
        SetTitle( _( "Hierarchical Label Properties" ) );
        break;
    case SCH_LABEL_T:
        SetTitle( _( "Label Properties" ) );
        break;
    case SCH_SHEET_PIN_T:
        SetTitle( _( "Hierarchical Sheet Pin Properties" ) );
        break;
    default:
        SetTitle( _( "Text Properties" ) );
        break;
    }

    m_valueMultiLine->SetEOLMode( wxSTC_EOL_LF );

    if( firstItem->IsMultilineAllowed() )
    {
        m_activeTextCtrl  = m_valueMultiLine;
        m_activeTextEntry = nullptr;

        m_labelSingleLine->Show( false );
        m_valueSingleLine->Show( false );

        m_labelCombo->Show( false );
        m_valueCombo->Show( false );

        m_textEntrySizer->AddGrowableRow( 0 );
    }
    else if( firstItem->Type() == SCH_GLOBAL_LABEL_T || firstItem->Type() == SCH_LABEL_T )
    {
        m_activeTextCtrl  = m_valueCombo;
        m_activeTextEntry = m_valueCombo;

        m_labelSingleLine->Show( false );
        m_valueSingleLine->Show( false );

        m_labelMultiLine->Show( false );
        m_valueMultiLine->Show( false );

        m_valueCombo->SetValidator( m_netNameValidator );
    }
    else
    {
        m_activeTextCtrl  = m_valueSingleLine;
        m_activeTextEntry = m_valueSingleLine;

        m_labelCombo->Show( false );
        m_valueCombo->Show( false );

        m_labelMultiLine->Show( false );
        m_valueMultiLine->Show( false );

        if( firstItem->Type() != SCH_TEXT_T )
            m_valueSingleLine->SetValidator( m_netNameValidator );

        m_valueCombo->SetValidator( m_netNameValidator );
    }

    SetInitialFocus( m_activeTextCtrl );

    bool isShapableLabel =
            ( firstItem->Type() == SCH_GLOBAL_LABEL_T || firstItem->Type() == SCH_HIER_LABEL_T );

    m_labelShape->Show( isShapableLabel );
    m_comboShape->Show( isShapableLabel );

    if( isShapableLabel )
    {
        for( auto& shapeEntry : shapeTypeNames )
        {
            m_comboShape->Append( shapeEntry.second.name, KiBitmap( shapeEntry.second.bitmap ) );
        }
    }

    for( auto& orientationEntry : orientationOptionsMap )
    {
        m_comboOrientation->Append(
                orientationEntry.second.name, KiBitmap( orientationEntry.second.bitmap ) );
    }

    for( auto& styleEntry : textStylesMap )
    {
        m_comboStyle->Append( styleEntry.second.name, KiBitmap( styleEntry.second.bitmap ) );
    }

    m_sdbSizer1OK->SetDefault();
    Layout();

    // wxTextCtrls fail to generate wxEVT_CHAR events when the wxTE_MULTILINE flag is set,
    // so we have to listen to wxEVT_CHAR_HOOK events instead.
    m_valueMultiLine->Connect(
            wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_EDITOR::OnCharHook ), nullptr, this );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient because the
    // various versions have different controls so we want to store sizes for each version.
    m_hash_key = TO_UTF8( GetTitle() );


    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_LABEL_EDITOR::~DIALOG_LABEL_EDITOR()
{
    m_valueMultiLine->Disconnect(
            wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_EDITOR::OnCharHook ), nullptr, this );
}


bool DIALOG_LABEL_EDITOR::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    wxASSERT( m_textItems.size() > 0 );

    SCH_TEXT* firstItem = m_textItems.front();

    if( std::all_of( m_textItems.begin() + 1, m_textItems.end(),
                [&]( const SCH_TEXT* r ) { 
                    return r->GetText() == firstItem->GetText(); 
                } ) )
    {
        if( m_activeTextEntry )
            m_activeTextEntry->SetValue( UnescapeString( firstItem->GetText() ) );
        else
            m_valueMultiLine->SetValue( UnescapeString( firstItem->GetText() ) );
    }
    else
    {
        if( m_activeTextEntry )
            m_activeTextEntry->SetValue( "" );
        else
            m_valueMultiLine->SetValue( "" );
    }


    if( m_valueCombo->IsShown() )
    {
        // Load the combobox with the existing labels of the same type
        std::set<wxString> existingLabels;
        SCH_SCREENS        allScreens;

        for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
        {
            for( auto item : screen->Items().OfType( firstItem->Type() ) )
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
    if( std::all_of( m_textItems.begin() + 1, m_textItems.end(), 
                [&]( const SCH_TEXT* r ) {
                    return r->GetLabelSpinStyle() == firstItem->GetLabelSpinStyle();
                } ) )
    {
        auto findIt = orientationOptionsMap.find( firstItem->GetLabelSpinStyle() );

        wxCHECK_MSG( findIt != orientationOptionsMap.end(), false,
                "Text orientation not found in the orientation options map" );

        int idx = std::distance( orientationOptionsMap.cbegin(), findIt );
        m_comboOrientation->SetSelection( idx );
    }
    else
    {
        m_comboOrientation->SetSelection( wxNOT_FOUND );
    }

    if( m_comboShape->IsShown() )
    {
        if( std::all_of( m_textItems.begin() + 1, m_textItems.end(),
                    [&]( const SCH_TEXT* r ) 
                    { 
                        return r->GetShape() == firstItem->GetShape(); 
                    } ) )
        {
            auto findIt = shapeTypeNames.find( firstItem->GetShape() );

            wxCHECK_MSG( findIt != shapeTypeNames.end(), false,
                    "Text shape not found in the shape definition map" );

            int idx = std::distance( shapeTypeNames.cbegin(), findIt );
            m_comboShape->SetSelection( idx );
        }
        else
        {
            m_comboShape->SetSelection( wxNOT_FOUND );
        }
    }


    if( std::all_of( m_textItems.begin() + 1, m_textItems.end(), [&]( const SCH_TEXT* r ) {
            return r->IsItalic() == firstItem->IsItalic() && r->IsBold() == firstItem->IsBold();
        } ) )
    {
        int style = 0;

        if( firstItem->IsItalic() )
            style = 1;

        if( firstItem->IsBold() )
            style += 2;

        m_comboStyle->SetSelection( style );
    }
    else
    {
        m_comboStyle->SetSelection( wxNOT_FOUND );
    }


    if( std::all_of( m_textItems.begin() + 1, m_textItems.end(), [&]( const SCH_TEXT* r ) {
            return r->GetTextWidth() == firstItem->GetTextWidth();
        } ) )
    {
        m_textSize.SetValue( firstItem->GetTextWidth() );
    }
    else
    {
        m_textSize.SetValue( INDETERMINATE );
    }


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

    for( auto& textItem : m_textItems )
    {
        /* save old text in undo list if not already in edit */
        if( textItem->GetEditFlags() == 0 )
            m_Parent->SaveCopyInUndoList( textItem, UR_CHANGED );

        m_Parent->GetCanvas()->Refresh();

        // Escape string only if is is a label. For a simple graphic text do not change anything
        if( textItem->Type() == SCH_TEXT_T )
            text = m_valueMultiLine->GetValue();
        else
            text = EscapeString( m_activeTextEntry->GetValue(), CTX_NETNAME );

        if( !text.IsEmpty() )
            textItem->SetText( text );
        else if( !textItem->IsNew() && m_textItems.size() == 1 )
        {
            DisplayError( this, _( "Empty Text!" ) );
            return false;
        }

        if( m_comboOrientation->GetSelection() != wxNOT_FOUND )
        {
            int selection = m_comboOrientation->GetSelection();

            wxCHECK_MSG( selection < (int) orientationOptionsMap.size(), false,
                    "Selected line type index exceeds size of line type lookup map" );

            auto it = orientationOptionsMap.begin();
            std::advance( it, selection );

            textItem->SetLabelSpinStyle( it->first );
        }

        if( !m_textSize.IsIndeterminate() )
        {
            textItem->SetTextSize( wxSize( m_textSize.GetValue(), m_textSize.GetValue() ) );
        }

        if( m_comboShape->IsShown() && m_comboShape->GetSelection() != wxNOT_FOUND )
        {
            int selection = m_comboShape->GetSelection();

            wxCHECK_MSG( selection < (int) shapeTypeNames.size(), false,
                    "Selected line type index exceeds size of line type lookup map" );

            auto it = shapeTypeNames.begin();
            std::advance( it, selection );

            textItem->SetShape( it->first );
        }

        if( m_comboStyle->GetSelection() != wxNOT_FOUND )
        {
            int style = m_comboStyle->GetSelection();

            textItem->SetItalic( ( style & 1 ) );

            if( ( style & 2 ) )
            {
                textItem->SetBold( true );
                textItem->SetThickness( GetPenSizeForBold( textItem->GetTextWidth() ) );
            }
            else
            {
                textItem->SetBold( false );
                textItem->SetThickness( 0 );
            }
        }

        m_Parent->RefreshItem( textItem );
        m_Parent->GetCanvas()->Refresh();
        m_Parent->OnModify();

        // Make the text size the new default size ( if it is a new text ):
        if( textItem->IsNew() )
            SetDefaultTextSize( textItem->GetTextWidth() );
    }

    return true;
}