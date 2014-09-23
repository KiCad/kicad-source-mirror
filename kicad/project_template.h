/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Brian Sidebotham <brian.sidebotham@gmail.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
    @page prj_tmp Project Templates

    Author: Brian Sidebotham

    Proposal
    --------

    To add new project template functionality to KiCad to facilitate the easy setup of projects
    which have common attributes such as pre-defined board outlines, connector positions,
    schematic elements, design rules, etc.


    Definitions
    -----------

    A template is a directory of files, which includes a directory of metadata. The template system
    name (SYSNAME) is the directory name under which the template files are stored. The metadata
    directory (METADIR) contains pre-defined files which provide information about the template.

    All files and directories in a template are copied to the new project path when a project is
    created using a template, except METADIR.

    All files and directories which start with SYSNAME will have SYSNAME replaced by the new
    project file name, excluding the file extension.


    Metadata
    --------

    A template's METADIR must contain the required files, and might optionally contain any of the
    optional files

        Required Files
        ~~~~~~~~~~~~~~

        /info.html  - Contains html formatted information about the template which is used by the
                      user to determine if the template is what they are after. The &lt;title&gt; tag
                      determines the actual name of the template that is exposed to the user for
                      template selection. Using html to format this document means that images can
                      be in-lined without having to invent a new scheme. Only HTML supported by
                      wxHTML can be used to format this document.

        Optional Files
        ~~~~~~~~~~~~~~

        /icon.png   - A 64 x 64px PNG icon file which is used as a clickable icon in the template
                      selection dialog.


    Operation
    ---------

    The KiCad File menu will be modified to change New from a menu item to a pop-out menu item, in
    the same manor as Open Recent. There will be two options on the pop-out menu:

    Blank -> Will act exactly the same as the current new menu item so that anyone who wishes to
    create a blank project won't have their settings lost or feel alienated.

    From Template -> Will open the template selection dialog.

    The template selection dialog will have a list of icons on the left, and a wxHTML window to the
    right. A single click on a template's icon on the left will load that templates info.html
    metadata file and display it in the wxHTML window.

    A double click on a template's icon will start the new project creation and will open a new
    file dialog. If the user selects a valid location for the new project, the template will be
    copied to the new project location ( excluding METADIR as mentioned earlier ) and any files
    that match the string replacement rules will be renamed to reflect the new project's name.

    The list of available templates will be gathered from the following sources:

    wxStandardPaths::GetExecutableDir()/../share/template/
    wxStandardPaths::GetUserDataDir()/templates/
    wxGetEnv(wxT("KICAD_PTEMPLATES"))
    wxGetEnv(wxT("KICAD"))/template/

*/

#ifndef PROJECT_TEMPLATE_H
#define PROJECT_TEMPLATE_H

#include <vector>

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/filename.h>

/**
 * @brief A directory which contains information about the project template and does not get
 * copied. This define is the default filename for this directory
 *
 */
#define METADIR             wxT("meta")


/**
 * @brief A required html formatted file which contains information about the project template.
 * This define is the default filename for this file
 *
 */
#define METAFILE_INFO_HTML  wxT("info.html")


/**
 * @brief An optional png icon, exactly 64px x 64px which is used in the template selector if
 * present. This define is the default filename for this file
 *
 */
#define METAFILE_ICON       wxT("icon.png")

/**
 * @brief A class which provides project template functionality.
 *
 *
 *
 */
class PROJECT_TEMPLATE {
private:
protected:
    wxFileName templateBasePath;
    wxFileName templateMetaPath;
    wxFileName templateMetaHtmlFile;
    wxFileName templateMetaIconFile;
    wxBitmap* metaIcon;
    wxString title;

public:

    /**
     * @brief Create a new project instance from \a aPath. \a aPath should be a directory that
     * conforms to the project template requirements
     *
     * @param aPath Should be a directory containing the template
     */
    PROJECT_TEMPLATE( const wxString& aPath );

    /**
     * @brief Non-virtual destructor (so no dervied classes)
     */
    ~PROJECT_TEMPLATE();

    /**
     * @brief Get the dir name of the project template
     * (i.e. the name of the last folder containing the template files)
     * @return the dir name of the template
     */
    wxString GetPrjDirName();

    /**
     * @brief Get the full Html filename for the project template
     * @return the html meta information file for this template
     */
    wxFileName GetHtmlFile();

    /**
     * @brief Copies and renames all template files to create a new project.
     * @param aNewProjectPath The full path of the new project file to create
     */
    bool CreateProject( wxFileName& aNewProjectPath );

    /**
     * @brief Get the 64px^2 icon for the project template
     * @return an image file of 64px x 64px which is the templates icon
     */
    wxBitmap* GetIcon();

    /**
     * @brief Get the title of the project (extracted from the html title tag)
     */
    wxString* GetTitle();

    /**
     * @brief Get a vector list of filenames for the template. The files are the source files,
     * and have not yet been through any renaming
     */
    std::vector<wxFileName> GetFileList();
};

#endif
