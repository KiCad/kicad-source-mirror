/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
Note:
If you are using this plugin without the supporting nginx caching server, then you
will never be happy with its performance.  However, it is the fastest plugin in
existence when used with a local nginx and the nginx configuration file in this
directory.  Nginx can be running in house on your network anywhere for this statement
to be true.

Comments below pertain to use without nginx, so are not relevant in every case:

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
named "Last-Modified" as seen below.

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

#include <kicad_curl/kicad_curl_easy.h>     // Include before any wx file
#include <sstream>
#include <boost/ptr_container/ptr_map.hpp>
#include <set>

#include <wx/zipstrm.h>
#include <wx/mstream.h>
#include <wx/uri.h>

#include <fctsys.h>

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


typedef boost::ptr_map< wxString, wxZipEntry >  MODULE_MAP;
typedef MODULE_MAP::iterator                    MODULE_ITER;
typedef MODULE_MAP::const_iterator              MODULE_CITER;


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
    return "Github";
}


const wxString GITHUB_PLUGIN::GetFileExtension() const
{
    return wxEmptyString;
}


void GITHUB_PLUGIN::FootprintEnumerate( wxArrayString& aFootprintNames,
        const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    //D(printf("%s: this:%p  aLibraryPath:\"%s\"\n", __func__, this, TO_UTF8(aLibraryPath) );)
    cacheLib( aLibraryPath, aProperties );

    typedef std::set<wxString>      MYSET;

    MYSET   unique;

    if( m_pretty_dir.size() )
    {
        wxArrayString locals;

        PCB_IO::FootprintEnumerate( locals, m_pretty_dir );

        for( unsigned i=0; i<locals.GetCount();  ++i )
            unique.insert( locals[i] );
    }

    for( MODULE_ITER it = m_gh_cache->begin();  it!=m_gh_cache->end();  ++it )
    {
        unique.insert( it->first );
    }

    for( MYSET::const_iterator it = unique.begin();  it != unique.end();  ++it )
    {
        aFootprintNames.Add( *it );
    }
}


void GITHUB_PLUGIN::PrefetchLib(
        const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    if( m_lib_path != aLibraryPath )
    {
        m_zip_image.clear();
    }

    remoteGetZip( aLibraryPath );
}


MODULE* GITHUB_PLUGIN::FootprintLoad( const wxString& aLibraryPath,
        const wxString& aFootprintName, const PROPERTIES* aProperties )
{
    // D(printf("%s: this:%p  aLibraryPath:\"%s\"\n", __func__, this, TO_UTF8(aLibraryPath) );)

    // clear or set to valid the variable m_pretty_dir
    cacheLib( aLibraryPath, aProperties );

    if( m_pretty_dir.size() )
    {
        // API has FootprintLoad() *not* throwing an exception if footprint not found.
        MODULE* local = PCB_IO::FootprintLoad( m_pretty_dir, aFootprintName, aProperties );

        if( local )
        {
            // It has worked, see <src>/scripts/test_kicad_plugin.py.  So this was not firing:
            // wxASSERT( aFootprintName == FROM_UTF8( local->GetFPID().GetLibItemName().c_str() ) );
            // Moving it to higher API layer FP_LIB_TABLE::FootprintLoad().

            return local;
        }
    }

    MODULE_CITER it = m_gh_cache->find( aFootprintName );

    if( it != m_gh_cache->end() )  // fp_name is present
    {
        //std::string::data() ensures that the referenced data block is contiguous.
        wxMemoryInputStream mis( m_zip_image.data(), m_zip_image.size() );

        // This decoder should always be UTF8, since it was saved that way by git.
        // That is, since pretty footprints are UTF8, and they were pushed to the
        // github repo, they are still UTF8.
        wxZipInputStream    zis( mis, wxConvUTF8 );
        wxZipEntry*         entry = (wxZipEntry*) it->second;   // remove "const"-ness

        if( zis.OpenEntry( *entry ) )
        {
            INPUTSTREAM_LINE_READER reader( &zis, aLibraryPath );

            // I am a PCB_IO derivative with my own PCB_PARSER
            m_parser->SetLineReader( &reader );     // ownership not passed

            MODULE* ret = (MODULE*) m_parser->Parse();

            // In a github library, (as well as in a "KiCad" library) the name of
            // the pretty file defines the footprint name.  That filename trumps
            // any name found in the pretty file; any name in the pretty file
            // must be ignored here.  Also, the library nickname is unknown in
            // this context so clear it just in case.
            ret->SetFPID( LIB_ID( wxEmptyString, aFootprintName ) );

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

        string msg = StrPrintf( "Github library\n\"%s\"\nis only writable if you set option \"%s\" in Library Tables dialog.",
                TO_UTF8( aLibraryPath ), PRETTY_DIR );

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

        wxArrayString pretties;

        PCB_IO::FootprintEnumerate( pretties, m_pretty_dir, aProperties );

        if( pretties.Index( aFootprintName ) != wxNOT_FOUND )
        {
            PCB_IO::FootprintDelete( m_pretty_dir, aFootprintName, aProperties );
        }
        else
        {
            wxString msg = wxString::Format(
                    _( "Footprint\n\"%s\"\nis not in the writable portion of this Github library\n\"%s\"" ),
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

        string msg = StrPrintf( "Github library\n\"%s\"\nis only writable if you set option \"%s\" in Library Tables dialog.",
                TO_UTF8( aLibraryPath ), PRETTY_DIR );

        THROW_IO_ERROR( msg );
    }
}


void GITHUB_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // set m_pretty_dir to either empty or something in aProperties
    cacheLib( aLibraryPath, aProperties );

    if( m_pretty_dir.size() )
        PCB_IO::FootprintLibCreate( m_pretty_dir, aProperties );
}


bool GITHUB_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // set m_pretty_dir to either empty or something in aProperties
    cacheLib( aLibraryPath, aProperties );

    if( m_pretty_dir.size() )
        return PCB_IO::FootprintLibDelete( m_pretty_dir, aProperties );

    return false;
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

        if( !m_lib_path.empty() )
        {
            // Library path wasn't empty before - it's been changed. Flush out the prefetch cache.
            m_zip_image.clear();
        }

        if( aProperties )
        {
            UTF8  pretty_dir;

            if( aProperties->Value( PRETTY_DIR, &pretty_dir ) )
            {
                wxString    wx_pretty_dir = pretty_dir;

                wx_pretty_dir = LIB_TABLE::ExpandSubstitutions( wx_pretty_dir );

                wxFileName wx_pretty_fn = wx_pretty_dir;

                if( !wx_pretty_fn.IsOk() ||
                    !wx_pretty_fn.IsDirWritable() ||
                    wx_pretty_fn.GetExt() != "pretty"
                  )
                {
                    wxString msg = wxString::Format(
                            _( "option \"%s\" for Github library \"%s\" must point to a writable directory ending with '.pretty'." ),
                            GetChars( FROM_UTF8( PRETTY_DIR ) ),
                            GetChars( aLibraryPath )
                            );

                    THROW_IO_ERROR( msg );
                }

                m_pretty_dir = wx_pretty_dir;
            }
        }

        // operator==( wxString, wxChar* ) does not exist, construct wxString once here.
        const wxString    kicad_mod( "kicad_mod" );

        //D(printf("%s: this:%p  m_lib_path:'%s'  aLibraryPath:'%s'\n", __func__, this, TO_UTF8( m_lib_path), TO_UTF8(aLibraryPath) );)
        m_gh_cache = new GH_CACHE();

        // INIT_LOGGER( "/tmp", "test.log" );
        remoteGetZip( aLibraryPath );
        // UNINIT_LOGGER();

        m_lib_path = aLibraryPath;

        wxMemoryInputStream mis( &m_zip_image[0], m_zip_image.size() );

        // Recently the zip standard adopted UTF8 encoded filenames within the
        // internal zip directory block.  Please only use zip files that conform
        // to that standard.  Github seems to now, but may not have earlier.
        wxZipInputStream    zis( mis, wxConvUTF8 );

        wxZipEntry* entry;
        wxString    fp_name;

        while( ( entry = zis.GetNextEntry() ) != NULL )
        {
            wxFileName  fn( entry->GetName() );     // chop long name into parts

            if( fn.GetExt() == kicad_mod )
            {
                fp_name = fn.GetName();    // omit extension & path

                m_gh_cache->insert( fp_name, entry );
            }
            else
                delete entry;
        }
    }
}


long long GITHUB_PLUGIN::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    // This plugin currently relies on the nginx server for caching (see comments
    // at top of file).
    // Since only the nginx server holds the timestamp information, we must defeat
    // all caching above the nginx server.
    return wxDateTime::Now().GetValue().GetValue();

#if 0
    // If we have no cache, return a number which won't match any stored timestamps
    if( !m_gh_cache || m_lib_path != aLibraryPath )
        return wxDateTime::Now().GetValue().GetValue();

    long long hash = m_gh_cache->GetTimestamp();

    if( m_pretty_dir.size() )
        hash += PCB_IO::GetLibraryTimestamp( m_pretty_dir );

    return hash;
#endif
}


bool GITHUB_PLUGIN::repoURL_zipURL( const wxString& aRepoURL, std::string* aZipURL )
{
    // e.g. "https://github.com/liftoff-sr/pretty_footprints"
    //D(printf("aRepoURL:%s\n", TO_UTF8( aRepoURL ) );)

    wxURI   repo( aRepoURL );

    if( repo.HasServer() && repo.HasPath() )
    {
        // scheme might be "http" or if truly github.com then "https".
        wxString zip_url;

        if( repo.GetServer() == "github.com" )
        {
            //codeload.github.com only supports https
            zip_url = "https://";
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
            zip_url += "codeload.github.com";
            zip_url += repo.GetPath();      // path comes with a leading '/'
            zip_url += "/zip/master";
#endif
        }

        else
        {
            zip_url = repo.GetScheme();
            zip_url += "://";

            // This is the generic code path for any server which can serve
            // up zip files. The schemes tested include: http and https.

            // zip_url goal: "<scheme>://<server>[:<port>]/<path>"

            // Remember that <scheme>, <server>, <port> if present, and <path> all came
            // from the lib_path in the fp-lib-table row.

            // This code path is used with the nginx proxy setup, but is useful
            // beyond that.

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


void GITHUB_PLUGIN::remoteGetZip( const wxString& aRepoURL )
{
    std::string  zip_url;

    if( !m_zip_image.empty() )
        return;

    if( !repoURL_zipURL( aRepoURL, &zip_url ) )
    {
        wxString msg = wxString::Format( _( "Unable to parse URL:\n\"%s\"" ), GetChars( aRepoURL ) );
        THROW_IO_ERROR( msg );
    }

    wxLogDebug( wxT( "Attempting to download: " ) + zip_url );

    KICAD_CURL_EASY kcurl;      // this can THROW_IO_ERROR

    kcurl.SetURL( zip_url.c_str() );
    kcurl.SetUserAgent( "http://kicad-pcb.org" );
    kcurl.SetHeader( "Accept", "application/zip" );
    kcurl.SetFollowRedirects( true );

    try
    {
        kcurl.Perform();
        m_zip_image = kcurl.GetBuffer();
    }
    catch( const IO_ERROR& ioe )
    {
        // https "GET" has failed, report this to API caller.
        // Note: kcurl.Perform() does not return an error if the file to download is not found
        static const char errorcmd[] = "http GET command failed";  // Do not translate this message

        UTF8 fmt( _( "%s\nCannot get/download Zip archive: \"%s\"\nfor library path: \"%s\".\nReason: \"%s\"" ) );

        std::string msg = StrPrintf( fmt.c_str(),
                                     errorcmd,
                                     zip_url.c_str(),
                                     TO_UTF8( aRepoURL ),
                                     TO_UTF8( ioe.What() )
                                     );

        THROW_IO_ERROR( msg );
    }

    // If the zip archive is not existing, the received data is "Not Found" or "404: Not Found",
    // and no error is returned by kcurl.Perform().
    if( ( m_zip_image.compare( 0, 9, "Not Found", 9 ) == 0 ) ||
        ( m_zip_image.compare( 0, 14, "404: Not Found", 14 ) == 0 ) )
    {
        UTF8 fmt( _( "Cannot download library \"%s\".\nThe library does not exist on the server" ) );
        std::string msg = StrPrintf( fmt.c_str(), TO_UTF8( aRepoURL ) );

        THROW_IO_ERROR( msg );
    }
}

#if 0 && defined(STANDALONE)

int main( int argc, char** argv )
{
    INIT_LOGGER( ".", "test.log" );

    GITHUB_PLUGIN gh;

    try
    {
        wxArrayString fps = gh.FootprintEnumerate(
                "https://github.com/liftoff-sr/pretty_footprints",
                NULL
                );

        for( int i=0; i<(int)fps.Count(); ++i )
        {
            printf("[%d]:%s\n", i, TO_UTF8( fps[i] ) );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        printf( "%s\n", TO_UTF8(ioe.What()) );
    }

    UNINIT_LOGGER();

    return 0;
}

#endif
