/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <properties.h>

#include <sch_io_mgr.h>

#define FMT_UNIMPLEMENTED   _( "Plugin \"%s\" does not implement the \"%s\" function." )

/**
 * Function not_implemented
 * throws an IO_ERROR and complains of an API function not being implemented.
 *
 * @param aPlugin is a SCH_PLUGIN instance
 * @param aCaller is the name of the unimplemented API function.
 */
static void not_implemented( const SCH_PLUGIN* aPlugin, const char* aCaller )
{
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED,
                                      aPlugin->GetName().GetData(),
                                      wxString::FromUTF8( aCaller ).GetData() ) );
}


void SCH_PLUGIN::SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties )
{
    not_implemented( this, __FUNCTION__ );
}


SCH_SHEET* SCH_PLUGIN::Load( const wxString& aFileName, KIWAY* aKiway, SCH_SHEET* aAppendToMe,
                             const PROPERTIES* aProperties )
{
    not_implemented( this, __FUNCTION__ );
    return NULL;
}


void SCH_PLUGIN::Save( const wxString& aFileName, SCH_SCREEN* aSchematic, KIWAY* aKiway,
                       const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
}


void SCH_PLUGIN::EnumerateSymbolLib( wxArrayString&    aAliasNameList,
                                     const wxString&   aLibraryPath,
                                     const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
}


void SCH_PLUGIN::EnumerateSymbolLib( std::vector<LIB_ALIAS*>& aAliasList,
                                     const wxString&   aLibraryPath,
                                     const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
}


LIB_ALIAS* SCH_PLUGIN::LoadSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                   const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
    return NULL;
}


void SCH_PLUGIN::SaveSymbol( const wxString& aLibraryPath, const LIB_PART* aSymbol,
                             const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
}


void SCH_PLUGIN::DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
                              const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
}


void SCH_PLUGIN::DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                               const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
}


void SCH_PLUGIN::CreateSymbolLib( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
}


bool SCH_PLUGIN::DeleteSymbolLib( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
    return false;
}


bool SCH_PLUGIN::IsSymbolLibWritable( const wxString& aLibraryPath )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
    return false;
}


void SCH_PLUGIN::SymbolLibOptions( PROPERTIES* aListToAppendTo ) const
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
        ));

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
    not_implemented( this, __FUNCTION__ );
    return false;
}


const wxString& SCH_PLUGIN::GetError() const
{
    // not pure virtual so that plugins only have to implement subset of the SCH_PLUGIN interface.
    not_implemented( this, __FUNCTION__ );
    static wxString error;
    return error;
}
