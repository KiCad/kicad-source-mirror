#ifndef EAGLE_PLUGIN_H_
#define EAGLE_PLUGIN_H_

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

#include <io_mgr.h>


// forward declaration on ptree template so we can confine use of big boost
// headers to only the implementation *.cpp file.

#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <map>


class MODULE;
typedef boost::ptr_map< std::string, MODULE >   MODULE_MAP;


struct ENET
{
    int         netcode;
    std::string netname;

    ENET( int aNetCode, const std::string& aNetName ) :
        netcode( aNetCode ),
        netname( aNetName )
    {}

    ENET() :
        netcode( 0 )
    {}
};

typedef std::map< std::string, ENET >           NET_MAP;
typedef NET_MAP::const_iterator                 NET_MAP_CITER;

/*
#include
namespace boost {
    namespace property_tree
    {
        template < class Key, class Data, class KeyCompare = std::less<Key> >
            class basic_ptree;

        typedef basic_ptree< std::string, std::string > ptree;
    }
}
*/

typedef boost::property_tree::ptree     PTREE;
typedef const PTREE                     CPTREE;

/**
 * Class EAGLE_PLUGIN
 * works with Eagle 6.x XML board files and footprints.
 */
class EAGLE_PLUGIN : public PLUGIN
{
public:

    //-----<PUBLIC PLUGIN API>--------------------------------------------------
    const wxString& PluginName() const;

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,  PROPERTIES* aProperties = NULL );

    const wxString& GetFileExtension() const;

/*
    void Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties = NULL );

    wxArrayString FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL);

    MODULE* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, PROPERTIES* aProperties = NULL );

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, PROPERTIES* aProperties = NULL );

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName );

    void FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL );

    void FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL );

    bool IsFootprintLibWritable( const wxString& aLibraryPath );
*/

    //-----</PUBLIC PLUGIN API>-------------------------------------------------

    typedef int     BIU;

    EAGLE_PLUGIN();
    ~EAGLE_PLUGIN();

private:

    NET_MAP         m_pads_to_nets;

    MODULE_MAP      m_templates;    ///< is part of a MODULE factory that operates
                                    ///< using copy construction.
                                    ///< lookup key is libname.packagename

    PROPERTIES*     m_props;        ///< passed via Save() or Load(), no ownership, may be NULL.

    BOARD*  m_board;                ///< which BOARD, no ownership here
    double  mm_per_biu;             ///< how many mm in each BIU
    double  biu_per_mm;             ///< how many bius in a mm

    /// initialize PLUGIN like a constructor would, and futz with fresh BOARD if needed.
    void    init( PROPERTIES* aProperties );

    int     kicad( double d ) const;
    int     kicad_y( double y ) const       { return -kicad( y ); }
    int     kicad_x( double x ) const       { return kicad( x ); }
    wxSize  kicad_fontz( double d ) const;


    static int kicad_layer( int aLayer );

    double  eagle( BIU d ) const            { return mm_per_biu * d; }
    double  eagle_x( BIU x ) const          { return eagle( x ); }
    double  eagle_y( BIU y ) const          { return eagle( y ); }


#if 0
    /// encapsulate the BIU formatting tricks in one place.
    int biuSprintf( char* buf, BIU aValue ) const;

    /**
     * Function fmtBIU
     * converts a BIU to engineering units by scaling and formatting to ASCII.
     * This function is the complement of biuParse().  One has to know what the
     * other is doing.
     */
    std::string fmtBIU( BIU aValue ) const;

    std::string fmtBIUPair( BIU first, BIU second ) const;

    std::string fmtBIUPoint( const wxPoint& aPoint ) const
    {
        return fmtBIUPair( aPoint.x, aPoint.y );
    }

    std::string fmtBIUSize( const wxSize& aSize ) const
    {
        return fmtBIUPair( aSize.x, aSize.y );
    }
#endif

    // all these loadXXX() throw IO_ERROR or ptree_error exceptions:

    void loadAllSections( CPTREE& aEagleBoard, const std::string& aXpath, bool aAppendToMe );

    void loadPlain( CPTREE& aPlain, const std::string& aXpath );

    void loadSignals( CPTREE& aSignals, const std::string& aXpath );

    void loadLibraries( CPTREE& aLibs, const std::string& aXpath );

    void loadElements( CPTREE& aElements, const std::string& aXpath );

    /**
     * Function fmtDEG
     * formats an angle in a way particular to a board file format.  This function
     * is the opposite or complement of degParse().  One has to know what the
     * other is doing.
     */
    std::string fmtDEG( double aAngle ) const;

    /**
     * Function makeModule
     * creates a MODULE from an Eagle package.
     */
    MODULE* makeModule( CPTREE& aPackage, const std::string& aPkgName ) const;

    void packageWire( MODULE* aModule, CPTREE& aTree ) const;
    void packagePad( MODULE* aModule, CPTREE& aTree ) const;
    void packageText( MODULE* aModule, CPTREE& aTree ) const;
    void packageRectangle( MODULE* aModule, CPTREE& aTree ) const;
    void packagePolygon( MODULE* aModule, CPTREE& aTree ) const;
    void packageCircle( MODULE* aModule, CPTREE& aTree ) const;
    void packageHole( MODULE* aModule, CPTREE& aTree ) const;
    void packageSMD( MODULE* aModule, CPTREE& aTree ) const;

};

#endif  // EAGLE_PLUGIN_H_
