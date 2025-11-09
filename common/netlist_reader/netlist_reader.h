/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <macros.h>
#include <lib_id.h>


class LINE_READER;


/**
 * Read a component footprint link file (*.cmp) format.
 */
class CMP_READER
{
public:
    /**
     * @param aLineReader is a LINE_READER (in fact a FILE_LINE_READER)
     * which is owned by me ( and deleted by me) to read
     * the component footprint link file.
     */
    CMP_READER( LINE_READER* aLineReader )
    {
        m_lineReader = aLineReader;
    }

    ~CMP_READER();

    /**
     * Read the *.cmp file format contains the component footprint assignments created by CvPcb
     * into \a aNetlist.
     *
     * Sample file footprint assignment entry:
     *
     * Cmp-Mod V01 Genere by CvPcb 29/10/2003-13: 11:6 *
     *  BeginCmp
     *  TimeStamp = /32307DE2/AA450F67;
     *  Reference = C1;
     *  ValeurCmp = 47uF;
     *  IdModule  = CP6;
     *  EndCmp
     *
     * @todo At some point in the future, use the footprint field in the new s-expression
     *       netlist file to assign a footprint to a component instead of using a secondary
     *       (*.cmp) file.
     *
     * @param aNetlist is the #NETLIST to read into.
     * @throw IO_ERROR if a the #LINE_READER IO error occurs.
     * @throw PARSE_ERROR if an error occurs while parsing the file.
     * @return true if OK, false if a component reference found in the
     * .cmp file is not found in netlist, which means the .cmp file
     * is not updated. This is an usual case, in CvPcb, but can be used to
     * print a warning in Pcbnew.
     */
    bool Load( NETLIST* aNetlist );

private:
    LINE_READER* m_lineReader;            ///< The line reader to read.
};


/**
 * Base class to derive netlist readers from.
 *
 * This base class can be used without PCB-specific dependencies.
 * For PCB-specific functionality (like handling FOOTPRINT objects),
 * use the derived class in pcbnew.
 */
class NETLIST_READER
{
public:

    enum NETLIST_FILE_T
    {
        UNKNOWN = -1,
        ORCAD,
        LEGACY,
        KICAD,

        // Add new types here.  Don't forget to create the appropriate class derived from
        // NETCLASS_READER and add the entry to the NETLIST_READER::GetNetlistReader()
        // function.
    };

    /**
     * @param aLineReader ownership is taken of this #LINE_READER.
     * @param aNetlist the #NETLIST object to read into.
     * @param aFootprintLinkReader ownership is taken of this #CMP_READER.
     */
    NETLIST_READER( LINE_READER*  aLineReader,
                    NETLIST*      aNetlist,
                    CMP_READER*   aFootprintLinkReader = nullptr )
    {
        wxASSERT( aLineReader != nullptr );

        m_lineReader           = aLineReader;
        m_footprintReader      = aFootprintLinkReader;
        m_netlist              = aNetlist;
        m_loadFootprintFilters = true;
        m_loadNets             = true;
    }

    virtual ~NETLIST_READER();

    /**
     * Look at \a aFileHeaderLine to see if it matches any of the netlist file types it
     * knows about.
     *
     * @param aLineReader is the #LINE_READER object containing lines from the netlist to test.
     * @return the #NETLIST_FILE_T of \a aLineReader.
     */
    static NETLIST_FILE_T GuessNetlistFileType( LINE_READER* aLineReader );

    /**
     * Attempt to determine the net list file type of \a aNetlistFileName and return the
     * appropriate NETLIST_READER type.
     *
     * @param aNetlist is the netlist to load \a aNetlistFileName into.
     * @param aNetlistFileName is the full path and file name of the net list to read.
     * @param aCompFootprintFileName is the full path and file name of the component footprint
     *                               associations to read.  Set to wxEmptyString if loading the
     *                               footprint association file is not required.
     * @return the appropriate NETLIST_READER if \a aNetlistFileName is a valid netlist or
     *         NULL if \a aNetlistFileName is not a valid netlist files.
     */
    static NETLIST_READER* GetNetlistReader( NETLIST*        aNetlist,
                                             const wxString& aNetlistFileName,
                                             const wxString& aCompFootprintFileName = wxEmptyString );

    /**
     * Load the contents of the netlist file into \a aNetlist.
     *
     * @throw IO_ERROR if a file IO error occurs.
     * @throw PARSE_ERROR if an error occurs while parsing the file.
     */
    virtual void LoadNetlist() = 0;

protected:
    NETLIST*     m_netlist;               ///< The net list to read the file(s) into.
    bool         m_loadFootprintFilters;  ///< Load the component footprint filters section if true.
    bool         m_loadNets;              ///< Load the nets section of the netlist file if true.
    LINE_READER* m_lineReader;            ///< The line reader of the netlist.

    /// The reader used to load the footprint links.  If NULL, footprint links are not read.
    CMP_READER*  m_footprintReader;
};


/**
 * Read the KiCad legacy and the old Orcad netlist formats.
 *
 * The KiCad legacy netlist format was derived directly from an old Orcad netlist format.  The
 * primary difference is the header was changed so this reader can read both formats.
 */
class LEGACY_NETLIST_READER : public NETLIST_READER
{
    /**
     * Read the \a aLine containing the description of a component from a legacy format
     * netlist and add it to the netlist.
     *
     * Analyze the first line of a component description in netlist:
     * ( /40C08647 $noname R20 4.7K {Lib=R}
     *
     * @param  aText contains the first line of description.
     * @return the new component created by parsing \a aLine.
     * @throw PARSE_ERROR when \a aLine is not a valid component description.
     */
    COMPONENT* loadComponent( char* aText );

    /**
     * Load the footprint filter section of netlist file.
     *
     * Sample legacy footprint filter section:
     *  { Allowed footprints by component:
     *  $component R11
     *  R?
     *  SM0603
     *  SM0805
     *  R?-*
     *  SM1206
     *  $endlist
     *  $endfootprintlist
     *  }
     *
     * @throw IO_ERROR if a file IO error occurs.
     * @throw PARSE_ERROR if an error occurs while parsing the file.
     */
    void loadFootprintFilters();

    /**
     * Function loadNet
     * read a component net description from \a aText.
     *
     * @param aText is current line read from the netlist.
     * @param aComponent is the component to add the net to.
     * @throw PARSE_ERROR if a error occurs reading \a aText.
     */
    void loadNet( char* aText, COMPONENT* aComponent );

public:

    LEGACY_NETLIST_READER( LINE_READER*  aLineReader,
                           NETLIST*      aNetlist,
                           CMP_READER*   aFootprintLinkReader = nullptr ) :
        NETLIST_READER( aLineReader, aNetlist, aFootprintLinkReader )
    {
    }

    /**
     * Read the netlist file in the legacy format into \a aNetlist.
     *
     * The legacy netlist format is:
     * \# EESchema Netlist Version 1.0 generee le  18/5/2005-12:30:22
     *  (
     *  ( 40C08647 $noname R20 4,7K {Lib=R}
     *  (    1 VCC )
     *  (    2 MODB_1 )
     *  )
     *  ( 40C0863F $noname R18 4,7_k {Lib=R}
     *  (    1 VCC )
     *  (    2 MODA_1 )
     *  )
     *  }
     * \#End
     *
     * @throw IO_ERROR if a file IO error occurs.
     * @throw PARSE_ERROR if an error occurs while parsing the file.
     */
    virtual void LoadNetlist() override;
};


/**
 * Read the new s-expression based KiCad netlist format.
 */
class KICAD_NETLIST_READER : public NETLIST_READER
{
public:
    KICAD_NETLIST_READER( LINE_READER*  aLineReader,
                          NETLIST*      aNetlist,
                          CMP_READER*   aFootprintLinkReader = nullptr ) :
        NETLIST_READER( aLineReader, aNetlist, aFootprintLinkReader )
    { }

    virtual ~KICAD_NETLIST_READER()
    { }

    virtual void LoadNetlist() override;
};
