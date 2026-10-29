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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

    wxString GetRepoName() const { return m_txtName->GetValue(); }

    void     SetRepoURL( const wxString& aURL ) { m_txtURL->SetValue( aURL ); }
    wxString GetRepoURL() const { return m_txtURL->GetValue(); }

    void     SetUsername( const wxString& aUsername ) { m_txtUsername->SetValue( aUsername ); }

    wxString GetUsername() const
    {
        KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( GetRepoURL() );
        return parsed.username.IsEmpty() ? m_txtUsername->GetValue() : parsed.username;
    }

    void     SetPassword( const wxString& aPassword ) { m_txtPassword->SetValue( aPassword ); }
    wxString GetPassword() const { return m_txtPassword->GetValue(); }

    void SetRepoSSHPath( const wxString& aPath ) { m_fpSSHKey->SetFileName( aPath ); }
    /// The private key, also when its .pub was picked.
    wxString GetRepoSSHPath() const
    {
        wxString path = m_fpSSHKey->GetFileName().GetFullPath();
        return path.ends_with( wxS( ".pub" ) ) ? path.Left( path.size() - 4 ) : path;
    }

    void     SetEncrypted( bool aEncrypted = true );

    void     SetSkipButtonLabel( const wxString& aLabel )
    {
        if( m_sdbSizerCancel )
        {
            m_sdbSizerCancel->SetLabel( aLabel );
            m_sdbSizerCancel->Layout();
        }
    }

protected:
    virtual bool TransferDataToWindow() override;

private:
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnLocationExit( wxFocusEvent& event ) override;
    void OnOKClick( wxCommandEvent& event ) override;

    void OnSelectConnType( wxCommandEvent& event ) override;
    void OnTestClick( wxCommandEvent& event ) override;

    void OnFileUpdated( wxFileDirPickerEvent& event ) override;
    void onCbCustom( wxCommandEvent& event ) override;

    void setDefaultSSHKey( const wxString& aHost );

    /// Show why the chosen SSH key cannot be used. False when there is no such problem.
    bool reportUnusableKey();

    void updateAuthControls();
    void updateURLData();
    bool extractClipboardData();

    git_repository* m_repository;
    wxString        m_incomeURL;

    bool            m_tempRepo;
    wxString        m_tempPath;
};

#endif /* DIALOG_GIT_REPOSITORY_H_ */
