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

#ifndef DIALOG_WORKSPACE_CONFIG_H
#define DIALOG_WORKSPACE_CONFIG_H

#include <dialog_shim.h>
#include <git/kigit_remote_url.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_library_sources.h>

#include <memory>
#include <optional>
#include <vector>

#include <wx/string.h>

class MDW_SESSION;
class PROJECT;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxFilePickerCtrl;
class wxTextCtrl;
class wxStaticText;


/**
 * Bind the active project to a Design Workspace backend, with its workspace, binding blob and credentials. OK
 * returns the binding for the caller to save in the project file.
 */
class DIALOG_WORKSPACE_CONFIG : public DIALOG_SHIM
{
public:
    /**
     * aUsername is who the project signs in as, empty for the server's default account, and aSSHKey the private
     * key its git remote signs in with, empty for the default keys
     */
    DIALOG_WORKSPACE_CONFIG( wxWindow* aParent, const wxString& aProjectPath, const MDW_PROJECT_BINDING& aBinding,
                             const wxString& aUsername, const wxString& aSSHKey );

    ~DIALOG_WORKSPACE_CONFIG() override;

    /// Bound only when a backend was chosen
    const MDW_PROJECT_BINDING& GetBinding() const { return m_result; }

    wxString GetUsername() const;

    /// The private key the project's git remote signs in with, empty for the default keys, or none when not asked
    std::optional<wxString> GetSSHKey() const;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onTestConnection( wxCommandEvent& aEvent );
    void onFindProduct( wxCommandEvent& aEvent );

    /// Stop the backend a passing test left running, since what it was tested with may have changed
    void dropTested();

    /// Show the binding fields the chosen backend declares, or both when its manifest does not say
    void updateFields();

    MDW_PROJECT_BINDING collectBinding() const;

    /// Enable the sign-in fields the project's git remote uses, as the git remote dialog does
    void updateAuthControls();

    /// The key a sign-in sends, empty for the default keys
    wxString sshKeyFile() const;

    /// True, with the reason in the status line, when the picked key cannot be offered
    bool reportUnusableKey();

    wxString            m_projectPath;
    MDW_PROJECT_BINDING m_binding;
    MDW_PROJECT_BINDING m_result;
    wxString            m_username;

    wxChoice*         m_backendChoice;
    wxTextCtrl*       m_workspaceIdCtrl;
    wxTextCtrl*       m_bindingCtrl;
    wxCheckBox*       m_sshKeyCheck;
    wxFilePickerCtrl* m_sshKeyPicker;
    wxTextCtrl*       m_usernameCtrl;
    wxTextCtrl*       m_passwordCtrl;
    wxStaticText*     m_passwordLabel;
    wxStaticText*     m_statusText;
    wxStaticText*     m_workspaceIdLabel;
    wxStaticText*     m_bindingLabel;
    wxStaticText*     m_noFieldsNote;
    wxButton*         m_findProduct;

    /// The transport of the project's git remote when the backend signs in through it, else none
    std::optional<KIGIT::GIT_CONN_TYPE> m_remoteType;
    wxString                            m_remoteUsername; ///< The SSH username the remote's address names

    /// The session of the last passing test, which lists products without a second login
    std::shared_ptr<MDW_SESSION> m_tested;

    std::vector<MDW_BACKEND_CHOICE> m_backends; ///< Parallel to m_backendChoice entries
};


/**
 * MDW_SAVE_GUARD::RecordBinding() for aProject, then close any session opened under the old binding. An unbound
 * aBinding disconnects the project.
 *
 * @return false when the binding could not be saved.
 */
bool ApplyWorkspaceBinding( PROJECT& aProject, const wxString& aProjectPath, const MDW_PROJECT_BINDING& aBinding );

#endif // DIALOG_WORKSPACE_CONFIG_H
