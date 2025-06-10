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

#ifndef JOB_EXPORT_STEP_H
#define JOB_EXPORT_STEP_H

#include <kicommon.h>
#include <math/vector2d.h>
#include "job.h"

// Default value to chain 2 shapes when creating the board outlines
// from shapes on Edges.Cut layer
#define BOARD_DEFAULT_CHAINING_EPSILON 0.01

class KICOMMON_API EXPORTER_STEP_PARAMS
{
public:
    EXPORTER_STEP_PARAMS() :
            m_Origin(),
            m_Overwrite( false ),
            m_UseGridOrigin( false ),
            m_UseDrillOrigin( false ),
            m_UseDefinedOrigin( false ),
            m_UsePcbCenterOrigin( false ),
            m_IncludeUnspecified( true ),
            m_IncludeDNP( true ),
            m_SubstModels( true ),
            m_BoardOutlinesChainingEpsilon( BOARD_DEFAULT_CHAINING_EPSILON ),
            m_BoardOnly( false ),
            m_CutViasInBody( false ),
            m_ExportBoardBody( true ),
            m_ExportComponents( true ),
            m_ExportTracksVias( false ),
            m_ExportPads( false ),
            m_ExportZones( false ),
            m_ExportInnerCopper( false ),
            m_ExportSilkscreen( false ),
            m_ExportSoldermask( false ),
            m_FuseShapes( false ),
            m_FillAllVias( false ),
            m_OptimizeStep( true ),
            m_ExtraPadThickness( true ),
            m_Format( FORMAT::STEP ),
            m_OutputFile()
    {};

    enum class FORMAT
    {
        STEP,
        STEPZ,
        BREP,
        XAO,
        GLB,
        PLY,
        STL,
        U3D,
        PDF
    };

    wxString m_NetFilter;
    wxString m_ComponentFilter;

    VECTOR2D m_Origin;

    bool     m_Overwrite;
    bool     m_UseGridOrigin;
    bool     m_UseDrillOrigin;
    bool     m_UseDefinedOrigin;
    bool     m_UsePcbCenterOrigin;
    bool     m_IncludeUnspecified;
    bool     m_IncludeDNP;
    bool     m_SubstModels;
    double   m_BoardOutlinesChainingEpsilon;
    bool     m_BoardOnly;
    bool     m_CutViasInBody;
    bool     m_ExportBoardBody;
    bool     m_ExportComponents;
    bool     m_ExportTracksVias;
    bool     m_ExportPads;
    bool     m_ExportZones;
    bool     m_ExportInnerCopper;
    bool     m_ExportSilkscreen;
    bool     m_ExportSoldermask;
    bool     m_FuseShapes;
    bool     m_FillAllVias;
    bool     m_OptimizeStep;
    bool     m_ExtraPadThickness;
    FORMAT   m_Format;
    wxString m_OutputFile;

    wxString GetDefaultExportExtension() const;
    wxString GetFormatName() const;
};


class KICOMMON_API JOB_EXPORT_PCB_3D : public JOB
{
public:
    JOB_EXPORT_PCB_3D();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    void SetStepFormat( EXPORTER_STEP_PARAMS::FORMAT aFormat );

    enum class FORMAT
    {
        UNKNOWN, // defefer to arg
        STEP,
        STEPZ,
        BREP,
        XAO,
        GLB,
        VRML,
        PLY,
        STL,
        U3D,
        PDF
    };

    enum class VRML_UNITS
    {
        INCH,  // Do not use IN: it conflicts with a Windows header
        MM,
        METERS,
        TENTHS // inches
    };

public:
    bool                      m_hasUserOrigin;
    wxString                  m_filename;

    JOB_EXPORT_PCB_3D::FORMAT m_format;

    /// Despite the name; also used for other formats
    EXPORTER_STEP_PARAMS      m_3dparams;

    VRML_UNITS                m_vrmlUnits;
    wxString                  m_vrmlModelDir;
    bool                      m_vrmlRelativePaths;
};

#endif
