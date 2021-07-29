/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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
#include <core/optional.h>
#include <layer_ids.h>

class PCB_SELECTION;
class COMMIT;
class PCB_BASE_FRAME;
class PAD;

class DIALOG_TRACK_VIA_PROPERTIES : public DIALOG_TRACK_VIA_PROPERTIES_BASE
{
public:
    DIALOG_TRACK_VIA_PROPERTIES( PCB_BASE_FRAME* aParent, const PCB_SELECTION& aItems,
                                 COMMIT& aCommit );

    bool TransferDataFromWindow() override;

private:
    void onViaNotFreeClicked( wxCommandEvent& event ) override;
    void onTrackNetclassCheck( wxCommandEvent& aEvent ) override;
    void onWidthSelect( wxCommandEvent& aEvent ) override;
    void onWidthEdit( wxCommandEvent& aEvent ) override;
    void onViaNetclassCheck( wxCommandEvent& aEvent ) override;
    void onViaSelect( wxCommandEvent& aEvent ) override;
    void onViaEdit( wxCommandEvent& aEvent ) override;

    bool confirmPadChange( const std::vector<PAD*>& connectedPads );

    PCB_BASE_FRAME*      m_frame;
    const PCB_SELECTION& m_items;      // List of items to be modified.
    COMMIT&              m_commit;     // An undo record to add any changes to.

    UNIT_BINDER          m_trackStartX, m_trackStartY;
    UNIT_BINDER          m_trackEndX, m_trackEndY;
    UNIT_BINDER          m_trackWidth;

    UNIT_BINDER          m_viaX, m_viaY;
    UNIT_BINDER          m_viaDiameter, m_viaDrill;

    bool                 m_tracks;     // True if dialog displays any track properties.
    bool                 m_vias;       // True if dialog displays any via properties.
};
