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

#ifndef JOB_EXPORT_SCH_PLOT_H
#define JOB_EXPORT_SCH_PLOT_H

#include <vector>
#include <kicommon.h>
#include "job.h"


enum class JOB_PAGE_SIZE
{
    PAGE_SIZE_AUTO,
    PAGE_SIZE_A4,
    PAGE_SIZE_A
};


enum class SCH_PLOT_FORMAT
{
    HPGL,
    POST,
    DXF,
    PDF,
    SVG
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT : public JOB
{
public:
    JOB_EXPORT_SCH_PLOT( bool aOutputIsDirectory );

    SCH_PLOT_FORMAT       m_plotFormat;
    wxString              m_filename;
    wxString              m_drawingSheet;
    wxString              m_defaultFont;
    wxString              m_variant;

    bool                  m_plotAll;
    bool                  m_plotDrawingSheet;
    std::vector<wxString> m_plotPages;

    bool                  m_show_hop_over;
    bool                  m_blackAndWhite;
    JOB_PAGE_SIZE         m_pageSizeSelect;
    bool                  m_useBackgroundColor;
    int                   m_minPenWidth;
    bool                  m_PDFPropertyPopups;
    bool                  m_PDFHierarchicalLinks;
    bool                  m_PDFMetadata;
    wxString              m_theme;

    // Variant names to export. Empty vector means default variant only.
    std::vector<wxString> m_variantNames;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_PDF : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_PDF( bool aOutputIsDirectory = true );
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_DXF : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_DXF();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_SVG : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_SVG();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_PS : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_PS();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_HPGL : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_HPGL();
    wxString GetDefaultDescription() const override;
};

#endif