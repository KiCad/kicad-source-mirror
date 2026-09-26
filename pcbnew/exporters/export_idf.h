/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EXPORT_IDF_H
#define EXPORT_IDF_H

#include <memory>
#include <math/vector2d.h>
#include <wx/string.h>

class BOARD;
class IDF3_BOARD;
class JOB_EXPORT_PCB_IDF;
class FILENAME_RESOLVER;
class REPORTER;


class IDF_EXPORTER
{
public:
    explicit IDF_EXPORTER( BOARD* aBoard, FILENAME_RESOLVER* aResolver, JOB_EXPORT_PCB_IDF* aSettings,
                           REPORTER* aReporter = nullptr );

    /**
     * Generate IDFv3 compliant board (*.emn) and library (*.emp) files for @p aPcb.
     *
     * Headless-friendly core of the IDF exporter. @p aResolver locates 3D model files and must be
     * non-null; the GUI passes the project resolver while non-GUI callers may pass a bare resolver.
     * On failure the routine returns false and, when @p aErrorMsg is non-null, stores a
     * human-readable message rather than showing a dialog.
     */
    bool Export( const wxString& aFullFileName ) const;

private:
    VECTOR2D getOrigin() const;

    /// Exports the given footprint to an in-progress IDF file
    void exportFootprint( FOOTPRINT* aFootprint, IDF3_BOARD& aIDFBoard ) const;

    BOARD*              m_board = nullptr;
    FILENAME_RESOLVER*  m_resolver = nullptr;
    JOB_EXPORT_PCB_IDF* m_settings = nullptr;
    REPORTER*           m_reporter = nullptr;

    std::unique_ptr<JOB_EXPORT_PCB_IDF> m_defaultSettings;
};

#endif // EXPORT_IDF_H
