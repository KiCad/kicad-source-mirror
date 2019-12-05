/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015-2019 KiCad Developers, see change_log.txt for contributors.
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

#include <sch_draw_panel.h>
#include <class_library.h>
#include <confirm.h>
#include <invoke_sch_dialog.h>
#include <kiway.h>
#include <project_rescue.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_edit_frame.h>
#include <symbol_lib_table.h>
#include <viewlib_frame.h>
#include <wildcards_and_files_ext.h>

#include <cctype>
#include <map>


typedef std::pair<SCH_COMPONENT*, wxString> COMPONENT_NAME_PAIR;


// Helper sort function, used in get_components, to sort a component list by lib_id
static bool sort_by_libid( const SCH_COMPONENT* ref, SCH_COMPONENT* cmp )
{
    return ref->GetLibId() < cmp->GetLibId();
}


/**
 * Fill a vector with all of the project's symbols, to ease iterating over them.
 *
 * The list is sorted by #LIB_ID, therefore components using the same library
 * symbol are grouped, allowing later faster calculations (one library search by group
 * of symbols)
 *
 * @param aComponents - a vector that will take the symbols
 */
static void get_components( std::vector<SCH_COMPONENT*>& aComponents )
{
    SCH_SCREENS screens;

    // Get the full list
    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );
            aComponents.push_back( component );
        }
    }

    if( aComponents.empty() )
        return;

    // sort aComponents by lib part. Components will be grouped by same lib part.
    std::sort( aComponents.begin(), aComponents.end(), sort_by_libid );
}


/**
 * Search the libraries for the first component with a given name.
 *
 * @param aName - name to search for
 * @param aLibs - the loaded PART_LIBS
 * @param aCached - whether we are looking for the cached part
 */
static LIB_PART* find_component( const wxString& aName, PART_LIBS* aLibs, bool aCached )
{
    LIB_PART *part = NULL;
    wxString new_name = LIB_ID::FixIllegalChars( aName, LIB_ID::ID_SCH );

    for( PART_LIB& each_lib : *aLibs )
    {
        if( aCached && !each_lib.IsCache() )
            continue;

        if( !aCached && each_lib.IsCache() )
            continue;

        part = each_lib.FindPart( new_name );

        if( part )
            break;
    }

    return part;
}


static wxFileName GetRescueLibraryFileName()
{
    wxFileName fn( g_RootSheet->GetScreen()->GetFileName() );
    fn.SetName( fn.GetName() + wxT( "-rescue" ) );
    fn.SetExt( SchematicLibraryFileExtension );
    return fn;
}


RESCUE_CASE_CANDIDATE::RESCUE_CASE_CANDIDATE( const wxString& aRequestedName,
                                              const wxString& aNewName,
                                              LIB_PART* aLibCandidate )
{
    m_requested_name = aRequestedName;
    m_new_name = aNewName;
    m_lib_candidate = aLibCandidate;
}


void RESCUE_CASE_CANDIDATE::FindRescues( RESCUER& aRescuer,
                                         boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
{
    typedef std::map<wxString, RESCUE_CASE_CANDIDATE> candidate_map_t;
    candidate_map_t candidate_map;
    // Remember the list of components is sorted by part name.
    // So a search in libraries is made only once by group
    LIB_ALIAS* case_sensitive_match = nullptr;
    std::vector<LIB_ALIAS*> case_insensitive_matches;

    wxString last_part_name;

    for( SCH_COMPONENT* each_component : *( aRescuer.GetComponents() ) )
    {
        wxString part_name = each_component->GetLibId().GetLibItemName();

        if( last_part_name != part_name )
        {
            // A new part name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            last_part_name = part_name;
            case_insensitive_matches.clear();

            LIB_ID id( wxEmptyString, part_name );

            case_sensitive_match = aRescuer.GetPrj()->SchLibs()->FindLibraryAlias( id );

            if( !case_sensitive_match )
                // the case sensitive match failed. Try a case insensitive match
                aRescuer.GetPrj()->SchLibs()->FindLibraryNearEntries( case_insensitive_matches,
                                                                      part_name );
        }

        if( case_sensitive_match || !( case_insensitive_matches.size() ) )
            continue;

        RESCUE_CASE_CANDIDATE candidate( part_name, case_insensitive_matches[0]->GetName(),
                                         case_insensitive_matches[0]->GetPart() );

        candidate_map[part_name] = candidate;
    }

    // Now, dump the map into aCandidates
    for( const candidate_map_t::value_type& each_pair : candidate_map )
    {
        aCandidates.push_back( new RESCUE_CASE_CANDIDATE( each_pair.second ) );
    }
}


wxString RESCUE_CASE_CANDIDATE::GetActionDescription() const
{
    wxString action;
    action.Printf( _( "Rename to %s" ), m_new_name );
    return action;
}


bool RESCUE_CASE_CANDIDATE::PerformAction( RESCUER* aRescuer )
{
    for( SCH_COMPONENT* each_component : *aRescuer->GetComponents() )
    {
        if( each_component->GetLibId().GetLibItemName() != UTF8( m_requested_name ) )
            continue;

        LIB_ID libId;

        libId.SetLibItemName( m_new_name, false );
        each_component->SetLibId( libId );
        each_component->ClearFlags();
        aRescuer->LogRescue( each_component, m_requested_name, m_new_name );
    }

    return true;
}


RESCUE_CACHE_CANDIDATE::RESCUE_CACHE_CANDIDATE( const wxString& aRequestedName,
                                                const wxString& aNewName,
                                                LIB_PART* aCacheCandidate,
                                                LIB_PART* aLibCandidate )
{
    m_requested_name = aRequestedName;
    m_new_name = aNewName;
    m_cache_candidate = aCacheCandidate;
    m_lib_candidate = aLibCandidate;
}


RESCUE_CACHE_CANDIDATE::RESCUE_CACHE_CANDIDATE()
{
    m_cache_candidate = NULL;
    m_lib_candidate = NULL;
}


void RESCUE_CACHE_CANDIDATE::FindRescues( RESCUER& aRescuer,
                                          boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
{
    typedef std::map<wxString, RESCUE_CACHE_CANDIDATE> candidate_map_t;
    candidate_map_t candidate_map;

    // Remember the list of components is sorted by part name.
    // So a search in libraries is made only once by group
    LIB_PART* cache_match = nullptr;
    LIB_PART* lib_match = nullptr;
    wxString old_part_name;

    for( SCH_COMPONENT* each_component : *( aRescuer.GetComponents() ) )
    {
        wxString part_name = each_component->GetLibId().GetLibItemName();

        if( old_part_name != part_name )
        {
            // A new part name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            old_part_name = part_name;
            cache_match = find_component( part_name, aRescuer.GetPrj()->SchLibs(), true );
            lib_match = find_component( part_name, aRescuer.GetPrj()->SchLibs(), false );

            if( !cache_match && !lib_match )
                continue;

            // Test whether there is a conflict or if the symbol can only be found in the cache
            // and the symbol name does not have any illegal characters.
            if( LIB_ID::HasIllegalChars( part_name, LIB_ID::ID_SCH ) == -1 )
            {
                if( cache_match && lib_match &&
                    !cache_match->PinsConflictWith( *lib_match, true, true, true, true, false ) )
                    continue;

                if( !cache_match && lib_match )
                    continue;
            }

            // Check if the symbol has already been rescued.
            wxString new_name = LIB_ID::FixIllegalChars( part_name, LIB_ID::ID_SCH );

            RESCUE_CACHE_CANDIDATE candidate( part_name, new_name, cache_match, lib_match );

            candidate_map[part_name] = candidate;
        }
    }

    // Now, dump the map into aCandidates
    for( const candidate_map_t::value_type& each_pair : candidate_map )
    {
        aCandidates.push_back( new RESCUE_CACHE_CANDIDATE( each_pair.second ) );
    }
}


wxString RESCUE_CACHE_CANDIDATE::GetActionDescription() const
{
    wxString action;

    if( !m_cache_candidate && !m_lib_candidate )
        action.Printf( _( "Cannot rescue symbol %s which is not available in any library or "
                          "the cache." ), m_requested_name );
    else if( m_cache_candidate && !m_lib_candidate )
        action.Printf( _( "Rescue symbol %s found only in cache library to %s." ),
                       m_requested_name, m_new_name );
    else
        action.Printf( _( "Rescue modified symbol %s to %s" ),
                       m_requested_name, m_new_name );

    return action;
}


bool RESCUE_CACHE_CANDIDATE::PerformAction( RESCUER* aRescuer )
{
    LIB_PART* tmp = ( m_cache_candidate ) ? m_cache_candidate : m_lib_candidate;

    wxCHECK_MSG( tmp, false, "Both cache and library symbols undefined." );

    LIB_PART new_part( *tmp );
    new_part.SetName( m_new_name );
    new_part.RemoveAllAliases();
    aRescuer->AddPart( &new_part );

    for( SCH_COMPONENT* each_component : *aRescuer->GetComponents() )
    {
        if( each_component->GetLibId().GetLibItemName() != UTF8( m_requested_name ) )
            continue;

        LIB_ID libId;

        libId.SetLibItemName( m_new_name, false );
        each_component->SetLibId( libId );
        each_component->ClearFlags();
        aRescuer->LogRescue( each_component, m_requested_name, m_new_name );
    }

    return true;
}


RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::RESCUE_SYMBOL_LIB_TABLE_CANDIDATE(
    const LIB_ID& aRequestedId,
    const LIB_ID& aNewId,
    LIB_PART* aCacheCandidate,
    LIB_PART* aLibCandidate ) : RESCUE_CANDIDATE()
{
    m_requested_id = aRequestedId;
    m_requested_name = aRequestedId.Format();
    m_new_id = aNewId;
    m_lib_candidate = aLibCandidate;
    m_cache_candidate = aCacheCandidate;
}


RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::RESCUE_SYMBOL_LIB_TABLE_CANDIDATE()
{
    m_cache_candidate = NULL;
    m_lib_candidate = NULL;
}


void RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::FindRescues(
    RESCUER& aRescuer,
    boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
{
    typedef std::map<LIB_ID, RESCUE_SYMBOL_LIB_TABLE_CANDIDATE> candidate_map_t;

    candidate_map_t candidate_map;

    // Remember the list of components is sorted by LIB_ID.
    // So a search in libraries is made only once by group
    LIB_PART* cache_match = nullptr;
    LIB_PART* lib_match = nullptr;
    LIB_ID old_part_id;

    for( SCH_COMPONENT* each_component : *( aRescuer.GetComponents() ) )
    {
        const LIB_ID& part_id = each_component->GetLibId();

        if( old_part_id != part_id )
        {
            // A new part name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            old_part_id = part_id;
            cache_match = find_component( part_id.Format().wx_str(), aRescuer.GetPrj()->SchLibs(),
                                          true );

            lib_match = SchGetLibPart( part_id, aRescuer.GetPrj()->SchSymbolLibTable() );

            if( !cache_match && !lib_match )
                continue;

            // Test whether there is a conflict or if the symbol can only be found in the cache.
            if( LIB_ID::HasIllegalChars( part_id.GetLibItemName(), LIB_ID::ID_SCH ) == -1 )
            {
                if( cache_match && lib_match &&
                    !cache_match->PinsConflictWith( *lib_match, true, true, true, true, false ) )
                    continue;

                if( !cache_match && lib_match )
                    continue;
            }

            // Fix illegal LIB_ID name characters.
            wxString new_name = LIB_ID::FixIllegalChars( part_id.GetLibItemName(), LIB_ID::ID_SCH );

            // Differentiate symbol name in the rescue library by appending the symbol library
            // table nickname to the symbol name to prevent name clashes in the rescue library.
            wxString libNickname = GetRescueLibraryFileName().GetName();

            // Spaces in the file name will break the symbol name because they are not
            // quoted in the symbol library file format.
            libNickname.Replace( " ", "-" );
            LIB_ID new_id( libNickname, new_name + "-" + part_id.GetLibNickname().wx_str() );

            RESCUE_SYMBOL_LIB_TABLE_CANDIDATE candidate( part_id, new_id, cache_match, lib_match );

            candidate_map[part_id] = candidate;
        }
    }

    // Now, dump the map into aCandidates
    for( const candidate_map_t::value_type& each_pair : candidate_map )
    {
        aCandidates.push_back( new RESCUE_SYMBOL_LIB_TABLE_CANDIDATE( each_pair.second ) );
    }
}


wxString RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::GetActionDescription() const
{
    wxString action;

    if( !m_cache_candidate && !m_lib_candidate )
        action.Printf( _( "Cannot rescue symbol %s which is not available in any library or "
                          "the cache." ), m_requested_id.GetLibItemName().wx_str() );
    else if( m_cache_candidate && !m_lib_candidate )
        action.Printf( _( "Rescue symbol %s found only in cache library to %s." ),
                       m_requested_id.Format().wx_str(), m_new_id.Format().wx_str() );
    else
        action.Printf( _( "Rescue modified symbol %s to %s" ),
                       m_requested_id.Format().wx_str(), m_new_id.Format().wx_str() );

    return action;
}


bool RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::PerformAction( RESCUER* aRescuer )
{
    LIB_PART* tmp = ( m_cache_candidate ) ? m_cache_candidate : m_lib_candidate;

    wxCHECK_MSG( tmp, false, "Both cache and library symbols undefined." );

    LIB_PART new_part( *tmp );
    new_part.SetLibId( m_new_id );
    new_part.SetName( m_new_id.GetLibItemName() );
    new_part.RemoveAllAliases();
    aRescuer->AddPart( &new_part );

    for( SCH_COMPONENT* each_component : *aRescuer->GetComponents() )
    {
        if( each_component->GetLibId() != m_requested_id )
            continue;

        each_component->SetLibId( m_new_id );
        each_component->ClearFlags();
        aRescuer->LogRescue( each_component, m_requested_id.Format(), m_new_id.Format() );
    }

    return true;
}


RESCUER::RESCUER( PROJECT& aProject, SCH_SHEET_PATH* aCurrentSheet,
                  EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType )
{
    get_components( m_components );
    m_prj = &aProject;
    m_currentSheet = aCurrentSheet;
    m_galBackEndType = aGalBackEndType;
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
    for( RESCUE_CANDIDATE* each_candidate : m_chosen_candidates )
    {
        if( ! each_candidate->PerformAction( this ) )
            return false;
    }

    return true;
}


void RESCUER::UndoRescues()
{
    for( RESCUE_LOG& each_logitem : m_rescue_log )
    {
        LIB_ID libId;

        libId.SetLibItemName( each_logitem.old_name, false );
        each_logitem.component->SetLibId( libId );
        each_logitem.component->ClearFlags();
    }
}


bool SCH_EDIT_FRAME::RescueLegacyProject( bool aRunningOnDemand )
{
    LEGACY_RESCUER rescuer( Prj(), &GetCurrentSheet(), GetCanvas()->GetBackend() );

    return rescueProject( rescuer, aRunningOnDemand );
}


bool SCH_EDIT_FRAME::RescueSymbolLibTableProject( bool aRunningOnDemand )
{
    SYMBOL_LIB_TABLE_RESCUER rescuer( Prj(), &GetCurrentSheet(), GetCanvas()->GetBackend() );

    return rescueProject( rescuer, aRunningOnDemand );
}


bool SCH_EDIT_FRAME::rescueProject( RESCUER& aRescuer, bool aRunningOnDemand )
{
    if( !RESCUER::RescueProject( this, aRescuer, aRunningOnDemand ) )
        return false;

    if( aRescuer.GetCandidateCount() )
    {
        LIB_VIEW_FRAME* viewer = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, false );

        if( viewer )
            viewer->ReCreateListLib();

        GetScreen()->ClearUndoORRedoList( GetScreen()->m_UndoList, 1 );
        SyncView();
        GetCanvas()->Refresh();
        OnModify();
    }

    return true;
}


bool RESCUER::RescueProject( wxWindow* aParent, RESCUER& aRescuer, bool aRunningOnDemand )
{
    aRescuer.FindCandidates();

    if( !aRescuer.GetCandidateCount() )
    {
        if( aRunningOnDemand )
        {
            wxMessageDialog dlg( aParent, _( "This project has nothing to rescue." ),
                                 _( "Project Rescue Helper" ) );
            dlg.ShowModal();
        }

        return true;
    }

    aRescuer.RemoveDuplicates();
    aRescuer.InvokeDialog( aParent, !aRunningOnDemand );

    // If no symbols were rescued, let the user know what's going on. He might
    // have clicked cancel by mistake, and should have some indication of that.
    if( !aRescuer.GetChosenCandidateCount() )
    {
        wxMessageDialog dlg( aParent, _( "No symbols were rescued." ),
                             _( "Project Rescue Helper" ) );
        dlg.ShowModal();

        // Set the modified flag even on Cancel. Many users seem to instinctively want to Save at
        // this point, due to the reloading of the symbols, so we'll make the save button active.
        return true;
    }

    aRescuer.OpenRescueLibrary();

    if( !aRescuer.DoRescues() )
    {
        aRescuer.UndoRescues();
        return false;
    }

    aRescuer.WriteRescueLibrary( aParent );

    return true;
}


void RESCUER::RemoveDuplicates()
{
    std::vector<wxString> names_seen;

    for( boost::ptr_vector<RESCUE_CANDIDATE>::iterator it = m_all_candidates.begin();
         it != m_all_candidates.end(); )
    {
        bool seen_already = false;

        for( wxString& name_seen : names_seen )
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


void LEGACY_RESCUER::FindCandidates()
{
    RESCUE_CASE_CANDIDATE::FindRescues( *this, m_all_candidates );
    RESCUE_CACHE_CANDIDATE::FindRescues( *this, m_all_candidates );
}


void LEGACY_RESCUER::InvokeDialog( wxWindow* aParent, bool aAskShowAgain )
{
    InvokeDialogRescueEach( aParent, static_cast< RESCUER& >( *this ), m_currentSheet,
                            m_galBackEndType, aAskShowAgain );
}


void LEGACY_RESCUER::OpenRescueLibrary()
{
    wxFileName fn = GetRescueLibraryFileName();

    std::unique_ptr<PART_LIB> rescue_lib( new PART_LIB( LIBRARY_TYPE_EESCHEMA, fn.GetFullPath() ) );

    m_rescue_lib = std::move( rescue_lib );
    m_rescue_lib->EnableBuffering();

    // If a rescue library already exists copy the contents of that library so we do not
    // lose an previous rescues.
    PART_LIB* rescueLib = m_prj->SchLibs()->FindLibrary( fn.GetName() );

    if( rescueLib )
    {
        // For items in the rescue library, aliases are the root symbol.
        std::vector< LIB_ALIAS* > aliases;

        rescueLib->GetAliases( aliases );

        for( auto alias : aliases )
        {
            LIB_PART* part = alias->GetPart();

            wxCHECK2( part, continue );

            m_rescue_lib->AddPart( new LIB_PART( *part, m_rescue_lib.get() ) );
        }
    }
}


bool LEGACY_RESCUER::WriteRescueLibrary( wxWindow *aParent )
{
    try
    {
        m_rescue_lib->Save( false );
    }
    catch( ... /* IO_ERROR ioe */ )
    {
        wxString msg;

        msg.Printf( _( "Failed to create symbol library file \"%s\"" ),
                    m_rescue_lib->GetFullFileName() );
        DisplayError( aParent, msg );
        return false;
    }

    wxArrayString libNames;
    wxString libPaths;

    wxString libName = m_rescue_lib->GetName();
    PART_LIBS *libs = dynamic_cast<PART_LIBS*>( m_prj->GetElem( PROJECT::ELEM_SCH_PART_LIBS ) );

    if( !libs )
    {
        libs = new PART_LIBS();
        m_prj->SetElem( PROJECT::ELEM_SCH_PART_LIBS, libs );
    }

    try
    {
        PART_LIBS::LibNamesAndPaths( m_prj, false, &libPaths, &libNames );

        // Make sure the library is not already in the list
        while( libNames.Index( libName ) != wxNOT_FOUND )
            libNames.Remove( libName );

        // Add the library to the top of the list and save.
        libNames.Insert( libName, 0 );
        PART_LIBS::LibNamesAndPaths( m_prj, true, &libPaths, &libNames );
    }
    catch( const IO_ERROR& )
    {
        // Could not get or save the current libraries.
        return false;
    }

    // Save the old libraries in case there is a problem after clear(). We'll
    // put them back in.
    boost::ptr_vector<PART_LIB> libsSave;
    libsSave.transfer( libsSave.end(), libs->begin(), libs->end(), *libs );

    m_prj->SetElem( PROJECT::ELEM_SCH_PART_LIBS, NULL );

    libs = new PART_LIBS();

    try
    {
        libs->LoadAllLibraries( m_prj );
    }
    catch( const PARSE_ERROR& )
    {
        // Some libraries were not found. There's no point in showing the error,
        // because it was already shown. Just don't do anything.
    }
    catch( const IO_ERROR& )
    {
        // Restore the old list
        libs->clear();
        libs->transfer( libs->end(), libsSave.begin(), libsSave.end(), libsSave );
        return false;
    }

    m_prj->SetElem( PROJECT::ELEM_SCH_PART_LIBS, libs );

    // Update the schematic symbol library links since the library list has changed.
    SCH_SCREENS schematic;

    schematic.UpdateSymbolLinks();

    return true;
}


void LEGACY_RESCUER::AddPart( LIB_PART* aNewPart )
{
    wxCHECK_RET( aNewPart, "Invalid LIB_PART pointer." );

    aNewPart->SetLib( m_rescue_lib.get() );
    m_rescue_lib->AddPart( aNewPart );
}


SYMBOL_LIB_TABLE_RESCUER::SYMBOL_LIB_TABLE_RESCUER( PROJECT& aProject,
                                                    SCH_SHEET_PATH* aCurrentSheet,
                                                    EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType ) :
    RESCUER( aProject, aCurrentSheet, aGalBackEndType )
{
    m_properties = std::make_unique<PROPERTIES>();
}


void SYMBOL_LIB_TABLE_RESCUER::FindCandidates()
{
    RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::FindRescues( *this, m_all_candidates );
}


void SYMBOL_LIB_TABLE_RESCUER::InvokeDialog( wxWindow* aParent, bool aAskShowAgain )
{
    InvokeDialogRescueEach( aParent, static_cast< RESCUER& >( *this ), m_currentSheet,
                            m_galBackEndType, aAskShowAgain );
}


void SYMBOL_LIB_TABLE_RESCUER::OpenRescueLibrary()
{
    m_pi.set( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );
    (*m_properties)[ SCH_LEGACY_PLUGIN::PropBuffering ] = "";
}


bool SYMBOL_LIB_TABLE_RESCUER::WriteRescueLibrary( wxWindow *aParent )
{
    wxString msg;
    wxFileName fn = GetRescueLibraryFileName();

    // If the rescue library already exists in the symbol library table no need save it to add
    // it to the table.
    if( !m_prj->SchSymbolLibTable()->HasLibrary( fn.GetName() ) )
    {
        try
        {
            m_pi->SaveLibrary( fn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Failed to save rescue library %s." ), fn.GetFullPath() );
            DisplayErrorMessage( aParent, msg, ioe.What() );
            return false;
        }

        wxString uri = "${KIPRJMOD}/" + fn.GetFullName();
        wxString libNickname = fn.GetName();

        // Spaces in the file name will break the symbol name because they are not
        // quoted in the symbol library file format.
        libNickname.Replace( " ", "-" );

        SYMBOL_LIB_TABLE_ROW* row = new SYMBOL_LIB_TABLE_ROW( libNickname, uri,
                                                              wxString( "Legacy" ) );
        m_prj->SchSymbolLibTable()->InsertRow( row );

        fn = wxFileName( m_prj->GetProjectPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        try
        {
            m_prj->SchSymbolLibTable()->Save( fn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error occurred saving project specific symbol library table." ) );
            DisplayErrorMessage( aParent, msg, ioe.What() );
            return false;
        }
    }

    // Relaod the symbol library table.
    m_prj->SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, NULL );

    // This can only happen if the symbol library table file was currupted on write.
    if( !m_prj->SchSymbolLibTable() )
        return false;

    // Update the schematic symbol library links since the library list has changed.
    SCH_SCREENS schematic;

    schematic.UpdateSymbolLinks( true );
    return true;
}


void SYMBOL_LIB_TABLE_RESCUER::AddPart( LIB_PART* aNewPart )
{
    wxCHECK_RET( aNewPart, "Invalid LIB_PART pointer." );

    wxFileName fn = GetRescueLibraryFileName();

    try
    {
        if( !m_prj->SchSymbolLibTable()->HasLibrary( fn.GetName() ) )
            m_pi->SaveSymbol( fn.GetFullPath(), new LIB_PART( *aNewPart ), m_properties.get() );
        else
            m_prj->SchSymbolLibTable()->SaveSymbol( fn.GetName(), new LIB_PART( *aNewPart ) );
    }
    catch( ... /* IO_ERROR */ )
    {
    }
}
