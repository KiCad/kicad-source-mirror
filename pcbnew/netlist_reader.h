#ifndef NETLIST_READER_H
#define  NETLIST_READER_H

/**
 * @file netlist_reader.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras.
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

#include <algorithm>
#include <boost/ptr_container/ptr_vector.hpp>

#include <fctsys.h>
#include <kicad_string.h>
#include <wxPcbStruct.h>
#include <richio.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>

/*
 * Helper class, to store for a footprint the footprint filter info,
 * found in new format KiCad netlist.
 * For CvPcb only
 * Note: features for CvPcb are for a temporary use.
 * They could be removed when CvPcb is modified
 * (perhaps when it does not use anumore a netlist to build the component to footprint link)
 */
class LIPBART_INFO
{
public:
    wxString m_Libpart;                 // the libpart name.
    wxArrayString m_FootprintFilter;    // an array of footprint filters found in netlist,
                                        // for this footprint

public:

    LIPBART_INFO( const wxString& aLibpart )
    {
        m_Libpart = aLibpart;
    }
};

typedef std::vector <LIPBART_INFO *> LIPBART_INFO_LIST;


/*
 * Helper class, to store components and footprints info found in netlist.
 * (component reference and time stamp, footprint name ...
 */
class COMPONENT_INFO
{
public:
    wxString m_Footprint;               // the footprint name found in netlist, the in .cmp file
    wxString m_Reference;               // the schematic reference found in netlist
    wxString m_Value;                   // the schematic value found in netlist
    // ZZZ This timestamp is string, not time_t
    wxString m_TimeStamp;               // the schematic full time stamp found in netlist
    wxString m_Libpart;                 // the schematic libpart found in netlist
    wxArrayString m_FootprintFilter;    // a footprint filters list found in old format netlist
    int m_pinCount;                     // the number of pins found in the netlist

public: COMPONENT_INFO( const wxString& libname,
                     const wxString& cmpname,
                     const wxString& value,
                     const wxString& timestamp )
    {
        m_Footprint = libname;
        m_Reference = cmpname;
        m_Value = value;
        m_TimeStamp = timestamp;
        m_pinCount = 0;
    }

    ~COMPONENT_INFO() { };
};

enum typenetlist
{
    NETLIST_TYPE_UNSPECIFIED = 0,
    NETLIST_TYPE_ORCADPCB2,     // the basic format used by pcbnew
    NETLIST_TYPE_PCBNEW,        // the format used by pcbnew, basic format + more info
    NETLIST_TYPE_KICAD          // new format using common S expression
};


typedef std::vector <COMPONENT_INFO*> COMPONENT_INFO_LIST;
/*
 * Helper class, to read a netlist.
 */
class NETLIST_READER
{
private:
    PCB_EDIT_FRAME*  m_pcbframe;            // the main Pcbnew frame (or NULL for CvPcb)
    wxTextCtrl*      m_messageWindow;       // a textctrl to show messages (can be NULL)
    wxString         m_netlistFullName;     // The full netlist filename
    wxString         m_cmplistFullName;     // The full component/footprint association filename
    MODULE*          m_currModule;          // The footprint currently being read in netlist
    COMPONENT_INFO_LIST m_componentsInNetlist;    // The list of footprints, found in netlist
                                            // (must be loaded from libraries)
    COMPONENT_INFO_LIST m_newModulesList;      // The list of new footprints,
                                            // found in netlist, but not on board
                                            // (must be loaded from libraries)
    LIPBART_INFO_LIST  m_libpartList;       // For Kicad new netlist format:
                                            // list of libpart found in netlist
                                            // A libpart contains the footprint filters for CvPcb
    bool m_buildModuleListOnly;             // if true read netlist, populates m_componentsInNetlist
                                            // but do not read and change nets and modules on board
    bool m_readLibpartSection;              // if true read Libparts section,
                                            // and therefore the footprints filters
    enum typenetlist m_typeNetlist;         // type opt the netlist currently read

public:
    bool m_UseCmpFile;              // true to use .cmp files as component/footprint file link
                                    // false to use netlist only to know component/footprint link
    bool m_UseTimeStamp;            // Set to true to identify footprints by time stamp
                                    // false to use schematic reference
    bool m_ChangeFootprints;        // Set to true to change existing footprints to new ones
                                    // when netlist gives a different footprint name

public:

    NETLIST_READER( PCB_EDIT_FRAME* aFrame, wxTextCtrl* aMessageWindow = NULL )
    {
        m_pcbframe = aFrame;
        m_messageWindow    = aMessageWindow;
        m_UseTimeStamp     = false;
        m_ChangeFootprints = false;
        m_UseCmpFile = true;
        m_buildModuleListOnly = false;
        m_readLibpartSection = false;
        m_typeNetlist = NETLIST_TYPE_UNSPECIFIED;
    }

    ~NETLIST_READER()
    {
        // Free modules info list:
        for( unsigned ii = 0; ii < m_newModulesList.size(); ii++ )
            delete m_componentsInNetlist[ii];

        m_componentsInNetlist.clear();
        m_newModulesList.clear();

        // Free libpart info list:
        for( unsigned ii = 0; ii < m_libpartList.size(); ii++ )
            delete m_libpartList[ii];
        m_libpartList.clear();
    }

    /**
     * Function GetNetlistType
     * @return the type of netlist read:
     *  NETLIST_TYPE_UNSPECIFIED:   Unknown format
     *  NETLIST_TYPE_ORCADPCB2:     the basic format used by pcbnew
     *  NETLIST_TYPE_PCBNEW:        the format used by pcbnew, basic format + more info
     *  NETLIST_TYPE_KICAD:         the new format
     */
    int GetNetlistType()
    {
        return m_typeNetlist;
    }

    /**
     * Function GetComponentInfoList
     * @return the component info list built from the netlist
     */
    COMPONENT_INFO_LIST& GetComponentInfoList()
    {
        return m_componentsInNetlist;
    }

    /**
     * Function GetComponentInfoList
     * @return a reference to the libpart info corresponding to a given part
     * @param aPartname = the name of the libpart
     */
    LIPBART_INFO* GetLibpart(const wxString & aPartname);

    /**
     * Function IsCvPcbMode
     * @return true if the netlist is read by CvPcb
     * In cvpcb mode, nets are stored in module info,
     * and the footprint filters list is read.
     * There is also no board in CvPcb
     */
    bool IsCvPcbMode() { return m_pcbframe == 0; }

    /**
     * Function AddModuleInfo
     * Add a new module info to the main list of modules ifo
     * @param aModInfo = a reference to the item to add
     */
    void AddModuleInfo( COMPONENT_INFO* aModInfo )
    {
        m_componentsInNetlist.push_back( aModInfo );
    }

    /**
     * Function AddLibpartInfo
     * LIPBART_INFO items (and therefore footprint filter strings) are stored in
     * m_libpartList
     * @param aPartInfo = a refernce to the LIPBART_INFO to add in list
     */
    void AddLibpartInfo( LIPBART_INFO * aPartInfo )
    {
        m_libpartList.push_back( aPartInfo );
    }

    /**
     * Function ReadLibpartSectionSetOpt
     * Set to true or false the read Partlists section.
     * footprint filters are found in this section
     * When this option is false, the Partlists section is ignored
     * When this option is true, the Partlists section is read,
     * Libpart items (and therefore footprint filter strings) are stored in
     * m_libpartList
     * @param aOpt = the value of option
     */
    void ReadLibpartSectionSetOpt( bool aOpt )
    {
        m_readLibpartSection = aOpt;
    }

    /**
     * Function ReadLibpartSectionOpt
     * @return the readPartlist option
     */
    bool ReadLibpartSectionOpt() { return m_readLibpartSection; }

    /**
     * Function BuildModuleListOnlySetOpt
     * Set to true or false the Build Module List Only option
     * When this option is false, a full netlist read is made,
     * and modules are added/modified
     * When this option is true, a partial netlist read is made
     * and only the list of modules found in netlist is built
     * @param aOpt = the value of option
     */
    void BuildModuleListOnlySetOpt( bool aOpt )
    {
        m_buildModuleListOnly = aOpt;
    }

    /**
     * Function BuildModuleListOnlyOpt
     * Get the Build Module List Only option state
     * @return the state of option (true/false)
     */
     bool BuildModuleListOnlyOpt()
    {
        return m_buildModuleListOnly;
    }

    /**
     * Function InitializeModules
     * Called when reading a netlist and after the module info list is populated
     * Load new module and clear pads netnames
     * return true if all modules are loaded, false if some are missing
     */
    bool InitializeModules();

    /**
     * Function TestFootprintsMatchingAndExchange
     * Called when reading a netlist, after the module info list is populated
     * module reference updated (after a call to InitializeModules)
     * Test, for each module, if the current footprint matches the footprint
     * given by the netlist (or the cmp file, if used)
     * print a list of mismatches od exchange footprints i
     * m_ChangeFootprints == true
     */
    void TestFootprintsMatchingAndExchange();


    /**
     * Function SetFilesnames
     * initialize filenames
     * @param aNetlistFileName = full filename of netlist
     * @param aCmplistFileName = full filename of components file (can be empty)
     * and the components file will be non used
     */
     void SetFilesnames( const wxString& aNetlistFileName,
                      const wxString& aCmplistFileName )
    {
        m_netlistFullName = aNetlistFileName;
        m_cmplistFullName = aCmplistFileName;
    }

    /**
     * Function ReadNetList
     * The main function to detect a netlist format, read the netlist,
     * and update the board
     * depending on the detected format, calls ReadOldFmtdNetList or ReadKicadNetList
     * @param aFile = the already opened file (will be closed by the netlist reader)
     * @return true if success
     */
    bool ReadNetList( FILE* aFile );

    /**
     * Function ReadOldFmtdNetList
     * The main function to read a netlist (old netlist format),
     * and update the board
     * @param aFile = the already opened file (will be closed by ReadOldFmtdNetList)
     * @return true if success
     */
    bool ReadOldFmtdNetList( FILE* aFile );

    /**
     * Function ReadOldFmtFootprintFilterList
     * Read the section "Allowed footprints" like:
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
     *  And add the strings giving the footprint filter to m_FootprintFilter
     *  of the corresponding module info
     *  <p>This section is used by CvPcb, and is not useful in Pcbnew,
     *  therefore it it not always read </p>
     */
    bool ReadOldFmtFootprintFilterList(  FILE_LINE_READER& aNetlistReader );

    /**
     * Function ReadKicadNetList
     * The main function to read a netlist (new netlist format, using S expressions),
     * and update the board
     * @param aFile = the already opened file (will be closed by ReadKicadNetList)
     * @return true if success
     */
    bool ReadKicadNetList( FILE* aFile );

    /**
     * function RemoveExtraFootprints
     * Remove (delete) not locked footprints found on board, but not in netlist
     * The netlist is expected to be read, and the main module list info up to date
     */
    void  RemoveExtraFootprints( );

    /**
     * Function SetPadsNetName
     *  Update pads netnames for a given module.
     *  Because a pad name can be found more than once in this module,
     *  all pads matching the pad name are updated
     *  @param aModule = module reference
     *  @param aPadname = pad name (pad num)
     *  @param aNetname = new net name of the pad
     *  @param aPadList = a std::vector<D_PAD*>& buffer where the updated pads can be stored
     *  @return the pad count
     */
    int SetPadsNetName( const wxString & aModule, const wxString & aPadname,
                          const wxString & aNetname, std::vector<D_PAD*> & aPadList );

private:

    /**
     * Function FindModule
     *  search for a module id the modules existing in the current BOARD.
     *  @param aId = the key to identify the module to find:
     *   The reference or the full time stamp, according to m_UseTimeStamp
     * @return the module found, or NULL.
     */
    MODULE* FindModule( const wxString& aId );

    /**
     * Function SetPadNetName
     *  Update a pad netname using the current footprint
     *  from the netlist (line format: ( \<pad number\> \<net name\> ) )
     *  @param aText = current line read from netlist
     */
    bool    SetPadNetName( char* aText );

    /**
     * Function ReadOldFmtNetlistModuleDescr
     * Read the full description of a footprint, from the netlist
     * and update the corresponding module.
     * @param aBuildList bool to switch between 2 modes:
     *      aBuildList = true:
     *          add module info added to m_newModulesList
     *      aBuildList = false:
     *          The module is searched in the board modules list
     * @param  aText contains the first line of description
     * This function uses m_useFichCmp as a flag to know the footprint name:
     *      If true: component file *.cmp is used
     *      If false: the netlist only is used
     *      This flag is reset to false if the .cmp file is not found
     * @return if aBuildList = true, a reference to the COMPONENT_INFO
     *         if aBuildList = false, a reference to the corresponding MODULE on board (NULL if not found)
     */
    void* ReadOldFmtNetlistModuleDescr( char* aText, bool aBuildList );

    /**
     * Function loadNewModules
     * Load from libraries new modules found in netlist and add them to the current Board.
     * modules to load come from m_newModulesList
     * @return false if a footprint is not found, true if all footprints are loaded
     */
    bool    loadNewModules();

    /**
     * function readModuleComponentLinkfile
     * read the *.cmp file ( filename in m_cmplistFullName )
     * and initialize the m_Footprint member of each item in m_componentsInNetlist,
     * when it is found in file, and with a non empty footprint value
     * giving the equivalence between footprint names and components
     * to find the footprint name corresponding to aCmpIdent
     * @return true and the file can be read
     */
    bool    readModuleComponentLinkfile();
};

#endif  // NETLIST_READER_H
