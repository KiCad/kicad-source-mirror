/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job.h>
#include <wx/filename.h>
#include <common.h>
#include <project.h>

JOB::JOB( const std::string& aType, bool aOutputIsDirectory ) :
        m_type( aType ),
        m_varOverrides(),
        m_tempOutputDirectory(),
        m_outputPath(),
        m_outputPathIsDirectory( aOutputIsDirectory ),
        m_description(),
        m_workingOutputPath()
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "description",
                                                    &m_description, m_description ) );

    if( m_outputPathIsDirectory )
    {
        m_params.emplace_back( new JOB_PARAM<wxString>( "output_dir",
                                                        &m_outputPath, m_outputPath ) );
    }
    else
    {
        m_params.emplace_back( new JOB_PARAM<wxString>( "output_filename",
                                                        &m_outputPath, m_outputPath ) );
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


wxString JOB::GetDefaultDescription() const
{
    return wxEmptyString;
}


wxString JOB::GetSettingsDialogTitle() const
{
    return _( "Job Settings" );
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


wxString JOB::ResolveOutputPath( const wxString& aPath, bool aPathIsDirectory, PROJECT* aProject ) const
{
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                if( m_titleBlock.TextVarResolver( token, aProject ) )
                    return true;
                if( aProject )
                    return aProject->TextVarResolver( token );
                return false;
            };

    wxString outPath = aPath;
    outPath = ExpandTextVars( outPath, &textResolver );

    if( !m_tempOutputDirectory.IsEmpty() )
    {
        if( aPathIsDirectory )
        {
            wxFileName fn( outPath );

            if( fn.IsAbsolute() || outPath.IsEmpty() )
                fn.AssignDir( m_tempOutputDirectory );
            else
                PrependDirectoryToPath( fn, m_tempOutputDirectory );

            return fn.GetFullPath();
        }
        else
        {
            wxFileName fn( outPath );
            if( fn.IsAbsolute() )
            {
                // uhhh, do nothing
                // its a full path passed by cli, so we return as-is
                // the job handlers should have fixed empty paths
            }
            else
            {
                PrependDirectoryToPath( fn, m_tempOutputDirectory );
            }

            return fn.GetFullPath();
        }
    }

    return outPath;
}


wxString JOB::GetFullOutputPath( PROJECT* aProject ) const
{
    return ResolveOutputPath( m_workingOutputPath.IsEmpty() ? m_outputPath : m_workingOutputPath,
                              m_outputPathIsDirectory, aProject );
}


void JOB::SetConfiguredOutputPath( const wxString& aPath )
{
    m_outputPath = aPath;
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