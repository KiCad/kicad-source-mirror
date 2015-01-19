/**
 * @file wizard_add_fplib.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/wx.h>
#include <dialog_helpers.h>
#include <wizard_add_fplib_base.h>

// A helper class to handle the different types of lib depending
// on the plugin: ext, type of lib: files/folders ... and info
// needed to populate the main fp lib table
class LIB_DESCR
{
public:
    wxString m_PluginName;  // The "official" name of the plugin (see fp lib table dialog)
    wxString m_Ext;         // standard extension (.mod, .pretty ...)
    wxString m_EnvVarName;  // the environment var if selected, or empty
    wxString m_DefaultPath;
    bool     m_IsAbsolutePath;  // true if absolue path is selected
    bool     m_IsFile;      // true for libs which are single files,
                            // false for libs which are directories containing footprints
    bool     m_IsGitHub;    // true only for GitHub plugin

    LIB_DESCR()
    {
        m_IsAbsolutePath = true;
        m_IsFile = true;
        m_IsGitHub = false;
    }
};


class WIZARD_FPLIB_TABLE : public WIZARD_FPLIB_TABLE_BASE
{
    int m_rowPrjEnvVarPosition;     // the row of the PROJECT_VAR_NAME
    int m_predefinedEnvVarCnt;      // number of predefined env var when calling the wizard
                                    // at least 3 are always defined
    LIB_DESCR * m_currLibDescr;

    // static members to store options during a session
    static int m_last_plugin_choice;
    static int m_last_defaultpath_choice;

    // This enum must have the same order than m_rbPathManagement
    enum OPT_PATH {
        PROJECT_PATH,
        ENV_VAR_PATH,
        ABSOLUTE_PATH
    };

    // This enum must have the same order than m_rbFpLibFormat
    enum OPT_PLUGIN {
        KICAD_PLUGIN,
        GITHUB_PLUGIN,
        LEGACY_PLUGIN,
        EAGLE_PLUGIN,
        GEDA_PCB_PLUGIN
    };

public:
    WIZARD_FPLIB_TABLE( wxWindow* aParent, wxArrayString& aEnvVariableList );
    wxWizardPage* GetFirstPage() { return m_pages[0]; }

    ~WIZARD_FPLIB_TABLE();

    /**
     * Return info on lib at line aIdx in aLibDescr
     * @param aLibDescr = a wxArrayString to return the nickname, the lib URI and the lin type
     * @return true if aIdx lin exists
     */
    bool GetLibDescr( int aIdx, wxArrayString& aLibDescr )
    {
        int count = m_gridFpListLibs->GetTable()->GetRowsCount();

        if( aIdx >= count )
            return false;

        // Return info
        // Add the nickname:
        aLibDescr.Add( m_gridFpListLibs->GetCellValue( aIdx, 0 ) );
        // Add the full path:
        aLibDescr.Add( m_gridFpListLibs->GetCellValue( aIdx, 1 ) );
        // Add the plugin name:
        aLibDescr.Add( m_gridFpListLibs->GetCellValue( aIdx, 2 ) );

        return true;
    }

private:
    void initDlg( wxArrayString& aEnvVariableList );
    wxString getSelectedEnvVar();       // return the selected env variable
    wxString getSelectedEnvVarValue();  // return the selected env variable value
    bool setSecondPage();               // Init prms for the second wizard page
    bool setLastPage();                 // Init prms for the last wizard page
    void selectLibsFiles();             // select a set of library files
    void selectLibsFolders();           // select a set of library folders

    /** select a set of library on Github, using the Web viewer to explore
     * the repos
     */
    void selectLibsGithubWithWebViewer();

    /** Get the list of .pretty libraries on Github,
     * without using the viewer, from the lib list extracted from the KiCad repos
     */
    void getLibsListGithub( wxArrayString& aList );

    /** Helper function.
     * add the .pretty libraries found in aUrlList, after calculating a nickname and
     * replacing the path by an env variable, if allowed and possible
     */
    void installGithubLibsFromList( wxArrayString& aUrlList );

    /**
     * Download the .pretty libraries found in aUrlLis and store them on disk
     * in a master folder
     * @return true if OK, false on error
     * @param aUrlList is the list of Github .pretty libs to download
     * @param aErrorMessage is a wxString pointer to store error messages if any.
     */
    bool downloadGithubLibsFromList( wxArrayString& aUrlList, wxString * aErrorMessage = NULL );

    void updateFromPlugingChoice();     // update dialog options and widgets
                                        // depending on the plugin choice
    int GetEnvVarCount()                // Get the number of rows in env var table
    {
        return m_gridEnvironmentVariablesList->GetTable()->GetRowsCount();
    }

    int GetLibsCount()                // Get the number of rows in libs table
    {
        return m_gridFpListLibs->GetTable()->GetRowsCount();
    }

    bool IsGithubPlugin()               // Helper funct, return true if
    {                                   // the Github plugin is the choice
        return m_rbFpLibFormat->GetSelection() == GITHUB_PLUGIN;
    }


    bool IsKicadPlugin()                // Helper funct, return true if
    {                                   // the Kicad plugin is the choice
        return m_rbFpLibFormat->GetSelection() == KICAD_PLUGIN;
    }

    int HasGithubEnvVarCompatible();    // Return the first index to one env var
                                        // which defines a url compatible github
                                        // or -1 if not found

    // Populate the library list with the currently selected libs
    void populateLibList( const wxArrayString& aNickNames,
                          const wxArrayString& aPaths,
                          const wxString&      aPluginName );

    // Virtual event functions, from WIZARD_FPLIB_TABLE_BASE
    void OnFinish( wxWizardEvent& event ) { event.Skip(); }
    void OnPageChanged( wxWizardEvent& event );
    void OnPageChanging( wxWizardEvent& event );
    void OnAddEVariable( wxCommandEvent& event );
    void OnRemoveEVariable( wxCommandEvent& event );
    void OnAddFpLibs( wxCommandEvent& event );
    void OnRemoveFpLibs( wxCommandEvent& event );
    void OnPathManagementSelection( wxCommandEvent& event );
    void OnSelectEnvVarCell( wxGridEvent& event );
	void OnPluginSelection( wxCommandEvent& event );
#ifdef BUILD_GITHUB_PLUGIN
    void OnGithubLibsList( wxCommandEvent& event );
#endif
	bool ValidateOptions();
};


// Specialized helper classes to handle the different plugin types:
class LIB_DESCR_KICAD: public LIB_DESCR
{
public:
    LIB_DESCR_KICAD(): LIB_DESCR()
    {
        m_PluginName = IO_MGR::ShowType( IO_MGR::KICAD );
        m_Ext = wxT("pretty");
        m_IsFile = false;
    }
};


class LIB_DESCR_GITHUB: public LIB_DESCR
{
public:
    LIB_DESCR_GITHUB(): LIB_DESCR()
    {
        m_PluginName = IO_MGR::ShowType( IO_MGR::GITHUB );
        m_Ext = wxT("pretty");
        m_IsFile = false;
        m_IsGitHub = true;
    }
};

class LIB_DESCR_LEGACY: public LIB_DESCR
{
public:
    LIB_DESCR_LEGACY(): LIB_DESCR()
    {
        m_PluginName = IO_MGR::ShowType( IO_MGR::LEGACY );
        m_Ext = wxT("mod");
    }
};


class LIB_DESCR_EAGLE: public LIB_DESCR
{
public:
    LIB_DESCR_EAGLE(): LIB_DESCR()
    {
        m_PluginName = IO_MGR::ShowType( IO_MGR::EAGLE );
        m_Ext = wxT("lbr");
        m_IsFile = true;
    }
};


class LIB_DESCR_GEDA: public LIB_DESCR
{
public:
    // No specific extension known for folders
    LIB_DESCR_GEDA(): LIB_DESCR()
    {
        m_PluginName = IO_MGR::ShowType( IO_MGR::GEDA_PCB );
        m_IsFile = false;
    }
};

