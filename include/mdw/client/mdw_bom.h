/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

/**
 * @file mdw_bom.h
 * A backend's bill of materials for a design (CG_BOM). Read only.
 */

#ifndef MDW_CLIENT_BOM_H
#define MDW_CLIENT_BOM_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>

#include <wx/datetime.h>

#include <memory>
#include <vector>

class MDW_SOURCE;

struct MDW_BOM_LINE
{
    MDW_ITEM_REF          part;             ///< A part in the PARTS id space
    wxString              part_number;
    std::vector<wxString> references;       ///< May be empty
    double                quantity = 0.0;
    wxString              unit;             ///< Empty means pieces
};

struct MDW_BOM_DATA
{
    wxString                  bom_id;
    wxString                  bom_type;
    wxString                  label;
    wxDateTime                updated;
    wxString                  updated_by;
    std::vector<MDW_BOM_LINE> lines;
};

class KICOMMON_API MDW_BOM
{
public:
    explicit MDW_BOM( std::shared_ptr<MDW_SOURCE> aSource );

    /// ITEM_NOT_FOUND when the design has no BOM
    MDW_OP Get( const MDW_ITEM_REF& aProject, const wxString& aRevisionId, MDW_DONE<MDW_BOM_DATA> aDone );

private:
    std::shared_ptr<MDW_SOURCE> m_source;
};

#endif // MDW_CLIENT_BOM_H
