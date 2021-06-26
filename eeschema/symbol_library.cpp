/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_library.cpp
 */

#include <algorithm>
#include <kiface_i.h>
#include <eda_base_frame.h>
#include <kicad_string.h>
#include <macros.h>
#include <richio.h>
#include <config_params.h>
#include <wildcards_and_files_ext.h>
#include <project/project_file.h>
#include <project_rescue.h>
#include <properties.h>
#include <widgets/app_progress_dialog.h>

#include <general.h>
#include <symbol_library.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>

#include <wx/log.h>
#include <wx/progdlg.h>
#include <wx/tokenzr.h>
#include <wx/regex.h>

#define DUPLICATE_NAME_MSG  \
    _(  "Library \"%s\" has duplicate entry name \"%s\".\n" \
        "This may cause some unexpected behavior when loading symbols into a schematic." )


SYMBOL_LIB::SYMBOL_LIB( SCH_LIB_TYPE aType, const wxString& aFileName,
                    SCH_IO_MGR::SCH_FILE_T aPluginType ) :
    // start @ != 0 so each additional library added
    // is immediately detectable, zero would not be.
    m_mod_hash( SYMBOL_LIBS::GetModifyGeneration() ),
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
    m_properties = std::make_unique<PROPERTIES>();
}


SYMBOL_LIB::~SYMBOL_LIB()
{
}


void SYMBOL_LIB::Save( bool aSaveDocFile )
{
    wxCHECK_RET( m_plugin != nullptr, wxString::Format( "no plugin defined for library `%s`.",
                                                        fileName.GetFullPath() ) );

    PROPERTIES props;

    if( !aSaveDocFile )
        props[ SCH_LEGACY_PLUGIN::PropNoDocFile ] = "";

    m_plugin->SaveLibrary( fileName.GetFullPath(), &props );
    isModified = false;
}


void SYMBOL_LIB::Create( const wxString& aFileName )
{
    wxString tmpFileName = fileName.GetFullPath();

    if( !aFileName.IsEmpty() )
        tmpFileName = aFileName;

    m_plugin->CreateSymbolLib( tmpFileName, m_properties.get() );
}


void SYMBOL_LIB::SetPluginType( SCH_IO_MGR::SCH_FILE_T aPluginType )
{
    if( m_pluginType != aPluginType )
    {
        m_pluginType = aPluginType;
        m_plugin.reset( SCH_IO_MGR::FindPlugin( m_pluginType ) );
    }
}


bool SYMBOL_LIB::IsCache() const
{
    return m_properties->Exists( SCH_LEGACY_PLUGIN::PropNoDocFile );
}


void SYMBOL_LIB::SetCache()
{
    (*m_properties)[ SCH_LEGACY_PLUGIN::PropNoDocFile ] = "";
}


bool SYMBOL_LIB::IsBuffering() const
{
    return m_properties->Exists( SCH_LEGACY_PLUGIN::PropBuffering );
}


void SYMBOL_LIB::EnableBuffering( bool aEnable )
{
    if( aEnable )
        (*m_properties)[ SCH_LEGACY_PLUGIN::PropBuffering ] = "";
    else
        m_properties->Clear( SCH_LEGACY_PLUGIN::PropBuffering );
}


void SYMBOL_LIB::GetSymbolNames( wxArrayString& aNames ) const
{
    m_plugin->EnumerateSymbolLib( aNames, fileName.GetFullPath(), m_properties.get() );

    aNames.Sort();
}


void SYMBOL_LIB::GetSymbols( std::vector<LIB_SYMBOL*>& aSymbols ) const
{
    m_plugin->EnumerateSymbolLib( aSymbols, fileName.GetFullPath(), m_properties.get() );

    std::sort( aSymbols.begin(), aSymbols.end(),
            [](LIB_SYMBOL *lhs, LIB_SYMBOL *rhs) -> bool
                { return lhs->GetName() < rhs->GetName(); });
}


LIB_SYMBOL* SYMBOL_LIB::FindSymbol( const wxString& aName ) const
{
    LIB_SYMBOL* symbol = m_plugin->LoadSymbol( fileName.GetFullPath(), aName, m_properties.get() );

    // Set the library to this even though technically the legacy cache plugin owns the
    // symbols.  This allows the symbol library table conversion tool to determine the
    // correct library where the symbol was found.
    if( symbol && !symbol->GetLib() )
        symbol->SetLib( const_cast<SYMBOL_LIB*>( this ) );

    return symbol;
}


LIB_SYMBOL* SYMBOL_LIB::FindSymbol( const LIB_ID& aLibId ) const
{
    return FindSymbol( aLibId.Format().wx_str() );
}


void SYMBOL_LIB::AddSymbol( LIB_SYMBOL* aSymbol )
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


LIB_SYMBOL* SYMBOL_LIB::RemoveSymbol( LIB_SYMBOL* aEntry )
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


LIB_SYMBOL* SYMBOL_LIB::ReplaceSymbol( LIB_SYMBOL* aOldSymbol, LIB_SYMBOL* aNewSymbol )
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


SYMBOL_LIB* SYMBOL_LIB::LoadLibrary( const wxString& aFileName )
{
    std::unique_ptr<SYMBOL_LIB> lib = std::make_unique<SYMBOL_LIB>( SCH_LIB_TYPE::LT_EESCHEMA,
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

    SYMBOL_LIB* ret = lib.release();
    return ret;
}


SYMBOL_LIB* SYMBOL_LIBS::AddLibrary( const wxString& aFileName )
{
    SYMBOL_LIB* lib;

    wxFileName fn = aFileName;
    // Don't reload the library if it is already loaded.
    lib = FindLibrary( fn.GetName() );

    if( lib )
        return lib;

    try
    {
        lib = SYMBOL_LIB::LoadLibrary( aFileName );
        push_back( lib );

        return lib;
    }
    catch( ... )
    {
        return nullptr;
    }
}


SYMBOL_LIB* SYMBOL_LIBS::AddLibrary( const wxString& aFileName, SYMBOL_LIBS::iterator& aIterator )
{
    // Don't reload the library if it is already loaded.
    wxFileName fn( aFileName );
    SYMBOL_LIB* lib = FindLibrary( fn.GetName() );

    if( lib )
        return lib;

    try
    {
        lib = SYMBOL_LIB::LoadLibrary( aFileName );

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


SYMBOL_LIB* SYMBOL_LIBS::FindLibrary( const wxString& aName )
{
    for( SYMBOL_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->GetName() == aName )
            return &*it;
    }

    return nullptr;
}


SYMBOL_LIB* SYMBOL_LIBS::GetCacheLibrary()
{
    for( SYMBOL_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->IsCache() )
            return &*it;
    }

    return nullptr;
}


SYMBOL_LIB* SYMBOL_LIBS::FindLibraryByFullFileName( const wxString& aFullFileName )
{
    for( SYMBOL_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->GetFullFileName() == aFullFileName )
            return &*it;
    }

    return nullptr;
}


wxArrayString SYMBOL_LIBS::GetLibraryNames( bool aSorted )
{
    wxArrayString cacheNames;
    wxArrayString names;

    for( SYMBOL_LIB& lib : *this )
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


LIB_SYMBOL* SYMBOL_LIBS::FindLibSymbol( const LIB_ID& aLibId, const wxString& aLibraryName )
{
    LIB_SYMBOL* part = nullptr;

    for( SYMBOL_LIB& lib : *this )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        part = lib.FindSymbol( aLibId.GetLibItemName().wx_str() );

        if( part )
            break;
    }

    return part;
}


void SYMBOL_LIBS::FindLibraryNearEntries( std::vector<LIB_SYMBOL*>& aCandidates,
                                          const wxString& aEntryName,
                                          const wxString& aLibraryName )
{
    for( SYMBOL_LIB& lib : *this )
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


int        SYMBOL_LIBS::s_modify_generation = 1;     // starts at 1 and goes up
std::mutex SYMBOL_LIBS::s_generationMutex;


int SYMBOL_LIBS::GetModifyHash()
{
    int hash = 0;

    for( SYMBOL_LIBS::const_iterator it = begin();  it != end();  ++it )
    {
        hash += it->GetModHash();
    }

    // Rebuilding the cache (m_cache) does not change the GetModHash() value,
    // but changes SYMBOL_LIBS::s_modify_generation.
    // Take this change in account:
    hash += SYMBOL_LIBS::GetModifyGeneration();

    return hash;
}


void SYMBOL_LIBS::LibNamesAndPaths( PROJECT* aProject, bool doSave,
                                    wxString* aPaths, wxArrayString* aNames )
{
    wxCHECK_RET( aProject, "Null PROJECT in LibNamesAndPaths" );

    PROJECT_FILE& project = aProject->GetProjectFile();

    if( doSave )
    {
        if( aPaths )
            project.m_LegacyLibDir = *aPaths;

        if( aNames )
            project.m_LegacyLibNames = *aNames;
    }
    else
    {
        if( aPaths )
            *aPaths = project.m_LegacyLibDir;

        if( aNames )
            *aNames = project.m_LegacyLibNames;
    }
}


const wxString SYMBOL_LIBS::CacheName( const wxString& aFullProjectFilename )
{
    wxFileName  name = aFullProjectFilename;

    name.SetName( name.GetName() + "-cache" );
    name.SetExt( LegacySymbolLibFileExtension );

    if( name.FileExists() )
        return name.GetFullPath();

    return wxEmptyString;
}


void SYMBOL_LIBS::LoadAllLibraries( PROJECT* aProject, bool aShowProgress )
{
    wxString        filename;
    wxString        libs_not_found;
    SEARCH_STACK*   lib_search = aProject->SchSearchS();

#if defined(DEBUG) && 0
    lib_search->Show( __func__ );
#endif

    wxArrayString   lib_names;

    LibNamesAndPaths( aProject, false, nullptr, &lib_names );

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

        wxString progress_message;

        for( unsigned i = 0; i < lib_names.GetCount();  ++i )
        {
            if( aShowProgress )
            {
                lib_dialog.Update( i, wxString::Format( _( "Loading %s..." ), lib_names[i] ) );
            }

            // lib_names[] does not store the file extension. Set it.
            // Remember lib_names[i] can contain a '.' in name, so using a wxFileName
            // before adding the extension can create incorrect full filename
            wxString fullname = lib_names[i] + "." + LegacySymbolLibFileExtension;
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
                msg.Printf( _( "Symbol library '%s' failed to load. Error:\n %s" ),
                            filename,
                            ioe.What() );

                wxLogError( msg );
            }
        }
    }

    // add the special cache library.
    wxString cache_name = CacheName( aProject->GetProjectFullName() );
    SYMBOL_LIB* cache_lib;

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
            wxString msg =
                    wxString::Format( _( "Symbol library \"%s\" failed to load.\nError: %s" ),
                            cache_name, ioe.What() );

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
