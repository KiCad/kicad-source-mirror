/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Seth Hillbrand <hillbrand@ucdavis.edu>
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

#include <sch_line.h>
#include <dialog_line_properties.h>
#include <dialogs/dialog_color_picker.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <sch_edit_frame.h>
#include <stroke_params.h>
#include <widgets/color_swatch.h>
#include <widgets/line_ending_bitmap.h>
#include <sch_commit.h>
#include <line_ending.h>


void DIALOG_LINE_PROPERTIES::createLineEndingControls( SCH_EDIT_FRAME* aParent )
{
    wxSizer* mainSizer = GetSizer();
    wxCHECK_RET( mainSizer, wxT( "Line properties dialog has no main sizer" ) );

    wxBoxSizer* endingsSizer = new wxBoxSizer( wxVERTICAL );

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

    endingsSizer->Add( gbSizerEndings, 0, wxEXPAND, 0 );

    m_endingsHelpLabel = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_endingsHelpLabel->Wrap( -1 );
    endingsSizer->Add( m_endingsHelpLabel, 0, wxTOP | wxBOTTOM, 5 );

    mainSizer->Insert( 1, endingsSizer, 0, wxEXPAND | wxRIGHT | wxLEFT, 10 );

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


DIALOG_LINE_PROPERTIES::DIALOG_LINE_PROPERTIES( SCH_EDIT_FRAME* aParent,
                                                std::deque<SCH_LINE*>& aLines ) :
        DIALOG_LINE_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_lines( aLines ),
        m_width( aParent, m_staticTextWidth, m_lineWidth, m_staticWidthUnits, true )
{
    createLineEndingControls( aParent );

    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_colorSwatch->SetSwatchBackground( canvas.ToColour() );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

    m_endingsHelpLabel->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_endingsHelpLabel->SetLabel( wxString::Format( _( "Shape sizes of 0 = auto (%g\u00d7 line width)." ),
                                                    LINE_ENDING::DEFAULT_RATIO_LENGTH ) );

    SetInitialFocus( m_lineWidth );

    for( const auto& [ lineStyle, lineStyleDesc ] : lineTypeNames )
        m_typeCombo->Append( lineStyleDesc.name, KiBitmapBundle( lineStyleDesc.bitmap ) );

    SetupStandardButtons( { { wxID_APPLY, _( "Default" ) } } );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_LINE_PROPERTIES::TransferDataToWindow()
{
    SCH_LINE* first_stroke_item = m_lines.front();

    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
            [&]( const SCH_LINE* r )
            {
                return r->GetPenWidth() == first_stroke_item->GetPenWidth();
            } ) )
    {
        m_width.SetValue( first_stroke_item->GetStroke().GetWidth() );
    }
    else
    {
        m_width.SetValue( INDETERMINATE_ACTION );
    }

    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
            [&]( const SCH_LINE* r )
            {
                return r->GetStroke().GetColor() == first_stroke_item->GetStroke().GetColor();
            } ) )
    {
        m_colorSwatch->SetSwatchColor( first_stroke_item->GetStroke().GetColor(), false );
    }
    else
    {
        m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }

    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
            [&]( const SCH_LINE* r )
            {
                return r->GetStroke().GetLineStyle() == first_stroke_item->GetStroke().GetLineStyle();
            } ) )
    {
        int style = static_cast<int>( first_stroke_item->GetStroke().GetLineStyle() );

        if( style >= 0 && style < (int) lineTypeNames.size() )
            m_typeCombo->SetSelection( style );
        else
            m_typeCombo->SetSelection( 0 );
    }
    else
    {
        m_typeCombo->Append( INDETERMINATE_STYLE );
        m_typeCombo->SetStringSelection( INDETERMINATE_STYLE );
    }

    // Line endings - Start Shape
    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
                     [&]( const SCH_LINE* r )
                     {
                         return r->GetStartEndingStyle() == first_stroke_item->GetStartEndingStyle();
                     } ) )
    {
        m_startShapeChoice->SetSelection( LINE_ENDING::StyleToChoiceIndex( first_stroke_item->GetStartEndingStyle() ) );
    }
    else
    {
        m_startShapeChoice->Append( INDETERMINATE_STYLE );
        m_startShapeChoice->SetStringSelection( INDETERMINATE_STYLE );
    }

    // End Shape
    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
                     [&]( const SCH_LINE* r )
                     {
                         return r->GetEndEndingStyle() == first_stroke_item->GetEndEndingStyle();
                     } ) )
    {
        m_endShapeChoice->SetSelection( LINE_ENDING::StyleToChoiceIndex( first_stroke_item->GetEndEndingStyle() ) );
    }
    else
    {
        m_endShapeChoice->Append( INDETERMINATE_STYLE );
        m_endShapeChoice->SetStringSelection( INDETERMINATE_STYLE );
    }

    // Start Length
    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
                     [&]( const SCH_LINE* r )
                     {
                         return r->GetStartEndingLength() == first_stroke_item->GetStartEndingLength();
                     } ) )
    {
        m_startLength->SetValue( first_stroke_item->GetStartEndingLength() );
    }
    else
    {
        m_startLength->SetValue( INDETERMINATE_ACTION );
    }

    // Start Width
    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
                     [&]( const SCH_LINE* r )
                     {
                         return r->GetStartEndingWidth() == first_stroke_item->GetStartEndingWidth();
                     } ) )
    {
        m_startWidth->SetValue( first_stroke_item->GetStartEndingWidth() );
    }
    else
    {
        m_startWidth->SetValue( INDETERMINATE_ACTION );
    }

    // Start Stroke Width
    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
                     [&]( const SCH_LINE* r )
                     {
                         return r->GetStartEndingStrokeWidth() == first_stroke_item->GetStartEndingStrokeWidth();
                     } ) )
    {
        m_startStrokeWidth->SetValue( first_stroke_item->GetStartEndingStrokeWidth() );
    }
    else
    {
        m_startStrokeWidth->SetValue( INDETERMINATE_ACTION );
    }

    // End Length
    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
                     [&]( const SCH_LINE* r )
                     {
                         return r->GetEndEndingLength() == first_stroke_item->GetEndEndingLength();
                     } ) )
    {
        m_endLength->SetValue( first_stroke_item->GetEndEndingLength() );
    }
    else
    {
        m_endLength->SetValue( INDETERMINATE_ACTION );
    }

    // End Width
    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
                     [&]( const SCH_LINE* r )
                     {
                         return r->GetEndEndingWidth() == first_stroke_item->GetEndEndingWidth();
                     } ) )
    {
        m_endWidth->SetValue( first_stroke_item->GetEndEndingWidth() );
    }
    else
    {
        m_endWidth->SetValue( INDETERMINATE_ACTION );
    }

    // End Stroke Width
    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
                     [&]( const SCH_LINE* r )
                     {
                         return r->GetEndEndingStrokeWidth() == first_stroke_item->GetEndEndingStrokeWidth();
                     } ) )
    {
        m_endStrokeWidth->SetValue( first_stroke_item->GetEndEndingStrokeWidth() );
    }
    else
    {
        m_endStrokeWidth->SetValue( INDETERMINATE_ACTION );
    }

    return true;
}


void DIALOG_LINE_PROPERTIES::resetDefaults( wxCommandEvent& event )
{
    m_width.SetValue( 0 );
    m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );

    m_typeCombo->SetStringSelection( DEFAULT_LINE_STYLE_LABEL );

    m_startShapeChoice->SetSelection( 0 );
    m_endShapeChoice->SetSelection( 0 );
    m_startLength->SetValue( 0 );
    m_startWidth->SetValue( 0 );
    m_startStrokeWidth->SetValue( 0 );
    m_endLength->SetValue( 0 );
    m_endWidth->SetValue( 0 );
    m_endStrokeWidth->SetValue( 0 );

    Refresh();
}


bool DIALOG_LINE_PROPERTIES::TransferDataFromWindow()
{
    SCH_COMMIT commit( m_frame );

    for( SCH_LINE* line : m_lines )
    {
        // Commit the change only if the line is not new. If new this is useless
        // and can create dangling pointers if the line creation is aborted
        if( !line->HasFlag( IS_NEW ) )
            commit.Modify( line, m_frame->GetScreen() );

        if( !m_width.IsIndeterminate() )
            line->SetLineWidth( std::max( 0, m_width.GetIntValue() ) );

        auto it = lineTypeNames.begin();
        std::advance( it, m_typeCombo->GetSelection() );

        if( it == lineTypeNames.end() )
            line->SetLineStyle( LINE_STYLE::DEFAULT );
        else
            line->SetLineStyle( it->first );

        line->SetLineColor( m_colorSwatch->GetSwatchColor() );

        // Line endings
        int startSel = m_startShapeChoice->GetSelection();

        if( startSel >= 0 && startSel < LINE_ENDING::s_defaultChoiceCount )
            line->SetStartEndingStyle( LINE_ENDING::s_defaultChoiceOrder[startSel] );

        int endSel = m_endShapeChoice->GetSelection();

        if( endSel >= 0 && endSel < LINE_ENDING::s_defaultChoiceCount )
            line->SetEndEndingStyle( LINE_ENDING::s_defaultChoiceOrder[endSel] );

        if( !m_startLength->IsIndeterminate() )
            line->SetStartEndingLength( std::max( 0, m_startLength->GetIntValue() ) );

        if( !m_startWidth->IsIndeterminate() )
            line->SetStartEndingWidth( std::max( 0, m_startWidth->GetIntValue() ) );

        if( !m_startStrokeWidth->IsIndeterminate() )
            line->SetStartEndingStrokeWidth( std::max( 0, m_startStrokeWidth->GetIntValue() ) );

        if( !m_endLength->IsIndeterminate() )
            line->SetEndEndingLength( std::max( 0, m_endLength->GetIntValue() ) );

        if( !m_endWidth->IsIndeterminate() )
            line->SetEndEndingWidth( std::max( 0, m_endWidth->GetIntValue() ) );

        if( !m_endStrokeWidth->IsIndeterminate() )
            line->SetEndEndingStrokeWidth( std::max( 0, m_endStrokeWidth->GetIntValue() ) );
    }

    commit.Push( m_lines.size() == 1 ? _( "Edit Line" ) : _( "Edit Lines" ) );
    return true;
}
