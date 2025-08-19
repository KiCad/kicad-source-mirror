/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <dialogs/dialog_track_via_properties_base.h>
#include <widgets/unit_binder.h>
#include <optional>
#include <layer_ids.h>
#include <via_protection_ui_mixin.h>

class PCB_SELECTION;
class PCB_BASE_EDIT_FRAME;
class PAD;
class PADSTACK;

class DIALOG_TRACK_VIA_PROPERTIES : public DIALOG_TRACK_VIA_PROPERTIES_BASE, public VIA_PROTECTION_UI_MIXIN
{
public:
    DIALOG_TRACK_VIA_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, const PCB_SELECTION& aItems );

    ~DIALOG_TRACK_VIA_PROPERTIES();

    bool TransferDataFromWindow() override;

private:
    void onNetSelector( wxCommandEvent& aEvent );
    void onViaNotFreeClicked( wxCommandEvent& aEvent ) override;
    void onWidthSelect( wxCommandEvent& aEvent ) override;
    void onWidthEdit( wxCommandEvent& aEvent ) override;
    void onViaSelect( wxCommandEvent& aEvent ) override;
    void onViaEdit( wxCommandEvent& aEvent ) override;
    void onTrackEdit( wxCommandEvent& aEvent ) override;
    void onPadstackModeChanged( wxCommandEvent& aEvent ) override;
    void onEditLayerChanged( wxCommandEvent& aEvent ) override;
    void onBackdrillChange( wxCommandEvent& aEvent ) override;
    void onTopPostMachineChange( wxCommandEvent& aEvent ) override;
    void onBottomPostMachineChange( wxCommandEvent& aEvent ) override;

    void onUnitsChanged( wxCommandEvent& aEvent );
    void onTeardropsUpdateUi( wxUpdateUIEvent& event ) override;

    bool confirmShortingNets( int aNet, const std::set<int>& shortingNets );
    bool confirmPadChange( const std::set<PAD*>& connectedPads );

    int getLayerDepth();
    void afterPadstackModeChanged();

    ///< Get data from the PCB board and display it to dialog
    bool TransferDataToWindow() override;

private:
    PCB_BASE_EDIT_FRAME* m_frame;
    const PCB_SELECTION& m_items;      // List of items to be modified.

    UNIT_BINDER          m_trackStartX, m_trackStartY;
    UNIT_BINDER          m_trackEndX, m_trackEndY;
    UNIT_BINDER          m_trackWidth;
    UNIT_BINDER          m_trackMaskMargin;

    UNIT_BINDER          m_viaX, m_viaY;
    UNIT_BINDER          m_viaDiameter, m_viaDrill;

    UNIT_BINDER          m_topPostMachineSize1Binder;
    UNIT_BINDER          m_topPostMachineSize2Binder;
    UNIT_BINDER          m_bottomPostMachineSize1Binder;
    UNIT_BINDER          m_bottomPostMachineSize2Binder;

    UNIT_BINDER          m_teardropHDPercent;
    UNIT_BINDER          m_teardropLenPercent;
    UNIT_BINDER          m_teardropMaxLen;
    UNIT_BINDER          m_teardropWidthPercent;
    UNIT_BINDER          m_teardropMaxWidth;

    bool                 m_tracks;     // True if dialog displays any track properties.
    bool                 m_vias;       // True if dialog displays any via properties.

    /// Temporary padstack of the edited via(s)
    std::unique_ptr<PADSTACK> m_viaStack;

    /// The currently-shown copper layer of the edited via(s)
    PCB_LAYER_ID m_editLayer;
    std::map<int, PCB_LAYER_ID> m_editLayerCtrlMap;
    bool                 m_backdrillStartIndeterminate;
    bool                 m_backdrillEndIndeterminate;
    bool                 m_padstackDirty;
};
