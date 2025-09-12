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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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