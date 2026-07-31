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
 * @file mdw_review.h
 * Design reviews of a revision (CG_REVIEW), shown in Release Design.
 */

#ifndef MDW_CLIENT_REVIEW_H
#define MDW_CLIENT_REVIEW_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>

#include <api/mdw/backend_review.pb.h>

#include <memory>
#include <string>

class MDW_SOURCE;

class KICOMMON_API MDW_REVIEW
{
public:
    explicit MDW_REVIEW( std::shared_ptr<MDW_SOURCE> aSource );

    MDW_OP Get( const std::string& aReviewId, MDW_DONE<kiapi::mdw::Review> aDone );

    /// An empty aRevisionId lists reviews of every revision. Reviews naming another item are dropped.
    MDW_OP List( const MDW_ITEM_REF& aItem, const wxString& aRevisionId, const MDW_PAGE& aPage,
                 MDW_DONE<kiapi::mdw::ListReviewsReply> aDone );

private:
    std::shared_ptr<MDW_SOURCE> m_source;
};

#endif // MDW_CLIENT_REVIEW_H
