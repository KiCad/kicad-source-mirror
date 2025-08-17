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
#include <project_sch.h>
#include <libraries/legacy_symbol_library.h>
#include <libraries/symbol_library_adapter.h>

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
    auto libs = static_cast<LEGACY_SYMBOL_LIBS*>( aProject->GetElem( PROJECT::ELEM::LEGACY_SYMBOL_LIBS ) );

    wxASSERT( !libs || libs->ProjectElementType() == PROJECT::ELEM::LEGACY_SYMBOL_LIBS );

    if( !libs )
    {
        libs = new LEGACY_SYMBOL_LIBS();

        // Make PROJECT the new SYMBOL_LIBS owner.
        aProject->SetElem( PROJECT::ELEM::LEGACY_SYMBOL_LIBS, libs );

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


SYMBOL_LIBRARY_ADAPTER* PROJECT_SCH::SymbolLibAdapter( PROJECT* aProject )
{
    LIBRARY_MANAGER& mgr = Pgm().GetLibraryManager();
    std::optional<LIBRARY_MANAGER_ADAPTER*> adapter = mgr.Adapter( LIBRARY_TABLE_TYPE::SYMBOL );

    if( !adapter )
    {
        mgr.RegisterAdapter( LIBRARY_TABLE_TYPE::SYMBOL,
                             std::make_unique<SYMBOL_LIBRARY_ADAPTER>( mgr ) );

        std::optional<LIBRARY_MANAGER_ADAPTER*> created = mgr.Adapter( LIBRARY_TABLE_TYPE::SYMBOL );
        wxCHECK( created && ( *created )->Type() == LIBRARY_TABLE_TYPE::SYMBOL, nullptr );
        return static_cast<SYMBOL_LIBRARY_ADAPTER*>( *created );
    }

    wxCHECK( ( *adapter )->Type() == LIBRARY_TABLE_TYPE::SYMBOL, nullptr );
    return static_cast<SYMBOL_LIBRARY_ADAPTER*>( *adapter );
}
