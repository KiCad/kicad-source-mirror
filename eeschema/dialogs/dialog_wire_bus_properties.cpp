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
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <dialog_wire_bus_properties.h>
#include <dialogs/dialog_color_picker.h>
#include <settings/settings_manager.h>
#include <sch_edit_frame.h>
#include <stroke_params.h>
#include <widgets/color_swatch.h>
#include <sch_commit.h>


DIALOG_WIRE_BUS_PROPERTIES::DIALOG_WIRE_BUS_PROPERTIES( SCH_EDIT_FRAME* aParent,
                                                        std::deque<SCH_ITEM*>& aItems ) :
        DIALOG_WIRE_BUS_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_items( aItems ),
        m_wireWidth( aParent, m_staticTextWidth, m_lineWidth, m_staticWidthUnits ),
        m_junctionSize( aParent, m_dotSizeLabel, m_dotSizeCtrl, m_dotSizeUnits )
{
    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_colorSwatch->SetSwatchBackground( canvas.ToColour() );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

    SetInitialFocus( m_lineWidth );

    for( const auto& [ lineStyle, lineStyleDesc ] : lineTypeNames )
        m_typeCombo->Append( lineStyleDesc.name, KiBitmapBundle( lineStyleDesc.bitmap ) );

    m_typeCombo->Append( DEFAULT_WIRE_STYLE_LABEL );

    SetupStandardButtons( { { wxID_APPLY, _( "Default" ) } } );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_WIRE_BUS_PROPERTIES::TransferDataToWindow()
{
    STROKE_PARAMS stroke;
    COLOR4D       color;
    int           dotSize = -1;     // set value to "not found"

    for( SCH_ITEM* item : m_items )
    {
        if( item->HasLineStroke() )
        {
            stroke = item->GetStroke();
            color = stroke.GetColor();
        }
        else
        {
            wxASSERT( item->Type() == SCH_JUNCTION_T );
            SCH_JUNCTION* junction = static_cast<SCH_JUNCTION*>( item );
            color = junction->GetColor();
            dotSize = junction->GetDiameter();
        }
    }

    if( std::all_of( m_items.begin(), m_items.end(),
            [&]( const SCH_ITEM* item )
            {
                return !item->HasLineStroke() || item->GetStroke().GetWidth() == stroke.GetWidth();
            } ) )
    {
        m_wireWidth.SetValue( stroke.GetWidth() );
    }
    else
    {
        m_wireWidth.SetValue( INDETERMINATE_ACTION );
    }

    if( std::all_of( m_items.begin(), m_items.end(),
            [&]( const SCH_ITEM* item )
            {
                if( item->HasLineStroke() )
                    return item->GetStroke().GetColor() == color;
                else
                    return static_cast<const SCH_JUNCTION*>( item )->GetColor() == color;
            } ) )
    {
        m_colorSwatch->SetSwatchColor( color, false );
    }
    else
    {
        m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }

    if( std::all_of( m_items.begin(), m_items.end(),
            [&]( const SCH_ITEM* item )
            {
                return !item->HasLineStroke()
                        || item->GetStroke().GetLineStyle() == stroke.GetLineStyle();
            } ) )
    {
        int style = static_cast<int>( stroke.GetLineStyle() );

        if( style == -1 )
            m_typeCombo->SetStringSelection( DEFAULT_WIRE_STYLE_LABEL );
        else if( style < (int) lineTypeNames.size() )
            m_typeCombo->SetSelection( style );
        else
            wxFAIL_MSG( "Line type not found in the type lookup map" );
    }
    else
    {
        m_typeCombo->Append( INDETERMINATE_STYLE );
        m_typeCombo->SetStringSelection( INDETERMINATE_STYLE );
    }

    if( std::all_of( m_items.begin(), m_items.end(),
            [&]( const SCH_ITEM* item )
            {
                return item->Type() != SCH_JUNCTION_T
                        || static_cast<const SCH_JUNCTION*>( item )->GetDiameter() == dotSize;
            } ) )
    {
        if( dotSize >=0 )
        {
            m_junctionSize.SetValue( dotSize );
        }
        else
        {
            // No junction found in selected items: disable m_junctionSize
            m_junctionSize.Enable( false );
            m_junctionSize.SetValue( INDETERMINATE_ACTION );
        }
    }
    else
    {
        m_junctionSize.SetValue( INDETERMINATE_ACTION );
    }

    return true;
}


void DIALOG_WIRE_BUS_PROPERTIES::resetDefaults( wxCommandEvent& event )
{
    m_wireWidth.SetValue( 0 );
    m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    m_typeCombo->SetStringSelection( DEFAULT_WIRE_STYLE_LABEL );
    m_junctionSize.SetValue( 0 );

    Refresh();
}


bool DIALOG_WIRE_BUS_PROPERTIES::TransferDataFromWindow()
{
    SCH_COMMIT commit( m_frame );

    for( SCH_ITEM* item : m_items )
    {
        commit.Modify( item, m_frame->GetScreen() );

        if( item->HasLineStroke() )
        {
            if( !m_wireWidth.IsIndeterminate() )
            {
                int width = std::max( 0, m_wireWidth.GetIntValue() );

                if( item->Type() == SCH_LINE_T )
                    static_cast<SCH_LINE*>( item )->SetLineWidth( width );
                else if( dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ) )
                    static_cast<SCH_BUS_ENTRY_BASE*>( item )->SetPenWidth( width );
            }

            if( m_typeCombo->GetStringSelection() != INDETERMINATE_STYLE )
            {
                LINE_STYLE lineStyle = LINE_STYLE::DEFAULT;

                size_t lineTypeSelection = m_typeCombo->GetSelection();
                auto it = lineTypeNames.begin();
                std::advance( it, lineTypeSelection );

                if( it != lineTypeNames.end() )
                    lineStyle = it->first;

                if( item->Type() == SCH_LINE_T )
                    static_cast<SCH_LINE*>( item )->SetLineStyle( lineStyle );
                else if( dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ) )
                    static_cast<SCH_BUS_ENTRY_BASE*>( item )->SetLineStyle( lineStyle );
            }

            COLOR4D color = m_colorSwatch->GetSwatchColor();

            if( item->Type() == SCH_LINE_T )
                static_cast<SCH_LINE*>( item )->SetLineColor( color );
            else if( dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ) )
                static_cast<SCH_BUS_ENTRY_BASE*>( item )->SetBusEntryColor( color );
        }
        else
        {
            SCH_JUNCTION* junction = static_cast<SCH_JUNCTION*>( item );

            junction->SetColor( m_colorSwatch->GetSwatchColor() );

            if( !m_junctionSize.IsIndeterminate() )
                junction->SetDiameter( m_junctionSize.GetValue() );
        }
    }

    commit.Push( wxString::Format( _( "Edit %s" ), m_items.size() == 1 ? _( "Wire/Bus" )
                                                                       : _( "Wires/Buses" ) ) );
    return true;
}
