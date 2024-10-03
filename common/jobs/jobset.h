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

#ifndef JOBS_FILE_H
#define JOBS_FILE_H

#include <jobs/job.h>
#include <jobs/jobs_output.h>
#include <settings/json_settings.h>
#include <settings/parameters.h>
#include <ctime>

struct KICOMMON_API JOBSET_JOB
{
    JOBSET_JOB() : m_job( nullptr ) {}

    JOBSET_JOB( wxString id, wxString type, JOB* job ) : m_id( id ), m_type( type ), m_job( job ) {}

    wxString m_id;
    wxString m_type;
    JOB*     m_job;

    bool operator==( const JOBSET_JOB& rhs ) const;
};

enum class JOBSET_OUTPUT_TYPE
{
    FOLDER,
    ARCHIVE
};

struct KICOMMON_API JOBSET_OUTPUT
{
    JOBSET_OUTPUT() :
            m_type( JOBSET_OUTPUT_TYPE::FOLDER ), m_outputHandler( nullptr )
            {}

    JOBSET_OUTPUT( wxString id, JOBSET_OUTPUT_TYPE type, JOBS_OUTPUT_HANDLER* outputHandler ) :
            m_id( id ), m_type( type ), m_outputHandler( outputHandler )
    {
    }

    wxString              m_id;
    JOBSET_OUTPUT_TYPE    m_type;
    JOBS_OUTPUT_HANDLER*  m_outputHandler;
    std::vector<wxString> m_only;

    bool operator==( const JOBSET_OUTPUT& rhs ) const;
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

    std::vector<JOBSET_JOB> GetJobsForOutput( JOBSET_OUTPUT* aOutput );

    std::vector<JOBSET_OUTPUT>& GetOutputs() { return m_outputs; }

    JOBSET_OUTPUT* GetOutput( wxString& aOutput );

    bool SaveToFile( const wxString& aDirectory = "", bool aForce = false ) override;

    void SetDirty() { m_dirty = true; }
    bool GetDirty() const { return m_dirty; }

    wxString GetFullName() const { return m_fileNameWithoutPath; }

    void AddNewJob( wxString aType, JOB* aJob );
    JOBSET_OUTPUT AddNewJobOutput( JOBSET_OUTPUT_TYPE aType,
                                      JOBS_OUTPUT_HANDLER*  aJobOutput );

    void RemoveOutput( JOBSET_OUTPUT* aOutput );
    void MoveJobUp( size_t aJobIdx );
    void MoveJobDown( size_t aJobIdx );
    void RemoveJob( size_t aJobIdx );

protected:
    wxString getFileExt() const override;

private:
    std::vector<JOBSET_JOB> m_jobs;
    std::vector<JOBSET_OUTPUT> m_outputs;

    bool m_dirty;
    wxString m_fileNameWithoutPath;
};

KICOMMON_API void to_json( nlohmann::json& j, const JOBSET_JOB& f );
KICOMMON_API void from_json( const nlohmann::json& j, JOBSET_JOB& f );

KICOMMON_API void to_json( nlohmann::json& j, const JOBSET_OUTPUT& f );
KICOMMON_API void from_json( const nlohmann::json& j, JOBSET_OUTPUT& f );

#if defined( __MINGW32__ )
template class KICOMMON_API PARAM_LIST<struct JOBSET_JOB>;
template class KICOMMON_API PARAM_LIST<struct JOBSET_OUTPUT>;
#else
extern template class APIVISIBLE PARAM_LIST<JOBSET_JOB>;
extern template class APIVISIBLE PARAM_LIST<JOBSET_OUTPUT>;
#endif

#endif