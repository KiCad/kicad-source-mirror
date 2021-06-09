/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_junction.h>
#include <dialog_junction_props.h>
#include <settings/settings_manager.h>
#include <sch_edit_frame.h>
#include <widgets/color_swatch.h>


DIALOG_JUNCTION_PROPS::DIALOG_JUNCTION_PROPS( SCH_EDIT_FRAME* aParent,
                                              std::deque<SCH_JUNCTION*>& aJunctions ) :
          DIALOG_JUNCTION_PROPS_BASE( aParent ),
          m_frame( aParent ),
          m_junctions( aJunctions ),
          m_diameter( aParent, m_staticTextDiameter, m_textCtrlDiameter,
                      m_staticTextDiameterUnits, true )
{
    m_sdbSizerApply->SetLabel( _( "Default" ) );

    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    SetInitialFocus( m_textCtrlDiameter );

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_JUNCTION_PROPS::TransferDataToWindow()
{
    auto firstJunction = m_junctions.front();

    if( std::all_of( m_junctions.begin() + 1, m_junctions.end(),
            [&]( const SCH_JUNCTION* r )
            {
                return r->GetDiameter() == firstJunction->GetDiameter();
            } ) )
    {
        m_diameter.SetValue( firstJunction->GetDiameter() );
    }
    else
    {
        m_diameter.SetValue( INDETERMINATE_ACTION );
    }

    if( std::all_of( m_junctions.begin() + 1, m_junctions.end(),
            [&]( const SCH_JUNCTION* r )
            {
                return r->GetColor() == firstJunction->GetColor();
            } ) )
    {
        m_colorSwatch->SetSwatchColor( firstJunction->GetColor(), false );
    }
    else
    {
        m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }

    return true;
}


void DIALOG_JUNCTION_PROPS::resetDefaults( wxCommandEvent& event )
{
    m_diameter.SetValue( 0 );
    m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );

    Refresh();
}


bool DIALOG_JUNCTION_PROPS::TransferDataFromWindow()
{
    PICKED_ITEMS_LIST pickedItems;

    for( SCH_JUNCTION* junction : m_junctions )
        pickedItems.PushItem( ITEM_PICKER( m_frame->GetScreen(), junction, UNDO_REDO::CHANGED ) );

    m_frame->SaveCopyInUndoList( pickedItems, UNDO_REDO::CHANGED, false );

    for( SCH_JUNCTION* junction : m_junctions )
    {
        if( !m_diameter.IsIndeterminate() )
            junction->SetDiameter( m_diameter.GetValue() );

        junction->SetColor( m_colorSwatch->GetSwatchColor() );

        m_frame->GetCanvas()->GetView()->Update( junction );
    }

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return true;
}
