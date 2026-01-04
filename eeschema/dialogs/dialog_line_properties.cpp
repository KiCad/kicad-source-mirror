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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <sch_line.h>
#include <dialog_line_properties.h>
#include <dialogs/dialog_color_picker.h>
#include <settings/settings_manager.h>
#include <sch_edit_frame.h>
#include <stroke_params.h>
#include <widgets/color_swatch.h>
#include <sch_commit.h>


DIALOG_LINE_PROPERTIES::DIALOG_LINE_PROPERTIES( SCH_EDIT_FRAME* aParent,
                                                std::deque<SCH_LINE*>& aLines ) :
        DIALOG_LINE_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_lines( aLines ),
        m_width( aParent, m_staticTextWidth, m_lineWidth, m_staticWidthUnits, true )
{
    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_colorSwatch->SetSwatchBackground( canvas.ToColour() );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

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

    return true;
}


void DIALOG_LINE_PROPERTIES::resetDefaults( wxCommandEvent& event )
{
    m_width.SetValue( 0 );
    m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );

    m_typeCombo->SetStringSelection( DEFAULT_LINE_STYLE_LABEL );

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
    }

    commit.Push( m_lines.size() == 1 ? _( "Edit Line" ) : _( "Edit Lines" ) );
    return true;
}
