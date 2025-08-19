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

class PCB_SELECTION;
class PCB_BASE_EDIT_FRAME;
class PAD;
class PADSTACK;

class DIALOG_TRACK_VIA_PROPERTIES : public DIALOG_TRACK_VIA_PROPERTIES_BASE
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


    enum class IPC4761_SURFACE
    {
        FROM_RULES = 0,
        NONE = 1,
        FRONT = 2,
        BACK = 3,
        BOTH = 4,
        CUSTOM = 5
    };

    enum class IPC4761_DRILL
    {
        FROM_RULES = 0,
        NOT_SET = 1,
        SET = 2
    };

    enum class IPC4761_PRESET
    {
        FROM_RULES = 0,
        NONE = 1,
        IA = 2,
        IB = 3,
        IA_INVERTED = 4,
        IIA = 5,
        IIB = 6,
        IIA_INVERTED = 7,
        IIIA = 8,
        IIIB = 9,
        IIIA_INVERTED = 10,
        IVA = 11,
        IVB = 12,
        IVA_INVERTED = 13,
        V = 14,
        VIA = 15,
        VIB = 16,
        VIA_INVERTED = 17,
        VII = 18,
        CUSTOM = 19,
        END = 20
    };

    struct IPC4761_CONFIGURATION
    {
        IPC4761_SURFACE tent{ IPC4761_SURFACE::NONE };
        IPC4761_SURFACE cover{ IPC4761_SURFACE::NONE };
        IPC4761_SURFACE plug{ IPC4761_SURFACE::NONE };
        IPC4761_DRILL   fill{ IPC4761_DRILL::NOT_SET };
        IPC4761_DRILL   cap{ IPC4761_DRILL::NOT_SET };

        bool operator==( const IPC4761_CONFIGURATION& other ) const;
    };

    const std::map<IPC4761_PRESET, IPC4761_CONFIGURATION> m_IPC4761Presets{
        { IPC4761_PRESET::FROM_RULES,
          { IPC4761_SURFACE::FROM_RULES, IPC4761_SURFACE::FROM_RULES, IPC4761_SURFACE::FROM_RULES,
            IPC4761_DRILL::FROM_RULES, IPC4761_DRILL::FROM_RULES } },
        { IPC4761_PRESET::NONE, {} },
        { IPC4761_PRESET::IA, { .tent = IPC4761_SURFACE::FRONT } },
        { IPC4761_PRESET::IB, { .tent = IPC4761_SURFACE::BOTH } },
        { IPC4761_PRESET::IA_INVERTED, { .tent = IPC4761_SURFACE::BACK } },
        { IPC4761_PRESET::IIA,
          { .tent = IPC4761_SURFACE::FRONT, .cover = IPC4761_SURFACE::FRONT } },
        { IPC4761_PRESET::IIB, { .tent = IPC4761_SURFACE::BOTH, .cover = IPC4761_SURFACE::BOTH } },
        { IPC4761_PRESET::IIA_INVERTED,
          { .tent = IPC4761_SURFACE::BACK, .cover = IPC4761_SURFACE::BACK } },
        { IPC4761_PRESET::IIIA, { .plug = IPC4761_SURFACE::FRONT } },
        { IPC4761_PRESET::IIIB, { .plug = IPC4761_SURFACE::BOTH } },
        { IPC4761_PRESET::IIIA_INVERTED, { .plug = IPC4761_SURFACE::BACK } },
        { IPC4761_PRESET::IVA, { .tent = IPC4761_SURFACE::FRONT, .plug = IPC4761_SURFACE::FRONT } },
        { IPC4761_PRESET::IVB, { .tent = IPC4761_SURFACE::BOTH, .plug = IPC4761_SURFACE::BOTH } },
        { IPC4761_PRESET::IVA_INVERTED,
          { .tent = IPC4761_SURFACE::BACK, .plug = IPC4761_SURFACE::BACK } },
        { IPC4761_PRESET::V, { .fill = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::VIA, { .tent = IPC4761_SURFACE::FRONT, .fill = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::VIB, { .tent = IPC4761_SURFACE::BOTH, .fill = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::VIA_INVERTED,
          { .tent = IPC4761_SURFACE::BACK, .fill = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::VII, { .fill = IPC4761_DRILL::SET, .cap = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::CUSTOM, {} },
        { IPC4761_PRESET::END, {} }
    };

    const std::map<IPC4761_PRESET, wxString> m_IPC4761Names{
        { IPC4761_PRESET::FROM_RULES, _( "From rules" ) },
        { IPC4761_PRESET::NONE, _( "None" ) },
        { IPC4761_PRESET::IA, _( "Type I a ( tented top )" ) },
        { IPC4761_PRESET::IB, _( "Type I b ( tented both sides )" ) },
        { IPC4761_PRESET::IA_INVERTED, _( "Type I a ( tented bottom )" ) },
        { IPC4761_PRESET::IIA, _( "Type II a ( covered and tented top )" ) },
        { IPC4761_PRESET::IIB, _( "Type II b ( covered and tented both sides )" ) },
        { IPC4761_PRESET::IIA_INVERTED, _( "Type II a ( covered and tented bottom )" ) },
        { IPC4761_PRESET::IIIA, _( "Type III a ( plugged top )" ) },
        { IPC4761_PRESET::IIIB, _( "Type III b ( plugged both sides )" ) },
        { IPC4761_PRESET::IIIA_INVERTED, _( "Type III a ( plugged bottom )" ) },
        { IPC4761_PRESET::IVA, _( "Type IV a ( plugged and tented top )" ) },
        { IPC4761_PRESET::IVB, _( "Type IV b ( plugged and tented both sides )" ) },
        { IPC4761_PRESET::IVA_INVERTED, _( "Type IV a ( plugged and tented bottom )" ) },
        { IPC4761_PRESET::V, _( "Type V ( filled )" ) },
        { IPC4761_PRESET::VIA, _( "Type VI a ( filled and tented top )" ) },
        { IPC4761_PRESET::VIB, _( "Type VI b ( filled and tented both sides )" ) },
        { IPC4761_PRESET::VIA_INVERTED, _( "Type VI a ( filled and tented bottom )" ) },
        { IPC4761_PRESET::VII, _( "Type VII ( filled and capped )" ) },
        { IPC4761_PRESET::CUSTOM, _( "Custom" ) },
        { IPC4761_PRESET::END, _( "End" ) }
    };
};
