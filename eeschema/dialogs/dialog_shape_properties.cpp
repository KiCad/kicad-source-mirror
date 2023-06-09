/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/color_swatch.h>
#include <stroke_params.h>
#include <sch_edit_frame.h>
#include <sch_shape.h>
#include <dialog_shape_properties.h>
#include <settings/color_settings.h>
#include <sch_commit.h>


DIALOG_SHAPE_PROPERTIES::DIALOG_SHAPE_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_SHAPE* aShape ) :
    DIALOG_SHAPE_PROPERTIES_BASE( aParent ),
    m_frame( aParent ),
    m_shape( aShape ),
    m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits, true )
{
    SetTitle( wxString::Format( GetTitle(), aShape->EDA_SHAPE::GetFriendlyName() ) );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

    SetInitialFocus( m_borderWidthCtrl );

    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    for( const std::pair<const PLOT_DASH_TYPE, lineTypeStruct>& typeEntry : lineTypeNames )
        m_borderStyleCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );

    m_borderStyleCombo->Append( DEFAULT_STYLE );

    m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_borderColorSwatch->SetSwatchBackground( canvas.ToColour() );
    m_fillColorSwatch->SetSwatchBackground( canvas.ToColour() );

    if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
        m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );

    // Required under wxGTK if we want to dismiss the dialog with the ESC key
    SetFocus();

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

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

    int style = static_cast<int>( m_shape->GetStroke().GetPlotStyle() );

    if( style == -1 )
        m_borderStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    m_filledCtrl->SetValue( m_shape->IsFilled() );
    m_fillColorSwatch->SetSwatchColor( m_shape->GetFillColor(), false );

    return true;
}


void DIALOG_SHAPE_PROPERTIES::onBorderChecked( wxCommandEvent& event )
{
    bool border = m_borderCheckbox->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( schIUScale.MilsToIU( m_frame->eeconfig()->m_Drawing.default_line_thickness ) );

    m_borderWidth.Enable( border );
    m_borderColorLabel->Enable( border );
    m_borderColorSwatch->Enable( border );
    m_borderStyleLabel->Enable( border );
    m_borderStyleCombo->Enable( border );
}


void DIALOG_SHAPE_PROPERTIES::onFillChecked( wxCommandEvent& aEvent )
{
    bool fill = m_filledCtrl->GetValue();

    m_fillColorLabel->Enable( fill );
    m_fillColorSwatch->Enable( fill );
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_COMMIT commit( m_frame );

    if( !m_shape->IsNew() )
        commit.Modify( m_shape, m_frame->GetScreen() );

    STROKE_PARAMS stroke = m_shape->GetStroke();

    if( m_borderCheckbox->GetValue() )
        stroke.SetWidth( std::max( 0, m_borderWidth.GetIntValue() ) );
    else
        stroke.SetWidth( -1 );

    auto it = lineTypeNames.begin();
    std::advance( it, m_borderStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    else
        stroke.SetPlotStyle( it->first );

    stroke.SetColor( m_borderColorSwatch->GetSwatchColor() );

    m_shape->SetStroke( stroke );

    m_shape->SetFillMode( m_filledCtrl->GetValue() ? FILL_T::FILLED_WITH_COLOR : FILL_T::NO_FILL );
    m_shape->SetFillColor( m_fillColorSwatch->GetSwatchColor() );

    if( !commit.Empty() )
    {
        commit.Push( wxString::Format( _( "Edit %s" ), m_shape->EDA_SHAPE::GetFriendlyName() ),
                     SKIP_CONNECTIVITY );
    }

    return true;
}


