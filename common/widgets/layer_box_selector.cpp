/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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


#include <wx/dcmemory.h>
#include <wx/odcombo.h>
#include <wx/menuitem.h>
#include <wx/settings.h>

#include <dpi_scaling_common.h>
#include <layer_ids.h>
#include <widgets/layer_box_selector.h>
#include "kiplatform/ui.h"


LAYER_SELECTOR::LAYER_SELECTOR()
{
    m_layerhotkeys = true;
}


bool LAYER_SELECTOR::SetLayersHotkeys( bool value )
{
    m_layerhotkeys = value;
    return m_layerhotkeys;
}


LAYER_BOX_SELECTOR::LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                        const wxSize& size, int n, const wxString choices[] ) :
        wxBitmapComboBox( parent, id, wxEmptyString, pos, size, n, choices, wxCB_READONLY ),
        LAYER_SELECTOR()
{
#ifdef __WXMAC__
    GetParent()->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( LAYER_BOX_SELECTOR::onKeyDown ),
                          nullptr, this );
#endif
}


LAYER_BOX_SELECTOR::~LAYER_BOX_SELECTOR()
{
#ifdef __WXMAC__
    GetParent()->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( LAYER_BOX_SELECTOR::onKeyDown ),
                             nullptr, this );
#endif
}


int LAYER_BOX_SELECTOR::GetLayerSelection() const
{
    if( GetSelection() < 0 )
        return UNDEFINED_LAYER;

    return (int)(intptr_t) GetClientData( GetSelection() );
}


int LAYER_BOX_SELECTOR::SetLayerSelection( int layer )
{
    for( int i = 0; i < (int) GetCount(); i++ )
    {
        if( GetClientData( (unsigned) i ) == (void*)(intptr_t) layer )
        {
            if( GetSelection() != i )   // Element (i) is not selected
            {
                SetSelection( i );
                return i;
            }
            else
            {
                return i;               // If element already selected; do nothing
            }
        }
    }

    // Not Found
    SetSelection( -1 );
    return -1;
}


#ifdef __WXMAC__

void LAYER_BOX_SELECTOR::onKeyDown( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_ESCAPE && IsPopupShown() )
    {
        Dismiss();
        return;
    }

    aEvent.Skip();
}


void LAYER_BOX_SELECTOR::OnDrawBackground( wxDC& dc, const wxRect& rect, int item, int flags) const
{
    if( ( flags & wxODCB_PAINTING_CONTROL ) && !IsEnabled() )
    {
        wxColour fgCol = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );
        wxColour bgCol = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );

        if( KIPLATFORM::UI::IsDarkTheme() )
            bgCol = bgCol.ChangeLightness( 106 );
        else
            bgCol = bgCol.ChangeLightness( 160 );

        dc.SetTextForeground( fgCol );
        dc.SetBrush( bgCol );
        dc.SetPen( bgCol );
        dc.DrawRectangle( rect.Inflate( 1, 1 ) );
        dc.SetClippingRegion( rect );
        return;
    }

    wxBitmapComboBox::OnDrawBackground( dc, rect, item, flags );
}

#endif
