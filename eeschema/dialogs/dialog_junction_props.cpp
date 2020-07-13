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

#include <bitmaps.h>
#include <sch_junction.h>
#include <dialog_junction_props.h>
#include <dialogs/dialog_color_picker.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <sch_edit_frame.h>

const int BUTT_COLOR_MINSIZE_X = 32;
const int BUTT_COLOR_MINSIZE_Y = 20;


static COLOR4D GetIndeterminateColor()
{
    COLOR4D indeterminateColor;

    indeterminateColor.r = indeterminateColor.b = indeterminateColor.g =
            indeterminateColor.a = -1.0;

    return indeterminateColor;
}


DIALOG_JUNCTION_PROPS::DIALOG_JUNCTION_PROPS( SCH_EDIT_FRAME* aParent,
                                                std::deque<SCH_JUNCTION*>& aJunctions ) :
          DIALOG_JUNCTION_PROPS_BASE( aParent ),
          m_frame( aParent ),
          m_junctions( aJunctions ),
          m_diameter( aParent, m_staticTextDiameter, m_textCtrlDiameter,
                      m_staticTextDiameterUnits, true )
{
    m_sdbSizerApply->SetLabel( _( "Default" ) );

    SetInitialFocus( m_textCtrlDiameter );

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
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
        setColor( firstJunction->GetColor() );
    }
    else
    {
        setColor( GetIndeterminateColor() );
    }

    return true;
}


void DIALOG_JUNCTION_PROPS::onColorButtonClicked( wxCommandEvent& event )
{
    COLOR4D             newColor = COLOR4D::UNSPECIFIED;
    DIALOG_COLOR_PICKER dialog( this, m_selectedColor, false );

    if( dialog.ShowModal() == wxID_OK )
        newColor = dialog.GetColor();

    if( m_selectedColor == newColor )
        return;

    setColor( newColor );
}


void DIALOG_JUNCTION_PROPS::updateColorButton( COLOR4D& aColor )
{
    wxMemoryDC iconDC;

    if( aColor == COLOR4D::UNSPECIFIED || aColor == GetIndeterminateColor() )
    {
        m_buttonColor->SetBitmap( KiBitmap( question_mark_xpm ) );
    }
    else
    {
        wxBitmap bitmap( std::max( m_buttonColor->GetSize().x, BUTT_COLOR_MINSIZE_X ),
                std::max( m_buttonColor->GetSize().y, BUTT_COLOR_MINSIZE_Y ) );

        iconDC.SelectObject( bitmap );
        iconDC.SetPen( *wxBLACK_PEN );

        wxBrush brush( aColor.ToColour() );
        iconDC.SetBrush( brush );

        // Paint the full bitmap in aColor:
        iconDC.SetBackground( brush );
        iconDC.Clear();

        m_buttonColor->SetBitmap( bitmap );
    }

    m_buttonColor->Refresh();

    Refresh( false );
}


void DIALOG_JUNCTION_PROPS::resetDefaults( wxCommandEvent& event )
{
    m_diameter.SetValue( 0 );
    setColor( COLOR4D::UNSPECIFIED );

    Refresh();
}


void DIALOG_JUNCTION_PROPS::setColor( const COLOR4D& aColor )
{
    m_selectedColor = aColor;

    if( aColor == COLOR4D::UNSPECIFIED )
    {
        COLOR4D defaultColor = Pgm().GetSettingsManager().GetColorSettings()->GetColor(
                m_junctions.front()->GetLayer() );
        updateColorButton( defaultColor );
    }
    else
    {
        updateColorButton( m_selectedColor );
    }
}


bool DIALOG_JUNCTION_PROPS::TransferDataFromWindow()
{
    PICKED_ITEMS_LIST pickedItems;

    for( SCH_JUNCTION* junction : m_junctions )
        pickedItems.PushItem( ITEM_PICKER( m_frame->GetScreen(), junction, UR_CHANGED ) );

    m_frame->SaveCopyInUndoList( pickedItems, UR_CHANGED, false );

    for( auto& junction : m_junctions )
    {
        if( !m_diameter.IsIndeterminate() )
            junction->SetDiameter( m_diameter.GetValue() );

        if( m_selectedColor != GetIndeterminateColor() )
            junction->SetColor( m_selectedColor );
    }

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return true;
}
