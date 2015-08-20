/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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


/*

While exploring the possibility of local caching of the zip file, I discovered
this command to retrieve the time stamp of the last commit into any particular
repo:

 $time curl -I -i https://api.github.com/repos/KiCad/Mounting_Holes.pretty/commits

This gets just the header to what would otherwise return information on the repo
in JSON format, and is reasonably faster than actually getting the repo
in zip form.  However it still takes 5 seconds or more when github is busy, so
I have lost my enthusiasm for local caching until a faster time stamp retrieval
mechanism can be found, or github gets more servers.  But note that the occasionally
slow response is the exception rather than the norm.  Normally the response is
down around a 1/3 of a second.  The information we would use is in the header
named "Last-Modified" as seen below.  This would need parsing, but avhttp may
offer some help there, if not, then boost async probably does.


HTTP/1.1 200 OK
Server: GitHub.com
Date: Mon, 27 Jan 2014 15:46:46 GMT
Content-Type: application/json; charset=utf-8
Status: 200 OK
X-RateLimit-Limit: 60
X-RateLimit-Remaining: 49
X-RateLimit-Reset: 1390839612
Cache-Control: public, max-age=60, s-maxage=60
Last-Modified: Mon, 02 Dec 2013 10:08:51 GMT
ETag: "3d04d760f469f2516a51a56eac63bbd5"
Vary: Accept
X-GitHub-Media-Type: github.beta
X-Content-Type-Options: nosniff
Content-Length: 6310
Access-Control-Allow-Credentials: true
Access-Control-Expose-Headers: ETag, Link, X-RateLimit-Limit, X-RateLimit-Remaining, X-RateLimit-Reset, X-OAuth-Scopes, X-Accepted-OAuth-Scopes, X-Poll-Interval
Access-Control-Allow-Origin: *
X-GitHub-Request-Id: 411087C2:659E:50FD6E6:52E67F66
Vary: Accept-Encoding

*/


#ifndef WIN32_LEAN_AND_MEAN
// when WIN32_LEAN_AND_MEAN is defined, some useless includes in <window.h>
// are skipped, and this avoid some compil issues
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef WIN32
 // defines needed by avhttp
 // Minimal Windows version is XP: Google for _WIN32_WINNT
 #define _WIN32_WINNT   0x0501
 #define WINVER         0x0501
#endif

#include <sstream>
#include <boost/ptr_container/ptr_map.hpp>
#include <set>

#include <wx/zipstrm.h>
#include <wx/mstream.h>
#include <wx/uri.h>

#include <fctsys.h>
// Under Windows Mingw/msys, avhttp.hpp should be included after fctsys.h
// in fact after wx/wx.h, included by fctsys.h,
// to avoid issues (perhaps due to incompatible defines)
#include <avhttp.hpp>                       // chinese SSL magic

#include <io_mgr.h>
#include <richio.h>
#include <pcb_parser.h>
#include <class_board.h>
#include <github_plugin.h>
#include <class_module.h>
#include <macros.h>
#include <fp_lib_table.h>       // ExpandSubstitutions()
#include <github_getliblist.h>

using namespace std;


static const char* PRETTY_DIR = "allow_pretty_writing_to_this_dir";


typedef boost::ptr_map<string, wxZipEntry>  MODULE_MAP;
typedef MODULE_MAP::iterator                MODULE_ITER;
typedef MODULE_MAP::const_iterator          MODULE_CITER;


/**
 * Class GH_CACHE
 * assists only within GITHUB_PLUGIN and holds a map of footprint name to wxZipEntry
 */
struct GH_CACHE : public MODULE_MAP
{
    // MODULE_MAP is a boost::ptr_map template, made into a class hereby.
};


GITHUB_PLUGIN::GITHUB_PLUGIN() :
    PCB_IO(),
    m_gh_cache( 0 )
{
}


GITHUB_PLUGIN::~GITHUB_PLUGIN()
{
    delete m_gh_cache;
}


const wxString GITHUB_PLUGIN::PluginName() const
{
    return wxT( "Github" );
}


const wxString GITHUB_PLUGIN::GetFileExtension() const
{
    return wxEmptyString;
}


wxArrayString GITHUB_PLUGIN::FootprintEnumerate(
        const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    //D(printf("%s: this:%p  aLibraryPath:'%s'\n", __func__, this, TO_UTF8(aLibraryPath) );)
    cacheLib( aLibraryPath, aProperties );

    typedef std::set<wxString>      MYSET;

    MYSET   unique;

    if( m_pretty_dir.size() )
    {
        wxArrayString locals = PCB_IO::FootprintEnumerate( m_pretty_dir );

        for( unsigned i=0; i<locals.GetCount();  ++i )
            unique.insert( locals[i] );
    }

    for( MODULE_ITER it = m_gh_cache->begin();  it!=m_gh_cache->end();  ++it )
    {
        unique.insert( FROM_UTF8( it->first.c_str() ) );
    }

    wxArrayString ret;

    for( MYSET::const_iterator it = unique.begin();  it != unique.end();  ++it )
    {
        ret.Add( *it );
    }

    return ret;
}


MODULE* GITHUB_PLUGIN::FootprintLoad( const wxString& aLibraryPath,
        const wxString& aFootprintName, const PROPERTIES* aProperties )
{
    // D(printf("%s: this:%p  aLibraryPath:'%s'\n", __func__, this, TO_UTF8(aLibraryPath) );)

    // clear or set to valid the variable m_pretty_dir
    cacheLib( aLibraryPath, aProperties );

    if( m_pretty_dir.size() )
    {
        // API has FootprintLoad() *not* throwing an exception if footprint not found.
        MODULE* local = PCB_IO::FootprintLoad( m_pretty_dir, aFootprintName, aProperties );

        if( local )
        {
            // It has worked, see <src>/scripts/test_kicad_plugin.py.  So this was not firing:
            // wxASSERT( aFootprintName == FROM_UTF8( local->GetFPID().GetFootprintName().c_str() ) );
            // Moving it to higher API layer FP_LIB_TABLE::FootprintLoad().

            return local;
        }
    }

    UTF8 fp_name = aFootprintName;

    MODULE_CITER it = m_gh_cache->find( fp_name );

    if( it != m_gh_cache->end() )  // fp_name is present
    {
        wxMemoryInputStream mis( &m_zip_image[0], m_zip_image.size() );

        // This decoder should always be UTF8, since it was saved that way by git.
        // That is, since pretty footprints are UTF8, and they were pushed to the
        // github repo, they are still UTF8.
        wxZipInputStream    zis( mis, wxConvUTF8 );
        wxZipEntry*         entry = (wxZipEntry*) it->second;   // remove "const"-ness

        if( zis.OpenEntry( *entry ) )
        {
            INPUTSTREAM_LINE_READER reader( &zis, aLibraryPath );
#if 1
            // I am a PCB_IO derivative with my own PCB_PARSER
            m_parser->SetLineReader( &reader );     // ownership not passed

            MODULE* ret = (MODULE*) m_parser->Parse();
#else
            PCB_PARSER              parser( &reader );

            MODULE* ret = (MODULE*) parser.Parse();
#endif

            // Dude, the footprint name comes from the file name in
            // a github library.  Zero out the library name, we don't know it here.
            // Some caller may set the library nickname, one such instance is
            // FP_LIB_TABLE::FootprintLoad().
            ret->SetFPID( fp_name );

            return ret;
        }
    }

    return NULL;    // this API function returns NULL for "not found", per spec.
}


bool GITHUB_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    if( m_pretty_dir.size() )
        return PCB_IO::IsFootprintLibWritable( m_pretty_dir );
    else
        return false;
}


void GITHUB_PLUGIN::FootprintSave( const wxString& aLibraryPath,
        const MODULE* aFootprint, const PROPERTIES* aProperties )
{
    // set m_pretty_dir to either empty or something in aProperties
    cacheLib( aLibraryPath, aProperties );

    if( GITHUB_PLUGIN::IsFootprintLibWritable( aLibraryPath ) )
    {
        PCB_IO::FootprintSave( m_pretty_dir, aFootprint, aProperties );
    }
    else
    {
        // This typically will not happen if the caller first properly calls
        // IsFootprintLibWritable() to determine if calling FootprintSave() is
        // even legal, so I spend no time on internationalization here:

        string msg = StrPrintf( "Github library\n'%s'\nis only writable if you set option '%s' in Library Tables dialog.",
                (const char*) TO_UTF8( aLibraryPath ), PRETTY_DIR );

        THROW_IO_ERROR( msg );
    }
}


void GITHUB_PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
        const PROPERTIES* aProperties )
{
    // set m_pretty_dir to either empty or something in aProperties
    cacheLib( aLibraryPath, aProperties );

    if( GITHUB_PLUGIN::IsFootprintLibWritable( aLibraryPath ) )
    {
        // Does the PCB_IO base class have this footprint?
        // We cannot write to github.

        wxArrayString pretties = PCB_IO::FootprintEnumerate( m_pretty_dir, aProperties );

        if( pretties.Index( aFootprintName ) != wxNOT_FOUND )
        {
            PCB_IO::FootprintDelete( m_pretty_dir, aFootprintName, aProperties );
        }
        else
        {
            wxString msg = wxString::Format(
                    _( "Footprint\n'%s'\nis not in the writable portion of this Github library\n'%s'" ),
                    GetChars( aFootprintName ),
                    GetChars( aLibraryPath )
                    );

            THROW_IO_ERROR( msg );
        }
    }
    else
    {
        // This typically will not happen if the caller first properly calls
        // IsFootprintLibWritable() to determine if calling FootprintSave() is
        // even legal, so I spend no time on internationalization here:

        string msg = StrPrintf( "Github library\n'%s'\nis only writable if you set option '%s' in Library Tables dialog.",
                (const char*) TO_UTF8( aLibraryPath ), PRETTY_DIR );

        THROW_IO_ERROR( msg );
    }
}


void GITHUB_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // set m_pretty_dir to either empty or something in aProperties
    cacheLib( aLibraryPath, aProperties );

    if( m_pretty_dir.size() )
    {
        PCB_IO::FootprintLibCreate( m_pretty_dir, aProperties );
    }
    else
    {
        // THROW_IO_ERROR()   @todo
    }
}


bool GITHUB_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // set m_pretty_dir to either empty or something in aProperties
    cacheLib( aLibraryPath, aProperties );

    if( m_pretty_dir.size() )
    {
        return PCB_IO::FootprintLibDelete( m_pretty_dir, aProperties );
    }
    else
    {
        // THROW_IO_ERROR()   @todo
        return false;
    }
}


void GITHUB_PLUGIN::FootprintLibOptions( PROPERTIES* aListToAppendTo ) const
{
    // inherit options supported by all PLUGINs.
    PLUGIN::FootprintLibOptions( aListToAppendTo );

    (*aListToAppendTo)[ PRETTY_DIR ] = UTF8( _(
        "Set this property to a directory where footprints are to be written as pretty "
        "footprints when saving to this library. Anything saved will take precedence over "
        "footprints by the same name in the github repo.  These saved footprints can then "
        "be sent to the library maintainer as updates. "
        "<p>The directory <b>must</b> have a <b>.pretty</b> file extension because the "
        "format of the save is pretty.</p>"
        ));

    /*
    (*aListToAppendTo)["cache_github_zip_in_this_dir"] = UTF8( _(
        "Set this property to a directory where the github *.zip file will be cached. "
        "This should speed up subsequent visits to this library."
        ));
    */
}


void GITHUB_PLUGIN::cacheLib( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // This is edge triggered based on a change in 'aLibraryPath',
    // usually it does nothing.  When the edge fires, m_pretty_dir is set
    // to either:
    // 1) empty or
    // 2) a verified and validated, writable, *.pretty directory.

    if( !m_gh_cache || m_lib_path != aLibraryPath )
    {
        delete m_gh_cache;
        m_gh_cache = 0;

        m_pretty_dir.clear();

        if( aProperties )
        {
            UTF8  pretty_dir;

            if( aProperties->Value( PRETTY_DIR, &pretty_dir ) )
            {
                wxString    wx_pretty_dir = pretty_dir;

                wx_pretty_dir = FP_LIB_TABLE::ExpandSubstitutions( wx_pretty_dir );

                wxFileName wx_pretty_fn = wx_pretty_dir;

                if( !wx_pretty_fn.IsOk() ||
                    !wx_pretty_fn.IsDirWritable() ||
                    wx_pretty_fn.GetExt() != wxT( "pretty" )
                  )
                {
                    wxString msg = wxString::Format(
                            _( "option '%s' for Github library '%s' must point to a writable directory ending with '.pretty'." ),
                            GetChars( FROM_UTF8( PRETTY_DIR ) ),
                            GetChars( aLibraryPath )
                            );

                    THROW_IO_ERROR( msg );
                }

                m_pretty_dir = wx_pretty_dir;
            }
        }

        // operator==( wxString, wxChar* ) does not exist, construct wxString once here.
        const wxString    kicad_mod( wxT( "kicad_mod" ) );

        //D(printf("%s: this:%p  m_lib_path:'%s'  aLibraryPath:'%s'\n", __func__, this, TO_UTF8( m_lib_path), TO_UTF8(aLibraryPath) );)
        m_gh_cache = new GH_CACHE();

        // INIT_LOGGER( "/tmp", "test.log" );
        remote_get_zip( aLibraryPath );
        // UNINIT_LOGGER();

        m_lib_path = aLibraryPath;

        wxMemoryInputStream mis( &m_zip_image[0], m_zip_image.size() );

        // @todo: generalize this name encoding from a PROPERTY (option) later
        wxZipInputStream    zis( mis, wxConvUTF8 );

        wxZipEntry* entry;

        while( ( entry = zis.GetNextEntry() ) != NULL )
        {
            wxFileName  fn( entry->GetName() );     // chop long name into parts

            if( fn.GetExt() == kicad_mod )
            {
                UTF8 fp_name = fn.GetName();    // omit extension & path

                m_gh_cache->insert( fp_name, entry );
            }
            else
                delete entry;
        }
    }
}


bool GITHUB_PLUGIN::repoURL_zipURL( const wxString& aRepoURL, string* aZipURL )
{
    // e.g. "https://github.com/liftoff-sr/pretty_footprints"
    //D(printf("aRepoURL:%s\n", TO_UTF8( aRepoURL ) );)

    wxURI   repo( aRepoURL );

    if( repo.HasServer() && repo.HasPath() )
    {
        // scheme might be "http" or if truly github.com then "https".
        wxString zip_url = repo.GetScheme();

        zip_url += "://";

        if( repo.GetServer() == "github.com" )
        {
#if 0       // A proper code path would be this one, but it is not the fastest.
            zip_url += repo.GetServer();
            zip_url += repo.GetPath();      // path comes with a leading '/'
            zip_url += "/archive/master.zip";
#else
            // Github issues a redirect for the "master.zip".  i.e.
            //  "https://github.com/liftoff-sr/pretty_footprints/archive/master.zip"
            // would be redirected to:
            //  "https://codeload.github.com/liftoff-sr/pretty_footprints/zip/master"

            // In order to bypass this redirect, saving time, we use the
            // redirected URL on first attempt to save one HTTP GET hit.
            // avhttp would do the redirect behind the scenes normally, but that would
            // be slower than doing this bypass.
            zip_url += "codeload.github.com";
            zip_url += repo.GetPath();      // path comes with a leading '/'
            zip_url += "/zip/master";
#endif
        }
        else
        {
            // This is the generic code path for any server which can serve
            // up zip files which is not github.com. The schemes tested include:
            // http and https, I don't know what the avhttp library supports beyond that.

            // zip_url goal: "<scheme>://<server>[:<port>]/<path>"

            // Remember that <scheme>, <server>, <port> if present, and <path> all came
            // from the lib_path in the fp-lib-table row.

            zip_url += repo.GetServer();

            if( repo.HasPort() )
            {
                zip_url += ':';
                zip_url += repo.GetPort();
            }

            zip_url += repo.GetPath();      // path comes with a leading '/'

            // Do not modify the path, we cannot anticipate the needs of all
            // servers which are serving up zip files directly.  URL modifications
            // are more generally done in the server, rather than contaminating
            // this code path with the needs of one particular inflexible server.
        }

        *aZipURL = zip_url.utf8_str();
        return true;
    }
    return false;
}


void GITHUB_PLUGIN::remote_get_zip( const wxString& aRepoURL ) throw( IO_ERROR )
{
    string  zip_url;

    if( !repoURL_zipURL( aRepoURL, &zip_url ) )
    {
        wxString msg = wxString::Format( _( "Unable to parse URL:\n'%s'" ), GetChars( aRepoURL ) );
        THROW_IO_ERROR( msg );
    }

    boost::asio::io_service io;
    avhttp::http_stream     h( io );
    avhttp::request_opts    options;

    options.insert( "Accept",       "application/zip" );
    options.insert( "User-Agent",   "http://kicad-pcb.org" );   // THAT WOULD BE ME.
    h.request_options( options );

    try
    {
        ostringstream os;

        h.open( zip_url );      // only one file, therefore do it synchronously.
        os << &h;

        // Keep zip file byte image in RAM.  That plus the MODULE_MAP will constitute
        // the cache.  We don't cache the MODULEs per se, we parse those as needed from
        // this zip file image.
        m_zip_image = os.str();

        // 4 lines, using SSL, top that.
    }
    catch( boost::system::system_error& e )
    {
        // https "GET" has faild, report this to API caller.
        static const char errorcmd[] = "http GET command failed";  // Do not translate this message

        UTF8 fmt( _( "%s\nCannot get/download Zip archive: '%s'\nfor library path: '%s'.\nReason: '%s'" ) );

        string msg = StrPrintf( fmt.c_str(),
                errorcmd,
                // Report both secret zip_url and Lib Path, to user.  The secret
                // zip_url may go bad at some point in future if github changes
                // their server architecture.  Then fix repoURL_zipURL() to reflect
                // new architecture.
                zip_url.c_str(), TO_UTF8( aRepoURL ),
                e.what() );

        THROW_IO_ERROR( msg );
    }
}

// This GITHUB_GETLIBLIST method should not be here, but in github_getliblist.cpp !
// However it is here just because we need to include <avhttp.hpp> to compile it.
// and when we include avhttp in two .cpp files, the link fails because it detects duplicate
// avhttp functions.
// So until it is fixed, this code is here.
bool GITHUB_GETLIBLIST::remote_get_json( std::string* aFullURLCommand, wxString* aMsgError )
{
    boost::asio::io_service io;
    avhttp::http_stream     h( io );
    avhttp::request_opts    options;


    options.insert( "Accept", m_option_string );
    options.insert( "User-Agent", "http://kicad-pcb.org" );   // THAT WOULD BE ME.
    h.request_options( options );

    try
    {
        std::ostringstream os;

        h.open( *aFullURLCommand );      // only one file, therefore do it synchronously.
        os << &h;

        // Keep downloaded text file image in RAM.
        m_image = os.str();

        // 4 lines, using SSL, top that.
    }
    catch( boost::system::system_error& e )
    {
        // https "GET" has faild, report this to API caller.
        static const char errorcmd[] = "https GET command failed";  // Do not translate this message

        UTF8 fmt( _( "%s\nCannot get/download data from: '%s'\nReason: '%s'" ) );

        std::string msg = StrPrintf( fmt.c_str(),
                errorcmd,
                // Report secret list_url to user.  The secret
                // list_url may go bad at some point in future if github changes
                // their server architecture.  Then fix repoURL_zipURL() to reflect
                // new architecture.
                aFullURLCommand->c_str(), e.what() );

        if( aMsgError )
        {
            *aMsgError = FROM_UTF8( msg.c_str() );
            return false;
        }
    }

    return true;
}

#if 0 && defined(STANDALONE)

int main( int argc, char** argv )
{
    INIT_LOGGER( ".", "test.log" );

    GITHUB_PLUGIN gh;

    try
    {
        wxArrayString fps = gh.FootprintEnumerate(
                wxT( "https://github.com/liftoff-sr/pretty_footprints" ),
                NULL
                );

        for( int i=0; i<(int)fps.Count(); ++i )
        {
            printf("[%d]:%s\n", i, TO_UTF8( fps[i] ) );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        printf( "%s\n", TO_UTF8(ioe.errorText) );
    }

    UNINIT_LOGGER();

    return 0;
}

#endif
