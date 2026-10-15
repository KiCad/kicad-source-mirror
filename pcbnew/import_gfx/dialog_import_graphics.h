/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <widgets/unit_binder.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <import_gfx/dialog_import_graphics_base.h>
#include <import_gfx/graphics_importer_pcbnew.h>

class GRAPHICS_IMPORT_MGR;


class DIALOG_IMPORT_GRAPHICS : public DIALOG_IMPORT_GRAPHICS_BASE
{
public:
    DIALOG_IMPORT_GRAPHICS( PCB_BASE_FRAME* aParent );

    /**
     * @return a list of items imported from a vector graphics file.
     */
    std::list<std::unique_ptr<EDA_ITEM>>& GetImportedItems() { return m_importer->GetItems(); }

    /**
     * @return true if the placement is interactive, i.e. all imported
     * items must be moved by the mouse cursor to the final position
     * false means the imported items are placed to the final position after import.
     */
    bool IsPlacementInteractive() { return !m_placeAtCheckbox->GetValue(); }

    /**
     * @return true if discontinuities in the shapes should be fixed.
     */
    bool ShouldFixDiscontinuities() { return m_rbFixDiscontinuities->GetValue(); }

    /**
     * @return tolerance to connect the shapes.
     */
    int GetTolerance() { return m_tolerance.GetIntValue(); }

    /**
     * @return true if imported items must be placed in a new PCB_GROUP.
     */
    bool ShouldGroupItems() { return m_cbGroupItems->IsChecked(); }

    /**
     * Set the filename override to be applied in TransferDataToWindow.
     */
    void SetFilenameOverride( const wxString& aFilenameOverride );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    // Virtual event handlers
    void onBrowseFiles( wxCommandEvent& event ) override;
    void onFilenameChanged( wxCommandEvent& event ) override;
    void onThresholdChanged( wxScrollEvent& event ) override;
    void onInvertChanged( wxCommandEvent& event ) override;
    void onScaleChanged( wxCommandEvent& event ) override;
    void onWidthChanged( wxCommandEvent& event ) override;
    void onHeightChanged( wxCommandEvent& event ) override;
    void onLockAspectChanged( wxCommandEvent& event ) override;
    void onUpdateUI( wxUpdateUIEvent& event ) override;

    /// Read the chosen file, which is where the preview and the source size come from.
    void loadSourceFile();

    /// Show the image controls only when the chosen file is a raster image.
    void updateImageControls();

    void updatePreview();

    /// Offer the size fields only when the source has a size to scale.
    void updateSizeControls();

    /// Write the scale into the size fields.
    void sizeFromScale();

    /// Work the scale out from a size the user typed.
    void scaleFromSize( bool aFromWidth );

private:
    PCB_BASE_FRAME*                           m_parent;
    std::unique_ptr<GRAPHICS_IMPORTER_PCBNEW> m_importer;
    std::unique_ptr<GRAPHICS_IMPORT_MGR>      m_gfxImportMgr;

    UNIT_BINDER          m_xOrigin;
    UNIT_BINDER          m_yOrigin;
    UNIT_BINDER          m_defaultLineWidth;
    UNIT_BINDER          m_tolerance;
    UNIT_BINDER          m_importWidth;
    UNIT_BINDER          m_importHeight;

    wxImage  m_previewSource;

    /// The file the source size and the preview were taken from.
    wxString m_sourcePath;

    /// Size of the source file's contents in mm, (0,0) when it has none to report.
    VECTOR2D m_sourceSizeMM;

    double m_xScale;
    double m_yScale;

    wxString m_filenameOverride;
};
