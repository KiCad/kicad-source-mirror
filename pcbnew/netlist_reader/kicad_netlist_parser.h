/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <netlist_lexer.h>
#include <netlist_reader/pcb_netlist.h>

/**
 * The parser for reading the KiCad s-expression netlist format.
 */
class KICAD_NETLIST_PARSER : public NETLIST_LEXER
{
public:
    KICAD_NETLIST_PARSER( LINE_READER* aReader, NETLIST* aNetlist );

    /**
     * Function Parse
     * parse the full netlist
     */
    void Parse();

    // Useful for debug only:
    const char* getTokenName( NL_T::T aTok ) { return NETLIST_LEXER::TokenName( aTok ); }

private:
    /**
     * Skip the current token level, i.e
     * search for the RIGHT parenthesis which closes the current description.
     */
    void skipCurrent();

    /**
     * Parse a component description:
     * (comp (ref P1)
     *   (value DB25FEMELLE)
     *   (footprint DB25FC)
     *   (libsource (lib conn) (part DB25))
     *   (property (name PINCOUNT) (value 25))
     *   (sheetpath (names /) (tstamps /))
     *   (tstamp 3256759C))
     */
    void parseComponent();

    /**
     * Parse a net section
     * (net (code 20) (name /PC-A0)
     *   (node (ref BUS1) (pin 62))
     *   (node (ref U3) (pin 3))
     *   (node (ref U9) (pin M6)))
     *
     * and set the corresponding pads netnames
     */
    void parseNet();

    /**
     * Parse a group section
     *   (group (name "GroupName")
     *      (member (uuid "..."))))
     *
     * and set the corresponding pads netnames
     */
    void parseGroup();

    /**
     * Parse a variant section
     *   (variant (name "VariantName") (description "Description"))
     *
     * and add it to the netlist variant registry
     */
    void parseVariant();

    /**
     * Read the section "libparts" in the netlist:
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
     *  And add the strings giving the footprint filter (subsection footprints) of the
     *  corresponding footprint info
     *  <p>This section is used by CvPcb, and is not useful in Pcbnew, therefore it it not
     *  always read </p>
     */
    void parseLibPartList();

    NL_T::T      token;
    LINE_READER* m_lineReader; ///< The line reader used to parse the netlist.  Not owned.
    NETLIST*     m_netlist;    ///< The netlist to parse into.  Not owned.
};
