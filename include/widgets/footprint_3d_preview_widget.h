/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef FOOTPRINT_3D_PREVIEW_WIDGET_H
#define FOOTPRINT_3D_PREVIEW_WIDGET_H

#include <import_export.h>
#include <lib_id.h>

#include <wx/panel.h>

class KIWAY;
class wxStaticText;
class wxSizer;
class wxWindow;


enum class FOOTPRINT_3D_PREVIEW_STATUS
{
    DISPLAYED,
    FOOTPRINT_NOT_FOUND,
    NO_3D_MODEL
};


class FOOTPRINT_3D_PREVIEW_PANEL_BASE
{
public:
    virtual ~FOOTPRINT_3D_PREVIEW_PANEL_BASE() {}

    virtual FOOTPRINT_3D_PREVIEW_STATUS DisplayFootprint( LIB_ID const& aFPID ) = 0;
    virtual void ClearPreview() = 0;
    virtual void RefreshAll() = 0;
    virtual void Shutdown() = 0;
    virtual wxWindow* GetWindow() = 0;

    static FOOTPRINT_3D_PREVIEW_PANEL_BASE* Create( wxWindow* aParent, KIWAY& aKiway );
};


class FOOTPRINT_3D_PREVIEW_WIDGET : public wxPanel
{
public:
    FOOTPRINT_3D_PREVIEW_WIDGET( wxWindow* aParent, KIWAY& aKiway );

    bool IsInitialized() const { return m_prevPanel != nullptr; }

    void SetStatusText( const wxString& aText );
    void ClearStatus();
    void ClearPreview();
    void DisplayFootprint( const LIB_ID& aFPID );
    void RefreshAll();
    void Shutdown();

protected:
    FOOTPRINT_3D_PREVIEW_PANEL_BASE* m_prevPanel;

    wxStaticText*                    m_status;
    wxPanel*                         m_statusPanel;
    wxSizer*                         m_statusSizer;
    wxSizer*                         m_outerSizer;
    LIB_ID                           m_libid;
};

#endif // FOOTPRINT_3D_PREVIEW_WIDGET_H
