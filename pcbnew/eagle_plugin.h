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
#include <layers_id_colors_and_visibility.h>


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

typedef std::map< std::string, ENET >   NET_MAP;
typedef NET_MAP::const_iterator         NET_MAP_CITER;

typedef boost::property_tree::ptree     PTREE;
typedef const PTREE                     CPTREE;

struct EELEMENT;
class XPATH;
struct ERULES;
struct EATTR;
class TEXTE_MODULE;


/**
 * Class EAGLE_PLUGIN
 * works with Eagle 6.x XML board files and footprints to implement the
 * Pcbnew PLUGIN API, or a portion of it.
 */
class EAGLE_PLUGIN : public PLUGIN
{
public:

    //-----<PUBLIC PLUGIN API>--------------------------------------------------
    const wxString PluginName() const;

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,  const PROPERTIES* aProperties = NULL );

    const wxString GetFileExtension() const;

    wxArrayString FootprintEnumerate( const wxString& aLibraryPath, const PROPERTIES* aProperties = NULL);

    MODULE* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, const PROPERTIES* aProperties = NULL );

    bool IsFootprintLibWritable( const wxString& aLibraryPath )
    {
        return false;   // until someone writes others like FootprintSave(), etc.
    }

    void FootprintLibOptions( PROPERTIES* aProperties ) const;

/*
    void Save( const wxString& aFileName, BOARD* aBoard, const PROPERTIES* aProperties = NULL );

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, const PROPERTIES* aProperties = NULL );

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName, const PROPERTIES* aProperties = NULL );

    void FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties = NULL );

    bool FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties = NULL );
*/

    //-----</PUBLIC PLUGIN API>-------------------------------------------------

    typedef int BIU;

    EAGLE_PLUGIN();
    ~EAGLE_PLUGIN();

private:

    int         m_cu_map[17];       ///< map eagle to kicad, cu layers only.

    ERULES*     m_rules;            ///< Eagle design rules.
    XPATH*      m_xpath;            ///< keeps track of what we are working on within
                                    ///< XML document during a Load().

    int         m_hole_count;       ///< generates unique module names from eagle "hole"s.

    NET_MAP     m_pads_to_nets;     ///< net list

    MODULE_MAP  m_templates;        ///< is part of a MODULE factory that operates
                                    ///< using copy construction.
                                    ///< lookup key is either libname.packagename or simply
                                    ///< packagename if FootprintLoad() or FootprintEnumberate()

    const PROPERTIES* m_props;            ///< passed via Save() or Load(), no ownership, may be NULL.
    BOARD*      m_board;            ///< which BOARD is being worked on, no ownership here

    int         m_min_trace;        ///< smallest trace we find on Load(), in BIU.
    int         m_min_via;          ///< smallest via we find on Load(), in BIU.
    int         m_min_via_hole;     ///< smallest via diameter hole we find on Load(), in BIU.

    double      mm_per_biu;         ///< how many mm in each BIU
    double      biu_per_mm;         ///< how many bius in a mm

    wxString    m_lib_path;
    wxDateTime  m_mod_time;

    /// initialize PLUGIN like a constructor would, and futz with fresh BOARD if needed.
    void    init( const PROPERTIES* aProperties );

    void    clear_cu_map();

    /// Convert an Eagle distance to a KiCad distance.
    int     kicad( double d ) const;
    int     kicad_y( double y ) const       { return -kicad( y ); }
    int     kicad_x( double x ) const       { return kicad( x ); }

    /// create a font size (fontz) from an eagle font size scalar
    wxSize  kicad_fontz( double d ) const;

    /// Convert an Eagle layer to a KiCad layer.
    LAYER_ID kicad_layer( int aLayer ) const;

    /// Convert a KiCad distance to an Eagle distance.
    double  eagle( BIU d ) const            { return mm_per_biu * d; }
    double  eagle_x( BIU x ) const          { return eagle( x ); }
    double  eagle_y( BIU y ) const          { return eagle( y ); }

    /// This PLUGIN only caches one footprint library, this determines which one.
    void    cacheLib( const wxString& aLibraryPath );

    /// get a file's  or dir's modification time.
    static wxDateTime getModificationTime( const wxString& aPath );

    // all these loadXXX() throw IO_ERROR or ptree_error exceptions:

    void loadAllSections( CPTREE& aDocument );
    void loadDesignRules( CPTREE& aDesignRules );
    void loadLayerDefs( CPTREE& aLayers );
    void loadPlain( CPTREE& aPlain );
    void loadSignals( CPTREE& aSignals );

    /**
     * Function loadLibrary
     * loads the Eagle "library" XML element, which can occur either under
     * a "libraries" element (if a *.brd file) or under a "drawing" element if a
     * *.lbr file.
     * @param aLib is the portion of the loaded XML document tree that is the "library"
     *   element.
     * @param aLibName is a pointer to the library name or NULL.  If NULL this means
     *   we are loading a *.lbr not a *.brd file and the key used in m_templates is to exclude
     *   the library name.
     */
    void loadLibrary( CPTREE& aLib, const std::string* aLibName );

    void loadLibraries( CPTREE& aLibs );
    void loadElements( CPTREE& aElements );

    void orientModuleAndText( MODULE* m, const EELEMENT& e, const EATTR* nameAttr, const EATTR* valueAttr );
    void orientModuleText( MODULE* m, const EELEMENT& e, TEXTE_MODULE* txt, const EATTR* a );


    /// move the BOARD into the center of the page
    void centerBoard();

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
