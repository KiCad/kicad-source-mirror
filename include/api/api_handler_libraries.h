/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_API_HANDLER_LIBRARIES_H
#define KICAD_API_HANDLER_LIBRARIES_H

#include <functional>
#include <vector>

#include <api/api_handler.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/commands/library_commands.pb.h>
#include <libraries/library_table.h>


class LIB_ID;
class LIBRARY_MANAGER_ADAPTER;
class PROJECT;

/**
 * Base class for API handlers related to library management.
 *
 * These are separate from the API_HANDLER_EDITOR derived classes because
 * libraries (other than design blocks) need to be opened in a kiface context,
 * but not associated with one particular frame.
 */
class API_HANDLER_LIBRARIES : public API_HANDLER
{
public:
    API_HANDLER_LIBRARIES( LIBRARY_TABLE_TYPE aType = LIBRARY_TABLE_TYPE::DESIGN_BLOCK );

    ~API_HANDLER_LIBRARIES() override = default;

protected:
    HANDLER_RESULT<kiapi::common::commands::LibraryItemsResponse>
    handleGetLibraryItems( const HANDLER_CONTEXT<kiapi::common::commands::GetLibraryItems>& aCtx );

    HANDLER_RESULT<kiapi::common::commands::LibraryStatusResponse>
    handleGetLibraryStatuses( const HANDLER_CONTEXT<kiapi::common::commands::GetLibraryStatuses>& aCtx );

    HANDLER_RESULT<kiapi::common::types::LibraryCommandStatus>
    handleReloadLibrary( const HANDLER_CONTEXT<kiapi::common::commands::ReloadLibrary>& aCtx );

    HANDLER_RESULT<kiapi::common::commands::GetItemsResponse>
    handleGetItemsFromLibrary( const HANDLER_CONTEXT<kiapi::common::commands::GetItemsFromLibrary>& aCtx );

    virtual bool packLibraryItem( const LIB_ID& aId, google::protobuf::Any& aOutput ) const
    {
        return false;
    }

    LIBRARY_TABLE_TYPE libraryType() const { return m_type; }

    virtual LIBRARY_MANAGER_ADAPTER* adapterForProject( PROJECT& aProject ) const;

    /**
     * Returns the names of all entries in the given library, or an empty vector if the
     * library is not loaded (or cannot be enumerated).
     */
    virtual std::vector<wxString> getItemNames( LIBRARY_MANAGER_ADAPTER& aAdapter, const wxString& aNickname ) const;

private:
    LIBRARY_TABLE_TYPE m_type;
};

#endif //KICAD_API_HANDLER_LIBRARIES_H
