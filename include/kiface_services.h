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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KIFACE_SERVICES_H
#define KIFACE_SERVICES_H

#include <set>
#include <string>
#include <type_traits>

#include <wx/string.h>

#include <kiface_ids.h>
#include <kiway.h>

class FOOTPRINT_LIBRARY_ADAPTER;
class PROJECT;
class REPORTER;
class WIDGET_DIFF_CANVAS;
class wxWindow;

namespace KICAD_DIFF
{
class DIFF_CANVAS_CONTEXT;
struct DOCUMENT_DIFF;
}

/**
 * Describes an object or function exported by a KIFACE through IfaceOrAddress()
 */
template <KIFACE_ADDR_ID aId, typename T>
struct KIFACE_SERVICE
{
    static constexpr KIFACE_ADDR_ID Id = aId;
    using Type = T;
};

using KIFACE_FOOTPRINT_LIBRARY_ADAPTER_SERVICE =
        KIFACE_SERVICE<KIFACE_FOOTPRINT_LIBRARY_ADAPTER, FOOTPRINT_LIBRARY_ADAPTER*>;

using KIFACE_FILTER_FOOTPRINTS_SERVICE =
        KIFACE_SERVICE<KIFACE_FILTER_FOOTPRINTS, wxString (*)( const wxString& )>;

using KIFACE_TRIGGER_FOOTPRINTS_LOAD_SERVICE =
        KIFACE_SERVICE<KIFACE_TRIGGER_FOOTPRINTS_LOAD, bool (*)()>;

using KIFACE_FOOTPRINTS_LOAD_PROGRESS_SERVICE =
        KIFACE_SERVICE<KIFACE_FOOTPRINTS_LOAD_PROGRESS, float (*)()>;

using KIFACE_FILTER_FOOTPRINT_USERS_SERVICE =
        KIFACE_SERVICE<KIFACE_FILTER_FOOTPRINT_USERS, wxString (*)( const wxString& )>;

using KIFACE_TRIGGER_SYMBOLS_LOAD_SERVICE =
        KIFACE_SERVICE<KIFACE_TRIGGER_SYMBOLS_LOAD, bool (*)()>;

using KIFACE_SYMBOLS_LOAD_PROGRESS_SERVICE =
        KIFACE_SERVICE<KIFACE_SYMBOLS_LOAD_PROGRESS, float (*)()>;

using KIFACE_NETLIST_SCHEMATIC_SERVICE =
        KIFACE_SERVICE<KIFACE_NETLIST_SCHEMATIC, bool (*)( const wxString&, std::string&, KIWAY* )>;

using KIFACE_MERGE_DOCUMENT_SERVICE =
        KIFACE_SERVICE<KIFACE_MERGE_DOCUMENT, int (*)( int, const wxString&, const wxString&,
                                                       const wxString&, const wxString&, bool, bool,
                                                       REPORTER* )>;

using KIFACE_OPEN_DIFF_DIALOG_SERVICE =
        KIFACE_SERVICE<KIFACE_OPEN_DIFF_DIALOG, int (*)( int, const wxString&, const wxString&,
                                                         const wxString&, const wxString&, wxWindow*,
                                                         REPORTER* )>;

using KIFACE_ATTACH_DIFF_CANVASES_SERVICE =
        KIFACE_SERVICE<KIFACE_ATTACH_DIFF_CANVASES,
                       KICAD_DIFF::DIFF_CANVAS_CONTEXT* (*)( int, const wxString&, const wxString&,
                                                            WIDGET_DIFF_CANVAS*, WIDGET_DIFF_CANVAS*,
                                                            KICAD_DIFF::DOCUMENT_DIFF*, REPORTER* )>;

using KIFACE_TEST_FOOTPRINT_LINK_SERVICE =
        KIFACE_SERVICE<KIFACE_TEST_FOOTPRINT_LINK, int (*)( const wxString&, PROJECT* )>;

using KIFACE_FOOTPRINT_PAD_NUMBERS_SERVICE =
        KIFACE_SERVICE<KIFACE_FOOTPRINT_PAD_NUMBERS,
                       void (*)( const wxString&, PROJECT*, std::set<wxString>& )>;

/**
 * Return a service address for IfaceOrAddress().
 *
 * @tparam Service is the KIFACE_SERVICE descriptor for the exported object
 * @param aAddress is the producer's object or function address
 */
template <typename Service, typename T>
void* KifaceServiceAddress( T aAddress )
{
    static_assert( std::is_same_v<T, typename Service::Type>,
                   "KIFACE service address does not match its service descriptor" );
    return reinterpret_cast<void*>( aAddress );
}

/**
 * Look up a service exported by a KIFACE.
 *
 * @tparam Service is the KIFACE_SERVICE descriptor for the requested object
 * @return the service address, or nullptr if the KIFACE does not export it
 */
template <typename Service>
typename Service::Type GetKifaceService( KIFACE* aKiface )
{
    if( !aKiface )
        return nullptr;

    return reinterpret_cast<typename Service::Type>( aKiface->IfaceOrAddress( Service::Id ) );
}

#endif // KIFACE_SERVICES_H
