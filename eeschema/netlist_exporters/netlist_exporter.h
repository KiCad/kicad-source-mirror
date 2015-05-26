/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers
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

#ifndef NETLIST_EXPORTER_H
#define NETLIST_EXPORTER_H

#include <kicad_string.h>

#include <class_netlist_object.h>
#include <lib_pin.h>
#include <sch_component.h>
#include <sch_text.h>
#include <sch_sheet.h>

/**
 * Class UNIQUE_STRINGS
 * tracks unique wxStrings and is useful in telling if a string
 * has been seen before.
 */
class UNIQUE_STRINGS
{
    std::set<wxString>      m_set;    ///< set of wxStrings already found

    typedef std::set<wxString>::iterator us_iterator;

public:
    /**
     * Function Clear
     * erases the record.
     */
    void Clear()  {  m_set.clear();  }

    /**
     * Function Lookup
     * returns true if \a aString already exists in the set, otherwise returns
     * false and adds \a aString to the set for next time.
     */
    bool Lookup( const wxString& aString )
    {
        std::pair<us_iterator, bool> pair = m_set.insert( aString );

        return !pair.second;
    }
};


/**
 * Class NETLIST_EXPORTER
 * is a abstract class used for the netlist exporters that eeschema supports.
 */
class NETLIST_EXPORTER
{
protected:
    NETLIST_OBJECT_LIST* m_masterList;      /// The main connected items flat list

    PART_LIBS*          m_libs;             /// no ownership

    /// Used to temporary store and filter the list of pins of a schematic component
    /// when generating schematic component data in netlist (comp section)
    NETLIST_OBJECT_LIST m_SortedComponentPinList;

    /// Used for "multi parts per package" components,
    /// avoids processing a lib component more than once.
    UNIQUE_STRINGS      m_ReferencesAlreadyFound;

    // share a code generated std::set<void*> to reduce code volume

    std::set<void*>     m_LibParts;     ///< unique library parts used

    std::set<void*>     m_Libraries;    ///< unique libraries used


    /**
     * Function sprintPinNetName
     * formats the net name for \a aPin using \a aNetNameFormat into \a aResult.
     * <p>
     *  Net name is:
     *  <ul>
     * <li> "?" if pin not connected
     * <li> "netname" for global net (like gnd, vcc ..
     * <li> "/path/netname" for the usual nets
     * </ul>
     * if aUseNetcodeAsNetName is true, the net name is just the net code (SPICE only)
     */
    static void sprintPinNetName( wxString& aResult, const wxString& aNetNameFormat,
                                  NETLIST_OBJECT* aPin, bool aUseNetcodeAsNetName = false );

    /**
     * Function findNextComponentAndCreatePinList
     * finds a component from the DrawList and builds
     * its pin list in m_SortedComponentPinList. This list is sorted by pin num.
     * the component is the next actual component after aItem
     * (power symbols and virtual components that have their reference starting by '#'are skipped).
     */
    SCH_COMPONENT* findNextComponentAndCreatePinList( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath );

    SCH_COMPONENT* findNextComponent( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath );


    /**
     * Function eraseDuplicatePins
     * erase duplicate Pins from m_SortedComponentPinList (i.e. set pointer in this list to NULL).
     * (This is a list of pins found in the whole schematic, for a single
     * component.) These duplicate pins were put in list because some pins (powers... )
     * are found more than one time when we have a multiple parts per package
     * component. For instance, a 74ls00 has 4 parts, and therefore the VCC pin
     * and GND pin appears 4 times in the list.
     * Note: this list *MUST* be sorted by pin number (.m_PinNum member value)
     * Also set the m_Flag member of "removed" NETLIST_OBJECT pin item to 1
     */
    void eraseDuplicatePins();

    /**
     * Function addPinToComponentPinList
     * adds a new pin description to the pin list m_SortedComponentPinList.
     * A pin description is a pointer to the corresponding structure
     * created by BuildNetList() in the table g_NetObjectslist.
     */
    bool addPinToComponentPinList( SCH_COMPONENT*  Component,
                                   SCH_SHEET_PATH* sheet,
                                   LIB_PIN*        PinEntry );

    /**
     * Function findAllInstancesOfComponent
     * is used for "multiple parts per package" components.
     * <p>
     * Search the entire design for all instances of \a aComponent based on
     * matching reference designator, and for each part, add all its pins
     * to the temporary sorted pin list.
     */
    void findAllInstancesOfComponent( SCH_COMPONENT*  aComponent,
                                      LIB_PART*       aEntry,
                                      SCH_SHEET_PATH* aSheetPath );

public:
    NETLIST_EXPORTER( NETLIST_OBJECT_LIST* aMasterList, PART_LIBS* aLibs )
    {
        m_masterList = aMasterList;
        m_libs = aLibs;
    }

    virtual ~NETLIST_EXPORTER()
    {
    }

    /**
     * Function Write
     * writes to specified output file
     */
    virtual bool Write( const wxString& aOutFileName, unsigned aNetlistOptions )
    {
        return false;
    }

    /**
     * Function MakeCommandLine
     * builds up a string that describes a command line for
     * executing a child process. The input and output file names
     * along with any options to the executable are all possibly
     * in the returned string.
     *
     * @param aFormatString holds:
     *   <ul>
     *   <li>the name of the external program
     *   <li>any options needed by that program
     *   <li>formatting sequences, see below.
     *   </ul>
     *
     * @param aTempfile is the name of an input file to the
     *  external program.
     * @param aFinalFile is the name of an output file that
     *  the user expects.
     * @param aProjectDirectory is used for %P replacement, it should omit
     *  the trailing '/'.
     *
     *  <p> Supported formatting sequences and their meaning:
     *  <ul>
     *  <li> %B => base filename of selected output file, minus
     *       path and extension.
     *  <li> %I => complete filename and path of the temporary
     *       input file.
     *  <li> %O => complete filename and path of the user chosen
     *       output file.
     *  <li> %P => project directory, without name and without trailing '/'
     *  </ul>
     */
    static wxString MakeCommandLine( const wxString& aFormatString,
            const wxString& aTempfile, const wxString& aFinalFile,
            const wxString& aProjectDirectory
            );
};

#endif
