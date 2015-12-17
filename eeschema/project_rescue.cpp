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
#include <project_rescue.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <schframe.h>
#include <wildcards_and_files_ext.h>

#include <cctype>
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

    try
    {
        PART_LIBS::LibNamesAndPaths( aProject, false, &libPaths, &libNames );

        // Make sure the library is not already in the list
        while( libNames.Index( libName ) != wxNOT_FOUND )
            libNames.Remove( libName );

        // Add the library to the list and save
        libNames.Insert( libName, aIndex );
        PART_LIBS::LibNamesAndPaths( aProject, true, &libPaths, &libNames );
    }
    catch( const IO_ERROR& e )
    {
        // Could not get or save the current libraries.
        return false;
    }

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


void RESCUER::RemoveDuplicates()
{
    std::vector<wxString> names_seen;

    for( boost::ptr_vector<RESCUE_CANDIDATE>::iterator it = m_all_candidates.begin();
            it != m_all_candidates.end(); )
    {
        bool seen_already = false;
        BOOST_FOREACH( wxString& name_seen, names_seen )
        {
            if( name_seen == it->GetRequestedName() )
            {
                seen_already = true;
                break;
            }
        }

        if( seen_already )
        {
            it = m_all_candidates.erase( it );
        }
        else
        {
            names_seen.push_back( it->GetRequestedName() );
            ++it;
        }
    }
}


class RESCUE_CASE_CANDIDATE: public RESCUE_CANDIDATE
{
    wxString m_requested_name;
    wxString m_new_name;
    LIB_PART* m_lib_candidate;

public:
    /**
     * Function FindRescues
     * Grab all possible RESCUE_CASE_CANDIDATES into a vector.
     * @param aRescuer - the working RESCUER instance.
     * @param aCandidates - the vector the will hold the candidates.
     */
    static void FindRescues( RESCUER& aRescuer, boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
    {
        typedef std::map<wxString, RESCUE_CASE_CANDIDATE> candidate_map_t;
        candidate_map_t candidate_map;

        BOOST_FOREACH( SCH_COMPONENT* each_component, *( aRescuer.GetComponents() ) )
        {
            wxString part_name( each_component->GetPartName() );

            LIB_ALIAS* case_sensitive_match = aRescuer.GetLibs()->FindLibraryEntry( part_name );
            std::vector<LIB_ALIAS*> case_insensitive_matches;
            aRescuer.GetLibs()->FindLibraryNearEntries( case_insensitive_matches, part_name );

            if( case_sensitive_match || !( case_insensitive_matches.size() ) )
                continue;

            RESCUE_CASE_CANDIDATE candidate(
                part_name, case_insensitive_matches[0]->GetName(),
                case_insensitive_matches[0]->GetPart() );
            candidate_map[part_name] = candidate;
        }

        // Now, dump the map into aCandidates
        BOOST_FOREACH( const candidate_map_t::value_type& each_pair, candidate_map )
        {
            aCandidates.push_back( new RESCUE_CASE_CANDIDATE( each_pair.second ) );
        }
    }

    /**
     * Constructor RESCUE_CANDIDATE
     * @param aRequestedName - the name the schematic asks for
     * @param aNewName - the name we want to change it to
     * @param aLibCandidate - the part that will give us
     */
    RESCUE_CASE_CANDIDATE( const wxString& aRequestedName, const wxString& aNewName,
            LIB_PART* aLibCandidate )
        : m_requested_name( aRequestedName ),
          m_new_name( aNewName ),
          m_lib_candidate( aLibCandidate ) { }

    RESCUE_CASE_CANDIDATE() { m_lib_candidate = NULL; }

    virtual wxString GetRequestedName() const { return m_requested_name; }
    virtual wxString GetNewName() const { return m_new_name; }
    virtual LIB_PART* GetLibCandidate() const { return m_lib_candidate; }
    virtual wxString GetActionDescription() const
    {
        wxString action;
        action.Printf( _( "Rename to %s" ), m_new_name );
        return action;
    }

    virtual bool PerformAction( RESCUER* aRescuer )
    {
        BOOST_FOREACH( SCH_COMPONENT* each_component, *aRescuer->GetComponents() )
        {
            if( each_component->GetPartName() != m_requested_name ) continue;
            each_component->SetPartName( m_new_name );
            each_component->ClearFlags();
            aRescuer->LogRescue( each_component, m_requested_name, m_new_name );
        }
        return true;
    }
};


class RESCUE_CACHE_CANDIDATE: public RESCUE_CANDIDATE
{
    wxString m_requested_name;
    wxString m_new_name;
    LIB_PART* m_cache_candidate;
    LIB_PART* m_lib_candidate;

    static std::auto_ptr<PART_LIB> m_rescue_lib;
    static wxFileName m_library_fn;

public:
    /**
     * Function FindRescues
     * Grab all possible RESCUE_CACHE_CANDIDATEs into a vector.
     * @param aRescuer - the working RESCUER instance.
     * @param aCandidates - the vector the will hold the candidates.
     */
    static void FindRescues( RESCUER& aRescuer, boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
    {
        typedef std::map<wxString, RESCUE_CACHE_CANDIDATE> candidate_map_t;
        candidate_map_t candidate_map;

        wxString part_name_suffix = aRescuer.GetPartNameSuffix();

        BOOST_FOREACH( SCH_COMPONENT* each_component, *( aRescuer.GetComponents() ) )
        {
            wxString part_name( each_component->GetPartName() );

            LIB_PART* cache_match = find_component( part_name, aRescuer.GetLibs(), /* aCached */ true );
            LIB_PART* lib_match = aRescuer.GetLibs()->FindLibPart( part_name );

            // Test whether there is a conflict
            if( !cache_match || !lib_match )
                continue;
            if( !cache_match->PinsConflictWith( *lib_match, /* aTestNums */ true,
                    /* aTestNames */ true, /* aTestType */ true, /* aTestOrientation */ true,
                    /* aTestLength */ false ))
                continue;

            RESCUE_CACHE_CANDIDATE candidate(
                part_name, part_name + part_name_suffix,
                cache_match, lib_match );
            candidate_map[part_name] = candidate;
        }

        // Now, dump the map into aCandidates
        BOOST_FOREACH( const candidate_map_t::value_type& each_pair, candidate_map )
        {
            aCandidates.push_back( new RESCUE_CACHE_CANDIDATE( each_pair.second ) );
        }
    }

    /**
     * Constructor RESCUE_CACHE_CANDIDATE
     * @param aRequestedName - the name the schematic asks for
     * @param aNewName - the name we want to change it to
     * @param aCacheCandidate - the part from the cache
     * @param aLibCandidate - the part that would be loaded from the library
     */
    RESCUE_CACHE_CANDIDATE( const wxString& aRequestedName, const wxString& aNewName,
            LIB_PART* aCacheCandidate, LIB_PART* aLibCandidate )
        : m_requested_name( aRequestedName ),
          m_new_name( aNewName ),
          m_cache_candidate( aCacheCandidate ),
          m_lib_candidate( aLibCandidate ) { }

    RESCUE_CACHE_CANDIDATE()
        : m_cache_candidate( NULL ), m_lib_candidate( NULL ) {}

    virtual wxString GetRequestedName() const { return m_requested_name; }
    virtual wxString GetNewName() const { return m_new_name; }
    virtual LIB_PART* GetCacheCandidate() const { return m_cache_candidate; }
    virtual LIB_PART* GetLibCandidate() const { return m_lib_candidate; }
    virtual wxString GetActionDescription() const
    {
        wxString action;
        action.Printf( _( "Rescue %s as %s" ), m_requested_name, m_new_name );
        return action;
    }

    /**
     * Function OpenRescueLibrary
     * Creates the new rescue library. Must be called before calling any PerformAction()s.
     */
    static void OpenRescueLibrary()
    {
        wxFileName fn( g_RootSheet->GetScreen()->GetFileName() );
        fn.SetName( fn.GetName() + wxT( "-rescue" ) );
        fn.SetExt( SchematicLibraryFileExtension );
        m_library_fn.SetPath( fn.GetPath() );
        m_library_fn.SetName( fn.GetName() );
        m_library_fn.SetExt( wxT( "lib" ) );

        std::auto_ptr<PART_LIB> rescue_lib( new PART_LIB( LIBRARY_TYPE_EESCHEMA,
                        fn.GetFullPath() ) );

        m_rescue_lib = rescue_lib;
    }

    virtual bool PerformAction( RESCUER* aRescuer )
    {
        LIB_PART new_part( *m_cache_candidate, m_rescue_lib.get() );
        new_part.SetName( m_new_name );
        new_part.RemoveAllAliases();
        RESCUE_CACHE_CANDIDATE::m_rescue_lib.get()->AddPart( &new_part );

        BOOST_FOREACH( SCH_COMPONENT* each_component, *aRescuer->GetComponents() )
        {
            if( each_component->GetPartName() != m_requested_name ) continue;
            each_component->SetPartName( m_new_name );
            each_component->ClearFlags();
            aRescuer->LogRescue( each_component, m_requested_name, m_new_name );
        }
        return true;
    }

    /**
     * Function WriteRescueLibrary
     * Writes out the rescue library. Called after successful PerformAction()s. If this fails,
     * undo the actions.
     * @return True on success.
     */
    static bool WriteRescueLibrary( SCH_EDIT_FRAME *aEditFrame, PROJECT* aProject )
    {

        if( !save_library( m_library_fn.GetFullPath(), m_rescue_lib.get(), aEditFrame ) )
            return false;
        return insert_library( aProject, m_rescue_lib.get(), 0 );
    }
};

std::auto_ptr<PART_LIB> RESCUE_CACHE_CANDIDATE::m_rescue_lib;
wxFileName RESCUE_CACHE_CANDIDATE::m_library_fn;

RESCUER::RESCUER( SCH_EDIT_FRAME& aEditFrame, PROJECT& aProject )
{
    get_components( m_components );
    m_prj = &aProject;
    m_libs = m_prj->SchLibs();
    m_edit_frame = &aEditFrame;
}


void RESCUER::FindCandidates()
{
    RESCUE_CASE_CANDIDATE::FindRescues( *this, m_all_candidates );
    RESCUE_CACHE_CANDIDATE::FindRescues( *this, m_all_candidates );
}


void RESCUER::InvokeDialog( bool aAskShowAgain )
{
    InvokeDialogRescueEach( m_edit_frame, *this, aAskShowAgain );
}


void RESCUER::LogRescue( SCH_COMPONENT *aComponent, const wxString &aOldName,
        const wxString &aNewName )
{
    RESCUE_LOG logitem;
    logitem.component = aComponent;
    logitem.old_name = aOldName;
    logitem.new_name = aNewName;
    m_rescue_log.push_back( logitem );
}


bool RESCUER::DoRescues()
{
    BOOST_FOREACH( RESCUE_CANDIDATE* each_candidate, m_chosen_candidates )
    {
        if( ! each_candidate->PerformAction( this ) )
            return false;
    }
    return true;
}


void RESCUER::UndoRescues()
{
    BOOST_FOREACH( RESCUE_LOG& each_logitem, m_rescue_log )
    {
        each_logitem.component->SetPartName( each_logitem.old_name );
        each_logitem.component->ClearFlags();
    }
}


wxString RESCUER::GetPartNameSuffix()
{
    wxString suffix = wxT( "-RESCUE-" );
    wxString pname = GetPrj()->GetProjectName();
    for( size_t i = 0; i < pname.Len(); ++i )
    {
        if( isspace( pname[i].GetValue() ) )
            suffix.Append( '_' );
        else
            suffix.Append( pname[i] );
    }

    return suffix;
}


bool SCH_EDIT_FRAME::RescueProject( bool aRunningOnDemand )
{
    RESCUER rescuer( *this, Prj() );

    rescuer.FindCandidates();

    if( ! rescuer.GetCandidateCount() )
    {
        if( aRunningOnDemand )
        {
            wxMessageDialog dlg( this, _( "This project has nothing to rescue." ),
                    _( "Project Rescue Helper" ) );
            dlg.ShowModal();
        }
        return true;
    }

    rescuer.RemoveDuplicates();

    rescuer.InvokeDialog( !aRunningOnDemand );

    // If no components were rescued, let the user know what's going on. He might
    // have clicked cancel by mistake, and should have some indication of that.
    if( !rescuer.GetChosenCandidateCount() )
    {
        wxMessageDialog dlg( this, _( "No symbols were rescued." ),
                _( "Project Rescue Helper" ) );
        dlg.ShowModal();

        // Set the modified flag even on Cancel. Many users seem to instinctively want to Save at
        // this point, due to the reloading of the symbols, so we'll make the save button active.
        OnModify();
        return true;
    }

    RESCUE_CACHE_CANDIDATE::OpenRescueLibrary();

    if( !rescuer.DoRescues() )
    {
        rescuer.UndoRescues();
        return false;
    }

    RESCUE_CACHE_CANDIDATE::WriteRescueLibrary( this, &Prj() );

    Prj().SetElem( PROJECT::ELEM_SCH_PART_LIBS, NULL );

    // Clean up wire ends
    INSTALL_UNBUFFERED_DC( dc, m_canvas );
    GetScreen()->SchematicCleanUp( NULL, &dc );
    m_canvas->Refresh( true );
    OnModify();

    return true;
}
