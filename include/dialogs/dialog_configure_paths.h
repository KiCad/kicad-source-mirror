/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef _DIALOG_CONFIGURE_PATHS_H_
#define _DIALOG_CONFIGURE_PATHS_H_

#include <memory>

#include <wx/string.h>
#include <wx/valtext.h>

#include <../common/dialogs/dialog_configure_paths_base.h>


class HTML_WINDOW;


class DIALOG_CONFIGURE_PATHS: public DIALOG_CONFIGURE_PATHS_BASE
{
public:
    DIALOG_CONFIGURE_PATHS(  wxWindow* aParent );
    ~DIALOG_CONFIGURE_PATHS() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

protected:
    // Various button callbacks
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnAddEnvVar( wxCommandEvent& event ) override;
    void OnRemoveEnvVar( wxCommandEvent& event ) override;
    void OnHelp( wxCommandEvent& event ) override;

    void AppendEnvVar( const wxString& aName, const wxString& aPath, bool isExternal );
    void AppendSearchPath( const wxString& aName, const wxString& aPath, const wxString& aDesc );

private:
    wxString            m_errorMsg;
    wxGrid*             m_errorGrid;
    int                 m_errorRow;
    int                 m_errorCol;

    wxString            m_curdir;
    wxTextValidator     m_aliasValidator;

    HTML_WINDOW*        m_helpBox;
    int                 m_heightBeforeHelp;
};

#endif    // _DIALOG_CONFIGURE_PATHS_H_
