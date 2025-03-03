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

#pragma once

#include "panel_jobset_base.h"
#include <memory>
#include <grid_tricks.h>

class wxAuiNotebook;
class JOBSET;
class KICAD_MANAGER_FRAME;
class PANEL_JOBSET;
class PANEL_DESTINATION;
struct JOBSET_DESTINATION;

enum COL_ORDER
{
    COL_NUMBER,
    COL_SOURCE,
    COL_DESCR,

    COL_COUNT // keep as last
};

class JOBS_GRID_TRICKS : public GRID_TRICKS
{
    enum
    {
        JOB_DESCRIPTION = GRIDTRICKS_FIRST_CLIENT_ID,
        JOB_PROPERTIES
    };

public:
    explicit JOBS_GRID_TRICKS( PANEL_JOBSET* aParent, WX_GRID* aGrid );

    ~JOBS_GRID_TRICKS() override = default;

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

    bool handleDoubleClick( wxGridEvent& aEvent ) override;

protected:
    PANEL_JOBSET* m_parent;
    int           m_doubleClickRow;
};


class PANEL_JOBSET : public PANEL_JOBSET_BASE
{
public:
    PANEL_JOBSET( wxAuiNotebook* aParent, KICAD_MANAGER_FRAME* aFrame,
                  std::unique_ptr<JOBSET> aJobsFile );

    ~PANEL_JOBSET();

    void RemoveDestination( PANEL_DESTINATION* aPanel );

    void EnsurePcbSchFramesOpen();

    wxString GetFilePath() const;
    void     UpdateTitle();

    JOBSET* GetJobsFile() { return m_jobsFile.get(); }

    bool OpenJobOptionsForListItem( size_t aItemIndex );
    void OnJobButtonDelete( wxCommandEvent& aEvent ) override;

    std::vector<PANEL_DESTINATION*> GetDestinationPanels();

protected:
    virtual void OnSizeGrid( wxSizeEvent& aEvent ) override;
    virtual void OnAddJobClick( wxCommandEvent& aEvent ) override;
    virtual void OnAddDestinationClick( wxCommandEvent& aEvent ) override;
    virtual void OnSaveButtonClick( wxCommandEvent& aEvent ) override;
    virtual void OnJobButtonUp( wxCommandEvent& aEvent ) override;
    virtual void OnJobButtonDown( wxCommandEvent& aEvent ) override;
    virtual void OnGenerateAllDestinationsClick( wxCommandEvent& event ) override;
    virtual void OnGridCellChange( wxGridEvent& aEvent ) override;

    bool GetCanClose() override;

private:
    void rebuildJobList();
    void buildDestinationList();
    void addDestinationPanel( JOBSET_DESTINATION* aDestination );

private:
    wxAuiNotebook*          m_parentBook;
    KICAD_MANAGER_FRAME*    m_frame;
    std::unique_ptr<JOBSET> m_jobsFile;
};