#ifndef KICAD_PLUGIN_H_
#define KICAD_PLUGIN_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <string>

typedef int     BIU;

class PCB_TARGET;
class MODULE;
class DRAWSEGMENT;
class NETINFO;
class TEXTE_PCB;
class TRACK;
class NETCLASS;
class ZONE_CONTAINER;
class DIMENSION;
class NETINFO_ITEM;
class TEXTE_MODULE;
class EDGE_MODULE;
class TRACK;
class SEGZONE;
class D_PAD;

/**
 * Class KICAD_PLUGIN
 * is a PLUGIN derivation which could possibly be put into a DLL/DSO.
 * It is not thread safe, but it is re-entrant multiple times in sequence.
 */
class KICAD_PLUGIN : public PLUGIN
{

public:

    //-----<PLUGIN>-------------------------------------------------------------

    const wxString& PluginName()
    {
        static const wxString name = wxT( "KiCad" );
        return name;
    }

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties = NULL );   // overload

    void Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties = NULL );          // overload

    //-----</PLUGIN>------------------------------------------------------------

protected:

    wxString        m_error;        ///< for throwing exceptions
    BOARD*          m_board;        ///< which BOARD, no ownership here

    LINE_READER*    m_reader;       ///< no ownership here.
    FILE*           m_fp;           ///< no ownership here.
    wxString        m_filename;     ///< for saves only, name is in m_reader for loads

    wxString        m_field;        ///< reused to stuff MODULE fields.

    /// initialize PLUGIN like a constructor would, and futz with fresh BOARD if needed.
    void    init( PROPERTIES* aProperties );

    double  biuToDisk;      ///< convert from BIUs to disk engineering units with this scale factor
    double  diskToBiu;      ///< convert from disk engineering units to BIUs with this scale factor

    /**
     * Function biuParse
     * parses an ASCII decimal floating point value and scales it into a BIU
     * according to the current value of diskToBui.
     *
     * @param aValue is the ASCII value in C locale form with possible leading whitespace
     *
     * @param nptrptr may be NULL, but if not, then it tells where to put a
     *  pointer to the next unconsumed input text. See "man strtod" for more information.
     *
     * @return BIU - the converted Board Internal Unit.
     */
    BIU biuParse( const char* aValue, const char** nptrptr = NULL );

    /**
     * Function degParse
     * parses an ASCII decimal floating point value which is certainy an angle.  This
     * is a dedicated function for encapsulating support for the migration from
     * tenths of degrees to degrees in floating point.
     *
     * @param aValue is the ASCII value in C locale form with possible leading whitespace
     *
     * @param nptrptr may be NULL, but if not, then it tells where to put a
     *  pointer to the next unconsumed input text. See "man strtod" for more information.
     *
     * @return double - the string converted to a primitive double type
     */
    double degParse( const char* aValue, const char** nptrptr = NULL );

    //-----<load/parse functions>-----------------------------------------------

    void checkVersion();

    void loadAllSections( bool doAppend );

    void loadGENERAL();
    void loadSETUP();
    void loadSHEET();

    void loadMODULE();
    void load3D( MODULE* aModule );
    void loadPAD( MODULE* aModule );
    void loadMODULE_TEXT( TEXTE_MODULE* aText );
    void loadMODULE_EDGE( MODULE* aModule );

    void loadPCB_LINE();
    void loadNETINFO_ITEM();
    void loadPCB_TEXT();
    void loadNETCLASS();

    /**
     * Function loadTrackList
     * reads a list of segments (Tracks and Vias, or Segzones)
     *
     * @param aInsertBeforeMe may be either NULL indicating append, or it may
     *  be an insertion point before which all the segments are inserted.
     *
     * @param aStructType is either PCB_TRACE_T to indicate tracks and vias, or
     *        PCB_ZONE_T to indicate oldschool zone segments (before polygons came to be).
     */
    void loadTrackList( TRACK* aInsertBeforeMe, int aStructType );

    void loadZONE_CONTAINER();      // "$CZONE_OUTLINE"
    void loadDIMENSION();           // "$COTATION"
    void loadPCB_TARGET();          // "$PCB_TARGET"

    //-----</ load/parse functions>---------------------------------------------


    //-----<save functions>-----------------------------------------------------

    /**
     * Function writeError
     * returns an error message wxString containing the filename being
     * currently written.
     */
    wxString writeError() const;

    int biuSprintf( char* buf, BIU aValue ) const;

    /// convert a BIU to engineering units by scaling and formatting to ASCII.
    std::string fmtBIU( BIU aValue ) const;

    std::string fmtBIUPair( BIU first, BIU second ) const;

    std::string fmtBIUPoint( const wxPoint& aPoint ) const
    {
        return fmtBIUPair( aPoint.x, aPoint.y );
    }

    std::string fmtBIUSize( const wxSize& aSize ) const
    {
        // unfortunately there is inconsistency in the order of saving wxSize,
        // so sometimes we use fmtBIUPair() directly in the saveXXX() functions.
        return fmtBIUPair( aSize.x, aSize.y );
    }

    void saveAllSections() const;
    void saveGENERAL() const;
    void saveSHEET() const;
    void saveSETUP() const;
    void saveBOARD() const;

    void saveMODULE( const MODULE* aModule ) const;
    void saveMODULE_TEXT( const TEXTE_MODULE* aText ) const;
    void saveMODULE_EDGE( const EDGE_MODULE* aGraphic ) const;
    void savePAD( const D_PAD* aPad ) const;
    void save3D( const MODULE* aModule ) const;

    void saveNETINFO_ITEM( const NETINFO_ITEM* aNet ) const;
    void saveNETCLASSES() const;
    void saveNETCLASS( const NETCLASS* aNetclass ) const;

    void savePCB_TEXT( const TEXTE_PCB* aText ) const;
    void savePCB_TARGET( const PCB_TARGET* aTarget ) const;
    void savePCB_LINE( const DRAWSEGMENT* aStroke ) const;
    void saveDIMENTION( const DIMENSION* aDimension ) const;
    void saveTRACK( const TRACK* aTrack ) const;

    /**
     * Function saveZONE_CONTAINER
     * saves the new polygon zones.
     */
    void saveZONE_CONTAINER( const ZONE_CONTAINER* aZone ) const;

    //-----</save functions>----------------------------------------------------

};

#endif  // KICAD_PLUGIN_H_
