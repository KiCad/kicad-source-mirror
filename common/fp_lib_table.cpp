/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-12 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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


#include <wx/config.h>      // wxExpandEnvVars()
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <set>

#include <appl_wxstruct.h>
#include <pcb_netlist.h>
#include <reporter.h>
#include <footprint_info.h>
#include <wildcards_and_files_ext.h>
#include <fpid.h>
#include <fp_lib_table_lexer.h>
#include <fp_lib_table.h>
#include <class_module.h>

using namespace FP_LIB_TABLE_T;


/**
 * Definition for enabling and disabling footprint library trace output.  See the
 * wxWidgets documentation on using the WXTRACE environment variable.
 */
static const wxString traceFpLibTable( wxT( "KicadFpLibTable" ) );


/// The footprint library table name used when no project file is passed to Pcbnew or CvPcb.
/// This is used temporarily to store the project specific library table until the project
/// file being edited is save.  It is then moved to the file fp-lib-table in the folder where
/// the project file is saved.
static wxString defaultProjectFileName( wxT( "prj-fp-lib-table" ) );

static wxString defaultFileName( wxT( "fp-lib-table" ) );


void FP_LIB_TABLE::ROW::SetType( const wxString& aType )
{
    type = IO_MGR::EnumFromStr( aType );

    if( IO_MGR::PCB_FILE_T( -1 ) == type )
        type = IO_MGR::KICAD;
}


void FP_LIB_TABLE::ROW::SetFullURI( const wxString& aFullURI )
{
    uri_user = aFullURI;

#if !FP_LATE_ENVVAR
    uri_expanded = FP_LIB_TABLE::ExpandSubstitutions( aFullURI );
#endif
}


const wxString FP_LIB_TABLE::ROW::GetFullURI( bool aSubstituted ) const
{
    if( aSubstituted )
    {
#if !FP_LATE_ENVVAR         // early expansion
        return uri_expanded;

#else   // late expansion
        return FP_LIB_TABLE::ExpandSubstitutions( uri_user );
#endif
    }
    else
        return uri_user;
}


FP_LIB_TABLE::ROW::ROW( const ROW& a ) :
    nickName( a.nickName ),
    type( a.type ),
    options( a.options ),
    description( a.description ),
    properties( 0 )
{
    // may call ExpandSubstitutions()
    SetFullURI( a.uri_user );

    if( a.properties )
        properties = new PROPERTIES( *a.properties );
}


FP_LIB_TABLE::ROW& FP_LIB_TABLE::ROW::operator=( const ROW& r )
{
    nickName     = r.nickName;
    type         = r.type;
    options      = r.options;
    description  = r.description;
    properties   = r.properties ? new PROPERTIES( *r.properties ) : NULL;

    // may call ExpandSubstitutions()
    SetFullURI( r.uri_user );

    // Do not copy the PLUGIN, it is lazily created.  Delete any existing
    // destination plugin.
    setPlugin( NULL );

    return *this;
}


bool FP_LIB_TABLE::ROW::operator==( const ROW& r ) const
{
    return nickName == r.nickName
        && uri_user == r.uri_user
        && type == r.type
        && options == r.options
        && description == r.description
        ;
}


FP_LIB_TABLE::FP_LIB_TABLE( FP_LIB_TABLE* aFallBackTable ) :
    fallBack( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "nickName not found".
}


wxArrayString FP_LIB_TABLE::FootprintEnumerate( const wxString& aNickname )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    return row->plugin->FootprintEnumerate( row->GetFullURI( true ), row->GetProperties() );
}


MODULE* FP_LIB_TABLE::FootprintLoad( const wxString& aNickname, const wxString& aFootprintName )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );

    MODULE* ret = row->plugin->FootprintLoad( row->GetFullURI( true ), aFootprintName, row->GetProperties() );

    // The library cannot know its own name, because it might have been renamed or moved.
    // Therefore footprints cannot know their own library nickname when residing in
    // a footprint library.
    // Only at this API layer can we tell the footprint about its actual library nickname.
    if( ret )
    {
        // remove "const"-ness, I really do want to set nickname without
        // having to copy the FPID and its two strings, twice each.
        FPID& fpid = (FPID&) ret->GetFPID();

        // Catch any misbehaving plugin, which should be setting internal footprint name properly:
        wxASSERT( aFootprintName == (wxString) fpid.GetFootprintName() );

        // and clearing nickname
        wxASSERT( !fpid.GetLibNickname().size() );

        fpid.SetLibNickname( row->GetNickName() );
    }

    return ret;
}


FP_LIB_TABLE::SAVE_T FP_LIB_TABLE::FootprintSave( const wxString& aNickname, const MODULE* aFootprint, bool aOverwrite )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );

    if( !aOverwrite )
    {
        // Try loading the footprint to see if it already exists, caller wants overwrite
        // protection, which is atypical, not the default.

        wxString fpname = aFootprint->GetFPID().GetFootprintName();

        std::auto_ptr<MODULE>   m( row->plugin->FootprintLoad( row->GetFullURI( true ), fpname, row->GetProperties() ) );

        if( m.get() )
            return SAVE_SKIPPED;
    }

    row->plugin->FootprintSave( row->GetFullURI( true ), aFootprint, row->GetProperties() );

    return SAVE_OK;
}


void FP_LIB_TABLE::FootprintDelete( const wxString& aNickname, const wxString& aFootprintName )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    return row->plugin->FootprintDelete( row->GetFullURI( true ), aFootprintName, row->GetProperties() );
}


bool FP_LIB_TABLE::IsFootprintLibWritable( const wxString& aNickname )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    return row->plugin->IsFootprintLibWritable( row->GetFullURI( true ) );
}


void FP_LIB_TABLE::FootprintLibDelete( const wxString& aNickname )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    row->plugin->FootprintLibDelete( row->GetFullURI( true ), row->GetProperties() );
}


void FP_LIB_TABLE::FootprintLibCreate( const wxString& aNickname )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    row->plugin->FootprintLibCreate( row->GetFullURI( true ), row->GetProperties() );
}


const wxString FP_LIB_TABLE::GetDescription( const wxString& aNickname )
{
    // use "no exception" form of find row:
    const ROW* row = findRow( aNickname );
    if( row )
        return row->description;
    else
        return wxEmptyString;
}


void FP_LIB_TABLE::Parse( FP_LIB_TABLE_LEXER* in ) throw( IO_ERROR, PARSE_ERROR )
{
    /*
        (fp_lib_table
            (lib (name NICKNAME)(descr DESCRIPTION)(type TYPE)(full_uri FULL_URI)(options OPTIONS))
            :
        )

        Elements after (name) are order independent.
    */

    T       tok;

    // This table may be nested within a larger s-expression, or not.
    // Allow for parser of that optional containing s-epression to have looked ahead.
    if( in->CurTok() != T_fp_lib_table )
    {
        in->NeedLEFT();
        if( ( tok = in->NextTok() ) != T_fp_lib_table )
            in->Expecting( T_fp_lib_table );
    }

    while( ( tok = in->NextTok() ) != T_RIGHT )
    {
        ROW     row;        // reconstructed for each row in input stream.

        if( tok == T_EOF )
            in->Expecting( T_RIGHT );

        if( tok != T_LEFT )
            in->Expecting( T_LEFT );

        // in case there is a "row integrity" error, tell where later.
        int lineNum = in->CurLineNumber();
        int offset  = in->CurOffset();

        if( ( tok = in->NextTok() ) != T_lib )
            in->Expecting( T_lib );

        // (name NICKNAME)
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_name )
            in->Expecting( T_name );

        in->NeedSYMBOLorNUMBER();

        row.SetNickName( in->FromUTF8() );

        in->NeedRIGHT();

        // After (name), remaining (lib) elements are order independent, and in
        // some cases optional.
        bool    sawType = false;
        bool    sawOpts = false;
        bool    sawDesc = false;
        bool    sawUri  = false;

        while( ( tok = in->NextTok() ) != T_RIGHT )
        {
            if( tok == T_EOF )
                in->Unexpected( T_EOF );

            if( tok != T_LEFT )
                in->Expecting( T_LEFT );

            tok = in->NeedSYMBOLorNUMBER();

            switch( tok )
            {
            case T_uri:
                if( sawUri )
                    in->Duplicate( tok );
                sawUri = true;
                in->NeedSYMBOLorNUMBER();
                row.SetFullURI( in->FromUTF8() );
                break;

            case T_type:
                if( sawType )
                    in->Duplicate( tok );
                sawType = true;
                in->NeedSYMBOLorNUMBER();
                row.SetType( in->FromUTF8() );
                break;

            case T_options:
                if( sawOpts )
                    in->Duplicate( tok );
                sawOpts = true;
                in->NeedSYMBOLorNUMBER();
                row.SetOptions( in->FromUTF8() );
                break;

            case T_descr:
                if( sawDesc )
                    in->Duplicate( tok );
                sawDesc = true;
                in->NeedSYMBOLorNUMBER();
                row.SetDescr( in->FromUTF8() );
                break;

            default:
                in->Unexpected( tok );
            }

            in->NeedRIGHT();
        }

        if( !sawType )
            in->Expecting( T_type );

        if( !sawUri )
            in->Expecting( T_uri );

        // all nickNames within this table fragment must be unique, so we do not
        // use doReplace in InsertRow().  (However a fallBack table can have a
        // conflicting nickName and ours will supercede that one since in
        // FindLib() we search this table before any fall back.)
        if( !InsertRow( row ) )
        {
            wxString msg = wxString::Format(
                                _( "'%s' is a duplicate footprint library nickName" ),
                                GetChars( row.nickName ) );
            THROW_PARSE_ERROR( msg, in->CurSource(), in->CurLine(), lineNum, offset );
        }
    }
}


void FP_LIB_TABLE::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR )
{
    out->Print( nestLevel, "(fp_lib_table\n" );

    for( ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
        it->Format( out, nestLevel+1 );

    out->Print( nestLevel, ")\n" );
}


void FP_LIB_TABLE::ROW::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR )
{
    out->Print( nestLevel, "(lib (name %s)(type %s)(uri %s)(options %s)(descr %s))\n",
                out->Quotew( GetNickName() ).c_str(),
                out->Quotew( GetType() ).c_str(),
                out->Quotew( GetFullURI() ).c_str(),
                out->Quotew( GetOptions() ).c_str(),
                out->Quotew( GetDescr() ).c_str()
                );
}


void FP_LIB_TABLE::Save( const wxFileName& aPath ) const throw( IO_ERROR )
{
    wxFileName fn = GetProjectFileName( aPath );

    wxLogTrace( traceFpLibTable, wxT( "Saving footprint libary table <%s>." ),
                GetChars( fn.GetFullPath() ) );

    FILE_OUTPUTFORMATTER sf( fn.GetFullPath() );
    Format( &sf, 0 );
}


#define OPT_SEP     '|'         ///< options separator character

PROPERTIES* FP_LIB_TABLE::ParseOptions( const std::string& aOptionsList )
{
    if( aOptionsList.size() )
    {
        const char* cp  = &aOptionsList[0];
        const char* end = cp + aOptionsList.size();

        PROPERTIES  props;
        std::string pair;

        // Parse all name=value pairs
        while( cp < end )
        {
            pair.clear();

            // Skip leading white space.
            while( cp < end && isspace( *cp )  )
                ++cp;

            // Find the end of pair/field
            while( cp < end )
            {
                if( *cp=='\\'  &&  cp+1<end  &&  cp[1]==OPT_SEP  )
                {
                    ++cp;           // skip the escape
                    pair += *cp++;  // add the separator
                }
                else if( *cp==OPT_SEP )
                {
                    ++cp;           // skip the separator
                    break;          // process the pair
                }
                else
                    pair += *cp++;
            }

            // stash the pair
            if( pair.size() )
            {
                // first equals sign separates 'name' and 'value'.
                size_t  eqNdx = pair.find( '=' );
                if( eqNdx != pair.npos )
                {
                    std::string name  = pair.substr( 0, eqNdx );
                    std::string value = pair.substr( eqNdx + 1 );
                    props[name] = value;
                }
                else
                    props[pair] = "";       // property is present, but with no value.
            }
        }

        if( props.size() )
            return new PROPERTIES( props );
    }
    return NULL;
}


UTF8 FP_LIB_TABLE::FormatOptions( const PROPERTIES* aProperties )
{
    UTF8 ret;

    if( aProperties )
    {
        for( PROPERTIES::const_iterator it = aProperties->begin();  it != aProperties->end();  ++it )
        {
            const std::string& name  = it->first;

            const UTF8& value = it->second;

            if( ret.size() )
                ret += OPT_SEP;

            ret += name;

            // the separation between name and value is '='
            if( value.size() )
            {
                ret += '=';

                for( std::string::const_iterator si = value.begin();  si != value.end();  ++si )
                {
                    // escape any separator in the value.
                    if( *si == OPT_SEP )
                        ret += '\\';

                    ret += *si;
                }
            }
        }
    }

    return ret;
}


std::vector<wxString> FP_LIB_TABLE::GetLogicalLibs()
{
    // Only return unique logical library names.  Use std::set::insert() to
    // quietly reject any duplicates, which can happen when encountering a duplicate
    // nickname from one of the fall back table(s).

    std::set<wxString>          unique;
    std::vector<wxString>       ret;
    const FP_LIB_TABLE*         cur = this;

    do
    {
        for( ROWS_CITER it = cur->rows.begin();  it!=cur->rows.end();  ++it )
        {
            unique.insert( it->nickName );
        }

    } while( ( cur = cur->fallBack ) != 0 );

    // return a sorted, unique set of nicknames in a std::vector<wxString> to caller
    for( std::set<wxString>::const_iterator it = unique.begin();  it!=unique.end();  ++it )
        ret.push_back( *it );

    return ret;
}


FP_LIB_TABLE::ROW* FP_LIB_TABLE::findRow( const wxString& aNickName ) const
{
    FP_LIB_TABLE* cur = (FP_LIB_TABLE*) this;

    do
    {
        cur->ensureIndex();

        INDEX_CITER  it = cur->nickIndex.find( aNickName );

        if( it != cur->nickIndex.end() )
        {
            return &cur->rows[it->second];  // found
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


const FP_LIB_TABLE::ROW* FP_LIB_TABLE::FindRowByURI( const wxString& aURI )
{
    FP_LIB_TABLE* cur = this;

    do
    {
        cur->ensureIndex();

        for( unsigned i = 0;  i < cur->rows.size();  i++ )
        {
            wxString uri = cur->rows[i].GetFullURI( true );

            if( wxFileName::GetPathSeparator() == wxChar( '\\' ) && uri.Find( wxChar( '/' ) ) >= 0 )
                uri.Replace( wxT( "/" ), wxT( "\\" ) );

            if( (wxFileName::IsCaseSensitive() && uri == aURI)
              || (!wxFileName::IsCaseSensitive() && uri.Upper() == aURI.Upper() ) )
            {
                return &cur->rows[i];  // found
            }
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


bool FP_LIB_TABLE::InsertRow( const ROW& aRow, bool doReplace )
{
    ensureIndex();

    INDEX_CITER it = nickIndex.find( aRow.nickName );

    if( it == nickIndex.end() )
    {
        rows.push_back( aRow );
        nickIndex.insert( INDEX_VALUE( aRow.nickName, rows.size() - 1 ) );
        return true;
    }

    if( doReplace )
    {
        rows[it->second] = aRow;
        return true;
    }

    return false;
}


const FP_LIB_TABLE::ROW* FP_LIB_TABLE::FindRow( const wxString& aNickname )
    throw( IO_ERROR )
{
    ROW* row = findRow( aNickname );

    if( !row )
    {
        wxString msg = wxString::Format(
            _( "fp-lib-table files contain no lib with nickname '%s'" ),
            GetChars( aNickname ) );

        THROW_IO_ERROR( msg );
    }

    // We've been 'lazy' up until now, but it cannot be deferred any longer,
    // instantiate a PLUGIN of the proper kind if it is not already in this ROW.
    if( !row->plugin )
        row->setPlugin( IO_MGR::PluginFind( row->type ) );

    return row;
}


// wxGetenv( wchar_t* ) is not re-entrant on linux.
// Put a lock on multithreaded use of wxGetenv( wchar_t* ), called from wxEpandEnvVars(),
// needed by bool ReadFootprintFiles( FP_LIB_TABLE* aTable, const wxString* aNickname = NULL );
#if 1

#include <ki_mutex.h>

const wxString FP_LIB_TABLE::ExpandSubstitutions( const wxString& aString )
{
    static MUTEX    getenv_mutex;

    MUTLOCK lock( getenv_mutex );

    // We reserve the right to do this another way, by providing our own member
    // function.
    return wxExpandEnvVars( aString );
}

#else

const wxString FP_LIB_TABLE::ExpandSubstitutions( const wxString& aString )
{
    // We reserve the right to do this another way, by providing our own member
    // function.
    return wxExpandEnvVars( aString );
}

#endif

bool FP_LIB_TABLE::IsEmpty( bool aIncludeFallback )
{
    if( !aIncludeFallback || (fallBack == NULL) )
        return rows.empty();

    return fallBack->IsEmpty() && rows.empty();
}


bool FP_LIB_TABLE::MissingLegacyLibs( const wxArrayString& aLibNames, wxString* aErrorMsg )
{
    bool retv = false;

    for( unsigned i = 0;  i < aLibNames.GetCount();  i++ )
    {
        wxFileName fn = wxFileName( wxEmptyString, aLibNames[i], LegacyFootprintLibPathExtension );
        wxString legacyLibPath = wxGetApp().FindLibraryPath( fn );

        if( legacyLibPath.IsEmpty() )
            continue;

        if( FindRowByURI( legacyLibPath ) == 0 )
        {
            retv = true;

            if( aErrorMsg )
                *aErrorMsg += wxT( "\"" ) + legacyLibPath + wxT( "\"\n" );
        }
    }

    return retv;
}


bool FP_LIB_TABLE::ConvertFromLegacy( NETLIST& aNetList, const wxArrayString& aLibNames,
                                      REPORTER* aReporter ) throw( IO_ERROR )
{
    wxString   msg;
    FPID       lastFPID;
    COMPONENT* component;
    MODULE*    module = 0;
    bool       retv = true;

    if( aNetList.IsEmpty() )
        return true;

    aNetList.SortByFPID();

    wxString   libPath;
    wxFileName fn;

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

    for( unsigned ii = 0; ii < aNetList.GetCount(); ii++ )
    {
        component = aNetList.GetComponent( ii );

        // The footprint hasn't been assigned yet so ignore it.
        if( component->GetFPID().empty() )
            continue;

        if( component->GetFPID() != lastFPID )
        {
            module = NULL;

            for( unsigned ii = 0; ii < aLibNames.GetCount(); ii++ )
            {
                fn = wxFileName( wxEmptyString, aLibNames[ii], LegacyFootprintLibPathExtension );

                libPath = wxGetApp().FindLibraryPath( fn );

                if( !libPath )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Cannot find footprint library file '%s' in any of the "
                                       "KiCad legacy library search paths.\n" ),
                                    GetChars( fn.GetFullPath() ) );
                        aReporter->Report( msg );
                    }

                    retv = false;
                    continue;
                }

                module = pi->FootprintLoad( libPath, component->GetFPID().GetFootprintName() );

                if( module )
                {
                    lastFPID = component->GetFPID();
                    break;
                }
            }
        }

        if( module == NULL )
        {
            if( aReporter )
            {
                msg.Printf( _( "Component `%s` footprint '%s' was not found in any legacy "
                               "library.\n" ),
                            GetChars( component->GetReference() ),
                            GetChars( component->GetFPID().Format() ) );
                aReporter->Report( msg );
            }

            // Clear the footprint assignment since the old library lookup method is no
            // longer valid.
            FPID emptyFPID;
            component->SetFPID( emptyFPID );
            retv = false;
            continue;
        }
        else
        {
            wxString libNickname;

            FP_LIB_TABLE* cur = this;

            do
            {
                cur->ensureIndex();

                for( unsigned i = 0;  i < cur->rows.size();  i++ )
                {
                    wxString uri = cur->rows[i].GetFullURI( true );

                    if( wxFileName::GetPathSeparator() == wxChar( '\\' )
                        && uri.Find( wxChar( '/' ) ) >= 0 )
                        uri.Replace( wxT( "/"), wxT( "\\" ) );
#ifdef __WINDOWS__
                    if( uri.CmpNoCase( libPath ) )
#else
                    if( uri == libPath )
#endif
                    {
                        libNickname = cur->rows[i].GetNickName();
                        break;
                    }
                }

            } while( ( cur = cur->fallBack ) != 0 && libNickname.IsEmpty() );

            if( libNickname.IsEmpty() )
            {
                if( aReporter )
                {
                    msg.Printf( _( "Component `%s` footprint '%s' legacy library path <%s > "
                                   "was not found in the footprint library table.\n" ),
                                GetChars( component->GetReference() ),
                                GetChars( component->GetFPID().Format() ) );
                    aReporter->Report( msg );
                }

                retv = false;
            }
            else
            {
                FPID newFPID = lastFPID;
                newFPID.SetLibNickname( libNickname );

                if( !newFPID.IsValid() )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Component `%s` FPID '%s' is not valid.\n" ),
                                    GetChars( component->GetReference() ),
                                    GetChars( newFPID.Format() ) );
                        aReporter->Report( msg );
                    }

                    retv = false;
                }
                else
                {
                    // The footprint name should already be set.
                    component->SetFPID( newFPID );
                }
            }
        }
    }

    return retv;
}


void FP_LIB_TABLE::SetProjectPathEnvVariable( const wxFileName& aPath )
{
    wxString path;

    if( !aPath.IsOk() || !aPath.DirExists() )
        path = wxEmptyString;
    else
        path = aPath.GetPath();

    wxLogTrace( traceFpLibTable, wxT( "Setting env %s to '%s'." ),
                GetChars( ProjectPathEnvVariableName() ), GetChars( path ) );
    wxSetEnv( ProjectPathEnvVariableName(), path );
}


const wxString FP_LIB_TABLE::ProjectPathEnvVariableName()
{
    return  wxT( "KIPRJMOD" );
}


const wxString FP_LIB_TABLE::GlobalPathEnvVariableName()
{
    return  wxT( KISYSMOD );
}


wxString FP_LIB_TABLE::GetProjectFileName( const wxFileName& aPath )
{
    wxFileName fn = aPath;

    // Set $KICAD_PRJ_PATH to user's configuration path if aPath is not set or does not exist.
    if( !aPath.IsOk() || !aPath.DirExists() )
    {
        fn.AssignDir( wxStandardPaths::Get().GetUserConfigDir() );

#if defined( __WINDOWS__ )
        fn.AppendDir( wxT( "kicad" ) );
#endif

        fn.SetName( defaultProjectFileName );
    }
    else
    {
        fn.AssignDir( aPath.GetPath() );
        fn.SetName( defaultFileName );
    }

    wxLogTrace( traceFpLibTable, wxT( "Project specific footprint library table file '%s'." ),
                GetChars( fn.GetFullPath() ) );

    return fn.GetFullPath();
}


bool FP_LIB_TABLE::LoadGlobalTable( FP_LIB_TABLE& aTable ) throw (IO_ERROR, PARSE_ERROR )
{
    bool tableExists = true;
    wxFileName fn = GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        tableExists = false;

        if( !fn.DirExists() && !fn.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Cannot create global library table path '%s'." ),
                                              GetChars( fn.GetPath() ) ) );
        }

        // Attempt to copy the default global file table from the KiCad template folder to
        // the users home configuration path.
        wxString fileName = wxGetApp().FindLibraryPath( defaultFileName );

        // The fallback is to create an empty global footprint table for the user to populate.
        if( fileName.IsEmpty() || !::wxCopyFile( fileName, fn.GetFullPath(), false ) )
        {
            FP_LIB_TABLE emptyTable;
            FILE_OUTPUTFORMATTER sf( fn.GetFullPath() );
            emptyTable.Format( &sf, 0 );
        }
    }

    FILE_LINE_READER reader( fn.GetFullPath() );
    FP_LIB_TABLE_LEXER lexer( &reader );

    aTable.Parse( &lexer );
    return tableExists;
}


wxString FP_LIB_TABLE::GetGlobalTableFileName()
{
    wxFileName fn;

    fn.SetPath( wxStandardPaths::Get().GetUserConfigDir() );

#if defined( __WINDOWS__ )
    fn.AppendDir( wxT( "kicad" ) );
#endif

    fn.SetName( GetFileName() );

    wxLogTrace( traceFpLibTable, wxT( "Global footprint library table file '%s'." ),
                GetChars( fn.GetFullPath() ) );

    return fn.GetFullPath();
}


const wxString& FP_LIB_TABLE::GetFileName()
{
    return defaultFileName;
}


void FP_LIB_TABLE::Load( const wxFileName& aFileName, FP_LIB_TABLE* aFallBackTable )
    throw( IO_ERROR )
{
    fallBack = aFallBackTable;

    // Empty footprint library tables are valid.
    if( aFileName.IsOk() && aFileName.FileExists() )
    {
        FILE_LINE_READER reader( aFileName.GetFullPath() );
        FP_LIB_TABLE_LEXER lexer( &reader );
        Parse( &lexer );
    }
}

