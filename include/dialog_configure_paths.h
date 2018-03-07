/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2015-2018 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <../common/dialogs/dialog_configure_paths_base.h>

#include <wx/valtext.h>
#include <pgm_base.h>


class EDA_DRAW_FRAME;
class FILENAME_RESOLVER;

/**
 * DIALOG_CONFIGURE_PATHS class declaration
 */

class DIALOG_CONFIGURE_PATHS: public DIALOG_CONFIGURE_PATHS_BASE
{
public:
    DIALOG_CONFIGURE_PATHS(  wxWindow* aParent, FILENAME_RESOLVER* aResolver  );
    virtual ~DIALOG_CONFIGURE_PATHS();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

protected:
    // Various button callbacks
    virtual void OnGridCellRightClick( wxGridEvent& event ) override;
    virtual void OnGridSize( wxSizeEvent& event ) override;
    virtual void OnUpdateUI( wxUpdateUIEvent& event ) override;
    virtual void OnGridCellChanging( wxGridEvent& event );
    virtual void OnAddEnvVar( wxCommandEvent& event ) override;
    virtual void OnRemoveEnvVar( wxCommandEvent& event ) override;
    virtual void OnAddSearchPath( wxCommandEvent& event ) override;
    virtual void OnDeleteSearchPath( wxCommandEvent& event ) override;
    virtual void OnSearchPathMoveUp( wxCommandEvent& event ) override;
    virtual void OnSearchPathMoveDown( wxCommandEvent& event ) override;
    virtual void OnHelp( wxCommandEvent& event ) override;

    void AppendEnvVar( const wxString& aName, const wxString& aPath,
                       bool isExternal );
    void AppendSearchPath( const wxString& aName, const wxString& aPath,
                           const wxString& aDescription );
    void AdjustGridColumns( int aWidth );

    /**
     * Determine if a particular ENV_VAR is protected
     */
    bool IsEnvVarImmutable( const wxString aEnvVar );

private:
    wxString               m_errorMsg;
    wxGrid*                m_errorGrid;
    int                    m_errorRow;
    int                    m_errorCol;

    FILENAME_RESOLVER* m_resolver;
    wxString               m_curdir;
    wxTextValidator        m_aliasValidator;

};

#endif    // _DIALOG_CONFIGURE_PATHS_H_
