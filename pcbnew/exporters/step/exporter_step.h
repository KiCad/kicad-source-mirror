/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EXPORTER_STEP_H
#define EXPORTER_STEP_H

#include "step_pcb_model.h"
#include <geometry/shape_poly_set.h>
#include <gal/color4d.h>

// Default value to chain 2 shapes when creating the board outlines
// from shapes on Edges.Cut layer
#define BOARD_DEFAULT_CHAINING_EPSILON 0.01

class PCBMODEL;
class BOARD;
class FOOTPRINT;
class FILENAME_RESOLVER;

class EXPORTER_STEP_PARAMS
{
public:
    EXPORTER_STEP_PARAMS() :
            m_origin(),
            m_overwrite( false ),
            m_useGridOrigin( false ),
            m_useDrillOrigin( false ),
            m_includeExcludedBom( true ),
            m_substModels( true ),
            m_boardOutlinesChainingEpsilon( BOARD_DEFAULT_CHAINING_EPSILON ),
            m_boardOnly( false ) {};

    wxString m_outputFile;

    VECTOR2D m_origin;

    bool     m_overwrite;
    bool     m_useGridOrigin;
    bool     m_useDrillOrigin;
    bool     m_includeExcludedBom;
    bool     m_substModels;
    double   m_boardOutlinesChainingEpsilon;
    bool     m_boardOnly;
};

class EXPORTER_STEP
{
public:
    EXPORTER_STEP( BOARD* aBoard, const EXPORTER_STEP_PARAMS& aParams );
    ~EXPORTER_STEP();

    bool Export();

    wxString m_outputFile;

    void SetError() { m_error = true; }
    void SetFail() { m_fail = true; }
    void SetWarn() { m_warn = true; }

private:
    bool composePCB();
    bool composePCB( FOOTPRINT* aFootprint, VECTOR2D aOrigin );
    void determinePcbThickness();

    EXPORTER_STEP_PARAMS m_params;
    std::unique_ptr<FILENAME_RESOLVER> m_resolver;

    bool m_error;
    bool m_fail;
    bool m_warn;

    bool            m_hasDrillOrigin;
    bool            m_hasGridOrigin;
    BOARD*          m_board;
    std::unique_ptr<STEP_PCB_MODEL> m_pcbModel;

    /// the name of the project (board short filename (no path, no ext)
    /// used to identify items in step file
    wxString        m_pcbBaseName;

    // minimum distance between points to treat them as separate entities (mm)
    double   m_minDistance;

    double   m_boardThickness;

    KIGFX::COLOR4D m_solderMaskColor;
};

#endif