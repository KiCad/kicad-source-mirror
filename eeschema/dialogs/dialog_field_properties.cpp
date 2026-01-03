/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright The KiCad Developers, see AITHORS.txt for contributors.
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
#include <widgets/std_bitmap_button.h>
#include <widgets/font_choice.h>
#include <widgets/color_swatch.h>
#include <settings/color_settings.h>
#include <bitmaps.h>
#include <kiway.h>
#include <kiway_express.h>
#include <confirm.h>
#include <common.h>
#include <string_utils.h>
#include <sch_edit_frame.h>
#include <sch_collectors.h>
#include <sch_symbol.h>
#include <template_fieldnames.h>
#include <sch_validators.h>
#include <schematic.h>
#include <sch_commit.h>
#include <dialog_field_properties.h>
#include <sch_text.h>
#include <scintilla_tricks.h>
#include <wildcards_and_files_ext.h>


DIALOG_FIELD_PROPERTIES::DIALOG_FIELD_PROPERTIES( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                                                  const SCH_FIELD* aField ) :
    DIALOG_FIELD_PROPERTIES_BASE( aParent, wxID_ANY, aTitle ),
    m_posX( aParent, m_xPosLabel, m_xPosCtrl, m_xPosUnits, true ),
    m_posY( aParent, m_yPosLabel, m_yPosCtrl, m_yPosUnits, true ),
    m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true ),
    m_font( nullptr ),
    m_firstFocus( true ),
    m_scintillaTricks( nullptr ),
    m_field( aField )
{
    COLOR_SETTINGS* colorSettings = aParent->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    wxASSERT( m_field );

    m_note->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_note->Show( false );

    m_scintillaTricks = new SCINTILLA_TRICKS( m_StyledTextCtrl, wxT( "{}" ), true,
            // onAcceptFn
            [this]( wxKeyEvent& aEvent )
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );

    m_StyledTextCtrl->SetEOLMode( wxSTC_EOL_LF );   // Normalize EOL across platforms

#ifdef __WXGTK__
    m_StyledTextCtrl->SetExtraAscent( 6 );
    m_StyledTextCtrl->SetExtraDescent( 6 );
#elif defined ( __WXMSW__ )
    // Do nothing: SetExtraAscent() + SetExtraDescent(), when set to a value not 0
    // Generate a strange bug: the text is not always shown (Perhaps a wxMSW bug)
    // and this call is not needed on WXMSW
#else
    m_StyledTextCtrl->SetExtraAscent( 1 );
    m_StyledTextCtrl->SetExtraDescent( 2 );
#endif

#ifdef _WIN32
    // Without this setting, on Windows, some esoteric unicode chars create display issue
    // in a wxStyledTextCtrl.
    // for SetTechnology() info, see https://www.scintilla.org/ScintillaDoc.html#SCI_SETTECHNOLOGY
    m_StyledTextCtrl->SetTechnology(wxSTC_TECHNOLOGY_DIRECTWRITE);
#endif

    m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_textColorSwatch->SetSwatchBackground( schematicBackground );

    m_separator1->SetIsSeparator();

    m_horizontal->SetIsRadioButton();
    m_horizontal->SetBitmap( KiBitmapBundle( BITMAPS::text_horizontal ) );
    m_vertical->SetIsRadioButton();
    m_vertical->SetBitmap( KiBitmapBundle( BITMAPS::text_vertical ) );

    m_separator2->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmapBundle( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmapBundle( BITMAPS::text_italic ) );

    m_separator3->SetIsSeparator();

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmapBundle( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmapBundle( BITMAPS::text_align_right ) );

    m_separator4->SetIsSeparator();

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_bottom ) );

    m_separator5->SetIsSeparator();

    m_horizontal->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onOrientButton, this );
    m_vertical->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onOrientButton, this );

    m_hAlignLeft->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onHAlignButton, this );
    m_hAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onHAlignButton, this );
    m_hAlignRight->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onHAlignButton, this );

    m_vAlignTop->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onVAlignButton, this );
    m_vAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onVAlignButton, this );
    m_vAlignBottom->Bind( wxEVT_BUTTON, &DIALOG_FIELD_PROPERTIES::onVAlignButton, this );

    // show text variable cross-references in a human-readable format
    if( aField->Schematic() )
    {
        const SCH_SHEET_PATH& sheetPath = aField->Schematic()->CurrentSheet();
        wxString variant = aField->Schematic()->GetCurrentVariant();

        m_text = aField->Schematic()->ConvertKIIDsToRefs( aField->GetText( &sheetPath, variant ) );
    }
    else
    {
        m_text = aField->GetText();
    }

    m_font = m_field->GetFont();
    m_isItalic = aField->IsItalic();
    m_isBold = aField->IsBold();
    m_color = aField->GetTextColor();
    m_position = aField->GetTextPos();
    m_size = aField->GetTextWidth();
    m_isVertical = aField->GetTextAngle().IsVertical();
    m_verticalJustification = aField->GetVertJustify();
    m_horizontalJustification = aField->GetHorizJustify();
    m_isVisible = aField->IsVisible();

    m_isSheetFilename = false;
    m_fieldId = aField->GetId();

    if( aField->GetParent() && aField->GetParent()->Type() == LIB_SYMBOL_T )
    {
        const LIB_SYMBOL* symbol = static_cast<const LIB_SYMBOL*>( aField->GetParentSymbol() );

        /*
         * Symbol netlist format:
         *   pinNumber pinName <tab> pinNumber pinName...
         *   fpFilter fpFilter...
         */
        wxString      netlist;
        wxArrayString pins;

        for( SCH_PIN* pin : symbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ) )
        {
            bool valid = false;
            auto expanded = pin->GetStackedPinNumbers( &valid );

            if( valid && !expanded.empty() )
            {
                for( const wxString& num : expanded )
                    pins.push_back( num + ' ' + pin->GetShownName() );
            }
            else
            {
                pins.push_back( pin->GetNumber() + ' ' + pin->GetShownName() );
            }
        }

        if( !pins.IsEmpty() )
        {
            wxString dbg = wxJoin( pins, '\t' );
            wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "Chooser payload pins (LIB_SYMBOL): %s" ), dbg );
            netlist << EscapeString( dbg, CTX_LINE );
        }

        netlist << wxS( "\r" );

        wxArrayString fpFilters = symbol->GetFPFilters();

        if( !fpFilters.IsEmpty() )
            netlist << EscapeString( wxJoin( fpFilters, ' ' ), CTX_LINE );

        netlist << wxS( "\r" );

        m_netlist = netlist;
    }
    else if( aField->GetParent() && aField->GetParent()->Type() == SCH_SYMBOL_T )
    {
        const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( aField->GetParentSymbol() );
        SCH_SHEET_PATH    sheetPath = static_cast<SCH_EDIT_FRAME*>( aParent )->GetCurrentSheet();

        /*
         * Symbol netlist format:
         *   pinNumber pinName <tab> pinNumber pinName...
         *   fpFilter fpFilter...
         */
        wxString netlist;

        // We need the list of pins of the lib symbol, not just the pins of the current
        // sch symbol, that can be just an unit of a multi-unit symbol, to be able to
        // select/filter right footprints
        wxArrayString pins;

        const std::unique_ptr< LIB_SYMBOL >& lib_symbol = symbol->GetLibSymbolRef();

        if( lib_symbol )
        {
            for( SCH_PIN* pin : lib_symbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ) )
            {
                bool valid = false;
                auto expanded = pin->GetStackedPinNumbers( &valid );
                if( valid && !expanded.empty() )
                {
                    for( const wxString& num : expanded )
                        pins.push_back( num + ' ' + pin->GetShownName() );
                }
                else
                {
                    pins.push_back( pin->GetNumber() + ' ' + pin->GetShownName() );
                }
            }
        }

        if( !pins.IsEmpty() )
        {
            wxString dbg = wxJoin( pins, '\t' );
            wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "Chooser payload pins (SCH_SYMBOL): %s" ), dbg );
            netlist << EscapeString( dbg, CTX_LINE );
        }

        netlist << wxS( "\r" );

        wxArrayString fpFilters;

        if( symbol->GetLibSymbolRef() )     // can be null with very old schematic
            fpFilters = symbol->GetLibSymbolRef()->GetFPFilters();

        if( !fpFilters.IsEmpty() )
            netlist << EscapeString( wxJoin( fpFilters, ' ' ), CTX_LINE );

        netlist << wxS( "\r" );

        m_netlist = netlist;
    }

    if( aField->GetId() == FIELD_T::SHEET_FILENAME )
    {
        m_isSheetFilename = true;
        m_note->Show( true );
    }

    m_textLabel->SetLabel( aField->GetName() + wxS( ":" ) );

    m_position = m_field->GetPosition();

    m_isNameVisible = m_field->IsNameShown();
    m_allowAutoplace = m_field->CanAutoplace();

    m_horizontalJustification = m_field->GetEffectiveHorizJustify();
    m_verticalJustification = m_field->GetEffectiveVertJustify();

    m_StyledTextCtrl->Bind( wxEVT_STC_CHARADDED,
                            &DIALOG_FIELD_PROPERTIES::onScintillaCharAdded, this );
    m_StyledTextCtrl->Bind( wxEVT_STC_AUTOCOMP_CHAR_DELETED,
                            &DIALOG_FIELD_PROPERTIES::onScintillaCharAdded, this );

    m_nameVisible->Show();
    m_cbAllowAutoPlace->Show();

    init();

    if( m_isSheetFilename || m_field->IsGeneratedField() )
    {
        m_StyledTextCtrl->Enable( false );
        m_TextCtrl->Enable( false );
    }
}


DIALOG_FIELD_PROPERTIES::~DIALOG_FIELD_PROPERTIES()
{
    delete m_scintillaTricks;
    m_scintillaTricks = nullptr;
}


void DIALOG_FIELD_PROPERTIES::init()
{
    // Disable options for graphic text editing which are not needed for fields.
    m_commonToAllBodyStyles->Show( false );
    m_commonToAllUnits->Show( false );

    // Predefined fields cannot contain some chars and cannot be empty, so they need a
    // SCH_FIELD_VALIDATOR (m_StyledTextCtrl cannot use a SCH_FIELD_VALIDATOR).
    if( m_fieldId == FIELD_T::REFERENCE
            || m_fieldId == FIELD_T::FOOTPRINT
            || m_fieldId == FIELD_T::DATASHEET
            || m_fieldId == FIELD_T::SHEET_NAME
            || m_fieldId == FIELD_T::SHEET_FILENAME )
    {
        m_TextCtrl->SetValidator( FIELD_VALIDATOR( m_fieldId, &m_text ) );
        SetInitialFocus( m_TextCtrl );

        m_StyledTextCtrl->Show( false );
        m_StyledTextCtrlBorder->Show( false );
    }
    else
    {
        SetInitialFocus( m_StyledTextCtrl );

        m_TextCtrl->Show( false );
    }

    // Show the unit selector for reference fields on multi-unit schematic symbols
    bool showUnitSelector = m_fieldId == FIELD_T::REFERENCE
                            && m_field->GetParentSymbol()
                            && m_field->GetParentSymbol()->Type() == SCH_SYMBOL_T
                            && m_field->GetParentSymbol()->IsMultiUnit();

    m_unitLabel->Show( showUnitSelector );
    m_unitChoice->Show( showUnitSelector );

    // Show the footprint selection dialog if this is the footprint field.
    m_TextValueSelectButton->SetBitmap( KiBitmapBundle( BITMAPS::small_library ) );
    m_TextValueSelectButton->Show( m_fieldId == FIELD_T::FOOTPRINT );

    m_TextCtrl->Enable( true );

    GetSizer()->SetSizeHints( this );

    // Adjust the height of the scintilla editor after the first layout to show a single line
    // (multiline text is not supported in fields and will be removed)
    if( m_StyledTextCtrl->IsShown() )
    {
        wxSize maxSize;
        maxSize.x = -1;     // Do not fix the max width
        maxSize.y = m_StyledTextCtrl->TextHeight( 0 );
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

    if( KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, this ) )
    {
        KIWAY_EXPRESS event( FRAME_FOOTPRINT_CHOOSER, MAIL_SYMBOL_NETLIST, m_netlist );
        frame->KiwayMailIn( event );

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
        if( m_fieldId == FIELD_T::REFERENCE || m_fieldId == FIELD_T::VALUE || m_fieldId == FIELD_T::SHEET_NAME )
            m_TextCtrl->Update();
#endif

        if( m_fieldId == FIELD_T::REFERENCE )
            KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_TextCtrl ) );
        else if( m_fieldId == FIELD_T::VALUE || m_fieldId == FIELD_T::SHEET_NAME )
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
        m_TextCtrl->SetValue( EscapeString( m_text, CTX_LINE ) );
    }
    else if( m_StyledTextCtrl->IsShown() )
    {
        m_StyledTextCtrl->SetValue( EscapeString( m_text, CTX_LINE ) );
        m_StyledTextCtrl->EmptyUndoBuffer();
    }

    if( m_unitChoice->IsShown() )
    {
        const SYMBOL* parent = m_field->GetParentSymbol();

        // Control shouldn't be shown for non-schematic symbols
        wxCHECK( parent->Type() == SCH_SYMBOL_T, true );

        const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( parent );

        for( int ii = 1; ii <= symbol->GetUnitCount(); ii++ )
            m_unitChoice->Append( symbol->GetUnitDisplayName( ii, false ) );

        if( symbol->GetUnit() <= (int) m_unitChoice->GetCount() )
            m_unitChoice->SetSelection( symbol->GetUnit() - 1 );
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
    case GR_TEXT_H_ALIGN_LEFT:          m_hAlignLeft->Check( true );   break;
    case GR_TEXT_H_ALIGN_CENTER:        m_hAlignCenter->Check( true ); break;
    case GR_TEXT_H_ALIGN_RIGHT:         m_hAlignRight->Check( true );  break;
    case GR_TEXT_H_ALIGN_INDETERMINATE:                                break;
    }

    switch ( m_verticalJustification )
    {
    case GR_TEXT_V_ALIGN_TOP:           m_vAlignTop->Check( true );    break;
    case GR_TEXT_V_ALIGN_CENTER:        m_vAlignCenter->Check( true ); break;
    case GR_TEXT_V_ALIGN_BOTTOM:        m_vAlignBottom->Check( true ); break;
    case GR_TEXT_V_ALIGN_INDETERMINATE:                                break;
    }

    m_visible->SetValue( m_isVisible );
    m_nameVisible->SetValue( m_isNameVisible );
    m_cbAllowAutoPlace->SetValue( m_allowAutoplace );

    return true;
}


bool DIALOG_FIELD_PROPERTIES::TransferDataFromWindow()
{
    if( m_TextCtrl->IsShown() )
        m_text = UnescapeString( m_TextCtrl->GetValue() );
    else if( m_StyledTextCtrl->IsShown() )
        m_text = UnescapeString( m_StyledTextCtrl->GetValue() );

    if( m_fieldId == FIELD_T::SHEET_FILENAME )
        m_text = EnsureFileExtension( m_text, FILEEXT::KiCadSchematicFileExtension );

    m_position = VECTOR2I( m_posX.GetIntValue(), m_posY.GetIntValue() );
    m_size = m_textSize.GetIntValue();

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


void DIALOG_FIELD_PROPERTIES::updateText( SCH_FIELD* aField )
{
    if( aField->GetTextWidth() != m_size )
        aField->SetTextSize( VECTOR2I( m_size, m_size ) );

    aField->SetFont( m_font );
    aField->SetVisible( m_isVisible );
    aField->SetTextAngle( m_isVertical ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
    aField->SetItalic( m_isItalic );
    aField->SetBold( m_isBold );
    aField->SetTextColor( m_color );
}


void DIALOG_FIELD_PROPERTIES::UpdateField( SCH_FIELD* aField )
{
    if( aField->Schematic() )
    {
        const SCH_SHEET_PATH& sheetPath = aField->Schematic()->CurrentSheet();
        wxString variant = aField->Schematic()->GetCurrentVariant();

        aField->SetText( m_text, &sheetPath, variant );
    }
    else
    {
        aField->SetText( m_text );
    }

    updateText( aField );

    aField->SetNameShown( m_isNameVisible );
    aField->SetCanAutoplace( m_allowAutoplace );

    aField->SetHorizJustify( EDA_TEXT::MapHorizJustify( m_horizontalJustification ) );
    aField->SetVertJustify( EDA_TEXT::MapVertJustify( m_verticalJustification  ) );
    aField->SetTextPos( m_position );
}


void DIALOG_FIELD_PROPERTIES::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    m_field->OnScintillaCharAdded( m_scintillaTricks, aEvent );
}


void DIALOG_FIELD_PROPERTIES::UpdateField( SCH_COMMIT* aCommit, SCH_FIELD* aField,
                                           SCH_SHEET_PATH* aSheetPath )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );
    SCH_ITEM*       parent = dynamic_cast<SCH_ITEM*>( aField->GetParent() );
    bool            fieldTextSet = false;
    SCH_SHEET_PATH  sheetPath;
    wxString        variantName;

    // convert any text variable cross-references to their UUIDs
    m_text = aField->Schematic()->ConvertRefsToKIIDs( m_text );

    if( aField->Schematic() )
    {
        sheetPath = aField->Schematic()->CurrentSheet();
        variantName = aField->Schematic()->GetCurrentVariant();
    }

    if( parent && parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( parent );

        if( m_fieldId == FIELD_T::REFERENCE )
            symbol->SetRef( aSheetPath, m_text );
        else
            symbol->SetFieldText( aField->GetName(), m_text, &sheetPath, variantName );

        fieldTextSet = true;

        // Set the unit selection in multiple units per package
        if( m_unitChoice->IsShown() )
        {
            int unit_selection = m_unitChoice->IsEnabled() ? m_unitChoice->GetSelection() + 1 : 1;
            symbol->SetUnitSelection( aSheetPath, unit_selection );
            symbol->SetUnit( unit_selection );
        }
    }
    else if( parent && parent->Type() == SCH_SHEET_T )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( parent );

        if( !aField->IsMandatory() )
        {
            sheet->SetFieldText( aField->GetName(), m_text, &sheetPath, variantName );
            fieldTextSet = true;
        }
    }
    else if( parent && parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        if( m_fieldId == FIELD_T::INTERSHEET_REFS )
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

    // Changing a sheetname need to update the hierarchy navigator
    bool needUpdateHierNav = false;

    if( m_fieldId == FIELD_T::SHEET_NAME )
        needUpdateHierNav = m_text != aField->GetText();

    if( !fieldTextSet )
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

        if( symbol->IsAnnotated( aSheetPath ) && ( m_fieldId == FIELD_T::VALUE
                                                || m_fieldId == FIELD_T::FOOTPRINT
                                                || m_fieldId == FIELD_T::DATASHEET ) )
        {
            wxString ref = symbol->GetRef( aSheetPath );
            int      unit = symbol->GetUnit();
            LIB_ID   libId = symbol->GetLibId();

            for( SCH_SHEET_PATH& sheet : editFrame->Schematic().Hierarchy() )
            {
                SCH_SCREEN*              screen = sheet.LastScreen();
                std::vector<SCH_SYMBOL*> otherUnits;

                CollectOtherUnits( ref, unit, libId, sheet, &otherUnits );

                for( SCH_SYMBOL* otherUnit : otherUnits )
                {
                    aCommit->Modify( otherUnit, screen );
                    otherUnit->GetField( m_fieldId )->SetText( m_text );
                    editFrame->UpdateItem( otherUnit, false, true );
                }
            }
        }
    }

    if( positioningModified && parent )
        parent->SetFieldsAutoplaced( AUTOPLACE_NONE );

    // Update the hierarchy navigator labels if needed.
    if( editFrame && needUpdateHierNav )
        editFrame->UpdateLabelsHierarchyNavigator();
}
