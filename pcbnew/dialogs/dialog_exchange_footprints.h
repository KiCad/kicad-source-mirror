/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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
    ~DIALOG_EXCHANGE_FOOTPRINTS() = default;

    bool TransferDataToWindow() override;

private:
    void updateMatchModeRadioButtons( wxUpdateUIEvent& aEvent ) override;
    void OnMatchAllClicked( wxCommandEvent& aEvent ) override;
    void OnMatchSelectedClicked( wxCommandEvent& aEvent ) override;
    void OnMatchRefClicked( wxCommandEvent& aEvent ) override;
    void OnMatchValueClicked( wxCommandEvent& aEvent ) override;
    void OnMatchIDClicked( wxCommandEvent& aEvent ) override;
    void OnOKClicked( wxCommandEvent& aEvent ) override;
    void ViewAndSelectFootprint( wxCommandEvent& aEvent ) override;

    void onCheckAll( wxCommandEvent& aEvent ) override
    {
        checkAll( true );
    }

    void onUncheckAll( wxCommandEvent& aEvent ) override
    {
        checkAll( false );
    }

    void checkAll( bool aCheck );

    wxRadioButton* getRadioButtonForMode();

    bool isMatch( FOOTPRINT* );
    void processMatchingFootprints();
    void processFootprint( FOOTPRINT* aFootprint, const LIB_ID& aNewFPID );

private:
    BOARD_COMMIT    m_commit;
    PCB_EDIT_FRAME* m_parent;
    FOOTPRINT*      m_currentFootprint;
    EDA_ITEMS       m_newFootprints;
    bool            m_updateMode;
    int*            m_matchMode;
};

#endif // DIALOG_EXCHANGE_FOOTPRINTS_H_
