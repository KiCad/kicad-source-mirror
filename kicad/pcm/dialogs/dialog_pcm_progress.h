/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
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

#ifndef DIALOG_PCM_PROGRESS_H_
#define DIALOG_PCM_PROGRESS_H_


#include "dialog_pcm_progress_base.h"
#include "reporter.h"
#include <atomic>
#include <widgets/progress_reporter_base.h>
#include <wx/appprogress.h>


/**
 * @brief Progress dialog for PCM system
 *
 * This dialog is designed to work with PCM_TASK_MANAGER's threading system.
 * Some of it's methods are safe to call from a non-UI thread.
 */
class DIALOG_PCM_PROGRESS : public DIALOG_PCM_PROGRESS_BASE, public PROGRESS_REPORTER_BASE
{
protected:
    // Handlers for DIALOG_PCM_PROGRESS_BASE events.
    void OnCancelClicked( wxCommandEvent& event ) override;
    void OnCloseClicked( wxCommandEvent& event ) override;

public:
    /** Constructor */
    DIALOG_PCM_PROGRESS( wxWindow* parent, bool aShowDownloadSection = true );

    ///< Safe to call from non-UI thread. Adds a message to detailed report window.
    void PCMReport( const wxString& aText, SEVERITY aSeverity );

    ///< Safe to call from non-UI thread. Sets the download progress of the current zip entry.
    void SetDownloadProgress( uint64_t aDownloaded, uint64_t aTotal );

    ///< Safe to call from non-UI thread. Sets the download prgress of the current package.
    void SetPackageProgress( uint64_t aProgress, uint64_t aTotal );

    ///< Safe to call from non-UI thread. Advances to the next package.
    void AdvancePhase() override;

    ///< Safe to call from non-UI thread. Disables cancel button, enables close button.
    void SetFinished();

private:
    bool updateUI() override;

    static uint64_t toKb( uint64_t aValue );

private:
    std::atomic_int64_t  m_downloaded;
    std::atomic_int64_t  m_downloadTotal;

    std::atomic_int64_t  m_currentProgress;
    std::atomic_int64_t  m_currentProgressTotal;

    std::atomic_bool     m_finished;

    std::vector< std::pair<wxString, SEVERITY> > m_reports;

    wxWindowDisabler m_disabler;

    wxAppProgressIndicator m_appProgressIndicator;
};

#endif // DIALOG_PCM_PROGRESS_H_
