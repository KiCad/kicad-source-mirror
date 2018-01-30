/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_libedit_options.cpp
 */

#include <fctsys.h>
#include <base_screen.h>

#include <dialog_libedit_options.h>
#include <lib_edit_frame.h>


DIALOG_LIBEDIT_OPTIONS::DIALOG_LIBEDIT_OPTIONS( LIB_EDIT_FRAME* parent ) :
    DIALOG_LIBEDIT_OPTIONS_BASE( parent ),
    m_last_scale( -1 )
{
    m_sdbSizerOK->SetDefault();

    SetRepeatLabelInc( Parent()->GetRepeatDeltaLabel() );
    SetItemRepeatStep( Parent()->GetRepeatStep() );
    SetPinRepeatStep( Parent()->GetRepeatPinStep() );

    m_scaleSlider->SetStep( 25 );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


void DIALOG_LIBEDIT_OPTIONS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_scaleAuto->SetValue( false );
}


void DIALOG_LIBEDIT_OPTIONS::OnScaleAuto( wxCommandEvent& aEvent )
{
    if( m_scaleAuto->GetValue() )
    {
        m_last_scale = m_scaleSlider->GetValue();
        m_scaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        if( m_last_scale >= 0 )
            m_scaleSlider->SetValue( m_last_scale );
    }
}


bool DIALOG_LIBEDIT_OPTIONS::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    const auto parent = static_cast<LIB_EDIT_FRAME*>( GetParent() );
    const int scale_fourths = parent->GetIconScale();

    if( scale_fourths <= 0 )
    {
        m_scaleAuto->SetValue( true );
        m_scaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        m_scaleAuto->SetValue( false );
        m_scaleSlider->SetValue( scale_fourths * 25 );
    }

    return true;
}


bool DIALOG_LIBEDIT_OPTIONS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    const int scale_fourths = m_scaleAuto->GetValue() ? -1 : m_scaleSlider->GetValue() / 25;
    const auto parent = static_cast<LIB_EDIT_FRAME*>( GetParent() );

    if( parent->GetIconScale() != scale_fourths )
        parent->SetIconScale( scale_fourths );

    return true;
}


void DIALOG_LIBEDIT_OPTIONS::SetGridSizes( const GRIDS& grid_sizes, int grid_id )
{
    wxASSERT( grid_sizes.size() > 0 );

    int select = wxNOT_FOUND;

    for( size_t i = 0; i < grid_sizes.size(); i++ )
    {
        wxString tmp;
        tmp.Printf( wxT( "%0.1f" ), grid_sizes[i].m_Size.x );
        m_choiceGridSize->Append( tmp );

        if( grid_sizes[i].m_CmdId == grid_id )
            select = (int) i;
    }

    m_choiceGridSize->SetSelection( select );
}
