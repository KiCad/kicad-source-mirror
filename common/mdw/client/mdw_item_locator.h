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
 * @file mdw_item_locator.h
 * Which workspace item a local file is. Client implementations only.
 */

#ifndef MDW_ITEM_LOCATOR_H
#define MDW_ITEM_LOCATOR_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>

#include <memory>
#include <optional>
#include <vector>

class MDW_SOURCE;
class PROJECT;

/**
 * What a one-path Locate reply says of that path. aLocated gets the item and its base, the item's
 * source and path filled from aLocator and aRelative when the backend left them empty, and stays
 * empty for a file that is not an item.
 */
KICOMMON_API MDW_RESULT MdwLocatedItem( bool aWorkingCopy, const MDW_RESULT& aResult,
                                        const std::vector<MDW_LOCATED_ITEM>& aFound, const wxString& aLocator,
                                        const wxString& aRelative, std::optional<MDW_LOCATED_ITEM>& aLocated );

/**
 * A file checked out from an upload backend is bound to its item in the project's local settings. For any
 * other file only the backend knows, so an item it names is kept until the session reconnects or a check-in
 * may have renamed something. A non-item is asked about again, since it may be tracked outside KiCad.
 */
class KICOMMON_API MDW_ITEM_LOCATOR
{
public:
    static MDW_ITEM_LOCATOR& Get();

    /// The open project's source when aProject is the open project, else null
    static std::shared_ptr<MDW_SOURCE> SourceFor( const PROJECT& aProject );

    /**
     * What is known of aLocalPath without asking the backend. The inner value is empty when the file
     * cannot be an item of this source, and the outer one when only the backend can tell.
     */
    std::optional<std::optional<MDW_ITEM_REF>> Known( const std::shared_ptr<MDW_SOURCE>& aSource,
                                                      const PROJECT& aProject, const wxString& aLocalPath );

    /**
     * The item at aLocalPath and the revision it was taken from, asking the backend on a miss. The payload
     * is empty when the file is not an item, and its base is empty when unknown.
     */
    MDW_OP Locate( const std::shared_ptr<MDW_SOURCE>& aSource, const PROJECT& aProject, const wxString& aLocalPath,
                   MDW_DONE<std::optional<MDW_LOCATED_ITEM>> aDone );

private:
    MDW_ITEM_LOCATOR();
    ~MDW_ITEM_LOCATOR();

    void Forget();

    /// Known() with the base, for aRelative, the path MdwRootRelativeUp() gave for aLocalPath
    std::optional<std::optional<MDW_LOCATED_ITEM>> known( const std::shared_ptr<MDW_SOURCE>& aSource,
                                                          const PROJECT& aProject, const wxString& aLocalPath,
                                                          const wxString& aRelative );

    struct IMPL;
    std::unique_ptr<IMPL> m_impl;
};

#endif // MDW_ITEM_LOCATOR_H
