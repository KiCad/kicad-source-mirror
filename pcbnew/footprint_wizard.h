/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FOOTPRINT_WIZARD_H
#define FOOTPRINT_WIZARD_H

#include <optional>
#include <tl/expected.hpp>

#include <import_export.h>
#include <api/common/types/wizards.pb.h>

class FOOTPRINT;



// Wrapper for WizardMetaInfo protobuf with UTF8-converted strings
struct WIZARD_META_INFO
{
    wxString identifier;
    wxString name;
    wxString description;
    std::set<kiapi::common::types::WizardContentType> types_generated;

    void FromProto( const kiapi::common::types::WizardMetaInfo& aProto );
};

class WIZARD_PARAMETER
{
public:
    WIZARD_PARAMETER() = default;
    virtual ~WIZARD_PARAMETER() = default;

    virtual void Reset() = 0;

    /**
     * Packs the current state of this parameter back into a protobuf message
     * @param aCompact will only include the identifier and value if true
     * @return a protobuf message describing this parameter
     */
    virtual kiapi::common::types::WizardParameter Pack( bool aCompact = true );

    wxString identifier;
    wxString name;
    wxString description;
    kiapi::common::types::WizardParameterCategory category = kiapi::common::types::WPC_UNKNOWN;
    kiapi::common::types::WizardParameterDataType type = kiapi::common::types::WPDT_UNKNOWN;

    static std::unique_ptr<WIZARD_PARAMETER> Create( const kiapi::common::types::WizardParameter& aProto );

    static wxString ParameterCategoryName( kiapi::common::types::WizardParameterCategory aCategory );
};

class WIZARD_INT_PARAMETER : public WIZARD_PARAMETER
{
public:
    void Reset() override { value = default_value; }
    kiapi::common::types::WizardParameter Pack( bool aCompact = true ) override;

    int value = 0;
    int default_value = 0;
    std::optional<int> min;
    std::optional<int> max;
    std::optional<int> multiple;

    void FromProto( const kiapi::common::types::WizardIntParameter& aProto );
};

class WIZARD_REAL_PARAMETER : public WIZARD_PARAMETER
{
public:
    void Reset() override { value = default_value; }
    kiapi::common::types::WizardParameter Pack( bool aCompact = true ) override;

    double value = 0.0;
    double default_value = 0.0;
    std::optional<double> min;
    std::optional<double> max;

    void FromProto( const kiapi::common::types::WizardRealParameter& aProto );
};

class WIZARD_BOOL_PARAMETER : public WIZARD_PARAMETER
{
public:
    void Reset() override { value = default_value; }
    kiapi::common::types::WizardParameter Pack( bool aCompact = true ) override;

    bool value = false;
    bool default_value = false;

    void FromProto( const kiapi::common::types::WizardBoolParameter& aProto );
};

class WIZARD_STRING_PARAMETER : public WIZARD_PARAMETER
{
public:
    void Reset() override { value = default_value; }
    kiapi::common::types::WizardParameter Pack( bool aCompact = true ) override;

    wxString value;
    wxString default_value;
    std::optional<wxString> validation_regex;

    void FromProto( const kiapi::common::types::WizardStringParameter& aProto );
};

// Wrapper for WizardInfo protobuf
struct WIZARD_INFO
{
    WIZARD_META_INFO meta;
    std::vector<std::unique_ptr<WIZARD_PARAMETER>> parameters;

    void FromProto( const kiapi::common::types::WizardInfo& aProto );
};


class FOOTPRINT_WIZARD
{
public:
    FOOTPRINT_WIZARD() {}
    ~FOOTPRINT_WIZARD() = default;

    WIZARD_INFO& Info() { return m_info; }
    const WIZARD_INFO& Info() const { return m_info; }

    const wxString& Identifier() const { return m_identifier; }
    void SetIdentifier( const wxString& aId ) { m_identifier = aId; }

    void ResetParameters();

private:
    WIZARD_INFO m_info;

    // Identifier of the plugin action
    wxString m_identifier;
};

/**
 * The footprint wizard manager interfaces with API_PLUGINs that can generate footprints.
 * It uses API_PLUGIN_MANAGER to enumerate the loaded wizard plugins, and filters to those that
 * generate footprints.  It then handles calling the plugin to query capabilities, generate
 * footprints, and so on.
 */
class FOOTPRINT_WIZARD_MANAGER
{
public:
    FOOTPRINT_WIZARD_MANAGER() {}
    ~FOOTPRINT_WIZARD_MANAGER() = default;

    /**
     * Goes through the list of IPC API plugins that provide wizard actions and
     * attempts to refresh the info of each one, placing the ones that work in to
     * the internal list returned by Wizards().  Note that doing so clears the existing
     * list and invalidates any existing pointers to wizards.
     */
    void ReloadWizards();

    std::optional<FOOTPRINT_WIZARD*> GetWizard( const wxString& aIdentifier );

    std::vector<FOOTPRINT_WIZARD*> Wizards() const;

    /**
     * Runs a wizard plugin with the --get-info argument, which should result in the plugin
     * dumping a WizardInfo protobuf message in JSON format to stdout.
     * @return true if the call succeeded
     */
    static bool RefreshInfo( FOOTPRINT_WIZARD* aWizard );

    /**
     * Generates a footprint using a given wizard
     * @param aWizard is the wizard data instance (with parameters set) to feed into the wizard
     * @return a generated footprint, or an error message if generation failed
     */
    tl::expected<FOOTPRINT*, wxString> Generate( FOOTPRINT_WIZARD* aWizard );

private:

    // Loaded wizards, mapped by identifier
    std::map<wxString, std::unique_ptr<FOOTPRINT_WIZARD>> m_wizards;
};


#endif /* FOOTPRINT_WIZARD_H */
