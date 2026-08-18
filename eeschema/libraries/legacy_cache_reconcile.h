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

#ifndef LEGACY_CACHE_RECONCILE_H
#define LEGACY_CACHE_RECONCILE_H

#include <vector>

#include <wx/string.h>

struct LOAD_MESSAGE;
class SCH_SCREENS;
class SYMBOL_LIBRARY_ADAPTER;


/**
 * Point symbols that are only available in a legacy project cache library at that cache.
 *
 * When a legacy schematic is opened, any symbol whose library identifier names an entry of the
 * project cache library is re-linked to the cache unless one of the other libraries in the
 * library table still supplies it.  Without this the symbol resolves to nothing and is drawn as
 * a placeholder box.
 *
 * Every library consulted is loaded synchronously.  Testing whether the background preload
 * happens to have reached a library would make the result depend on load timing, so the same
 * project could be re-linked differently from one open to the next.
 *
 * @param aAdapter is the symbol library adapter of the project being opened.
 * @param aCacheNickname is the library table nickname of the project cache library.
 * @param aScreens are the screens of the schematic being opened.
 * @param aErrors collects one message per library that could not be loaded.
 * @return the number of symbols pointed at the cache library.
 */
int ReconcileLegacyCacheSymbols( SYMBOL_LIBRARY_ADAPTER& aAdapter, const wxString& aCacheNickname,
                                 SCH_SCREENS& aScreens, std::vector<LOAD_MESSAGE>& aErrors );

#endif // LEGACY_CACHE_RECONCILE_H
