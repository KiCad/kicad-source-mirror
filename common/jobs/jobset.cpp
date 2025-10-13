/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include <i18n_utility.h>
#include <json_common.h>

#include <settings/parameters.h>
#include <wildcards_and_files_ext.h>

#include <jobs/jobset.h>
#include <jobs/job_registry.h>
#include <jobs/jobs_output_folder.h>
#include <jobs/jobs_output_archive.h>
#include <kiid.h>
#include <reporter.h>
#include <wx/filename.h>

#include <algorithm>
#include <memory>

const int jobsFileSchemaVersion = 1;


KICOMMON_API std::map<JOBSET_DESTINATION_T, JOBSET_DESTINATION_T_INFO> JobsetDestinationTypeInfos =
{
    { JOBSET_DESTINATION_T::FOLDER,
        { _HKI( "Folder" ), BITMAPS::small_folder, true, "" } },
    { JOBSET_DESTINATION_T::ARCHIVE,
        { _HKI( "Archive" ), BITMAPS::zip, false, FILEEXT::ZipFileWildcard() } },
};


NLOHMANN_JSON_SERIALIZE_ENUM( JOBSET_DESTINATION_T,
                              {
                                      { JOBSET_DESTINATION_T::FOLDER, "folder" },
                                      { JOBSET_DESTINATION_T::ARCHIVE, "archive" }
                              } )

KICOMMON_API void to_json( nlohmann::json& j, const JOBSET_JOB& f )
{
    j = nlohmann::json{ { "id", f.m_id },
                        { "type", f.m_type },
                        { "description", f.m_description },
                        { "settings", nlohmann::json::object( {} ) }
                      };

    f.m_job->ToJson( j.at( "settings" ) );
}


KICOMMON_API void from_json( const nlohmann::json& j, JOBSET_JOB& f )
{
    j.at( "type" ).get_to( f.m_type );
    j.at( "id" ).get_to( f.m_id );
    f.m_description = j.value( "description", "" );

    nlohmann::json settings_obj = j.at( "settings" );

    f.m_job.reset( JOB_REGISTRY::CreateInstance<JOB>( f.m_type ) );

    if( f.m_job != nullptr )
    {
        f.m_job->FromJson( settings_obj );
    }
}


KICOMMON_API void to_json( nlohmann::json& j, const JOBSET_DESTINATION& destination )
{
    j = nlohmann::json{ { "id", destination.m_id },
                        { "type", destination.m_type },
                        { "only", destination.m_only },
                        { "description", destination.m_description },
                        { "settings", nlohmann::json::object( {} ) }
                        };

    destination.m_outputHandler->ToJson( j.at( "settings" ) );
}


KICOMMON_API void from_json( const nlohmann::json& j, JOBSET_DESTINATION& destination )
{
    // During 9.0 development outputs didn't get ids.
    if( j.contains( "id" ) )
        j.at( "id" ).get_to( destination.m_id );
    else
        destination.m_id = KIID().AsString();

    j.at( "type" ).get_to( destination.m_type );
    destination.m_only = j.value( "only", std::vector<wxString>() );
    destination.m_description = j.value( "description", "" );

    const nlohmann::json& settings_obj = j.at( "settings" );

    destination.InitOutputHandler();

    if( destination.m_outputHandler != nullptr )
        destination.m_outputHandler->FromJson( settings_obj );
}


JOBSET_DESTINATION::JOBSET_DESTINATION() :
        m_type( JOBSET_DESTINATION_T::FOLDER ),
        m_outputHandler( nullptr ),
        m_lastRunSuccess(),
        m_lastRunReporters()
{
}


JOBSET_DESTINATION::JOBSET_DESTINATION( const wxString& id, JOBSET_DESTINATION_T type ) :
        m_id( id ),
        m_type( type ),
        m_outputHandler( nullptr ),
        m_lastRunSuccess(),
        m_lastRunReporters()
{
    InitOutputHandler();
}


void JOBSET_DESTINATION::InitOutputHandler()
{
    if( m_type == JOBSET_DESTINATION_T::FOLDER )
    {
        m_outputHandler = std::make_shared<JOBS_OUTPUT_FOLDER>( );
    }
    else if( m_type == JOBSET_DESTINATION_T::ARCHIVE )
    {
        m_outputHandler = std::make_shared<JOBS_OUTPUT_ARCHIVE>( );
    }
}


wxString JOBSET_DESTINATION::GetDescription() const
{
    return m_description.IsEmpty() ? m_outputHandler->GetDefaultDescription() : m_description;
}


wxString JOBSET_DESTINATION::GetPathInfo() const
{
    return m_outputHandler->GetOutputPath();
}

void JOBSET_DESTINATION::SetDescription( const wxString& aDescription )
{
    if( aDescription == m_outputHandler->GetDefaultDescription() )
        m_description = wxEmptyString;
    else
        m_description = aDescription;
}


bool JOBSET_JOB::operator==( const JOBSET_JOB & rhs ) const
{
    return rhs.m_type == m_type;
}


wxString JOBSET_JOB::GetDescription() const
{
    return m_description.IsEmpty() ? m_job->GetDefaultDescription() : m_description;
}


void JOBSET_JOB::SetDescription( const wxString& aDescription )
{
    if( aDescription == m_job->GetDefaultDescription() )
        m_description = wxEmptyString;
    else
        m_description = aDescription;
}


bool JOBSET_DESTINATION::operator==( const JOBSET_DESTINATION& rhs ) const
{
    return rhs.m_type == m_type;
}


JOBSET::JOBSET( const wxString& aFilename ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::NONE, jobsFileSchemaVersion ),
        m_dirty( false )
{
    m_params.emplace_back( new PARAM_LIST<JOBSET_JOB>( "jobs", &m_jobs, {} ) );
    m_params.emplace_back( new PARAM_LIST<JOBSET_DESTINATION>( "outputs", &m_destinations, {} ) );

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


JOBSET_DESTINATION* JOBSET::AddNewDestination( JOBSET_DESTINATION_T aType )
{
    m_destinations.emplace_back( KIID().AsString(), aType );
    SetDirty();

    return &m_destinations.back();
}


void JOBSET::RemoveDestination( JOBSET_DESTINATION* aDestination )
{
    std::erase_if( m_destinations,
                   [&]( JOBSET_DESTINATION const& destination )
                   {
                       return destination.m_id == aDestination->m_id;
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


JOBSET_DESTINATION* JOBSET::FindDestination( wxString& aDestinationStr )
{
    auto is_matching_dest = [&]( const JOBSET_DESTINATION& destination )
    {
        if( destination.m_id == aDestinationStr || destination.m_description == aDestinationStr )
            return true;

        return false;
    };

    auto count = std::count_if( m_destinations.begin(), m_destinations.end(), is_matching_dest );

    // we want to intentionally fail if more than one matching dest exists
    // as theres no good way to handle it
    if( count != 1 )
        return nullptr;

    auto it = std::find_if( m_destinations.begin(), m_destinations.end(), is_matching_dest );

    if( it != m_destinations.end() )
        return &(*it);

    return nullptr;
}


std::vector<JOBSET_JOB> JOBSET::GetJobsForDestination( JOBSET_DESTINATION* aDestination )
{
    wxASSERT( aDestination != nullptr );

    if( aDestination->m_only.size() == 0 )
    {
        return m_jobs;
    }

    std::vector<JOBSET_JOB> result;
    for( wxString& onlyId : aDestination->m_only )
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
template class KICOMMON_API PARAM_LIST<JOBSET_DESTINATION>;
#endif