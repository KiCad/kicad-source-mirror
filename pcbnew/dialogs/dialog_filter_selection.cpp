/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_filter_selection.h>
#include <pcb_edit_frame.h>


DIALOG_FILTER_SELECTION::DIALOG_FILTER_SELECTION( PCB_BASE_FRAME* aParent, OPTIONS& aOptions ) :
    DIALOG_FILTER_SELECTION_BASE( aParent ),
    m_options( aOptions )
{
    m_Include_Modules->SetValue( m_options.includeModules );
    m_IncludeLockedModules->SetValue( m_options.includeLockedModules );

    if( m_Include_Modules->GetValue() )
        m_IncludeLockedModules->Enable();
    else
        m_IncludeLockedModules->Disable();

    m_Include_Tracks->SetValue( m_options.includeTracks );
    m_Include_Vias->SetValue( m_options.includeVias );
    m_Include_Zones->SetValue( m_options.includeZones );
    m_Include_Draw_Items->SetValue( m_options.includeItemsOnTechLayers );
    m_Include_Edges_Items->SetValue( m_options.includeBoardOutlineLayer );
    m_Include_PcbTextes->SetValue( m_options.includePcbTexts );

    m_sdbSizer1OK->SetDefault();
    SetFocus();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_FILTER_SELECTION::checkBoxClicked( wxCommandEvent& aEvent )
{
    if( m_Include_Modules->GetValue() )
        m_IncludeLockedModules->Enable();
    else
        m_IncludeLockedModules->Disable();
}


bool DIALOG_FILTER_SELECTION::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    m_options.includeModules           = m_Include_Modules->GetValue();
    m_options.includeLockedModules     = m_IncludeLockedModules->GetValue();
    m_options.includeTracks            = m_Include_Tracks->GetValue();
    m_options.includeVias              = m_Include_Vias->GetValue();
    m_options.includeZones             = m_Include_Zones->GetValue();
    m_options.includeItemsOnTechLayers = m_Include_Draw_Items->GetValue();
    m_options.includeBoardOutlineLayer = m_Include_Edges_Items->GetValue();
    m_options.includePcbTexts          = m_Include_PcbTextes->GetValue();

    return true;
}
