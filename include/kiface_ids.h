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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KIFACE_IDS_H
#define KIFACE_IDS_H

/**
 * IDs of objects that may be returned by KIFACE::IfaceOrAddress.
 */
enum KIFACE_ADDR_ID : int
{
    KIFACE_ID_INVALID,

    KIFACE_FOOTPRINT_LIBRARY_ADAPTER,

    /// Function pointer type: wxString (*)(const wxString& aFilterJson)
    /// Input JSON: {"pin_count": N, "filters": ["pattern1", ...], "zero_filters": bool, "max_results": N}
    /// Output JSON: ["lib:footprint1", "lib:footprint2", ...]
    KIFACE_FILTER_FOOTPRINTS,

    KIFACE_LOAD_SCHEMATIC,
    KIFACE_NETLIST_SCHEMATIC,

    /// int (*)( int aKind, const wxString& aAncestor, const wxString& aOurs,
    ///          const wxString& aTheirs, const wxString& aOutput, bool aInteractive,
    ///          REPORTER* aReporter )
    /// aKind is a KICAD_DIFF::DOC_KIND. Runs a 3-way document/library merge in
    /// the kiface and returns a CLI exit code. Replaces the former merge JOBs.
    KIFACE_MERGE_DOCUMENT,

    /// int (*)( int aKind, const wxString& aFileA, const wxString& aFileB,
    ///          const wxString& aLabelA, const wxString& aLabelB, wxWindow* aParent,
    ///          REPORTER* aReporter )
    /// aKind is a KICAD_DIFF::DOC_KIND. Loads the two files and opens
    /// DIALOG_KICAD_DIFF modally. Replaces the former JOB_OPEN_DIFF_DIALOG.
    KIFACE_OPEN_DIFF_DIALOG,

    KIFACE_TEST_FOOTPRINT_LINK,
    KIFACE_TEST_FOOTPRINT_LINK_NO_LIBRARY,
    KIFACE_TEST_FOOTPRINT_LINK_LIBRARY_NOT_ENABLED,
    KIFACE_TEST_FOOTPRINT_LINK_NO_FOOTPRINT,

    /// Function pointer type: void (*)( const wxString& aFootprint, PROJECT* aProject,
    ///                                  std::set<wxString>& aPadNumbers )
    /// Fills aPadNumbers with the numbered pads of aFootprint.
    KIFACE_FOOTPRINT_PAD_NUMBERS
};

#endif // KIFACE_IDS
