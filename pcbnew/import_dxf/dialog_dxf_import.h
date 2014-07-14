/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_dxf_import_base.h>
#include <wxPcbStruct.h>
#include <dxf2brd_items.h>

class DIALOG_DXF_IMPORT : public DIALOG_DXF_IMPORT_BASE
{
public:
    DIALOG_DXF_IMPORT( PCB_BASE_FRAME* aParent );
    ~DIALOG_DXF_IMPORT();

    /**
     * Function GetImportedItems()
     *
     * Returns a list of items imported from a DXF file.
     */
    const std::list<BOARD_ITEM*>& GetImportedItems() const
    {
        return m_dxfImporter.GetItemsList();
    }

private:
    PCB_BASE_FRAME*      m_parent;
    wxConfigBase*        m_config;               // Current config
    DXF2BRD_CONVERTER    m_dxfImporter;

    static wxString      m_dxfFilename;
    static int           m_offsetSelection;
    static LAYER_NUM     m_layer;

    // Virtual event handlers
    void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
    void OnOKClick( wxCommandEvent& event );
    void OnBrowseDxfFiles( wxCommandEvent& event );
};
