/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <gerbview.h>
#include <gerbview_frame.h>

#include "panel_gerbview_settings.h"


PANEL_GERBVIEW_SETTINGS::PANEL_GERBVIEW_SETTINGS( GERBVIEW_FRAME *aFrame, wxWindow* aWindow ) :
        PANEL_GERBVIEW_SETTINGS_BASE( aWindow, wxID_ANY ),
        m_Parent( aFrame )
{
}


bool PANEL_GERBVIEW_SETTINGS::TransferDataToWindow( )
{
    m_PolarDisplay->SetSelection( m_Parent->m_DisplayOptions.m_DisplayPolarCood ? 1 : 0 );
    m_BoxUnits->SetSelection( m_Parent->GetUserUnits() ? 1 : 0 );
    m_ShowPageLimitsOpt->SetValue( m_Parent->m_DisplayOptions.m_DisplayPageLimits );

    for( unsigned i = 0;  i < arrayDim( g_GerberPageSizeList );  ++i )
    {
        if( g_GerberPageSizeList[i] == m_Parent->GetPageSettings().GetType() )
        {
            m_PageSize->SetSelection( i );
            break;
        }
    }

    return true;
}


bool PANEL_GERBVIEW_SETTINGS::TransferDataFromWindow()
{
    m_Parent->m_DisplayOptions.m_DisplayPolarCood = ( m_PolarDisplay->GetSelection() != 0 );
    m_Parent->SetUserUnits( m_BoxUnits->GetSelection() == 0 ? INCHES : MILLIMETRES );
    m_Parent->m_DisplayOptions.m_DisplayPageLimits = m_ShowPageLimitsOpt->GetValue();

    PAGE_INFO pageInfo( g_GerberPageSizeList[ m_PageSize->GetSelection() ] );
    m_Parent->SetPageSettings( pageInfo );

    return true;
}

