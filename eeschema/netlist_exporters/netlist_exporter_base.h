/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers
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

#include <lib_pin.h>
#include <sch_component.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <schematic.h>

/**
 * UNIQUE_STRINGS
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
 * Struct LIB_PART_LESS_THAN
 * is used by std:set<LIB_PART*> instantiation which uses LIB_PART name as its key.
 */
struct LIB_PART_LESS_THAN
{
    // a "less than" test on two LIB_PARTs (.m_name wxStrings)
    bool operator()( LIB_PART* const& libpart1, LIB_PART* const& libpart2 ) const
    {
        // Use case specific GetName() wxString compare
        return libpart1->GetLibId() < libpart2->GetLibId();
    }
};

struct PIN_INFO
{
    PIN_INFO( const wxString& aPinNumber, const wxString& aNetName ) :
            num( aPinNumber ),
            netName( aNetName )
    {}

    wxString num;
    wxString netName;
};

/**
 * NETLIST_EXPORTER_BASE
 * is a abstract class used for the netlist exporters that eeschema supports.
 */
class NETLIST_EXPORTER_BASE
{
protected:
    /// Used to temporarily store and filter the list of pins of a schematic symbol when
    /// generating schematic symbol data in netlist (comp section). No ownership of members.
    /// TODO(snh): Descope this object
    std::vector<PIN_INFO> m_sortedSymbolPinList;

    /// Used for "multiple parts per package" symbols to avoid processing a lib part more than
    /// once
    UNIQUE_STRINGS        m_referencesAlreadyFound;

    /// unique library parts used. LIB_PART items are sorted by names
    std::set<LIB_PART*, LIB_PART_LESS_THAN> m_libParts;

    /// The schematic we're generating a netlist for
    SCHEMATIC_IFACE*            m_schematic;

    /// The schematic's CurrentSheet when we entered.  Restore on exiting.
    SCH_SHEET_PATH        m_savedCurrentSheet;

    /**
     * Function findNextSymbolAndCreatePinList
     * finds a symbol from the DrawList and builds its pin list in m_sortedSymbolPinList. This
     * list is sorted by pin num. The symbol is the next actual symbol after aSymbol.
     *
     * Power symbols and virtual symbols that have their reference designators starting with
     * '#' are skipped.
     */
    void CreatePinList( SCH_COMPONENT* aSymbol, SCH_SHEET_PATH* aSheetPath );

    /**
     * Checks if the given symbol should be processed for netlisting.
     * Prevents processing multi-unit symbols more than once, etc.
     * @param aItem is a symbol to check
     * @param aSheetPath is the sheet to check the symbol for
     * @return the symbol if it should be processed, or nullptr
     */
    SCH_COMPONENT* findNextSymbol( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath );

    /**
     * Function eraseDuplicatePins
     * erase duplicate Pins from m_sortedSymbolPinList (i.e. set pointer in this list to NULL).
     * (This is a list of pins found in the whole schematic, for a single symbol.) These
     * duplicate pins were put in list because some pins (power pins...) are found more than
     * once when in "multiple parts per package" symbols. For instance, a 74ls00 has 4 parts,
     * and therefore the VCC pin and GND pin appears 4 times in the list.
     * Note: this list *MUST* be sorted by pin number (.m_PinNum member value)
     * Also set the m_Flag member of "removed" NETLIST_OBJECT pin item to 1
     */
    void eraseDuplicatePins();

    /**
     * Function findAllUnitsOfSymbol
     * is used for "multiple parts per package" symbols.
     * <p>
     * Search the entire design for all units of \a aSymbol based on matching reference
     * designator, and for each unit, add all its pins to the temporary sorted pin list,
     * m_sortedSymbolPinList.
     */
    void findAllUnitsOfSymbol( SCH_COMPONENT* aSymbol, LIB_PART* aPart,
                               SCH_SHEET_PATH* aSheetPath );


public:

    /**
     * Constructor
     * @param aMasterList we take ownership of this here.
     * @param aLibTable is the symbol library table of the project.
     */
    NETLIST_EXPORTER_BASE( SCHEMATIC_IFACE* aSchematic ) :
        m_schematic( aSchematic )
    {
        wxASSERT( aSchematic );
        m_savedCurrentSheet = m_schematic->CurrentSheet();
    }

    virtual ~NETLIST_EXPORTER_BASE()
    {
        m_schematic->SetCurrentSheet( m_savedCurrentSheet );
    }

    /**
     * Function WriteNetlist
     * writes to specified output file
     */
    virtual bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
    {
        return false;
    }

    /**
     * Function MakeCommandLine
     * builds up a string that describes a command line for executing a child process. The
     * input and output file names along with any options to the executable are all possibly
     * in the returned string.
     *
     * @param aFormatString holds:
     *   <ul>
     *   <li>the name of the external program
     *   <li>any options needed by that program
     *   <li>formatting sequences, see below.
     *   </ul>
     *
     * @param aNetlistFile is the name of the input file for the external program, that is a
     *                     intermediate netlist file in xml format.
     * @param aFinalFile is the name of the output file that the user expects.
     * @param aProjectDirectory is used for %P replacement, it should omit the trailing '/'.
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
    static wxString MakeCommandLine( const wxString& aFormatString, const wxString& aNetlistFile,
                                     const wxString& aFinalFile, const wxString& aProjectDirectory );
};

#endif
