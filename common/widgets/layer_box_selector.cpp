/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <layers_id_colors_and_visibility.h>
#include <bitmaps.h>

#include <wx/dcmemory.h>
#include <wx/odcombo.h>
#include <wx/menuitem.h>

#include <widgets/layer_box_selector.h>


LAYER_SELECTOR::LAYER_SELECTOR()
{
    m_layerhotkeys = true;
}


bool LAYER_SELECTOR::SetLayersHotkeys( bool value )
{
    m_layerhotkeys = value;
    return m_layerhotkeys;
}


void LAYER_SELECTOR::DrawColorSwatch( wxBitmap& aLayerbmp, COLOR4D aBackground, COLOR4D aColor )
{
    wxMemoryDC bmpDC;
    wxBrush    brush;

    // Prepare Bitmap
    bmpDC.SelectObject( aLayerbmp );

    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    if( aBackground != COLOR4D::UNSPECIFIED )
    {
        brush.SetColour( aBackground.WithAlpha( 1.0 ).ToColour() );
        bmpDC.SetBrush( brush );
        bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );
    }

    brush.SetColour( aColor.ToColour() );
    bmpDC.SetBrush( brush );
    bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );

    bmpDC.SetBrush( *wxTRANSPARENT_BRUSH );
    bmpDC.SetPen( *wxBLACK_PEN );
    bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );
}


LAYER_BOX_SELECTOR::LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                                        const wxPoint& pos, const wxSize& size,
                                        int n, const wxString choices[] ) :
    wxBitmapComboBox( parent, id, wxEmptyString, pos, size, n, choices, wxCB_READONLY ),
    LAYER_SELECTOR()
{
    if( choices != nullptr )
        ResyncBitmapOnly();

    GetParent()->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( LAYER_BOX_SELECTOR::onKeyDown ),
                          nullptr, this );
}


LAYER_BOX_SELECTOR::LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                                        const wxPoint& pos, const wxSize& size,
                                        const wxArrayString& choices ) :
    wxBitmapComboBox( parent, id, wxEmptyString, pos, size, choices, wxCB_READONLY ),
    LAYER_SELECTOR()
{
    if( !choices.IsEmpty() )
        ResyncBitmapOnly();

    GetParent()->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( LAYER_BOX_SELECTOR::onKeyDown ),
                          nullptr, this );
}


LAYER_BOX_SELECTOR::~LAYER_BOX_SELECTOR()
{
    GetParent()->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( LAYER_BOX_SELECTOR::onKeyDown ),
                             nullptr, this );
}


int LAYER_BOX_SELECTOR::GetChoice()
{
    return GetSelection();
}


LAYER_NUM LAYER_BOX_SELECTOR::GetLayerSelection() const
{
    if( GetSelection() < 0 )
        return UNDEFINED_LAYER;

    return (LAYER_NUM)(intptr_t) GetClientData( GetSelection() );
}


int LAYER_BOX_SELECTOR::SetLayerSelection( LAYER_NUM layer )
{
    int elements = GetCount();

    for( int i = 0; i < elements; i++ )
    {
        if( GetClientData( (unsigned) i ) == (void*)(intptr_t) layer )
        {
            if( GetSelection() != i )   // Element (i) is not selected
            {
                SetSelection( i );
                return i;
            }
            else
                return i;               //If element already selected; do nothing
        }
    }

    // Not Found
    SetSelection( -1 );
    return -1;
}


void LAYER_BOX_SELECTOR::ResyncBitmapOnly()
{
    int elements = GetCount();

    for( LAYER_NUM i = 0; i < elements; ++i )
    {
        wxBitmap layerbmp( 14, 14 );
        DrawColorSwatch( layerbmp, getLayerColor( LAYER_PCB_BACKGROUND ), getLayerColor( i ) );
    }
}


void LAYER_BOX_SELECTOR::onKeyDown( wxKeyEvent& aEvent )
{
#ifdef __WXOSX_MAC__
    if( aEvent.GetKeyCode() == WXK_ESCAPE && IsPopupShown() )
    {
        Dismiss();
        return;
    }
#endif

    aEvent.Skip();
}
