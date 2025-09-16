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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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
    ~DIALOG_IMPORT_GRAPHICS();

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
    void onFilename( wxCommandEvent& event );
    void onUpdateUI( wxUpdateUIEvent& event ) override;

private:
    PCB_BASE_FRAME*                           m_parent;
    std::unique_ptr<GRAPHICS_IMPORTER_PCBNEW> m_importer;
    std::unique_ptr<GRAPHICS_IMPORT_MGR>      m_gfxImportMgr;

    UNIT_BINDER          m_xOrigin;
    UNIT_BINDER          m_yOrigin;
    UNIT_BINDER          m_defaultLineWidth;
    UNIT_BINDER          m_tolerance;

    wxString m_filenameOverride;
};
