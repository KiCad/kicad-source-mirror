/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef GIT_PROGRESS_H_
#define GIT_PROGRESS_H_

#include <widgets/wx_progress_reporters.h>
#include <import_export.h>

#include <memory>

class APIEXPORT GIT_PROGRESS
{
public:
    GIT_PROGRESS() :
            m_previousProgress( 0 )
    {
        m_progressReporter.reset();
    }

    void SetProgressReporter( std::unique_ptr<WX_PROGRESS_REPORTER> aProgressReporter )
    {
        m_progressReporter = std::move( aProgressReporter );
    }

    void ReportProgress( int aCurrent, int aTotal, const wxString& aMessage )
    {
        if( m_progressReporter )
        {
            if( aCurrent == m_previousProgress || aTotal == 0 )
            {
                m_progressReporter->Pulse( aMessage );
            }
            else
            {
                m_progressReporter->SetCurrentProgress( static_cast<double>( aCurrent ) / aTotal );
                m_progressReporter->Report( aMessage );
            }

            m_previousProgress = aCurrent;
            m_progressReporter->KeepRefreshing();
        }
    }

    virtual void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) {};


protected:
    int m_previousProgress;

    std::unique_ptr<WX_PROGRESS_REPORTER> m_progressReporter;
};

#endif // GIT_PROGRESS_H__
