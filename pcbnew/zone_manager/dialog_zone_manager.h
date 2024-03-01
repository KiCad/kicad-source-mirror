/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_ZONE_MANAGER_H
#define DIALOG_ZONE_MANAGER_H

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
#include <widgets/unit_binder.h>
#include <zone.h>
#include <pad.h>
#include <board.h>
#include <trigo.h>
#include <eda_pattern_match.h>

#include "dialog_zone_manager_base.h"


class PANEL_ZONE_PROPERTIES;
class MODEL_ZONES_PRIORITY_LIST;
class MODEL_ZONES_OVERVIEW_TABLE;
class MODEL_ZONE_LAYERS_LIST;
class ZONES_CONTAINER;
class PANE_ZONE_VIEWER;
class ZONE_FILLER;
class COMMIT;
class PANEL_ZONE_GAL;
enum class ZONE_INDEX_MOVEMENT;
class DIALOG_ZONE_MANAGER : public DIALOG_ZONE_MANAGER_BASE
{
    enum
    {
        ZONE_VIEWER = ID_DIALOG_COPPER_ZONE_BASE + 10,
    };

public:
    DIALOG_ZONE_MANAGER( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aZoneInfo );
    ~DIALOG_ZONE_MANAGER() override;

protected:
    void OnZoneSelectionChanged( ZONE* aZone );

    void OnDataViewCtrlSelectionChanged( wxDataViewEvent& event ) override;

    void SelectZoneTableItem( wxDataViewItem const& aItem );

    void OnViewZonesOverviewOnLeftUp( wxMouseEvent& aEvent ) override;


    void OnOk( wxCommandEvent& aEvt );


#if wxUSE_DRAG_AND_DROP

    void OnBeginDrag( wxDataViewEvent& aEvent );

    void OnDropPossible( wxDataViewEvent& aEvent );

    void OnDrop( wxDataViewEvent& aEvent );

#endif // wxUSE_DRAG_AND_DROP

    void OnZoneNameUpdate( wxCommandEvent& aEvent );

    void OnZonesTableRowCountChange( wxCommandEvent& aEvent );

    void OnCheckBoxClicked( wxCommandEvent& aEvent );

    void MoveSelectedZonePriority( ZONE_INDEX_MOVEMENT aMove );

    void OnMoveUpClick( wxCommandEvent& aEvent );

    void OnMoveDownClick( wxCommandEvent& aEvent );

    void OnFilterCtrlCancel( wxCommandEvent& aEvent ) override;

    void OnFilterCtrlSearch( wxCommandEvent& aEvent ) override;

    void OnFilterCtrlTextChange( wxCommandEvent& aEvent ) override;

    void OnFilterCtrlEnter( wxCommandEvent& aEvent ) override;

    void OnRepourCheck( wxCommandEvent& aEvent ) override;

    void OnButtonApplyClick( wxCommandEvent& aEvent ) override;

    void PostProcessZoneViewSelectionChange( wxDataViewItem const& item );

    void OnTableChar( wxKeyEvent& event ) override;

    void OnTableCharHook( wxKeyEvent& event ) override;

private:
    void GenericProcessChar( wxKeyEvent& event );

    void OnIDle( wxIdleEvent& aEvent );

    void FitCanvasToScreen();


private:
    PCB_BASE_FRAME*                             m_pcbFrame;
    ZONE_SETTINGS*                              m_zoneInfo;
    std::unique_ptr<ZONES_CONTAINER>            m_zonesContainer;
    PANEL_ZONE_PROPERTIES*                      m_panelZoneProperties;
    wxObjectDataPtr<MODEL_ZONES_OVERVIEW_TABLE> m_modelZoneOverviewTable;
    PANE_ZONE_VIEWER*                           m_zoneViewer;
    std::optional<unsigned>                     m_priorityDragIndex;
    std::unique_ptr<ZONE_FILLER>                m_filler;
    bool                                        m_needZoomGAL;
    bool                                        m_isFillingZones;
    bool                                        m_zoneFillComplete;
};

#endif