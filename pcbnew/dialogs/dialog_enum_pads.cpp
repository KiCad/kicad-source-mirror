/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "dialog_enum_pads.h"

DIALOG_ENUM_PADS::DIALOG_ENUM_PADS( wxWindow*                          aParent,
                                    SEQUENTIAL_PAD_ENUMERATION_PARAMS& aParams ) :
        DIALOG_ENUM_PADS_BASE( aParent ),
        m_params( aParams )
{
    // Transfer data from the params to the dialog
    m_padStartNum->SetValue( m_params.m_start_number );
    m_padNumStep->SetValue( m_params.m_step );
    m_padPrefix->SetValue( m_params.m_prefix.value_or( "" ) );

    SetInitialFocus( m_padPrefix );

    if( m_stdButtons->GetAffirmativeButton() )
        m_stdButtons->GetAffirmativeButton()->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}

bool DIALOG_ENUM_PADS::TransferDataFromWindow()
{
    // Transfer data from the dialog to the params
    m_params.m_start_number = m_padStartNum->GetValue();
    m_params.m_step = m_padNumStep->GetValue();
    m_params.m_prefix = m_padPrefix->GetValue();

    // No other validation implemented
    return true;
}