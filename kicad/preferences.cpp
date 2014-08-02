/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file preferences.cpp
 */

#include <fctsys.h>
#include <pgm_kicad.h>
#include <confirm.h>
#include <gestfich.h>

#include <kicad.h>

#include <wx/fontdlg.h>


void KICAD_MANAGER_FRAME::OnUpdateDefaultPdfBrowser( wxUpdateUIEvent& event )
{
    event.Check( Pgm().UseSystemPdfBrowser() );
}


void KICAD_MANAGER_FRAME::OnSelectDefaultPdfBrowser( wxCommandEvent& event )
{
    Pgm().ForceSystemPdfBrowser( true );
    Pgm().WritePdfBrowserInfos();
}


void KICAD_MANAGER_FRAME::OnUpdatePreferredPdfBrowser( wxUpdateUIEvent& event )
{
    event.Check( !Pgm().UseSystemPdfBrowser() );
}


void KICAD_MANAGER_FRAME::OnSelectPreferredPdfBrowser( wxCommandEvent& event )
{
    bool select = event.GetId() == ID_SELECT_PREFERED_PDF_BROWSER_NAME;

    if( !Pgm().GetPdfBrowserName() && !select )
    {
        DisplayError( this,
                      _( "You must choose a PDF viewer before using this option." ) );
    }

    wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
    mask += wxT( ".exe" );
#endif

    wxString wildcard = _( "Executable files (" ) + mask + wxT( ")|" ) + mask;

    Pgm().ReadPdfBrowserInfos();
    wxFileName fn = Pgm().GetPdfBrowserName();

    wxFileDialog dlg( this, _( "Select Preferred Pdf Browser" ), fn.GetPath(),
                      fn.GetFullPath(), wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    Pgm().SetPdfBrowserName( dlg.GetPath() );
    Pgm().WritePdfBrowserInfos();
}

