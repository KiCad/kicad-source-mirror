/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers
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

#ifndef NETLIST_EXPORTER_PSPICE_H
#define NETLIST_EXPORTER_PSPICE_H

#include "netlist_exporter_base.h"
#include <list>
#include <map>

class PROJECT;

/// Flags for Spice netlist generation (can be combined)
enum SPICE_NETLIST_OPTIONS {
    NET_ADJUST_INCLUDE_PATHS = 8, // use full paths for included files (if they are in search path)
    NET_ADJUST_PASSIVE_VALS = 16, // reformat passive component values (e.g. 1M -> 1Meg)
    NET_ALL_FLAGS = 0xffff
};

enum SPICE_FIELD {
    SF_PRIMITIVE,
    SF_MODEL,
    SF_ENABLED,
    SF_NODE_SEQUENCE,
    SF_LIB_FILE,
    SF_END     // sentinel
};

///< Basic Spice component primitives
enum SPICE_PRIMITIVE {
    SP_UNKNOWN      = ' ',
    SP_RESISTOR     = 'R',
    SP_CAPACITOR    = 'C',
    SP_INDUCTOR     = 'L',
    SP_DIODE        = 'D',
    SP_BJT          = 'Q',
    SP_MOSFET       = 'M',
    SP_JFET         = 'J',
    SP_SUBCKT       = 'X',
    SP_VSOURCE      = 'V',
    SP_ISOURCE      = 'I'
};

/// @todo add NET_ADJUST_INCLUDE_PATHS & NET_ADJUST_PASSIVE_VALS checkboxes in the netlist
///       export dialog.

/**
 * Structure to represent a schematic component in the Spice simulation.
 */
struct SPICE_ITEM
{
    ///< Schematic component represented by this SPICE_ITEM.
    SCH_COMPONENT* m_parent;

    ///< Spice primitive type (@see SPICE_PRIMITIVE).
    wxChar m_primitive;

    ///< Library model (for semiconductors and subcircuits), component value (for passive
    ///< components) or voltage/current (for sources).
    wxString m_model;

    ///<
    wxString m_refName;

    ///< Flag to indicate whether the component should be used in simulation.
    bool m_enabled;

    ///< Array containing Standard Pin Name
    std::vector<wxString> m_pins;

    ///< Numeric indices into m_SortedComponentPinList
    std::vector<int> m_pinSequence;
};


/**
 * Generate a PSPICE compatible netlist.
 */
class NETLIST_EXPORTER_PSPICE : public NETLIST_EXPORTER_BASE
{
public:
    NETLIST_EXPORTER_PSPICE( SCHEMATIC_IFACE* aSchematic ) :
            NETLIST_EXPORTER_BASE( aSchematic )
    {
    }

    virtual ~NETLIST_EXPORTER_PSPICE()
    {
    }

    typedef std::list<SPICE_ITEM> SPICE_ITEM_LIST;

    ///< Net name to circuit node number mapping
    typedef std::map<wxString, int> NET_INDEX_MAP;

    /**
     * Return list of items representing schematic components in the Spice world.
     */
    const SPICE_ITEM_LIST& GetSpiceItems() const
    {
        return m_spiceItems;
    }

    /**
     * Return name of Spice device corresponding to a schematic component.
     *
     * @param aComponent is the component reference.
     * @return Spice device name or empty string if there is no such component in the netlist. The
     * name is either plain reference if the first character of reference corresponds to the
     * assigned device model type or it is the reference prefixed with a character defining
     * the device model type.
     */
    wxString GetSpiceDevice( const wxString& aComponent ) const;

    /**
     * Write to specified output file
     */
    bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions ) override;

    ///< @copydoc NETLIST_EXPORTER_BASE::Format()
    bool Format( OUTPUTFORMATTER* aFormatter, unsigned aCtl );

    /**
     * Process the netlist to create net mapping and a list of SPICE_ITEMs.
     * It is automatically called by WriteNetlist(), but might be used separately,
     * if only net mapping and the list of SPICE_ITEMs are required.
     * @return True if successful.
     */
    bool ProcessNetlist( unsigned aCtl );


    /**
     * Replace illegal spice net name characters with an underscore.
     *
     * @param aNetName is the net name to modify.
     */
     static void ReplaceForbiddenChars( wxString& aNetName );

    /**
     * Return a map of circuit nodes to net names.
     */
    const NET_INDEX_MAP& GetNetIndexMap() const
    {
        return m_netMap;
    }

    /**
     * Return a vector of component field names related to Spice simulation.
     */
    static const std::vector<wxString>& GetSpiceFields()
    {
        return m_spiceFields;
    }

    /**
     * Return a string used for a particular component field related to Spice simulation.
     */
    static const wxString& GetSpiceFieldName( SPICE_FIELD aField )
    {
        return m_spiceFields[(int) aField];
    }

    /**
     * Retrieve either the requested field value or the default value.
     */
    static wxString GetSpiceField( SPICE_FIELD aField, SCH_COMPONENT* aSymbol, unsigned aCtl );

    /**
     * Retrieve the default value for a given field.
     */
    static wxString GetSpiceFieldDefVal( SPICE_FIELD aField, SCH_COMPONENT* aSymbol,
                                         unsigned aCtl );

    /**
     * Update the vector of Spice directives placed in the schematics.
     */
    void UpdateDirectives( unsigned aCtl );

    /**
     * Return a vector of Spice directives found in the schematics.
     */
    const std::vector<wxString> GetDirectives() const
    {
        return m_directives;
    }

    /**
     * Convert typical boolean string values (no/yes, true/false, 1/0) to a boolean value.
     */
    static bool StringToBool( const wxString& aStr )
    {
        if( aStr.IsEmpty() )
            return false;

        char c = tolower( aStr[0] );

        // Different ways of saying false (no/false/0)
        return !( c == 'n' || c == 'f' || c == '0' );
    }

protected:
    /**
     * Save the Spice directives.
     */
    virtual void writeDirectives( OUTPUTFORMATTER* aFormatter, unsigned aCtl ) const;

private:
    ///< Spice simulation title found in the processed schematic sheet
    wxString m_title;

    ///< Spice directives found in the processed schematic sheet
    std::vector<wxString> m_directives;

    ///< Libraries used by the simulated circuit
    std::set<wxString> m_libraries;

    ///< Map circuit nodes to net names
    NET_INDEX_MAP m_netMap;

    ///< List of items representing schematic components in the Spice world
    SPICE_ITEM_LIST m_spiceItems;

    // Component fields that are processed during netlist export & simulation
    static const std::vector<wxString> m_spiceFields;
};

#endif
