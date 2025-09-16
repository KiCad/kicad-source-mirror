/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#ifndef _APPEARANCE_CONTROLS_H
#define _APPEARANCE_CONTROLS_H

#include <vector>

#include <board.h>
#include <gal/color4d.h>
#include <layer_ids.h>
#include <project/board_project_settings.h>
#include <widgets/appearance_controls_base.h>


class BITMAP_TOGGLE;
class COLOR_SWATCH;
class INDICATOR_ICON;
class PCB_BASE_FRAME;
class PCBNEW_SETTINGS;
class ROW_ICON_PROVIDER;
class GRID_BITMAP_TOGGLE_RENDERER;
class WX_COLLAPSIBLE_PANE;
class wxStaticLine;
class wxSlider;
class wxRadioButton;

using KIGFX::COLOR4D;


struct NET_GRID_ENTRY
{
    NET_GRID_ENTRY( int aCode, const wxString& aName, const COLOR4D& aColor, bool aVisible )
    {
        code    = aCode;
        name    = aName;
        color   = aColor;
        visible = aVisible;
    }

    int      code;
    wxString name;
    COLOR4D  color;
    bool     visible;
};


class NET_GRID_TABLE : public wxGridTableBase
{
public:
    enum COLUMNS
    {
        COL_COLOR,
        COL_VISIBILITY,
        COL_LABEL,
        COL_SIZE
    };

    static void* ColorToVoid( COLOR4D& aColor )
    {
        return static_cast<void*>( &aColor );
    }

    static COLOR4D VoidToColor( void* aColor )
    {
        return *static_cast<COLOR4D*>( aColor );
    }

public:
    NET_GRID_TABLE( PCB_BASE_FRAME* aFrame, wxColor aBackgroundColor );
    ~NET_GRID_TABLE();

    int GetNumberRows() override
    {
        return m_nets.size();
    }

    int GetNumberCols() override
    {
        return COL_SIZE;
    }

    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind ) override;

    wxString GetValue( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;

    wxString GetTypeName( int aRow, int aCol ) override;

    bool GetValueAsBool( int aRow, int aCol ) override;

    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;

    void* GetValueAsCustom( int aRow, int aCol, const wxString& aTypeName ) override;

    void SetValueAsCustom( int aRow, int aCol, const wxString& aTypeName, void* aValue ) override;

    NET_GRID_ENTRY& GetEntry( int aRow );

    int GetRowByNetcode( int aCode ) const;

    void Rebuild();

    void ShowAllNets();

    void HideOtherNets( const NET_GRID_ENTRY& aNet );

private:
    void updateNetVisibility( const NET_GRID_ENTRY& aNet );

    void updateNetColor( const NET_GRID_ENTRY& aNet );

private:
    PCB_BASE_FRAME* m_frame;

    std::vector<NET_GRID_ENTRY> m_nets;

    wxGridCellAttr* m_defaultAttr;
    wxGridCellAttr* m_labelAttr;
};



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
        bool     can_control_visibility;
        bool     spacer;

        wxPanel*        ctl_panel;
        INDICATOR_ICON* ctl_indicator;
        BITMAP_TOGGLE*  ctl_visibility;
        COLOR_SWATCH*   ctl_color;
        wxStaticText*   ctl_text;
        wxSlider*       ctl_opacity;

        APPEARANCE_SETTING( const wxString& aLabel, int aId,
                            const wxString& aTooltip = wxEmptyString,
                            bool aCanControlOpacity = false,
                            bool aCanControlVisibility = true ) :
                id( aId ),
                label( aLabel ),
                tooltip( aTooltip ),
                visible( true ),
                can_control_opacity( aCanControlOpacity ),
                can_control_visibility( aCanControlVisibility ),
                spacer( false ),
                ctl_panel( nullptr ),
                ctl_indicator( nullptr ),
                ctl_visibility( nullptr ),
                ctl_color( nullptr ),
                ctl_text( nullptr ),
                ctl_opacity( nullptr )
        {
        }

        APPEARANCE_SETTING() :
                id( -1 ),
                visible( false ),
                can_control_opacity( false ),
                can_control_visibility( true ),
                spacer( true ),
                ctl_panel( nullptr ),
                ctl_indicator( nullptr ),
                ctl_visibility( nullptr ),
                ctl_color( nullptr ),
                ctl_text( nullptr ),
                ctl_opacity( nullptr )
        {
        }
    };

    APPEARANCE_CONTROLS( PCB_BASE_FRAME* aParent, wxWindow* aFocusOwner, bool aFpEditor = false );
    ~APPEARANCE_CONTROLS();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    APPEARANCE_CONTROLS( const APPEARANCE_CONTROLS& ) = delete;
    APPEARANCE_CONTROLS& operator=( const APPEARANCE_CONTROLS& ) = delete;

    wxSize GetBestSize() const;

    ///< Update the panel contents from the application and board models.
    void OnBoardChanged();

    void OnBoardNetSettingsChanged( BOARD& aBoard ) override;
    void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                 std::vector<BOARD_ITEM*>& aRemovedItems,
                                 std::vector<BOARD_ITEM*>& aChangedItems ) override;

    ///< Update the colors on all the widgets from the new chosen color theme.
    void OnColorThemeChanged();

    ///< Respond to change in OS's DarkMode
    void OnDarkModeToggle();

    ///< Update the widget when the active board layer is changed.
    void OnLayerChanged();

    /// Notifies the panel when a net has been hidden or shown via the external tool.
    void OnNetVisibilityChanged( int aNetCode, bool aVisibility );

    ///< Manually update visibility for a given layer
    void SetLayerVisible( int aLayer, bool isVisible );

    void SetObjectVisible( GAL_LAYER_ID aLayer, bool isVisible = true );

    void UpdateDisplayOptions();

    ///< Return a list of the layer presets created by the user.
    std::vector<LAYER_PRESET> GetUserLayerPresets() const;

    ///< Update the current layer presets from those saved in the project file.
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

    const wxArrayString& GetLayerPresetsMRU() { return m_presetMRU; }

    ///< Return a list of viewports created by the user.
    std::vector<VIEWPORT> GetUserViewports() const;

    ///< Update the current viewports from those saved in the project file.
    void SetUserViewports( std::vector<VIEWPORT>& aPresetList );

    void ApplyViewport( const wxString& aPresetName );

    void ApplyViewport( const VIEWPORT& aPreset );

    const wxArrayString& GetViewportsMRU() { return m_viewportMRU; }

    void OnColorSwatchChanged( wxCommandEvent& aEvent );

    void OnLayerContextMenu( wxCommandEvent& aEvent );

    ///< Return the index of the current tab (0-2).
    int GetTabIndex() const;

    ///< Set the current notebook tab.
    void SetTabIndex( int aTab );

    /**
     * Function to force a redraw of the collapsible panes in this control.
     */
    void RefreshCollapsiblePanes();

    bool IsLayerOptionsExpanded();
    bool IsNetOptionsExpanded();

    bool IsTogglingNetclassRatsnestVisibility();
    bool IsTogglingNetRatsnestVisibility();

protected:
    void OnNotebookPageChanged( wxNotebookEvent& event ) override;
    void OnSetFocus( wxFocusEvent& aEvent ) override;
    void OnSize( wxSizeEvent& aEvent ) override;
    void OnNetGridClick( wxGridEvent& event ) override;
    void OnNetGridDoubleClick( wxGridEvent& event ) override;
    void OnNetGridRightClick( wxGridEvent& event ) override;
    void OnNetGridMouseEvent( wxMouseEvent& aEvent );
    void OnLanguageChanged( wxCommandEvent& aEvent );

private:
    void createControls();

    void rebuildLayers();

    void rebuildLayerContextMenu();

    void syncColorsAndVisibility();

    void rebuildObjects();

    void syncObjectSettings();

    void buildNetClassMenu( wxMenu& aMenu, bool isDefaultClass, const wxString& aName );

    void rebuildNets();

    void loadDefaultLayerPresets();

    void rebuildLayerPresetsWidget( bool aReset );

    void syncLayerPresetSelection();

    void rebuildViewportsWidget();

    void onLayerLeftClick( wxMouseEvent& aEvent );

    void rightClickHandler( wxMouseEvent& aEvent );

    void onLayerVisibilityToggled( PCB_LAYER_ID aLayer );

    void onObjectVisibilityChanged( GAL_LAYER_ID aLayer, bool isVisible, bool isFinal );

    void setVisibleLayers( const LSET& aLayers );

    bool isLayerEnabled( PCB_LAYER_ID aLayer ) const;

    void setVisibleObjects( GAL_SET aObjects );

    LSET getVisibleLayers();

    GAL_SET getVisibleObjects();

    void onObjectOpacitySlider( int aLayer, float aOpacity );

    void updateLayerPresetSelection( const wxString& aName );

    void onLayerPresetChanged( wxCommandEvent& aEvent ) override;

    void doApplyLayerPreset( const LAYER_PRESET& aPreset );

    void updateViewportSelection( const wxString& aName );

    void onViewportChanged( wxCommandEvent& aEvent ) override;

    void doApplyViewport( const VIEWPORT& aViewport );

    void onNetclassVisibilityChanged( wxCommandEvent& aEvent );

    void showNetclass( const wxString& aClassName, bool aShow = true );

    void onNetContextMenu( wxCommandEvent& aEvent );

    void onNetclassColorChanged( wxCommandEvent& aEvent );

    wxString netclassNameFromEvent( wxEvent& aEvent );

    void onNetColorMode( wxCommandEvent& aEvent );

    void onRatsnestMode( wxCommandEvent& aEvent );

    void onNetclassContextMenu( wxCommandEvent& aEvent );

    void handleBoardItemsChanged();

    void passOnFocus();

    void idleFocusHandler( wxIdleEvent& aEvent );

    void onReadOnlySwatch();

    bool doesBoardItemNeedRebuild( BOARD_ITEM* aBoardItem );

    bool doesBoardItemNeedRebuild( std::vector<BOARD_ITEM*>& aBoardItems );

    PCB_BASE_FRAME* m_frame;

    wxWindow* m_focusOwner;

    static const APPEARANCE_SETTING s_objectSettings[];

    ROW_ICON_PROVIDER* m_iconProvider;

    BOARD* m_board;

    bool m_isFpEditor;

    // Nets grid view
    NET_GRID_TABLE* m_netsTable;

    GRID_BITMAP_TOGGLE_RENDERER* m_toggleGridRenderer;

    /// Grid cell that is being hovered over, for tooltips
    wxGridCellCoords m_hoveredCell;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_layerSettings;
    std::map<PCB_LAYER_ID, APPEARANCE_SETTING*>      m_layerSettingsMap;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_objectSettings;
    std::map<GAL_LAYER_ID, APPEARANCE_SETTING*>      m_objectSettingsMap;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_netclassSettings;
    std::map<wxString, APPEARANCE_SETTING*>          m_netclassSettingsMap;

    // TODO(JE) Move preset storage to the PCB_CONTROL tool

    std::map<wxString, LAYER_PRESET> m_layerPresets;
    LAYER_PRESET*                    m_currentPreset;
    LAYER_PRESET*                    m_lastSelectedUserPreset;
    wxArrayString                    m_presetMRU;

    std::map<wxString, VIEWPORT>     m_viewports;
    VIEWPORT*                        m_lastSelectedViewport;
    wxArrayString                    m_viewportMRU;

    wxMenu*                          m_layerContextMenu;

    /// Stores wxIDs for each netclass for control event mapping
    std::map<int, wxString> m_netclassIdMap;

    /// The name of the netclass that was right-clicked
    wxString m_contextMenuNetclass;

    wxBoxSizer* m_layersOuterSizer;
    wxBoxSizer* m_objectsOuterSizer;

    // The built-in layer presets
    static LAYER_PRESET presetNoLayers;
    static LAYER_PRESET presetAllLayers;
    static LAYER_PRESET presetAllCopper;
    static LAYER_PRESET presetInnerCopper;
    static LAYER_PRESET presetFront;
    static LAYER_PRESET presetFrontAssembly;
    static LAYER_PRESET presetBack;
    static LAYER_PRESET presetBackAssembly;
    // a LAYER_PRESET used only to store the objects visibility of the
    // last selected built-in LAYER_PRESET preset
    static LAYER_PRESET m_lastBuiltinPreset;

    int m_pointSize;

    wxColour m_layerPanelColour;

    // Layer display options controls
    WX_COLLAPSIBLE_PANE* m_paneLayerDisplayOptions;
    wxStaticText*        m_inactiveLayersLabel;
    wxRadioButton*       m_rbHighContrastNormal;
    wxRadioButton*       m_rbHighContrastDim;
    wxRadioButton*       m_rbHighContrastOff;
    wxStaticLine*        m_layerDisplaySeparator;
    wxCheckBox*          m_cbFlipBoard;

    // Net display options controls
    WX_COLLAPSIBLE_PANE* m_paneNetDisplayOptions;
    wxStaticText*        m_txtNetDisplayTitle;
    wxRadioButton*       m_rbNetColorAll;
    wxRadioButton*       m_rbNetColorRatsnest;
    wxRadioButton*       m_rbNetColorOff;
    wxStaticText*        m_txtRatsnestVisibility;
    wxRadioButton*       m_rbRatsnestAllLayers;
    wxRadioButton*       m_rbRatsnestVisLayers;
    wxRadioButton*       m_rbRatsnestNone;

    // Bitmap caches
    wxBitmapBundle      m_visibleBitmapBundle;
    wxBitmapBundle      m_notVisibileBitmapBundle;

    enum POPUP_ID
    {
        ID_CHANGE_COLOR = wxID_HIGHEST,
        ID_SET_NET_COLOR,
        ID_CLEAR_NET_COLOR,
        ID_USE_SCHEMATIC_NET_COLOR,
        ID_SHOW_ALL_NETS,
        ID_HIDE_OTHER_NETS,
        ID_HIGHLIGHT_NET,
        ID_SELECT_NET,
        ID_DESELECT_NET,
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

    bool m_togglingNetclassRatsnestVisibility;
};

#endif
