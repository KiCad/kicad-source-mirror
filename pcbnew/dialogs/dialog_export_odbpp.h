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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "dialog_export_odbpp_base.h"
#include <jobs/job_export_pcb_odb.h>
#include <pcb_io/odbpp/odb_export_job.h>

class PCB_EDIT_FRAME;

class DIALOG_EXPORT_ODBPP : public DIALOG_EXPORT_ODBPP_BASE
{
public:
    DIALOG_EXPORT_ODBPP( PCB_EDIT_FRAME* aParent );
    DIALOG_EXPORT_ODBPP( JOB_EXPORT_PCB_ODB* aJob, PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent );
    ~DIALOG_EXPORT_ODBPP() override;

private:
    void onBrowseClicked( wxCommandEvent& event ) override;
    void onFormatChoice( wxCommandEvent& event ) override;
    void onOKClick( wxCommandEvent& event ) override;

    void OnFmtChoiceOptionChanged();
    void updatePrecisionRange();
    void updateVariantFilename();
    void refreshLayerRows();
    /// Fold the grid's edits for the rows on screen into the layer overrides
    void harvestLayerOverrides();
    void setupControls();
    void populateJob( JOB_EXPORT_PCB_ODB& aJob ) const;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    PCB_EDIT_FRAME*     m_parent;
    JOB_EXPORT_PCB_ODB* m_job;
    std::vector<ODB_LAYER_OVERRIDE>     m_layerOverrides;
    std::vector<ODB_MATRIX_PREVIEW_ROW> m_previewRows;
    bool                                m_autoVariantSuffix = false;
};
