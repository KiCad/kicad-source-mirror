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

#include <netlist_lexer.h>    // netlist_lexer is common to Eeschema and Pcbnew


using namespace NL_T;


class MODULE;
class LINE_READER;
class REPORTER;


/**
 * Class COMPONENT_NET
 * is used to store the component pin name to net name associations stored in a netlist.
 */
class COMPONENT_NET
{
    wxString m_pinName;
    wxString m_netNumber;
    wxString m_netName;

public:
    COMPONENT_NET() {}

    COMPONENT_NET( const wxString& aPinName, const wxString& aNetName )
    {
        m_pinName = aPinName;
        m_netName = aNetName;
    }

    const wxString& GetPinName() const { return m_pinName; }

    const wxString& GetNetName() const { return m_netName; }

    bool IsValid() const { return !m_pinName.IsEmpty(); }

    bool operator <( const COMPONENT_NET& aNet ) const
    {
        return m_pinName < aNet.m_pinName;
    }

#if defined(DEBUG)
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param aNestLevel An aid to prettier tree indenting, and is the level
     *                   of nesting of this object within the overall tree.
     * @param aReporter A reference to a #REPORTER object to output to.
     */
    virtual void Show( int aNestLevel, REPORTER& aReporter );
#endif
};


typedef std::vector< COMPONENT_NET > COMPONENT_NETS;


/**
 * Class COMPONENT
 * is used to store components and all of their related information found in a netlist.
 */
class COMPONENT
{
    COMPONENT_NETS m_nets;
    wxArrayString  m_footprintFilters; ///< Footprint filters found in netlist.
    wxString       m_reference;        ///< The component reference designator found in netlist.
    wxString       m_value;            ///< The component value found in netlist.

    // ZZZ This timestamp is string, not time_t
    wxString       m_timeStamp;        ///< The component full time stamp found in netlist.

    /// The name of the component in #m_library used when it was placed on the schematic..
    wxString       m_name;

    /**
     * The name of the component library where #m_name was found.  This will be set to
     * wxEmptyString for legacy netlist files.
     */
    wxString       m_library;

    /// The name of the footprint in the footprint library assigned to the component.
    wxString       m_footprintName;

    /**
     * The name of the footprint library that #m_footprintName is located.  This will be
     * set to wxEmptyString for legacy netlist formats indicating that all libraries need
     * to be searched.
     */
    wxString       m_footprintLib;

    /// The #MODULE loaded for #m_footprintName found in #m_footprintLib.
    std::auto_ptr< MODULE > m_footprint;

    /// Set to true if #m_footprintName or #m_footprintLib was changed when the footprint
    /// link file was read.
    bool           m_footprintChanged;

    static COMPONENT_NET    m_emptyNet;

public:
    COMPONENT( const wxString& aFootprintName,
               const wxString& aReference,
               const wxString& aValue,
               const wxString& aTimeStamp )
    {
        m_footprintName    = aFootprintName;
        m_reference        = aReference;
        m_value            = aValue;
        m_timeStamp        = aTimeStamp;
        m_footprintChanged = false;
    }

    virtual ~COMPONENT() { };

    void AddNet( const wxString& aPinName, const wxString& aNetName )
    {
        m_nets.push_back( COMPONENT_NET( aPinName, aNetName ) );
    }

    unsigned GetNetCount() const { return m_nets.size(); }

    const COMPONENT_NET& GetNet( unsigned aIndex ) const { return m_nets[aIndex]; }

    const COMPONENT_NET& GetNet( const wxString& aPinName );

    void SortPins() { sort( m_nets.begin(), m_nets.end() ); }

    void SetName( const wxString& aName ) { m_name = aName;}
    const wxString& GetName() const { return m_name; }

    void SetLibrary( const wxString& aLibrary ) { m_library = aLibrary; }
    const wxString& GetLibrary() const { return m_library; }

    const wxString& GetReference() const { return m_reference; }

    const wxString& GetValue() const { return m_value; }

    void SetFootprintName( const wxString& aFootprintName )
    {
        m_footprintChanged = !m_footprintName.IsEmpty() && (m_footprintName != aFootprintName);
        m_footprintName = aFootprintName;
    }

    const wxString& GetFootprintName() const { return m_footprintName; }

    void SetFootprintLib( const wxString& aFootprintLib )
    {
        m_footprintChanged = !m_footprintLib.IsEmpty() && (m_footprintLib != aFootprintLib);
        m_footprintLib = aFootprintLib;
    }

    const wxString& GetFootprintLib() const { return m_footprintLib; }

    const wxString& GetTimeStamp() const { return m_timeStamp; }

    void SetFootprintFilters( const wxArrayString& aFilterList )
    {
        m_footprintFilters = aFilterList;
    }

    const wxArrayString& GetFootprintFilters() const { return m_footprintFilters; }

    /**
     * Function MatchesFootprintFilters
     *
     * @return true if \a aFootprintName matches any of the footprint filters or no footprint
     *         filters are defined.
     */
    bool MatchesFootprintFilters( const wxString& aFootprintName ) const;

    MODULE* GetModule( bool aRelease = false )
    {
        return ( aRelease ) ? m_footprint.release() : m_footprint.get();
    }

    void SetModule( MODULE* aModule );

    bool IsLibSource( const wxString& aLibrary, const wxString& aName ) const
    {
        return aLibrary == m_library && aName == m_name;
    }

    bool FootprintChanged() const { return m_footprintChanged; }

#if defined(DEBUG)
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param aNestLevel An aid to prettier tree indenting, and is the level
     *                   of nesting of this object within the overall tree.
     * @param aReporter A reference to a #REPORTER object to output to.
     */
    virtual void Show( int aNestLevel, REPORTER& aReporter );
#endif
};


typedef boost::ptr_vector< COMPONENT > COMPONENTS;
typedef COMPONENTS::iterator           COMPONENTS_ITER;
typedef COMPONENTS::const_iterator     COMPONENTS_CITER;


/**
 * Class NETLIST
 * stores all of information read from a netlist along with the flags used to update
 * the NETLIST in the #BOARD.
 */
class NETLIST
{
    COMPONENTS         m_components;           ///< Components found in the netlist.

    /// Remove footprints from #BOARD not found in netlist when true.
    bool               m_deleteExtraFootprints;

    /// Do not actually make any changes.  Only report changes to #BOARD from netlist
    /// when true.
    bool               m_isDryRun;

    /// Find component by time stamp if true or reference designator if false.
    bool               m_findByTimeStamp;

    /// Replace component footprints when they differ from the netlist if true.
    bool               m_replaceFootprints;

public:
    NETLIST() :
        m_deleteExtraFootprints( false ),
        m_isDryRun( false ),
        m_findByTimeStamp( false ),
        m_replaceFootprints( false )
    {
    }

    /**
     * Function IsEmpty()
     * @return true if there are no components in the netlist.
     */
    bool IsEmpty() const { return m_components.empty(); }

    /**
     * Function Clear
     * removes all components from the netlist.
     */
    void Clear() { m_components.clear(); }

    /**
     * Function GetCount
     * @return the number of components in the netlist.
     */
    unsigned GetCount() const { return m_components.size(); }

    /**
     * Function GetComponent
     * returns the #COMPONENT at \a aIndex.
     *
     * @param aIndex the index in #m_components to fetch.
     * @return a pointer to the #COMPONENT at \a Index.
     */
    COMPONENT* GetComponent( unsigned aIndex ) { return &m_components[ aIndex ]; }

    /**
     * Function AddComponent
     * adds \a aComponent to the NETLIST.
     *
     * @note If \a aComponent already exists in the NETLIST, \a aComponent is deleted
     *       to prevent memory leaks.  An assertion is raised in debug builds.
     *
     * @param aComponent is the COMPONENT to save to the NETLIST.
     */
    void AddComponent( COMPONENT* aComponent );

    /*
     * Function GetComponentByReference
     * returns a #COMPONENT by \a aReference.
     *
     * @param aReference is the reference designator the #COMPONENT.
     * @return a pointer to the #COMPONENT that matches \a aReference if found.  Otherwise NULL.
     */
    COMPONENT* GetComponentByReference( const wxString& aReference );

    /*
     * Function GetComponentByTimeStamp
     * returns a #COMPONENT by \a aTimeStamp.
     *
     * @param aTimeStamp is the time stamp the #COMPONENT.
     * @return a pointer to the #COMPONENT that matches \a aTimeStamp if found.  Otherwise NULL.
     */
    COMPONENT* GetComponentByTimeStamp( const wxString& aTimeStamp );

    void SortByFootprintName();

    void SortByReference();

    void SetDeleteExtraFootprints( bool aDeleteExtraFootprints )
    {
        m_deleteExtraFootprints = aDeleteExtraFootprints;
    }

    bool GetDeleteExtraFootprints() const { return m_deleteExtraFootprints; }

    void SetIsDryRun( bool aIsDryRun ) { m_isDryRun = aIsDryRun; }

    bool IsDryRun() const { return m_isDryRun; }

    void SetFindByTimeStamp( bool aFindByTimeStamp ) { m_findByTimeStamp = aFindByTimeStamp; }

    bool IsFindByTimeStamp() const { return m_findByTimeStamp; }

    void SetReplaceFootprints( bool aReplaceFootprints )
    {
        m_replaceFootprints = aReplaceFootprints;
    }

    bool GetReplaceFootprints() const { return m_replaceFootprints; }

    /**
     * Function AnyFootprintsLinked
     * @return true if any component with a footprint link is found.
     */
    bool AnyFootprintsLinked() const;

    /**
     * Function AllFootprintsLinked
     * @return true if all components have a footprint link.
     */
    bool AllFootprintsLinked() const;

    /**
     * Function NoFootprintsLinked
     * @return true if none of the components have a footprint link.
     */
    bool NoFootprintsLinked() const { return !AnyFootprintsLinked(); }

    /**
     * Function AnyFootprintsChanged
     * @return true if any components footprints were changed when the footprint link file
     *         (*.cmp)  was loaded.
     */
    bool AnyFootprintsChanged() const;

#if defined(DEBUG)
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param aNestLevel An aid to prettier tree indenting, and is the level
     *                   of nesting of this object within the overall tree.
     * @param aReporter A reference to a #REPORTER object to output to.
     */
    virtual void Show( int aNestLevel, REPORTER& aReporter );
#endif
};


/**
 * Class CMP_READER
 * reads a component footprint link file (*.cmp) format.
 */
class CMP_READER
{
    LINE_READER* m_lineReader;            ///< The line reader to read.

public:
    CMP_READER( LINE_READER* aLineReader )
    {
        m_lineReader = aLineReader;
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
    T            token;
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
    const char* getTokenName( T aTok )
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
