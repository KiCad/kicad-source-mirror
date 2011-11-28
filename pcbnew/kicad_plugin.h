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
typedef double  BFU;

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


class KICAD_PLUGIN : public PLUGIN
{

public:

    //-----<PLUGIN>---------------------------------------------------------------------

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties = NULL );

    void Save( const wxString* aFileName, BOARD* aBoard, PROPERTIES* aProperties = NULL );

    const wxString& Name()
    {
        static const wxString name = wxT( "KiCad" );
        return name;
    }

    //-----</PLUGIN>--------------------------------------------------------------------

protected:

    wxString        m_Error;        ///< for throwing exceptions

    LINE_READER*    m_Reader;       ///< no ownership here.

    /// initialize PLUGIN like a constructor would, and futz with fresh BOARD if needed.
    void    init( BOARD* board, PROPERTIES* aProperties );

    int     NbDraw;
    int     NbTrack;
    int     NbZone;
    int     NbMod;
    int     NbNets;

    BFU     biuToDisk;      ///< convert from BIUs to disk engineering units with this scale factor
    BFU     diskToBiu;      ///< convert from disk engineering units to BIUs with this scale factor

    /// convert a BIU to engineering units by scaling and formatting to ASCII.
    std::string biuFmt( BIU aValue );

    // load / parse functions

    void loadGeneral( BOARD* me );
    void loadSetup( BOARD* me );
    void loadSheet( BOARD* me );

    void load( PCB_TARGET* me );
    void load( MODULE* me );
    void load( DRAWSEGMENT* me );
    void load( NETINFO* me );
    void load( TEXTE_PCB* me );
    void load( TRACK* me );
    void load( NETCLASS* me );
    void load( ZONE_CONTAINER* me );
    void load( DIMENSION* me );
    void load( NETINFO_ITEM* me );
//    void load( SEGZONE* me );

};

#endif  // KICAD_PLUGIN_H_
