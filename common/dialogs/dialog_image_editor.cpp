/**
 * @file dialog_image_editor.cpp
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "class_drawpanel.h"

#include "class_bitmap_base.h"

#include "dialog_image_editor.h"

DIALOG_IMAGE_EDITOR::DIALOG_IMAGE_EDITOR( wxWindow* aParent, BITMAP_BASE* aItem )
    : DIALOG_IMAGE_EDITOR_BASE( aParent )
{
    m_workingImage = new BITMAP_BASE( * aItem );
    wxString msg;
    msg.Printf( wxT("%f"), m_workingImage->m_Scale );
    m_textCtrlScale->SetValue( msg );;

    GetSizer()->SetSizeHints( this );
    Layout();
    Fit();
    SetMinSize( GetBestSize() );

    Centre();
    SetFocus();
}

void DIALOG_IMAGE_EDITOR::OnMirrorX_click( wxCommandEvent& event )
{

    m_workingImage->Mirror( true );
    m_panelDraw->Refresh();
}

void DIALOG_IMAGE_EDITOR::OnMirrorY_click( wxCommandEvent& event )
{
    m_workingImage->Mirror( false );
    m_panelDraw->Refresh();
}

void DIALOG_IMAGE_EDITOR::OnRotateClick( wxCommandEvent& event )
{
    m_workingImage->Rotate( false );
    m_panelDraw->Refresh();
}

void DIALOG_IMAGE_EDITOR::OnOK_Button( wxCommandEvent& aEvent )
{
    EndModal( wxID_OK );
}

void DIALOG_IMAGE_EDITOR::OnCancel_Button( wxCommandEvent& aEvent )
{
    EndModal( wxID_CANCEL );
}

void DIALOG_IMAGE_EDITOR::OnRedrawPanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelDraw );
    wxSize size = m_panelDraw->GetClientSize();
    dc.SetDeviceOrigin( size.x/2, size.y/2 );

    double scale = 1.0 / m_workingImage->GetScalingFactor();
    dc.SetUserScale( scale, scale );
    m_workingImage->DrawBitmap( NULL, &dc, wxPoint(0,0) );
}

void DIALOG_IMAGE_EDITOR::TransfertToImage(BITMAP_BASE* aItem )
{
    wxString msg = m_textCtrlScale->GetValue();
    msg.ToDouble( &m_workingImage->m_Scale );
    m_textCtrlScale->SetValue( msg );
    aItem->ImportData( m_workingImage );
}

