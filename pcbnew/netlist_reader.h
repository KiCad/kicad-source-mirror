#ifndef NETLIST_READER_H
#define NETLIST_READER_H

/**
 * @file netlist_reader.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras.
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>.
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <boost/ptr_container/ptr_vector.hpp>

#include <fctsys.h>
#include <macros.h>
#include <fpid.h>

#include <netlist_lexer.h>    // netlist_lexer is common to Eeschema and Pcbnew


class NETLIST;
class COMPONENT;


/**
 * Class CMP_READER
 * reads a component footprint link file (*.cmp) format.
 */
class CMP_READER
{
    LINE_READER* m_lineReader;            ///< The line reader to read.

public:
    /**
     * CMP_READER constructor.
     * @param aLineReader is a LINE_READER (in fact a FILE_LINE_READER)
     * which is owned by me ( and deleted by me) to read
     * the component footprint link file.
     */
    CMP_READER( LINE_READER* aLineReader )
    {
        m_lineReader = aLineReader;
    }

    ~CMP_READER()
    {
        if( m_lineReader )
        {
            delete m_lineReader;
            m_lineReader = NULL;
        }
    }

    /**
     * Function Load
     * read the *.cmp file format contains the component footprint assignments created by CvPcb
     * into \a aNetlist.
     *
     * @param aNetlist is the #NETLIST to read into.
     *
     * @todo At some point in the future, use the footprint field in the new s-expression
     *       netlist file to assign a footprint to a component instead of using a secondary
     *       (*.cmp) file.
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
     * @throw IO_ERROR if a the #LINE_READER IO error occurs.
     * @throw PARSE_ERROR if an error occurs while parsing the file.
     * @return true if OK, false if a component reference found in the
     * .cmp file is not found in netlist, which means the .cmp file
     * is not updated. This is an usual case, in CvPcb, but can be used to
     * print a warning in Pcbnew.
     */
    bool Load( NETLIST* aNetlist ) throw( IO_ERROR, PARSE_ERROR );
};


/**
 * Class NETLIST_READER
 * is a pure virtual class to derive a specific type of netlist reader from.
 */
class NETLIST_READER
{
protected:
    NETLIST*     m_netlist;               ///< The net list to read the file(s) into.
    bool         m_loadFootprintFilters;  ///< Load the component footprint filters section if true.
    bool         m_loadNets;              ///< Load the nets section of the netlist file if true.
    LINE_READER* m_lineReader;            ///< The line reader of the netlist.

    /// The reader used to load the footprint links.  If NULL, footprint links are not read.
    CMP_READER*  m_footprintReader;

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

    NETLIST_READER( LINE_READER*  aLineReader,
                    NETLIST*      aNetlist,
                    CMP_READER*   aFootprintLinkReader = NULL )
    {
        wxASSERT( aLineReader != NULL );

        m_lineReader           = aLineReader;
        m_footprintReader      = aFootprintLinkReader;
        m_netlist              = aNetlist;
        m_loadFootprintFilters = true;
        m_loadNets             = true;
    }

    virtual ~NETLIST_READER();

    /**
     * Function GuessNetlistFileType
     * looks at \a aFileHeaderLine to see if it matches any of the netlist file types it
     * knows about.
     *
     * @param aLineReader is the #LINE_READER object containing lines from the netlist to test.
     * @return the #NETLIST_FILE_T of \a aLineReader.
     */
    static NETLIST_FILE_T GuessNetlistFileType( LINE_READER* aLineReader );

    /**
     * Function GetNetlistReader
     * attempts to determine the net list file type of \a aNetlistFileName and return the
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
                                             const wxString& aCompFootprintFileName = wxEmptyString )
        throw( IO_ERROR );

    /**
     * Function LoadNetlist
     * loads the contents of the netlist file into \a aNetlist.
     *
     * @throw IO_ERROR if a file IO error occurs.
     * @throw PARSE_ERROR if an error occurs while parsing the file.
     */
    virtual void LoadNetlist() throw ( IO_ERROR, PARSE_ERROR ) = 0;

    /**
     * Function GetLineReader()
     * @return the #LINE_READER associated with the #NETLIST_READER.
     */
    LINE_READER* GetLineReader();
};


/**
 * Class LEGACY_NETLIST_READER
 * reads the KiCad legacy and the old Orcad netlist formats.
 *
 * The KiCad legacy netlist format was derived directly from an old Orcad netlist format.  The
 * primary difference is the header was changed so this reader can read both formats.
 */
class LEGACY_NETLIST_READER : public NETLIST_READER
{
    /**
     * Function loadComponent
     * read the \a aLine containing the description of a component from a legacy format
     * netlist and add it to the netlist.
     *
     * Analyze the first line of a component description in netlist:
     * ( /40C08647 $noname R20 4.7K {Lib=R}
     *
     * @param  aText contains the first line of description
     * @return the new component created by parsing \a aLine
     * @throw PARSE_ERROR when \a aLine is not a valid component description.
     */
    COMPONENT* loadComponent( char* aText ) throw( PARSE_ERROR );

    /**
     * Function loadFootprintFilters
     * loads the footprint filter section of netlist file.
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
    void loadFootprintFilters() throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function loadNet
     * read a component net description from \a aText.
     *
     * @param aText is current line read from the netlist.
     * @param aComponent is the component to add the net to.
     * @throw PARSE_ERROR if a error occurs reading \a aText.
     */
    void loadNet( char* aText, COMPONENT* aComponent ) throw( PARSE_ERROR );

public:

    LEGACY_NETLIST_READER( LINE_READER*  aLineReader,
                           NETLIST*      aNetlist,
                           CMP_READER*   aFootprintLinkReader = NULL ) :
        NETLIST_READER( aLineReader, aNetlist, aFootprintLinkReader )
    {
    }

    /**
     * Function LoadNetlist
     * read the netlist file in the legacy format into \a aNetlist.
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
    virtual void LoadNetlist() throw ( IO_ERROR, PARSE_ERROR );
};


/**
 * Class KICAD_NETLIST_PARSER
 * is the parser for reading the KiCad s-expression netlist format.
 */
class KICAD_NETLIST_PARSER : public NETLIST_LEXER
{
private:
    NL_T::T      token;
    LINE_READER* m_lineReader;  ///< The line reader used to parse the netlist.  Not owned.
    NETLIST*     m_netlist;     ///< The netlist to parse into.  Not owned.

    /**
     * Function skipCurrent
     * Skip the current token level, i.e
     * search for the RIGHT parenthesis which closes the current description
     */
    void skipCurrent() throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function parseComponent
     * parse a component description:
     * (comp (ref P1)
     * (value DB25FEMELLE)
     * (footprint DB25FC)
     * (libsource (lib conn) (part DB25))
     * (sheetpath (names /) (tstamps /))
     * (tstamp 3256759C))
     */
    void parseComponent() throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function parseNet
     * Parses a section like
     * (net (code 20) (name /PC-A0)
     *  (node (ref BUS1) (pin 62))
     *  (node (ref U3) (pin 3))
     *  (node (ref U9) (pin M6)))
     *
     * and set the corresponding pads netnames
     */
    void parseNet() throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function parseLibPartList
     * reads the section "libparts" in the netlist:
     * (libparts
     *   (libpart (lib device) (part C)
     *     (description "Condensateur non polarise")
     *     (footprints
     *       (fp SM*)
     *       (fp C?)
     *       (fp C1-1))
     *     (fields
     *       (field (name Reference) C)
     *       (field (name Value) C))
     *     (pins
     *       (pin (num 1) (name ~) (type passive))
     *       (pin (num 2) (name ~) (type passive))))
     *
     *  And add the strings giving the footprint filter (subsection footprints)
     *  of the corresponding module info
     *  <p>This section is used by CvPcb, and is not useful in Pcbnew,
     *  therefore it it not always read </p>
     */
    void parseLibPartList() throw( IO_ERROR, PARSE_ERROR );


public:
    KICAD_NETLIST_PARSER( LINE_READER* aReader, NETLIST* aNetlist );

    void SetLineReader( LINE_READER* aLineReader );

    void SetNetlist( NETLIST* aNetlist ) { m_netlist = aNetlist; }

    /**
     * Function Parse
     * parse the full netlist
     */
    void Parse() throw( IO_ERROR, PARSE_ERROR );

    // Useful for debug only:
    const char* getTokenName( NL_T::T aTok )
    {
        return NETLIST_LEXER::TokenName( aTok );
    }
};


/**
 * Class KICAD_NETLIST_READER
 * read the new s-expression based KiCad netlist format.
 */
class KICAD_NETLIST_READER : public NETLIST_READER
{
    KICAD_NETLIST_PARSER* m_parser;     ///< The s-expression format parser.

public:
    KICAD_NETLIST_READER( LINE_READER*  aLineReader,
                          NETLIST*      aNetlist,
                          CMP_READER*   aFootprintLinkReader = NULL ) :
        NETLIST_READER( aLineReader, aNetlist, aFootprintLinkReader ),
        m_parser( new KICAD_NETLIST_PARSER( aLineReader, aNetlist ) )
    {
    }

    virtual ~KICAD_NETLIST_READER()
    {
        if( m_parser )
            delete m_parser;
    }

    virtual void LoadNetlist() throw ( IO_ERROR, PARSE_ERROR );
};


#endif   // NETLIST_READER_H
