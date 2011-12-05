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

    wxString        m_field;        ///< reused to stuff MODULE fields.

    /// initialize PLUGIN like a constructor would, and futz with fresh BOARD if needed.
    void    init( PROPERTIES* aProperties );

    int     NbDraw;
    int     NbTrack;
    int     NbZone;
    int     NbMod;
    int     NbNets;

    double  biuToDisk;      ///< convert from BIUs to disk engineering units with this scale factor
    double  diskToBiu;      ///< convert from disk engineering units to BIUs with this scale factor

    /// convert a BIU to engineering units by scaling and formatting to ASCII.
    std::string biuFmt( BIU aValue );

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
    void loadTEXTE_MODULE( TEXTE_MODULE* aText );
    void loadEDGE_MODULE( MODULE* aModule );

    void loadDRAWSEGMENT();
    void loadNETINFO_ITEM();
    void loadPCB_TEXTE();
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
};

#endif  // KICAD_PLUGIN_H_
