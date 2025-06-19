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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JOB_EXPORT_PCB_PDF_H
#define JOB_EXPORT_PCB_PDF_H

#include <kicommon.h>
#include <kicommon.h>
#include <layer_ids.h>
#include <lseq.h>
#include <jobs/job_export_pcb_plot.h>


class KICOMMON_API JOB_EXPORT_PCB_PDF : public JOB_EXPORT_PCB_PLOT
{
public:
    JOB_EXPORT_PCB_PDF();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    bool m_pdfFrontFPPropertyPopups;
    bool m_pdfBackFPPropertyPopups;
    bool m_pdfMetadata;

    bool m_pdfSingle;

    ///< This is a hack to deal with cli having the wrong behavior
    ///< We will deprecate out the wrong behavior, at which point this enum
    ///< can be replaced with a bool
    enum class GEN_MODE
    {
        ///< DEPRECATED MODE
        ALL_LAYERS_ONE_FILE,
        ///< "Single Document" mode
        ONE_PAGE_PER_LAYER_ONE_FILE,
        ///< The most traditional output mode KiCad has had
        ALL_LAYERS_SEPARATE_FILE
    };

    ///< uused by the cli, will be removed when the other behavior is deprecated
    GEN_MODE m_pdfGenMode;

    ///< The background color specified in a hex string
    wxString m_pdfBackgroundColor;
};

#endif
