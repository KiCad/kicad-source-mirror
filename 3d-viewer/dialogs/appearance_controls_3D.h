/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef APPEARANCE_CONTROLS_3D_H
#define APPEARANCE_CONTROLS_3D_H

#include <vector>

#include <gal/color4d.h>
#include <layer_ids.h>
#include <3d_canvas/board_adapter.h>
#include <dialogs/appearance_controls_3D_base.h>


class BITMAP_TOGGLE;
class COLOR_SWATCH;
class EDA_3D_VIEWER_FRAME;
class ROW_ICON_PROVIDER;
class GRID_BITMAP_TOGGLE_RENDERER;

using KIGFX::COLOR4D;


class APPEARANCE_CONTROLS_3D : public APPEARANCE_CONTROLS_3D_BASE
{
public:

    /**
     * Container for an appearance setting (can control a layer class, object type, etc.)
     */
    struct APPEARANCE_SETTING_3D
    {
        int      id;
        wxString label;
        wxString tooltip;
        bool     visible;
        bool     spacer;

        BITMAP_TOGGLE*  ctl_visibility;
        COLOR_SWATCH*   ctl_color;

        APPEARANCE_SETTING_3D( const wxString& aLabel, int aId, const wxString& aTooltip ) :
                id( aId ),
                label( aLabel ),
                tooltip( aTooltip ),
                visible( true ),
                spacer( false ),
                ctl_visibility( nullptr ),
                ctl_color( nullptr )
        {
        }

        APPEARANCE_SETTING_3D() :
                id( -1 ),
                visible( false ),
                spacer( true ),
                ctl_visibility( nullptr ),
                ctl_color( nullptr )
        {
        }
    };

    APPEARANCE_CONTROLS_3D( EDA_3D_VIEWER_FRAME* aParent, wxWindow* aFocusOwner );
    ~APPEARANCE_CONTROLS_3D();

    wxSize GetBestSize() const;

    void OnLanguageChanged();
    void OnDarkModeToggle();
    void OnLayerVisibilityChanged( int aLayer, bool isVisible );

    void CommonSettingsChanged();

    void UpdateLayerCtls();

    void ApplyLayerPreset( const wxString& aPresetName );

    const wxArrayString& GetLayerPresetsMRU() { return m_presetMRU; }

    ///< Return a list of viewports created by the user.
    std::vector<VIEWPORT3D> GetUserViewports() const;

    ///< Update the current viewports from those saved in the project file.
    void SetUserViewports( std::vector<VIEWPORT3D>& aPresetList );

    void ApplyViewport( const wxString& aPresetName );

    const wxArrayString& GetViewportsMRU() { return m_viewportMRU; }

protected:
    void OnSetFocus( wxFocusEvent& aEvent ) override;
    void OnSize( wxSizeEvent& aEvent ) override;

private:
    void rebuildLayers();

    void rebuildLayerPresetsWidget();

    void syncLayerPresetSelection();

    void rebuildViewportsWidget();

    void onColorSwatchChanged( COLOR_SWATCH* aSwatch );

    void updateLayerPresetWidget( const wxString& aName );

    void onLayerPresetChanged( wxCommandEvent& aEvent ) override;

    void doApplyLayerPreset( const LAYER_PRESET_3D& aPreset );

    void onViewportChanged( wxCommandEvent& aEvent ) override;
    void onUpdateViewportsCb( wxUpdateUIEvent& aEvent ) override;

    void doApplyViewport( const VIEWPORT3D& aViewport );

    void passOnFocus();

private:
    EDA_3D_VIEWER_FRAME* m_frame;
    wxWindow*            m_focusOwner;

    static const APPEARANCE_SETTING_3D s_layerSettings[];

    GRID_BITMAP_TOGGLE_RENDERER* m_toggleGridRenderer;

    std::vector<std::unique_ptr<APPEARANCE_SETTING_3D>> m_layerSettings;
    std::map<int, APPEARANCE_SETTING_3D*>               m_layerSettingsMap;

    wxArrayString                       m_presetMRU;

    std::map<wxString, VIEWPORT3D>      m_viewports;
    VIEWPORT3D*                         m_lastSelectedViewport;
    wxArrayString                       m_viewportMRU;

    wxBoxSizer* m_layersOuterSizer;
    wxBoxSizer* m_envOuterSizer;
    int         m_pointSize;
    wxColour    m_layerPanelColour;
};

#endif
