/**********************************************************/
/*  libclass.cpp                                          */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "kicad_string.h"
#include "confirm.h"
#include "gestfich.h"
#include "eda_doc.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"

#include <boost/foreach.hpp>

#include <wx/tokenzr.h>
#include <wx/regex.h>


static const wxChar* duplicate_name_msg =
_( "Library <%s> has duplicate entry name <%s>.\n\
This may cause some unexpected behavior when loading components into a schematic." );


static bool DuplicateEntryName( const CMP_LIB_ENTRY& aItem1,
                                const CMP_LIB_ENTRY& aItem2 )
{
    return aItem1.GetName().CmpNoCase( aItem2.GetName() ) == 0;
}


bool operator==( const CMP_LIBRARY& aLibrary, const wxChar* aName )
{
    return aLibrary.GetName().CmpNoCase( aName ) == 0;
}


bool operator!=( const CMP_LIBRARY& aLibrary, const wxChar* aName )
{
    return !( aLibrary == aName );
}


bool operator<( const CMP_LIBRARY& aItem1, const CMP_LIBRARY& aItem2 )
{
    /* The cache library always is sorted to the end of the library list. */
    if( aItem2.IsCache() )
        return true;
    if( aItem1.IsCache() )
        return false;

    /* If the sort order array isn't set, then sort alphabetically except. */
    if( CMP_LIBRARY::GetSortOrder().IsEmpty() )
        return aItem1.GetName().CmpNoCase( aItem2.GetName() ) < 0;

    int i1 = CMP_LIBRARY::GetSortOrder().Index( aItem1.GetName(), false );
    int i2 = CMP_LIBRARY::GetSortOrder().Index( aItem2.GetName(), false );

    if( i1 == wxNOT_FOUND && i2 == wxNOT_FOUND )
        return true;

    if( i1 == wxNOT_FOUND && i2 != wxNOT_FOUND )
        return false;

    if( i1 != wxNOT_FOUND && i2 == wxNOT_FOUND )
        return true;

    return ( i1 - i2 ) < 0;
}


CMP_LIBRARY::CMP_LIBRARY( int aType, const wxFileName& aFileName )
{
    m_Type = aType;
    isModified = false;
    timeStamp = 0;
    m_Flags = 0;
    isCache = false;
    timeStamp = wxDateTime::Now();

    if( aFileName.IsOk() )
        fileName = aFileName;
    else
        fileName = wxFileName( wxT( "unnamed.lib" ) );
}


CMP_LIBRARY::~CMP_LIBRARY()
{
}


void CMP_LIBRARY::GetEntryNames( wxArrayString& aNames, bool aSort, bool aMakeUpperCase )
{
    BOOST_FOREACH( CMP_LIB_ENTRY& entry, entries )
    {
        if( aMakeUpperCase )
        {
            wxString tmp = entry.GetName();
            tmp.MakeUpper();
            aNames.Add( tmp );
        }
        else
        {
            aNames.Add( entry.GetName() );
        }
    }

    if( aSort )
        aNames.Sort();
}


void CMP_LIBRARY::SearchEntryNames( wxArrayString& aNames,
                                    const wxString& aNameSearch,
                                    const wxString& aKeySearch,
                                    bool aSort )
{
    BOOST_FOREACH( CMP_LIB_ENTRY& entry, entries )
    {
        if( !aKeySearch.IsEmpty() && KeyWordOk( aKeySearch, entry.GetKeyWords() ) )
            aNames.Add( entry.GetName() );
        if( !aNameSearch.IsEmpty() && WildCompareString( aNameSearch,
                                                         entry.GetName(),
                                                         false ) )
            aNames.Add( entry.GetName() );
    }

    if( aSort )
        aNames.Sort();
}


void CMP_LIBRARY::SearchEntryNames( wxArrayString& aNames, const wxRegEx& aRe,
                                    bool aSort )
{
    if( !aRe.IsValid() )
        return;

    BOOST_FOREACH( CMP_LIB_ENTRY& entry, entries )
    {
        if( aRe.Matches( entry.GetKeyWords() ) )
            aNames.Add( entry.GetName() );
    }

    if( aSort )
        aNames.Sort();
}


CMP_LIB_ENTRY* CMP_LIBRARY::FindEntry( const wxChar* aName )
{
    BOOST_FOREACH( CMP_LIB_ENTRY& entry, entries )
    {
        if( entry.GetName().CmpNoCase( aName ) == 0 )
            return &entry;
    }

    return NULL;
}


CMP_LIB_ENTRY* CMP_LIBRARY::FindEntry( const wxChar* aName, LibrEntryType aType )
{
    BOOST_FOREACH( CMP_LIB_ENTRY& entry, entries )
    {
        if( entry.GetName().CmpNoCase( aName ) == 0 && entry.GetType() == aType )
            return &entry;
    }

    return NULL;
}

/**
 * Return the first entry in the library.
 * @return The first entry or NULL if the library has no entries.
 */
CMP_LIB_ENTRY* CMP_LIBRARY::GetFirstEntry()
{
    if( entries.size() )
        return &entries.front();
    else
        return NULL;
}

LIB_COMPONENT* CMP_LIBRARY::FindComponent( const wxChar* aName )
{
    LIB_COMPONENT* component = NULL;
    CMP_LIB_ENTRY* entry = FindEntry( aName );

    if( entry != NULL && entry->isAlias() )
    {
        LIB_ALIAS* alias = (LIB_ALIAS*) entry;
        component = alias->GetComponent();
    }
    else
    {
        component = (LIB_COMPONENT*) entry;
    }

    return component;
}


bool CMP_LIBRARY::AddAlias( LIB_ALIAS* aAlias )
{
    wxASSERT( aAlias != NULL );

    if( FindEntry( aAlias->GetName() ) != NULL )
    {
        wxString msg;

        msg.Printf( _( "Cannot add duplicate alias <%s> to library <%s>." ),
                    GetChars( aAlias->GetName() ),
                    GetChars( fileName.GetName() ) );
        return false;
    }

    entries.push_back( (CMP_LIB_ENTRY*) aAlias );
    SetModifyFlags( );
    return true;
}


/**
 * Add /a aComponent entry to library.
 * Note a component can have an alias list,
 * so these alias will be added in library.
 * Conflicts can happen if aliases are already existing.
 * User is asked to choose what alias is removed (existing, or new)
 * a special case is the library cache:
 *   user is not asked, and old aliases removed.
 *   this is not perfect, but sufficient to create a library cache project
 * @param aComponent - Component to add.
 * @return Added component if successful.
 */
LIB_COMPONENT* CMP_LIBRARY::AddComponent( LIB_COMPONENT* aComponent )
{
    if( aComponent == NULL )
        return NULL;

    LIB_COMPONENT* newCmp = new LIB_COMPONENT( *aComponent, this );
    newCmp->ClearAliasDataDoc();    // Remove data used only in edition

    // Conflict detection: See if already existing aliases exist,
    // and if yes, ask user for continue or abort
    // Special case: if the library is the library cache of the project,
    // old aliases are always removed to avoid conflict,
    //      and user is not prompted )
    if( !IsCache() )
    {
        wxString msg;
        int conflict_count = 0;
        for( size_t i = 0; i < newCmp->m_AliasList.GetCount(); i++ )
        {
            LIB_ALIAS* alias = FindAlias( newCmp->m_AliasList[ i ] );

            if( alias == NULL )
                continue;
            LIB_COMPONENT*  cparent = alias->GetComponent();

            if( cparent == NULL ||  // Lib error, should not occur.
                ( cparent->GetName().CmpNoCase( newCmp->GetName() ) != 0 ) )
            {
                if( cparent )
                    msg = cparent->GetName();
                else
                    msg = _( "unknown" );
                wxString msg1;
                wxString parentName;
                if( cparent )
                    parentName = cparent->GetName();
                else
                    parentName = _("not found");
                msg1.Printf( _( "alias <%s> already exists and has root name<%s>" ),
                             GetChars( alias->GetName() ),
                             GetChars( parentName ) );
                msg << msg1 << wxT( "\n" );
                conflict_count++;
            }

            if( conflict_count > 20 )
                break;
        }

        if( conflict_count ) // Conflict: ask user what he wants: remove all aliases or abort:
        {
            wxString title;
            wxString msg1;
            title.Printf( _( "Conflict in library <%s>"), GetChars( fileName.GetName()));
            msg1.Printf( _("and appears in alias list of current component <%s>." ),
                         GetChars( newCmp->GetName() ) );
            msg << wxT( "\n\n" ) << msg1;
            msg << wxT( "\n\n" ) << _( "All old aliases will be removed. Continue ?" );
            int diag = wxMessageBox( msg, title, wxYES | wxICON_QUESTION );
            if( diag != wxYES )
                return NULL;
        }
    }

    for( size_t i = 0; i < newCmp->m_AliasList.GetCount(); i++ )
    {
        wxString aliasname = newCmp->m_AliasList[ i ];
        LIB_ALIAS* alias = FindAlias( aliasname );

        if( alias == NULL )
        {
            alias = new LIB_ALIAS( aliasname, newCmp );
            entries.push_back( alias );
        }
        else
        {
            LIB_COMPONENT* cparent = alias->GetComponent();

            if( cparent == NULL ||  // Lib error, should not occurs
                ( cparent->GetName().CmpNoCase( newCmp->GetName() ) != 0) )
            {
                // Remove alias from library and alias list of its root component
                RemoveEntry( alias );
                alias = new LIB_ALIAS( aliasname, newCmp );
                entries.push_back( alias );
            }
        }
        // Update alias data:
        alias->SetDescription( aComponent->GetAliasDataDoc( aliasname ) );
        alias->SetKeyWords( aComponent->GetAliasDataKeyWords( aliasname ) );
        alias->SetDocFileName( aComponent->GetAliasDataDocFileName( aliasname ) );
    }

    entries.push_back( (CMP_LIB_ENTRY*) newCmp );
    SetModifyFlags( );

    entries.sort();
    entries.unique( DuplicateEntryName );

    return newCmp;
}

/** function RemoveEntryName
 * Remove an /a aName entry from the library list names.
 * Warning: this is a partiel remove, because if aName is an alias
 * it is not removed from its root component.
 * this is for internal use only
 * Use RemoveEntry( CMP_LIB_ENTRY* aEntry ) to remove safely an entry in library.
 * @param aName - Entry name to remove from library.
 */
void CMP_LIBRARY::RemoveEntryName( const wxString& aName )
{
    LIB_ENTRY_LIST::iterator i;

    for( i = entries.begin(); i < entries.end(); i++ )
    {
        if( i->GetName().CmpNoCase( aName ) == 0 )
        {
            entries.erase( i );
            return;
        }
    }
}


/**
 * Remove safely an /a aEntry from the library.
 *
 * If the entry is an alias, the alias is removed from the library and from
 * the alias list of the root component.  If the entry is a root component
 * with no aliases, it is removed from the library.  If the entry is a root
 * component with aliases, the root component is renamed to the name of
 * the first alias and the root name for all remaining aliases are updated
 * to reflect the new root name.
 *
 * @param aEntry - Entry to remove from library.
 */
 void CMP_LIBRARY::RemoveEntry( CMP_LIB_ENTRY* aEntry )
{
    wxASSERT( aEntry != NULL );

    LIB_COMPONENT* root;
    LIB_ALIAS*  alias;

    SetModifyFlags( );

    if( aEntry->isAlias() )
    {
        alias = (LIB_ALIAS*) aEntry;
        root = alias->GetComponent();

        /* Remove alias name from the root component alias list */
        if( root == NULL )  // Should not occur, but is not a fatal error
        {
            wxLogDebug( wxT( "No root component found for alias <%s> in library <%s>." ),
                          GetChars( aEntry->GetName() ),
                          GetChars( fileName.GetName() ) );
        }
        else
        {
            int index = root->m_AliasList.Index( aEntry->GetName(), false );

            if( index == wxNOT_FOUND )  // Should not occur, but is not a fatal error
            {
                wxLogDebug( wxT( "Alias <%s> not found in component <%s> alias list in \
library <%s>" ),
                              GetChars( aEntry->GetName() ),
                              GetChars( root->GetName() ),
                              GetChars( fileName.GetName() ) );
            }
            else
                root->m_AliasList.RemoveAt( index );
        }

        RemoveEntryName( alias->GetName() );

        return;
    }

    root = ( LIB_COMPONENT* ) aEntry;

    /* Entry is a component with no aliases so removal is simple. */
    if( root->m_AliasList.GetCount() == 0 )
    {
        RemoveEntryName( root->GetName() );
        return;
    }

    /* Entry is a component with one or more alias. */
    wxString aliasName = root->m_AliasList[0];

    /* The root component is not really deleted, it is renamed with the first
     * alias name. */
    alias = FindAlias( aliasName );

    if( alias == NULL )
    {
        wxLogWarning( wxT( "Alias <%s> for component <%s> not found in library <%s>" ),
                      GetChars( aliasName ),
                      GetChars( root->GetName() ),
                      GetChars( fileName.GetName() ) );
        return;
    }

    /* Remove the first alias name from the component alias list. */
    root->m_AliasList.RemoveAt( 0 );

    /* Rename the component to the name of the first alias. */
    root->SetDescription( alias->GetDescription() );
    root->SetKeyWords( alias->GetKeyWords() );

    /* Remove the first alias from library. */
    RemoveEntryName( aliasName );

    /* Change the root name. */
    root->SetName( aliasName );
}


/**
 * Replace an existing component entry in the library.
 *
 * @param aOldComponent - The component to replace.
 * @param aNewComponent - The new component.
 * the new component and the old component are expected having the same name.
 */
LIB_COMPONENT* CMP_LIBRARY::ReplaceComponent( LIB_COMPONENT* aOldComponent,
                                              LIB_COMPONENT* aNewComponent )
{
    wxASSERT( aOldComponent != NULL );
    wxASSERT( aNewComponent != NULL );
    wxASSERT( aOldComponent->GetName().CmpNoCase( aNewComponent->GetName() )== 0 );

    size_t i;

    LIB_COMPONENT* newCmp = new LIB_COMPONENT( *aNewComponent, this );
    newCmp->ClearAliasDataDoc( );   // this data is currently used only when editing the component

    /* We want to remove the old root component, so we must remove old aliases.
     * even if they are not modified, because their root component will be removed
    */
    for( i = 0; i < aOldComponent->m_AliasList.GetCount(); i++ )
    {
        RemoveEntryName( aOldComponent->m_AliasList[ i ] );
    }

    /* Now, add current aliases. */
    for( i = 0; i < aNewComponent->m_AliasList.GetCount(); i++ )
    {
        wxString aliasname = aNewComponent->m_AliasList[ i ];
        LIB_ALIAS* alias = new LIB_ALIAS( aliasname, newCmp );
        // Update alias data:
        alias->SetDescription( aNewComponent->GetAliasDataDoc( aliasname ) );
        alias->SetKeyWords( aNewComponent->GetAliasDataKeyWords( aliasname ) );
        alias->SetDocFileName( aNewComponent->GetAliasDataDocFileName( aliasname ) );
        // Add it in library
        entries.push_back( alias );
    }

    RemoveEntryName( aOldComponent->GetName() );
    entries.push_back( newCmp );
    entries.sort();

    SetModifyFlags( );
    return newCmp;
}


CMP_LIB_ENTRY* CMP_LIBRARY::GetNextEntry( const wxChar* aName )
{
    size_t i;
    CMP_LIB_ENTRY* entry = NULL;

    for( i = 0; i < entries.size(); i++ )
    {
        if( entries[i].GetName().CmpNoCase( aName ) == 0 )
        {
            if( i < entries.size() - 1 )
            {
                entry = &entries[ i + 1 ];
                break;
            }
        }
    }

    if( entry == NULL )
        entry = &entries.front();

    return entry;
}


CMP_LIB_ENTRY* CMP_LIBRARY::GetPreviousEntry( const wxChar* aName )
{
    size_t i;
    CMP_LIB_ENTRY* entry = NULL;

    for( i = 0; i < entries.size(); i++ )
    {
        if( entries[i].GetName().CmpNoCase( aName ) == 0 && entry )
            break;

        entry = &entries[i];
    }

    return entry;
}


bool CMP_LIBRARY::Load( wxString& aErrorMsg )
{
    FILE*          file;
    int            lineNumber = 0;
    char           line[LINE_BUFFER_LEN_LARGE];  // Use a very large buffer to load data
    LIB_COMPONENT* libEntry;
    wxString       msg;

    if( fileName.GetFullPath().IsEmpty() )
    {
        aErrorMsg = _( "The component library file name is not set." );
        return false;
    }

    file = wxFopen( fileName.GetFullPath(), wxT( "rt" ) );

    if( file == NULL )
    {
        aErrorMsg = _( "The file could not be opened." );
        return false;
    }

    if( GetLine( file, line, &lineNumber, sizeof( line ) ) == NULL )
    {
        aErrorMsg = _( "The file is empty!" );
        return false;
    }

    /* There is no header if this is a symbol library. */
    if( m_Type == LIBRARY_TYPE_EESCHEMA )
    {
        wxString tmp;

        header = CONV_FROM_UTF8( line );

        wxStringTokenizer tkn( header );

        /*
         * The file header (first line) in library versions 2.0 and lower
         * apparently started with EESchema-LIB.  Sometime after 2.0, it
         * was changed to EESchema-LIBRARY.  Therefore, the test for
         * EESchema-LIB will work in both cases.  Don't change this unless
         * backwards compatibility is no longer required.
         */
        if( !tkn.HasMoreTokens()
            || !tkn.GetNextToken().Upper().StartsWith(wxT( "EESCHEMA-LIB" ) ) )
        {
            aErrorMsg = _( "The file is NOT an EESCHEMA library!" );
            return false;
        }

        if( !tkn.HasMoreTokens() )
        {
            aErrorMsg = _( "The file header is missing version and time stamp information." );
            return false;
        }

        if( tkn.GetNextToken() != wxT( "Version" ) || !tkn.HasMoreTokens() )
        {
            aErrorMsg = wxT( "The file header version information is invalid." );
            return false;
        }

        long major, minor;
        wxStringTokenizer vers( tkn.GetNextToken(), wxT( "." ) );

        if( !vers.HasMoreTokens() || !vers.GetNextToken().ToLong( &major )
            || major < 1L || !vers.HasMoreTokens()
            || !vers.GetNextToken().ToLong( & minor ) || minor < 0L
            || minor > 99 )
        {
#if 0   // Note for developers:
        // Not sure this warning is very useful: old designs *must* be always loadable
            wxLogWarning( wxT( "The component library <%s> header version \
number is invalid.\n\nIn future versions of EESchema this library may not \
load correctly.  To resolve this problem open the library in the library \
editor and save it.  If this library is the project cache library, save \
the current schematic." ),
                          GetChars( GetName() ) );
#endif
        }
        else
        {
            versionMajor = (int) major;
            versionMinor = (int) minor;

            wxLogDebug( wxT( "Component library <%s> is version %d.%d." ),
                        GetChars( GetName() ), versionMajor, versionMinor );
        }
    }

    while( GetLine( file, line, &lineNumber, sizeof( line ) ) )
    {
        if( m_Type == LIBRARY_TYPE_EESCHEMA
            && strnicmp( line, "$HEADER", 7 ) == 0 )
        {
            if( !LoadHeader( file, &lineNumber ) )
            {
                aErrorMsg = _( "An error occurred attempting to read the header." );
                return false;
            }

            continue;
        }

        if( strnicmp( line, "DEF", 3 ) == 0 )
        {
            /* Read one DEF/ENDDEF part entry from library: */
            libEntry = new LIB_COMPONENT( wxEmptyString, this );

            if( libEntry->Load( file, line, &lineNumber, msg ) )
            {
                /* Check for duplicate entry names and warn the user about
                 * the potential conflict.
                 */
                if( FindEntry( libEntry->GetName() ) != NULL )
                {
                    wxString msg( wxGetTranslation(duplicate_name_msg));
                    wxLogWarning( msg,
                                  GetChars( fileName.GetName() ),
                                  GetChars( libEntry->GetName() ) );
                }

                /* If we are here, this part is O.k. - put it in: */
                entries.push_back( libEntry );
                LoadAliases( libEntry );
            }
            else
            {
                wxLogWarning( _( "Library <%s> component load error %s." ),
                              GetChars( fileName.GetName() ),
                              GetChars( msg ) );
                msg.Clear();
                delete libEntry;
            }
        }
    }

    entries.sort();

    return true;
}


void CMP_LIBRARY::LoadAliases( LIB_COMPONENT* component )
{
    wxASSERT( component != NULL && component->isComponent() );

    LIB_ALIAS* alias;
    unsigned   ii;

    for( ii = 0; ii < component->m_AliasList.GetCount(); ii++ )
    {
        if( FindEntry( component->m_AliasList[ii] ) != NULL )
        {
            wxString msg( wxGetTranslation(duplicate_name_msg));
            wxLogError( msg,
                        GetChars( fileName.GetName() ),
                        GetChars( component->m_AliasList[ii] ) );
        }

        alias = new LIB_ALIAS( component->m_AliasList[ii], component, this );
        entries.push_back( alias );
    }
}


bool CMP_LIBRARY::LoadHeader( FILE* libfile, int* LineNum )
{
    char Line[LINE_BUFFER_LEN], * text, * data;

    while( GetLine( libfile, Line, LineNum, sizeof(Line) ) )
    {
        text = strtok( Line, " \t\r\n" );
        data = strtok( NULL, " \t\r\n" );
        if( stricmp( text, "TimeStamp" ) == 0 )
            timeStamp = atol( data );
        if( stricmp( text, "$ENDHEADER" ) == 0 )
            return TRUE;
    }

    return FALSE;
}


bool CMP_LIBRARY::LoadDocs( wxString& aErrorMsg )
{
    int            lineNumber = 0;
    char           line[LINE_BUFFER_LEN_LARGE], * name, * text;
    CMP_LIB_ENTRY* entry;
    FILE*          file;
    wxString       msg;
    wxFileName     fn = fileName;

    fn.SetExt( DOC_EXT );

    file = wxFopen( fn.GetFullPath(), wxT( "rt" ) );

    if( file == NULL )
    {
        aErrorMsg.Printf( _( "Could not open component document library file <%s>." ),
                          GetChars( fn.GetFullPath() ) );
        return false;
    }

    if( GetLine( file, line, &lineNumber, sizeof(line) ) == NULL )
    {
        aErrorMsg.Printf( _( "Component document library file <%s> is empty." ),
                          GetChars( fn.GetFullPath() ) );
        fclose( file );
        return false;
    }

    if( strnicmp( line, DOCFILE_IDENT, 10 ) != 0 )
    {
        aErrorMsg.Printf( _( "File <%s> is not a valid component library document file." ),
                          GetChars( fn.GetFullPath() ) );
        fclose( file );
        return false;
    }

    while( GetLine( file, line, &lineNumber, sizeof(line) ) )
    {
        if( strncmp( line, "$CMP", 4 ) != 0 )
        {
            aErrorMsg.Printf( wxT( "$CMP command expected in line %d, aborted." ),
                              lineNumber );
            fclose( file );
            return false;
        }

        /* Read one $CMP/$ENDCMP part entry from library: */
        name = strtok( line + 5, "\n\r" );

        wxString cmpname = CONV_FROM_UTF8( name );

        entry = FindEntry( cmpname );

        while( GetLine( file, line, &lineNumber, sizeof(line) ) )
        {
            if( strncmp( line, "$ENDCMP", 7 ) == 0 )
                break;
            text = strtok( line + 2, "\n\r" );

            switch( line[0] )
            {
            case 'D':
                if( entry )
                    entry->SetDescription( CONV_FROM_UTF8( text ) );
                break;

            case 'K':
                if( entry )
                    entry->SetKeyWords( CONV_FROM_UTF8( text ) );
                break;

            case 'F':
                if( entry )
                    entry->SetDocFileName( CONV_FROM_UTF8( text ) );
                break;
            }
        }
    }

    fclose( file );
    return true;
}


bool CMP_LIBRARY::Save( const wxString& aFullFileName, bool aOldDocFormat )
{
    FILE* libfile;
    wxString msg;
    wxFileName libFileName = aFullFileName;
    wxFileName backupFileName = aFullFileName;

    /* the old .lib file is renamed .bak */
    if( libFileName.FileExists() )
    {
        backupFileName.SetExt( wxT( "bak" ) );
        wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( libFileName.GetFullPath(),
                           backupFileName.GetFullPath() ) )
        {
            libFileName.MakeAbsolute();
            msg = wxT( "Failed to rename old component library file " ) +
                backupFileName.GetFullPath();
            DisplayError( NULL, msg );
        }
    }

    libfile = wxFopen( libFileName.GetFullPath(), wxT( "wt" ) );

    if( libfile == NULL )
    {
        libFileName.MakeAbsolute();
        msg = wxT( "Failed to create component library file " ) +
            libFileName.GetFullPath();
        DisplayError( NULL, msg );
        return false;
    }

    ClearModifyFlag( );

    timeStamp = GetTimeStamp();
    if( !SaveHeader( libfile ) )
    {
        fclose( libfile );
        return false;
    }

    bool success = true;

    BOOST_FOREACH( CMP_LIB_ENTRY& entry, entries )
    {
        if ( entry.isComponent() )
        {
            LIB_COMPONENT* component = ( LIB_COMPONENT* ) &entry;
            if ( !component->Save( libfile ) )
                success = false;
        }
    }

    if( fprintf( libfile, "#\n#End Library\n" ) < 0 )
        success = false;

    fclose( libfile );

    if( USE_OLD_DOC_FILE_FORMAT( versionMajor, versionMinor ) && aOldDocFormat )
        success = SaveDocFile( aFullFileName );

    return success;
}


bool CMP_LIBRARY::SaveDocFile( const wxString& aFullFileName )
{
    FILE* docfile;
    wxString msg;
    wxFileName backupFileName = aFullFileName;
    wxFileName docFileName = aFullFileName;

    docFileName.SetExt( DOC_EXT );

    /* Save current doc file as .bck */
    if( docFileName.FileExists() )
    {
        backupFileName = docFileName;
        backupFileName.SetExt( wxT( "bck" ) );
        wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( docFileName.GetFullPath(),
                           backupFileName.GetFullPath() ) )
        {
            msg = wxT( "Failed to save old library document file " ) +
                backupFileName.GetFullPath();
            DisplayError( NULL, msg );
        }
    }

    docfile = wxFopen( docFileName.GetFullPath(), wxT( "wt" ) );

    if( docfile == NULL )
    {
        docFileName.MakeAbsolute();
        msg = wxT( "Failed to create component document library file " ) +
            docFileName.GetFullPath();
        DisplayError( NULL, msg );
        return false;
    }

    char line[256];
    if( fprintf( docfile, "%s  Date: %s\n", DOCFILE_IDENT,
                 DateAndTime( line ) ) < 0 )
    {
        fclose( docfile );
        return false;
    }

    bool success = true;

    BOOST_FOREACH( CMP_LIB_ENTRY& entry, entries )
    {
        if ( !entry.SaveDoc( docfile ) )
            success = false;
    }

    if ( fprintf( docfile, "#\n#End Doc Library\n" ) < 0 )
        success = false;

    fclose( docfile );

    return success;
}


bool CMP_LIBRARY::SaveHeader( FILE* aFile )
{
    char BufLine[1024];
    bool succes = true;

    DateAndTime( BufLine );
    if( fprintf( aFile, "%s %d.%d  Date: %s\n", LIBFILE_IDENT,
                 LIB_VERSION_MAJOR, LIB_VERSION_MINOR, BufLine ) < 0 )
        succes = false;
#if 0
    if( ( fprintf( aFile, "$HEADER\n" ) < 0 )
        || ( fprintf( aFile, "TimeStamp %8.8lX\n", m_TimeStamp ) < 0 )
        || ( fprintf( aFile, "Parts %d\n", entries.size() ) != 2 )
        || ( fprintf( aFile, "$ENDHEADER\n" ) != 1 ) )
        succes = false;
#endif
    return succes;
}


/*
 * The static library list and list management methods.
 */
CMP_LIBRARY_LIST CMP_LIBRARY::libraryList;
wxArrayString CMP_LIBRARY::libraryListSortOrder;


CMP_LIBRARY* CMP_LIBRARY::LoadLibrary( const wxFileName& aFileName, wxString& aErrorMsg )
{
    CMP_LIBRARY* lib = NULL;

    lib = new CMP_LIBRARY( LIBRARY_TYPE_EESCHEMA, aFileName );

    wxBusyCursor ShowWait;

    if( !lib->Load( aErrorMsg ) )
    {
        delete lib;
        return NULL;
    }

    if( USE_OLD_DOC_FILE_FORMAT( lib->versionMajor, lib->versionMinor ) )
        lib->LoadDocs( aErrorMsg );

    return lib;
}


bool CMP_LIBRARY::AddLibrary( const wxFileName& aFileName, wxString& aErrorMsg )
{
    CMP_LIBRARY* lib;

    /* Don't reload the library if it is already loaded. */
    lib = FindLibrary( aFileName.GetName() );

    if( lib != NULL )
        return true;

    lib = LoadLibrary( aFileName, aErrorMsg );

    if( lib == NULL )
        return false;

    libraryList.push_back( lib );

    return true;
}


bool CMP_LIBRARY::AddLibrary( const wxFileName& aFileName, wxString& aErrorMsg,
                              CMP_LIBRARY_LIST::iterator& aIterator )
{
    CMP_LIBRARY* lib;

    /* Don't reload the library if it is already loaded. */
    lib = FindLibrary( aFileName.GetName() );

    if( lib != NULL )
        return true;

    lib = LoadLibrary( aFileName, aErrorMsg );

    if( lib == NULL )
        return false;

    if( aIterator >= libraryList.begin() && aIterator < libraryList.end() )
        libraryList.insert( aIterator, lib );
    else
        libraryList.push_back( lib );

    return true;
}


void CMP_LIBRARY::RemoveLibrary( const wxString& aName )
{
    if( aName.IsEmpty() )
        return;

    CMP_LIBRARY_LIST::iterator i;

    for ( i = libraryList.begin(); i < libraryList.end(); i++ )
    {
        if( i->GetName().CmpNoCase( aName ) == 0 )
        {
            CMP_LIBRARY::libraryList.erase( i );
            return;
        }
    }
}


 /**
 * Test for an existing library.
 * @param aLibptr - aLibptr.
 * @return true found.  false if not found.
 */
bool CMP_LIBRARY::LibraryExists( const CMP_LIBRARY* aLibptr )
{
    BOOST_FOREACH( CMP_LIBRARY& lib, libraryList )
    {
        if( &lib == aLibptr )
            return true;
    }

    return false;
}

CMP_LIBRARY* CMP_LIBRARY::FindLibrary( const wxString& aName )
{
    BOOST_FOREACH( CMP_LIBRARY& lib, libraryList )
    {
        if( lib == aName )
            return &lib;
    }

    return NULL;
}


wxArrayString CMP_LIBRARY::GetLibraryNames( bool aSorted )
{
    wxString cacheName;
    wxArrayString names;

    BOOST_FOREACH( CMP_LIBRARY& lib, CMP_LIBRARY::libraryList )
    {
        if( lib.isCache && aSorted )
            cacheName = lib.GetName();
        else
            names.Add( lib.GetName() );
    }

    /* Even sorted, the cache library is always at the end of the list. */
    if( aSorted )
        names.Sort();

    if( !cacheName.IsEmpty() )
        names.Add( cacheName );

    return names;
}


LIB_COMPONENT* CMP_LIBRARY::FindLibraryComponent( const wxString& aName,
                                                  const wxString& aLibraryName )
{
    LIB_COMPONENT* component = NULL;

    BOOST_FOREACH( CMP_LIBRARY& lib, libraryList )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        component = lib.FindComponent( aName );

        if( component != NULL )
            break;
    }

    return component;
}


CMP_LIB_ENTRY* CMP_LIBRARY::FindLibraryEntry( const wxString& aName,
                                              const wxString& aLibraryName )
{
    CMP_LIB_ENTRY* entry = NULL;

    BOOST_FOREACH( CMP_LIBRARY& lib, libraryList )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        entry = lib.FindEntry( aName );

        if( entry != NULL )
            break;
    }

    return entry;
}


void CMP_LIBRARY::RemoveCacheLibrary()
{
    CMP_LIBRARY_LIST::iterator i;

    for ( i = libraryList.begin(); i < libraryList.end(); i++ )
    {
        if( i->isCache )
            libraryList.erase( i-- );
    }
}
