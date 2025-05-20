/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
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

#ifndef FOOTPRINT_CHOOSER_FRAME_H
#define FOOTPRINT_CHOOSER_FRAME_H


#include <wx/gdicmn.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <netlist_reader/pcb_netlist.h>
#include <lib_tree_model.h>
#include <3d_canvas/board_adapter.h>
#include <3d_rendering/track_ball.h>

class PANEL_FOOTPRINT_CHOOSER;
class wxCheckBox;
class BITMAP_BUTTON;
class BOARD;
class CAMERA;

namespace PCB { struct IFACE; }


class FOOTPRINT_CHOOSER_FRAME : public PCB_BASE_FRAME
{
public:
    ~FOOTPRINT_CHOOSER_FRAME();

    ///< @copydoc PCB_BASE_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;

    /**
     * @param aFootprint an optional FPID string to initialize the viewer with and to
     *                   return a selected footprint through.
     */
    bool ShowModal( wxString* aFootprint, wxWindow* aParent ) override;

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

    /**
     * Force the position of the dialog to a new position.  This mimics the DIALOG_SHIM
     * implementation.
     * @param aNewPosition is the new forced position
     */
    void SetPosition( const wxPoint& aNewPosition );

    bool Show( bool show ) override;

    // We don't have a status bar
    void SetStatusText( const wxString& aText, int aNumber = 0 ) override {}
    void UpdateStatusBar() override {}

protected:
    FOOTPRINT_CHOOSER_FRAME( KIWAY* aKiway, wxWindow* aParent );

private:
    bool filterByPinCount();
    bool filterByFPFilters();
    bool filterFootprint( LIB_TREE_NODE& aNode );
    void Show3DViewerFrame();

    void setupUIConditions() override;

    /// @copydoc PCB_BASE_FRAME::Update3DView
    void Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle = nullptr ) override;

    void OnPaint( wxPaintEvent& aEvent );
    void OnOK( wxCommandEvent& aEvent );

    void doCloseWindow() override;
    void closeFootprintChooser( wxCommandEvent& aEvent );

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;
    COLOR_SETTINGS* GetColorSettings( bool aForceRefresh ) const override;

    void toggleBottomSplit( wxCommandEvent& event );
    void on3DviewReq( wxCommandEvent& event );
    void onFpViewReq( wxCommandEvent& event );
    void onExternalViewer3DEnable( wxCommandEvent& aEvent );

    /**
     * Show hide footprint view panel and/or 3d view panel according to the options
     * (display 3D shapes and use external 3D viewer)
     */
    void updatePanelsVisibility();

    /**
     * Must be called after loading a new footprint: update footprint and/or 3D views
     */
    void updateViews();

    // A command event sent by a PANEL_FOOTPRINT_CHOOSER will fire this event:
    void onFpChanged( wxCommandEvent& event );

    void build3DCanvas();

    DECLARE_EVENT_TABLE()

    friend struct PCB::IFACE;       // constructor called from here only

private:
    PANEL_FOOTPRINT_CHOOSER* m_chooserPanel;
    wxPanel*                 m_bottomPanel;
    inline static bool       m_showDescription = true; // Init true to show the m_details panel
    inline static bool       m_showFpMode = true;  // Init true to show the footprint
    inline static bool       m_show3DMode = false; // Init false to hide the 3D model
    wxCheckBox*              m_filterByPinCount;
    wxCheckBox*              m_filterByFPFilters;
    wxCheckBox*              m_show3DViewer;

    BOARD_ADAPTER            m_boardAdapter;
    EDA_3D_CANVAS*           m_preview3DCanvas;
    CAMERA&                  m_currentCamera;
    TRACK_BALL               m_trackBallCamera;
    BITMAP_BUTTON*           m_toggleDescription;
    BITMAP_BUTTON*           m_grButtonFpView;
    BITMAP_BUTTON*           m_grButton3DView;

    int                                             m_pinCount;
    std::vector<std::unique_ptr<EDA_PATTERN_MATCH>> m_fpFilters;

    // On MacOS (at least) SetFocus() calls made in the constructor will fail because a
    // window that isn't yet visible will return false to AcceptsFocus().  So we must delay
    // the initial-focus SetFocus() call to the first paint event.
    bool                     m_firstPaintEvent;
};

#endif  // FOOTPRINT_CHOOSER_FRAME_H
