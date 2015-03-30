/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2014-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wizard_add_fplib_base.h>
#include <io_mgr.h>
#include <boost/optional.hpp>

class KIWAY_PLAYER;

class WIZARD_FPLIB_TABLE : public WIZARD_FPLIB_TABLE_BASE
{
public:
    WIZARD_FPLIB_TABLE( wxWindow* aParent );
    ~WIZARD_FPLIB_TABLE();

    ///> Source of the libraries (local files or Github)
    enum LIB_SOURCE { LOCAL, GITHUB };

    ///> Scope (global/project)
    enum LIB_SCOPE { GLOBAL = 1, PROJECT = 2 };

    /**
     * Function GetFirstPage
     * Returns the welcoming page for the wizard.
     */
    inline wxWizardPage* GetFirstPage() const
    {
        return m_welcomeDlg;
    }

    /**
     * Function GetGithubURL
     * Returns the current Github repository URL set in the wizard.
     */
    inline wxString GetGithubURL() const
    {
        return m_textCtrlGithubURL->GetValue();
    }

    /**
     * Function SetGithubURL
     * Sets the current Github repository URL used by the wizard.
     * @param aUrl is the new URL to be applied.
     */
    inline void SetGithubURL( const wxString& aUrl )
    {
        m_textCtrlGithubURL->SetValue( aUrl );
    }

    /**
     * Function GetLibSource
     * Returns the source of libraries (local / Github).
     */
    LIB_SOURCE GetLibSource() const;

    /**
     * Function GetLibScope
     * Returns the scope for the added libraries (global / project specific).
     */
    LIB_SCOPE GetLibScope() const;

    // Wizard event handlers
    void OnSourceCheck( wxCommandEvent& aEvent );
    void OnSelectFiles( wxCommandEvent& aEvent );
    void OnCheckGithubList( wxCommandEvent& aEvent );
    void OnPageChanged( wxWizardEvent& aEvent );
    void OnSelectAllGH( wxCommandEvent& aEvent );
    void OnUnselectAllGH( wxCommandEvent& aEvent );
    void OnChangeSearch( wxCommandEvent& aEvent );
    void OnWizardFinished( wxWizardEvent& aEvent );
    void OnBrowseButtonClick( wxCommandEvent& aEvent );
    void OnCheckSaveCopy( wxCommandEvent& aEvent );

    class LIBRARY
    {
    public:
        LIBRARY( const wxString& aPath, const wxString& aDescription = wxEmptyString );
        ~LIBRARY()
        {
            std::cout << "destroyed " << this << std::endl;
        }

        ///> Possible states of validation.
        enum STATUS { OK, INVALID, NOT_CHECKED };

        /**
         * Function Test
         * Uses the associated plugin to validate the library contents.
         * @return True if the library and the matched plugin type are valid.
         */
        bool Test();

        /**
         * Function GetPluginType
         * Returns the plugin type, autodetected basing on the path.
         * @return Returns empty boost::optional if the type could not be autodetected.
         */
        inline boost::optional<IO_MGR::PCB_FILE_T> GetPluginType() const
        {
            return m_plugin;
        }

        /**
         * Function GetPluginName
         * Returns the plugin name, as used in the FPLIB table editor.
         */
        wxString GetPluginName() const;

        /**
         * Function GetAbsolutePath
         * Returns the absolute path for the library.
         */
        inline const wxString& GetAbsolutePath() const
        {
            return m_path;
        }

        /**
         * Function GetRelativePath
         * Returns the relative path, based on the input path with the base part replaced.
         * @param aBase is the base for the relative path.
         * @param aSubstitution is the string to be replace the base path.
         * @return Adjusted path if possible, or the absolute path when it is not possible.
         */
        wxString GetRelativePath( const wxString& aBase, const wxString& aSubstitution = wxEmptyString ) const;

        /**
         * Function GetAutoPath
         * Returns path that is either absolute or related to KISYSMOD/KIPRJMOD if the files
         * are stored within one of the mentioned paths.
         * @param aScoep is the scope for the library. It determines the environmental variables
         * that are used to check the path (GLOBAL scope checks only KISYSMOD, while PROJECT
         * scope checks both KISYSMOD & KIPRJMOD).
         */
        wxString GetAutoPath( LIB_SCOPE aScope ) const;

        /**
         * Function GetDescription
         * Returns the description for the library. If it is not specified in the constructor,
         * it is just the filename.
         */
        wxString GetDescription() const;

        /**
         * Function GetStatus
         * Returns the validity status for the library. It requires running Test() before, to get
         * a result different than NOT_CHECKED.
         */
        STATUS GetStatus() const
        {
            return m_status;
        }

    protected:
        inline void setPath( const wxString& aPath )
        {
            m_path = aPath;
        }

        inline void setPluginType( IO_MGR::PCB_FILE_T aType )
        {
            m_plugin = aType;
        }

        /**
         * Function replaceEnv
         * replaces path with environmental variable if applicable.
         * @param aEnvVar is the environmental variable that should be substituted.
         * @param aFilePath determines if the path is a file path (contrary to e.g. http address).
         */
        wxString replaceEnv( const wxString& aEnvVar, bool aFilePath = true ) const;

        wxString m_path;
        wxString m_description;
        boost::optional<IO_MGR::PCB_FILE_T> m_plugin;
        STATUS m_status;

        friend class WIZARD_FPLIB_TABLE;
    };

    /**
     * Function GetLibraries
     * Returns libraries selected by the user.
     */
    const std::vector<LIBRARY>& GetLibraries() const
    {
        return m_libraries;
    }


protected:
    // Initialization of wizard pages
    void setupDialogOrder();
    void setupGithubList();
    void setupFileSelect();
    void setupReview();

    ///> Checks the selection in file picker
    bool checkFiles() const;

    ///> Sets the target directory for libraries downloaded from Github
    void setDownloadDir( const wxString& aDir )
    {
        m_downloadDir->SetLabel( aDir );
    }

    ///> Gets the current target for downloaded libraries
    inline wxString getDownloadDir()
    {
        return m_downloadDir->GetLabel();
    }

    ///> Downloads the list of Github libraries
    void getLibsListGithub( wxArrayString& aList );

    ///> Saves a list of Github libraries locally.
    bool downloadGithubLibsFromList( wxArrayString& aUrlList, wxString* aErrorMessage );

    ///> Does the user want a local copy of Github libraries?
    inline bool wantLocalCopy() const { return m_downloadGithub->GetValue(); }

    ///> Enables Github widgets depending on the selected options.
    void updateGithubControls();

    ///> Updates m_libraries basing on dialogs contents
    void updateLibraries();

    ///> Enables/disable 'Next' button
    inline void enableNext( bool aEnable )
    {
        wxWindow* nextBtn = FindWindowById( wxID_FORWARD );

        if( nextBtn )
            nextBtn->Enable( aEnable );
    }

    ///> Cache for the downloaded Github library list
    wxArrayString m_githubLibs;

    ///> Libraries selected in the wizard
    std::vector<LIBRARY> m_libraries;

    // Aliases for wizard pages to make code more readable
    wxWizardPageSimple* const m_welcomeDlg;
    wxWizardPageSimple* const m_fileSelectDlg;
    wxWizardPageSimple* const m_githubListDlg;
    wxWizardPageSimple* const m_reviewDlg;
    wxWizardPageSimple* const m_targetDlg;

    // Since the same event is used for changing the selection/filter type, we need a way to
    // determine what's the real cause of the event. Therefore here we store the number of the
    // selected file type filter.
    int m_selectedFilter;
};
