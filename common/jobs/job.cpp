/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job.h>
#include <wx/filename.h>

JOB::JOB( const std::string& aType, bool aOutputIsDirectory, bool aIsCli ) :
        m_type( aType ),
        m_isCli( aIsCli ),
        m_varOverrides(),
        m_tempOutputDirectory(),
        m_outputPath(),
        m_outputPathIsDirectory( aOutputIsDirectory )
{
    if( m_outputPathIsDirectory )
    {
        m_params.emplace_back(
                new JOB_PARAM<wxString>( "output_dir", &m_outputPath, m_outputPath ) );
    }
    else
    {
        m_params.emplace_back(
                new JOB_PARAM<wxString>( "output_filename", &m_outputPath, m_outputPath ) );
    }
}


JOB::~JOB()
{
    for( JOB_PARAM_BASE* param : m_params )
        delete param;

    m_params.clear();
}


void JOB::FromJson( const nlohmann::json& j )
{
    for( JOB_PARAM_BASE* param : m_params )
        param->FromJson( j );
}


void JOB::ToJson( nlohmann::json& j ) const
{
    for( JOB_PARAM_BASE* param : m_params )
        param->ToJson( j );
}


wxString JOB::GetDescription()
{
    return wxEmptyString;
}


void JOB::SetTempOutputDirectory( const wxString& aBase )
{
    m_tempOutputDirectory = aBase;
}


void PrependDirectoryToPath( wxFileName& aFileName, const wxString aDirPath )
{
    wxFileName fn( aDirPath + wxFileName::GetPathSeparator() + aFileName.GetFullPath() );

    aFileName = fn;
}


wxString JOB::GetFullOutputPath() const
{
    if( !m_tempOutputDirectory.IsEmpty() )
    {
        if( m_outputPathIsDirectory )
        {
            wxFileName fn( m_outputPath );
            if( fn.IsAbsolute() || m_outputPath.IsEmpty() )
            {
                fn.AssignDir( m_tempOutputDirectory );
            }
            else
            {
                PrependDirectoryToPath( fn, m_tempOutputDirectory );
            }


            return fn.GetFullPath();
        }
        else
        {
            wxFileName fn( m_outputPath );
            if( fn.IsAbsolute() || m_outputPath.IsEmpty() )
            {
                // uhhh, do nothing
                // either its a full path passed by cli, so we return as-is
                // or it's a empty path from either...in which case things will fail but who cares
                // the job handlers should have fixed empty paths
            }
            else
            {
                PrependDirectoryToPath( fn, m_tempOutputDirectory );
            }

            return fn.GetFullPath();
        }
	}

    return m_outputPath;
}


void JOB::SetOutputPath( const wxString& aPath )
{
    m_outputPath = aPath;
}


bool JOB::OutputPathFullSpecified() const
{
    if( m_outputPath.IsEmpty() )
    {
        return false;
    }

    wxFileName fn( m_outputPath );
    if( m_outputPathIsDirectory )
    {
        return fn.IsDir();
    }
    else
    {
        return !fn.IsDir();
    }
}


KICOMMON_API void to_json( nlohmann::json& j, const JOB& f )
{
    f.ToJson( j );
}


KICOMMON_API void from_json( const nlohmann::json& j, JOB& f )
{
    f.FromJson( j );
}


JOB_PARAM_BASE::JOB_PARAM_BASE( const std::string& aJsonPath ) :
        m_jsonPath( aJsonPath )
{
}