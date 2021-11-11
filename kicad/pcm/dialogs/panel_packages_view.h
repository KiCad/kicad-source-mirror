/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
    PANEL_PACKAGES_VIEW( wxWindow* parent, std::shared_ptr<PLUGIN_CONTENT_MANAGER> aPcm );
    ~PANEL_PACKAGES_VIEW();

    /**
     * @brief Recreates package panels and displays daya
     *
     * @param aPackageData list of package view data
     * @param aCallback (un)install button callback
     */
    void SetData( const std::vector<PACKAGE_VIEW_DATA>& aPackageData, ActionCallback aCallback );

    /**
     * @brief Set the state of package
     *
     * @param aPackageId id of the package
     * @param aState new state
     */
    void SetPackageState( const wxString& aPackageId, const PCM_PACKAGE_STATE aState ) const;

    ///< Destroys package panels
    void ClearData();

    ///< Selects full row of the clicked cell
    void OnVersionsCellClicked( wxGridEvent& event ) override;

    ///< Opens file chooser dialog and downloads selected package version archive
    void OnDownloadVersionClicked( wxCommandEvent& event ) override;

    ///< Schedules installation of selected package version
    void OnInstallVersionClicked( wxCommandEvent& event ) override;

    ///< Shows all versions including incompatible ones
    void OnShowAllVersionsClicked( wxCommandEvent& event ) override;

    ///< Ranks packages for entered search term and rearranges/hides panels according to their rank
    void OnSearchTextChanged( wxCommandEvent& event );

    void OnSizeInfoBox( wxSizeEvent& event ) override;

private:
    ///< Updates package listing according to search term
    void updatePackageList();

    ///< Updates details panel
    void setPackageDetails( const PACKAGE_VIEW_DATA& aPackageData );

    ///< Clears details panel
    void unsetPackageDetails();

    ///< Bytes to Kb/Mb/Gb string or "-" if absent
    wxString toHumanReadableSize( const boost::optional<uint64_t> size ) const;

private:
    ActionCallback                               m_actionCallback;
    std::unordered_map<wxString, PANEL_PACKAGE*> m_packagePanels;
    std::vector<wxString>                        m_packageInitialOrder;
    PANEL_PACKAGE*                               m_currentSelected;
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
