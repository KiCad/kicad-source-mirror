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
#include <sim/sim_model.h>
#include <sim/sim_library.h>


struct SPICE_ITEM
{
    wxString refName;
    wxString libraryPath;
    std::vector<wxString> pins;
    std::unique_ptr<const SIM_MODEL> model;
    wxString modelName;
};


class NETLIST_EXPORTER_SPICE : public NETLIST_EXPORTER_BASE
{
public:
    enum OPTIONS
    {
        OPTION_ADJUST_INCLUDE_PATHS = 8,
        OPTION_ADJUST_PASSIVE_VALS = 16,
        OPTION_SAVE_ALL_VOLTAGES = 16,
        OPTION_SAVE_ALL_CURRENTS = 32,
        OPTION_ALL_FLAGS = 0xFFFF
    };

    NETLIST_EXPORTER_SPICE( SCHEMATIC_IFACE* aSchematic ) : NETLIST_EXPORTER_BASE( aSchematic ) {}

    /**
     * Write to specified output file.
     */
    bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions ) override;

    /**
     * Generate the netlist in aFormatter.
     */
    bool GenerateNetlist( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions );

    /**
     * Process the schematic and Spice libraries to create net mapping and a list of SPICE_ITEMs.
     * It is automatically called by WriteNetlist(), but might be used separately,
     * if only net mapping and the list of SPICE_ITEMs are required.
     * @return True if successful.
     */
    bool ReadSchematicAndLibraries( unsigned aNetlistOptions );

    /**
     * Replace illegal spice net name characters with underscores.
     */
    static void ReplaceForbiddenChars( wxString& aNetName );

    /**
     * Return the list of nets.
     */
    const std::set<wxString>& GetNets() const { return m_nets; }

    /**
     * Return name of Spice device corresponding to a schematic symbol.
     *
     * @param aRefName is the component reference.
     * @return Spice device name or empty string if there is no such symbol in the netlist.
     * Normally the name is either plain reference if the first character of reference corresponds
     * to the assigned device model type or it is the reference prefixed with a character defining
     * the device model type.
     */
    wxString GetItemName( const wxString& aRefName ) const;

    /**
     * Return the list of items representing schematic components in the Spice world.
     */
    const std::list<SPICE_ITEM>& GetItems() const { return m_items; }

    const std::vector<wxString>& GetDirectives() { return m_directives; }

protected:
    void ReadDirectives();
    virtual void WriteDirectives( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions ) const;

private:
    void readLibraryField( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem );
    void readNameField( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem );
    void readEnabledField( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem );
    bool readRefName( SCH_SHEET_PATH& aSheet, SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
                      std::set<wxString>& aRefNames );
    bool readModel( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem );
    void readPins( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem, int& notConnectedCounter );

    void writeInclude( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions,
                       const wxString& aPath );

    void writeIncludes( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions );
    void writeModels( OUTPUTFORMATTER& aFormatter );
    void writeItems( OUTPUTFORMATTER& aFormatter );

    wxString                m_title;        ///< Spice simulation title found in the schematic sheet
    std::vector<wxString>   m_directives;   ///< Spice directives found in the schematic sheet
    std::map<wxString, std::unique_ptr<SIM_LIBRARY>> m_libraries; ///< Spice libraries
    std::set<wxString>      m_rawIncludes;  ///< include directives found in symbols
    std::set<wxString>      m_nets;
    std::list<SPICE_ITEM>   m_items;        ///< Items representing schematic symbols in Spice world
};


#endif // NETLIST_EXPORTER_PSPICE_H
