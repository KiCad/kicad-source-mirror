/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

#include <class_drawpanel.h>
#include <class_library.h>
#include <confirm.h>
#include <invoke_sch_dialog.h>
#include <kicad_device_context.h>
#include <lib_cache_rescue.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <schframe.h>
#include <wildcards_and_files_ext.h>

#include <boost/foreach.hpp>
#include <map>


typedef std::pair<SCH_COMPONENT*, wxString> COMPONENT_NAME_PAIR;


/**
 * Function save_library
 * writes the library out to disk. Returns true on success.
 *
 * @param aFileName - Filename to receive the library
 * @param aLibrary - Library to write
 * @param aEditFrame - the calling SCH_EDIT_FRAME
 */
static bool save_library( const wxString& aFileName, PART_LIB* aLibrary, SCH_EDIT_FRAME* aEditFrame )
{
    try
    {
        FILE_OUTPUTFORMATTER formatter( aFileName );

        if( !aLibrary->Save( formatter ) )
        {
            wxString msg = wxString::Format( _(
                "An error occurred attempting to save component library '%s'." ),
                GetChars( aFileName )
                );
            DisplayError( aEditFrame, msg );
            return false;
        }
    }
    catch( ... /* IO_ERROR ioe */ )
    {
        wxString msg = wxString::Format( _(
            "Failed to create component library file '%s'" ),
            GetChars( aFileName )
            );
        DisplayError( aEditFrame, msg );
        return false;
    }

    return true;
}


/**
 * Function insert_library
 * inserts a library into the project and refreshes libraries.
 *
 * @param aProject - project that will be modified
 * @param aLibrary - PART_LIB to add
 * @param aIndex - index in the list at which the library is to be inserted
 *
 * @return true on success, false on failure
 */
static bool insert_library( PROJECT *aProject, PART_LIB *aLibrary, size_t aIndex ) throw( boost::bad_pointer )
{
    wxArrayString libNames;
    wxString libPaths;

    wxString libName = aLibrary->GetName();
    PART_LIBS *libs = dynamic_cast<PART_LIBS*>( aProject->GetElem( PROJECT::ELEM_SCH_PART_LIBS ) );
    if( !libs )
    {
        libs = new PART_LIBS();
        aProject->SetElem( PROJECT::ELEM_SCH_PART_LIBS, libs );
    }

    PART_LIBS::LibNamesAndPaths( aProject, false, &libPaths, &libNames );

    // Make sure the library is not already in the list
    while( libNames.Index( libName ) != wxNOT_FOUND )
        libNames.Remove( libName );

    // Add the library to the list and save
    libNames.Insert( libName, aIndex );
    PART_LIBS::LibNamesAndPaths( aProject, true, &libPaths, &libNames );

    // Save the old libraries in case there is a problem after clear(). We'll
    // put them back in.
    boost::ptr_vector<PART_LIB> libsSave;
    libsSave.transfer( libsSave.end(), libs->begin(), libs->end(), *libs );

    aProject->SetElem( PROJECT::ELEM_SCH_PART_LIBS, NULL );

    libs = new PART_LIBS();
    try
    {
        libs->LoadAllLibraries( aProject );
    }
    catch( const PARSE_ERROR& e )
    {
        // Some libraries were not found. There's no point in showing the error,
        // because it was already shown. Just don't do anything.
    }
    catch( const IO_ERROR& e )
    {
        // Restore the old list
        libs->clear();
        libs->transfer( libs->end(), libsSave.begin(), libsSave.end(), libsSave );
        return false;
    }
    aProject->SetElem( PROJECT::ELEM_SCH_PART_LIBS, libs );

    return true;
}


/**
 * Function get_components
 * Fills a vector with all of the project's components, to ease iterating over them.
 *
 * @param aComponents - a vector that will take the components
 */
static void get_components( std::vector<SCH_COMPONENT*>& aComponents )
{
    SCH_SCREENS screens;
    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T ) continue;
            SCH_COMPONENT* component = dynamic_cast<SCH_COMPONENT*>( item );
            aComponents.push_back( component );
        }
    }
}


/**
 * Function find_component
 * Search the libraries for the first component with a given name.
 *
 * @param aName - name to search for
 * @param aLibs - the loaded PART_LIBS
 * @param aCached - whether we are looking for the cached part
 */
static LIB_PART* find_component( wxString aName, PART_LIBS* aLibs, bool aCached )
{
    LIB_PART *part = NULL;

    BOOST_FOREACH( PART_LIB& each_lib, *aLibs )
    {
        if( aCached && !each_lib.IsCache() )
            continue;

        if( !aCached && each_lib.IsCache() )
            continue;

        part = each_lib.FindPart( aName );
        if( part )
            break;
    }

    return part;
}


/**
 * Function find_rescues
 * Search components for any that request a part that conflicts with the
 * library parts.
 *
 * This is done from the component side to track requested aliases.
 *
 * @param aComponents - a vector of components to scan
 * @param aLibs - the loaded PART_LIBS
 * @param aCandidates - a vector to hold rescue candidates
 */
static void find_rescues( std::vector<SCH_COMPONENT*>& aComponents, PART_LIBS* aLibs,
        std::vector<RESCUE_CANDIDATE>& aCandidates )
{
    // We need to narrow down the list and avoid having multiple copies of the
    // same name. Therefore, we'll assemble in a map first, before pushing to
    // the vector.
    typedef std::map<wxString, RESCUE_CANDIDATE> candidate_map_t;
    candidate_map_t candidate_map;
    BOOST_FOREACH( SCH_COMPONENT* each_component, aComponents )
    {
        wxString part_name( each_component->GetPartName() );
        LIB_PART* cache_match = find_component( part_name, aLibs, /* aCached */ true );
        LIB_PART* lib_match = find_component( part_name, aLibs, /* aCached */ false );

        // Test whether there is a conflict
        if( !cache_match || !lib_match )
            continue;
        if( !cache_match->PinsConflictWith( *lib_match, /* aTestNums */ true, /* aTestNames */ true,
                /* aTestType */ true, /* aTestOrientation */ true, /* aTestLength */ false ))
            continue;

        RESCUE_CANDIDATE candidate;
        candidate.requested_name = part_name;
        candidate.cache_candidate = cache_match;
        candidate.lib_candidate = lib_match;

        candidate_map[part_name] = candidate;
    }

    // Now, dump the map into aCandidates
    BOOST_FOREACH( const candidate_map_t::value_type& each_pair, candidate_map )
    {
        aCandidates.push_back( each_pair.second );
    }
}


/**
 * Function create_rescue_library
 * Creates and returns a PART_LIB object for storing rescued components.
 * @param aFileName - wxFileName to receive the library's file name
 */
static PART_LIB* create_rescue_library( wxFileName& aFileName )
{
    wxFileName fn( g_RootSheet->GetScreen()->GetFileName() );
    fn.SetName( fn.GetName() + wxT( "-rescue" ) );
    fn.SetExt( SchematicLibraryFileExtension );
    aFileName.SetPath( fn.GetPath() );
    aFileName.SetName( fn.GetName() );
    aFileName.SetExt( wxT( "lib" ) );
    return new PART_LIB( LIBRARY_TYPE_EESCHEMA, fn.GetFullPath() );
}


/**
 * Function rescue_components
 * Rescues components from aCandidates into aLibrary
 * @param aCandidates - list of final rescue candidates to be rescued
 * @param aLibrary - library for them to be rescued into
 * @param aSuffix - part name suffix to apply to them
 */
static void rescue_components( std::vector<RESCUE_CANDIDATE>& aCandidates, PART_LIB* aLibrary, const wxString &aSuffix )
{
    BOOST_FOREACH( RESCUE_CANDIDATE& each_candidate, aCandidates )
    {
        LIB_PART new_part( *each_candidate.cache_candidate, aLibrary );
        new_part.SetName( each_candidate.requested_name + aSuffix );
        new_part.RemoveAllAliases();
        aLibrary->AddPart( &new_part );
    }
}


/**
 * Function update_components
 * Update components to reflect changed names of rescued parts.
 * Saves components with the original names to aRescueLog to allow recovering from errors and
 * displaying summary.
 *
 * @param aComponents - a populated list of all components
 * @param aCandidates - list of rescue candidates
 * @param aSuffix - part name suffix
 * @param aRescueLog - rescue log
 */
static void update_components( std::vector<SCH_COMPONENT*>& aComponents, std::vector<RESCUE_CANDIDATE>& aCandidates,
        const wxString& aSuffix, std::vector<RESCUE_LOG>& aRescueLog )
{
    BOOST_FOREACH( RESCUE_CANDIDATE& each_candidate, aCandidates )
    {
        BOOST_FOREACH( SCH_COMPONENT* each_component, aComponents )
        {
            if( each_component->GetPartName() != each_candidate.requested_name ) continue;

            wxString new_name = each_candidate.requested_name + aSuffix;
            each_component->SetPartName( new_name );

            RESCUE_LOG log_item;
            log_item.component = each_component;
            log_item.old_name = each_candidate.requested_name;
            log_item.new_name = new_name;
            aRescueLog.push_back( log_item );
        }
    }
}


bool SCH_EDIT_FRAME::RescueCacheConflicts( bool aRunningOnDemand )
{
    // Data that will be used throughout the operation
    std::vector<RESCUE_CANDIDATE>   candidates;
    std::vector<SCH_COMPONENT*>     components;
    PART_LIBS*              libs;
    wxString                part_name_suffix;
    PROJECT*                prj;

    // Prepare data
    get_components( components );
    prj = &Prj();
    libs = prj->SchLibs();
    part_name_suffix =  wxT( "-RESCUE-" ) + prj->GetProjectName();

    // Start!
    find_rescues( components, libs, candidates );
    if( candidates.empty() )
    {
        if( aRunningOnDemand )
        {
            wxMessageDialog dlg( this, _( "There are no conflicting symbols to rescue from the cache." ) );
            dlg.ShowModal();
        }
        return true;
    }
    InvokeDialogRescueEach( this, candidates, components, /* aAskShowAgain */ !aRunningOnDemand );
    wxFileName library_fn;
    std::auto_ptr<PART_LIB> rescue_lib( create_rescue_library( library_fn ) );
    rescue_components( candidates, rescue_lib.get(), part_name_suffix );

    // If no components were rescued, let the user know what's going on. He might
    // have clicked cancel by mistake, and should have some indication of that.
    if( candidates.empty() )
    {
        wxMessageDialog dlg( this, _( "No cached symbols were rescued." ) );
        dlg.ShowModal();
        return true;
    }

    if( !save_library( library_fn.GetFullPath(), rescue_lib.get(), this ) )
    {
        // Save failed. Do not update the components.
        return false;
    }

    // Update components to reflect changed names
    std::vector<RESCUE_LOG> rescue_log;
    update_components( components, candidates, part_name_suffix, rescue_log );

    wxASSERT( !rescue_log.empty() );

    // Try inserting the library into the project
    if( insert_library( prj, rescue_lib.get(), 0 ) )
    {
        // Clean up wire ends
        INSTALL_UNBUFFERED_DC( dc, m_canvas );
        GetScreen()->SchematicCleanUp( NULL, &dc );
        m_canvas->Refresh( true );
        OnModify();

        return true;
    }
    else
    {
        // Unsuccessful! Restore all the components
        BOOST_FOREACH( RESCUE_LOG& rescue_log_item, rescue_log )
        {
            rescue_log_item.component->SetPartName( rescue_log_item.old_name );
        }
        wxMessageDialog dlg( this, _( "An error occurred while attempting to rescue components. No changes have been made." ) );
        dlg.ShowModal();
        return false;
    }
}
