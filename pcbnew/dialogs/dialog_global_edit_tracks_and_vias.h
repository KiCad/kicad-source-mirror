/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2016 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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

#include "dialog_global_edit_tracks_and_vias_base.h"

#include <widgets/unit_binder.h>
#include <tools/pcb_selection.h>
#include <via_protection_ui_mixin.h>

class DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS : public DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE,
                                           public VIA_PROTECTION_UI_MIXIN
{
public:
    DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParent );
    ~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS() override;

protected:
    void onActionButtonChange( wxCommandEvent& event ) override;

    void OnNetclassFilterSelect( wxCommandEvent& event ) override
    {
        m_netclassFilterOpt->SetValue( true );
    }
    void OnNetFilterSelect( wxCommandEvent& event )
    {
        m_netFilterOpt->SetValue( true );
    }
    void OnLayerFilterSelect( wxCommandEvent& event ) override
    {
        m_layerFilterOpt->SetValue( true );
    }
    void OnTrackWidthText( wxCommandEvent& aEvent ) override
    {
        m_filterByTrackWidth->SetValue( true );
    }
    void OnViaSizeText( wxCommandEvent& aEvent ) override
    {
        m_filterByViaSize->SetValue( true );
    }

    void updateViasCheckbox();

    void onVias( wxCommandEvent& aEvent ) override;
    void onViaType( wxCommandEvent& aEvent ) override;
    void onUnitsChanged( wxCommandEvent& aEvent );

private:
    void visitItem( PICKED_ITEMS_LIST* aUndoList, PCB_TRACK* aItem );
    void processItem( PICKED_ITEMS_LIST* aUndoList, PCB_TRACK* aItem );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void buildFilterLists();

private:
    PCB_EDIT_FRAME* m_parent;
    BOARD*          m_brd;
    PCB_SELECTION   m_selection;

    UNIT_BINDER     m_trackWidthFilter;
    UNIT_BINDER     m_viaSizeFilter;

    std::vector<BOARD_ITEM*>  m_items_changed;        // a list of modified items
};