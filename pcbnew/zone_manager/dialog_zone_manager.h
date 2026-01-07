/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#pragma once

#include <memory>
#include <optional>
#include <wx/dataview.h>
#include <wx/event.h>
#include <wx/radiobut.h>
#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <zones.h>
#include <zone_settings_bag.h>
#include <widgets/unit_binder.h>
#include <zone.h>
#include <pad.h>
#include <board.h>
#include <trigo.h>
#include <eda_pattern_match.h>

#include "dialog_zone_manager_base.h"


class PANEL_ZONE_PROPERTIES;
class MODEL_ZONES_OVERVIEW;
class ZONE_PREVIEW_NOTEBOOK;
class ZONE_FILLER;
class COMMIT;
class ZONE_PREVIEW_CANVAS;
enum class ZONE_INDEX_MOVEMENT;


class DIALOG_ZONE_MANAGER : public DIALOG_ZONE_MANAGER_BASE
{
public:
    DIALOG_ZONE_MANAGER( PCB_BASE_FRAME* aParent );
    ~DIALOG_ZONE_MANAGER() override;

    bool TransferDataToWindow() override;

    bool GetRepourOnClose() { return m_checkRepour->GetValue(); }

protected:
    void OnZoneSelectionChanged( ZONE* aZone );
    void OnDataViewCtrlSelectionChanged( wxDataViewEvent& event ) override;
    void SelectZoneTableItem( wxDataViewItem const& aItem );
    void OnViewZonesOverviewOnLeftUp( wxMouseEvent& aEvent ) override;
	void onDialogResize( wxSizeEvent& event ) override;
    void OnOk( wxCommandEvent& aEvt ) override;

#if wxUSE_DRAG_AND_DROP
    void OnBeginDrag( wxDataViewEvent& aEvent );
    void OnDropPossible( wxDataViewEvent& aEvent );
    void OnDrop( wxDataViewEvent& aEvent );
#endif // wxUSE_DRAG_AND_DROP

    void OnZoneNameUpdate( wxCommandEvent& aEvent );
    void OnZoneNetUpdate( wxCommandEvent& aEvent );
    void OnZonesTableRowCountChange( wxCommandEvent& aEvent );
    void OnCheckBoxClicked( wxCommandEvent& aEvent );

    void MoveSelectedZonePriority( ZONE_INDEX_MOVEMENT aMove );

    void OnMoveUpClick( wxCommandEvent& aEvent ) override;
    void OnMoveDownClick( wxCommandEvent& aEvent ) override;
    void OnFilterCtrlCancel( wxCommandEvent& aEvent ) override;
    void OnFilterCtrlSearch( wxCommandEvent& aEvent ) override;
    void OnFilterCtrlTextChange( wxCommandEvent& aEvent ) override;
    void OnFilterCtrlEnter( wxCommandEvent& aEvent ) override;
    void OnUpdateDisplayedZonesClick( wxCommandEvent& aEvent ) override;

    void PostProcessZoneViewSelChange( wxDataViewItem const& aItem );

    void OnTableChar( wxKeyEvent& event ) override;
    void OnTableCharHook( wxKeyEvent& event ) override;

private:
    void GenericProcessChar( wxKeyEvent& event );

    void OnIdle( wxIdleEvent& aEvent );

private:
    PCB_BASE_FRAME*                       m_pcbFrame;
    ZONE_SETTINGS_BAG                     m_zoneSettingsBag;
    PANEL_ZONE_PROPERTIES*                m_panelZoneProperties;
    wxObjectDataPtr<MODEL_ZONES_OVERVIEW> m_modelZonesOverview;
    ZONE_PREVIEW_NOTEBOOK*                m_zonePreviewNotebook;
    std::optional<unsigned>               m_priorityDragIndex;
    std::unique_ptr<ZONE_FILLER>          m_filler;
    bool                                  m_isFillingZones;
    bool                                  m_zoneFillComplete;
};
