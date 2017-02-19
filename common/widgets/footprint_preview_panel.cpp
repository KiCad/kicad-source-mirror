/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
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

#include <widgets/footprint_preview_panel.h>
#include <wx/stattext.h>
#include <kiway.h>


FOOTPRINT_PREVIEW_PANEL* FOOTPRINT_PREVIEW_PANEL::InstallOnPanel(
        KIWAY& aKiway, wxPanel* aPanel, bool aStatus )
{
    FOOTPRINT_PREVIEW_PANEL* fpp = NULL;

    KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );
    if( !kiface )
        return NULL;

    try {
        fpp = static_cast<FOOTPRINT_PREVIEW_PANEL*>(
                kiface->CreateWindow( aPanel, FRAME_PCB_FOOTPRINT_PREVIEW, &aKiway ) );
    } catch( ... )
    {
        return NULL;
    }

    aPanel->SetBackgroundColour( *wxBLACK );
    aPanel->SetForegroundColour( *wxWHITE );

    auto sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( fpp, 1, wxALL | wxEXPAND, 0 );

    if( aStatus )
    {
        auto label = new wxStaticText( aPanel, -1, wxEmptyString );
        auto sizer2 = new wxBoxSizer( wxVERTICAL );
        sizer2->Add( 0, 0, 1 );
        sizer2->Add( label, 0, wxALL | wxALIGN_CENTER, 0 );
        sizer2->Add( 0, 0, 1 );
        sizer->Add( sizer2, 1, wxALL | wxALIGN_CENTER, 0 );
        sizer2->ShowItems( false );
        fpp->LinkErrorLabel( label );
        fpp->SetHideSizer( sizer2 );
    }

    aPanel->SetSizer( sizer );

    return fpp;
}
