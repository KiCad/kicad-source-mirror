/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2004-2023 KiCad Developers, see AITHORS.txt for contributors.
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
#include <widgets/color_swatch.h>
#include <settings/color_settings.h>
#include <bitmaps.h>
#include <kiway.h>
#include <confirm.h>
#include <common.h>
#include <string_utils.h>
#include <sch_edit_frame.h>
#include <ee_collectors.h>
#include <sch_symbol.h>
#include <sch_label.h>
#include <lib_field.h>
#include <template_fieldnames.h>
#include <symbol_library.h>
#include <sch_validators.h>
#include <schematic.h>
#include <sch_commit.h>
#include <dialog_field_properties.h>
#include <sch_text.h>
#include <scintilla_tricks.h>
#include <wildcards_and_files_ext.h>
#include <sim/sim_model.h>
#include <sim/sim_lib_mgr.h>


DIALOG_FIELD_PROPERTIES::DIALOG_FIELD_PROPERTIES( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                                                  const EDA_TEXT* aTextItem ) :
    DIALOG_FIELD_PROPERTIES_BASE( aParent, wxID_ANY, aTitle ),
    m_posX( aParent, m_xPosLabel, m_xPosCtrl, m_xPosUnits, true ),
    m_posY( aParent, m_yPosLabel, m_yPosCtrl, m_yPosUnits, true ),
    m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true ),
    m_font( nullptr ),
    m_firstFocus( true ),
    m_scintillaTricks( nullptr )
{
    COLOR_SETTINGS* colorSettings = aParent->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    wxASSERT( aTextItem );

    m_note->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_note->Show( false );

    // The field ID is initialized in the derived object's ctor.
    m_fieldId = VALUE_FIELD;

    m_scintillaTricks = new SCINTILLA_TRICKS( m_StyledTextCtrl, wxT( "{}" ), true,
            [this]()
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );
    m_StyledTextCtrl->SetEOLMode( wxSTC_EOL_LF );   // Normalize EOL across platforms

    m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_textColorSwatch->SetSwatchBackground( schematicBackground );

    m_separator1->SetIsSeparator();

    m_horizontal->SetIsRadioButton();
    m_horizontal->SetBitmap( KiBitmap( BITMAPS::text_horizontal ) );
    m_vertical->SetIsRadioButton();
    m_vertical->SetBitmap( KiBitmap( BITMAPS::text_vertical ) );

    m_separator2->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmap( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmap( BITMAPS::text_italic ) );

    m_separator3->SetIsSeparator();

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmap( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmap( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmap( BITMAPS::text_align_right ) );

    m_separator4->SetIsSeparator();

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmap( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmap( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmap( BITMAPS::text_valign_bottom ) );

    m_separator5->SetIsSeparator();

    m_horizontal->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onOrientButton, this );
    m_vertical->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onOrientButton, this );

    m_hAlignLeft->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onHAlignButton, this );
    m_hAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onHAlignButton, this );
    m_hAlignRight->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onHAlignButton, this );

    m_vAlignTop->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onVAlignButton, this );
    m_vAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onVAlignButton, this );
    m_vAlignBottom->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onVAlignButton, this );

    m_text = aTextItem->GetText();
    m_isItalic = aTextItem->IsItalic();
    m_isBold = aTextItem->IsBold();
    m_color = aTextItem->GetTextColor();
    m_position = aTextItem->GetTextPos();
    m_size = aTextItem->GetTextWidth();
    m_isVertical = aTextItem->GetTextAngle().IsVertical();
    m_verticalJustification = aTextItem->GetVertJustify();
    m_horizontalJustification = aTextItem->GetHorizJustify();
    m_isVisible = aTextItem->IsVisible();

    // These should be initialized in the child classes implementing dialogs for lib and sch items.
    m_isNameVisible  = false;
    m_allowAutoplace = true;
}


DIALOG_FIELD_PROPERTIES::~DIALOG_FIELD_PROPERTIES()
{
    delete m_scintillaTricks;
}


void DIALOG_FIELD_PROPERTIES::init()
{
    // Disable options for graphic text editing which are not needed for fields.
    m_CommonConvert->Show( false );
    m_CommonUnit->Show( false );

    // Predefined fields cannot contain some chars, or cannot be empty,
    // and need a SCH_FIELD_VALIDATOR (m_StyledTextCtrl cannot use a SCH_FIELD_VALIDATOR).
    bool use_validator = m_fieldId == REFERENCE_FIELD
                         || m_fieldId == FOOTPRINT_FIELD
                         || m_fieldId == DATASHEET_FIELD
                         || m_fieldId == SHEETNAME_V
                         || m_fieldId == SHEETFILENAME_V;

    if( use_validator )
    {
        m_TextCtrl->SetValidator( FIELD_VALIDATOR( m_fieldId, &m_text ) );
        SetInitialFocus( m_TextCtrl );

        m_StyledTextCtrl->Show( false );
    }
    else
    {
        SetInitialFocus( m_StyledTextCtrl );

        m_TextCtrl->Show( false );
    }

    // Show the footprint selection dialog if this is the footprint field.
    m_TextValueSelectButton->SetBitmap( KiBitmap( BITMAPS::small_library ) );
    m_TextValueSelectButton->Show( m_fieldId == FOOTPRINT_FIELD );

    m_TextCtrl->Enable( true );

    GetSizer()->SetSizeHints( this );

    // Adjust the height of the scintilla text editor after the first layout
    // To show only one line
    // (multiline text are is supported in fields and will be removed)
    if( m_StyledTextCtrl->IsShown() )
    {
        wxSize maxSize = m_StyledTextCtrl->GetSize();
        maxSize.x = -1;     // Do not fix the max width
        maxSize.y = m_xPosCtrl->GetSize().y;
        m_StyledTextCtrl->SetMaxSize( maxSize );
        m_StyledTextCtrl->SetUseVerticalScrollBar( false );
        m_StyledTextCtrl->SetUseHorizontalScrollBar( false );
    }

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_FIELD_PROPERTIES::OnTextValueSelectButtonClick( wxCommandEvent& aEvent )
{
    // pick a footprint using the footprint picker.
    wxString fpid;

    if( m_StyledTextCtrl->IsShown() )
        fpid = m_StyledTextCtrl->GetValue();
    else
        fpid = m_TextCtrl->GetValue();

    if( KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, true ) )
    {
        if( frame->ShowModal( &fpid, this ) )
        {
            if( m_StyledTextCtrl->IsShown() )
                m_StyledTextCtrl->SetValue( fpid );
            else
                m_TextCtrl->SetValue( fpid );
        }

        frame->Destroy();
    }
}


void DIALOG_FIELD_PROPERTIES::OnSetFocusText( wxFocusEvent& event )
{
    if( m_firstFocus )
    {
#ifdef __WXGTK__
        // Force an update of the text control before setting the text selection
        // This is needed because GTK seems to ignore the selection on first update
        //
        // Note that we can't do this on OSX as it tends to provoke Apple's
        // "[NSAlert runModal] may not be invoked inside of transaction begin/commit pair"
        // bug.  See: https://bugs.launchpad.net/kicad/+bug/1837225
        if( m_fieldId == REFERENCE_FIELD || m_fieldId == VALUE_FIELD || m_fieldId == SHEETNAME_V )
            m_TextCtrl->Update();
#endif

        if( m_fieldId == REFERENCE_FIELD )
            KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_TextCtrl ) );
        else if( m_fieldId == VALUE_FIELD || m_fieldId == SHEETNAME_V )
            m_TextCtrl->SetSelection( -1, -1 );

        m_firstFocus = false;
    }

    event.Skip();
}


void DIALOG_FIELD_PROPERTIES::onOrientButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_horizontal, m_vertical } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_FIELD_PROPERTIES::onHAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_hAlignLeft, m_hAlignCenter, m_hAlignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_FIELD_PROPERTIES::onVAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignCenter, m_vAlignBottom } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


bool DIALOG_FIELD_PROPERTIES::TransferDataToWindow()
{
    if( m_TextCtrl->IsShown() )
    {
        m_TextCtrl->SetValue( m_text );
    }
    else if( m_StyledTextCtrl->IsShown() )
    {
        m_StyledTextCtrl->SetValue( m_text );
        m_StyledTextCtrl->EmptyUndoBuffer();
    }

    m_fontCtrl->SetFontSelection( m_font );

    m_posX.SetValue( m_position.x );
    m_posY.SetValue( m_position.y );
    m_textSize.SetValue( m_size );

    m_horizontal->Check( !m_isVertical );
    m_vertical->Check( m_isVertical );

    m_italic->Check( m_isItalic );
    m_bold->Check( m_isBold );

    m_textColorSwatch->SetSwatchColor( m_color, false );

    switch ( m_horizontalJustification )
    {
    case GR_TEXT_H_ALIGN_LEFT:   m_hAlignLeft->Check( true );   break;
    case GR_TEXT_H_ALIGN_CENTER: m_hAlignCenter->Check( true ); break;
    case GR_TEXT_H_ALIGN_RIGHT:  m_hAlignRight->Check( true );  break;
    }

    switch ( m_verticalJustification )
    {
    case GR_TEXT_V_ALIGN_TOP:    m_vAlignTop->Check( true );    break;
    case GR_TEXT_V_ALIGN_CENTER: m_vAlignCenter->Check( true ); break;
    case GR_TEXT_V_ALIGN_BOTTOM: m_vAlignBottom->Check( true ); break;
    }

    m_visible->SetValue( m_isVisible );
    m_nameVisible->SetValue( m_isNameVisible );
    m_cbAllowAutoPlace->SetValue( m_allowAutoplace );

    return true;
}


bool DIALOG_FIELD_PROPERTIES::TransferDataFromWindow()
{
    if( m_TextCtrl->IsShown() )
        m_text = m_TextCtrl->GetValue();
    else if( m_StyledTextCtrl->IsShown() )
        m_text = m_StyledTextCtrl->GetValue();

    if( m_fieldId == REFERENCE_FIELD )
    {
        // Test if the reference string is valid:
        if( !SCH_SYMBOL::IsReferenceStringValid( m_text ) )
        {
            DisplayError( this, _( "Illegal reference designator value!" ) );
            return false;
        }
    }
    else if( m_fieldId == SHEETFILENAME_V )
    {
        m_text = EnsureFileExtension( m_text, KiCadSchematicFileExtension );
    }

    m_position = VECTOR2I( m_posX.GetValue(), m_posY.GetValue() );
    m_size = m_textSize.GetValue();

    if( m_fontCtrl->HaveFontSelection() )
        m_font = m_fontCtrl->GetFontSelection( m_bold->IsChecked(), m_italic->IsChecked() );

    m_isVertical = m_vertical->IsChecked();

    m_isBold = m_bold->IsChecked();
    m_isItalic = m_italic->IsChecked();
    m_color = m_textColorSwatch->GetSwatchColor();

    if( m_hAlignLeft->IsChecked() )
        m_horizontalJustification = GR_TEXT_H_ALIGN_LEFT;
    else if( m_hAlignCenter->IsChecked() )
        m_horizontalJustification = GR_TEXT_H_ALIGN_CENTER;
    else
        m_horizontalJustification = GR_TEXT_H_ALIGN_RIGHT;

    if( m_vAlignTop->IsChecked() )
        m_verticalJustification = GR_TEXT_V_ALIGN_TOP;
    else if( m_vAlignCenter->IsChecked() )
        m_verticalJustification = GR_TEXT_V_ALIGN_CENTER;
    else
        m_verticalJustification = GR_TEXT_V_ALIGN_BOTTOM;

    m_isVisible      = m_visible->GetValue();
    m_isNameVisible  = m_nameVisible->GetValue();
    m_allowAutoplace = m_cbAllowAutoPlace->GetValue();

    return true;
}


void DIALOG_FIELD_PROPERTIES::updateText( EDA_TEXT* aText )
{
    if( aText->GetTextWidth() != m_size )
        aText->SetTextSize( VECTOR2I( m_size, m_size ) );

    aText->SetFont( m_font );
    aText->SetVisible( m_isVisible );
    aText->SetTextAngle( m_isVertical ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
    aText->SetItalic( m_isItalic );
    aText->SetBold( m_isBold );
    aText->SetTextColor( m_color );
}


DIALOG_LIB_FIELD_PROPERTIES::DIALOG_LIB_FIELD_PROPERTIES( SCH_BASE_FRAME* aParent,
                                                          const wxString& aTitle,
                                                          const LIB_FIELD* aField ) :
        DIALOG_FIELD_PROPERTIES( aParent, aTitle, aField )
{
    m_fieldId        = aField->GetId();
    m_isNameVisible  = aField->IsNameShown();
    m_allowAutoplace = aField->CanAutoplace();

    if( m_fieldId == VALUE_FIELD )
        m_text = UnescapeString( aField->GetText() );

    m_font = aField->GetFont();

    m_nameVisible->Show();
    m_cbAllowAutoPlace->Show();

    init();
}


void DIALOG_LIB_FIELD_PROPERTIES::UpdateField( LIB_FIELD* aField )
{
    aField->SetText( m_text );

    updateText( aField );

    aField->SetNameShown( m_isNameVisible );
    aField->SetCanAutoplace( m_allowAutoplace );

    aField->SetHorizJustify( EDA_TEXT::MapHorizJustify( m_horizontalJustification ) );
    aField->SetVertJustify( EDA_TEXT::MapVertJustify( m_verticalJustification  ) );
    aField->SetTextPos( m_position );
}


DIALOG_SCH_FIELD_PROPERTIES::DIALOG_SCH_FIELD_PROPERTIES( SCH_BASE_FRAME* aParent,
                                                          const wxString& aTitle,
                                                          const SCH_FIELD* aField ) :
        DIALOG_FIELD_PROPERTIES( aParent, aTitle, aField ),
        m_field( aField )
{
    m_isSheetFilename = false;

    if( aField->GetParent() && aField->GetParent()->Type() == SCH_SYMBOL_T )
    {
        m_fieldId = aField->GetId();
    }
    else if( aField->GetParent() && aField->GetParent()->Type() == SCH_SHEET_T )
    {
        switch( aField->GetId() )
        {
        case SHEETNAME:
            m_fieldId = SHEETNAME_V;
            break;

        case SHEETFILENAME:
            m_isSheetFilename = true;
            m_fieldId = SHEETFILENAME_V;
            m_note->SetLabel( wxString::Format( m_note->GetLabel(),
                              _( "Sheet filename can only be modified in Sheet Properties dialog." ) ) );
            m_note->Show( true );
            break;

        default:
            m_fieldId = SHEETUSERFIELD_V;
            break;
        }
    }
    else if( aField->GetParent() && aField->GetParent()->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
    {
        m_fieldId = LABELUSERFIELD_V;
    }

    // show text variable cross-references in a human-readable format
    m_text = aField->Schematic()->ConvertKIIDsToRefs( aField->GetText() );

    m_font = m_field->GetFont();

    m_textLabel->SetLabel( aField->GetName() + wxS( ":" ) );

    m_position = m_field->GetPosition();

    m_isNameVisible = m_field->IsNameShown();
    m_allowAutoplace = m_field->CanAutoplace();

    m_horizontalJustification = m_field->GetEffectiveHorizJustify();
    m_verticalJustification = m_field->GetEffectiveVertJustify();

    m_StyledTextCtrl->Bind( wxEVT_STC_CHARADDED, &DIALOG_SCH_FIELD_PROPERTIES::onScintillaCharAdded,
                            this );
    m_StyledTextCtrl->Bind( wxEVT_STC_AUTOCOMP_CHAR_DELETED,
                            &DIALOG_SCH_FIELD_PROPERTIES::onScintillaCharAdded, this );

    m_nameVisible->Show();
    m_cbAllowAutoPlace->Show();

    init();

    if( m_isSheetFilename || m_field->IsNamedVariable() )
    {
        m_StyledTextCtrl->Enable( false );
        m_TextCtrl->Enable( false );
    }
}


void DIALOG_SCH_FIELD_PROPERTIES::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    int key = aEvent.GetKey();

    SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( GetParent() );
    wxArrayString   autocompleteTokens;
    int             pos = m_StyledTextCtrl->GetCurrentPos();
    int             start = m_StyledTextCtrl->WordStartPosition( pos, true );
    wxString        partial;

    // Currently, '\n' is not allowed in fields. So remove it when entered
    // TODO: see if we must close the dialog. However this is not obvious, as
    // if a \n is typed (and removed) when a text is selected, this text is deleted
    // (in fact replaced by \n, that is removed by the filter)
    if( key == '\n' )
    {
        wxString text = m_StyledTextCtrl->GetText();
        int currpos = m_StyledTextCtrl->GetCurrentPos();
        text.Replace( wxS( "\n" ), wxS( "" ) );
        m_StyledTextCtrl->SetText( text );
        m_StyledTextCtrl->GotoPos( currpos-1 );
        return;
    }

    auto textVarRef =
            [&]( int pt )
            {
                return pt >= 2
                        && m_StyledTextCtrl->GetCharAt( pt - 2 ) == '$'
                        && m_StyledTextCtrl->GetCharAt( pt - 1 ) == '{';
            };

    // Check for cross-reference
    if( start > 1 && m_StyledTextCtrl->GetCharAt( start - 1 ) == ':' )
    {
        int refStart = m_StyledTextCtrl->WordStartPosition( start - 1, true );

        if( textVarRef( refStart ) )
        {
            partial = m_StyledTextCtrl->GetRange( start, pos );

            wxString ref = m_StyledTextCtrl->GetRange( refStart, start - 1 );

            if( ref == wxS( "OP" ) )
            {
                // SPICE operating points use ':' syntax for ports
                SCH_SYMBOL*     symbol = dynamic_cast<SCH_SYMBOL*>( m_field->GetParent() );
                SCH_SHEET_PATH& sheet = editFrame->Schematic().CurrentSheet();

                if( symbol )
                {
                    SIM_LIB_MGR mgr( &Prj() );
                    SIM_MODEL&  model = mgr.CreateModel( &sheet, *symbol ).model;

                    for( wxString pin : model.GetPinNames() )
                    {
                        if( pin.StartsWith( '<' ) && pin.EndsWith( '>' ) )
                            autocompleteTokens.push_back( pin.Mid( 1, pin.Length() - 2 ) );
                        else
                            autocompleteTokens.push_back( pin );
                    }
                }
            }
            else
            {
                SCH_SHEET_LIST     sheets = editFrame->Schematic().GetSheets();
                SCH_REFERENCE_LIST refs;
                SCH_SYMBOL*        refSymbol = nullptr;

                sheets.GetSymbols( refs );

                for( size_t jj = 0; jj < refs.GetCount(); jj++ )
                {
                    if( refs[ jj ].GetSymbol()->GetRef( &refs[ jj ].GetSheetPath(), true ) == ref )
                    {
                        refSymbol = refs[ jj ].GetSymbol();
                        break;
                    }
                }

                if( refSymbol )
                    refSymbol->GetContextualTextVars( &autocompleteTokens );
            }
        }
    }
    else if( textVarRef( start ) )
    {
        partial = m_StyledTextCtrl->GetTextRange( start, pos );

        SCH_SYMBOL*     symbol = dynamic_cast<SCH_SYMBOL*>( m_field->GetParent() );
        SCH_SHEET*      sheet = dynamic_cast<SCH_SHEET*>( m_field->GetParent() );
        SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( m_field->GetParent() );

        if( symbol )
        {
            symbol->GetContextualTextVars( &autocompleteTokens );

            SCHEMATIC* schematic = symbol->Schematic();

            if( schematic && schematic->CurrentSheet().Last() )
                schematic->CurrentSheet().Last()->GetContextualTextVars( &autocompleteTokens );
        }

        if( sheet )
            sheet->GetContextualTextVars( &autocompleteTokens );

        if( label )
            label->GetContextualTextVars( &autocompleteTokens );

        for( std::pair<wxString, wxString> entry : Prj().GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    m_scintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    m_StyledTextCtrl->SetFocus();
}


void DIALOG_SCH_FIELD_PROPERTIES::UpdateField( SCH_COMMIT* aCommit, SCH_FIELD* aField,
                                               SCH_SHEET_PATH* aSheetPath )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );
    SCH_ITEM*       parent = dynamic_cast<SCH_ITEM*>( aField->GetParent() );
    int             fieldType = aField->GetId();

    if( parent && parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( parent );

        if( fieldType == REFERENCE_FIELD )
            symbol->SetRef( aSheetPath, m_text );
        else if( fieldType == VALUE_FIELD )
            symbol->SetValueFieldText( m_text );
        else if( fieldType == FOOTPRINT_FIELD )
            symbol->SetFootprintFieldText( m_text );
    }
    else if( parent && parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        if( aField->GetCanonicalName() == wxT( "Intersheetrefs" ) )
        {
            if( m_visible->GetValue() != parent->Schematic()->Settings().m_IntersheetRefsShow )
            {
                DisplayInfoMessage( this, _( "Intersheet reference visibility is "
                                             "controlled globally from "
                                             "Schematic Setup > General > Formatting" ) );
            }
        }
    }

    bool positioningModified = false;

    if( aField->GetPosition() != m_position )
        positioningModified = true;

    if( aField->GetTextAngle().IsVertical() != m_isVertical )
        positioningModified = true;

    if( aField->GetEffectiveHorizJustify() != m_horizontalJustification )
        positioningModified = true;

    if( aField->GetEffectiveVertJustify() != m_verticalJustification )
        positioningModified = true;

    // convert any text variable cross-references to their UUIDs
    m_text = aField->Schematic()->ConvertRefsToKIIDs( m_text );

    aField->SetText( m_text );
    updateText( aField );
    aField->SetPosition( m_position );

    aField->SetNameShown( m_isNameVisible );
    aField->SetCanAutoplace( m_allowAutoplace );

    // Note that we must set justifications before we can ask if they're flipped.  If the old
    // justification is center then it won't know (whereas if the new justification is center
    // the we don't care).
    aField->SetHorizJustify( m_horizontalJustification );
    aField->SetVertJustify( m_verticalJustification );

    if( aField->IsHorizJustifyFlipped() )
        aField->SetHorizJustify( EDA_TEXT::MapHorizJustify( -m_horizontalJustification ) );

    if( aField->IsVertJustifyFlipped() )
        aField->SetVertJustify( EDA_TEXT::MapVertJustify( -m_verticalJustification ) );

    // The value, footprint and datasheet fields should be kept in sync in multi-unit parts.
    // Of course the symbol must be annotated to collect other units.
    if( editFrame && parent && parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( parent );

        if( symbol->IsAnnotated( aSheetPath ) && ( fieldType == VALUE_FIELD
                                                || fieldType == FOOTPRINT_FIELD
                                                || fieldType == DATASHEET_FIELD ) )
        {
            wxString ref = symbol->GetRef( aSheetPath );
            int      unit = symbol->GetUnit();
            LIB_ID   libId = symbol->GetLibId();

            for( SCH_SHEET_PATH& sheet : editFrame->Schematic().GetSheets() )
            {
                SCH_SCREEN*              screen = sheet.LastScreen();
                std::vector<SCH_SYMBOL*> otherUnits;

                CollectOtherUnits( ref, unit, libId, sheet, &otherUnits );

                for( SCH_SYMBOL* otherUnit : otherUnits )
                {
                    aCommit->Modify( otherUnit, screen );

                    if( fieldType == VALUE_FIELD )
                        otherUnit->SetValueFieldText( m_text );
                    else if( fieldType == FOOTPRINT_FIELD )
                        otherUnit->SetFootprintFieldText( m_text );
                    else
                        otherUnit->GetField( DATASHEET_FIELD )->SetText( m_text );

                    editFrame->UpdateItem( otherUnit, false, true );
                }
            }
        }
    }

    if( positioningModified && parent )
        parent->ClearFieldsAutoplaced();
}
