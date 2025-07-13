/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#ifndef SYMBOL_CHOOSER_FRAME_H
#define SYMBOL_CHOOSER_FRAME_H

#include <sch_base_frame.h>

class PANEL_SYMBOL_CHOOSER;


/**
 * Symbol library viewer main window.
 */
class SYMBOL_CHOOSER_FRAME : public SCH_BASE_FRAME
{
public:
    /**
     * @param aKiway
     * @param aParent is the parent frame of the viewer.
     * @param aFrameType must be either #FRAME_SCH_LIB_VIEWER or #FRAME_SCH_LIB_VIEWER_MODAL.
     * @param aLibrary is the library to open when starting (default = NULL).
     */
    SYMBOL_CHOOSER_FRAME( KIWAY* aKiway, wxWindow* aParent, bool& aCancelled );

    ~SYMBOL_CHOOSER_FRAME();

    /**
     * Runs the symbol viewer as a modal dialog.
     *
     * @param aSymbol an optional FPID string to initialize the viewer with and to
     *                return a selected footprint through.
     */
    bool ShowModal( wxString* aSymbol, wxWindow* aParent ) override;

    /**
     * Set a filter to display only libraries and/or symbols which match the filter.
     *
     * @param aFilter is a filter to pass the allowed library name list and/or some other filter
     *                see SCH_BASE_FRAME::SelectSymbolFromLibrary() for details.
     *                if aFilter == NULL, remove all filtering.
     */
    void SetFilter( std::function<bool( LIB_TREE_NODE& aNode )>* aFilter );

    EDA_DRAW_PANEL_GAL* GetPreviewCanvas() const;

private:
    void OnPaint( wxPaintEvent& aEvent );
    void OnOK( wxCommandEvent& aEvent );

    void doCloseWindow() override;
    void CloseSymbolChooser( wxCommandEvent& aEvent );

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    DECLARE_EVENT_TABLE()

private:
    PANEL_SYMBOL_CHOOSER* m_chooserPanel;

    // On MacOS (at least) SetFocus() calls made in the constructor will fail because a
    // window that isn't yet visible will return false to AcceptsFocus().  So we must delay
    // the initial-focus SetFocus() call to the first paint event.
    bool                  m_firstPaintEvent;
};

#endif  // SYMBOL_CHOOSER_FRAME_H

