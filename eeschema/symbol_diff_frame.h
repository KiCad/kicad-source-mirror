/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SYMBOL_DIFF_FRAME_H_
#define __SYMBOL_DIFF_FRAME_H_

#include <sch_base_frame.h>

/**
 * Symbol library viewer main window.
 */
class SYMBOL_DIFF_FRAME : public SCH_BASE_FRAME
{
public:

    /**
     * @param aKiway
     * @param aParent is the parent frame of the viewer.
     * @param aFrameType must be either #FRAME_SCH_LIB_VIEWER or #FRAME_SCH_LIB_VIEWER_MODAL.
     * @param aLibrary is the library to open when starting (default = NULL).
     */
    SYMBOL_DIFF_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                         const wxString& aLibraryName = wxEmptyString );

    ~SYMBOL_DIFF_FRAME();

    /**
     * Runs the symbol viewer as a modal dialog.
     *
     * @param aSymbol an optional FPID string to initialize the viewer with and to
     *                return a selected footprint through.
     */
    bool ShowModal( wxString* aSymbol, wxWindow* aParent ) override;

    /**
     * Send the selected symbol back to the caller.
     */
    void FinishModal();

    void OnSize( wxSizeEvent& event ) override;

    void doCloseWindow() override;
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override {}

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    void CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged ) override;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    SELECTION& GetCurrentSelection() override;

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

protected:
    void setupUIConditions() override;

    void doReCreateMenuBar() override;

private:
    // Set up the tool framework.
    void setupTools();

    /**
     * Called when the frame is activated to reload the libraries and symbol lists
     * that can be changed by the schematic editor or the library editor.
     */
    void OnActivate( wxActivateEvent& event );


    DECLARE_EVENT_TABLE()
};
#endif