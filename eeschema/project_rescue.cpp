/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015-2021 KiCad Developers, see change_log.txt for contributors.
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
#include <symbol_library.h>
#include <confirm.h>
#include <connection_graph.h>
#include <invoke_sch_dialog.h>
#include <kiway.h>
#include <symbol_viewer_frame.h>
#include <project_rescue.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <symbol_lib_table.h>
#include <wildcards_and_files_ext.h>

#include <cctype>
#include <map>


typedef std::pair<SCH_SYMBOL*, wxString> SYMBOL_NAME_PAIR;


// Helper sort function, used in getSymbols, to sort a symbol list by lib_id
static bool sort_by_libid( const SCH_SYMBOL* ref, SCH_SYMBOL* cmp )
{
    return ref->GetLibId() < cmp->GetLibId();
}


/**
 * Fill a vector with all of the project's symbols, to ease iterating over them.
 *
 * The list is sorted by #LIB_ID, therefore symbols using the same library
 * symbol are grouped, allowing later faster calculations (one library search by group
 * of symbols)
 *
 * @param aSymbols is a vector that will take the symbols.
 */
static void getSymbols( SCHEMATIC* aSchematic, std::vector<SCH_SYMBOL*>& aSymbols )
{
    SCH_SCREENS screens( aSchematic->Root() );

    // Get the full list
    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( auto aItem : screen->Items().OfType( SCH_SYMBOL_T ) )
            aSymbols.push_back( static_cast<SCH_SYMBOL*>( aItem ) );
    }

    if( aSymbols.empty() )
        return;

    // sort aSymbols by lib symbol. symbols will be grouped by same lib symbol.
    std::sort( aSymbols.begin(), aSymbols.end(), sort_by_libid );
}


/**
 * Search the libraries for the first symbol with a given name.
 *
 * @param aName - name to search for
 * @param aLibs - the loaded SYMBOL_LIBS
 * @param aCached - whether we are looking for the cached symbol
 */
static LIB_SYMBOL* findSymbol( const wxString& aName, SYMBOL_LIBS* aLibs, bool aCached )
{
    LIB_SYMBOL *symbol = NULL;
    wxString new_name = LIB_ID::FixIllegalChars( aName );

    for( SYMBOL_LIB& each_lib : *aLibs )
    {
        if( aCached && !each_lib.IsCache() )
            continue;

        if( !aCached && each_lib.IsCache() )
            continue;

        symbol = each_lib.FindSymbol( new_name );

        if( symbol )
            break;
    }

    return symbol;
}


static wxFileName GetRescueLibraryFileName( SCHEMATIC* aSchematic )
{
    wxFileName fn = aSchematic->GetFileName();
    fn.SetName( fn.GetName() + wxT( "-rescue" ) );
    fn.SetExt( LegacySymbolLibFileExtension );
    return fn;
}


RESCUE_CASE_CANDIDATE::RESCUE_CASE_CANDIDATE( const wxString& aRequestedName,
                                              const wxString& aNewName,
                                              LIB_SYMBOL* aLibCandidate,
                                              int aUnit,
                                              int aConvert )
{
    m_requested_name = aRequestedName;
    m_new_name = aNewName;
    m_lib_candidate = aLibCandidate;
    m_unit = aUnit;
    m_convert = aConvert;
}


void RESCUE_CASE_CANDIDATE::FindRescues( RESCUER& aRescuer,
                                         boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
{
    typedef std::map<wxString, RESCUE_CASE_CANDIDATE> candidate_map_t;
    candidate_map_t candidate_map;

    // Remember the list of symbols is sorted by symbol name.
    // So a search in libraries is made only once by group
    LIB_SYMBOL* case_sensitive_match = nullptr;
    std::vector<LIB_SYMBOL*> case_insensitive_matches;

    wxString symbol_name;
    wxString search_name;
    wxString last_symbol_name;

    for( SCH_SYMBOL* eachSymbol : *( aRescuer.GetSymbols() ) )
    {
        symbol_name = eachSymbol->GetLibId().GetLibItemName();
        search_name = LIB_ID::FixIllegalChars( symbol_name );

        if( last_symbol_name != symbol_name )
        {
            // A new symbol name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            last_symbol_name = symbol_name;
            case_insensitive_matches.clear();

            LIB_ID id( wxEmptyString, search_name );

            case_sensitive_match = aRescuer.GetPrj()->SchLibs()->FindLibSymbol( id );

            // If the case sensitive match failed, try a case insensitive match.
            if( !case_sensitive_match )
                aRescuer.GetPrj()->SchLibs()->FindLibraryNearEntries( case_insensitive_matches,
                                                                      search_name );
        }

        if( case_sensitive_match || !( case_insensitive_matches.size() ) )
            continue;

        RESCUE_CASE_CANDIDATE candidate( symbol_name, case_insensitive_matches[0]->GetName(),
                                         case_insensitive_matches[0],
                                         eachSymbol->GetUnit(),
                                         eachSymbol->GetConvert() );

        candidate_map[symbol_name] = candidate;
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
    action.Printf( _( "Rename %s to %s" ), m_requested_name, m_new_name );
    return action;
}


bool RESCUE_CASE_CANDIDATE::PerformAction( RESCUER* aRescuer )
{
    for( SCH_SYMBOL* eachSymbol : *aRescuer->GetSymbols() )
    {
        if( eachSymbol->GetLibId().GetLibItemName() != UTF8( m_requested_name ) )
            continue;

        LIB_ID libId;

        libId.SetLibItemName( m_new_name, false );
        eachSymbol->SetLibId( libId );
        eachSymbol->ClearFlags();
        aRescuer->LogRescue( eachSymbol, m_requested_name, m_new_name );
    }

    return true;
}


RESCUE_CACHE_CANDIDATE::RESCUE_CACHE_CANDIDATE( const wxString& aRequestedName,
                                                const wxString& aNewName,
                                                LIB_SYMBOL* aCacheCandidate,
                                                LIB_SYMBOL* aLibCandidate,
                                                int aUnit,
                                                int aConvert )
{
    m_requested_name = aRequestedName;
    m_new_name = aNewName;
    m_cache_candidate = aCacheCandidate;
    m_lib_candidate = aLibCandidate;
    m_unit = aUnit;
    m_convert = aConvert;
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

    // Remember the list of symbols is sorted by symbol name.
    // So a search in libraries is made only once by group
    LIB_SYMBOL* cache_match = nullptr;
    LIB_SYMBOL* lib_match = nullptr;
    wxString symbol_name;
    wxString search_name;
    wxString old_symbol_name;

    for( SCH_SYMBOL* eachSymbol : *( aRescuer.GetSymbols() ) )
    {
        symbol_name = eachSymbol->GetLibId().GetLibItemName();
        search_name = LIB_ID::FixIllegalChars( symbol_name );

        if( old_symbol_name != symbol_name )
        {
            // A new symbol name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            old_symbol_name = symbol_name;
            cache_match = findSymbol( search_name, aRescuer.GetPrj()->SchLibs(), true );
            lib_match = findSymbol( search_name, aRescuer.GetPrj()->SchLibs(), false );

            if( !cache_match && !lib_match )
                continue;

            // Test whether there is a conflict or if the symbol can only be found in the cache
            // and the symbol name does not have any illegal characters.
            if( cache_match && lib_match &&
                !cache_match->PinsConflictWith( *lib_match, true, true, true, true, false ) )
                continue;

            if( !cache_match && lib_match )
                continue;

            // Check if the symbol has already been rescued.
            RESCUE_CACHE_CANDIDATE candidate( symbol_name, search_name, cache_match, lib_match,
                                              eachSymbol->GetUnit(),
                                              eachSymbol->GetConvert() );

            candidate_map[symbol_name] = candidate;
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
    LIB_SYMBOL* tmp = ( m_cache_candidate ) ? m_cache_candidate : m_lib_candidate;

    wxCHECK_MSG( tmp, false, "Both cache and library symbols undefined." );

    std::unique_ptr<LIB_SYMBOL> new_symbol = tmp->Flatten();
    new_symbol->SetName( m_new_name );
    aRescuer->AddSymbol( new_symbol.get() );

    for( SCH_SYMBOL* eachSymbol : *aRescuer->GetSymbols() )
    {
        if( eachSymbol->GetLibId().GetLibItemName() != UTF8( m_requested_name ) )
            continue;

        LIB_ID libId;

        libId.SetLibItemName( m_new_name, false );
        eachSymbol->SetLibId( libId );
        eachSymbol->ClearFlags();
        aRescuer->LogRescue( eachSymbol, m_requested_name, m_new_name );
    }

    return true;
}


RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::RESCUE_SYMBOL_LIB_TABLE_CANDIDATE(
    const LIB_ID& aRequestedId,
    const LIB_ID& aNewId,
    LIB_SYMBOL* aCacheCandidate,
    LIB_SYMBOL* aLibCandidate,
    int aUnit,
    int aConvert ) : RESCUE_CANDIDATE()
{
    m_requested_id = aRequestedId;
    m_requested_name = aRequestedId.Format();
    m_new_id = aNewId;
    m_lib_candidate = aLibCandidate;
    m_cache_candidate = aCacheCandidate;
    m_unit = aUnit;
    m_convert = aConvert;
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

    // Remember the list of symbols is sorted by LIB_ID.
    // So a search in libraries is made only once by group
    LIB_SYMBOL* cache_match = nullptr;
    LIB_SYMBOL* lib_match = nullptr;
    LIB_ID old_symbol_id;

    for( SCH_SYMBOL* eachSymbol : *( aRescuer.GetSymbols() ) )
    {
        const LIB_ID& symbol_id = eachSymbol->GetLibId();

        if( old_symbol_id != symbol_id )
        {
            // A new symbol name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            old_symbol_id = symbol_id;

            // Get the library symbol from the cache library.  It will be a flattened
            // symbol by default (no inheritance).
            cache_match = findSymbol( symbol_id.Format().wx_str(), aRescuer.GetPrj()->SchLibs(),
                                      true );

            // Get the library symbol from the symbol library table.
            lib_match = SchGetLibPart( symbol_id, aRescuer.GetPrj()->SchSymbolLibTable() );

            if( !cache_match && !lib_match )
                continue;

            LIB_SYMBOL_SPTR lib_match_parent;

            // If it's a derive symbol, use the parent symbol to perform the pin test.
            if( lib_match && lib_match->IsAlias() )
            {
                lib_match_parent = lib_match->GetParent().lock();

                if( !lib_match_parent )
                {
                    lib_match = nullptr;
                }
                else
                {
                    lib_match = lib_match_parent.get();
                }
            }

            // Test whether there is a conflict or if the symbol can only be found in the cache.
            if( LIB_ID::HasIllegalChars( symbol_id.GetLibItemName() ) == -1 )
            {
                if( cache_match && lib_match &&
                    !cache_match->PinsConflictWith( *lib_match, true, true, true, true, false ) )
                    continue;

                if( !cache_match && lib_match )
                    continue;
            }

            // Fix illegal LIB_ID name characters.
            wxString new_name = LIB_ID::FixIllegalChars( symbol_id.GetLibItemName() );

            // Differentiate symbol name in the rescue library by appending the symbol library
            // table nickname to the symbol name to prevent name clashes in the rescue library.
            wxString libNickname = GetRescueLibraryFileName( aRescuer.Schematic() ).GetName();

            // Spaces in the file name will break the symbol name because they are not
            // quoted in the symbol library file format.
            libNickname.Replace( " ", "-" );
            LIB_ID new_id( libNickname, new_name + "-" + symbol_id.GetLibNickname().wx_str() );

            RESCUE_SYMBOL_LIB_TABLE_CANDIDATE candidate( symbol_id, new_id, cache_match, lib_match,
                                                         eachSymbol->GetUnit(),
                                                         eachSymbol->GetConvert() );

            candidate_map[symbol_id] = candidate;
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
    LIB_SYMBOL* tmp = ( m_cache_candidate ) ? m_cache_candidate : m_lib_candidate;

    wxCHECK_MSG( tmp, false, "Both cache and library symbols undefined." );

    std::unique_ptr<LIB_SYMBOL> new_symbol = tmp->Flatten();
    new_symbol->SetLibId( m_new_id );
    new_symbol->SetName( m_new_id.GetLibItemName() );
    aRescuer->AddSymbol( new_symbol.get() );

    for( SCH_SYMBOL* eachSymbol : *aRescuer->GetSymbols() )
    {
        if( eachSymbol->GetLibId() != m_requested_id )
            continue;

        eachSymbol->SetLibId( m_new_id );
        eachSymbol->ClearFlags();
        aRescuer->LogRescue( eachSymbol, m_requested_id.Format(), m_new_id.Format() );
    }

    return true;
}


RESCUER::RESCUER( PROJECT& aProject, SCHEMATIC* aSchematic, SCH_SHEET_PATH* aCurrentSheet,
                  EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType )
{
    m_schematic = aSchematic ? aSchematic : aCurrentSheet->LastScreen()->Schematic();

    wxASSERT( m_schematic );

    if( m_schematic )
        getSymbols( m_schematic, m_symbols );

    m_prj = &aProject;
    m_currentSheet = aCurrentSheet;
    m_galBackEndType = aGalBackEndType;
}


void RESCUER::LogRescue( SCH_SYMBOL* aSymbol, const wxString &aOldName,
                         const wxString &aNewName )
{
    RESCUE_LOG logitem;
    logitem.symbol = aSymbol;
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
        each_logitem.symbol->SetLibId( libId );
        each_logitem.symbol->ClearFlags();
    }
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
    wxFileName fn = GetRescueLibraryFileName( m_schematic );

    std::unique_ptr<SYMBOL_LIB> rescue_lib = std::make_unique<SYMBOL_LIB>( SCH_LIB_TYPE::LT_EESCHEMA,
                                                                       fn.GetFullPath() );

    m_rescue_lib = std::move( rescue_lib );
    m_rescue_lib->EnableBuffering();

    // If a rescue library already exists copy the contents of that library so we do not
    // lose an previous rescues.
    SYMBOL_LIB* rescueLib = m_prj->SchLibs()->FindLibrary( fn.GetName() );

    if( rescueLib )
    {
        // For items in the rescue library, aliases are the root symbol.
        std::vector< LIB_SYMBOL* > symbols;

        rescueLib->GetSymbols( symbols );

        for( auto symbol : symbols )
        {
            // The LIB_SYMBOL copy constructor flattens derived symbols (formerly known as aliases).
            m_rescue_lib->AddSymbol( new LIB_SYMBOL( *symbol, m_rescue_lib.get() ) );
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

        msg.Printf( _( "Failed to create symbol library file '%s'." ),
                    m_rescue_lib->GetFullFileName() );
        DisplayError( aParent, msg );
        return false;
    }

    wxArrayString libNames;
    wxString libPaths;

    wxString libName = m_rescue_lib->GetName();
    SYMBOL_LIBS *libs = dynamic_cast<SYMBOL_LIBS*>( m_prj->GetElem( PROJECT::ELEM_SCH_SYMBOL_LIBS ) );

    if( !libs )
    {
        libs = new SYMBOL_LIBS();
        m_prj->SetElem( PROJECT::ELEM_SCH_SYMBOL_LIBS, libs );
    }

    try
    {
        SYMBOL_LIBS::LibNamesAndPaths( m_prj, false, &libPaths, &libNames );

        // Make sure the library is not already in the list
        while( libNames.Index( libName ) != wxNOT_FOUND )
            libNames.Remove( libName );

        // Add the library to the top of the list and save.
        libNames.Insert( libName, 0 );
        SYMBOL_LIBS::LibNamesAndPaths( m_prj, true, &libPaths, &libNames );
    }
    catch( const IO_ERROR& )
    {
        // Could not get or save the current libraries.
        return false;
    }

    // Save the old libraries in case there is a problem after clear(). We'll
    // put them back in.
    boost::ptr_vector<SYMBOL_LIB> libsSave;
    libsSave.transfer( libsSave.end(), libs->begin(), libs->end(), *libs );

    m_prj->SetElem( PROJECT::ELEM_SCH_SYMBOL_LIBS, NULL );

    libs = new SYMBOL_LIBS();

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

    m_prj->SetElem( PROJECT::ELEM_SCH_SYMBOL_LIBS, libs );

    // Update the schematic symbol library links since the library list has changed.
    SCH_SCREENS schematic( m_schematic->Root() );
    schematic.UpdateSymbolLinks();
    return true;
}


void LEGACY_RESCUER::AddSymbol( LIB_SYMBOL* aNewSymbol )
{
    wxCHECK_RET( aNewSymbol, "Invalid LIB_SYMBOL pointer." );

    aNewSymbol->SetLib( m_rescue_lib.get() );
    m_rescue_lib->AddSymbol( aNewSymbol );
}


SYMBOL_LIB_TABLE_RESCUER::SYMBOL_LIB_TABLE_RESCUER( PROJECT& aProject, SCHEMATIC* aSchematic,
                                                    SCH_SHEET_PATH* aCurrentSheet,
                                                    EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType ) :
    RESCUER( aProject, aSchematic, aCurrentSheet, aGalBackEndType )
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
    wxFileName fn = GetRescueLibraryFileName( m_schematic );

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

    // This can only happen if the symbol library table file was corrupted on write.
    if( !m_prj->SchSymbolLibTable() )
        return false;

    // Update the schematic symbol library links since the library list has changed.
    SCH_SCREENS schematic( m_schematic->Root() );
    schematic.UpdateSymbolLinks();
    return true;
}


void SYMBOL_LIB_TABLE_RESCUER::AddSymbol( LIB_SYMBOL* aNewSymbol )
{
    wxCHECK_RET( aNewSymbol, "Invalid LIB_SYMBOL pointer." );

    wxFileName fn = GetRescueLibraryFileName( m_schematic );

    try
    {
        if( !m_prj->SchSymbolLibTable()->HasLibrary( fn.GetName() ) )
            m_pi->SaveSymbol( fn.GetFullPath(), new LIB_SYMBOL( *aNewSymbol ), m_properties.get() );
        else
            m_prj->SchSymbolLibTable()->SaveSymbol( fn.GetName(), new LIB_SYMBOL( *aNewSymbol ) );
    }
    catch( ... /* IO_ERROR */ )
    {
    }
}
