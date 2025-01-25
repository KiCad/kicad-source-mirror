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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <confirm.h>
#include <dialogs/html_message_box.h>
#include <kiface_base.h>
#include <pgm_base.h>
#include <wx/app.h>
#include <core/utf8.h>
#include <symbol_lib_table.h>
#include <project_sch.h>
#include <libraries/legacy_symbol_library.h>
#include <libraries/symbol_library_manager_adapter.h>

static std::mutex s_symbolTableMutex;

// non-member so it can be moved easily, and kept REALLY private.
// Do NOT Clear() in here.
static void add_search_paths( SEARCH_STACK* aDst, const SEARCH_STACK& aSrc, int aIndex )
{
    for( unsigned i=0; i<aSrc.GetCount();  ++i )
        aDst->AddPaths( aSrc[i], aIndex );
}


SEARCH_STACK* PROJECT_SCH::SchSearchS( PROJECT* aProject )
{
    SEARCH_STACK* ss = (SEARCH_STACK*) aProject->GetElem( PROJECT::ELEM::SCH_SEARCH_STACK );

    wxASSERT( !ss || dynamic_cast<SEARCH_STACK*>( ss ) );

    if( !ss )
    {
        ss = new SEARCH_STACK();

        // Make PROJECT the new SEARCH_STACK owner.
        aProject->SetElem( PROJECT::ELEM::SCH_SEARCH_STACK, ss );

        // to the empty SEARCH_STACK for SchSearchS(), add project dir as first
        ss->AddPaths( aProject->GetProjectDirectory() );

        // next add the paths found in *.pro, variable "LibDir"
        wxString        libDir;

        try
        {
            LEGACY_SYMBOL_LIBS::GetLibNamesAndPaths( aProject, &libDir );
        }
        catch( const IO_ERROR& )
        {
        }

        if( !!libDir )
        {
            wxArrayString   paths;

            SEARCH_STACK::Split( &paths, libDir );

            for( unsigned i =0; i<paths.GetCount();  ++i )
            {
                wxString path = aProject->AbsolutePath( paths[i] );

                ss->AddPaths( path );     // at the end
            }
        }

        // append all paths from aSList
        add_search_paths( ss, Kiface().KifaceSearch(), -1 );
    }

    return ss;
}


LEGACY_SYMBOL_LIBS* PROJECT_SCH::LegacySchLibs( PROJECT* aProject )
{
    LEGACY_SYMBOL_LIBS* libs = (LEGACY_SYMBOL_LIBS*) aProject->GetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS );

    wxASSERT( !libs || libs->ProjectElementType() == PROJECT::ELEM::SCH_SYMBOL_LIBS );

    if( !libs )
    {
        libs = new LEGACY_SYMBOL_LIBS();

        // Make PROJECT the new SYMBOL_LIBS owner.
        aProject->SetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS, libs );

        try
        {
            libs->LoadAllLibraries( aProject );
        }
        catch( const PARSE_ERROR& pe )
        {
            wxString    lib_list = UTF8( pe.inputLine );
            wxWindow*   parent = Pgm().App().GetTopWindow();

            // parent of this dialog cannot be NULL since that breaks the Kiway() chain.
            HTML_MESSAGE_BOX dlg( parent, _( "Not Found" ) );

            dlg.MessageSet( _( "The following libraries were not found:" ) );
            dlg.ListSet( lib_list );
            dlg.Layout();

            dlg.ShowModal();
        }
        catch( const IO_ERROR& ioe )
        {
            wxWindow* parent = Pgm().App().GetTopWindow();

            DisplayError( parent, ioe.What() );
        }
    }

    return libs;
}


SYMBOL_LIB_TABLE* PROJECT_SCH::SchSymbolLibTable( PROJECT* aProject )
{
    std::lock_guard<std::mutex> lock( s_symbolTableMutex );

    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.
    SYMBOL_LIB_TABLE* tbl =
            (SYMBOL_LIB_TABLE*) aProject->GetElem( PROJECT::ELEM::SYMBOL_LIB_TABLE );

    // its gotta be NULL or a SYMBOL_LIB_TABLE, or a bug.
    wxASSERT( !tbl || tbl->ProjectElementType() == PROJECT::ELEM::SYMBOL_LIB_TABLE );

    if( !tbl )
    {
        // Stack the project specific SYMBOL_LIB_TABLE overlay on top of the global table.
        // ~SYMBOL_LIB_TABLE() will not touch the fallback table, so multiple projects may
        // stack this way, all using the same global fallback table.
        tbl = new SYMBOL_LIB_TABLE( &SYMBOL_LIB_TABLE::GetGlobalLibTable() );

        aProject->SetElem( PROJECT::ELEM::SYMBOL_LIB_TABLE, tbl );

        wxString prjPath;

        wxGetEnv( PROJECT_VAR_NAME, &prjPath );

        if( !prjPath.IsEmpty() )
        {
            wxFileName fn( prjPath, SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

            try
            {
                tbl->Load( fn.GetFullPath() );
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg;
                msg.Printf( _( "Error loading the symbol library table '%s'." ), fn.GetFullPath() );
                DisplayErrorMessage( nullptr, msg, ioe.What() );
            }
        }
    }

    return tbl;
}


SYMBOL_LIBRARY_MANAGER_ADAPTER* PROJECT_SCH::SymbolLibManager( PROJECT* aProject )
{
    auto adapter = static_cast<SYMBOL_LIBRARY_MANAGER_ADAPTER*>(
            aProject->GetElem( PROJECT::ELEM::SYM_LIB_ADAPTER ) );

    wxASSERT( !adapter || adapter->ProjectElementType() == PROJECT::ELEM::SYM_LIB_ADAPTER );

    if( !adapter )
    {
        adapter = new SYMBOL_LIBRARY_MANAGER_ADAPTER( Pgm().GetLibraryManager(), *aProject );
        aProject->SetElem( PROJECT::ELEM::SYM_LIB_ADAPTER, adapter );
    }

    return adapter;
}
