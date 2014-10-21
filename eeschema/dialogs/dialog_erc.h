/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#include <dialog_erc_base.h>

/* Variable locales */
extern int           DiagErc[PIN_NMAX][PIN_NMAX];
extern bool          DiagErcTableInit; // go to true after DiagErc init
extern int           DefaultDiagErc[PIN_NMAX][PIN_NMAX];

/*  Control identifiers */
#define ID_MATRIX_0 1800

/*!
 * DIALOG_ERC class declaration
 */

class DIALOG_ERC : public DIALOG_ERC_BASE
{
    DECLARE_EVENT_TABLE()

private:
    SCH_EDIT_FRAME* m_parent;
    wxBitmapButton* m_buttonList[PIN_NMAX][PIN_NMAX];
    bool            m_initialized;
    const SCH_MARKER* m_lastMarkerFound;
    static bool     m_writeErcFile;

public:
    DIALOG_ERC( SCH_EDIT_FRAME* parent );
    ~DIALOG_ERC();

private:
    void Init();

    // from DIALOG_ERC_BASE:
	void OnCloseErcDialog( wxCloseEvent& event );
    void OnErcCmpClick( wxCommandEvent& event );
    void OnEraseDrcMarkersClick( wxCommandEvent& event );
    void OnButtonCloseClick( wxCommandEvent& event );
    void OnResetMatrixClick( wxCommandEvent& event );
    void OnLeftClickMarkersList( wxCommandEvent& event );

    // Double click on a marker info:
    void OnLeftDblClickMarkersList( wxCommandEvent& event );

    void TestErc( wxArrayString* aMessagesList );
    void DisplayERC_MarkersList();
    void SelLocal( wxCommandEvent& event );
    void SelNewCmp( wxCommandEvent& event );
    void ResetDefaultERCDiag( wxCommandEvent& event );
    void ChangeErrorLevel( wxCommandEvent& event );
    void ReBuildMatrixPanel();
    void setDRCMatrixButtonState( wxBitmapButton *aButton, int aState );
    void updateMarkerCounts( SCH_SCREENS *screens );
};


#endif

// _DIALOG_ERC_H_
