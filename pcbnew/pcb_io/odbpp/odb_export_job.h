/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <functional>
#include <vector>

#include <wx/string.h>
#include <layer_ids.h>
#include <exporters/fab_model/fab_sections.h>

class BOARD;
class JOB_EXPORT_PCB_ODB;
class PCB_EDIT_FRAME;
class PROGRESS_REPORTER;
class REPORTER;


struct ODB_EXPORT_RESULT
{
    bool                  m_ok = false;
    std::vector<wxString> m_outputs;
};

/// One LAYER block of matrix/matrix
struct ODB_MATRIX_ROW
{
    int      m_row = 0;
    wxString m_context;
    wxString m_type;
    wxString m_name;
    wxString m_polarity;
    wxString m_startName;
    wxString m_endName;
    wxString m_addType;
    wxString m_id;
    wxString m_ref;
    wxString m_cuTop;
    wxString m_cuBottom;
    wxString m_dielectricType;
};

/// One matrix row with its KiCad layer for the dialog's layer grid
struct ODB_MATRIX_PREVIEW_ROW
{
    ODB_MATRIX_ROW m_matrix;
    PCB_LAYER_ID   m_boardLayer = UNDEFINED_LAYER;
    wxString       m_displayLayer;
    bool           m_editable = false;
};


/// Generate the ODB++ tree or archive and return the write status and the paths written
ODB_EXPORT_RESULT GenerateODBPPFiles( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard,
                                      PCB_EDIT_FRAME*    aParentFrame = nullptr,
                                      PROGRESS_REPORTER* aProgressReporter = nullptr, REPORTER* aReporter = nullptr );

/// Build export matrix rows without layer overrides or writing files
std::vector<ODB_MATRIX_PREVIEW_ROW> PreviewOdbMatrix( BOARD* aBoard, const JOB_EXPORT_PCB_ODB& aJob );

/// Add or remove the automatic variant suffix on the file stem or final directory
wxString UpdateOdbVariantOutputPath( const wxString& aPath, bool aDirectory, bool aSeparate,
                                     bool aRemoveAutomaticSuffix );

/// Use an empty section key when Customize matches the data set default
wxString OdbSectionKeyForSelection( IPC2581::MODE aMode, const IPC2581::SECTION_SET& aSelection );

/// Sections a data set exports when no section key is given
IPC2581::SECTION_SET OdbDefaultSections( IPC2581::MODE aMode );

/// Matrix name of the row whose ID is aReference
wxString OdbPreviewReferenceName( const wxString& aReference, const std::vector<ODB_MATRIX_PREVIEW_ROW>& aRows );

/// Check whether variant output tokens could name the same file
bool OdbVariantFileTokensCollide( const std::vector<wxString>&                      aTokens,
                                  const std::function<wxString( const wxString& )>& aLocaleFold );
