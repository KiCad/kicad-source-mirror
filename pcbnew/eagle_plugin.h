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

class MODULE;
typedef boost::ptr_map< std::string, MODULE >   MODULE_MAP;


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

struct EWIRE;
struct EROT;
struct EATTR;
struct ECIRCLE;


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

    void Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties = NULL );

    wxArrayString FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL);

    MODULE* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, PROPERTIES* aProperties = NULL );

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, PROPERTIES* aProperties = NULL );

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName );

    void FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL );

    void FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL );

    bool IsFootprintLibWritable( const wxString& aLibraryPath );

    const wxString& GetFileExtension() const;

    //-----</PUBLIC PLUGIN API>-------------------------------------------------

    typedef int     BIU;

    EAGLE_PLUGIN();
    ~EAGLE_PLUGIN();

private:

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

    static int kicad_layer( int aLayer );

    double  eagle( BIU d ) const            { return mm_per_biu * d; }
    double  eagle_x( BIU x ) const          { return eagle( x ); }
    double  eagle_y( BIU y ) const          { return eagle( y ); }


#if 0
    /**
     * Function dblParse
     * parses an ASCII decimal floating point value and reports any error by throwing
     * an exception using the xpath in the error message text.
     *
     * @param aValue is the ASCII value in C locale form with possible leading whitespace
     *
     * @param aXpath tells where the value is within the XML document.
     *
     * @return double - aValue in binary, not scaled.
     */
    static double dblParse( const char* aValue, const std::string& aXpath );

    /**
     * Function biuParse
     * parses an ASCII decimal floating point value and scales it into a BIU
     * according to the current mm_per_biu.  This fuction is the complement of
     * fmtBIU().  One has to know what the other is doing.
     *
     * @param aValue is the ASCII value in C locale form with possible leading whitespace
     *
     * @param aXpath tells where the value is within the XML document.
     *
     * @return BIU - the converted Board Internal Unit.
     */
    BIU biuParse( const char* aValue, const std::string& aXpath );

    /**
     * Function degParse
     * parses an ASCII decimal floating point value which is certainly an angle.  This
     * is a dedicated function for encapsulating support for the migration from
     * tenths of degrees to degrees in floating point.  This function is the complement of
     * fmtDEG().  One has to know what the other is doing.
     *
     * @param aValue is the ASCII value in C locale form with possible leading whitespace
     *
     * @param nptrptr may be NULL, but if not, then it tells where to put a
     *  pointer to the next unconsumed input text. See "man strtod" for more information.
     *
     * @return double - the string converted to a primitive double type
     */
    double degParse( const char* aValue, const char** nptrptr = NULL );

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

    void loadNetsAndTracks( CPTREE& aSignals, const std::string& aXpath );

    void loadLibraries( CPTREE& aLibs, const std::string& aXpath );

    void loadElements( CPTREE& aElements, const std::string& aXpath );

    /**
     * Function ewire
     * converts a <wire>'s xml attributes to binary without additional conversion.
     * @param aResult is an EWIRE to fill in with the <wire> data converted to binary.
     */
    EWIRE   ewire( CPTREE& aWire ) const;

    ECIRCLE ecircle( CPTREE& aCircle ) const;

    EROT    erot( const std::string& aRot ) const;

    /**
     * Function eattr
     * parses an Eagle "attribute" element.  Note that an attribute element
     * is different than an XML element attribute.  The attribute element is a
     * full XML node in and of itself, and has attributes of its own.  Blame Eagle.
     */
    EATTR eattr( CPTREE& aAttribute ) const;

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

    void packageWire( MODULE* aModule, CPTREE aTree ) const;
    void packagePad( MODULE* aModule, CPTREE aTree ) const;
    void packageText( MODULE* aModule, CPTREE aTree ) const;
    void packageRectangle( MODULE* aModule, CPTREE aTree ) const;
    void packagePolygon( MODULE* aModule, CPTREE aTree ) const;
    void packageCircle( MODULE* aModule, CPTREE aTree ) const;
    void packageHole( MODULE* aModule, CPTREE aTree ) const;
    void packageSMD( MODULE* aModule, CPTREE aTree ) const;

};

#endif  // EAGLE_PLUGIN_H_
