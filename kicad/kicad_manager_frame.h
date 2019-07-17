/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_H
#define KICAD_H


#include <wx/process.h>
#include <eda_base_frame.h>
#include <kiway_player.h>

class TREEPROJECTFILES;
class TREE_PROJECT_FRAME;
class ACTION_TOOLBAR;

// Identify the type of files handled by Kicad manager
//
// When changing this enum  please verify (and perhaps update)
// TREE_PROJECT_FRAME::GetFileExt(),
// s_AllowedExtensionsToList[]

enum TreeFileType {
    TREE_ROOT = 0,
    TREE_PROJECT,
    TREE_SCHEMA,            // Schematic file (.sch)
    TREE_LEGACY_PCB,        // board file (.brd) legacy format
    TREE_SEXP_PCB,          // board file (.kicad_brd) new s expression format
    TREE_GERBER,            // Gerber  file (.pho, .g*)
    TREE_HTML,              // HTML file (.htm, *.html)
    TREE_PDF,               // PDF file (.pdf)
    TREE_TXT,               // ascii text file (.txt)
    TREE_NET,               // netlist file (.net)
    TREE_UNKNOWN,
    TREE_DIRECTORY,
    TREE_CMP_LINK,          // cmp/footprint link file (.cmp)
    TREE_REPORT,            // report file (.rpt)
    TREE_FP_PLACE,          // fooprints position (place) file (.pos)
    TREE_DRILL,             // Excellon drill file (.drl)
    TREE_DRILL_NC,          // Similar Excellon drill file (.nc)
    TREE_DRILL_XNC,         // Similar Excellon drill file (.xnc)
    TREE_SVG,               // SVG file (.svg)
    TREE_PAGE_LAYOUT_DESCR, // Page layout and title block descr file (.kicad_wks)
    TREE_FOOTPRINT_FILE,    // footprint file (.kicad_mod)
    TREE_SCHEMATIC_LIBFILE, // schematic library file (.lib)
    TREE_MAX
};


/**
 * The main KiCad project manager frame.  It is not a KIWAY_PLAYER.
 */
class KICAD_MANAGER_FRAME : public EDA_BASE_FRAME
{
public:
    KICAD_MANAGER_FRAME( wxWindow* parent, const wxString& title,
                         const wxPoint& pos, const wxSize& size );

    ~KICAD_MANAGER_FRAME();

    void OnCloseWindow( wxCloseEvent& Event );
    void OnSize( wxSizeEvent& event );

    void OnArchiveFiles( wxCommandEvent& event );
    void OnUnarchiveFiles( wxCommandEvent& event );

    void OnOpenFileInTextEditor( wxCommandEvent& event );
    void OnBrowseInFileExplorer( wxCommandEvent& event );

    void OnFileHistory( wxCommandEvent& event );
    void OnExit( wxCommandEvent& event );

    void ReCreateMenuBar() override;
    void RecreateBaseHToolbar();
    void RecreateLauncher();

    /**
     *  Open dialog to import Eagle schematic and board files.
     */
    void OnImportEagleFiles( wxCommandEvent& event );

    /**
     * Displays \a aText in the text panel.
     *
     * @param aText The text to display.
     */
    void PrintMsg( const wxString& aText );

    /**
     * Prints the current working directory name and the projet name on the text panel.
     */
    void PrintPrjInfo();

    /**
     * Erase the text panel.
     */
    void ClearMsg();

    void RefreshProjectTree();

    /** 
     * Creates a new project by setting up and initial project, schematic, and board files.
     *
     * The project file is copied from the kicad.pro template file if possible.  Otherwise,
     * a minimal project file is created from an empty project.  A minimal schematic and
     * board file are created to prevent the schematic and board editors from complaining.
     * If any of these files already exist, they are not overwritten.
     *
     * @param aProjectFileName is the absolute path of the project file name.
     */
    void CreateNewProject( const wxFileName& aProjectFileName );
    void LoadProject( const wxFileName& aProjectFileName );


    void LoadSettings( wxConfigBase* aCfg ) override;

    void SaveSettings( wxConfigBase* aCfg ) override;

    void ShowChangedLanguage() override;
    void CommonSettingsChanged( bool aEnvVarsChanged ) override;

    /**
     * Called by sending a event with id = ID_INIT_WATCHED_PATHS
     * rebuild the list of wahtched paths
     */
    void OnChangeWatchedPaths( wxCommandEvent& aEvent );

    void SyncToolbars() override;

    void InstallPreferences( PAGED_DIALOG* aParent, PANEL_HOTKEYS_EDITOR* aHotkeysPanel ) override;

    void SetProjectFileName( const wxString& aFullProjectProFileName );
    const wxString GetProjectFileName();

    // read only accessors
    const wxString SchFileName();
    const wxString PcbFileName();
    const wxString PcbLegacyFileName();

    void ReCreateTreePrj();

    DECLARE_EVENT_TABLE()

private:
    wxConfigBase* config() override;

    const SEARCH_STACK& sys_search() override;

    wxString help_name() override;

    void language_change( wxCommandEvent& event );

private:
    TREE_PROJECT_FRAME* m_leftWin;
    ACTION_TOOLBAR*     m_launcher;
    wxTextCtrl*         m_messagesBox;
    ACTION_TOOLBAR*     m_mainToolBar;

    int                 m_leftWinWidth;
    bool                m_active_project;
};


// The C++ project manager includes a single PROJECT in its link image.
class PROJECT;
extern PROJECT& Prj();

#endif
