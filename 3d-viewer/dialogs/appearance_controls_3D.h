/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <vector>

#include <gal/color4d.h>
#include <layer_ids.h>
#include <3d_canvas/board_adapter.h>
#include <dialogs/appearance_controls_3D_base.h>
#include <tool/tool_action.h>
#include <wx/intl.h>


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
    class APPEARANCE_SETTING_3D
    {
    public:
        int  m_Id;
        bool m_Visible;
        bool m_Spacer;

        BITMAP_TOGGLE* m_Ctl_visibility;
        COLOR_SWATCH*  m_Ctl_color;

        APPEARANCE_SETTING_3D( const wxString& aLabel, int aId, const wxString& aTooltip ) :
                m_Id( aId ),
                m_Visible( true ),
                m_Spacer( false ),
                m_Ctl_visibility( nullptr ),
                m_Ctl_color( nullptr ),
                m_tooltip( aTooltip ),
                m_label( aLabel )
        {
        }

        APPEARANCE_SETTING_3D( const wxString& aLabel, int aId, const TOOL_ACTION& aAction ) :
                m_Id( aId ),
                m_Visible( true ),
                m_Spacer( false ),
                m_Ctl_visibility( nullptr ),
                m_Ctl_color( nullptr ),
                m_label( aLabel ),
                m_action( &aAction )
        {
        }

        APPEARANCE_SETTING_3D() :
                m_Id( -1 ),
                m_Visible( false ),
                m_Spacer( true ),
                m_Ctl_visibility( nullptr ),
                m_Ctl_color( nullptr )
        {
        }

        wxString GetTooltip() const
        {
            if( m_tooltip.has_value() )
                return wxGetTranslation( m_tooltip.value() );
            else if( m_action.has_value() )
                return m_action.value()->GetTooltip( true );
            else
                return wxEmptyString;
        }

        wxString GetLabel() const
        {
            return wxGetTranslation( m_label );
        }

    private:
        wxString m_label;

        std::optional<wxString>           m_tooltip;
        std::optional<const TOOL_ACTION*> m_action;
    };

    APPEARANCE_CONTROLS_3D( EDA_3D_VIEWER_FRAME* aParent, wxWindow* aFocusOwner );
    ~APPEARANCE_CONTROLS_3D();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    APPEARANCE_CONTROLS_3D( const APPEARANCE_CONTROLS_3D& ) = delete;
    APPEARANCE_CONTROLS_3D& operator=( const APPEARANCE_CONTROLS_3D& ) = delete;

    wxSize GetBestSize() const;
    void OnDarkModeToggle();
    void OnLayerVisibilityChanged( int aLayer, bool isVisible );

    void CommonSettingsChanged();

    void UpdateLayerCtls();

    void ApplyLayerPreset( const wxString& aPresetName );

    const wxArrayString& GetLayerPresetsMRU() { return m_presetMRU; }

    /// Return a list of viewports created by the user.
    std::vector<VIEWPORT3D> GetUserViewports() const;

    /// Update the current viewports from those saved in the project file.
    void SetUserViewports( std::vector<VIEWPORT3D>& aPresetList );

    void ApplyViewport( const wxString& aPresetName );

    const wxArrayString& GetViewportsMRU() { return m_viewportMRU; }

protected:
    void OnSetFocus( wxFocusEvent& aEvent ) override;
    void OnSize( wxSizeEvent& aEvent ) override;
    void OnLanguageChanged( wxCommandEvent& aEvent );

private:
    void rebuildControls();

    void rebuildLayers();

    void rebuildLayerPresetsWidget( bool aReset );

    void syncLayerPresetSelection();

    void rebuildViewportsWidget();

    void onColorSwatchChanged( COLOR_SWATCH* aSwatch );

    void updateLayerPresetWidget( const wxString& aName );

    void onLayerPresetChanged( wxCommandEvent& aEvent ) override;

    void doApplyLayerPreset( const wxString& aPresetName );

    void onViewportChanged( wxCommandEvent& aEvent ) override;
    void onUpdateViewportsCb( wxUpdateUIEvent& aEvent ) override;

    void doApplyViewport( const VIEWPORT3D& aViewport );

    void passOnFocus();

private:
    static const APPEARANCE_SETTING_3D s_layerSettings[];

    EDA_3D_VIEWER_FRAME*           m_frame;
    wxWindow*                      m_focusOwner;

    std::vector<std::unique_ptr<APPEARANCE_SETTING_3D>> m_layerSettings;
    std::map<int, APPEARANCE_SETTING_3D*>               m_layerSettingsMap;

    wxArrayString                  m_presetMRU;

    std::map<wxString, VIEWPORT3D> m_viewports;
    VIEWPORT3D*                    m_lastSelectedViewport;
    wxArrayString                  m_viewportMRU;

    wxBoxSizer*                    m_layersOuterSizer;
    wxBoxSizer*                    m_envOuterSizer;
    int                            m_pointSize;
    wxColour                       m_layerPanelColour;
    GRID_BITMAP_TOGGLE_RENDERER*   m_toggleGridRenderer;
    wxCheckBox*                    m_cbUseBoardStackupColors;
    wxCheckBox*                    m_cbUseBoardEditorCopperColors;
};
