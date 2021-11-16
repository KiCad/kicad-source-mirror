/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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


DIALOG_SHAPE_PROPERTIES::DIALOG_SHAPE_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_SHAPE* aShape ) :
    DIALOG_SHAPE_PROPERTIES_BASE( aParent ),
    m_shape( aShape ),
    m_lineWidth( aParent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits, true )
{
    SetTitle( wxString::Format( GetTitle(), aShape->ShowShape() ) );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

    SetInitialFocus( m_lineWidthCtrl );

    m_lineColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    for( const std::pair<const PLOT_DASH_TYPE, lineTypeStruct>& typeEntry : lineTypeNames )
        m_lineStyleCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );

    m_lineStyleCombo->Append( DEFAULT_STYLE );

    m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

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

    m_lineWidth.SetValue( m_shape->GetWidth() );
    m_lineColorSwatch->SetSwatchColor( m_shape->GetStroke().GetColor(), false );

    int style = static_cast<int>( m_shape->GetStroke().GetPlotStyle() );

    if( style == -1 )
        m_lineStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_lineStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    m_filledCtrl->SetValue( m_shape->IsFilled() );
    m_fillColorSwatch->SetSwatchColor( m_shape->GetFillColor(), false );

    return true;
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    STROKE_PARAMS stroke = m_shape->GetStroke();

    if( !m_lineWidth.IsIndeterminate() )
        stroke.SetWidth( m_lineWidth.GetValue() );

    auto it = lineTypeNames.begin();
    std::advance( it, m_lineStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    else
        stroke.SetPlotStyle( it->first );

    stroke.SetColor( m_lineColorSwatch->GetSwatchColor() );

    m_shape->SetStroke( stroke );

    m_shape->SetFillMode( m_filledCtrl->GetValue() ? FILL_T::FILLED_WITH_COLOR : FILL_T::NO_FILL );
    m_shape->SetFillColor( m_fillColorSwatch->GetSwatchColor() );

    return true;
}


