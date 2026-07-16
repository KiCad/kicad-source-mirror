/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef PCBNEW_BOARD_LOADER_H
#define PCBNEW_BOARD_LOADER_H

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <i18n_utility.h>
#include <pcb_io/pcb_io_mgr.h>

class BOARD;
class PCB_IO;
class PROJECT;
class PROGRESS_REPORTER;
class REPORTER;


class BOARD_LOADER
{
public:
    struct OPTIONS
    {
        const std::map<std::string, UTF8>* properties = nullptr;
        PROGRESS_REPORTER* progress_reporter = nullptr;
        REPORTER* reporter = nullptr;

        // optional callback to configure the plugin, e.g. to set GUI callbacks
        std::function<void( PCB_IO& )> plugin_configurator;

        // optional callback invoked with the plugin after a successful load, e.g. to extract the
        // importer's cached library footprints before the plugin is destroyed
        std::function<void( PCB_IO& )> post_load_hook;

        // if present, will be called with (sheet path, error message) on failure to load sheet
        std::function<void( const wxString&, const wxString& )> drawing_sheet_error_callback;

        bool initialize_after_load = true;
    };

    // Will propagate exceptions (e.g. FUTURE_FORMAT_ERROR, IO_ERROR) from the plugin
    static std::unique_ptr<BOARD> Load( const wxString& aFileName,
                                        PCB_IO_MGR::PCB_FILE_T aFormat,
                                        PROJECT* aProject,
                                        const OPTIONS& aOptions );

    // Will propagate exceptions (e.g. FUTURE_FORMAT_ERROR, IO_ERROR) from the plugin
    static std::unique_ptr<BOARD> Load( const wxString& aFileName,
                                        PCB_IO_MGR::PCB_FILE_T aFormat,
                                        PROJECT* aProject );

    static std::unique_ptr<BOARD> CreateEmptyBoard( PROJECT* aProject );

    static bool SaveBoard( wxString& aFileName, BOARD* aBoard, PCB_IO_MGR::PCB_FILE_T aFormat );

    static bool SaveBoard( wxString& aFileName, BOARD* aBoard );

private:
    static void initializeLoadedBoard( BOARD* aBoard, const wxString& aFileName,
                                       PROJECT* aProject, const OPTIONS& aOptions );
};

#endif
