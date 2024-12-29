/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/string.h>
#include "job.h"


enum class JOB_HPGL_PLOT_ORIGIN_AND_UNITS
{
    PLOTTER_BOT_LEFT,
    PLOTTER_CENTER,
    USER_FIT_PAGE,
    USER_FIT_CONTENT,
};


enum class JOB_HPGL_PAGE_SIZE
{
    DEFAULT = 0,
    SIZE_A5,
    SIZE_A4,
    SIZE_A3,
    SIZE_A2,
    SIZE_A1,
    SIZE_A0,
    SIZE_A,
    SIZE_B,
    SIZE_C,
    SIZE_D,
    SIZE_E,
};


enum class JOB_PAGE_SIZE
{
    PAGE_SIZE_AUTO,
    PAGE_SIZE_A4,
    PAGE_SIZE_A
};


enum class SCH_PLOT_FORMAT
{
    HPGL,
    GERBER,
    POST,
    DXF,
    PDF,
    SVG
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT : public JOB
{
public:
    JOB_EXPORT_SCH_PLOT();

    SCH_PLOT_FORMAT       m_plotFormat;
    wxString              m_filename;
    wxString              m_drawingSheet;

    bool                  m_plotAll;
    bool                  m_plotDrawingSheet;
    std::vector<wxString> m_plotPages;

    bool                  m_blackAndWhite;
    JOB_PAGE_SIZE         m_pageSizeSelect;
    bool                  m_useBackgroundColor;
    double                m_HPGLPenSize; // for HPGL format only: pen size
    JOB_HPGL_PAGE_SIZE    m_HPGLPaperSizeSelect;
    bool                  m_PDFPropertyPopups;
    bool                  m_PDFHierarchicalLinks;
    bool                  m_PDFMetadata;
    wxString              m_theme;

    wxString m_outputDirectory;
    wxString m_outputFile;

    JOB_HPGL_PLOT_ORIGIN_AND_UNITS m_HPGLPlotOrigin;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_PDF : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_PDF();
    wxString GetDefaultDescription() const override;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_DXF : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_DXF();
    wxString GetDefaultDescription() const override;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_SVG : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_SVG();
    wxString GetDefaultDescription() const override;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_PS : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_PS();
    wxString GetDefaultDescription() const override;
};


class KICOMMON_API JOB_EXPORT_SCH_PLOT_HPGL : public JOB_EXPORT_SCH_PLOT
{
public:
    JOB_EXPORT_SCH_PLOT_HPGL();
    wxString GetDefaultDescription() const override;
};

#endif