/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fctsys.h>
#include <kiface_i.h>
#include <gr_basic.h>
#include <macros.h>
#include <eda_base_frame.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <eda_doc.h>
#include <richio.h>
#include <config_params.h>
#include <wildcards_and_files_ext.h>
#include <project_rescue.h>
#include <properties.h>

#include <general.h>
#include <class_library.h>
#include <sch_legacy_plugin.h>

#include <wx/progdlg.h>
#include <wx/tokenzr.h>
#include <wx/regex.h>

#define DUPLICATE_NAME_MSG  \
    _(  "Library \"%s\" has duplicate entry name \"%s\".\n" \
        "This may cause some unexpected behavior when loading components into a schematic." )


PART_LIB::PART_LIB( int aType, const wxString& aFileName, SCH_IO_MGR::SCH_FILE_T aPluginType ) :
    // start @ != 0 so each additional library added
    // is immediately detectable, zero would not be.
    m_mod_hash( PART_LIBS::s_modify_generation ),
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


PART_LIB::~PART_LIB()
{
}


void PART_LIB::Save( bool aSaveDocFile )
{
    wxCHECK_RET( m_plugin != NULL, wxString::Format( "no plugin defined for library `%s`.",
                                                     fileName.GetFullPath() ) );

    PROPERTIES props;

    if( !aSaveDocFile )
        props[ SCH_LEGACY_PLUGIN::PropNoDocFile ] = "";

    m_plugin->SaveLibrary( fileName.GetFullPath(), &props );
    isModified = false;
}


void PART_LIB::Create( const wxString& aFileName )
{
    wxString tmpFileName = fileName.GetFullPath();

    if( !aFileName.IsEmpty() )
        tmpFileName = aFileName;

    m_plugin->CreateSymbolLib( tmpFileName, m_properties.get() );
}


void PART_LIB::SetPluginType( SCH_IO_MGR::SCH_FILE_T aPluginType )
{
    if( m_pluginType != aPluginType )
    {
        m_pluginType = aPluginType;
        m_plugin.reset( SCH_IO_MGR::FindPlugin( m_pluginType ) );
    }
}


bool PART_LIB::IsCache() const
{
    return m_properties->Exists( SCH_LEGACY_PLUGIN::PropNoDocFile );
}


void PART_LIB::SetCache()
{
    (*m_properties)[ SCH_LEGACY_PLUGIN::PropNoDocFile ] = "";
}


bool PART_LIB::IsBuffering() const
{
    return m_properties->Exists( SCH_LEGACY_PLUGIN::PropBuffering );
}


void PART_LIB::EnableBuffering( bool aEnable )
{
    if( aEnable )
        (*m_properties)[ SCH_LEGACY_PLUGIN::PropBuffering ] = "";
    else
        m_properties->Clear( SCH_LEGACY_PLUGIN::PropBuffering );
}


void PART_LIB::GetAliasNames( wxArrayString& aNames ) const
{
    m_plugin->EnumerateSymbolLib( aNames, fileName.GetFullPath(), m_properties.get() );

    aNames.Sort();
}


void PART_LIB::GetAliases( std::vector<LIB_ALIAS*>& aAliases ) const
{
    m_plugin->EnumerateSymbolLib( aAliases, fileName.GetFullPath(), m_properties.get() );

    std::sort( aAliases.begin(), aAliases.end(),
            [](LIB_ALIAS *lhs, LIB_ALIAS *rhs) -> bool
                { return lhs->GetName() < rhs->GetName(); });
}


LIB_ALIAS* PART_LIB::FindAlias( const wxString& aName ) const
{
    LIB_ALIAS* alias = m_plugin->LoadSymbol( fileName.GetFullPath(), aName, m_properties.get() );

    // Set the library to this even though technically the legacy cache plugin owns the
    // symbols.  This allows the symbol library table conversion tool to determine the
    // correct library where the symbol was found.
    if( alias && alias->GetPart() && !alias->GetPart()->GetLib() )
        alias->GetPart()->SetLib( const_cast<PART_LIB*>( this ) );

    return alias;
}


LIB_ALIAS* PART_LIB::FindAlias( const LIB_ID& aLibId ) const
{
    return FindAlias( aLibId.Format().wx_str() );
}


LIB_PART* PART_LIB::FindPart( const wxString& aName ) const
{
    LIB_ALIAS* alias = FindAlias( aName );

    if( alias != NULL )
        return alias->GetPart();

    return NULL;
}


LIB_PART* PART_LIB::FindPart( const LIB_ID& aLibId ) const
{
    return FindPart( aLibId.Format().wx_str() );
}


void PART_LIB::AddPart( LIB_PART* aPart )
{
    // add a clone, not the caller's copy, the plugin take ownership of the new symbol.
    m_plugin->SaveSymbol( fileName.GetFullPath(), new LIB_PART( *aPart, this ), m_properties.get() );

    // If we are not buffering, the library file is updated immediately when the plugin
    // SaveSymbol() function is called.
    if( IsBuffering() )
        isModified = true;

    ++m_mod_hash;
}


LIB_ALIAS* PART_LIB::RemoveAlias( LIB_ALIAS* aEntry )
{
    wxCHECK_MSG( aEntry != NULL, NULL, "NULL pointer cannot be removed from library." );

    m_plugin->DeleteAlias( fileName.GetFullPath(), aEntry->GetName(), m_properties.get() );

    // If we are not buffering, the library file is updated immediately when the plugin
    // SaveSymbol() function is called.
    if( IsBuffering() )
        isModified = true;

    ++m_mod_hash;
    return NULL;
}


LIB_PART* PART_LIB::ReplacePart( LIB_PART* aOldPart, LIB_PART* aNewPart )
{
    wxASSERT( aOldPart != NULL );
    wxASSERT( aNewPart != NULL );

    m_plugin->DeleteSymbol( fileName.GetFullPath(), aOldPart->GetName(), m_properties.get() );

    LIB_PART* my_part = new LIB_PART( *aNewPart, this );

    m_plugin->SaveSymbol( fileName.GetFullPath(), my_part, m_properties.get() );

    // If we are not buffering, the library file is updated immediately when the plugin
    // SaveSymbol() function is called.
    if( IsBuffering() )
        isModified = true;

    ++m_mod_hash;
    return my_part;
}


PART_LIB* PART_LIB::LoadLibrary( const wxString& aFileName )
{
    std::unique_ptr<PART_LIB> lib( new PART_LIB( LIBRARY_TYPE_EESCHEMA, aFileName ) );

    std::vector<LIB_ALIAS*> aliases;
    // This loads the library.
    lib->GetAliases( aliases );

    // Now, set the LIB_PART m_library member but it will only be used
    // when loading legacy libraries in the future. Once the symbols in the
    // schematic have a full #LIB_ID, this will not get called.
    for( size_t ii = 0; ii < aliases.size(); ii++ )
    {
        LIB_ALIAS* alias = aliases[ii];

        if( alias->GetPart() )
            alias->GetPart()->SetLib( lib.get() );
    }

    PART_LIB* ret = lib.release();
    return ret;
}


PART_LIB* PART_LIBS::AddLibrary( const wxString& aFileName )
{
    PART_LIB* lib;

    wxFileName fn = aFileName;
    // Don't reload the library if it is already loaded.
    lib = FindLibrary( fn.GetName() );

    if( lib )
        return lib;

    lib = PART_LIB::LoadLibrary( aFileName );

    push_back( lib );

    return lib;
}


PART_LIB* PART_LIBS::AddLibrary( const wxString& aFileName, PART_LIBS::iterator& aIterator )
{
    // Don't reload the library if it is already loaded.
    wxFileName fn( aFileName );
    PART_LIB* lib = FindLibrary( fn.GetName() );

    if( lib )
        return lib;

    lib = PART_LIB::LoadLibrary( aFileName );

    if( aIterator >= begin() && aIterator < end() )
        insert( aIterator, lib );
    else
        push_back( lib );

    return lib;
}


PART_LIB* PART_LIBS::FindLibrary( const wxString& aName )
{
    for( PART_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->GetName() == aName )
            return &*it;
    }

    return NULL;
}


PART_LIB* PART_LIBS::GetCacheLibrary()
{
    for( PART_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->IsCache() )
            return &*it;
    }

    return NULL;
}


PART_LIB* PART_LIBS::FindLibraryByFullFileName( const wxString& aFullFileName )
{
    for( PART_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->GetFullFileName() == aFullFileName )
            return &*it;
    }

    return NULL;
}


wxArrayString PART_LIBS::GetLibraryNames( bool aSorted )
{
    wxArrayString cacheNames;
    wxArrayString names;

    for( PART_LIB& lib : *this )
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


LIB_PART* PART_LIBS::FindLibPart( const LIB_ID& aLibId, const wxString& aLibraryName )
{
    LIB_PART* part = NULL;

    for( PART_LIB& lib : *this )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        part = lib.FindPart( aLibId.GetLibItemName().wx_str() );

        if( part )
            break;
    }

    return part;
}


LIB_ALIAS* PART_LIBS::FindLibraryAlias( const LIB_ID& aLibId, const wxString& aLibraryName )
{
    LIB_ALIAS* entry = NULL;

    for( PART_LIB& lib : *this )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        entry = lib.FindAlias( aLibId.GetLibItemName().wx_str() );

        if( entry )
            break;
    }

    return entry;
}


void PART_LIBS::FindLibraryNearEntries( std::vector<LIB_ALIAS*>& aCandidates,
                                        const wxString& aEntryName,
                                        const wxString& aLibraryName )
{
    for( PART_LIB& lib : *this )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        wxArrayString aliasNames;

        lib.GetAliasNames( aliasNames );

        if( aliasNames.IsEmpty() )
            continue;

        for( size_t i = 0;  i < aliasNames.size();  i++ )
        {
            if( aliasNames[i].CmpNoCase( aEntryName ) == 0 )
                aCandidates.push_back( lib.FindAlias( aliasNames[i] ) );
        }
    }
}


int PART_LIBS::s_modify_generation = 1;     // starts at 1 and goes up


int PART_LIBS::GetModifyHash()
{
    int hash = 0;

    for( PART_LIBS::const_iterator it = begin();  it != end();  ++it )
    {
        hash += it->GetModHash();
    }

    // Rebuilding the cache (m_cache) does not change the GetModHash() value,
    // but changes PART_LIBS::s_modify_generation.
    // Take this change in account:
    hash += PART_LIBS::s_modify_generation;

    return hash;
}


void PART_LIBS::LibNamesAndPaths( PROJECT* aProject, bool doSave,
                                  wxString* aPaths, wxArrayString* aNames )
{
    wxString pro = aProject->GetProjectFullName();

    PARAM_CFG_ARRAY ca;

    if( aPaths )
        ca.push_back( new PARAM_CFG_FILENAME( "LibDir", aPaths ) );

    if( aNames )
        ca.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ),  aNames, GROUP_SCH_LIBS ) );

    if( doSave )
    {
        aProject->ConfigSave( Kiface().KifaceSearch(), GROUP_SCH, ca );

        /*
        {
            wxString msg = wxString::Format( _(
                "Unable save project's \"%s\" file" ),
                GetChars( pro )
                );
            THROW_IO_ERROR( msg );
        }
        */
    }
    else
    {
        if( !aProject->ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH, ca ) )
        {
            wxString msg = wxString::Format( _(
                "Unable to load project's \"%s\" file" ),
                GetChars( pro )
                );
            THROW_IO_ERROR( msg );
        }
    }
}


const wxString PART_LIBS::CacheName( const wxString& aFullProjectFilename )
{
    wxFileName  name = aFullProjectFilename;

    name.SetName( name.GetName() + "-cache" );
    name.SetExt( SchematicLibraryFileExtension );

    if( name.FileExists() )
        return name.GetFullPath();

    return wxEmptyString;
}


void PART_LIBS::LoadAllLibraries( PROJECT* aProject, bool aShowProgress )
{
    wxString        filename;
    wxString        libs_not_found;
    SEARCH_STACK*   lib_search = aProject->SchSearchS();

#if defined(DEBUG) && 0
    lib_search->Show( __func__ );
#endif

    wxArrayString   lib_names;

    LibNamesAndPaths( aProject, false, NULL, &lib_names );

    // Post symbol library table, this should be empty.  Only the cache library should get loaded.
    if( !lib_names.empty() )
    {
        wxProgressDialog lib_dialog( _( "Loading Symbol Libraries" ),
                                     wxEmptyString,
                                     lib_names.GetCount(),
                                     NULL,
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
                lib_dialog.Update( i, _( "Loading " + lib_names[i] ) );
            }

            // lib_names[] does not store the file extension. Set it.
            // Remember lib_names[i] can contain a '.' in name, so using a wxFileName
            // before adding the extension can create incorrect full filename
            wxString fullname = lib_names[i] + "." + SchematicLibraryFileExtension;
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
                msg.Printf( _( "Symbol library \"%s\" failed to load. Error:\n %s" ),
                            GetChars( filename ), GetChars( ioe.What() ) );

                wxLogError( msg );
            }
        }
    }

    // add the special cache library.
    wxString cache_name = CacheName( aProject->GetProjectFullName() );
    PART_LIB* cache_lib;

    if( !cache_name.IsEmpty() )
    {
        try
        {
            cache_lib = AddLibrary( cache_name );

            if( cache_lib )
                cache_lib->SetCache();
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format( _(
                    "Symbol library \"%s\" failed to load.\nError: %s" ),
                    GetChars( cache_name ),
                    GetChars( ioe.What() )
                    );

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

#if defined(DEBUG) && 1
    printf( "%s: lib_names:\n", __func__ );

    for( PART_LIBS::const_iterator it = begin(); it < end(); ++it )
        printf( " %s\n", TO_UTF8( it->GetName() ) );
#endif
}
