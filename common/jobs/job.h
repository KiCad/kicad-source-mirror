/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <kicommon.h>
#include <map>
#include <wx/string.h>
#include <settings/json_settings.h>

class KICOMMON_API JOB_PARAM_BASE
{
public:
    JOB_PARAM_BASE( const std::string& aJsonPath );

    virtual ~JOB_PARAM_BASE() = default;

    virtual void FromJson( const nlohmann::json& j ) const = 0;

    virtual void ToJson( nlohmann::json& j ) = 0;

protected:
    std::string m_jsonPath;
};

template <typename ValueType>
class JOB_PARAM : public JOB_PARAM_BASE
{
public:

    JOB_PARAM( const std::string& aJsonPath, ValueType* aPtr,
               ValueType aDefault ) :
            JOB_PARAM_BASE( aJsonPath ), m_ptr( aPtr ), m_default( aDefault )
    {
    }

    virtual void FromJson( const nlohmann::json& j ) const override
    {
        *m_ptr = j.value( m_jsonPath, m_default );
    }

    virtual void ToJson( nlohmann::json& j ) override { j[m_jsonPath] = *m_ptr; }

protected:
    ValueType* m_ptr;
    ValueType  m_default;
};

struct KICOMMON_API JOB_OUTPUT
{
    JOB_OUTPUT(){};

    JOB_OUTPUT( wxString outputPath ) { m_outputPath = outputPath; }

    wxString m_outputPath;
};

/**
 * An simple container class that lets us dispatch output jobs to kifaces
 */
class KICOMMON_API JOB
{
public:
    JOB( const std::string& aType, bool aOutputIsDirectory );

    virtual ~JOB();

    const std::string& GetType() const { return m_type; };

    const std::map<wxString, wxString>& GetVarOverrides() const { return m_varOverrides; }

    void SetVarOverrides( const std::map<wxString, wxString>& aVarOverrides )
    {
        m_varOverrides = aVarOverrides;
    }

    virtual void FromJson( const nlohmann::json& j );
    virtual void ToJson( nlohmann::json& j ) const;

    virtual wxString GetDescription();

    const std::vector<JOB_PARAM_BASE*>& GetParams() {
        return m_params;
    }

    void ClearExistingOutputs() {
        m_outputs.clear();
    }

    const std::vector<JOB_OUTPUT>& GetOutputs() {
        return m_outputs;
    }

    void AddOutput( wxString aOutputPath ) {
        m_outputs.emplace_back( aOutputPath );
    }

    void SetTempOutputDirectory( const wxString& aBase );


    void SetOutputPath( const wxString& aPath );
    wxString GetOutputPath() const { return m_outputPath; }
    wxString GetFullOutputPath() const;

    bool OutputPathFullSpecified() const;


protected:
    std::string m_type;
    std::map<wxString, wxString> m_varOverrides;

    wxString m_tempOutputDirectory;

    wxString m_outputPath;
    bool m_outputPathIsDirectory;

    std::vector<JOB_PARAM_BASE*> m_params;

    std::vector<JOB_OUTPUT> m_outputs;
};

KICOMMON_API void from_json( const nlohmann::json& j, JOB& f );
KICOMMON_API void to_json( nlohmann::json& j, const JOB& f );