/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <widgets/color_swatch.h>
#include <widgets/line_ending_bitmap.h>
#include <stroke_params.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <sch_shape.h>
#include <sch_rule_area.h>
#include <dialog_shape_properties.h>
#include <settings/color_settings.h>
#include <widgets/wx_infobar.h>
#include <symbol_editor_settings.h>
#include <sch_commit.h>
#include <string_utils.h>
#include <line_ending.h>


// Mapping between wxChoice index and LINE_ENDING_STYLE enum.
// Dropdown order: None, Arrow, Open Arrow, Circle, Square
static bool isOpenShape( SHAPE_T aShape )
{
    return aShape == SHAPE_T::ARC || aShape == SHAPE_T::BEZIER || aShape == SHAPE_T::POLY || aShape == SHAPE_T::SEGMENT;
}


void DIALOG_SHAPE_PROPERTIES::createLineEndingControls( SCH_BASE_FRAME* aParent )
{
    wxSizer* mainSizer = GetSizer();
    wxCHECK_RET( mainSizer, wxT( "Shape properties dialog has no main sizer" ) );

    m_endingsSizer = new wxBoxSizer( wxVERTICAL );

    wxGridBagSizer* gbSizerEndings = new wxGridBagSizer( 3, 0 );
    gbSizerEndings->SetFlexibleDirection( wxBOTH );
    gbSizerEndings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_startShapeLabel = new wxStaticText( this, wxID_ANY, _( "Start Shape:" ) );
    m_startShapeLabel->Wrap( -1 );
    gbSizerEndings->Add( m_startShapeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL | wxRIGHT,
                         5 );

    m_startShapeChoice = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0,
                                               nullptr, wxCB_READONLY );
    gbSizerEndings->Add( m_startShapeChoice, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL | wxEXPAND,
                         5 );

    m_endShapeLabel = new wxStaticText( this, wxID_ANY, _( "End Shape:" ) );
    m_endShapeLabel->Wrap( -1 );
    gbSizerEndings->Add( m_endShapeLabel, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ),
                         wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5 );

    m_endShapeChoice = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0,
                                             nullptr, wxCB_READONLY );
    gbSizerEndings->Add( m_endShapeChoice, wxGBPosition( 0, 5 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL | wxEXPAND,
                         5 );

    auto addEndingValue = [&]( int aRow, int aCol, const wxString& aLabel, wxStaticText*& aLabelCtrl,
                               wxTextCtrl*& aValueCtrl, wxStaticText*& aUnitsCtrl, int aFlags )
    {
        aLabelCtrl = new wxStaticText( this, wxID_ANY, aLabel );
        aLabelCtrl->Wrap( -1 );
        gbSizerEndings->Add( aLabelCtrl, wxGBPosition( aRow, aCol ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL | aFlags,
                             5 );

        aValueCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1, -1 ), 0 );
        gbSizerEndings->Add( aValueCtrl, wxGBPosition( aRow, aCol + 1 ), wxGBSpan( 1, 1 ),
                             wxALIGN_CENTER_VERTICAL | wxEXPAND, 5 );

        aUnitsCtrl = new wxStaticText( this, wxID_ANY, _( "unit" ) );
        aUnitsCtrl->Wrap( -1 );
        aUnitsCtrl->SetMinSize( wxSize( 60, -1 ) );
        gbSizerEndings->Add( aUnitsCtrl, wxGBPosition( aRow, aCol + 2 ), wxGBSpan( 1, 1 ),
                             wxALIGN_CENTER_VERTICAL | wxLEFT, 3 );
    };

    addEndingValue( 1, 0, _( "Start Length:" ), m_startLengthLabel, m_startLengthCtrl, m_startLengthUnits, wxRIGHT );
    addEndingValue( 1, 4, _( "End Length:" ), m_endLengthLabel, m_endLengthCtrl, m_endLengthUnits, wxLEFT | wxRIGHT );
    addEndingValue( 2, 0, _( "Start Width:" ), m_startWidthLabel, m_startWidthCtrl, m_startWidthUnits, wxRIGHT );
    addEndingValue( 2, 4, _( "End Width:" ), m_endWidthLabel, m_endWidthCtrl, m_endWidthUnits, wxLEFT | wxRIGHT );
    addEndingValue( 3, 0, _( "Start Stroke Width:" ), m_startStrokeWidthLabel, m_startStrokeWidthCtrl,
                    m_startStrokeWidthUnits, wxRIGHT );
    addEndingValue( 3, 4, _( "End Stroke Width:" ), m_endStrokeWidthLabel, m_endStrokeWidthCtrl, m_endStrokeWidthUnits,
                    wxLEFT | wxRIGHT );

    gbSizerEndings->AddGrowableCol( 1 );
    gbSizerEndings->AddGrowableCol( 5 );

    m_endingsSizer->Add( gbSizerEndings, 0, wxEXPAND | wxALL, 10 );

    m_endingsHelpLabel = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_endingsHelpLabel->Wrap( -1 );
    m_endingsSizer->Add( m_endingsHelpLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10 );

    mainSizer->Insert( 3, m_endingsSizer, 0, wxEXPAND, 5 );

    wxColour fg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
    wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    wxSize   iconSize( 80, 24 );

    struct
    {
        wxString          name;
        LINE_ENDING_STYLE style;
    } shapeItems[] = {
        { _( "None" ), LINE_ENDING_STYLE::NONE },
        { _( "Arrow" ), LINE_ENDING_STYLE::ARROW },
        { _( "Open Arrow" ), LINE_ENDING_STYLE::ARROW_OPEN },
        { _( "Circle" ), LINE_ENDING_STYLE::CIRCLE },
        { _( "Square" ), LINE_ENDING_STYLE::SQUARE },
    };

    for( const auto& item : shapeItems )
    {
        wxBitmap startBmp = MakeLineEndingBitmap( item.style, iconSize, fg, bg, this, false );
        wxBitmap endBmp = MakeLineEndingBitmap( item.style, iconSize, fg, bg, this, true );
        m_startShapeChoice->Append( item.name, startBmp );
        m_endShapeChoice->Append( item.name, endBmp );
    }

    m_startShapeChoice->SetSelection( 0 );
    m_endShapeChoice->SetSelection( 0 );

    m_startLength =
            std::make_unique<UNIT_BINDER>( aParent, m_startLengthLabel, m_startLengthCtrl, m_startLengthUnits, true );
    m_startWidth =
            std::make_unique<UNIT_BINDER>( aParent, m_startWidthLabel, m_startWidthCtrl, m_startWidthUnits, true );
    m_startStrokeWidth = std::make_unique<UNIT_BINDER>( aParent, m_startStrokeWidthLabel, m_startStrokeWidthCtrl,
                                                        m_startStrokeWidthUnits, true );
    m_endLength = std::make_unique<UNIT_BINDER>( aParent, m_endLengthLabel, m_endLengthCtrl, m_endLengthUnits, true );
    m_endWidth = std::make_unique<UNIT_BINDER>( aParent, m_endWidthLabel, m_endWidthCtrl, m_endWidthUnits, true );
    m_endStrokeWidth = std::make_unique<UNIT_BINDER>( aParent, m_endStrokeWidthLabel, m_endStrokeWidthCtrl,
                                                      m_endStrokeWidthUnits, true );
}


DIALOG_SHAPE_PROPERTIES::DIALOG_SHAPE_PROPERTIES( SCH_BASE_FRAME* aParent, SCH_SHAPE* aShape ) :
        DIALOG_SHAPE_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_shape( aShape ),
        m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits, true )
{
    SetTitle( wxString::Format( GetTitle(), aShape->GetFriendlyName() ) );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient because the
    // different shapes (and even whether or not we're within the symbol editor) cause different
    // dialog layouts.
    m_hash_key = TO_UTF8( GetTitle() + aParent->GetName() );

    createLineEndingControls( aParent );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

    m_endingsHelpLabel->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_endingsHelpLabel->SetLabel( wxString::Format( _( "Shape sizes of 0 = auto (%g\u00d7 line width)." ),
                                                    LINE_ENDING::DEFAULT_RATIO_LENGTH ) );

    COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_borderColorSwatch->SetSwatchBackground( schematicBackground );

    for( const auto& [ lineStyle, lineStyleDesc ] : lineTypeNames )
        m_borderStyleCombo->Append( lineStyleDesc.name, KiBitmapBundle( lineStyleDesc.bitmap ) );

    m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_fillColorSwatch->SetSwatchBackground( schematicBackground );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_borderColorSwatch->SetSwatchBackground( canvas.ToColour() );
    m_fillColorSwatch->SetSwatchBackground( canvas.ToColour() );

    if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
        m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );

    m_ruleAreaSizer->Show( dynamic_cast<SCH_RULE_AREA*>( aShape ) != nullptr );

    // Only show line ending controls for open shapes
    m_endingsSizer->Show( isOpenShape( aShape->GetShape() ) );

    SetInitialFocus( m_borderWidthCtrl );

    // Required under wxGTK if we want to dismiss the dialog with the ESC key
    SetFocus();

    SetupStandardButtons();

    if( SYMBOL_EDIT_FRAME* symbolEditor = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
    {
        m_fillBook->SetSelection( 1 );

        if( !symbolEditor->IsSymbolEditable() || symbolEditor->IsSymbolAlias() )
        {
            m_sdbSizerCancel->SetDefault();
            m_sdbSizerOK->SetLabel( _( "Read Only" ) );
            m_sdbSizerOK->Enable( false );
        }
    }
    else
    {
        m_fillBook->SetSelection( 0 );
        m_symbolEditorSizer->Show( false );
    }

    m_borderColorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onBorderSwatch, this );
    m_customColorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onCustomColorSwatch, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_SHAPE_PROPERTIES::~DIALOG_SHAPE_PROPERTIES()
{
    m_borderColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onBorderSwatch, this );
    m_customColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onCustomColorSwatch, this );
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( SCH_RULE_AREA* ruleArea = dynamic_cast<SCH_RULE_AREA*>( m_shape ) )
    {
        m_cbExcludeFromSim->SetValue( ruleArea->GetExcludedFromSim() );
        m_cbExcludeFromBom->SetValue( ruleArea->GetExcludedFromBOM() );
        m_cbExcludeFromBoard->SetValue( ruleArea->GetExcludedFromBoard() );
        m_cbDNP->SetValue( ruleArea->GetDNP() );
    }

    if( m_shape->GetWidth() >= 0 )
    {
        m_borderCheckbox->SetValue( true );
        m_borderWidth.SetValue( m_shape->GetWidth() );
    }
    else
    {
        m_borderCheckbox->SetValue( false );

        m_borderWidth.Enable( false );
        m_borderColorLabel->Enable( false );
        m_borderColorSwatch->Enable( false );
        m_borderStyleLabel->Enable( false );
        m_borderStyleCombo->Enable( false );
    }

    m_borderColorSwatch->SetSwatchColor( m_shape->GetStroke().GetColor(), false );

    int style = static_cast<int>( m_shape->GetStroke().GetLineStyle() );

    if( style == -1 )
        m_borderStyleCombo->SetStringSelection( DEFAULT_LINE_STYLE_LABEL );
    else if( style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( wxT( "Line type not found in the type lookup map" ) );

    if( dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
    {
        m_rbFillNone->Enable( true );
        m_rbFillOutline->Enable( true );
        m_rbFillBackground->Enable( true );
        m_rbFillCustom->Enable( true );
        m_customColorSwatch->Enable( true );

        if( m_shape->GetFillMode() == FILL_T::FILLED_SHAPE )
        {
            m_rbFillOutline->SetValue( true );

            COLOR4D color = m_shape->GetStroke().GetColor();

            if( color == COLOR4D::UNSPECIFIED )
                color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );

            m_customColorSwatch->SetSwatchColor( color, false );
        }
        else if( m_shape->GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
        {
            m_rbFillBackground->SetValue( true );

            COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
            m_customColorSwatch->SetSwatchColor( color, false );
        }
        else if( m_shape->GetFillMode() == FILL_T::FILLED_WITH_COLOR )
        {
            m_rbFillCustom->SetValue( true );
            m_customColorSwatch->SetSwatchColor( m_shape->GetFillColor(), false );
        }
        else
        {
            m_rbFillNone->SetValue( true );
            m_customColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
        }

        const SYMBOL* symbol = m_shape->GetParentSymbol();

        m_privateCheckbox->SetValue( m_shape->IsPrivate() );
        m_checkApplyToAllUnits->SetValue( symbol->IsMultiUnit() && m_shape->GetUnit() == 0 );
        m_checkApplyToAllUnits->Enable( symbol->IsMultiUnit() );
        m_checkApplyToAllBodyStyles->SetValue( symbol->IsMultiBodyStyle() && m_shape->GetBodyStyle() == 0 );
        m_checkApplyToAllBodyStyles->Enable( symbol->IsMultiBodyStyle() );
    }
    else
    {
        m_fillCtrl->SetSelection( m_shape->GetFillModeProp() );
        m_fillColorSwatch->SetSwatchColor( m_shape->GetFillColor(), false );
    }

    m_fillColorLabel->Enable( m_fillCtrl->GetSelection() != UI_FILL_MODE::NONE );
    m_fillColorSwatch->Enable( m_fillCtrl->GetSelection() != UI_FILL_MODE::NONE );

    // Line endings (only populated for open shapes)
    if( isOpenShape( m_shape->GetShape() ) )
    {
        m_startShapeChoice->SetSelection( LINE_ENDING::StyleToChoiceIndex( m_shape->GetStartEndingStyle() ) );
        m_endShapeChoice->SetSelection( LINE_ENDING::StyleToChoiceIndex( m_shape->GetEndEndingStyle() ) );
        m_startLength->SetValue( m_shape->GetStartEndingLength() );
        m_startWidth->SetValue( m_shape->GetStartEndingWidth() );
        m_startStrokeWidth->SetValue( m_shape->GetStartEndingStrokeWidth() );
        m_endLength->SetValue( m_shape->GetEndEndingLength() );
        m_endWidth->SetValue( m_shape->GetEndEndingWidth() );
        m_endStrokeWidth->SetValue( m_shape->GetEndEndingStrokeWidth() );
    }

    return true;
}


void DIALOG_SHAPE_PROPERTIES::onBorderChecked( wxCommandEvent& event )
{
    bool border = m_borderCheckbox->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
    {
        int defaultInMils;

        if( SYMBOL_EDIT_FRAME* symbolEditor = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
            defaultInMils = symbolEditor->libeditconfig()->m_Defaults.line_width;
        else
            defaultInMils = m_frame->eeconfig()->m_Drawing.default_line_thickness;

        m_borderWidth.SetValue( schIUScale.MilsToIU( defaultInMils ) );
    }

    m_borderWidth.Enable( border );
    m_borderColorLabel->Enable( border );
    m_borderColorSwatch->Enable( border );
    m_borderStyleLabel->Enable( border );
    m_borderStyleCombo->Enable( border );
}


void DIALOG_SHAPE_PROPERTIES::onFillChoice( wxCommandEvent& event )
{
    m_fillColorLabel->Enable( m_fillCtrl->GetSelection() != UI_FILL_MODE::NONE );
    m_fillColorSwatch->Enable( m_fillCtrl->GetSelection() != UI_FILL_MODE::NONE );
}


void DIALOG_SHAPE_PROPERTIES::onFillRadioButton( wxCommandEvent& event )
{
    if( event.GetId() == NO_FILL )
    {
        m_rbFillNone->SetValue( true );
        m_customColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }
    else if( event.GetId() == FILLED_SHAPE )
    {
        m_rbFillOutline->SetValue( true );

        COLOR4D color = m_borderColorSwatch->GetSwatchColor();

        if( color == COLOR4D::UNSPECIFIED || !m_rbFillOutline->GetValue() )
            color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );

        m_customColorSwatch->SetSwatchColor( color, false );
    }
    else if( event.GetId() == FILLED_WITH_BG_BODYCOLOR )
    {
        m_rbFillBackground->SetValue( true );

        COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
        m_customColorSwatch->SetSwatchColor( color, false );
    }
    else if( event.GetId() == FILLED_WITH_COLOR )
    {
        m_rbFillCustom->SetValue( true );
        m_customColorSwatch->GetNewSwatchColor();
    }
}


void DIALOG_SHAPE_PROPERTIES::onBorderSwatch( wxCommandEvent& aEvent )
{
    if( m_rbFillOutline->GetValue() )
        m_fillColorSwatch->SetSwatchColor( m_borderColorSwatch->GetSwatchColor(), false );

    if( m_rbFillOutline->IsEnabled() && m_rbFillOutline->GetValue() )
    {
        COLOR4D color = COLOR4D::UNSPECIFIED;

        if( m_rbFillOutline->GetValue() )
            color = m_fillColorSwatch->GetSwatchColor();

        if( color == COLOR4D::UNSPECIFIED )
            color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );

        m_customColorSwatch->SetSwatchColor( color, false );
    }
}


void DIALOG_SHAPE_PROPERTIES::onCustomColorSwatch( wxCommandEvent& aEvent )
{
    m_rbFillCustom->SetValue( true );
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_COMMIT commit( m_frame );

    if( !m_shape->IsNew() )
        commit.Modify( m_shape, m_frame->GetScreen() );

    if( SCH_RULE_AREA* ruleArea = dynamic_cast<SCH_RULE_AREA*>( m_shape ) )
    {
        ruleArea->SetExcludedFromSim( m_cbExcludeFromSim->GetValue() );
        ruleArea->SetExcludedFromBOM( m_cbExcludeFromBom->GetValue() );
        ruleArea->SetExcludedFromBoard( m_cbExcludeFromBoard->GetValue() );
        ruleArea->SetDNP( m_cbDNP->GetValue() );
    }

    STROKE_PARAMS stroke = m_shape->GetStroke();

    if( m_borderCheckbox->GetValue() )
    {
        if( !m_borderWidth.IsIndeterminate() )
            stroke.SetWidth( std::max( 0, m_borderWidth.GetIntValue() ) );
    }
    else
    {
        stroke.SetWidth( -1 );
    }

    auto it = lineTypeNames.begin();
    std::advance( it, m_borderStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetLineStyle( LINE_STYLE::SOLID );
    else
        stroke.SetLineStyle( it->first );

    stroke.SetColor( m_borderColorSwatch->GetSwatchColor() );

    m_shape->SetStroke( stroke );

    if( SYMBOL_EDIT_FRAME* symbolEditor = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
    {
        if( m_rbFillOutline->GetValue() )
            m_shape->SetFillMode( FILL_T::FILLED_SHAPE );
        else if( m_rbFillBackground->GetValue() )
            m_shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
        else if( m_rbFillCustom->GetValue() )
            m_shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        else
            m_shape->SetFillMode( FILL_T::NO_FILL );

        m_shape->SetFillColor( m_customColorSwatch->GetSwatchColor() );

        m_shape->SetPrivate( m_privateCheckbox->GetValue() );

        if( m_checkApplyToAllBodyStyles->IsChecked() )
            m_shape->SetBodyStyle( 0 );
        else
            m_shape->SetBodyStyle( symbolEditor->GetBodyStyle() );

        if( m_checkApplyToAllUnits->IsChecked() )
            m_shape->SetUnit( 0 );
        else
            m_shape->SetUnit( symbolEditor->GetUnit() );
    }
    else
    {
        m_shape->SetFillModeProp( (UI_FILL_MODE) m_fillCtrl->GetSelection() );
        m_shape->SetFillColor( m_fillColorSwatch->GetSwatchColor() );
    }

    // Line endings (only for open shapes)
    if( isOpenShape( m_shape->GetShape() ) )
    {
        int startSel = m_startShapeChoice->GetSelection();

        if( startSel >= 0 && startSel < LINE_ENDING::s_defaultChoiceCount )
            m_shape->SetStartEndingStyle( LINE_ENDING::s_defaultChoiceOrder[startSel] );

        int endSel = m_endShapeChoice->GetSelection();

        if( endSel >= 0 && endSel < LINE_ENDING::s_defaultChoiceCount )
            m_shape->SetEndEndingStyle( LINE_ENDING::s_defaultChoiceOrder[endSel] );

        if( !m_startLength->IsIndeterminate() )
            m_shape->SetStartEndingLength( std::max( 0, m_startLength->GetIntValue() ) );

        if( !m_startWidth->IsIndeterminate() )
            m_shape->SetStartEndingWidth( std::max( 0, m_startWidth->GetIntValue() ) );

        if( !m_startStrokeWidth->IsIndeterminate() )
            m_shape->SetStartEndingStrokeWidth( std::max( 0, m_startStrokeWidth->GetIntValue() ) );

        if( !m_endLength->IsIndeterminate() )
            m_shape->SetEndEndingLength( std::max( 0, m_endLength->GetIntValue() ) );

        if( !m_endWidth->IsIndeterminate() )
            m_shape->SetEndEndingWidth( std::max( 0, m_endWidth->GetIntValue() ) );

        if( !m_endStrokeWidth->IsIndeterminate() )
            m_shape->SetEndEndingStrokeWidth( std::max( 0, m_endStrokeWidth->GetIntValue() ) );
    }

    if( !commit.Empty() )
        commit.Push( wxString::Format( _( "Edit %s" ), m_shape->GetFriendlyName() ) );

    return true;
}
