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

#pragma once

#include <filesystem>

#include <wx/string.h>

#include <wx_filename.h>


class PROJECT;
class SETTINGS_MANAGER;

namespace KI_TEST
{
/*
 * A uniquely-named temporary directory removed on destruction.
 */
class SCOPED_TEMP_DIR
{
public:
    /**
     * Create a temporary directory with a unique name based on the given prefix.
     *
     * The directory is removed on destruction unless the environment variable
     * KICAD_QA_KEEP_TEMP, when interpreted as a comma-separated list of
     * prefixes, contains the given prefix. The special value "ALL" will keep all
     * temporary directories created by this class.
     *
     * @param aPrefix Prefix for the temporary directory name
     */
    SCOPED_TEMP_DIR( const wxString& aPrefix );

    ~SCOPED_TEMP_DIR();

    // Non-copyable: the destructor removes the directory, so instances must not be copied.
    SCOPED_TEMP_DIR( const SCOPED_TEMP_DIR& ) = delete;
    SCOPED_TEMP_DIR& operator=( const SCOPED_TEMP_DIR& ) = delete;

    /// Get the path to the temporary directory as a std::filesystem::path.
    const std::filesystem::path& Path() const { return m_path; }

    /// Get the path to the temporary directory as a wxString.
    wxString PathStr() const { return wxString::FromUTF8( m_path.string() ); }

    /**
     * Get the path to a direct child of the temporary directory as a wxString,
     * without creating the child.
     *
     * Note you don't need a ChildPath() as you can just use Path() / "childname"
     * to get a std::filesystem::path.
     *
     * Also note that this is the easiest/less-racy way to get a temp file
     * with a given extension rather than using wxFileName::CreateTempFileName() and
     * then renaming it, which is easy to leak or race.
     *
     * @param aName Name of the child file or directory, must be a single path component
     * @throws std::invalid_argument if aName is not a single path component
     */
    wxString ChildPathStr( const wxString& aName ) const;

    /**
     * Create and return the path to a direct child directory.
     *
     * @throws std::runtime_error if the directory cannot be created
     */
    std::filesystem::path CreateChildDir( const wxString& aName ) const;

    /**
     * Create and return the path to a direct child directory as a wxString.
     *
     * @throws std::runtime_error if the directory cannot be created
     */
    wxString CreateChildDirStr( const wxString& aName ) const;

    /**
     * Create and return the path to a direct child file.
     *
     * @throws std::runtime_error if the file cannot be created
     */
    std::filesystem::path CreateChildFile( const wxString& aName ) const;

    /**
     * Create and return the path to a direct child file as a wxString.
     *
     * @throws std::runtime_error if the file cannot be created
     */
    wxString CreateChildFileStr( const wxString& aName ) const;

    /**
     * Keep this directory after all
     *
     * It is nearly always wrong to use this in actual tests, but it can be
     * helpful for debugging purposes (e.g. conditionally keeping the
     * directory if a certain test fails).
     *
     * If a #SCOPED_PROCESS_TEMP_DIR is in use, the process-wide temporary
     * directory is kept as well.
     */
    void Retain();

    static bool AnyTempDirRetained() { return s_anyTempDirRetained; }

private:
    std::filesystem::path m_path;
    bool                  m_keep = false;

    // True if any temporary directory has been retained
    static bool s_anyTempDirRetained;
};


/*
 * A project loaded from its own temporary directory.
 *
 * The project is unloaded before the directory is removed so its lock file is released while the
 * file still exists.  Declare it ahead of any SCHEMATIC that refers to the project.
 */
class SCOPED_TEMP_PROJECT
{
public:
    SCOPED_TEMP_PROJECT( SETTINGS_MANAGER& aManager, const wxString& aPrefix, const wxString& aName );

    ~SCOPED_TEMP_PROJECT();

    SCOPED_TEMP_PROJECT( const SCOPED_TEMP_PROJECT& ) = delete;
    SCOPED_TEMP_PROJECT& operator=( const SCOPED_TEMP_PROJECT& ) = delete;

    PROJECT& Project() const { return *m_project; }

    /// Get the path to the temporary directory as a wxString.
    wxString DirStr() const { return m_dir.PathStr(); }

private:
    SCOPED_TEMP_DIR   m_dir;
    SETTINGS_MANAGER& m_manager;
    PROJECT*          m_project;
};

/**
 * Install a temporary directory for the process to use for temporary files,
 * which is (usually) removed on destruction.
 *
 * Declare this early in the test program, before any use of temporary directories.
 *
 * Both ends of its life can be outside the wx lifetime, so its constructor and
 * destructor make no wx logging calls: messages about kept or undeletable
 * directories go to stderr.
 *
 * The redirect applies to wxFileName and std::filesystem temp paths, but
 * won't affect hardcoded paths (which should be avoided anyway).
 *
 * This will then automatically tidy up any files created by
 * temp dir functions as in wxFileName and std::filesystem, even if
 * not properly scoped in a #SCOPED_TEMP_DIR.
 *
 * The directory is kept whenever any temporary directory created by
 * #SCOPED_TEMP_DIR is kept.
 */
class SCOPED_PROCESS_TEMP_DIR
{
public:
    SCOPED_PROCESS_TEMP_DIR( const wxString& aPrefix );
    ~SCOPED_PROCESS_TEMP_DIR();

    // No copying: the destructor removes the directory
    SCOPED_PROCESS_TEMP_DIR( const SCOPED_PROCESS_TEMP_DIR& ) = delete;
    SCOPED_PROCESS_TEMP_DIR& operator=( const SCOPED_PROCESS_TEMP_DIR& ) = delete;

private:
    SCOPED_TEMP_DIR m_dir;
};

} // namespace KI_TEST
