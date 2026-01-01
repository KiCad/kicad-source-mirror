/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef NETLIST_EXPORTER_SPICE_H
#define NETLIST_EXPORTER_SPICE_H

#include "netlist_exporter_base.h"
#include <sim/sim_lib_mgr.h>
#include <sim/sim_library.h>
#include <sim/sim_model.h>
#include <sim/spice_generator.h>


class wxWindow;


class NAME_GENERATOR
{
public:
    std::string Generate( const std::string& aProposedName );

private:
    std::unordered_set<std::string> m_names;
};


class NETLIST_EXPORTER_SPICE : public NETLIST_EXPORTER_BASE
{
public:
    enum OPTIONS
    {
        OPTION_ADJUST_INCLUDE_PATHS  = 0x0010,
        OPTION_ADJUST_PASSIVE_VALS   = 0x0020,
        OPTION_SAVE_ALL_VOLTAGES     = 0x0040,
        OPTION_SAVE_ALL_CURRENTS     = 0x0080,
        OPTION_SAVE_ALL_DISSIPATIONS = 0x0100,
        OPTION_CUR_SHEET_AS_ROOT     = 0x0200,
        OPTION_SIM_COMMAND           = 0x0400,
        OPTION_SAVE_ALL_EVENTS       = 0x0800,
        OPTION_DEFAULT_FLAGS =   OPTION_ADJUST_INCLUDE_PATHS
                               | OPTION_ADJUST_PASSIVE_VALS
                               | OPTION_SAVE_ALL_VOLTAGES
                               | OPTION_SAVE_ALL_CURRENTS
                               | OPTION_SAVE_ALL_DISSIPATIONS
                               | OPTION_SAVE_ALL_EVENTS
    };

    NETLIST_EXPORTER_SPICE( SCHEMATIC* aSchematic );

    /**
     * Write to specified output file.
     */
    bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions,
                       REPORTER& aReporter ) override;

    /**
     * Write the netlist in aFormatter.
     */
    bool DoWriteNetlist( const wxString& aSimCommand, unsigned aSimOptions,
                         OUTPUTFORMATTER& aFormatter, REPORTER& aReporter );

    /**
     * Write the netlist head (title and so on).
     */
    virtual void WriteHead( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions );

    /**
     * Write the tail (.end).
     */
    virtual void WriteTail( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions );

    /**
     * Process the schematic and Spice libraries to create net mapping and a list of SPICE_ITEMs.
     * It is automatically called by WriteNetlist(), but might be used separately,
     * if only net mapping and the list of SPICE_ITEMs are required.
     * @return True if successful.
     */
    virtual bool ReadSchematicAndLibraries( unsigned aNetlistOptions, REPORTER& aReporter );

    /**
     * Remove formatting wrappers and replace illegal spice net name characters with underscores.
     */
    static void ConvertToSpiceMarkup( wxString* aNetName );

    /**
     * Return the list of nets.
     */
    std::set<wxString> GetNets() const { return m_nets; }

    /**
     * Return name of Spice device corresponding to a schematic symbol.
     *
     * @param aRefName is the component reference.
     * @return Spice device name or empty string if there is no such symbol in the netlist.
     * Normally the name is either a plain reference if the first character of reference
     * corresponds to the assigned device model type or a reference prefixed with a character
     * defining the device model type.
     */
    wxString GetItemName( const wxString& aRefName ) const;

    /**
     * Return the list of items representing schematic symbols in the Spice world.
     */
    const std::list<SPICE_ITEM>& GetItems() const { return m_items; }

    /**
     * Find and return the item corresponding to \a aRefName.
     */
    const SPICE_ITEM* FindItem( const wxString& aRefName ) const;

    const std::vector<wxString>& GetDirectives() { return m_directives; }

protected:
    void ReadDirectives( unsigned aNetlistOptions );
    virtual void WriteDirectives( const wxString& aSimCommand, unsigned aSimOptions,
                                  OUTPUTFORMATTER& candidate ) const;

    virtual wxString GenerateItemPinNetName( const wxString& aNetName, int& aNcCounter ) const;

    /**
     * Return the paths of exported sheets (either all or the current one).
     */
    SCH_SHEET_LIST BuildSheetList( unsigned aNetlistOptions = 0 ) const;

private:
    void readRefName( SCH_SHEET_PATH& aSheet, SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
                      std::set<std::string>& aRefNames );

    /**
     * Collect merged Sim.Pins from all units of a multi-unit symbol.
     *
     * For multi-unit symbols, each unit may have its own partial Sim.Pins mapping. This method
     * finds all units with the same reference and merges their Sim.Pins fields into a single
     * combined mapping.
     *
     * @param aSymbol The symbol to collect pins for
     * @param aSheet The sheet path for the symbol
     * @return The merged Sim.Pins string, or empty string if symbol is not multi-unit
     */
    wxString collectMergedSimPins( SCH_SYMBOL& aSymbol, const SCH_SHEET_PATH& aSheet );

    void readModel( SCH_SHEET_PATH& aSheet, SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
                    REPORTER& aReporter );
    void readPinNumbers( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
                         const std::vector<PIN_INFO>& aPins );
    void readPinNetNames( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
                          const std::vector<PIN_INFO>& aPins, int& aNcCounter );
    void getNodePattern( SPICE_ITEM& aItem, std::vector<std::string>& aModifiers );
    void readNodePattern( SPICE_ITEM& aItem );

    void writeInclude( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions,
                       const wxString& aPath );

    void writeIncludes( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions );
    void writeModels( OUTPUTFORMATTER& aFormatter );
    void writeItems( OUTPUTFORMATTER& aFormatter );

    SIM_LIB_MGR             m_libMgr;             ///< Holds libraries and models
    NAME_GENERATOR          m_modelNameGenerator; ///< Generates unique model names

    std::vector<wxString>   m_directives;         ///< Spice directives found in the schematic sheet
    std::set<wxString>      m_rawIncludes;        ///< include directives found in symbols
    std::set<wxString>      m_nets;

    ///< Items representing schematic symbols in Spice world.
    std::list<SPICE_ITEM>   m_items;
};


#endif // NETLIST_EXPORTER_SPICE_H
