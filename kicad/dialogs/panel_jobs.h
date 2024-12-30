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

#pragma once

#include "panel_jobs_base.h"
#include <memory>
#include <grid_tricks.h>

class wxAuiNotebook;
class JOBSET;
class KICAD_MANAGER_FRAME;
class PANEL_JOBS;
class PANEL_JOB_OUTPUT;
struct JOBSET_OUTPUT;

class JOBS_GRID_TRICKS : public GRID_TRICKS
{
public:
    explicit JOBS_GRID_TRICKS( PANEL_JOBS* aParent, WX_GRID* aGrid );

    ~JOBS_GRID_TRICKS() override = default;

protected:
    PANEL_JOBS* m_parent;
};


class PANEL_JOBS : public PANEL_JOBS_BASE
{
public:
    PANEL_JOBS( wxAuiNotebook* aParent, KICAD_MANAGER_FRAME* aFrame,
                std::unique_ptr<JOBSET> aJobsFile );

    ~PANEL_JOBS();

    void RemoveOutput( JOBSET_OUTPUT* aOutput );

    void EnsurePcbSchFramesOpen();

    wxString GetFilePath() const;
    void     UpdateTitle();

    JOBSET* GetJobsFile() { return m_jobsFile.get(); }

    void OpenJobOptionsForListItem( size_t aItemIndex );

protected:
    virtual void OnSizeGrid( wxSizeEvent& aEvent ) override;
    virtual void OnAddJobClick( wxCommandEvent& aEvent ) override;
    virtual void OnAddOutputClick( wxCommandEvent& aEvent ) override;
    virtual void OnSaveButtonClick( wxCommandEvent& aEvent ) override;
    virtual void OnJobButtonUp( wxCommandEvent& aEvent ) override;
    virtual void OnJobButtonDown( wxCommandEvent& aEvent ) override;
    virtual void OnJobButtonDelete( wxCommandEvent& aEvent ) override;
    virtual void OnRunAllJobsClick( wxCommandEvent& event ) override;

    bool GetCanClose() override;

private:
    void rebuildJobList();
    void buildOutputList();
    void addJobOutputPanel( JOBSET_OUTPUT* aOutput );
    void adjustGridColumns();

private:
    wxAuiNotebook*             m_parentBook;
    KICAD_MANAGER_FRAME*       m_frame;
    std::unique_ptr<JOBSET> m_jobsFile;

    std::unordered_map<JOBSET_OUTPUT*, PANEL_JOB_OUTPUT*> m_outputPanelMap;
};