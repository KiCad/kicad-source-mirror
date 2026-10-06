/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <io/easypc/easypc_document.h>
#include <io/easypc/easypc_classes_project.h>

#include <set>

#include <ki_exception.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/translation.h>


namespace EASYPC
{

namespace
{

    const char16_t SCM_COMPONENT_STREAM[] = u"[ScmComponent]";


    bool isLibrary( FILE_KIND aKind )
    {
        return aKind == FILE_KIND::PCB_SYMBOL_LIBRARY || aKind == FILE_KIND::SCHEMATIC_SYMBOL_LIBRARY
               || aKind == FILE_KIND::COMPONENT_LIBRARY;
    }


    void requireEnd( const ARCHIVE& aAr, const wxString& aWhat )
    {
        if( !aAr.AtEnd() )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Easy-PC %s ends at offset %zu of %zu." ), aWhat, aAr.Pos(), aAr.Size() ) );
        }
    }


    /// A counted list of project items; null entries are dropped
    LOAD_TASK readItemList( ARCHIVE& aAr, std::vector<SOURCE_PROJECT_ITEM*>& aItems )
    {
        for( uint32_t i = aAr.Count(); i > 0; --i )
        {
            if( SOURCE_PROJECT_ITEM* item = co_await aAr.ObjectAs<SOURCE_PROJECT_ITEM>() )
                aItems.push_back( item );
        }
    }

} // namespace


LOAD_TASK SOURCE_PROJECT_ITEM::Load( ARCHIVE& aAr )
{
    // Item fields gate on the project format
    const int v = aAr.CurrentReadFormat();

    aAr.Skip( 4 ); // type
    Name = aAr.ReadString();
    aAr.SkipString();

    if( v > 2 )
        aAr.Skip( 5 );

    if( v == 4 )
        aAr.Skip( 4 );

    if( v > 3 )
        aAr.SkipString();

    if( v > 4 )
        aAr.SkipString();

    co_return;
}


LOAD_TASK SOURCE_PROJECT::Load( ARCHIVE& aAr )
{
    if( uint32_t oleItems = aAr.U32() )
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC project holds %u OLE items." ), oleItems ) );

    Format = aAr.I32();

    if( Format < 0 || Format > 6 )
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC project format %d is not supported." ), Format ) );

    aAr.SetReadFormat( Format );

    co_await readItemList( aAr, Boards );
    co_await readItemList( aAr, Schematics );

    std::vector<SOURCE_PROJECT_ITEM*> other;

    if( Format > 0 )
        co_await readItemList( aAr, other );

    // Project properties: eleven strings, then a string map
    if( Format >= 6 )
    {
        for( int i = 0; i < 11; ++i )
            aAr.SkipString();

        for( int32_t i = aAr.I32(); i > 0; --i )
        {
            aAr.SkipString();
            aAr.SkipString();
        }
    }
}


std::unique_ptr<DESIGN_DOCUMENT> LoadDesign( const wxString& aPath )
{
    COMPOUND_FILE                    file( aPath );
    std::unique_ptr<DESIGN_DOCUMENT> doc = std::make_unique<DESIGN_DOCUMENT>();

    doc->FileKind = file.FileKind();

    if( doc->FileKind != FILE_KIND::PCB_DESIGN && doc->FileKind != FILE_KIND::SCHEMATIC_DESIGN )
    {
        THROW_IO_ERROR( wxString::Format( _( "'%s' is a %s, not a design." ), aPath,
                                          wxString::FromUTF8( FileKindString( doc->FileKind ) ) ) );
    }

    const SOURCE_FB_ENTRY* contents = file.FindChild( file.Root(), u"Contents" );

    if( !contents || !contents->IsStream() )
        THROW_IO_ERROR( wxString::Format( _( "'%s' has no Contents stream." ), aPath ) );

    if( const SOURCE_FB_ENTRY* summary = file.FindChild( file.Root(), u"\x05"
                                                                      u"SummaryInformation" ) )
        doc->SummaryInformation = file.ReadStream( *summary );

    doc->Kind = doc->FileKind == FILE_KIND::PCB_DESIGN ? DOC_KIND::PCB : DOC_KIND::SCHEMATIC;

    std::vector<uint8_t> data = file.ReadStream( *contents );
    const char*          root = doc->Kind == DOC_KIND::PCB ? "CPcbDesign" : "CScmDesign";

    // The design root sets the product from the header
    ARCHIVE ar( data.data(), data.size(), doc->Kind, 0 );
    doc->Design = ARCHIVE::Create( root );
    ar.LoadEmbedded( *doc->Design, root );
    requireEnd( ar, _( "design" ) );
    doc->Objects = ar.TakeObjects();
    return doc;
}


long long LIBRARY_FILE::Timestamp( const wxString& aPath )
{
    wxFileName fn( aPath );
    return fn.FileExists() ? fn.GetModificationTime().GetValue().GetValue() : 0;
}


LIBRARY_FILE::LIBRARY_FILE( const wxString& aPath ) :
        m_path( aPath ),
        m_file( aPath ),
        m_kind( m_file.FileKind() )
{
    if( !isLibrary( m_kind ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "'%s' is a %s, not a library." ), m_path,
                                          wxString::FromUTF8( FileKindString( m_kind ) ) ) );
    }

    m_product = m_file.LibraryProduct();

    // Items are root streams, or for a component library root storages
    const bool wantStorage = m_kind == FILE_KIND::COMPONENT_LIBRARY;

    for( const SOURCE_FB_ENTRY* child : m_file.Children( m_file.Root() ) )
    {
        if( child->IsAppStream() )
            continue;

        if( wantStorage ? !child->IsStorage() : !child->IsStream() )
        {
            THROW_IO_ERROR( wxString::Format( _( "Library '%s' holds '%s', which is not an item." ), m_path,
                                              UnescapeItemName( child->Name ) ) );
        }

        m_entries.push_back( child );
    }

    std::set<wxString> originalNames;

    for( const SOURCE_FB_ENTRY* entry : m_entries )
        originalNames.insert( UnescapeItemName( entry->Name ) );

    for( size_t i = 0; i < m_entries.size(); ++i )
    {
        wxString name = UnescapeItemName( m_entries[i]->Name );

        if( m_byName.count( name ) )
        {
            const wxString original = name;

            for( int suffix = 2; m_byName.count( name ) || originalNames.count( name ); ++suffix )
                name = wxString::Format( wxS( "%s (%d)" ), original, suffix );

            m_warnings.push_back( wxString::Format( _( "Library '%s' lists duplicate item '%s' as '%s'." ), m_path,
                                                    original, name ) );
        }

        m_names.push_back( name );
        m_byName.emplace( name, i );
    }
}


std::unique_ptr<LIBRARY_ITEM> LIBRARY_FILE::LoadItem( const wxString& aItemName ) const
{
    auto it = m_byName.find( aItemName );

    if( it == m_byName.end() )
        THROW_IO_ERROR( wxString::Format( _( "Library '%s' has no item '%s'." ), m_path, aItemName ) );

    const SOURCE_FB_ENTRY&        entry = *m_entries[it->second];
    std::unique_ptr<LIBRARY_ITEM> item = std::make_unique<LIBRARY_ITEM>();

    item->FileKind = m_kind;
    item->Name = aItemName;

    std::vector<const SOURCE_FB_ENTRY*> streams = { &entry };
    DOC_KIND kind = m_kind == FILE_KIND::PCB_SYMBOL_LIBRARY ? DOC_KIND::PCB_SYMBOL : DOC_KIND::SCH_SYMBOL;

    if( m_kind == FILE_KIND::COMPONENT_LIBRARY )
    {
        // The component stream first, then each package
        const SOURCE_FB_ENTRY* scm = m_file.FindChild( entry, SCM_COMPONENT_STREAM );

        if( !scm )
            THROW_IO_ERROR( wxString::Format( _( "Component '%s' has no [ScmComponent] stream." ), aItemName ) );

        kind = DOC_KIND::COMPONENT;
        streams = { scm };

        for( const SOURCE_FB_ENTRY* child : m_file.Children( entry ) )
        {
            if( child != scm )
                streams.push_back( child );
        }
    }

    for( const SOURCE_FB_ENTRY* stream : streams )
    {
        std::vector<uint8_t> data = m_file.ReadStream( *stream );
        ARCHIVE              ar( data.data(), data.size(), kind, m_product );

        item->Streams.push_back( LIBRARY_STREAM{ ar.ReadObject(), UnescapeItemName( stream->Name ) } );

        for( std::unique_ptr<OBJECT>& obj : ar.TakeObjects() )
            item->Objects.push_back( std::move( obj ) );
    }

    return item;
}


std::unique_ptr<PROJECT_DOCUMENT> LoadProject( const wxString& aPath )
{
    std::vector<uint8_t>              data = ReadWholeFile( aPath );
    ARCHIVE                           ar( data.data(), data.size(), DOC_KIND::PROJECT, 0 );
    std::unique_ptr<PROJECT_DOCUMENT> doc = std::make_unique<PROJECT_DOCUMENT>();

    doc->Project = std::make_unique<SOURCE_PROJECT>();
    ar.LoadEmbedded( *doc->Project, "CProject" );
    requireEnd( ar, _( "project" ) );
    doc->Objects = ar.TakeObjects();
    return doc;
}


wxString ResolveProjectItem( const wxString& aProjectPath, const wxString& aItemName )
{
    // Item names are relative to the project's directory
    wxString name = aItemName;
    name.Replace( wxS( "\\" ), wxS( "/" ) );

    wxFileName fn( name, wxPATH_UNIX );
    fn.MakeAbsolute( wxFileName( aProjectPath ).GetPath() );

    if( fn.FileExists() )
        return fn.GetFullPath();

    // Match project entries when the file was copied to a case-sensitive filesystem.
    wxDir    folder( fn.GetPath() );
    wxString found;

    for( bool more = folder.IsOpened() && folder.GetFirst( &found, wxEmptyString, wxDIR_FILES ); more;
         more = folder.GetNext( &found ) )
    {
        if( found.CmpNoCase( fn.GetFullName() ) == 0 )
        {
            fn.SetFullName( found );
            break;
        }
    }

    return fn.GetFullPath();
}


FILE_INFO Probe( const wxString& aPath )
{
    try
    {
        if( wxFileName( aPath ).GetExt().CmpNoCase( wxS( "prj" ) ) == 0 )
        {
            LoadProject( aPath );
            return { true, std::nullopt };
        }

        return { false, COMPOUND_FILE( aPath ).FileKind() };
    }
    catch( const std::exception& )
    {
        return {};
    }
}

} // namespace EASYPC
