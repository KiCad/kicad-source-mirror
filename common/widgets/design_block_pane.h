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
#ifndef DESIGN_BLOCK_PANE_H
#define DESIGN_BLOCK_PANE_H

#include <design_block_tree_model_adapter.h>
#include <widgets/html_window.h>
#include <widgets/wx_panel.h>
#include <wx/checkbox.h>
#include <wx/filedlgcustomize.h>
#include <eda_draw_frame.h>


class DESIGN_BLOCK;
class PANEL_DESIGN_BLOCK_CHOOSER;


class DESIGN_BLOCK_PANE : public WX_PANEL
{
public:
    DESIGN_BLOCK_PANE( EDA_DRAW_FRAME* aParent, const LIB_ID* aPreselect, std::vector<LIB_ID>& aHistoryList );

    ~DESIGN_BLOCK_PANE() override;

    virtual void OnLanguageChanged( wxCommandEvent& aEvent );

    void SaveSettings();

    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;
    void   SelectLibId( const LIB_ID& aLibId );

    /**
     * Load design block from design block library table.
     *
     * @param aLibId is the design block library identifier to load.
     * @param aUseCacheLib set to true to fall back to cache library if design block is not found in
     *                     design block library table.
     * @param aShowErrorMessage set to true to show any error messages.
     * @return The design block found in the library or NULL if the design block was not found.
     */
    DESIGN_BLOCK* GetDesignBlock( const LIB_ID& aLibId, bool aUseCacheLib, bool aShowErrorMsg );
    DESIGN_BLOCK* GetSelectedDesignBlock( bool aUseCacheLib, bool aShowErrorMsg );

    void RefreshLibs();

    /**
     * If a library name is given, creates a new design block library in the project folder
     * with the given name. If no library name is given it prompts user for a library path,
     * then creates a new design block library at that location.
     * If library exists, user is warned about that, and is given a chance
     * to abort the new creation, and in that case existing library is first deleted.
     *
     * @param aProposedName is the initial path and filename shown in the file chooser dialog.
     * @return The newly created library path if library was successfully created, else
     *         wxEmptyString because user aborted or error.
     */
    wxString CreateNewDesignBlockLibrary( const wxString& aLibName = wxEmptyString,
                                          const wxString& aProposedName = wxEmptyString );

    /**
     * Add an existing library to either the global or project library table.
     *
     * @param aFileName the library to add; a file open dialog will be displayed if empty.
     * @return true if successfully added.
     */
    bool AddDesignBlockLibrary( const wxString& aFilename, DESIGN_BLOCK_LIB_TABLE* aTable );

    bool DeleteDesignBlockLibrary( const wxString& aLibName, bool aConfirm );

    bool DeleteDesignBlockFromLibrary( const LIB_ID& aLibId, bool aConfirm );

    bool EditDesignBlockProperties( const LIB_ID& aLibId );

    PANEL_DESIGN_BLOCK_CHOOSER* GetDesignBlockPanel() const { return m_chooserPanel; }

protected:
    virtual void setLabelsAndTooltips() = 0;

    EDA_DRAW_FRAME*             m_frame = nullptr;
    PANEL_DESIGN_BLOCK_CHOOSER* m_chooserPanel = nullptr;

private:
    bool checkOverwrite( wxWindow* aFrame, wxString& libname, wxString& newName );
    DESIGN_BLOCK_LIB_TABLE* selectDesignBlockLibTable( bool aOptional = false );
    wxString createNewDesignBlockLibrary( const wxString& aLibName, const wxString& aProposedName,
                                          DESIGN_BLOCK_LIB_TABLE* aTable );
};
#endif
