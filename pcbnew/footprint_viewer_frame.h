/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jap.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

class wxSashLayoutWindow;
class wxListBox;
class FP_LIB_TABLE;
class BOARD_ITEM;
class SELECTION;

namespace PCB { struct IFACE; }

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

    bool GetAutoZoom() override;
    void SetAutoZoom( bool aAutoZoom ) override;

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
     * Update the ID_ADD_FOOTPRINT_TO_BOARD tool state in main toolbar.
     */
    void OnUpdateFootprintButton(  wxUpdateUIEvent& aEvent );

    /**
     * Run the footprint viewer as a modal dialog.
     *
     * @param aFootprint an optional FPID string to initialize the viewer with and to
     *                   return a selected footprint through.
     */
    bool ShowModal( wxString* aFootprint, wxWindow* aParent ) override;

    COLOR_SETTINGS* GetColorSettings() const override;

protected:
    FOOTPRINT_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType );

    MAGNETIC_SETTINGS m_magneticItems;

    void setupUIConditions() override;

private:
    const wxString      getCurNickname();
    void                setCurNickname( const wxString& aNickname );

    const wxString      getCurFootprintName();
    void                setCurFootprintName( const wxString& aName );

    void OnSize( wxSizeEvent& event ) override;

    void ReCreateFootprintList();
    void OnIterateFootprintList( wxCommandEvent& event );

    /**
     * Update the window title with current library information.
     */
    void UpdateTitle();

    void doCloseWindow() override;
    void CloseFootprintViewer( wxCommandEvent& event );
    void OnExitKiCad( wxCommandEvent& event );

    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override;
    void ReCreateMenuBar() override;

    void OnLibFilter( wxCommandEvent& aEvent );
    void OnFPFilter( wxCommandEvent& aEvent );
    void OnCharHook( wxKeyEvent& aEvent ) override;

    void selectPrev( wxListBox* aListBox );
    void selectNext( wxListBox* aListBox );
    void ClickOnLibList( wxCommandEvent& aEvent );
    void ClickOnFootprintList( wxCommandEvent& aEvent );
    void DClickOnFootprintList( wxCommandEvent& aEvent );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    void CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged ) override;

    /**
     * Called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    void OnActivate( wxActivateEvent& event );

    /**
     * Export the current footprint name and close the library browser.
     */
    void AddFootprintToPCB( wxCommandEvent& aEvent );

    /**
     * Select and load the next or the previous footprint.
     *
     * If no current footprint, rebuild the list of footprints available in a given footprint
     * library.
     *
     * @param aMode #NEXT_PART or #PREVIOUS_PART.
     */
    void SelectAndViewFootprint( int aMode );

    /// @copydoc PCB_BASE_FRAME::Update3DView
    void Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle = nullptr ) override;

    void SaveCopyInUndoList( EDA_ITEM*, UNDO_REDO ) override {}
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST&, UNDO_REDO ) override {}

    void updateView();

    DECLARE_EVENT_TABLE()

    friend struct PCB::IFACE;       // constructor called from here only

    wxTextCtrl*         m_libFilter;
    wxListBox*          m_libList;        // The list of library names.
    wxTextCtrl*         m_fpFilter;
    wxListBox*          m_fpList;         // The list of footprint names.

    bool                m_autoZoom;
    double              m_lastZoom;
};

#endif  // FOOTPRINT_VIEWER_FRAME_H
