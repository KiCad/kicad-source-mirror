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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef IO_BASE_H_
#define IO_BASE_H_

#include <map>
#include <vector>
#include <string>

#include <kicommon.h>
#include <core/utf8.h>
#include <wx/string.h>
#include <widgets/report_severity.h>

class REPORTER;
class PROGRESS_REPORTER;
class DIALOG_SHIM;
class wxWindow;

class KICOMMON_API IO_BASE
{
public:
    /**
    * Container that describes file type info
    */
    struct KICOMMON_API IO_FILE_DESC
    {
        wxString                 m_Description;    ///< Description shown in the file picker dialog

        /// Filter used for file pickers if m_IsFile is true.
        std::vector<std::string> m_FileExtensions;

        ///< In case of folders: extensions of files inside.
        std::vector<std::string> m_ExtensionsInDir;
        bool                     m_IsFile;          ///< Whether the library is a folder or a file
        bool                     m_CanRead;         ///< Whether the IO can read this file type
        bool                     m_CanWrite;        ///< Whether the IO can write this file type

        IO_FILE_DESC( const wxString& aDescription, const std::vector<std::string>& aFileExtensions,
                      const std::vector<std::string>& aExtsInFolder = {}, bool aIsFile = true,
                      bool aCanRead = true, bool aCanWrite = true ) :
                m_Description( aDescription ),
                m_FileExtensions( aFileExtensions ), m_ExtensionsInDir( aExtsInFolder ),
                m_IsFile( aIsFile ), m_CanRead( aCanRead ), m_CanWrite( aCanWrite )
        {
        }

        IO_FILE_DESC() : IO_FILE_DESC( wxEmptyString, {} ) {}

        /**
         * @return translated description + wildcards string for file dialogs.
         */
        wxString FileFilter() const;

        operator bool() const { return !m_Description.empty(); }
    };

    virtual ~IO_BASE() = default;

    /**
     * Return a brief hard coded name for this IO interface.
     */
    const wxString& GetName() const { return m_name; }

    /**
     * Set an optional reporter for warnings/errors.
     */
    virtual void SetReporter( REPORTER* aReporter ) { m_reporter = aReporter; }

    /**
     * Set an optional progress reporter.
     */
    virtual void SetProgressReporter( PROGRESS_REPORTER* aReporter )
    {
        m_progressReporter = aReporter;
    }

    ////////////////////////////////////////////////////
    // Library-related functions
    ////////////////////////////////////////////////////

    /**
     * Get the descriptor for the library container that this IO plugin operates on.
     *
     * @return File descriptor for the container of the library elements
     */
    virtual const IO_FILE_DESC GetLibraryDesc() const = 0;

    /**
     * Get the descriptor for the individual library elements that this IO plugin operates on.
     * For libraries where all the elements are in a single container (e.g. all elements in a
     * single file), then this will return the descriptor from #IO_BASE::GetLibraryDesc().
     *
     * @return File descriptor for the library elements
     */
    virtual const IO_FILE_DESC GetLibraryFileDesc() const { return GetLibraryDesc(); }

    /**
     * Checks if this IO object can read the specified library file/directory.
     * If not overridden, extension check is used.
     */
    virtual bool CanReadLibrary( const wxString& aFileName ) const;

    /**
     * Create a new empty library at @a aLibraryPath empty.
     *
     * It is an error to attempt to create an existing library or to attempt to create
     * on a "read only" location.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file, or URL
     *                     containing several elements.
     * @param aProperties is an associative array that can be used to tell the library create
     *                    function anything special, because it can take any number of additional
     *                    named tuning arguments that the IO is known to support.  The caller
     *                    continues to own this object (IO may not delete it), and IOs
     *                    should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem finding the library, or creating it.
     */
    virtual void CreateLibrary( const wxString& aLibraryPath,
                                const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Delete an existing library and returns true, or if library does not
     * exist returns false, or throws an exception if library exists but is read only or
     * cannot be deleted for some other reason.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory or file which
     *                     will contain several elements.
     * @param aProperties is an associative array that can be used to tell the library delete
     *                    implementation function anything special, because it can take any
     *                    number of additional named tuning arguments that the plugin is known
     *                    to support. The caller continues to own this object (plugin may not
     *                    delete it), and plugins should expect it to be optionally NULL.
     *
     * @return true if library deleted, false if library did not exist.
     *
     * @throw IO_ERROR if there is a problem deleting an existing library.
     */
    virtual bool DeleteLibrary( const wxString& aLibraryPath,
                                const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Return true if the library at @a aLibraryPath is writable.
     *
     * The system libraries are typically read only because of where they are installed..
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file, or URL
     *                     containing several footprints.
     *
     * @throw IO_ERROR if no library at aLibraryPath exists.
     */
    virtual bool IsLibraryWritable( const wxString& aLibraryPath );

    /**
     * Append supported IO options to \a aListToAppenTo along with internationalized
     * descriptions.  Options are typically appended so that a derived IO_BASE can call
     * its base class function by the same name first, thus inheriting options declared there.
     * (Some base class options could pertain to all functions in all derived IOs.)
     * Note that since aListToAppendTo is a PROPERTIES object, all options
     * will be unique and last guy wins.
     *
     * @param aListToAppendTo holds a tuple of
     * <dl>
     *   <dt>option</dt>
     *   <dd>This eventually is what shows up into the "options"
     *       field, possibly combined with others.</dd>
     *   <dt>internationalized description</dt>
     *   <dd>The internationalized description is displayed in DIALOG_PLUGIN_OPTIONS.
     *      It may be multi-line and be quite explanatory of the option.</dd>
     *  </dl>
     * <br>
     *  In the future perhaps \a aListToAppendTo evolves to something capable of also
     *  holding a wxValidator for the cells in said dialog:
     *  http://forums.wxwidgets.org/viewtopic.php?t=23277&p=104180.
     *  This would require a 3 column list, and introducing wx GUI knowledge to
     *  #SCH_IO, which has been avoided to date.
     */
    virtual void GetLibraryOptions( std::map<std::string, UTF8>* aListToAppendTo ) const;


    /**
     * @return true if this plugin supports a GUI for configuration that can be launched from
     *         the library table configuration dialog or other UI contexts
     */
    virtual bool SupportsConfigurationDialog() const { return false; }

    /**
     * @return a new instance of dialog that configures this plugin, or nullptr if this plugin
     *         does not support a configuration dialog.  Caller takes ownership.
     */
    virtual DIALOG_SHIM* CreateConfigurationDialog( wxWindow* aParent ) { return nullptr; }

    virtual void Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED );

    virtual void AdvanceProgressPhase();

protected:
    // Delete the zero-argument base constructor to force proper construction
    IO_BASE() = delete;

    /**
     * @param aName is the user-visible name for the IO loader
     */
    IO_BASE( const wxString& aName ) :
        m_name( aName ),
        m_reporter( nullptr ),
        m_progressReporter( nullptr )
    {
    }


    /// Name of the IO loader
    wxString m_name;

    /// Reporter to log errors/warnings to, may be nullptr
    REPORTER* m_reporter;

    /// Progress reporter to track the progress of the operation, may be nullptr
    PROGRESS_REPORTER* m_progressReporter;
};

#endif // IO_BASE_H_
