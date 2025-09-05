/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef NETLIST_EXPORTER_H
#define NETLIST_EXPORTER_H

#include <schematic.h>

class SCH_SYMBOL;
class LIB_SYMBOL;
class REPORTER;

/**
 * Track unique wxStrings and is useful in telling if a string has been seen before.
 */
class UNIQUE_STRINGS
{
public:
    /**
     * Erase the record.
     */
    void Clear()  {  m_set.clear();  }

    /**
     * @return true if \a aString already exists in the set, otherwise return false and
     *         add \a aString to the set for next time.
     */
    bool Lookup( const wxString& aString )
    {
        std::pair<us_iterator, bool> pair = m_set.insert( aString );

        return !pair.second;
    }

    std::set<wxString>      m_set;    ///< set of wxStrings already found

    typedef std::set<wxString>::iterator us_iterator;
};

/**
 * Used by std:set<LIB_SYMBOL*> instantiation which uses #LIB_SYMBOL name as its key.
 */
struct LIB_SYMBOL_LESS_THAN
{
    // a "less than" test on two LIB_SYMBOLs (.m_name wxStrings)
    bool operator()( LIB_SYMBOL* const& libsymbol1, LIB_SYMBOL* const& libsymbol2 ) const;
};


struct PIN_INFO
{
    PIN_INFO( const wxString& aPinNumber, const wxString& aNetName, const wxString& aPinName ) :
            num( aPinNumber ),
            netName( aNetName ),
            pinName( aPinName )
    {}

    wxString num;
    wxString netName;
    wxString pinName;
};


/**
 * An abstract class used for the netlist exporters that Eeschema supports.
 */
class NETLIST_EXPORTER_BASE
{
public:
    NETLIST_EXPORTER_BASE( SCHEMATIC* aSchematic ) :
        m_schematic( aSchematic )
    {
        wxASSERT( aSchematic );
    }

    virtual ~NETLIST_EXPORTER_BASE() = default;

    /**
     * Write to specified output file.
     */
    virtual bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions,
                               REPORTER& aReporter )
    {
        return false;
    }

    /**
     * Build up a string that describes a command line for executing a child process.
     *
     * The input and output file names along with any options to the executable are all possibly
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
                                     const wxString& aFinalFile,
                                     const wxString& aProjectDirectory );

protected:
    /**
     * Find a symbol from the DrawList and builds its pin list.
     *
     * This list is sorted by pin number. The symbol is the next actual symbol after \a aSymbol.
     * Power symbols and virtual symbols that have their reference designators starting with
     * '#' are skipped.
     * if aKeepUnconnectedPins = false, unconnected pins will be removed from list
     * but usually we need all pins in netlists.
     */
    std::vector<PIN_INFO> CreatePinList( SCH_SYMBOL* aSymbol, const SCH_SHEET_PATH& aSheetPath,
                                         bool aKeepUnconnectedPins );

    /**
     * Check if the given symbol should be processed for netlisting.
     *
     * Prevent processing multi-unit symbols more than once, etc.
     *
     * @param aItem is a symbol to check
     * @param aSheetPath is the sheet to check the symbol for
     * @return the symbol if it should be processed, or nullptr
     */
    SCH_SYMBOL* findNextSymbol( EDA_ITEM* aItem, const SCH_SHEET_PATH& aSheetPath );

    /**
     * Erase duplicate pins.
     *
     * (This is a list of pins found in the whole schematic, for a single symbol.) These
     * duplicate pins were put in list because some pins (power pins...) are found more than
     * once when in "multiple symbols per package" symbols. For instance, a 74ls00 has 4 symbols,
     * and therefore the VCC pin and GND pin appears 4 times in the list.
     * Note: this list *MUST* be sorted by pin number (.m_PinNum member value)
     * Also set the m_Flag member of "removed" NETLIST_OBJECT pin item to 1
     */
    void eraseDuplicatePins( std::vector<PIN_INFO>& pins );

    /**
     * Find all units for symbols with multiple symbols per package.
     *
     * Search the entire design for all units of \a aSymbol based on matching reference
     * designator, and for each unit, add all its pins to the sorted pin list.
     * if aKeepUnconnectedPins = false, unconnected pins will be removed from list
     * but usually we need all pins in netlists.
     */
    void findAllUnitsOfSymbol( SCH_SYMBOL* aSchSymbol, const SCH_SHEET_PATH& aSheetPath,
                               std::vector<PIN_INFO>& aPins, bool aKeepUnconnectedPins );

    /// Used for "multiple symbols per package" symbols to avoid processing a lib symbol more than
    /// once
    UNIQUE_STRINGS        m_referencesAlreadyFound;

    /// unique library symbols used. LIB_SYMBOL items are sorted by names
    std::set<LIB_SYMBOL*, LIB_SYMBOL_LESS_THAN> m_libParts;

    /// The schematic we're generating a netlist for
    SCHEMATIC*      m_schematic;
};

#endif
