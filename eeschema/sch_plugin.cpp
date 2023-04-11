/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string_utf8_map.h>

#include <sch_io_mgr.h>
#include <wx/translation.h>

#define FMT_UNIMPLEMENTED wxT( "Plugin \"%s\" does not implement the \"%s\" function." )
#define NOT_IMPLEMENTED( aCaller )                                                   \
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED,                             \
                                      GetName().GetData(),                           \
                                      wxString::FromUTF8( aCaller ).GetData() ) );


void SCH_PLUGIN::SaveLibrary( const wxString& aFileName, const STRING_UTF8_MAP* aProperties )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


SCH_SHEET* SCH_PLUGIN::Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                             SCH_SHEET* aAppendToMe, const STRING_UTF8_MAP* aProperties )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_PLUGIN::Save( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic,
                       const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_PLUGIN::EnumerateSymbolLib( wxArrayString&    aAliasNameList,
                                     const wxString&   aLibraryPath,
                                     const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_PLUGIN::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                     const wxString&   aLibraryPath,
                                     const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


LIB_SYMBOL* SCH_PLUGIN::LoadSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                    const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_PLUGIN::SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                             const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_PLUGIN::DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                               const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_PLUGIN::CreateSymbolLib( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


bool SCH_PLUGIN::DeleteSymbolLib( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


bool SCH_PLUGIN::IsSymbolLibWritable( const wxString& aLibraryPath )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_PLUGIN::SymbolLibOptions( STRING_UTF8_MAP* aListToAppendTo ) const
{
    // disable all these in another couple of months, after everyone has seen them:
#if 1
    (*aListToAppendTo)["debug_level"] = UTF8( _(
        "Enable <b>debug</b> logging for Symbol*() functions in this SCH_PLUGIN."
        ) );

    (*aListToAppendTo)["read_filter_regex"] = UTF8( _(
        "Regular expression <b>symbol name</b> filter."
        ) );

    (*aListToAppendTo)["enable_transaction_logging"] = UTF8( _(
        "Enable transaction logging. The mere presence of this option turns on the "
        "logging, no need to set a Value."
        ) );

    (*aListToAppendTo)["username"] = UTF8( _(
        "User name for <b>login</b> to some special library server."
        ) );

    (*aListToAppendTo)["password"] = UTF8( _(
        "Password for <b>login</b> to some special library server."
        ) );
#endif

#if 1
    // Suitable for a C++ to python SCH_PLUGIN::Footprint*() adapter, move it to the adapter
    // if and when implemented.
    (*aListToAppendTo)["python_symbol_plugin"] = UTF8( _(
        "Enter the python symbol which implements the SCH_PLUGIN::Symbol*() functions."
        ) );
#endif
}


bool SCH_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


const wxString& SCH_PLUGIN::GetError() const
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}
