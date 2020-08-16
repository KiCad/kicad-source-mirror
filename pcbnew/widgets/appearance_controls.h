/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _APPEARANCE_CONTROLS_H
#define _APPEARANCE_CONTROLS_H

#include <vector>

#include <class_board.h>
#include <gal/color4d.h>
#include <layers_id_colors_and_visibility.h>
#include <project/board_project_settings.h>
#include <widgets/appearance_controls_base.h>


class BITMAP_TOGGLE;
class COLOR_SWATCH;
class INDICATOR_ICON;
class PCB_BASE_FRAME;
class ROW_ICON_PROVIDER;

using KIGFX::COLOR4D;


class APPEARANCE_CONTROLS : public APPEARANCE_CONTROLS_BASE, public BOARD_LISTENER
{
public:

    /**
     * Container for an appearance setting (can control a single board layer, or GAL layer, etc)
     */
    struct APPEARANCE_SETTING
    {
        int      id;
        wxString label;
        wxString tooltip;
        bool     visible;
        bool     can_control_opacity;
        bool     spacer;

        wxPanel*        ctl_panel;
        INDICATOR_ICON* ctl_indicator;
        BITMAP_TOGGLE*  ctl_visibility;
        COLOR_SWATCH*   ctl_color;
        wxStaticText*   ctl_text;
        wxSlider*       ctl_opacity;

        APPEARANCE_SETTING( const wxString& aLabel, int aId,
                            const wxString& aTooltip = wxEmptyString,
                            bool aCanControlOpacity = false )
        {
            label   = aLabel;
            id      = aId;
            tooltip = aTooltip;
            spacer  = false;
            visible = true;

            can_control_opacity = aCanControlOpacity;

            ctl_panel      = nullptr;
            ctl_indicator  = nullptr;
            ctl_visibility = nullptr;
            ctl_color      = nullptr;
            ctl_text       = nullptr;
            ctl_opacity    = nullptr;
        }

        APPEARANCE_SETTING() :
                id( -1 ), label( "" ), tooltip( "" ), visible( false ),
                can_control_opacity( false ), spacer( true )
        {
        }
    };

    APPEARANCE_CONTROLS( PCB_BASE_FRAME* aParent, wxWindow* aFocusOwner, bool aFpEditor = false );
    ~APPEARANCE_CONTROLS();

    wxSize GetBestSize() const;

    ///> Updates the panel contents from the application and board models
    void OnBoardChanged();

    void OnBoardNetSettingsChanged( BOARD& aBoard ) override;

    void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;

    void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;

    void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;

    ///> Updates the colors on all the widgets from the new chosen color theme
    void OnColorThemeChanged();

    ///> Updates the widget when the active board layer is changed
    void OnLayerChanged();

    ///> Manually update visibility for a given layer
    void SetLayerVisible( LAYER_NUM aLayer, bool isVisible );

    void SetObjectVisible( GAL_LAYER_ID aLayer, bool isVisible = true );

    ///> Updates the manual layer alpha overrides
    void OnLayerAlphaChanged();

    void UpdateDisplayOptions();

    ///> Returns a list of the layer presets created by the user
    std::vector<LAYER_PRESET> GetUserLayerPresets() const;

    ///> Updates the current layer presets from those saved in the project file
    void SetUserLayerPresets( std::vector<LAYER_PRESET>& aPresetList );

    void ApplyLayerPreset( const wxString& aPresetName );

    void ApplyLayerPreset( const LAYER_PRESET& aPreset );

    wxString GetActiveLayerPreset() const
    {
        if( m_currentPreset )
            return m_currentPreset->name;
        else
            return wxEmptyString;
    }

    void OnColorSwatchChanged( wxCommandEvent& aEvent );

    void OnLayerContextMenu( wxCommandEvent& aEvent );

protected:

    void OnLayerDisplayPaneChanged( wxCollapsiblePaneEvent& event ) override;

    void OnNetDisplayPaneChanged( wxCollapsiblePaneEvent& event ) override;

    void OnNotebookPageChanged( wxNotebookEvent& event ) override;

private:
    PCB_BASE_FRAME* m_frame;

    static const APPEARANCE_SETTING s_objectSettings[];

    ROW_ICON_PROVIDER* m_iconProvider;

    BOARD* m_board;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_layerSettings;

    std::map<PCB_LAYER_ID, APPEARANCE_SETTING*> m_layerSettingsMap;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_objectSettings;

    std::map<GAL_LAYER_ID, APPEARANCE_SETTING*> m_objectSettingsMap;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_netSettings;

    std::map<int, APPEARANCE_SETTING*> m_netSettingsMap;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_netclassSettings;

    std::map<wxString, APPEARANCE_SETTING*> m_netclassSettingsMap;

    // TODO(JE) Move preset storage to the PCBNEW_CONTROL tool

    // Storage for all layer presets
    std::map<wxString, LAYER_PRESET> m_layerPresets;

    LAYER_PRESET* m_currentPreset;

    wxMenu* m_layerContextMenu;

    /// Stores wxIDs for each netclass for control event mapping
    std::map<int, wxString> m_netclassIdMap;

    /// The net code of the net that was right-clicked
    int m_contextMenuNetCode;

    /// The name of the netclass that was right-clicked
    wxString m_contextMenuNetclass;

    // The built-in layer presets

    static LAYER_PRESET presetNoLayers;
    static LAYER_PRESET presetAllLayers;
    static LAYER_PRESET presetAllCopper;
    static LAYER_PRESET presetInnerCopper;
    static LAYER_PRESET presetFront;
    static LAYER_PRESET presetFrontAssembly;
    static LAYER_PRESET presetBack;
    static LAYER_PRESET presetBackAssembly;

    int m_pointSize;

    enum POPUP_ID
    {
        ID_CHANGE_COLOR = wxID_HIGHEST,
        ID_SET_NET_COLOR,
        ID_CLEAR_NET_COLOR,
        ID_SHOW_ALL_NETS,
        ID_HIDE_OTHER_NETS,
        ID_HIGHLIGHT_NET,
        ID_SELECT_NET,
        ID_SHOW_ALL_COPPER_LAYERS,
        ID_HIDE_ALL_COPPER_LAYERS,
        ID_HIDE_ALL_BUT_ACTIVE,
        ID_PRESET_NO_LAYERS,
        ID_PRESET_ALL_LAYERS,
        ID_PRESET_FRONT,
        ID_PRESET_FRONT_ASSEMBLY,
        ID_PRESET_INNER_COPPER,
        ID_PRESET_BACK,
        ID_PRESET_BACK_ASSEMBLY,
        ID_HIDE_ALL_NON_COPPER,
        ID_SHOW_ALL_NON_COPPER,
        ID_LAST_VALUE
    };

    void rebuildLayers();

    void rebuildLayerContextMenu();

    void syncColorsAndVisibility();

    void rebuildObjects();

    void syncObjectSettings();

    void rebuildNets();

    void loadDefaultLayerPresets();

    void rebuildLayerPresetsWidget();

    void syncLayerPresetSelection();

    void onLayerClick( wxMouseEvent& aEvent );

    void onLayerVisibilityChanged( PCB_LAYER_ID aLayer, bool isVisible, bool isFinal );

    void onObjectVisibilityChanged( GAL_LAYER_ID aLayer, bool isVisible, bool isFinal );

    void onObjectOpacitySlider( int aLayer, float aOpacity );

    void updateLayerPresetSelection( const wxString& aName );

    void onLayerPresetChanged( wxCommandEvent& aEvent ) override;

    void doApplyLayerPreset( const LAYER_PRESET& aPreset );

    void onNetclassVisibilityChanged( wxCommandEvent& aEvent );

    void showNetclass( const wxString& aClassName, bool aShow = true );

    void onNetContextMenu( wxCommandEvent& aEvent );

    void onNetclassColorChanged( wxCommandEvent& aEvent );

    wxString netclassNameFromEvent( wxEvent& aEvent );

    void onNetColorModeChanged( wxCommandEvent& aEvent );

    void onNetclassContextMenu( wxCommandEvent& aEvent );

    void handleBoardItemsChanged();
};

#endif
