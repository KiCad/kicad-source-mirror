/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCBNEW_JOBS_HANDLER_H
#define PCBNEW_JOBS_HANDLER_H

#include <jobs/job_dispatcher.h>
#include <pcb_plot_params.h>
#include <diff_merge/diff_doc_kind.h>

class KIWAY;
class BOARD;
class DS_PROXY_VIEW_ITEM;
class FOOTPRINT;
class JOB_EXPORT_PCB_GERBER;
class JOB_EXPORT_PCB_GERBERS;
class JOB_FP_EXPORT_SVG;
class TOOL_MANAGER;
class REPORTER;
class wxWindow;

class PCBNEW_JOBS_HANDLER : public JOB_DISPATCHER
{
public:
    PCBNEW_JOBS_HANDLER( KIWAY* aKiway );
    virtual ~PCBNEW_JOBS_HANDLER();

    int JobExportStep( JOB* aJob );
    int JobExportRender( JOB* aJob );
    int JobExportSvg( JOB* aJob );
    int JobExportDxf( JOB* aJob );
    int JobExportPdf( JOB* aJob );
    int JobExportPng( JOB* aJob );
    int JobExportPs( JOB* aJob );
    int JobExportGerber( JOB* aJob );
    int JobExportGerbers( JOB* aJob );
    int JobExportGencad( JOB* aJob );
    int JobExportDrill( JOB* aJob );
    int JobExportPos( JOB* aJob );
    int JobExportFpUpgrade( JOB* aJob );
    int JobExportFpSvg( JOB* aJob );
    int JobExportDrc( JOB* aJob );
    int JobExportIpc2581( JOB* aJob );
    int JobExportOdb( JOB* aJob );
    int JobExportIpcD356( JOB* aJob );
    int JobExportStats( JOB* aJob );
    int JobUpgrade( JOB* aJob );
    int JobImport( JOB* aJob );
    int JobDiff( JOB* aJob );
    int JobFpDiff( JOB* aJob );

    /// Non-job entry points (reached via the kiface KIFACE_MERGE_DOCUMENT /
    /// KIFACE_OPEN_DIFF_DIALOG function exports, not the JOB system).
    int RunMerge( KICAD_DIFF::DOC_KIND aKind, const wxString& aAncestor, const wxString& aOurs,
                  const wxString& aTheirs, const wxString& aOutput, bool aInteractive,
                  bool aSingleFile, REPORTER* aReporter );
    int OpenDiffDialog( KICAD_DIFF::DOC_KIND aKind, const wxString& aFileA, const wxString& aFileB,
                        const wxString& aLabelA, const wxString& aLabelB, wxWindow* aParent,
                        REPORTER* aReporter );

private:
    int runPcbMerge( const wxString& aAncestor, const wxString& aOurs, const wxString& aTheirs,
                     const wxString& aOutput, bool aInteractive );
    int runFpLibMerge( const wxString& aAncestor, const wxString& aOurs, const wxString& aTheirs,
                       const wxString& aOutput, bool aSingleFile );

public:

    /**
     * Clear the cached CLI board so the next job reloads from the current project.
     * Called when the API server switches documents.
     */
    void ClearCachedBoard();

private:
    BOARD* getBoard( const wxString& aPath = wxEmptyString );
    LSEQ convertLayerArg( wxString& aLayerString, BOARD* aBoard ) const;

    void populateGerberPlotOptionsFromJob( PCB_PLOT_PARAMS&  aPlotOpts,
                                           JOB_EXPORT_PCB_GERBER* aJob );
    void populateGerberPlotOptionsFromJob( PCB_PLOT_PARAMS& aPlotOpts,
                                           JOB_EXPORT_PCB_GERBERS* aJob );
    int  doFpExportSvg( JOB_FP_EXPORT_SVG* aSvgJob, const FOOTPRINT* aFootprint );
    void loadOverrideDrawingSheet( BOARD* brd, const wxString& aSheetPath );
    wxString resolveJobOutputPath( JOB* aJob, BOARD* aBoard, const wxString* aDrawingSheet = nullptr );

    DS_PROXY_VIEW_ITEM* getDrawingSheetProxyView( BOARD* aBrd );

    TOOL_MANAGER* getToolManager( BOARD* aBrd );

    std::unique_ptr<BOARD> m_cliBoard;
    std::unique_ptr<TOOL_MANAGER> m_toolManager;
};

#endif
