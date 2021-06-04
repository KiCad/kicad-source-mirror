/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_EXCHANGE_FOOTPRINTS_H_
#define DIALOG_EXCHANGE_FOOTPRINTS_H_

#include <dialog_exchange_footprints_base.h>

#include <board_commit.h>

class PCB_EDIT_FRAME;
class FOOTPRINT;
class LIB_ID;

class DIALOG_EXCHANGE_FOOTPRINTS : public DIALOG_EXCHANGE_FOOTPRINTS_BASE
{
public:
    DIALOG_EXCHANGE_FOOTPRINTS( PCB_EDIT_FRAME* aParent, FOOTPRINT* aFootprint, bool updateMode,
                                bool selectedMode );
    ~DIALOG_EXCHANGE_FOOTPRINTS() override;

private:
    void updateMatchModeRadioButtons( wxUpdateUIEvent& event ) override;
    void OnMatchAllClicked( wxCommandEvent& event ) override;
    void OnMatchSelectedClicked( wxCommandEvent& event ) override;
    void OnMatchRefClicked( wxCommandEvent& event ) override;
    void OnMatchValueClicked( wxCommandEvent& event ) override;
    void OnMatchIDClicked( wxCommandEvent& event ) override;
    void OnOKClicked( wxCommandEvent& event ) override;
    void ViewAndSelectFootprint( wxCommandEvent& event ) override;

    wxRadioButton* getRadioButtonForMode();

    bool isMatch( FOOTPRINT* );
    bool processMatchingFootprints();
    bool processFootprint( FOOTPRINT* aFootprint, const LIB_ID& aNewFPID );

    BOARD_COMMIT    m_commit;
    PCB_EDIT_FRAME* m_parent;
    FOOTPRINT*      m_currentFootprint;
    bool            m_updateMode;
    int*            m_matchMode;
};

#endif // DIALOG_EXCHANGE_FOOTPRINTS_H_
