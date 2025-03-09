/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
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

#include <sch_draw_panel.h>
#include <libraries/legacy_symbol_library.h>
#include <confirm.h>
#include <connection_graph.h>
#include <invoke_sch_dialog.h>
#include <kiway.h>
#include <symbol_viewer_frame.h>
#include <project_rescue.h>
#include <project_sch.h>
#include <sch_edit_frame.h>
#include <string_utils.h>
#include <symbol_lib_table.h>
#include <wildcards_and_files_ext.h>
#include <wx/msgdlg.h>

#include <cctype>
#include <map>
#include <pgm_base.h>
#include <libraries/symbol_library_manager_adapter.h>


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
        for( EDA_ITEM* aItem : screen->Items().OfType( SCH_SYMBOL_T ) )
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
static LIB_SYMBOL* findSymbol( const wxString& aName, LEGACY_SYMBOL_LIBS* aLibs, bool aCached )
{
    LIB_SYMBOL *symbol = nullptr;

    for( LEGACY_SYMBOL_LIB& each_lib : *aLibs )
    {
        if( aCached && !each_lib.IsCache() )
            continue;

        if( !aCached && each_lib.IsCache() )
            continue;

        symbol = each_lib.FindSymbol( aName );

        if( symbol )
            break;
    }

    return symbol;
}


static wxFileName GetRescueLibraryFileName( SCHEMATIC* aSchematic )
{
    wxFileName fn = aSchematic->GetFileName();
    fn.SetName( fn.GetName() + wxT( "-rescue" ) );
    fn.SetExt( FILEEXT::LegacySymbolLibFileExtension );
    return fn;
}


RESCUE_CASE_CANDIDATE::RESCUE_CASE_CANDIDATE( const wxString& aRequestedName, const wxString& aNewName,
                                              LIB_SYMBOL* aLibCandidate, int aUnit, int aBodyStyle ) :
        RESCUE_CANDIDATE( aRequestedName, aNewName, aLibCandidate, aUnit, aBodyStyle )
{}


void RESCUE_CASE_CANDIDATE::FindRescues( RESCUER& aRescuer, boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
{
    std::map<wxString, RESCUE_CASE_CANDIDATE> candidate_map;

    // Remember the list of symbols is sorted by symbol name.
    // So a search in libraries is made only once by group
    LIB_SYMBOL* case_sensitive_match = nullptr;
    std::vector<LIB_SYMBOL*> case_insensitive_matches;

    wxString symbol_name;
    wxString last_symbol_name;

    for( SCH_SYMBOL* eachSymbol : *( aRescuer.GetSymbols() ) )
    {
        symbol_name = eachSymbol->GetLibId().GetUniStringLibItemName();

        if( last_symbol_name != symbol_name )
        {
            // A new symbol name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            last_symbol_name = symbol_name;
            case_insensitive_matches.clear();

            LIB_ID id( wxEmptyString, symbol_name );

            case_sensitive_match = PROJECT_SCH::LegacySchLibs( aRescuer.GetPrj() )->FindLibSymbol( id );

            if( case_sensitive_match )
                continue;

            // If the case sensitive match failed, try a case insensitive match.
            PROJECT_SCH::LegacySchLibs( aRescuer.GetPrj() )
                            ->FindLibraryNearEntries( case_insensitive_matches, symbol_name );

            // If there are not case insensitive matches either, the symbol cannot be rescued.
            if( !case_insensitive_matches.size() )
                continue;

            RESCUE_CASE_CANDIDATE candidate( symbol_name, case_insensitive_matches[0]->GetName(),
                                             case_insensitive_matches[0], eachSymbol->GetUnit(),
                                             eachSymbol->GetBodyStyle() );

            candidate_map[symbol_name] = candidate;
        }
    }

    // Now, dump the map into aCandidates
    for( const auto& [ name, candidate ] : candidate_map )
        aCandidates.push_back( new RESCUE_CASE_CANDIDATE( candidate ) );
}


wxString RESCUE_CASE_CANDIDATE::GetActionDescription() const
{
    wxString action;
    action.Printf( _( "Rename %s to %s" ), m_requested_name, m_new_name );
    return action;
}


bool RESCUE_CASE_CANDIDATE::PerformAction( RESCUER* aRescuer )
{
    wxCHECK( m_lib_candidate, true );

    std::unique_ptr<LIB_SYMBOL> new_symbol = m_lib_candidate->Flatten();
    new_symbol->SetName( m_new_name );
    aRescuer->AddSymbol( new_symbol.get() );

    for( SCH_SYMBOL* eachSymbol : *aRescuer->GetSymbols() )
    {
        if( eachSymbol->GetLibId().GetLibItemName() != UTF8( m_requested_name ) )
            continue;

        LIB_ID libId;

        libId.SetLibItemName( m_new_name );
        eachSymbol->SetLibId( libId );
        eachSymbol->ClearFlags();
        aRescuer->LogRescue( eachSymbol, m_requested_name, m_new_name );
    }

    return true;
}


void RESCUE_CACHE_CANDIDATE::FindRescues( RESCUER& aRescuer, boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
{
    std::map<wxString, RESCUE_CACHE_CANDIDATE> candidate_map;

    // Remember the list of symbols is sorted by symbol name.
    // So a search in libraries is made only once by group
    LIB_SYMBOL* cache_match = nullptr;
    LIB_SYMBOL* lib_match = nullptr;
    wxString symbol_name;
    wxString old_symbol_name;

    for( SCH_SYMBOL* eachSymbol : *( aRescuer.GetSymbols() ) )
    {
        symbol_name = eachSymbol->GetLibId().GetUniStringLibItemName();

        if( old_symbol_name != symbol_name )
        {
            // A new symbol name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            old_symbol_name = symbol_name;
            cache_match = findSymbol( symbol_name, PROJECT_SCH::LegacySchLibs( aRescuer.GetPrj() ), true );
            lib_match = findSymbol( symbol_name, PROJECT_SCH::LegacySchLibs( aRescuer.GetPrj() ), false );

            // At some point during V5 development, the LIB_ID delimiter character ':' was
            // replaced by '_' when writing the symbol cache library so we have to test for
            // the LIB_NICKNAME_LIB_SYMBOL_NAME case.
            if( !cache_match && eachSymbol->GetLibId().IsValid() )
            {
                wxString tmp = wxString::Format( wxT( "%s-%s" ),
                                                 eachSymbol->GetLibId().GetLibNickname().wx_str(),
                                                 eachSymbol->GetLibId().GetLibItemName().wx_str() );
                cache_match = findSymbol( tmp, PROJECT_SCH::LegacySchLibs( aRescuer.GetPrj() ),
                                          true );
            }

            // Test whether there is a conflict or if the symbol can only be found in the cache
            // and the symbol name does not have any illegal characters.
            if( cache_match && lib_match
                    && !cache_match->PinsConflictWith( *lib_match, true, true, true, true, false ) )
            {
                continue;
            }

            if( !cache_match && lib_match )
                continue;

            // Check if the symbol has already been rescued.
            RESCUE_CACHE_CANDIDATE candidate( symbol_name, symbol_name, cache_match, lib_match,
                                              eachSymbol->GetUnit(), eachSymbol->GetBodyStyle() );

            candidate_map[symbol_name] = candidate;
        }
    }

    // Now, dump the map into aCandidates
    for( const auto& [name, candidate] : candidate_map )
        aCandidates.push_back( new RESCUE_CACHE_CANDIDATE( candidate ) );
}


wxString RESCUE_CACHE_CANDIDATE::GetActionDescription() const
{
    wxString action;

    if( !m_cache_candidate && !m_lib_candidate )
    {
        action.Printf( _( "Cannot rescue symbol %s which is not available in any library or the cache." ),
                       m_requested_name );
    }
    else if( m_cache_candidate && !m_lib_candidate )
    {
        action.Printf( _( "Rescue symbol %s found only in cache library to %s." ),
                       m_requested_name,
                       m_new_name );
    }
    else
    {
        action.Printf( _( "Rescue modified symbol %s to %s" ),
                       m_requested_name,
                       m_new_name );
    }

    return action;
}


bool RESCUE_CACHE_CANDIDATE::PerformAction( RESCUER* aRescuer )
{
    LIB_SYMBOL* tmp = ( m_cache_candidate ) ? m_cache_candidate : m_lib_candidate;

    // A symbol that cannot be rescued is a valid condition so just bail out here.
    if( !tmp )
        return true;

    std::unique_ptr<LIB_SYMBOL> new_symbol = tmp->Flatten();
    new_symbol->SetName( m_new_name );
    aRescuer->AddSymbol( new_symbol.get() );

    for( SCH_SYMBOL* eachSymbol : *aRescuer->GetSymbols() )
    {
        if( eachSymbol->GetLibId().GetLibItemName() != UTF8( m_requested_name ) )
            continue;

        LIB_ID libId;

        libId.SetLibItemName( m_new_name );
        eachSymbol->SetLibId( libId );
        eachSymbol->ClearFlags();
        aRescuer->LogRescue( eachSymbol, m_requested_name, m_new_name );
    }

    return true;
}


RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::RESCUE_SYMBOL_LIB_TABLE_CANDIDATE( const LIB_ID& aRequestedId,
                                                                      const LIB_ID& aNewId,
                                                                      LIB_SYMBOL* aCacheCandidate,
                                                                      LIB_SYMBOL* aLibCandidate,
                                                                      int aUnit, int aBodyStyle ) :
        RESCUE_CANDIDATE( aRequestedId.Format().wx_str(), wxEmptyString, aLibCandidate, aUnit, aBodyStyle ),
        m_requested_id( aRequestedId ),
        m_new_id( aNewId ),
        m_cache_candidate( aCacheCandidate )
{}


RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::RESCUE_SYMBOL_LIB_TABLE_CANDIDATE() :
        RESCUE_CANDIDATE( wxEmptyString, wxEmptyString, nullptr, 0, 0 ),
        m_requested_id(),
        m_new_id(),
        m_cache_candidate( nullptr )
{}


void RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::FindRescues( RESCUER& aRescuer,
                                                     boost::ptr_vector<RESCUE_CANDIDATE>& aCandidates )
{
    std::map<LIB_ID, RESCUE_SYMBOL_LIB_TABLE_CANDIDATE> candidate_map;

    // Remember the list of symbols is sorted by LIB_ID.
    // So a search in libraries is made only once by group
    LIB_SYMBOL* cache_match = nullptr;
    LIB_SYMBOL* lib_match = nullptr;
    LIB_ID old_symbol_id;

    wxString symbolName;

    for( SCH_SYMBOL* eachSymbol : *( aRescuer.GetSymbols() ) )
    {
        const LIB_ID& symbol_id = eachSymbol->GetLibId();

        if( old_symbol_id != symbol_id )
        {
            // A new symbol name is found (a new group starts here).
            // Search the symbol names candidates only once for this group:
            old_symbol_id = symbol_id;

            symbolName = symbol_id.Format().wx_str();

            // Get the library symbol from the cache library.  It will be a flattened
            // symbol by default (no inheritance).
            cache_match = findSymbol( symbolName, PROJECT_SCH::LegacySchLibs( aRescuer.GetPrj() ), true );

            // At some point during V5 development, the LIB_ID delimiter character ':' was
            // replaced by '_' when writing the symbol cache library so we have to test for
            // the LIB_NICKNAME_LIB_SYMBOL_NAME case.
            if( !cache_match )
            {
                symbolName.Printf( wxT( "%s-%s" ),
                                   symbol_id.GetLibNickname().wx_str(),
                                   symbol_id.GetLibItemName().wx_str() );
                cache_match = findSymbol( symbolName, PROJECT_SCH::LegacySchLibs( aRescuer.GetPrj() ), true );
            }

            // Get the library symbol from the symbol library table.
            lib_match = SchGetLibSymbol( symbol_id, PROJECT_SCH::SymbolLibManager( aRescuer.GetPrj() ) );

            if( !cache_match && !lib_match )
                continue;

            LIB_SYMBOL_SPTR lib_match_parent;

            // If it's a derived symbol, use the parent symbol to perform the pin test.
            if( lib_match && lib_match->IsDerived() )
            {
                lib_match_parent = lib_match->GetRootSymbol();

                if( !lib_match_parent )
                    lib_match = nullptr;
                else
                    lib_match = lib_match_parent.get();
            }

            // Test whether there is a conflict or if the symbol can only be found in the cache.
            if( LIB_ID::HasIllegalChars( symbol_id.GetLibItemName() ) == -1 )
            {
                if( cache_match && lib_match
                    && !cache_match->PinsConflictWith( *lib_match, true, true, true, true, false ) )
                {
                    continue;
                }

                if( !cache_match && lib_match )
                    continue;
            }

            // Fix illegal LIB_ID name characters.
            wxString new_name = EscapeString( symbol_id.GetLibItemName(), CTX_LIBID );

            // Differentiate symbol name in the rescue library by appending the original symbol
            // library table nickname to the symbol name to prevent name clashes in the rescue
            // library.
            wxString libNickname = GetRescueLibraryFileName( aRescuer.Schematic() ).GetName();

            LIB_ID new_id( libNickname, wxString::Format( wxT( "%s-%s" ),
                                                          new_name,
                                                          symbol_id.GetLibNickname().wx_str() ) );

            RESCUE_SYMBOL_LIB_TABLE_CANDIDATE candidate( symbol_id, new_id, cache_match, lib_match,
                                                         eachSymbol->GetUnit(),
                                                         eachSymbol->GetBodyStyle() );

            candidate_map[symbol_id] = candidate;
        }
    }

    // Now, dump the map into aCandidates
    for( const auto& [name, candidate] : candidate_map )
        aCandidates.push_back( new RESCUE_SYMBOL_LIB_TABLE_CANDIDATE( candidate ) );
}


wxString RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::GetActionDescription() const
{
    wxString action;

    if( !m_cache_candidate && !m_lib_candidate )
    {
        action.Printf( _( "Cannot rescue symbol %s which is not available in any library or the cache." ),
                       UnescapeString( m_requested_id.GetLibItemName().wx_str() ) );
    }
    else if( m_cache_candidate && !m_lib_candidate )
    {
        action.Printf( _( "Rescue symbol %s found only in cache library to %s." ),
                       UnescapeString( m_requested_id.Format().wx_str() ),
                       UnescapeString( m_new_id.Format().wx_str() ) );
    }
    else
    {
        action.Printf( _( "Rescue modified symbol %s to %s" ),
                       UnescapeString( m_requested_id.Format().wx_str() ),
                       UnescapeString( m_new_id.Format().wx_str() ) );
    }

    return action;
}


bool RESCUE_SYMBOL_LIB_TABLE_CANDIDATE::PerformAction( RESCUER* aRescuer )
{
    LIB_SYMBOL* tmp = ( m_cache_candidate ) ? m_cache_candidate : m_lib_candidate;

    wxCHECK_MSG( tmp, false, wxS( "Both cache and library symbols undefined." ) );

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


void RESCUER::LogRescue( SCH_SYMBOL* aSymbol, const wxString &aOldName, const wxString &aNewName )
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

        libId.SetLibItemName( each_logitem.old_name );
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

    std::unique_ptr<LEGACY_SYMBOL_LIB> rescue_lib =
            std::make_unique<LEGACY_SYMBOL_LIB>( SCH_LIB_TYPE::LT_EESCHEMA, fn.GetFullPath() );

    m_rescue_lib = std::move( rescue_lib );
    m_rescue_lib->EnableBuffering();

    // If a rescue library already exists copy the contents of that library so we do not
    // lose any previous rescues.
    LEGACY_SYMBOL_LIB* rescueLib = PROJECT_SCH::LegacySchLibs( m_prj )->FindLibrary( fn.GetName() );

    if( rescueLib )
    {
        // For items in the rescue library, aliases are the root symbol.
        std::vector< LIB_SYMBOL* > symbols;

        rescueLib->GetSymbols( symbols );

        for( LIB_SYMBOL* symbol : symbols )
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
        DisplayError( aParent, wxString::Format( _( "Failed to create symbol library file '%s'." ),
                                                 m_rescue_lib->GetFullFileName() ) );
        return false;
    }

    wxArrayString libNames;
    wxString libPaths;

    wxString libName = m_rescue_lib->GetName();
    LEGACY_SYMBOL_LIBS* libs = dynamic_cast<LEGACY_SYMBOL_LIBS*>( m_prj->GetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS ) );

    if( !libs )
    {
        libs = new LEGACY_SYMBOL_LIBS();
        m_prj->SetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS, libs );
    }

    try
    {
        LEGACY_SYMBOL_LIBS::GetLibNamesAndPaths( m_prj, &libPaths, &libNames );

        // Make sure the library is not already in the list
        while( libNames.Index( libName ) != wxNOT_FOUND )
            libNames.Remove( libName );

        // Add the library to the top of the list and save.
        libNames.Insert( libName, 0 );
        LEGACY_SYMBOL_LIBS::SetLibNamesAndPaths( m_prj, libPaths, libNames );
    }
    catch( const IO_ERROR& )
    {
        // Could not get or save the current libraries.
        return false;
    }

    // Save the old libraries in case there is a problem after clear(). We'll
    // put them back in.
    boost::ptr_vector<LEGACY_SYMBOL_LIB> libsSave;
    libsSave.transfer( libsSave.end(), libs->begin(), libs->end(), *libs );

    m_prj->SetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS, nullptr );

    libs = new LEGACY_SYMBOL_LIBS();

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

    m_prj->SetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS, libs );

    // Update the schematic symbol library links since the library list has changed.
    SCH_SCREENS schematic( m_schematic->Root() );
    schematic.UpdateSymbolLinks();
    return true;
}


void LEGACY_RESCUER::AddSymbol( LIB_SYMBOL* aNewSymbol )
{
    wxCHECK_RET( aNewSymbol, wxS( "Invalid LIB_SYMBOL pointer." ) );

    aNewSymbol->SetLib( m_rescue_lib.get() );
    m_rescue_lib->AddSymbol( aNewSymbol );
}


SYMBOL_LIB_TABLE_RESCUER::SYMBOL_LIB_TABLE_RESCUER( PROJECT& aProject, SCHEMATIC* aSchematic,
                                                    SCH_SHEET_PATH* aCurrentSheet,
                                                    EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType ) :
    RESCUER( aProject, aSchematic, aCurrentSheet, aGalBackEndType )
{
    m_properties = std::make_unique<std::map<std::string, UTF8>>();
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
    (*m_properties)[ SCH_IO_KICAD_LEGACY::PropBuffering ] = "";

    wxFileName fn = GetRescueLibraryFileName( m_schematic );

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();
    SYMBOL_LIBRARY_MANAGER_ADAPTER* adapter = PROJECT_SCH::SymbolLibManager( m_prj );

    // If a rescue library already exists copy the contents of that library so we do not
    // lose any previous rescues.
    if( std::optional<const LIBRARY_TABLE_ROW*> optRow =
            manager.GetRow( LIBRARY_TABLE_TYPE::SYMBOL, fn.GetName() ) )
    {
        const LIBRARY_TABLE_ROW* row = *optRow;

        if( SCH_IO_MGR::EnumFromStr( row->Type() ) == SCH_IO_MGR::SCH_KICAD )
            fn.SetExt( FILEEXT::KiCadSymbolLibFileExtension );

        for( LIB_SYMBOL* symbol : adapter->GetSymbols( fn.GetName() ) )
            m_rescueLibSymbols.emplace_back( std::make_unique<LIB_SYMBOL>( *symbol ) );
    }
}


bool SYMBOL_LIB_TABLE_RESCUER::WriteRescueLibrary( wxWindow *aParent )
{
    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    wxFileName fn = GetRescueLibraryFileName( m_schematic );

    std::optional<const LIBRARY_TABLE_ROW*> optRow =
                manager.GetRow( LIBRARY_TABLE_TYPE::SYMBOL, fn.GetName() );

    fn.SetExt( FILEEXT::KiCadSymbolLibFileExtension );

    try
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

        for( const std::unique_ptr<LIB_SYMBOL>& symbol : m_rescueLibSymbols )
            pi->SaveSymbol( fn.GetFullPath(), new LIB_SYMBOL( *symbol.get() ), m_properties.get() );

        pi->SaveLibrary( fn.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg;
        msg.Printf( _( "Failed to save rescue library %s." ), fn.GetFullPath() );
        DisplayErrorMessage( aParent, msg, ioe.What() );
        return false;
    }

    // If the rescue library already exists in the symbol library table no need save it to add
    // it to the table.
    if( !optRow || ( SCH_IO_MGR::EnumFromStr( ( *optRow )->Type() ) == SCH_IO_MGR::SCH_LEGACY ) )
    {
        wxString uri = wxS( "${KIPRJMOD}/" ) + fn.GetFullName();
        wxString libNickname = fn.GetName();

        std::optional<LIBRARY_TABLE*> optTable =
                Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL,
                                                 LIBRARY_TABLE_SCOPE::PROJECT );
        wxCHECK( optTable, false );
        LIBRARY_TABLE* projectTable = *optTable;
        LIBRARY_TABLE_ROW* row = nullptr;

        if( std::optional<LIBRARY_TABLE_ROW*> oldRow = projectTable->Row( libNickname ); oldRow )
            row = *oldRow;
        else
            row = &projectTable->Rows().emplace_back( projectTable->MakeRow() );

        row->SetNickname( libNickname );
        row->SetURI( uri );
        row->SetType( wxT( "KiCad" ) );

        bool success = true;

        manager.Save( projectTable ).map_error(
            [&success]( const LIBRARY_ERROR& aError )
            {
                success = false;
                wxMessageBox( wxString::Format( _( "Error saving project-specific library table:\n\n%s" ),
                                                aError.message ),
                              _( "File Save Error" ), wxOK | wxICON_ERROR );
            } );

        if( !success )
            return false;
    }

    // Update the schematic symbol library links since the library list has changed.
    SCH_SCREENS schematic( m_schematic->Root() );
    schematic.UpdateSymbolLinks();
    return true;
}


void SYMBOL_LIB_TABLE_RESCUER::AddSymbol( LIB_SYMBOL* aNewSymbol )
{
    wxCHECK_RET( aNewSymbol, wxS( "Invalid LIB_SYMBOL pointer." ) );

    m_rescueLibSymbols.emplace_back( std::make_unique<LIB_SYMBOL>( *aNewSymbol ) );
}
