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
    /// Output JSON: {"matches": ["lib:footprint1", "lib:footprint2", ...], "limited": bool, "success": bool}
    KIFACE_FILTER_FOOTPRINTS,

    /// Function pointer type: bool (*)()
    /// Starts loading the footprint libraries in the background. Never blocks.
    /// Returns true if a load was started, false if a load is already running, or if something failed.
    KIFACE_TRIGGER_FOOTPRINTS_LOAD,

    /// Function pointer type: float (*)()
    /// Returns how far the library loader has gotten as a fraction of the queued libraries
    /// processed so far, or 1.0 when there is nothing to wait for. Never blocks.
    KIFACE_FOOTPRINTS_LOAD_PROGRESS,

    /// Function pointer type: wxString (*)(const wxString& aQueryJson)
    /// Input JSON: the object written by FOOTPRINT_USERS_QUERY::ToJsonStr()
    /// Output JSON: the object written by FOOTPRINT_USERS_RESULT::ToJsonStr()
    KIFACE_FILTER_FOOTPRINT_USERS,

    /// Function pointer type: bool (*)()
    /// Starts loading the symbol libraries in the background. Never blocks.
    /// Returns true if a load was started, false if a load is already running, or if something failed.
    KIFACE_TRIGGER_SYMBOLS_LOAD,

    /// Function pointer type: float (*)()
    /// Returns how far the library loader has gotten as a fraction of the queued libraries
    /// processed so far, or 1.0 when there is nothing to wait for. Never blocks.
    KIFACE_SYMBOLS_LOAD_PROGRESS,

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

    /// KICAD_DIFF::DIFF_CANVAS_CONTEXT* (*)( int aKind, const wxString& aFileA, const wxString& aFileB,
    ///          WIDGET_DIFF_CANVAS* aBefore, WIDGET_DIFF_CANVAS* aAfter,
    ///          KICAD_DIFF::DOCUMENT_DIFF* aDiff, REPORTER* aReporter )
    /// aKind is a KICAD_DIFF::DOC_KIND. Loads the two files, fills aDiff, and draws the first
    /// file in aBefore and the second in aAfter with the kiface's own painter. Returns what
    /// keeps the documents alive while the canvases show them, or null on failure.
    KIFACE_ATTACH_DIFF_CANVASES,

    KIFACE_TEST_FOOTPRINT_LINK,
    KIFACE_TEST_FOOTPRINT_LINK_NO_LIBRARY,
    KIFACE_TEST_FOOTPRINT_LINK_LIBRARY_NOT_ENABLED,
    KIFACE_TEST_FOOTPRINT_LINK_NO_FOOTPRINT,

    /// Function pointer type: void (*)( const wxString& aFootprint, PROJECT* aProject,
    ///                                  std::set<wxString>& aPadNumbers )
    /// Fills aPadNumbers with the numbered pads of aFootprint.
    KIFACE_FOOTPRINT_PAD_NUMBERS,

    /// bool (*)( const std::string& aBefore, const std::string& aAfter, KICAD_DIFF::DOCUMENT_DIFF* aDiff )
    /// Names the symbols two versions of a symbol library file's text differ in, safe off the GUI thread.
    /// False when either side does not parse.
    KIFACE_DIFF_SYMBOL_LIBRARY_TEXT,

    /// std::optional<std::string> (*)( const wxString& aFile, PROJECT& aProject, PROGRESS_REPORTER* aReporter )
    /// The parts placed in the schematic or board saved at aFile, as a JSON array for Release Design, with the
    /// load's progress shown on aReporter when given. Nothing when the file does not load.
    KIFACE_COLLECT_PLACED_PARTS,

    /// std::optional<std::string> (*)( const std::string& aText )
    /// The text of a symbol library file in any format the kiface reads, as its current format saves it.
    /// Nothing when the text does not parse.
    KIFACE_CURRENT_SYMBOL_FORMAT,

    /// bool (*)( const std::vector<std::string>& aBefore, const std::vector<std::string>& aAfter,
    ///           KICAD_DIFF::DOCUMENT_DIFF* aDiff )
    /// KIFACE_DIFF_SYMBOL_LIBRARY_TEXT for a library held in several files, each read in its own format, with
    /// parents found in any of them. False when either side does not parse.
    KIFACE_DIFF_SYMBOL_LIBRARY_FILES
};

#endif // KIFACE_IDS
