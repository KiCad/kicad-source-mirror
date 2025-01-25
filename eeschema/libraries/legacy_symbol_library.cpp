/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2022 CERN
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

#include <algorithm>
#include <kiface_base.h>
#include <eda_base_frame.h>
#include <string_utils.h>
#include <macros.h>
#include <richio.h>
#include <wildcards_and_files_ext.h>
#include <project/project_file.h>
#include <project_rescue.h>
#include <project_sch.h>
#include <widgets/app_progress_dialog.h>

#include <libraries/legacy_symbol_library.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>

#include <wx/log.h>
#include <wx/progdlg.h>
#include <wx/tokenzr.h>
#include <sim/sim_model.h>

LEGACY_SYMBOL_LIB::LEGACY_SYMBOL_LIB( SCH_LIB_TYPE aType, const wxString& aFileName,
                        SCH_IO_MGR::SCH_FILE_T aPluginType ) :
    m_pluginType( aPluginType )
{
    type = aType;
    isModified = false;
    timeStamp = 0;
    timeStamp = wxDateTime::Now();
    versionMajor = 0;       // Will be updated after reading the lib file
    versionMinor = 0;       // Will be updated after reading the lib file

    fileName = aFileName;

    if( !fileName.IsOk() )
        fileName = "unnamed.lib";

    m_plugin.reset( SCH_IO_MGR::FindPlugin( m_pluginType ) );
    m_properties = std::make_unique<std::map<std::string, UTF8>>();
    m_mod_hash = 0;
}


LEGACY_SYMBOL_LIB::~LEGACY_SYMBOL_LIB()
{
}


void LEGACY_SYMBOL_LIB::Save( bool aSaveDocFile )
{
    wxCHECK_RET( m_plugin != nullptr,
                 wxString::Format( wxT( "no plugin defined for library `%s`." ),
                                   fileName.GetFullPath() ) );

    std::map<std::string, UTF8> props;

    if( !aSaveDocFile )
        props[ SCH_IO_KICAD_LEGACY::PropNoDocFile ] = "";

    m_plugin->SaveLibrary( fileName.GetFullPath(), &props );
    isModified = false;
}


void LEGACY_SYMBOL_LIB::Create( const wxString& aFileName )
{
    wxString tmpFileName = fileName.GetFullPath();

    if( !aFileName.IsEmpty() )
        tmpFileName = aFileName;

    m_plugin->CreateLibrary( tmpFileName, m_properties.get() );
}


void LEGACY_SYMBOL_LIB::SetPluginType( SCH_IO_MGR::SCH_FILE_T aPluginType )
{
    if( m_pluginType != aPluginType )
    {
        m_pluginType = aPluginType;
        m_plugin.reset( SCH_IO_MGR::FindPlugin( m_pluginType ) );
    }
}


bool LEGACY_SYMBOL_LIB::IsCache() const
{
    return m_properties->contains( SCH_IO_KICAD_LEGACY::PropNoDocFile );
}


void LEGACY_SYMBOL_LIB::SetCache()
{
    (*m_properties)[ SCH_IO_KICAD_LEGACY::PropNoDocFile ] = "";
}


bool LEGACY_SYMBOL_LIB::IsBuffering() const
{
    return m_properties->contains( SCH_IO_KICAD_LEGACY::PropBuffering );
}


void LEGACY_SYMBOL_LIB::EnableBuffering( bool aEnable )
{
    if( aEnable )
        (*m_properties)[ SCH_IO_KICAD_LEGACY::PropBuffering ] = "";
    else
        m_properties->erase( SCH_IO_KICAD_LEGACY::PropBuffering );
}


void LEGACY_SYMBOL_LIB::GetSymbolNames( wxArrayString& aNames ) const
{
    m_plugin->EnumerateSymbolLib( aNames, fileName.GetFullPath(), m_properties.get() );

    aNames.Sort();
}


void LEGACY_SYMBOL_LIB::GetSymbols( std::vector<LIB_SYMBOL*>& aSymbols ) const
{
    m_plugin->EnumerateSymbolLib( aSymbols, fileName.GetFullPath(), m_properties.get() );

    std::sort( aSymbols.begin(), aSymbols.end(),
            [](LIB_SYMBOL *lhs, LIB_SYMBOL *rhs) -> bool
            {
                return lhs->GetName() < rhs->GetName();
            } );
}


LIB_SYMBOL* LEGACY_SYMBOL_LIB::FindSymbol( const wxString& aName ) const
{
    LIB_SYMBOL* symbol = m_plugin->LoadSymbol( fileName.GetFullPath(), aName, m_properties.get() );

    if( symbol )
    {
        // Set the library to this even though technically the legacy cache plugin owns the
        // symbols.  This allows the symbol library table conversion tool to determine the
        // correct library where the symbol was found.
        if( !symbol->GetLib() )
            symbol->SetLib( const_cast<LEGACY_SYMBOL_LIB*>( this ) );

        SIM_MODEL::MigrateSimModel<LIB_SYMBOL>( *symbol, nullptr );
    }

    return symbol;
}


LIB_SYMBOL* LEGACY_SYMBOL_LIB::FindSymbol( const LIB_ID& aLibId ) const
{
    return FindSymbol( aLibId.Format().wx_str() );
}


void LEGACY_SYMBOL_LIB::AddSymbol( LIB_SYMBOL* aSymbol )
{
    // add a clone, not the caller's copy, the plugin take ownership of the new symbol.
    m_plugin->SaveSymbol( fileName.GetFullPath(),
                          new LIB_SYMBOL( *aSymbol->SharedPtr().get(), this ),
                          m_properties.get() );

    // If we are not buffering, the library file is updated immediately when the plugin
    // SaveSymbol() function is called.
    if( IsBuffering() )
        isModified = true;

    ++m_mod_hash;
}


LIB_SYMBOL* LEGACY_SYMBOL_LIB::RemoveSymbol( LIB_SYMBOL* aEntry )
{
    wxCHECK_MSG( aEntry != nullptr, nullptr, "NULL pointer cannot be removed from library." );

    m_plugin->DeleteSymbol( fileName.GetFullPath(), aEntry->GetName(), m_properties.get() );

    // If we are not buffering, the library file is updated immediately when the plugin
    // SaveSymbol() function is called.
    if( IsBuffering() )
        isModified = true;

    ++m_mod_hash;
    return nullptr;
}


LIB_SYMBOL* LEGACY_SYMBOL_LIB::ReplaceSymbol( LIB_SYMBOL* aOldSymbol, LIB_SYMBOL* aNewSymbol )
{
    wxASSERT( aOldSymbol != nullptr );
    wxASSERT( aNewSymbol != nullptr );

    m_plugin->DeleteSymbol( fileName.GetFullPath(), aOldSymbol->GetName(), m_properties.get() );

    LIB_SYMBOL* my_part = new LIB_SYMBOL( *aNewSymbol, this );

    m_plugin->SaveSymbol( fileName.GetFullPath(), my_part, m_properties.get() );

    // If we are not buffering, the library file is updated immediately when the plugin
    // SaveSymbol() function is called.
    if( IsBuffering() )
        isModified = true;

    ++m_mod_hash;
    return my_part;
}


LEGACY_SYMBOL_LIB* LEGACY_SYMBOL_LIB::LoadSymbolLibrary( const wxString& aFileName )
{
    std::unique_ptr<LEGACY_SYMBOL_LIB> lib = std::make_unique<LEGACY_SYMBOL_LIB>( SCH_LIB_TYPE::LT_EESCHEMA,
                                                                    aFileName );

    std::vector<LIB_SYMBOL*> parts;
    // This loads the library.
    lib->GetSymbols( parts );

    // Now, set the LIB_SYMBOL m_library member but it will only be used
    // when loading legacy libraries in the future. Once the symbols in the
    // schematic have a full #LIB_ID, this will not get called.
    for( size_t ii = 0; ii < parts.size(); ii++ )
    {
        LIB_SYMBOL* part = parts[ii];

        part->SetLib( lib.get() );
    }

    LEGACY_SYMBOL_LIB* ret = lib.release();
    return ret;
}


LEGACY_SYMBOL_LIB* LEGACY_SYMBOL_LIBS::AddLibrary( const wxString& aFileName )
{
    LEGACY_SYMBOL_LIB* lib;

    wxFileName fn = aFileName;
    // Don't reload the library if it is already loaded.
    lib = FindLibrary( fn.GetName() );

    if( lib )
        return lib;

    try
    {
        lib = LEGACY_SYMBOL_LIB::LoadSymbolLibrary( aFileName );
        push_back( lib );

        return lib;
    }
    catch( ... )
    {
        return nullptr;
    }
}


LEGACY_SYMBOL_LIB* LEGACY_SYMBOL_LIBS::AddLibrary( const wxString& aFileName, LEGACY_SYMBOL_LIBS::iterator& aIterator )
{
    // Don't reload the library if it is already loaded.
    wxFileName fn( aFileName );
    LEGACY_SYMBOL_LIB* lib = FindLibrary( fn.GetName() );

    if( lib )
        return lib;

    try
    {
        lib = LEGACY_SYMBOL_LIB::LoadSymbolLibrary( aFileName );

        if( aIterator >= begin() && aIterator < end() )
            insert( aIterator, lib );
        else
            push_back( lib );

        return lib;
    }
    catch( ... )
    {
        return nullptr;
    }
}


bool LEGACY_SYMBOL_LIBS::ReloadLibrary( const wxString &aFileName )
{
    wxFileName  fn = aFileName;
    LEGACY_SYMBOL_LIB* lib = FindLibrary( fn.GetName() );

    // Check if the library already exists.
    if( !lib )
        return false;

    // Create a clone of the library pointer in case we need to re-add it
    LEGACY_SYMBOL_LIB *cloneLib = lib;

    // Try to find the iterator of the library
    for( auto it = begin(); it != end(); ++it )
    {
        if( it->GetName() == fn.GetName() )
        {
            // Remove the old library and keep the pointer
            lib = &*it;
            release( it );
            break;
        }
    }

    // Try to reload the library
    try
    {
        lib = LEGACY_SYMBOL_LIB::LoadSymbolLibrary( aFileName );

        // If the library is successfully reloaded, add it back to the set.
        push_back( lib );
        return true;
    }
    catch( ... )
    {
        // If an exception occurs, ensure that the SYMBOL_LIBS remains unchanged
        // by re-adding the old library back to the set.
        push_back( cloneLib );
        return false;
    }
}


LEGACY_SYMBOL_LIB* LEGACY_SYMBOL_LIBS::FindLibrary( const wxString& aName )
{
    for( LEGACY_SYMBOL_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->GetName() == aName )
            return &*it;
    }

    return nullptr;
}


LEGACY_SYMBOL_LIB* LEGACY_SYMBOL_LIBS::GetCacheLibrary()
{
    for( LEGACY_SYMBOL_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->IsCache() )
            return &*it;
    }

    return nullptr;
}


LEGACY_SYMBOL_LIB* LEGACY_SYMBOL_LIBS::FindLibraryByFullFileName( const wxString& aFullFileName )
{
    for( LEGACY_SYMBOL_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->GetFullFileName() == aFullFileName )
            return &*it;
    }

    return nullptr;
}


wxArrayString LEGACY_SYMBOL_LIBS::GetLibraryNames( bool aSorted )
{
    wxArrayString cacheNames;
    wxArrayString names;

    for( LEGACY_SYMBOL_LIB& lib : *this )
    {
        if( lib.IsCache() && aSorted )
            cacheNames.Add( lib.GetName() );
        else
            names.Add( lib.GetName() );
    }

    // Even sorted, the cache library is always at the end of the list.
    if( aSorted )
        names.Sort();

    for( unsigned int i = 0; i<cacheNames.Count(); i++ )
        names.Add( cacheNames.Item( i ) );

    return names;
}


LIB_SYMBOL* LEGACY_SYMBOL_LIBS::FindLibSymbol( const LIB_ID& aLibId, const wxString& aLibraryName )
{
    LIB_SYMBOL* part = nullptr;

    for( LEGACY_SYMBOL_LIB& lib : *this )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        part = lib.FindSymbol( aLibId.GetLibItemName().wx_str() );

        if( part )
            break;
    }

    return part;
}


void LEGACY_SYMBOL_LIBS::FindLibraryNearEntries( std::vector<LIB_SYMBOL*>& aCandidates,
                                          const wxString& aEntryName,
                                          const wxString& aLibraryName )
{
    for( LEGACY_SYMBOL_LIB& lib : *this )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        wxArrayString partNames;

        lib.GetSymbolNames( partNames );

        if( partNames.IsEmpty() )
            continue;

        for( size_t i = 0;  i < partNames.size();  i++ )
        {
            if( partNames[i].CmpNoCase( aEntryName ) == 0 )
                aCandidates.push_back( lib.FindSymbol( partNames[i] ) );
        }
    }
}


void LEGACY_SYMBOL_LIBS::GetLibNamesAndPaths( PROJECT* aProject, wxString* aPaths, wxArrayString* aNames )
{
    wxCHECK_RET( aProject, "Null PROJECT in GetLibNamesAndPaths" );

    PROJECT_FILE& project = aProject->GetProjectFile();

    if( aPaths )
        *aPaths = project.m_LegacyLibDir;

    if( aNames )
        *aNames = project.m_LegacyLibNames;
}


void LEGACY_SYMBOL_LIBS::SetLibNamesAndPaths( PROJECT* aProject, const wxString& aPaths,
                                       const wxArrayString& aNames )
{
    wxCHECK_RET( aProject, "Null PROJECT in SetLibNamesAndPaths" );

    PROJECT_FILE& project = aProject->GetProjectFile();

    project.m_LegacyLibDir = aPaths;
    project.m_LegacyLibNames = aNames;
}


const wxString LEGACY_SYMBOL_LIBS::CacheName( const wxString& aFullProjectFilename )
{
    wxFileName filename( aFullProjectFilename );
    wxString   name = filename.GetName();

    filename.SetName( name + "-cache" );
    filename.SetExt( FILEEXT::LegacySymbolLibFileExtension );

    if( filename.FileExists() )
        return filename.GetFullPath();

    // Try the old (2007) cache name
    filename.SetName( name + ".cache" );

    if( filename.FileExists() )
        return filename.GetFullPath();

    return wxEmptyString;
}


void LEGACY_SYMBOL_LIBS::LoadAllLibraries( PROJECT* aProject, bool aShowProgress )
{
    wxString        filename;
    wxString        libs_not_found;
    SEARCH_STACK*   lib_search = PROJECT_SCH::SchSearchS( aProject );

#if defined(DEBUG) && 0
    lib_search->Show( __func__ );
#endif

    wxArrayString   lib_names;

    GetLibNamesAndPaths( aProject, nullptr, &lib_names );

    // Post symbol library table, this should be empty.  Only the cache library should get loaded.
    if( !lib_names.empty() )
    {
        APP_PROGRESS_DIALOG lib_dialog( _( "Loading Symbol Libraries" ),
                                        wxEmptyString,
                                        lib_names.GetCount(),
                                        nullptr,
                                        false,
                                        wxPD_APP_MODAL );

        if( aShowProgress )
        {
            lib_dialog.Show();
        }

        for( unsigned i = 0; i < lib_names.GetCount();  ++i )
        {
            if( aShowProgress )
            {
                lib_dialog.Update( i, wxString::Format( _( "Loading %s..." ), lib_names[i] ) );
            }

            // lib_names[] does not store the file extension. Set it.
            // Remember lib_names[i] can contain a '.' in name, so using a wxFileName
            // before adding the extension can create incorrect full filename
            wxString fullname = lib_names[i] + "." + FILEEXT::LegacySymbolLibFileExtension;

            // Now the full name is set, we can use a wxFileName.
            wxFileName fn( fullname );

            // Skip if the file name is not valid..
            if( !fn.IsOk() )
                continue;

            if( !fn.FileExists() )
            {
                filename = lib_search->FindValidPath( fn.GetFullPath() );

                if( !filename )
                {
                    libs_not_found += fn.GetFullPath();
                    libs_not_found += '\n';
                    continue;
                }
            }
            else
            {   // ensure the lib filename has a absolute path.
                // If the lib has no absolute path, and is found in the cwd by fn.FileExists(),
                // make a full absolute path, to avoid issues with load library functions which
                // expects an absolute path.
                if( !fn.IsAbsolute() )
                    fn.MakeAbsolute();

                filename = fn.GetFullPath();
            }

            try
            {
                AddLibrary( filename );
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg;
                msg.Printf( _( "Symbol library '%s' failed to load." ), filename );

                wxLogError( msg + wxS( "\n" ) + ioe.What() );
            }
        }
    }

    // add the special cache library.
    wxString cache_name = CacheName( aProject->GetProjectFullName() );
    LEGACY_SYMBOL_LIB* cache_lib;

    if( !aProject->IsNullProject() && !cache_name.IsEmpty() )
    {
        try
        {
            cache_lib = AddLibrary( cache_name );

            if( cache_lib )
                cache_lib->SetCache();
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format( _( "Error loading symbol library '%s'." )
                                             + wxS( "\n%s" ),
                                             cache_name,
                                             ioe.What() );

            THROW_IO_ERROR( msg );
        }
    }

    // Print the libraries not found
    if( !libs_not_found.IsEmpty() )
    {
        // Use a different exception type so catch()er can route to proper use
        // of the HTML_MESSAGE_BOX.
        THROW_PARSE_ERROR( wxEmptyString, __func__, TO_UTF8( libs_not_found ), 0, 0 );
    }
}
