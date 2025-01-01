/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_PACKAGES_VIEW_H_
#define PANEL_PACKAGES_VIEW_H_

#include "core/wx_stl_compat.h"
#include "panel_package.h"
#include "panel_packages_view_base.h"
#include "pcm.h"
#include "pcm_data.h"

#include <memory>
#include <unordered_map>
#include <vector>


class PANEL_PACKAGES_VIEW : public PANEL_PACKAGES_VIEW_BASE
{
public:
    PANEL_PACKAGES_VIEW( wxWindow* parent, std::shared_ptr<PLUGIN_CONTENT_MANAGER> aPcm,
                         const ActionCallback& aCallback, const PinCallback& aPinCallback );
    ~PANEL_PACKAGES_VIEW();

    /**
     * @brief Recreates package panels and displays data
     *
     * @param aPackageData list of package view data
     */
    void SetData( const std::vector<PACKAGE_VIEW_DATA>& aPackageData );

    /**
     * @brief Set the state of package
     *
     * @param aPackageId id of the package
     * @param aState new state
     * @param aPinned indicates pinned version
     */
    void SetPackageState( const wxString& aPackageId, const PCM_PACKAGE_STATE aState,
                          const bool aPinned );

    ///< Destroys package panels
    void ClearData();

    ///< Selects full row of the clicked cell
    void OnVersionsCellClicked( wxGridEvent& event ) override;

    ///< Opens file chooser dialog and downloads selected package version archive
    void OnDownloadVersionClicked( wxCommandEvent& event ) override;

    ///< Schedules relevant action for selected package version
    void OnVersionActionClicked( wxCommandEvent& event ) override;

    ///< Shows all versions including incompatible ones
    void OnShowAllVersionsClicked( wxCommandEvent& event ) override;

    ///< Ranks packages for entered search term and rearranges/hides panels according to their rank
    void OnSearchTextChanged( wxCommandEvent& event );

    void OnSizeInfoBox( wxSizeEvent& aEvent ) override;

    ///< Respond to a URL in the info window
    void OnURLClicked( wxHtmlLinkEvent& event ) override;

    ///< Respond to scrolling over the window
    void OnInfoMouseWheel( wxMouseEvent& event ) override;

    ///< Replacement of wxFormBuilder's ill-advised m_splitter1OnIdle
    void SetSashOnIdle( wxIdleEvent& );

    ///< Enqueues all available package updates
    void OnUpdateAllClicked( wxCommandEvent& event ) override;

private:
    ///< Updates package listing according to search term
    void updatePackageList();

    ///< Updates buttons below the package details: Download and Install
    void updateDetailsButtons();

    ///< Called when package state changes, currently used to calculate Update All button state
    void updateCommonState();

    ///< Updates details panel
    void setPackageDetails( const PACKAGE_VIEW_DATA& aPackageData );

    ///< Clears details panel
    void unsetPackageDetails();

    ///< Bytes to Kb/Mb/Gb string or "-" if absent
    wxString toHumanReadableSize( const std::optional<uint64_t> size ) const;

    ///< Returns true if it the download operation can be performed
    bool canDownload() const;

    ///< Returns true if the package action can be performed
    bool canRunAction() const;

    ///< Returns implied action for the action button
    PCM_PACKAGE_ACTION getAction() const;

private:
    const ActionCallback&                        m_actionCallback;
    const PinCallback&                           m_pinCallback;
    std::unordered_map<wxString, PANEL_PACKAGE*> m_packagePanels;
    std::vector<wxString>                        m_packageInitialOrder;
    PANEL_PACKAGE*                               m_currentSelected;
    std::unordered_set<wxString>                 m_updateablePackages;
    std::shared_ptr<PLUGIN_CONTENT_MANAGER>      m_pcm;

    enum PACKAGE_VERSIONS_GRID_COLUMNS
    {
        COL_VERSION = 0,
        COL_DOWNLOAD_SIZE,
        COL_INSTALL_SIZE,
        COL_COMPATIBILITY,
        COL_STATUS
    };

    static std::unordered_map<PCM_PACKAGE_VERSION_STATUS, wxString> STATUS_ENUM_TO_STR;
};

#endif // PANEL_PACKAGES_VIEW_H_
