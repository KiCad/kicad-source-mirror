
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


/*

Pcbnew PLUGIN for Eagle 6.x XML *.brd and footprint format.

XML parsing and converting:
Getting line numbers and byte offsets from the source XML file is not
possible using currently available XML libraries within KiCad project:
wxXmlDocument and boost::property_tree.

property_tree will give line numbers but no byte offsets, and only during
document loading. This means that if we have a problem after the document is
successfully loaded, there is no way to correlate back to line number and byte
offset of the problem. So a different approach is taken, one which relies on the
XML elements themselves using an XPATH type of reporting mechanism. The path to
the problem is reported in the error messages. This means keeping track of that
path as we traverse the XML document for the sole purpose of accurate error
reporting.

User can load the source XML file into firefox or other xml browser and follow
our error message.

*/

#include <errno.h>

#include <wx/string.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <eagle_plugin.h>

#include <common.h>
#include <macros.h>
#include <fctsys.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>


using namespace boost::property_tree;

typedef EAGLE_PLUGIN::BIU                   BIU;
typedef PTREE::const_assoc_iterator         CA_ITER;
typedef PTREE::const_iterator               CITER;

typedef MODULE_MAP::iterator                MODULE_ITER;
typedef MODULE_MAP::const_iterator          MODULE_CITER;


static inline std::string makePkgKey( const std::string& aLibName, const std::string& aPkgName )
{
    // The MODULE factory key is a simple concatonation of library name and
    // package name, using '\x02' as a separator.
    std::string key = aLibName + '\x02' +  aPkgName;

    return key;
}


EAGLE_PLUGIN::EAGLE_PLUGIN()
{
    init( NULL );
}


EAGLE_PLUGIN::~EAGLE_PLUGIN()
{
}


const wxString& EAGLE_PLUGIN::PluginName() const
{
    static const wxString name = wxT( "Eagle" );
    return name;
}


const wxString& EAGLE_PLUGIN::GetFileExtension() const
{
    static const wxString extension = wxT( "brd" );
    return extension;
}


int inline EAGLE_PLUGIN::kicad( double d ) const
{
    return KiROUND( biu_per_mm * d );
}


BOARD* EAGLE_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,  PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.
    PTREE       doc;

    init( aProperties );

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // delete on exception, iff I own m_board, according to aAppendToMe
    auto_ptr<BOARD> deleter( aAppendToMe ? NULL : m_board );

    try
    {
        // 8 bit filename should be encoded in current locale, not necessarily utf8.
        std::string filename = (const char*) aFileName.fn_str();

        read_xml( filename, doc, xml_parser::trim_whitespace | xml_parser::no_comments );

        std::string xpath = "eagle.drawing.board";
        CPTREE&     brd   = doc.get_child( xpath );

        loadAllSections( brd, xpath, bool( aAppendToMe ) );
    }

    // Class ptree_error is a base class for xml_parser_error & file_parser_error,
    // so one catch should be OK for all errors.
    catch( ptree_error pte )
    {
        // for xml_parser_error, what() has the line number in it,
        // but no byte offset.  That should be an adequate error message.
        THROW_IO_ERROR( pte.what() );
    }

    // IO_ERROR exceptions are left uncaught, they pass upwards from here.

    deleter.release();
    return m_board;
}


void EAGLE_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
}


void EAGLE_PLUGIN::init( PROPERTIES* aProperties )
{
    m_modules.clear();

    m_board = NULL;
    m_props = aProperties;

    mm_per_biu = 1/IU_PER_MM;
    biu_per_mm = IU_PER_MM;
}

/*
int EAGLE_PLUGIN::biuSprintf( char* buf, BIU aValue ) const
{
    double  engUnits = mm_per_biu * aValue;
    int     len;

    if( engUnits != 0.0 && fabs( engUnits ) <= 0.0001 )
    {
        // printf( "f: " );
        len = sprintf( buf, "%.10f", engUnits );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        ++len;
    }
    else
    {
        // printf( "g: " );
        len = sprintf( buf, "%.10g", engUnits );
    }
    return len;
}


std::string EAGLE_PLUGIN::fmtBIU( BIU aValue ) const
{
    char    temp[50];

    int len = biuSprintf( temp, aValue );

    return std::string( temp, len );
}


double EAGLE_PLUGIN::dblParse( const char* aValue, const std::string& aXpath )
{
    char*   nptr;

    errno = 0;

    double fval = strtod( aValue, &nptr );

    if( errno )
    {
        wxString emsg = wxString::Format(
            _( "invalid float number:'%s' in XML document at '%s'" ),
            aValue, aXpath.c_str() );

        THROW_IO_ERROR( emsg );
    }

    if( aValue == nptr )
    {
        wxString emsg = wxString::Format(
            _( "missing float number:'%s' in XML document at '%s'" ),
            aValue, aXpath.c_str() );

        THROW_IO_ERROR( emsg );
    }

    return fval;
}


BIU EAGLE_PLUGIN::biuParse( const char* aValue, const std::string& aXpath )
{
    double mm = dblParse( aValue, aXpath );
    return BIU( KiROUND( mm * biu_per_mm ) );
}

*/

void EAGLE_PLUGIN::loadAllSections( CPTREE& aEagleBoard, const std::string& aXpath, bool aAppendToMe )
{
    std::string xpath;

    {
        xpath = aXpath + '.' + "libraries";
        CPTREE&  libs = aEagleBoard.get_child( "libraries" );
        loadModules( libs, xpath );
    }

    // assume that MODULE_MAP is already loaded.
    {
        xpath = aXpath + '.' + "signals";
        CPTREE&  signals = aEagleBoard.get_child( "signals" );
        loadNetsAndTracks( signals, xpath );
    }


    ;
}


void EAGLE_PLUGIN::loadModules( CPTREE& aLibs, const std::string& aXpath )
{
    m_modules.clear();

    for( CITER library = aLibs.begin();  library != aLibs.end();  ++library )
    {
        const std::string& lib_name = library->second.get<std::string>( "<xmlattr>.name" );

        // library will have <xmlattr> node, skip that and get the packages node
        CPTREE& packages = library->second.get_child( "packages" );

        // Create a MODULE for all the eagle packages, for use later via a copy constructor
        // to instantiate needed MODULES in our BOARD.  Save the MODULE templates in
        // a MODULE_MAP using a single lookup key consisting of libname+pkgname.

        for( CITER package = packages.begin();  package != packages.end();  ++package )
        {
            const std::string& pack_name = package->second.get<std::string>( "<xmlattr>.name" );

            std::string key = makePkgKey( lib_name, pack_name );

            MODULE* m = makeModule( package->second, pack_name );

            // add the templating MODULE to the MODULE template factory "m_modules"
            std::pair<MODULE_ITER, bool> r = m_modules.insert( key, m );

            if( !r.second )
            {
                wxString lib = FROM_UTF8( lib_name.c_str() );
                wxString pkg = FROM_UTF8( pack_name.c_str() );

                wxString emsg = wxString::Format(
                    _( "<package> name:'%s' duplicated in eagle <library>:'%s'" ),
                    GetChars( lib ),
                    GetChars( pkg )
                    );
                THROW_IO_ERROR( emsg );
            }
        }
    }
}


EWIRE EAGLE_PLUGIN::wire( CPTREE aWire ) const
{
    EWIRE w;

    CPTREE& attribs = aWire.get_child( "<xmlattr>" );

    w.x1    = attribs.get<double>( "x1" );
    w.y1    = attribs.get<double>( "y1" );
    w.x2    = attribs.get<double>( "x2" );
    w.y2    = attribs.get<double>( "y2" );
    w.width = attribs.get<double>( "width" );
    w.layer = attribs.get<int>( "layer" );

    return w;
}


MODULE* EAGLE_PLUGIN::makeModule( CPTREE& aPackage, const std::string& aPkgName ) const
{
    std::auto_ptr<MODULE>   m( new MODULE( NULL ) );

    m->SetLibRef( FROM_UTF8( aPkgName.c_str() ) );

    boost::optional<std::string> description = aPackage.get_optional<std::string>( "description" );
    if( description )
        m->SetDescription( FROM_UTF8( description->c_str() ) );

    for( CITER it = aPackage.begin();  it != aPackage.end();  ++it )
    {
        CPTREE& t = it->second;

        if( !it->first.compare( "wire " ) )
            packageWire( m.get(), t );

        else if( !it->first.compare( "pad" ) )
            packagePad( m.get(), t );

        else if( !it->first.compare( "text" ) )
            packageText( m.get(), t );

        else if( !it->first.compare( "rectangle" ) )
            packageRectangle( m.get(), t );

        else if( !it->first.compare( "polygon" ) )
            packagePolygon( m.get(), t );

        else if( !it->first.compare( "circle" ) )
            packageCircle( m.get(), t );

        else if( !it->first.compare( "hole" ) )
            packageHole( m.get(), t );

        else if( !it->first.compare( "smd" ) )
            packageSMD( m.get(), t );
    }

    return m.release();
}


void EAGLE_PLUGIN::packageWire( MODULE* aModule, CPTREE aTree ) const
{
    EWIRE   w = wire( aTree );

    int     layer = kicad_layer( w.layer );
    wxPoint start( kicad_x( w.x1 ), kicad_y( w.y1 ) );
    wxPoint end(   kicad_x( w.x2 ), kicad_y( w.x2 ) );
    int     width = kicad( w.width );

    EDGE_MODULE* dwg = new EDGE_MODULE( aModule, S_SEGMENT );
    aModule->m_Drawings.PushBack( dwg );

    dwg->SetStart0( start );
    dwg->SetEnd0( end );
    dwg->SetLayer( layer );
    dwg->SetWidth( width );
}


void EAGLE_PLUGIN::packagePad( MODULE* aModule, CPTREE aTree ) const
{
    CPTREE  attrs = aTree.get_child( "<xmlattr>" );
}


void EAGLE_PLUGIN::packageText( MODULE* aModule, CPTREE aTree ) const
{
    CPTREE  attrs = aTree.get_child( "<xmlattr>" );
}


void EAGLE_PLUGIN::packageRectangle( MODULE* aModule, CPTREE aTree ) const
{
    CPTREE  attrs = aTree.get_child( "<xmlattr>" );
}


void EAGLE_PLUGIN::packagePolygon( MODULE* aModule, CPTREE aTree ) const
{
    CPTREE  attrs = aTree.get_child( "<xmlattr>" );
}


void EAGLE_PLUGIN::packageCircle( MODULE* aModule, CPTREE aTree ) const
{
    CPTREE  attrs = aTree.get_child( "<xmlattr>" );
}


void EAGLE_PLUGIN::packageHole( MODULE* aModule, CPTREE aTree ) const
{
    CPTREE  attrs = aTree.get_child( "<xmlattr>" );
}


void EAGLE_PLUGIN::packageSMD( MODULE* aModule, CPTREE aTree ) const
{
    CPTREE  attrs = aTree.get_child( "<xmlattr>" );
}


/*
             <package name="2X03">
              <description>&lt;b&gt;PIN HEADER&lt;/b&gt;</description>
              <wire x1="-3.81" y1="-1.905" x2="-3.175" y2="-2.54" width="0.1524" layer="21"/>
              <wire x1="-1.905" y1="-2.54" x2="-1.27" y2="-1.905" width="0.1524" layer="21"/>
              <wire x1="-1.27" y1="-1.905" x2="-0.635" y2="-2.54" width="0.1524" layer="21"/>
              <wire x1="0.635" y1="-2.54" x2="1.27" y2="-1.905" width="0.1524" layer="21"/>
              <wire x1="-3.81" y1="-1.905" x2="-3.81" y2="1.905" width="0.1524" layer="21"/>
              <wire x1="-3.81" y1="1.905" x2="-3.175" y2="2.54" width="0.1524" layer="21"/>
              <wire x1="-3.175" y1="2.54" x2="-1.905" y2="2.54" width="0.1524" layer="21"/>
              <wire x1="-1.905" y1="2.54" x2="-1.27" y2="1.905" width="0.1524" layer="21"/>
              <wire x1="-1.27" y1="1.905" x2="-0.635" y2="2.54" width="0.1524" layer="21"/>
              <wire x1="-0.635" y1="2.54" x2="0.635" y2="2.54" width="0.1524" layer="21"/>
              <wire x1="0.635" y1="2.54" x2="1.27" y2="1.905" width="0.1524" layer="21"/>
              <wire x1="-1.27" y1="1.905" x2="-1.27" y2="-1.905" width="0.1524" layer="21"/>
              <wire x1="1.27" y1="1.905" x2="1.27" y2="-1.905" width="0.1524" layer="21"/>
              <wire x1="-0.635" y1="-2.54" x2="0.635" y2="-2.54" width="0.1524" layer="21"/>
              <wire x1="-3.175" y1="-2.54" x2="-1.905" y2="-2.54" width="0.1524" layer="21"/>
              <wire x1="1.27" y1="-1.905" x2="1.905" y2="-2.54" width="0.1524" layer="21"/>
              <wire x1="3.175" y1="-2.54" x2="3.81" y2="-1.905" width="0.1524" layer="21"/>
              <wire x1="1.27" y1="1.905" x2="1.905" y2="2.54" width="0.1524" layer="21"/>
              <wire x1="1.905" y1="2.54" x2="3.175" y2="2.54" width="0.1524" layer="21"/>
              <wire x1="3.175" y1="2.54" x2="3.81" y2="1.905" width="0.1524" layer="21"/>
              <wire x1="3.81" y1="1.905" x2="3.81" y2="-1.905" width="0.1524" layer="21"/>
              <wire x1="1.905" y1="-2.54" x2="3.175" y2="-2.54" width="0.1524" layer="21"/>
              <pad name="1" x="-2.54" y="-1.27" drill="1.016" shape="octagon"/>
              <pad name="2" x="-2.54" y="1.27" drill="1.016" shape="octagon"/>
              <pad name="3" x="0" y="-1.27" drill="1.016" shape="octagon"/>
              <pad name="4" x="0" y="1.27" drill="1.016" shape="octagon"/>
              <pad name="5" x="2.54" y="-1.27" drill="1.016" shape="octagon"/>
              <pad name="6" x="2.54" y="1.27" drill="1.016" shape="octagon"/>
              <text x="-3.81" y="3.175" size="1.27" layer="25" ratio="10">&gt;NAME</text>
              <text x="-3.81" y="-4.445" size="1.27" layer="27">&gt;VALUE</text>
              <rectangle x1="-2.794" y1="-1.524" x2="-2.286" y2="-1.016" layer="51"/>
              <rectangle x1="-2.794" y1="1.016" x2="-2.286" y2="1.524" layer="51"/>
              <rectangle x1="-0.254" y1="1.016" x2="0.254" y2="1.524" layer="51"/>
              <rectangle x1="-0.254" y1="-1.524" x2="0.254" y2="-1.016" layer="51"/>
              <rectangle x1="2.286" y1="1.016" x2="2.794" y2="1.524" layer="51"/>
              <rectangle x1="2.286" y1="-1.524" x2="2.794" y2="-1.016" layer="51"/>
            </package>
*/


void EAGLE_PLUGIN::loadNetsAndTracks( CPTREE& aSignals, const std::string& aXpath )
{
    int netCode = 1;

    time_t  now = time( NULL );

    for( CITER nit = aSignals.begin();  nit != aSignals.end();  ++nit, ++netCode )
    {
        wxString netName = FROM_UTF8( nit->second.get<std::string>( "<xmlattr>.name" ).c_str() );

        m_board->AppendNet( new NETINFO_ITEM( m_board, netName, netCode ) );

        std::pair<CA_ITER, CA_ITER> wires = nit->second.equal_range( "wire" );

        for( CA_ITER wit = wires.first;  wit != wires.second;  ++wit )
        {
            EWIRE w = wire( wit->second );

            TRACK* t = new TRACK( m_board );

            t->SetTimeStamp( now );

            t->SetPosition( wxPoint( kicad_x( w.x1 ), kicad_y( w.y1 ) ) );
            t->SetEnd( wxPoint( kicad_x( w.x2 ), kicad_y( w.y2 ) ) );

            t->SetWidth( kicad( w.width ) );
            t->SetLayer( kicad_layer( w.layer ) );
            t->SetNet( netCode );

            m_board->m_Track.Insert( t, NULL );

            D(printf("wire:%s\n", wit->first.c_str() );)
        }
    }
}


int EAGLE_PLUGIN::kicad_layer( int aLayer ) const
{
    return aLayer;
}


wxArrayString EAGLE_PLUGIN::FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    return wxArrayString();
}


MODULE* EAGLE_PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, PROPERTIES* aProperties )
{
    return NULL;
}


void EAGLE_PLUGIN::FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, PROPERTIES* aProperties )
{
}


void EAGLE_PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName )
{
}


void EAGLE_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
}


void EAGLE_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
}


bool EAGLE_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    return true;
}

