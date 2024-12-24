/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <nlohmann/json.hpp>

#include <settings/parameters.h>
#include <wildcards_and_files_ext.h>

#include <jobs/jobset.h>
#include <jobs/job_registry.h>
#include <jobs/jobs_output_folder.h>
#include <jobs/jobs_output_archive.h>
#include <kiid.h>
#include <reporter.h>

#include <algorithm>

const int jobsFileSchemaVersion = 1;

NLOHMANN_JSON_SERIALIZE_ENUM( JOBSET_OUTPUT_TYPE,
                              {
                                      { JOBSET_OUTPUT_TYPE::FOLDER, "folder" },
                                      { JOBSET_OUTPUT_TYPE::ARCHIVE, "archive" }
                              } )

KICOMMON_API void to_json( nlohmann::json& j, const JOBSET_JOB& f )
{
    j = nlohmann::json{ { "id", f.m_id },
                        { "type", f.m_type },
                        { "settings", nlohmann::json::object( {} ) }
                      };

    f.m_job->ToJson( j.at( "settings" ) );
}


KICOMMON_API void from_json( const nlohmann::json& j, JOBSET_JOB& f )
{
    j.at( "type" ).get_to( f.m_type );
    j.at( "id" ).get_to( f.m_id );

    nlohmann::json settings_obj = j.at( "settings" );

    f.m_job.reset( JOB_REGISTRY::CreateInstance<JOB>( f.m_type ) );

    if( f.m_job != nullptr )
    {
        f.m_job->FromJson( settings_obj );
    }
}


KICOMMON_API void to_json( nlohmann::json& j, const JOBSET_OUTPUT& f )
{
    j = nlohmann::json{ { "type", f.m_type }, { "settings", nlohmann::json::object( {} ) } };

    f.m_outputHandler->ToJson( j.at( "settings" ) );
}


KICOMMON_API void from_json( const nlohmann::json& j, JOBSET_OUTPUT& f )
{
    j.at( "type" ).get_to( f.m_type );
    f.m_only = j.value( "only", std::vector<wxString>() );

    nlohmann::json settings_obj = j.at( "settings" );

    f.InitOutputHandler();

    if( f.m_outputHandler != nullptr )
    {
        f.m_outputHandler->FromJson( settings_obj );
    }
}


JOBSET_OUTPUT::JOBSET_OUTPUT() :
        m_type( JOBSET_OUTPUT_TYPE::FOLDER ),
        m_outputHandler( nullptr ),
        m_lastRunSuccess(),
        m_lastRunReporters()
{
}


JOBSET_OUTPUT::JOBSET_OUTPUT( wxString id, JOBSET_OUTPUT_TYPE type ) :
        m_id( id ),
        m_type( type ),
        m_outputHandler( nullptr ),
        m_lastRunSuccess(),
        m_lastRunReporters()
{
    InitOutputHandler();
}


JOBSET_OUTPUT::~JOBSET_OUTPUT()
{
    for( auto& reporter : m_lastRunReporters )
    {
        delete reporter.second;
    }
    m_lastRunReporters.clear();
}


void JOBSET_OUTPUT::InitOutputHandler()
{
    if( m_type == JOBSET_OUTPUT_TYPE::FOLDER )
    {
        m_outputHandler = new JOBS_OUTPUT_FOLDER();
    }
    else if( m_type == JOBSET_OUTPUT_TYPE::ARCHIVE )
    {
        m_outputHandler = new JOBS_OUTPUT_ARCHIVE();
    }
}


bool JOBSET_JOB::operator==( const JOBSET_JOB & rhs ) const
{
    return rhs.m_type == m_type;
}


bool JOBSET_OUTPUT::operator==( const JOBSET_OUTPUT& rhs ) const
{
    return rhs.m_type == m_type;
}


JOBSET::JOBSET( const wxString& aFilename ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::NONE, jobsFileSchemaVersion ),
        m_dirty( false )
{
    m_params.emplace_back( new PARAM_LIST<JOBSET_JOB>( "jobs", &m_jobs, {} ) );
    m_params.emplace_back( new PARAM_LIST<JOBSET_OUTPUT>( "outputs", &m_outputs, {} ) );

    m_fileNameWithoutPath = wxFileName( aFilename ).GetFullName();
}


wxString JOBSET::getFileExt() const
{
    return FILEEXT::KiCadJobSetFileExtension;
}


void JOBSET::AddNewJob( wxString aType, JOB* aJob )
{
    m_jobs.emplace_back( KIID().AsString(), aType, aJob );
    SetDirty();
}


JOBSET_OUTPUT* JOBSET::AddNewJobOutput( JOBSET_OUTPUT_TYPE aType )
{
    m_outputs.emplace_back( KIID().AsString(), aType);
    SetDirty();

    return &m_outputs.back();
}


void JOBSET::RemoveOutput( JOBSET_OUTPUT* aOutput )
{
    std::erase_if( m_outputs,
                   [&]( JOBSET_OUTPUT const& output )
                   {
                       return output.m_id == aOutput->m_id;
                   } );
}


void JOBSET::MoveJobUp( size_t aJobIdx )
{
    if( aJobIdx > 0 )
    {
        std::swap( m_jobs[aJobIdx], m_jobs[aJobIdx - 1] );
        SetDirty();
    }
}


void JOBSET::MoveJobDown( size_t aJobIdx )
{
    if( aJobIdx < m_jobs.size() - 1 )
    {
        std::swap( m_jobs[aJobIdx], m_jobs[aJobIdx + 1] );
        SetDirty();
    }
}


void JOBSET::RemoveJob( size_t aJobIdx )
{
	m_jobs.erase( m_jobs.begin() + aJobIdx );
	SetDirty();
}


bool JOBSET::SaveToFile( const wxString& aDirectory, bool aForce )
{
    bool success = JSON_SETTINGS::SaveToFile( aDirectory, aForce );
    if( success )
    {
        m_dirty = false;
    }

    return success;
}


JOBSET_OUTPUT* JOBSET::GetOutput( wxString& aOutput )
{
    auto it = std::find_if( m_outputs.begin(), m_outputs.end(),
                            [&]( const JOBSET_OUTPUT& output )
                            {
                                if( output.m_id == aOutput )
                                    return true;

                                return false;
                            } );

    if( it != m_outputs.end() )
        return &(*it);

    return nullptr;
}


std::vector<JOBSET_JOB> JOBSET::GetJobsForOutput( JOBSET_OUTPUT* aOutput )
{
    wxASSERT( aOutput != nullptr );

    if( aOutput->m_only.size() == 0 )
    {
        return m_jobs;
    }

    std::vector<JOBSET_JOB> result;
    for( wxString& onlyId : aOutput->m_only )
    {
        auto it = std::find_if( m_jobs.begin(), m_jobs.end(),
                                [&]( const JOBSET_JOB& job )
                                {
                                    if( job.m_id == onlyId )
                                        return true;

                                    return false;
                                } );

        if( it != m_jobs.end() )
            result.push_back( *it );
    }

    return result;
}


#if !defined( __MINGW32__ )
template class KICOMMON_API PARAM_LIST<JOBSET_JOB>;
template class KICOMMON_API PARAM_LIST<JOBSET_OUTPUT>;
#endif