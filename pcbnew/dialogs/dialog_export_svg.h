/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <dialog_export_svg_base.h>
#include <lseq.h>

class BOARD;
class PCB_EDIT_FRAME;
class JOB_EXPORT_PCB_SVG;

class DIALOG_EXPORT_SVG : public DIALOG_EXPORT_SVG_BASE
{
public:
    DIALOG_EXPORT_SVG( JOB_EXPORT_PCB_SVG* aJob, PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent );
    DIALOG_EXPORT_SVG( PCB_EDIT_FRAME* aEditFrame, BOARD* aBoard, wxWindow* aParent );
    ~DIALOG_EXPORT_SVG() override;

private:
    BOARD*              m_board;
    JOB_EXPORT_PCB_SVG* m_job;
    PCB_EDIT_FRAME*     m_editFrame;
    LSEQ                m_printMaskLayer;
    // the list of existing board layers in wxCheckListBox, with the
    // board layers id:
    std::map<int, std::pair<wxCheckListBox*, int>> m_boxSelectLayer;
    bool                            m_printBW;
    wxString                        m_outputDirectory;
    bool                            m_printMirror;
    bool                            m_oneFileOnly;

    void initDialog();

    void OnButtonPlot( wxCommandEvent& event ) override;

    void onPagePerLayerClicked( wxCommandEvent& event ) override;
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;
    void ExportSVGFile( bool aOnlyOneFile );

    LSET getCheckBoxSelectedLayers() const;
};