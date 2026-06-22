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

#ifndef QA_DRC_BENCHMARK_CORPUS_H
#define QA_DRC_BENCHMARK_CORPUS_H

#include <vector>

#include <wx/string.h>

#include <drc/drc_rule.h>


/**
 * One board+rules pairing from the out-of-tree corpus manifest. Paths are stored
 * absolute after resolution against the corpus root so downstream code never has to
 * carry the root around.
 */
struct CORPUS_ENTRY
{
    wxString board;     ///< Absolute path to the .kicad_pcb.
    wxString rules;     ///< Absolute path to the .kicad_dru, or empty for none.
    wxString tier;      ///< Free-form tier tag from the manifest (A/B/C).
    wxString source;    ///< Provenance string for traceability.
    wxString notes;     ///< Free-form annotation.
    bool     quick = false;   ///< Part of the fast iteration set selected by --quick.
};


/**
 * Read and resolve the corpus manifest pointed at by KICAD_DRC_BENCH_CORPUS.
 *
 * The env var names a directory containing corpus.json, an array of objects with
 * board/rules/tier/source/notes keys whose board and rules paths are relative to that
 * directory. A missing or unset env var is not an error; the loader simply reports an
 * empty result so the tool can print a skip and exit cleanly, matching the QA rule that
 * absent local corpora must never hard-fail.
 */
class CORPUS
{
public:
    /// True when KICAD_DRC_BENCH_CORPUS is set and names an existing directory.
    static bool IsConfigured();

    /// The resolved corpus root, or an empty string when unconfigured.
    static wxString Root();

    /**
     * Parse <root>/corpus.json into resolved entries. Returns false and fills aError on a
     * malformed or missing manifest. An unconfigured corpus returns true with no entries so
     * callers can treat it as a graceful skip.
     */
    static bool Load( std::vector<CORPUS_ENTRY>& aEntries, wxString& aError );
};


/// Human-readable token for a DRC_CONSTRAINT_T, matching the .kicad_dru keyword where one exists.
const char* ConstraintTypeName( DRC_CONSTRAINT_T aType );


/// Every DRC_CONSTRAINT_T the engine can carry rules for, in enum order, excluding NULL_CONSTRAINT.
const std::vector<DRC_CONSTRAINT_T>& AllConstraintTypes();


/// Every pcbexpr predicate registered in pcbexpr_functions.cpp, used for textual coverage scans.
const std::vector<wxString>& AllPredicateNames();


/**
 * Scan raw .kicad_dru text for occurrences of each registered predicate name. A predicate is
 * counted present when its name appears followed by an open paren, which is how the expression
 * grammar invokes it. Returns the set of predicate names found.
 */
std::vector<wxString> ScanPredicatesInRules( const wxString& aRulesText );


#endif // QA_DRC_BENCHMARK_CORPUS_H
