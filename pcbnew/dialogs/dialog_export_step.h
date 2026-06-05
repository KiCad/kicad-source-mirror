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

#include <vector>

class BOARD;
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

    /**
     * Resolve the board file to hand to the external 3D/STEP exporter.
     *
     * The exporter runs as a child process that reads the board from disk.  When @p
     * aContentModified is true the live @p aBoard is serialized to a unique temporary .kicad_pcb
     * beside @p aBoardPath, with a sibling .kicad_pro copy so project-relative paths (3D models,
     * library tables) resolve identically, and that temporary path is returned through @p
     * aInputPath.  Otherwise @p aBoardPath is returned unchanged.  Any files that must be removed
     * once the export process exits are appended to @p aTempFiles.
     *
     * @param aBoardPath        the board's on-disk path
     * @param aContentModified  whether the board has unsaved modifications
     * @param aBoard            the live board, serialized when modified
     * @param aInputPath        [out] path to hand to the exporter on success
     * @param aTempFiles        [out] temporary files to delete after the export process exits
     * @param aErrorDetail      [out] secondary error text (e.g. an IO_ERROR message) on failure
     * @return an empty string on success, or a user-facing primary error message on failure
     */
    static wxString StageBoardForExport( const wxString& aBoardPath, bool aContentModified, BOARD* aBoard,
                                         wxString& aInputPath, std::vector<wxString>& aTempFiles,
                                         wxString& aErrorDetail );

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

    wxString getSelectedVariant() const;

private:
    PCB_EDIT_FRAME*    m_editFrame;
    JOB_EXPORT_PCB_3D* m_job;
    UNIT_BINDER        m_originX;
    UNIT_BINDER        m_originY;
    wxString           m_boardPath;      // path to the exported board file
};