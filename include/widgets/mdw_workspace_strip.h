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

#ifndef MDW_WORKSPACE_STRIP_H
#define MDW_WORKSPACE_STRIP_H

#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_subscription.h>

#include <memory>
#include <vector>

#include <wx/panel.h>

class EDA_BASE_FRAME;
class MDW_SESSION;
class wxButton;
class wxStaticText;
struct MDW_ADD_ITEM_REQUEST;
struct MDW_ITEM_INFO;
struct MDW_RESULT;


/**
 * The Design Workspace line at the top of the Version Control pane. It names the workspace the
 * project is bound to and its connection, offers Connect when unbound, and asks the client to
 * connect so a bound project connects when it loads. Hidden while the preference is off.
 */
class MDW_WORKSPACE_STRIP : public wxPanel
{
public:
    MDW_WORKSPACE_STRIP( wxWindow* aParent, EDA_BASE_FRAME* aFrame );

    ~MDW_WORKSPACE_STRIP() override;

    /// Follow the preference, the project binding and its session.
    void Sync();

    /// Open the workspace Set Up dialog for the project and apply what the user chose.
    void ShowSettings();

private:
    enum class ACTION
    {
        NONE,
        CONNECT,
        RESTART,
        SIGN_IN,
        VERIFY,
        LIBRARIES
    };

    /// The preference is on and the workspace integration is running
    bool available() const;

    void setAction( ACTION aAction );

    /// What the project's team libraries still need, shown beside the connection and acted on when it needs none
    void showLibraries();

    void onAction( wxCommandEvent& aEvent );
    void onMenu( wxCommandEvent& aEvent );

    std::shared_ptr<MDW_SESSION> session() const;

    /// Make the design files no checkout bound and the backend cannot locate into new items
    void addToWorkspace();
    void addItems( const std::vector<MDW_ADD_ITEM_REQUEST>& aRequests );

    /// Check out the files the binding names into the project folder, leaving any file already there
    void openFromWorkspace();
    void listDesign();
    void checkOutDesign( const MDW_RESULT& aStatus, const std::vector<MDW_ITEM_INFO>& aItems );

    EDA_BASE_FRAME* m_frame;
    wxString        m_projectPath;
    ACTION          m_action = ACTION::NONE;

    wxStaticText* m_title;
    wxStaticText* m_state;
    wxButton*     m_actionButton;
    wxButton*     m_menuButton;

    MDW_SUBSCRIPTION m_connections;
    MDW_OP           m_addLocate;
    MDW_OP           m_openLocate;
};

#endif // MDW_WORKSPACE_STRIP_H
