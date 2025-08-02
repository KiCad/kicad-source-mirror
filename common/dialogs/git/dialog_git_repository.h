/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_GIT_REPOSITORY_H_
#define DIALOG_GIT_REPOSITORY_H_

#include "dialog_git_repository_base.h"

#include <git/kicad_git_common.h>
#include <git2.h>

class DIALOG_GIT_REPOSITORY : public DIALOG_GIT_REPOSITORY_BASE
{
public:
    DIALOG_GIT_REPOSITORY( wxWindow* aParent, git_repository* aRepository,
                           wxString aURL = wxEmptyString );
    ~DIALOG_GIT_REPOSITORY() override;

    void SetRepoType( KIGIT_COMMON::GIT_CONN_TYPE aType )
    {
        m_ConnType->SetSelection( static_cast<int>( aType ) );
        updateAuthControls();
    }

    KIGIT_COMMON::GIT_CONN_TYPE GetRepoType() const
    {
        return static_cast<KIGIT_COMMON::GIT_CONN_TYPE>( m_ConnType->GetSelection() );
    }

    void     SetRepoName( const wxString& aName ) { m_txtName->SetValue( aName ); }
    wxString GetRepoName() const { return m_txtName->GetValue(); }

    void     SetRepoURL( const wxString& aURL ) { m_txtURL->SetValue( aURL ); }
    wxString GetRepoURL() const { return m_txtURL->GetValue(); }

    /**
     * @brief Get the Bare Repo U R L object
     *
     * @return wxString without the protocol
     */
    wxString GetBareRepoURL() const
    {
        wxString url = m_txtURL->GetValue();

        if( url.StartsWith( "https://" ) )
            url = url.Mid( 8 );
        else if( url.StartsWith( "http://" ) )
            url = url.Mid( 7 );
        else if( url.StartsWith( "ssh://" ) )
            url = url.Mid( 6 );

        return url;
    }

    const wxString& GetFullURL() const { return m_fullURL; }

    void     SetUsername( const wxString& aUsername ) { m_txtUsername->SetValue( aUsername ); }
    wxString GetUsername() const { return m_txtUsername->GetValue(); }

    void     SetPassword( const wxString& aPassword ) { m_txtPassword->SetValue( aPassword ); }
    wxString GetPassword() const { return m_txtPassword->GetValue(); }

    void     SetRepoSSHPath( const wxString& aPath ) { m_fpSSHKey->SetFileName( aPath ); m_prevFile = aPath; }
    wxString GetRepoSSHPath() const { return m_fpSSHKey->GetFileName().GetFullPath(); }

    void     SetEncrypted( bool aEncrypted = true );

    void     SetSkipButtonLabel( const wxString& aLabel )
    {
        if( m_sdbSizerCancel )
        {
            m_sdbSizerCancel->SetLabel( aLabel );
            m_sdbSizerCancel->Layout();
        }
    }

private:
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnLocationExit( wxFocusEvent& event ) override;
    void OnOKClick( wxCommandEvent& event ) override;

    void OnSelectConnType( wxCommandEvent& event ) override;
    void OnTestClick( wxCommandEvent& event ) override;

    void OnFileUpdated( wxFileDirPickerEvent& event ) override;
    void onCbCustom( wxCommandEvent& event ) override;

    void setDefaultSSHKey();

    void updateAuthControls();
    void updateURLData();
    bool extractClipboardData();

    std::tuple<bool,wxString,wxString,wxString> isValidHTTPS( const wxString& url );
    std::tuple<bool,wxString, wxString> isValidSSH( const wxString& url );

private:
    git_repository* m_repository;
    wxString        m_fullURL;

    wxString        m_prevFile;

    bool            m_tempRepo;
    wxString        m_tempPath;
};

#endif /* DIALOG_GIT_REPOSITORY_H_ */