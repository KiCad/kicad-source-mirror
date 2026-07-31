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
 * @file mdw_eco.h
 * Engineering change orders (CG_ECO), listed and shown; their workflow stays in the backend.
 */

#ifndef MDW_CLIENT_ECO_H
#define MDW_CLIENT_ECO_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>

#include <api/mdw/backend_eco.pb.h>

#include <memory>
#include <string>
#include <vector>

class MDW_SOURCE;

class KICOMMON_API MDW_ECO
{
public:
    explicit MDW_ECO( std::shared_ptr<MDW_SOURCE> aSource );

    MDW_OP Schema( MDW_DONE<kiapi::workspace::Schema> aDone );
    MDW_OP Get( const std::string& aEcoId, MDW_DONE<kiapi::mdw::Eco> aDone );

    /// The change orders affecting any of aItems; each affected item names its source
    MDW_OP List( const std::vector<MDW_ITEM_REF>& aItems, bool aIncludeClosed, const MDW_PAGE& aPage,
                 MDW_DONE<MDW_PAGED<kiapi::mdw::Eco>> aDone );

private:
    std::shared_ptr<MDW_SOURCE> m_source;
};

#endif // MDW_CLIENT_ECO_H
