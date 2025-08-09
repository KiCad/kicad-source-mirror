/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo
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

#pragma once

#include "dialog_export_step_base.h"
#include <widgets/unit_binder.h>

class PCB_EDIT_FRAME;
class JOB_EXPORT_PCB_3D;

class DIALOG_EXPORT_STEP : public DIALOG_EXPORT_STEP_BASE
{
public:
    DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aEditFrame, const wxString& aBoardPath );
    DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent, const wxString& aBoardPath,
                        JOB_EXPORT_PCB_3D* aJob = nullptr );
    ~DIALOG_EXPORT_STEP() = default;

    bool TransferDataToWindow() override;

protected:
    void onBrowseClicked( wxCommandEvent& aEvent ) override;
    void onUpdateXPos( wxUpdateUIEvent& aEvent ) override;
    void onUpdateYPos( wxUpdateUIEvent& aEvent ) override;
    void onExportButton( wxCommandEvent& aEvent ) override;
    void onFormatChoice( wxCommandEvent& event ) override;
    void onCbExportComponents( wxCommandEvent& event ) override;
    void OnComponentModeChange( wxCommandEvent& event ) override;

    // Called to update filename extension after the output file format is changed
    void OnFmtChoiceOptionChanged();

private:
    PCB_EDIT_FRAME*    m_editFrame;
    JOB_EXPORT_PCB_3D* m_job;
    UNIT_BINDER        m_originX;
    UNIT_BINDER        m_originY;
    wxString           m_boardPath;      // path to the exported board file
};