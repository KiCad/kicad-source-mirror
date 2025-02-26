/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jap.charras at wanadoo.fr
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

#ifndef FOOTPRINT_VIEWER_FRAME_H
#define FOOTPRINT_VIEWER_FRAME_H


#include <wx/gdicmn.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <netlist_reader/pcb_netlist.h>

class wxSashLayoutWindow;
class WX_LISTBOX;
class wxSearchCtrl;
class FP_LIB_TABLE;
class BOARD_ITEM;
class SELECTION;

namespace PCB { struct IFACE; }

enum class FPVIEWER_CONSTANTS
{
    NEW_PART      = 0,
    NEXT_PART     = 1,
    PREVIOUS_PART = 2,
    RELOAD_PART   = 3
};

/**
 * Component library viewer main window.
 */
class FOOTPRINT_VIEWER_FRAME : public PCB_BASE_FRAME
{
public:
    ~FOOTPRINT_VIEWER_FRAME();

    ///< @copydoc PCB_BASE_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;

    SELECTION& GetCurrentSelection() override;

    virtual COLOR4D GetGridColor() override;

    MAGNETIC_SETTINGS* GetMagneticItemsSettings() override
    {
        return &m_magneticItems;
    }

    /**
     * Create or recreate the list of current loaded libraries.
     *
     * This list is sorted, with the library cache always at end of the list
     */
    void ReCreateLibraryList();

    /**
     * Override from PCB_BASE_FRAME which reloads the footprint from the library without
     * setting the footprint watcher
     */
    void ReloadFootprint( FOOTPRINT* aFootprint ) override;

    /**
     * Export the current footprint name and close the library browser.
     */
    void AddFootprintToPCB();

    /**
     * Select and load the next or the previous footprint.
     *
     * If no current footprint, rebuild the list of footprints available in a given footprint
     * library.
     *
     * @param aMode #NEXT_PART or #PREVIOUS_PART.
     */
    void SelectAndViewFootprint( FPVIEWER_CONSTANTS aMode );

    ///< @copydoc EDADRAW_FRAME::UpdateMsgPanel
    void UpdateMsgPanel() override;

    COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const override;

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

    void HardRedraw() override;

protected:
    FOOTPRINT_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent );

    MAGNETIC_SETTINGS m_magneticItems;

    void setupUIConditions() override;

    void doReCreateMenuBar() override;

private:
    const wxString      getCurNickname();
    void                setCurNickname( const wxString& aNickname );

    const wxString      getCurFootprintName();
    void                setCurFootprintName( const wxString& aName );

    void OnSize( wxSizeEvent& event ) override;

    void ReCreateFootprintList();

    /**
     * Update the window title with current library information.
     */
    void UpdateTitle();

    void displayFootprint( FOOTPRINT* aFootprint );

    void doCloseWindow() override;
    void CloseFootprintViewer( wxCommandEvent& event );
    void OnExitKiCad( wxCommandEvent& event );

    void OnLibFilter( wxCommandEvent& aEvent );
    void OnFPFilter( wxCommandEvent& aEvent );
    void OnCharHook( wxKeyEvent& aEvent ) override;

    void selectPrev( WX_LISTBOX* aListBox );
    void selectNext( WX_LISTBOX* aListBox );
    void ClickOnLibList( wxCommandEvent& aEvent );
    void ClickOnFootprintList( wxCommandEvent& aEvent );
    void DClickOnFootprintList( wxMouseEvent& aEvent );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    void CommonSettingsChanged( int aFlags ) override;

    /**
     * Called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    void OnActivate( wxActivateEvent& event );

    /// @copydoc PCB_BASE_FRAME::Update3DView
    void Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle = nullptr ) override;

    void updateView();

    DECLARE_EVENT_TABLE()

    friend struct PCB::IFACE;       // constructor called from here only

private:
    wxSearchCtrl*       m_libFilter;
    WX_LISTBOX*         m_libList;        // The list of library names.
    int                 m_libListWidth;   // Last width of the window.

    wxSearchCtrl*       m_fpFilter;
    WX_LISTBOX*         m_fpList;         // The list of footprint names.
    int                 m_fpListWidth;    // Last width of the window.

    COMPONENT           m_comp;

    bool                m_autoZoom;
    double              m_lastZoom;
};

#endif  // FOOTPRINT_VIEWER_FRAME_H
