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

#include <kicad_string.h>

#include <class_libentry.h>
#include <netlist_object.h>
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

/**
 * NETLIST_EXPORTER
 * is a abstract class used for the netlist exporters that eeschema supports.
 */
class NETLIST_EXPORTER
{
protected:
// TODO(JE) NETLISTING
#if 0
    /// Used to temporarily store and filter the list of pins of a schematic component
    /// when generating schematic component data in netlist (comp section). No ownership
    /// of members.
    /// TODO(snh): Descope this object
    NETLIST_OBJECTS       m_SortedComponentPinList;
#endif

    /// Used for "multi parts per package" components,
    /// avoids processing a lib component more than once.
    UNIQUE_STRINGS        m_ReferencesAlreadyFound;

    /// unique library parts used. LIB_PART items are sorted by names
    std::set<LIB_PART*, LIB_PART_LESS_THAN> m_LibParts;

    /// The schematic we're generating a netlist for
    SCHEMATIC* m_schematic;



    SCH_COMPONENT* findNextComponent( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath );

public:

    /**
     * Constructor
     * @param aMasterList we take ownership of this here.
     * @param aLibTable is the symbol library table of the project.
     */
    NETLIST_EXPORTER( SCHEMATIC* aSchematic ) :
        m_schematic( aSchematic )
    {
        wxASSERT( aSchematic );
    }

    virtual ~NETLIST_EXPORTER()
    {
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
     * @param aNetlistFile is the name of the input file for the
     *  external program, that is a intermediate netlist file in xml format.
     * @param aFinalFile is the name of the output file that
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
            const wxString& aNetlistFile, const wxString& aFinalFile,
            const wxString& aProjectDirectory
            );
};

#endif
