/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_ERC_H_
#define _DIALOG_ERC_H_

#include <wx/htmllbox.h>
#include <vector>
#include <lib_pin.h>        // For PINTYPE_COUNT definition

#include <dialog_erc_base.h>
#include <erc_settings.h>
#include "dialog_erc_listbox.h"

// DIALOG_ERC class declaration

class DIALOG_ERC : public DIALOG_ERC_BASE
{
    DECLARE_EVENT_TABLE()

private:
    SCH_EDIT_FRAME* m_parent;
    wxBitmapButton* m_buttonList[PINTYPE_COUNT][PINTYPE_COUNT];
    bool            m_initialized;
    const SCH_MARKER* m_lastMarkerFound;
    static bool     m_diagErcTableInit; // go to true after DiagErc init
    ERC_SETTINGS    m_settings;

public:
    DIALOG_ERC( SCH_EDIT_FRAME* parent );
    ~DIALOG_ERC();

private:
    void Init();

    // from DIALOG_ERC_BASE:
    void OnCloseErcDialog( wxCloseEvent& event ) override;
    void OnErcCmpClick( wxCommandEvent& event ) override;
    void OnEraseDrcMarkersClick( wxCommandEvent& event ) override;
    void OnButtonCloseClick( wxCommandEvent& event ) override;
    void OnResetMatrixClick( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    void RedrawDrawPanel();

    // Click on a marker info:
    void OnLeftClickMarkersList( wxHtmlLinkEvent& event ) override;

    // Double click on a marker info:
    void OnLeftDblClickMarkersList( wxMouseEvent& event ) override;

    void TestErc( REPORTER& aReporter );
    void DisplayERC_MarkersList();
    void ResetDefaultERCDiag( wxCommandEvent& event );
    void ChangeErrorLevel( wxCommandEvent& event );
    void ReBuildMatrixPanel();
    void setDRCMatrixButtonState( wxBitmapButton *aButton, int aState );
    void updateMarkerCounts( SCH_SCREENS *screens );
    void transferSettingsToControls();
    void transferControlsToSettings();
};


#endif

// _DIALOG_ERC_H_
