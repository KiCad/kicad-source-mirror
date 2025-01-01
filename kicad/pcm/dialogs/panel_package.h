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

#ifndef PANEL_PACKAGE_H_
#define PANEL_PACKAGE_H_

#include <functional>

#include "panel_package_base.h"
#include "pcm.h"

///< Collection of data relevant to the package display panel
struct PACKAGE_VIEW_DATA
{
    const PCM_PACKAGE package;
    wxBitmap*         bitmap;
    PCM_PACKAGE_STATE state;
    bool              pinned;
    wxString          repository_id;
    wxString          repository_name;
    wxString          current_version;
    wxString          update_version;
    PACKAGE_VIEW_DATA( const PCM_PACKAGE aPackage ) :
            package( std::move( aPackage ) ), bitmap( nullptr ), state( PPS_INSTALLED ),
            pinned( false ){};
    PACKAGE_VIEW_DATA( const PCM_INSTALLATION_ENTRY& aEntry ) :
            package( std::move( aEntry.package ) ), bitmap( nullptr )
    {
        state = PPS_INSTALLED;
        repository_id = aEntry.repository_id;
        repository_name = aEntry.repository_name;
        current_version = aEntry.current_version;
        pinned = aEntry.pinned;
    }
};


///< Callback for (un)install button
using ActionCallback = std::function<void( const PACKAGE_VIEW_DATA& aData,
                                           PCM_PACKAGE_ACTION aAction, const wxString& aVersion )>;

using PinCallback = std::function<void( const wxString& aPackageId, const PCM_PACKAGE_STATE aState,
                                        const bool aPinned )>;


class PANEL_PACKAGE : public PANEL_PACKAGE_BASE
{
public:
    PANEL_PACKAGE( wxWindow* parent, const ActionCallback& aCallback,
                   const PinCallback& aPinCallback, const PACKAGE_VIEW_DATA& aData );

    ///< Sets callback for OnClick action
    void SetSelectCallback( const std::function<void()>& aCallback );

    ///< Marks panel as selected
    void SetSelected( bool aSelected );

    void OnButtonClicked( wxCommandEvent& event ) override;

    ///< Changes state of the (un)install button
    void SetState( PCM_PACKAGE_STATE aState, bool aPinned );

    ///< Called when anywhere on the panel is clicked (except install button)
    void OnClick( wxMouseEvent& event ) override;

    void OnUninstallClick( wxCommandEvent& event );

    void OnPinVersionClick( wxCommandEvent& event );

    void OnSize( wxSizeEvent& event ) override;

    ///< Get preferred version. If criteria are not met, return wxEmptyString
    wxString GetPreferredVersion() const;

    const PACKAGE_VIEW_DATA& GetPackageData() const { return m_data; };

private:
    void OnPaint( wxPaintEvent& event ) override;

private:
    wxMenuItem*           m_pinVersionMenuItem;
    wxMenuItem*           m_actionMenuItem;
    std::function<void()> m_selectCallback;
    bool                  m_selected = false;
    const ActionCallback& m_actionCallback;
    const PinCallback&    m_pinCallback;
    PACKAGE_VIEW_DATA     m_data;
    int                   m_minHeight;
};


#endif // PANEL_PACKAGE_H_
