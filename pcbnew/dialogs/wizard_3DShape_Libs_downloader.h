/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Code derived from "wizard_add_fplib.h" (from Maciej Suminski <maciej.suminski@cern.ch>)
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

#include <wizard_3DShape_Libs_downloader_base.h>

class KIWAY_PLAYER;

class WIZARD_3DSHAPE_LIBS_DOWNLOADER : public WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE
{
public:
    WIZARD_3DSHAPE_LIBS_DOWNLOADER( wxWindow* aParent );
    ~WIZARD_3DSHAPE_LIBS_DOWNLOADER();

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

    // Wizard event handlers
    void OnSourceCheck( wxCommandEvent& aEvent );
    void OnCheckGithubList( wxCommandEvent& aEvent );
    void OnPageChanged( wxWizardEvent& aEvent );
    void OnSelectAll3Dlibs( wxCommandEvent& aEvent );
    void OnUnselectAll3Dlibs( wxCommandEvent& aEvent );
    void OnChangeSearch( wxCommandEvent& aEvent );
    void OnWizardFinished( wxWizardEvent& aEvent );
    void OnBrowseButtonClick( wxCommandEvent& aEvent );
    void OnCheckSaveCopy( wxCommandEvent& aEvent );
	void OnDefault3DPathButtonClick( wxCommandEvent& event );
	void OnGridLibReviewSize( wxSizeEvent& event );
    void OnLocalFolderChange( wxCommandEvent& event );

protected:
    // Initialization of wizard pages
    void setupDialogOrder();
    void setupGithubList(); // Prepare the second page
                            // (list of available .3dshapes libraries on github)
    void setupReview();     // Prepare the last page

    ///> Sets the target directory for libraries downloaded from Github
    void setDownloadDir( const wxString& aDir )
    {
        m_downloadDir->SetValue( aDir );
    }

    ///> Gets the current target for downloaded libraries
    inline wxString getDownloadDir()
    {
        return m_downloadDir->GetValue();
    }

    ///> Downloads the list of Github libraries
    void getLibsListGithub( wxArrayString& aList );

    ///> Saves a list of Github libraries locally.
    bool downloadGithubLibsFromList( wxArrayString& aUrlList, wxString* aErrorMessage );

    ///> Saves a Github library aLibURL locally in aLocalLibName.
    bool downloadOneLib( const wxString& aLibURL,
                         const wxString& aLocalLibName,
                         wxProgressDialog * aIndicator,
                         wxString* aErrorMessage );

    ///> Enables Github widgets depending on the selected options.
    void updateGithubControls();

    ///> Enables/disable 'Next' button
    inline void enableNext( bool aEnable )
    {
        wxWindow* nextBtn = FindWindowById( wxID_FORWARD );

        if( nextBtn )
            nextBtn->Enable( aEnable );
    }

    // A callback function to filter 3D filenames
    static bool filter3dshapesfiles( const wxString& aData )
    {
        return aData.Contains( wxT( ".wrl" ) ) || aData.Contains( wxT( ".wings" ) );
    }

    // A callback function to filter 3D folders names
    static bool filter3dshapeslibraries( const wxString& aData )
    {
        return aData.Contains( wxT( ".3dshapes" ) );
    }

    ///> Cache for the downloaded Github library list
    wxArrayString m_githubLibs;

    ///> Libraries names selected in the wizard
    wxArrayString m_libraries;

    // Aliases for wizard pages to make code more readable
    wxWizardPageSimple* m_welcomeDlg;
    wxWizardPageSimple* m_githubListDlg;
    wxWizardPageSimple* m_reviewDlg;
};
