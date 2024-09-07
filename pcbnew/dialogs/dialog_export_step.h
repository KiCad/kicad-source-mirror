/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo
 * Copyright (C) 2016-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include "dialog_export_step_base.h"

class PCB_EDIT_FRAME;

class DIALOG_EXPORT_STEP : public DIALOG_EXPORT_STEP_BASE
{
public:
    enum STEP_ORIGIN_OPTION
    {
        STEP_ORIGIN_0,             // absolute coordinates
        STEP_ORIGIN_PLOT_AXIS,     // origin is plot/drill axis origin
        STEP_ORIGIN_GRID_AXIS,     // origin is grid origin
        STEP_ORIGIN_BOARD_CENTER,  // origin is board center
        STEP_ORIGIN_USER,          // origin is entered by user
    };

    DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aEditFrame, const wxString& aBoardPath );
    ~DIALOG_EXPORT_STEP();

protected:
    void onBrowseClicked( wxCommandEvent& aEvent ) override;
    void onUpdateUnits( wxUpdateUIEvent& aEvent ) override;
    void onUpdateXPos( wxUpdateUIEvent& aEvent ) override;
    void onUpdateYPos( wxUpdateUIEvent& aEvent ) override;
    void onExportButton( wxCommandEvent& aEvent ) override;
    void onFormatChoice( wxCommandEvent& event ) override;
    void onCbExportComponents( wxCommandEvent& event ) override;
    void OnComponentModeChange( wxCommandEvent& event ) override;

    int GetOrgUnitsChoice() const
    {
        return m_STEP_OrgUnitChoice->GetSelection();
    }

    double GetXOrg() const;

    double GetYOrg();

    STEP_ORIGIN_OPTION GetOriginOption();

    bool GetNoUnspecifiedOption()
    {
        return m_cbRemoveUnspecified->GetValue();
    }

    bool GetNoDNPOption()
    {
        return m_cbRemoveDNP->GetValue();
    }

    bool GetSubstOption()
    {
        return m_cbSubstModels->GetValue();
    }

    bool GetOverwriteFile()
    {
        return m_cbOverwriteFile->GetValue();
    }

    // Called to update filename extension after the output file format is changed
    void OnFmtChoiceOptionChanged();

private:
    enum class COMPONENT_MODE
    {
        EXPORT_ALL,
        EXPORT_SELECTED,
        CUSTOM_FILTER
    };

    PCB_EDIT_FRAME*    m_editFrame;
    STEP_ORIGIN_OPTION m_origin;         // The last preference for STEP origin option
    double             m_userOriginX;    // remember last User Origin X value
    double             m_userOriginY;    // remember last User Origin Y value
    int                m_originUnits;    // remember last units for User Origin
    bool               m_noUnspecified;  // remember last preference for No Unspecified Component
    bool               m_noDNP;          // remember last preference for No DNP Component
    static bool        m_optimizeStep;   // remember last preference for Optimize STEP file (stored only for the session)
    static bool        m_exportBoardBody;  // remember last preference to export board body (stored only for the session)
    static bool        m_exportComponents; // remember last preference to export components (stored only for the session)
    static bool        m_exportTracks;   // remember last preference to export tracks and vias (stored only for the session)
    static bool        m_exportPads;     // remember last preference to export pads (stored only for the session)
    static bool        m_exportZones;    // remember last preference to export zones (stored only for the session)
    static bool        m_exportInnerCopper; // remember last preference to export inner layers (stored only for the session)
    static bool        m_exportSilkscreen;  // remember last preference to export silkscreen (stored only for the session)
    static bool        m_exportSoldermask;  // remember last preference to export soldermask (stored only for the session)
    static bool        m_fuseShapes;     // remember last preference to fuse shapes (stored only for the session)
    wxString           m_netFilter;      // filter copper nets
    static wxString    m_componentFilter; // filter component reference designators
    static COMPONENT_MODE m_componentMode;
    wxString           m_boardPath;      // path to the exported board file
    static int         m_toleranceLastChoice;  // Store m_tolerance option during a session
    static int         m_formatLastChoice; // Store format option during a session
};