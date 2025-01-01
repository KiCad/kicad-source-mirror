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

#ifndef NETLIST_EXPORTER_ALLEGRO_H
#define NETLIST_EXPORTER_ALLEGRO_H

#include "netlist_exporter_base.h"

/**
 * Generate a netlist compatible with Allegro.
 */
class NETLIST_EXPORTER_ALLEGRO : public NETLIST_EXPORTER_BASE
{
public:
    NETLIST_EXPORTER_ALLEGRO( SCHEMATIC* aSchematic ) :
        NETLIST_EXPORTER_BASE( aSchematic ),
        m_f( nullptr )
    {
    }

    /**
     * Write netlist to \a aOutFileName.
     * Generate the Allegro netlist format supported by Allegro.
     */
    bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions,
                       REPORTER& aReporter ) override;

    /**
     * Compare two std::pair<SCH_SYMBOL*, SCH_SHEET_PATH> variables.
     *
     * @param aItem1 comparing object 1
     * @param aItem2 comparing object 2
     * @return true if aItem1 < aItem2
     */
    static bool CompareSymbolSheetpath( const std::pair<SCH_SYMBOL*, SCH_SHEET_PATH>& aItem1,
                                        const std::pair<SCH_SYMBOL*, SCH_SHEET_PATH>& aItem2 );

    /**
     * Compare two wxString variables.
     *
     * @param aRefText1
     * @param aRefText2
     * @return bool value
     */
    static bool CompareSymbolRef( const wxString& aRefText1, const wxString& aRefText2 );

    /**
     * Compare two SCH_PIN* variables.
     *
     * @param aPin1
     * @param aPin2
     * @return bool value
     */
    static bool CompareLibPin( const SCH_PIN* aPin1, const SCH_PIN* aPin2 );

private:
    void extractComponentsInfo();

    /**
     * Write the $PACKAGES section
     *
     */
    void toAllegroPackages();

    /**
     * Write the $NETS section
     *
     */
    void toAllegroNets();

    /**
     * Write $A_PROPERTIES section
     *
     */
    void toAllegroPackageProperties();

    /**
     * Convert a string into one safe for a Telesis device name.
     *
     * These are all lowercase and have a more restricted set of characters.
     *
     * @bug Replace unsupported characters with an encoding instead.
     *
     * @param aString wxString to be formatted.
     * @return a formatted wxString.
     */
    wxString formatDevice( wxString aString );

    /**
     * Convert a string into Telesis-safe format. Unsupported characters are
     * replaced with ?'s, and the string is quoted if necessary.
     *
     * @bug Replace unsupported characters with an encoding to avoid having similar strings
     *      mapped to each other.
     *
     * @param aString
     * @return wxString
     */
    wxString formatText( wxString aString );

    /**
     * Generate a Telesis-compatible pin name from a pin node.
     (
     * Telesis requires all pin names to be unique, and doesn't have separate
     * fields for pin number and pin name/function, so we combine them together to
     * make a unique name that still describes its function if you check pin info.
     *
     * @bug Replace unsupported characters with an encoding instead.
     *
     * @param aPin
     * @return wxString
     */
    wxString formatPin( const SCH_PIN& aPin );

    /**
     * Generate the definition of a function in Telesis format, which consists of
     * multiple declarations (PINORDER, PINSWAP, and FUNCTIONs).
     *
     * @param aName
     * @param aPinList
     * @return wxString
     */
    wxString formatFunction( wxString aName, std::vector<SCH_PIN*> aPinList );

    /**
     * Look up a field for a component group, which may have mismatched case, or
     * the component group may not have the field defined and instead the library
     * entry has to be searched.  Returns None if no fields exist.
     *
     * @param aGroupIndex the component group index to query
     * @param aFieldArray one or more (equivalent) fields to query, in the order specified.
     *                   first field that exists is returned.
     * @param aSanitize  if true (default), will format/escape the field for Telesis output
     * @return return the found field, or return wxString("") if no field exist.
     */
    wxString getGroupField( int aGroupIndex, const wxArrayString& aFieldArray,
                            bool aSanitize = true );

    /**
     * Remove the str's tailing digits.
     *
     * @param aString
     * @return wxString
     */
    static wxString removeTailDigits( wxString aString );

    /**
     * Extract the str's tailing number.
     *
     * @param aString
     * @return unsigned int
     */
    static unsigned int extractTailNumber( wxString aString );

    struct NET_NODE
    {
        NET_NODE( SCH_PIN* aPin, const SCH_SHEET_PATH& aSheet, bool aNoConnect ) :
                m_Pin( aPin ),
                m_Sheet( aSheet ),
                m_NoConnect( aNoConnect )
        {}

        bool operator<( const NET_NODE& aNetNode ) const
        {
            wxString refText1 = m_Pin->GetParentSymbol()->GetRef( &m_Sheet );
            wxString refText2 = aNetNode.m_Pin->GetParentSymbol()->GetRef( &aNetNode.m_Sheet );

            if( refText1 == refText2 )
            {
                unsigned long val1, val2;

                //From wxWidgets 3.1.6, the function ToULong can be replaced with ToUInt
                bool convertingResult = m_Pin->GetShownNumber().ToULong( &val1 );
                convertingResult &= aNetNode.m_Pin->GetShownNumber().ToULong( &val2 );

                if( convertingResult )
                {
                    return val1 < val2;
                }
                else
                {
                    return m_Pin->GetShownNumber() < aNetNode.m_Pin->GetShownNumber();
                }
            }

            return CompareSymbolRef( refText1, refText2 );
        }

        SCH_PIN*       m_Pin;
        SCH_SHEET_PATH m_Sheet;
        bool           m_NoConnect;
    };

    FILE* m_f            ;    ///< File pointer for netlist file writing operation.
    wxString m_exportPath;    ///< Directory to store device files.
    std::multimap<wxString, wxString> m_packageProperties;

    /// Store the component group.
    std::multimap<int, std::pair<SCH_SYMBOL*, SCH_SHEET_PATH> > m_componentGroups;

    /// Store the ordered symbols with sheetpath.
    std::list<std::pair<SCH_SYMBOL*, SCH_SHEET_PATH>> m_orderedSymbolsSheetpath;
    std::multimap<wxString, NET_NODE> m_netNameNodes;    ///< Store the NET_NODE with the net name
};

#endif
