/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/color_swatch.h>
#include <settings/color_settings.h>
#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <gr_text.h>
#include <confirm.h>
#include <schematic.h>
#include <dialogs/html_message_box.h>
#include <dialog_label_properties.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <sch_label.h>
#include <sch_commit.h>


DIALOG_LABEL_PROPERTIES::DIALOG_LABEL_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_LABEL_BASE* aLabel ) :
        DIALOG_LABEL_PROPERTIES_BASE( aParent ),
        m_Parent( aParent ),
        m_currentLabel( aLabel ),
        m_activeTextEntry( nullptr ),
        m_netNameValidator( true ),
        m_fields( nullptr ),
        m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, false ),
        m_helpWindow( nullptr )
{
    COLOR_SETTINGS* colorSettings = m_Parent->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_fields = new FIELDS_GRID_TABLE<SCH_FIELD>( this, aParent, m_grid, m_currentLabel );
    m_width = 100;  // Will be later set to a better value
    m_delayedFocusRow = -1;
    m_delayedFocusColumn = FDC_VALUE;

    if( m_currentLabel->Type() == SCH_GLOBAL_LABEL_T || m_currentLabel->Type() == SCH_LABEL_T )
    {
        m_activeTextEntry = m_valueCombo;
        SetInitialFocus( m_valueCombo );

        m_labelSingleLine->Show( false );
        m_valueSingleLine->Show( false );

        m_valueCombo->SetValidator( m_netNameValidator );
    }
    else if( m_currentLabel->Type() == SCH_HIER_LABEL_T )
    {
        m_activeTextEntry = m_valueSingleLine;
        SetInitialFocus( m_valueSingleLine );

        m_labelCombo->Show( false );
        m_valueCombo->Show( false );

        m_valueSingleLine->SetValidator( m_netNameValidator );
    }
    else if( m_currentLabel->Type() == SCH_DIRECTIVE_LABEL_T )
    {
        SetInitialFocus( m_grid );
        m_delayedFocusRow = 0;

        m_labelSingleLine->Show( false );
        m_valueSingleLine->Show( false );
        m_labelCombo->Show( false );
        m_valueCombo->Show( false );
        m_syntaxHelp->Show( false );
        m_textEntrySizer->Show( false );

        m_textSizeLabel->SetLabel( _( "Pin length:" ) );
    }

    switch( m_currentLabel->Type() )
    {
    case SCH_GLOBAL_LABEL_T:    SetTitle( _( "Global Label Properties" ) );           break;
    case SCH_HIER_LABEL_T:      SetTitle( _( "Hierarchical Label Properties" ) );     break;
    case SCH_LABEL_T:           SetTitle( _( "Label Properties" ) );                  break;
    case SCH_DIRECTIVE_LABEL_T: SetTitle( _( "Directive Label Properties" ) );        break;
    case SCH_SHEET_PIN_T:       SetTitle( _( "Hierarchical Sheet Pin Properties" ) ); break;
    default:            UNIMPLEMENTED_FOR( m_currentLabel->GetClass() );              break;
    }

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this,
                                                      [&]( wxCommandEvent& aEvent )
                                                      {
                                                          OnAddField( aEvent );
                                                      } ) );
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Show/hide columns according to user's preference
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        m_grid->ShowHideColumns( cfg->m_Appearance.edit_label_visible_columns );
        m_shownColumns = m_grid->GetShownColumns();
    }

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmap( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmap( BITMAPS::small_down ) );

    m_separator1->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmap( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmap( BITMAPS::text_italic ) );

    m_separator2->SetIsSeparator();

    m_spin0->SetIsRadioButton();
    m_spin1->SetIsRadioButton();
    m_spin2->SetIsRadioButton();
    m_spin3->SetIsRadioButton();

    m_separator3->SetIsSeparator();

    m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_textColorSwatch->SetSwatchBackground( schematicBackground );

    // Show/hide relevant controls
    if( m_currentLabel->Type() == SCH_GLOBAL_LABEL_T || m_currentLabel->Type() == SCH_HIER_LABEL_T )
    {
        m_dot->Hide();
        m_circle->Hide();
        m_diamond->Hide();
        m_rectangle->Hide();

        m_spin0->SetBitmap( KiBitmap( BITMAPS::label_align_left ) );
        m_spin1->SetBitmap( KiBitmap( BITMAPS::label_align_right ) );
        m_spin2->SetBitmap( KiBitmap( BITMAPS::label_align_bottom ) );
        m_spin3->SetBitmap( KiBitmap( BITMAPS::label_align_top ) );
    }
    else if( m_currentLabel->Type() == SCH_DIRECTIVE_LABEL_T )
    {
        m_input->Hide();
        m_output->Hide();
        m_bidirectional->Hide();
        m_triState->Hide();
        m_passive->Hide();

        m_fontLabel->SetLabel( _( "Orientation:" ) );
        m_fontCtrl->Hide();
        m_separator1->Hide();
        m_bold->Hide();
        m_italic->Hide();
        m_separator2->Hide();
        m_spin0->SetBitmap( KiBitmap( BITMAPS::pinorient_down ) );
        m_spin1->SetBitmap( KiBitmap( BITMAPS::pinorient_up ) );
        m_spin2->SetBitmap( KiBitmap( BITMAPS::pinorient_right ) );
        m_spin3->SetBitmap( KiBitmap( BITMAPS::pinorient_left ) );
        m_separator3->Hide();

        m_formattingGB->Detach( m_fontCtrl );
        m_formattingGB->Detach( m_iconBar );
        m_formattingGB->Add( m_iconBar, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT, 5 );
    }
    else
    {
        m_shapeSizer->Show( false );

        m_spin0->SetBitmap( KiBitmap( BITMAPS::text_align_left ) );
        m_spin1->SetBitmap( KiBitmap( BITMAPS::text_align_right ) );
        m_spin2->SetBitmap( KiBitmap( BITMAPS::text_align_bottom ) );
        m_spin3->SetBitmap( KiBitmap( BITMAPS::text_align_top ) );
    }

    if( !m_currentLabel->AutoRotateOnPlacementSupported() )
    {
        m_autoRotate->Hide();
        wxSizer* parentSizer = m_autoRotate->GetContainingSizer();
        parentSizer->Detach( m_autoRotate );
        parentSizer->Layout();
    }

    SetupStandardButtons();

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient because the
    // various versions have different controls so we want to store sizes for each version.
    m_hash_key = TO_UTF8( GetTitle() );

    m_spin0->Bind( wxEVT_BUTTON, &DIALOG_LABEL_PROPERTIES::onSpinButton, this );
    m_spin1->Bind( wxEVT_BUTTON, &DIALOG_LABEL_PROPERTIES::onSpinButton, this );
    m_spin2->Bind( wxEVT_BUTTON, &DIALOG_LABEL_PROPERTIES::onSpinButton, this );
    m_spin3->Bind( wxEVT_BUTTON, &DIALOG_LABEL_PROPERTIES::onSpinButton, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        if( cfg->m_Appearance.edit_label_width > 0 && cfg->m_Appearance.edit_label_height > 0 )
            SetSize( cfg->m_Appearance.edit_label_width, cfg->m_Appearance.edit_label_height );
    }
}


DIALOG_LABEL_PROPERTIES::~DIALOG_LABEL_PROPERTIES()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_Appearance.edit_label_visible_columns = m_grid->GetShownColumnsAsString();
        cfg->m_Appearance.edit_label_width = GetSize().x;
        cfg->m_Appearance.edit_label_height = GetSize().y;
    }

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


bool DIALOG_LABEL_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( m_activeTextEntry )
    {
        // show control characters in a human-readable format
        wxString text = UnescapeString( m_currentLabel->GetText() );

        // show text variable cross-references in a human-readable format
        text = m_currentLabel->Schematic()->ConvertKIIDsToRefs( text );

        m_activeTextEntry->SetValue( text );
    }

    if( m_currentLabel->Type() == SCH_GLOBAL_LABEL_T || m_currentLabel->Type() == SCH_LABEL_T )
    {
        // Load the combobox with the existing labels of the same type
        std::set<wxString>                      existingLabels;
        std::vector<std::shared_ptr<BUS_ALIAS>> busAliases;
        SCH_SCREENS                             allScreens( m_Parent->Schematic().Root() );

        for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
        {
            for( SCH_ITEM* item : screen->Items().OfType( m_currentLabel->Type() ) )
            {
                const SCH_LABEL_BASE* label = static_cast<const SCH_LABEL_BASE*>( item );
                existingLabels.insert( UnescapeString( label->GetText() ) );
            }

            // Add global power labels from power symbols
            if( m_currentLabel->Type() == SCH_GLOBAL_LABEL_T )
            {
                for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_LOCATE_POWER_T ) )
                {
                    const SCH_SYMBOL* power = static_cast<const SCH_SYMBOL*>( item );

                    // Ensure the symbol has the Power (i.e. equivalent to a global label
                    // before adding its value in list
                    if( power->IsSymbolLikePowerGlobalLabel() )
                        existingLabels.insert( UnescapeString( power->GetField( VALUE_FIELD )->GetText() ) );
                }
            }

            auto& sheetAliases = screen->GetBusAliases();
            busAliases.insert( busAliases.end(), sheetAliases.begin(), sheetAliases.end() );
        }

        // Add bus aliases to label list
        for( const std::shared_ptr<BUS_ALIAS>& busAlias : busAliases )
            existingLabels.insert( wxT( "{" ) + busAlias->GetName() + wxT( "}" ) );

        wxArrayString existingLabelArray;

        for( const wxString& label : existingLabels )
            existingLabelArray.push_back( label );

        m_valueCombo->Append( existingLabelArray );
    }

    // Push a copy of each field into m_updateFields
    for( SCH_FIELD& field : m_currentLabel->GetFields() )
    {
        SCH_FIELD field_copy( field );

        // change offset to be symbol-relative
        field_copy.Offset( -m_currentLabel->GetPosition() );

        m_fields->push_back( field_copy );
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->size() );
    m_grid->ProcessTableMessage( msg );
    AdjustGridColumns( m_grid->GetRect().GetWidth() );

    if( m_shapeSizer->AreAnyItemsShown() )
    {
        switch( m_currentLabel->GetShape() )
        {
        case LABEL_FLAG_SHAPE::L_INPUT:       m_input->SetValue( true );         break;
        case LABEL_FLAG_SHAPE::L_OUTPUT:      m_output->SetValue( true );        break;
        case LABEL_FLAG_SHAPE::L_BIDI:        m_bidirectional->SetValue( true ); break;
        case LABEL_FLAG_SHAPE::L_TRISTATE:    m_triState->SetValue( true );      break;
        case LABEL_FLAG_SHAPE::L_UNSPECIFIED: m_passive->SetValue( true );       break;
        case LABEL_FLAG_SHAPE::F_DOT:         m_dot->SetValue( true );           break;
        case LABEL_FLAG_SHAPE::F_ROUND:       m_circle->SetValue( true );        break;
        case LABEL_FLAG_SHAPE::F_DIAMOND:     m_diamond->SetValue( true );       break;
        case LABEL_FLAG_SHAPE::F_RECTANGLE:   m_rectangle->SetValue( true );     break;
        }
    }

    m_fontCtrl->SetFontSelection( m_currentLabel->GetFont() );

    if( m_currentLabel->Type() == SCH_DIRECTIVE_LABEL_T )
        m_textSize.SetValue( static_cast<SCH_DIRECTIVE_LABEL*>( m_currentLabel )->GetPinLength() );
    else
        m_textSize.SetValue( m_currentLabel->GetTextWidth() );

    m_bold->Check( m_currentLabel->IsBold() );
    m_italic->Check( m_currentLabel->IsItalic() );
    m_textColorSwatch->SetSwatchColor( m_currentLabel->GetTextColor(), false );

    switch( m_currentLabel->GetTextSpinStyle() )
    {
    case TEXT_SPIN_STYLE::RIGHT:  m_spin0->Check( true ); break;
    case TEXT_SPIN_STYLE::LEFT:   m_spin1->Check( true ); break;
    case TEXT_SPIN_STYLE::UP:     m_spin2->Check( true ); break;
    case TEXT_SPIN_STYLE::BOTTOM: m_spin3->Check( true ); break;
    }

    if( m_currentLabel->AutoRotateOnPlacementSupported() )
        m_autoRotate->SetValue( m_currentLabel->AutoRotateOnPlacement() );

    return true;
}


/*!
 * wxEVT_COMMAND_ENTER event handler for single-line control
 */
void DIALOG_LABEL_PROPERTIES::OnEnterKey( wxCommandEvent& aEvent )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


void DIALOG_LABEL_PROPERTIES::OnValueCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_TAB )
    {
        if( aEvent.ShiftDown() )
        {
            m_textSizeCtrl->SetFocusFromKbd();
        }
        else if( !m_fields->empty() )
        {
            m_grid->SetFocusFromKbd();
            m_grid->MakeCellVisible( 0, 0 );
            m_grid->SetGridCursor( 0, 0 );
        }
        else
        {
            m_textSizeCtrl->SetFocusFromKbd();
        }
    }
    else
    {
        aEvent.Skip();
    }
}


static bool positioningChanged( const SCH_FIELD& a, const SCH_FIELD& b )
{
    return a.GetPosition() != b.GetPosition()
            || a.GetHorizJustify() != b.GetHorizJustify()
            || a.GetVertJustify() != b.GetVertJustify()
            || a.GetTextAngle() != b.GetTextAngle();
}


static bool positioningChanged( FIELDS_GRID_TABLE<SCH_FIELD>* a, std::vector<SCH_FIELD>& b )
{
    for( size_t i = 0; i < a->size() && i < b.size(); ++i )
    {
        if( positioningChanged( a->at( i ), b.at( i ) ) )
            return true;
    }

    return false;
}


bool DIALOG_LABEL_PROPERTIES::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Don't allow text to disappear; it can be difficult to correct if you can't select it
    if( !m_textSize.Validate( 0.01, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    SCH_COMMIT commit( m_Parent );
    wxString   text;

    /* save old text in undo list if not already in edit */
    if( m_currentLabel->GetEditFlags() == 0 )
        commit.Modify( m_currentLabel, m_Parent->GetScreen() );

    m_Parent->GetCanvas()->Refresh();

    if( m_activeTextEntry )
    {
        // labels need escaping
        text = EscapeString( m_activeTextEntry->GetValue(), CTX_NETNAME );

        // convert any text variable cross-references to their UUIDs
        text = m_currentLabel->Schematic()->ConvertRefsToKIIDs( text );

#ifdef __WXMAC__
        // On macOS CTRL+Enter produces '\r' instead of '\n' regardless of EOL setting
        text.Replace( wxS( "\r" ), wxS( "\n" ) );
#endif

        if( text.IsEmpty() && !m_currentLabel->IsNew() )
        {
            DisplayError( this, _( "Label can not be empty." ) );
            return false;
        }

        m_currentLabel->SetText( text );
    }

    bool doAutoplace = false;

    // change all field positions from relative to absolute
    for( SCH_FIELD& field : *m_fields )
    {
        field.Offset( m_currentLabel->GetPosition() );

        if( field.GetCanonicalName() == wxT( "Netclass" ) )
        {
            field.SetLayer( LAYER_NETCLASS_REFS );
        }
        else if( field.GetCanonicalName() == wxT( "Intersheetrefs" ) )
        {
            if( field.IsVisible() != m_Parent->Schematic().Settings().m_IntersheetRefsShow )
            {
                DisplayInfoMessage( this, _( "Intersheet reference visibility is "
                                             "controlled globally from "
                                             "Schematic Setup > General > Formatting" ) );
            }

            field.SetLayer( LAYER_INTERSHEET_REFS );
        }
        else
        {
            field.SetLayer( LAYER_FIELDS );
        }
    }

    if( positioningChanged( m_fields, m_currentLabel->GetFields() ) )
        m_currentLabel->ClearFieldsAutoplaced();
    else
        doAutoplace = true;

    for( int ii = m_fields->GetNumberRows() - 1; ii >= 0; ii-- )
    {
        SCH_FIELD&      field = m_fields->at( ii );
        const wxString& fieldName = field.GetCanonicalName();
        const wxString& fieldText = field.GetText();

        if( fieldName.IsEmpty() && fieldText.IsEmpty() )
            m_fields->erase( m_fields->begin() + ii );
        else if( fieldName == wxT( "Netclass" ) && fieldText.IsEmpty() )
            m_fields->erase( m_fields->begin() + ii );
        else if( fieldName.IsEmpty() )
            field.SetName( _( "untitled" ) );
    }

    m_currentLabel->SetFields( *m_fields );

    if( m_shapeSizer->AreAnyItemsShown() )
    {
        if( m_bidirectional->GetValue() )  m_currentLabel->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
        else if( m_input->GetValue() )     m_currentLabel->SetShape( LABEL_FLAG_SHAPE::L_INPUT );
        else if( m_output->GetValue() )    m_currentLabel->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );
        else if( m_triState->GetValue() )  m_currentLabel->SetShape( LABEL_FLAG_SHAPE::L_TRISTATE );
        else if( m_passive->GetValue() )   m_currentLabel->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );
        else if( m_dot->GetValue() )       m_currentLabel->SetShape( LABEL_FLAG_SHAPE::F_DOT );
        else if( m_circle->GetValue() )    m_currentLabel->SetShape( LABEL_FLAG_SHAPE::F_ROUND );
        else if( m_diamond->GetValue() )   m_currentLabel->SetShape( LABEL_FLAG_SHAPE::F_DIAMOND );
        else if( m_rectangle->GetValue() ) m_currentLabel->SetShape( LABEL_FLAG_SHAPE::F_RECTANGLE );
    }

    if( m_fontCtrl->HaveFontSelection() )
    {
        m_currentLabel->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(),
                                                               m_italic->IsChecked() ) );
    }

    if( m_currentLabel->Type() == SCH_DIRECTIVE_LABEL_T )
        static_cast<SCH_DIRECTIVE_LABEL*>( m_currentLabel )->SetPinLength( m_textSize.GetValue() );
    else if( m_currentLabel->GetTextWidth() != m_textSize.GetValue() )
        m_currentLabel->SetTextSize( VECTOR2I( m_textSize.GetValue(), m_textSize.GetValue() ) );

    if( m_bold->IsChecked() != m_currentLabel->IsBold() )
    {
        if( m_bold->IsChecked() )
        {
            m_currentLabel->SetBold( true );
            m_currentLabel->SetTextThickness( GetPenSizeForBold( m_currentLabel->GetTextWidth() ) );
        }
        else
        {
            m_currentLabel->SetBold( false );
            m_currentLabel->SetTextThickness( 0 ); // Use default pen width
        }
    }

    m_currentLabel->SetItalic( m_italic->IsChecked() );

    m_currentLabel->SetTextColor( m_textColorSwatch->GetSwatchColor() );

    TEXT_SPIN_STYLE selectedSpinStyle= TEXT_SPIN_STYLE::LEFT;

    if( m_spin0->IsChecked() )      selectedSpinStyle = TEXT_SPIN_STYLE::RIGHT;
    else if( m_spin1->IsChecked() ) selectedSpinStyle = TEXT_SPIN_STYLE::LEFT;
    else if( m_spin2->IsChecked() ) selectedSpinStyle = TEXT_SPIN_STYLE::UP;
    else if( m_spin3->IsChecked() ) selectedSpinStyle = TEXT_SPIN_STYLE::BOTTOM;

    if( m_currentLabel->AutoRotateOnPlacementSupported() )
    {
        SCH_EDIT_FRAME* frame = static_cast<SCH_EDIT_FRAME*>( m_parentFrame );
        m_currentLabel->SetAutoRotateOnPlacement( m_autoRotate->IsChecked() );
        frame->AutoRotateItem( frame->GetScreen(), m_currentLabel );
    }
    else
    {
        m_currentLabel->SetAutoRotateOnPlacement( false );
    }

    if( !m_currentLabel->AutoRotateOnPlacement()
        && m_currentLabel->GetTextSpinStyle() != selectedSpinStyle )
    {
        m_currentLabel->SetTextSpinStyle( selectedSpinStyle );
    }

    if( doAutoplace )
        m_currentLabel->AutoAutoplaceFields( m_Parent->GetScreen() );

    if( !commit.Empty() )
        commit.Push( _( "Edit Label" ), SKIP_CONNECTIVITY );

    return true;
}


void DIALOG_LABEL_PROPERTIES::onSpinButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_spin0, m_spin1, m_spin2, m_spin3 } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_LABEL_PROPERTIES::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = SCH_LABEL_BASE::ShowSyntaxHelp( this );
}


void DIALOG_LABEL_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int      fieldID = (int) m_fields->size();
    wxString fieldName;

    if( (int) fieldID == m_currentLabel->GetMandatoryFieldCount()
            || m_fields->at( m_fields->size()-1 ).GetCanonicalName() == wxT( "Netclass" ) )
    {
        fieldName = wxT( "Netclass" );
    }
    else
    {
        fieldName = SCH_LABEL_BASE::GetDefaultFieldName( fieldName, true );
    }

    SCH_FIELD newField( VECTOR2I( 0, 0 ), fieldID, m_currentLabel, fieldName );

    if( m_fields->size() > 0 )
    {
        newField.SetVisible( m_fields->at( m_fields->size() - 1 ).IsVisible() );
        newField.SetTextAngle( m_fields->at( m_fields->size() - 1 ).GetTextAngle() );
        newField.SetItalic( m_fields->at( m_fields->size() - 1 ).IsItalic() );
        newField.SetBold( m_fields->at( m_fields->size() - 1 ).IsBold() );
    }
    else
    {
        newField.SetVisible( true );
        newField.SetItalic( true );
    }

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_grid->MakeCellVisible( (int) m_fields->size() - 1, 0 );
    m_grid->SetGridCursor( (int) m_fields->size() - 1, 0 );

    m_grid->EnableCellEditControl();
    m_grid->ShowCellEditControl();
}


void DIALOG_LABEL_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    wxArrayInt selectedRows = m_grid->GetSelectedRows();

    if( selectedRows.empty() && m_grid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_grid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    for( int row : selectedRows )
    {
        if( row < m_currentLabel->GetMandatoryFieldCount() )
        {
            DisplayError( this, _( "The first field is mandatory." ) );
            return;
        }
    }

    m_grid->CommitPendingChanges( true /* quiet mode */ );

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort( []( int* first, int* second ) { return *second - *first; } );

    for( int row : selectedRows )
    {
        m_fields->erase( m_fields->begin() + row );

        // notify the grid
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
        m_grid->ProcessTableMessage( msg );

        if( m_grid->GetNumberRows() > 0 )
        {
            m_grid->MakeCellVisible( std::max( 0, row-1 ), m_grid->GetGridCursorCol() );
            m_grid->SetGridCursor( std::max( 0, row-1 ), m_grid->GetGridCursorCol() );
        }
    }
}


void DIALOG_LABEL_PROPERTIES::OnMoveUp( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int i = m_grid->GetGridCursorRow();

    if( i > m_currentLabel->GetMandatoryFieldCount() )
    {
        SCH_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i - 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i - 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void DIALOG_LABEL_PROPERTIES::OnMoveDown( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int i = m_grid->GetGridCursorRow();

    if( i >= m_currentLabel->GetMandatoryFieldCount() && i < m_grid->GetNumberRows() - 1 )
    {
        SCH_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i + 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i + 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void DIALOG_LABEL_PROPERTIES::AdjustGridColumns( int aWidth )
{
    m_width = aWidth;
    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    m_grid->AutoSizeColumn( 0 );
    m_grid->SetColSize( 0, std::max( 72, m_grid->GetColSize( 0 ) ) );

    int fixedColsWidth = m_grid->GetColSize( 0 );

    for( int i = 2; i < m_grid->GetNumberCols(); i++ )
        fixedColsWidth += m_grid->GetColSize( i );

    m_grid->SetColSize( 1, std::max( 120, aWidth - fixedColsWidth ) );
}


void DIALOG_LABEL_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    std::bitset<64> shownColumns = m_grid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_grid->IsCellEditControlShown() )
            AdjustGridColumns( m_grid->GetRect().GetWidth() );
    }

    // Handle a delayed focus
    if( m_delayedFocusRow >= 0 && m_delayedFocusRow < m_grid->GetNumberRows() )
    {
        m_grid->SetFocus();
        m_grid->MakeCellVisible( m_delayedFocusRow, m_delayedFocusColumn );
        m_grid->SetGridCursor( m_delayedFocusRow, m_delayedFocusColumn );
    }

    m_delayedFocusRow = -1;
    m_delayedFocusColumn = -1;
}


void DIALOG_LABEL_PROPERTIES::OnSizeGrid( wxSizeEvent& event )
{
    int new_size = event.GetSize().GetX();

    if( m_width != new_size )
        AdjustGridColumns( new_size );

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


