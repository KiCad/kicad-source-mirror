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

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "confirm.h"
#include "gestfich.h"

#include "kicad.h"

#include <wx/fontdlg.h>


void KICAD_MANAGER_FRAME::OnUpdateDefaultPdfBrowser( wxUpdateUIEvent& event )
{
    event.Check( wxGetApp().UseSystemPdfBrowser() );
}


void KICAD_MANAGER_FRAME::OnSelectDefaultPdfBrowser( wxCommandEvent& event )
{
    wxGetApp().WritePdfBrowserInfos();
}


void KICAD_MANAGER_FRAME::OnUpdatePreferredPdfBrowser( wxUpdateUIEvent& event )
{
    event.Check( !wxGetApp().UseSystemPdfBrowser() );
}


void KICAD_MANAGER_FRAME::OnSelectPreferredPdfBrowser( wxCommandEvent& event )
{
    bool select = event.GetId() == ID_SELECT_PREFERED_PDF_BROWSER_NAME;

    if( !wxGetApp().GetPdfBrowserFileName() && !select )
    {
        DisplayError( this,
                      _( "You must choose a PDF viewer before using this option." ) );
    }

    wxString wildcard( wxT( "*" ) );

#ifdef __WINDOWS__
    wildcard += wxT( ".exe" );
#endif

    wildcard = _( "Executable files (" ) + wildcard + wxT( ")|" ) + wildcard;

    wxGetApp().ReadPdfBrowserInfos();
    wxFileName fn = wxGetApp().GetPdfBrowserFileName();
    wxFileDialog dlg( this, _( "Select Preferred Pdf Browser" ), fn.GetPath(),
                      fn.GetFullName(), wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxGetApp().SetPdfBrowserFileName( dlg.GetPath() );
    wxGetApp().WritePdfBrowserInfos();
}


void KICAD_MANAGER_FRAME::SetLanguage( wxCommandEvent& event )
{
    EDA_BASE_FRAME::SetLanguage( event );
}
