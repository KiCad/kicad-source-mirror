/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/string.h>

#include <kicommon.h>
#include <map>
#include <settings/json_settings.h>
#include <lseq.h>
#include <lset.h>
#include <title_block.h>

class PROJECT;

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


template <typename ListElementType>
class JOB_PARAM_LIST : public JOB_PARAM_BASE
{
public:
    JOB_PARAM_LIST( const std::string& aJsonPath, std::vector<ListElementType>* aPtr,
                    std::vector<ListElementType> aDefault ) :
            JOB_PARAM_BASE( aJsonPath ),
            m_ptr( aPtr ),
            m_default( std::move( aDefault ) )
    { }

    virtual void FromJson( const nlohmann::json& j ) const override
    {
        if( j.contains( m_jsonPath ) )
        {
            auto js = j.at( m_jsonPath );
            std::vector<ListElementType> val;

            if( js.is_array() )
            {
                for( const auto& el : js.items() )
                    val.push_back( el.value().template get<ListElementType>() );
            }

            *m_ptr = val;
        }
        else
            *m_ptr = m_default;
    }

    void ToJson( nlohmann::json& j ) override
    {
        nlohmann::json js = nlohmann::json::array();

        for( const auto& el : *m_ptr )
            js.push_back( el );

        j[m_jsonPath] = js;
    }

protected:
    std::vector<ListElementType>* m_ptr;
    std::vector<ListElementType>  m_default;
};


class JOB_PARAM_LSEQ : public JOB_PARAM<LSEQ>
{
public:
    JOB_PARAM_LSEQ( const std::string& aJsonPath, LSEQ* aPtr, LSEQ aDefault ) :
            JOB_PARAM<LSEQ>( aJsonPath, aPtr, aDefault )
    {
    }

    virtual void FromJson( const nlohmann::json& j ) const override
    {
        if( j.contains( m_jsonPath ) )
        {
            auto js = j.at( m_jsonPath );

            LSEQ layers;
            if( js.is_array() )
            {
                for( const nlohmann::json& layer : js )
                {
                    if( layer.is_string() )
                    {
                        wxString name = layer.get<wxString>();
                        int      layerId = LSET::NameToLayer( name );
                        if( layerId != UNDEFINED_LAYER )
                            layers.push_back( static_cast<PCB_LAYER_ID>( layerId ) );
                    }
                    else
                    {
                        int layerId = layer.get<int>();
                        if( layerId != UNDEFINED_LAYER )
                            layers.push_back( static_cast<PCB_LAYER_ID>( layerId ) );
                    }
                }
            }
            *m_ptr = layers;
        }
        else
            *m_ptr = m_default;
    }

    void ToJson( nlohmann::json& j ) override
    {
        nlohmann::json js = nlohmann::json::array();

        for( PCB_LAYER_ID layer : ( *m_ptr ) )
            js.push_back( LSET::Name( layer ) );

        j[m_jsonPath] = js;
    }
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

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    JOB( const JOB& ) = delete;
    JOB& operator=( const JOB& ) = delete;

    const std::string& GetType() const { return m_type; };

    const std::map<wxString, wxString>& GetVarOverrides() const { return m_varOverrides; }

    void SetVarOverrides( const std::map<wxString, wxString>& aVarOverrides )
    {
        m_varOverrides = aVarOverrides;
    }

    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) { m_titleBlock = aTitleBlock; }

    virtual void FromJson( const nlohmann::json& j );
    virtual void ToJson( nlohmann::json& j ) const;

    virtual wxString GetDefaultDescription() const;
    virtual wxString GetSettingsDialogTitle() const;

    const std::vector<JOB_PARAM_BASE*>& GetParams() { return m_params; }

    void ClearExistingOutputs()                 { m_outputs.clear(); }
    const std::vector<JOB_OUTPUT>& GetOutputs() { return m_outputs; }
    void AddOutput( wxString aOutputPath )      { m_outputs.emplace_back( aOutputPath ); }

    /**
     * Sets the temporary output directory for the job, this is used to prefix with a given
     * output path when GetFullOutputPath is called. This is intended for use with running jobsets
     * and otherwise has no impact on individual job runs outside jobsets.
     */
    void SetTempOutputDirectory( const wxString& aBase );

    /**
     * Sets the configured output path for the job, this path is always saved to file
     */
    void SetConfiguredOutputPath( const wxString& aPath );

    /**
     * Returns the configured output path for the job
     */
    wxString GetConfiguredOutputPath() const { return m_outputPath; }

    /**
     * Sets a transient output path for the job, it takes priority over the configured output path
     * when GetFullOutputPath is called.
     */
    void     SetWorkingOutputPath( const wxString& aPath ) { m_workingOutputPath = aPath; }

    /**
     * Returns the working output path for the job, if one has been set
     */
    wxString GetWorkingOutputPath() const { return m_workingOutputPath; }

    /**
     * Returns the full output path for the job, taking into account the configured output path,
     * any configured working path and the temporary output directory.
     *
     * Additionally variable resolution will take place
     */
    wxString GetFullOutputPath( PROJECT* aProject ) const;

    wxString ResolveOutputPath( const wxString& aPath, bool aPathIsDirectory, PROJECT* aProject ) const;

    bool GetOutputPathIsDirectory() const { return m_outputPathIsDirectory; }

protected:
    std::string                  m_type;
    std::map<wxString, wxString> m_varOverrides;
    TITLE_BLOCK                  m_titleBlock;

    wxString m_tempOutputDirectory;

    wxString m_outputPath;
    bool     m_outputPathIsDirectory;
    wxString m_description;

    /**
     * The working output path is a transient path that takes priority over the configured
     * output path when determining where to write output files.
     */
    wxString m_workingOutputPath;

    std::vector<JOB_PARAM_BASE*> m_params;

    std::vector<JOB_OUTPUT> m_outputs;
};

KICOMMON_API void from_json( const nlohmann::json& j, JOB& f );
KICOMMON_API void to_json( nlohmann::json& j, const JOB& f );
