/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#pragma once

#include <map>
#include <vector>

#include <sim/sim_library.h>
#include <sim/sim_model.h>
#include <sim/sim_model_input.h>

class EMBEDDED_FILES;
class PROJECT;
class SCH_SYMBOL;


class SIM_LIB_MGR
{
public:
    SIM_LIB_MGR( const PROJECT* aPrj );
    virtual ~SIM_LIB_MGR() = default;

    void SetForceFullParse() { m_forceFullParse = true; }

    void Clear();

    void SetFilesStack( std::vector<EMBEDDED_FILES*> aFilesStack )
    {
        m_embeddedFilesStack = std::move( aFilesStack );
    }

    void SetLibrary( const wxString& aLibraryPath, REPORTER& aReporter );

    SIM_MODEL& CreateModel( SIM_MODEL::TYPE aType, std::span<const wxString> aPins,
                            REPORTER& aReporter );

    SIM_MODEL& CreateModel( const SIM_MODEL* aBaseModel, std::span<const wxString> aPins,
                            REPORTER& aReporter );

    SIM_MODEL& CreateModel( const SIM_MODEL* aBaseModel, std::span<const wxString> aPins,
                            const std::vector<SCH_FIELD>& aFields, bool aResolve, int aDepth,
                            REPORTER& aReporter );

    /** Resolve instance fields once; editors use the field-based overloads for unresolved input. */
    static SIM_MODEL_INPUT CaptureModelInput( const SCH_SHEET_PATH* aSheetPath, const SCH_SYMBOL& aSymbol,
                                              int aDepth, const wxString& aVariantName,
                                              const wxString& aMergedSimPins = wxEmptyString );

    /** Raw-SPICE fallback applies to field-defined models; library parsing keeps its own policy. */
    SIM_LIBRARY::MODEL CreateModel( const SIM_MODEL_INPUT& aInput, bool aAllowRawFallback,
                                    REPORTER& aReporter );

    // aMergedSimPins is an optional merged Sim.Pins string from all units of a multi-unit symbol.
    // If provided (non-empty), it will be used instead of the symbol's Sim.Pins field.
    SIM_LIBRARY::MODEL CreateModel( const SCH_SHEET_PATH* aSheetPath, const SCH_SYMBOL& aSymbol,
                                    bool aResolve, int aDepth, const wxString& aVariantName,
                                    REPORTER& aReporter, const wxString& aMergedSimPins = wxEmptyString );

    SIM_LIBRARY::MODEL CreateModel( const std::vector<SCH_FIELD>& aFields, bool aResolve, int aDepth,
                                    std::span<const wxString> aPins, REPORTER& aReporter );

    SIM_LIBRARY::MODEL CreateModel( const wxString& aLibraryPath, const std::string& aBaseModelName,
                                    const std::vector<SCH_FIELD>& aFields, bool aResolve, int aDepth,
                                    std::span<const wxString> aPins, REPORTER& aReporter );

    void SetModel( int aIndex, std::unique_ptr<SIM_MODEL> aModel );

    std::map<wxString, std::reference_wrapper<const SIM_LIBRARY>> GetLibraries() const;
    std::vector<std::reference_wrapper<SIM_MODEL>> GetModels() const;

    wxString ResolveLibraryPath( const wxString& aLibraryPath, REPORTER& aReporter );
    wxString ResolveEmbeddedLibraryPath( const wxString& aLibPath, const wxString& aRelativeLib,
                                         REPORTER& aReporter );

private:
    SIM_LIBRARY::MODEL createModel( const std::vector<SCH_FIELD>& aFields, bool aResolve, int aDepth,
                                    std::span<const wxString> aPins, REPORTER& aReporter,
                                    bool aAllowRawFallback );

    std::vector<EMBEDDED_FILES*>                     m_embeddedFilesStack;  // no ownership
    const PROJECT*                                   m_project;             // no ownership
    bool                                             m_forceFullParse;
    std::map<wxString, std::unique_ptr<SIM_LIBRARY>> m_libraries;
    std::vector<std::unique_ptr<SIM_MODEL>>          m_models;
};

