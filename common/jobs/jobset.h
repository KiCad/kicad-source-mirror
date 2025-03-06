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

#ifndef JOBS_FILE_H
#define JOBS_FILE_H

#include <bitmaps/bitmaps_list.h>
#include <jobs/job.h>
#include <jobs/jobs_output.h>
#include <settings/json_settings.h>
#include <settings/parameters.h>
#include <ctime>
#include <memory>

class REPORTER;

struct KICOMMON_API JOBSET_JOB
{
    JOBSET_JOB() :
            m_job( nullptr )
    {}

    JOBSET_JOB( const wxString& id, const wxString& type, JOB* job ) :
            m_id( id ),
            m_type( type ),
            m_job( job )
    {}

    wxString             m_id;
    wxString             m_type;
    wxString             m_description;
    std::shared_ptr<JOB> m_job;

    wxString GetDescription() const;
    void     SetDescription( const wxString& aDescription );

    bool operator==( const JOBSET_JOB& rhs ) const;
};


enum class KICOMMON_API JOBSET_DESTINATION_T
{
    FOLDER,
    ARCHIVE
};

struct KICOMMON_API JOBSET_DESTINATION_T_INFO
{
    wxString    name;
    BITMAPS     bitmap;
    bool        outputPathIsFolder;
    wxString    fileWildcard;
};

extern KICOMMON_API
std::map<JOBSET_DESTINATION_T, JOBSET_DESTINATION_T_INFO> JobsetDestinationTypeInfos;


struct KICOMMON_API JOBSET_DESTINATION
{
    JOBSET_DESTINATION();

    JOBSET_DESTINATION( const wxString& id, JOBSET_DESTINATION_T type );

    ~JOBSET_DESTINATION();

    void                  InitOutputHandler();

    wxString              m_id;
    JOBSET_DESTINATION_T  m_type;
    wxString              m_description;
    JOBS_OUTPUT_HANDLER*  m_outputHandler;
    std::vector<wxString> m_only;

    wxString GetDescription() const;
    void SetDescription( const wxString& aDescription );

    ///< Transient property, not stored for now
    std::optional<bool>   m_lastRunSuccess;
    std::unordered_map<wxString, std::optional<bool>> m_lastRunSuccessMap;
    std::unordered_map<wxString, REPORTER*>           m_lastRunReporters;

    bool operator==( const JOBSET_DESTINATION& rhs ) const;

};


class KICOMMON_API JOBSET : public JSON_SETTINGS
{
public:
    JOBSET( const wxString& aFilename );

    virtual ~JOBSET() {}

    std::vector<JOBSET_JOB>& GetJobs()
    {
        return m_jobs;
    }

    std::vector<JOBSET_JOB> GetJobsForDestination( JOBSET_DESTINATION* aDestination );

    std::vector<JOBSET_DESTINATION>& GetDestinations() { return m_destinations; }

    JOBSET_DESTINATION* GetDestination( wxString& aDestination );

    bool SaveToFile( const wxString& aDirectory = "", bool aForce = false ) override;

    void SetDirty( bool aFlag = true ) { m_dirty = aFlag; }
    bool GetDirty() const { return m_dirty; }

    wxString GetFullName() const { return m_fileNameWithoutPath; }

    void AddNewJob( wxString aType, JOB* aJob );
    JOBSET_DESTINATION* AddNewDestination( JOBSET_DESTINATION_T aType );

    void RemoveDestination( JOBSET_DESTINATION* aDestination );
    void MoveJobUp( size_t aJobIdx );
    void MoveJobDown( size_t aJobIdx );
    void RemoveJob( size_t aJobIdx );

protected:
    wxString getFileExt() const override;

private:
    std::vector<JOBSET_JOB>         m_jobs;
    std::vector<JOBSET_DESTINATION> m_destinations;

    bool                            m_dirty;
    wxString                        m_fileNameWithoutPath;
};


KICOMMON_API void to_json( nlohmann::json& j, const JOBSET_JOB& f );
KICOMMON_API void from_json( const nlohmann::json& j, JOBSET_JOB& f );

KICOMMON_API void to_json( nlohmann::json& j, const JOBSET_DESTINATION& f );
KICOMMON_API void from_json( const nlohmann::json& j, JOBSET_DESTINATION& f );

#if defined( __MINGW32__ )
template class KICOMMON_API PARAM_LIST<struct JOBSET_JOB>;
template class KICOMMON_API PARAM_LIST<struct JOBSET_DESTINATION>;
#else
extern template class APIVISIBLE PARAM_LIST<JOBSET_JOB>;
extern template class APIVISIBLE PARAM_LIST<JOBSET_DESTINATION>;
#endif

#endif