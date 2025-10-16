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

#pragma once

class SYMBOL_LIB_TABLE;
class PROJECT;
class SEARCH_STACK;
class SYMBOL_LIBS;
class SYMBOL_LIBRARY_MANAGER_ADAPTER;

class PROJECT_SCH
{
public:
    /// These are all prefaced with "Sch".
    static SYMBOL_LIBS* SchLibs( PROJECT* aProject );

    /// Accessor for Eeschema search stack.
    static SEARCH_STACK* SchSearchS( PROJECT* aProject );

    /// Accessor for project symbol library table.
    static SYMBOL_LIB_TABLE* SchSymbolLibTable( PROJECT* aProject );

    static SYMBOL_LIBRARY_MANAGER_ADAPTER*  SymbolLibManager( PROJECT* aProject );

private:
    PROJECT_SCH() {}
};
