/**
 * @file dialog_symbol_remap.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <dialog_symbol_remap_base.h>


#ifndef _DIALOG_SYMBOL_REMAP_H_
#define _DIALOG_SYMBOL_REMAP_H_


class LEGACY_SYMBOL_LIB;
class SCH_SYMBOL;
class REPORTER;


class DIALOG_SYMBOL_REMAP : public DIALOG_SYMBOL_REMAP_BASE
{
public:
    DIALOG_SYMBOL_REMAP( SCH_EDIT_FRAME* aParent );

    void OnRemapSymbols( wxCommandEvent& aEvent ) override;

protected:
    void OnUpdateUIRemapButton( wxUpdateUIEvent& aEvent ) override;

private:
    /**
     * Add libraries found in the legacy library list to \a aLibs that are not found in
     * the global symbol library table.
     *
     * This function is used to create a project symbol library table when converting
     * legacy projects over to the new symbol library table implementation.  This only
     * needs to be called the first time a legacy project is opened.  The cache library
     * is ignored.
     *
     * @param aLibs is a vector container to add all of the libraries not found in the
     *              global symbol library table that were found in the legacy library
     *              list.
     * @return the number of libraries found.
     */
    size_t getLibsNotInGlobalSymbolLibTable( std::vector< LEGACY_SYMBOL_LIB* >& aLibs );

    void createProjectSymbolLibTable( REPORTER& aReporter );

    void remapSymbolsToLibTable( REPORTER& aReporter );

    bool remapSymbolToLibTable( SCH_SYMBOL* aSymbol );

    /**
     * Backup all of the files that could be modified by the remapping with a time stamp
     * appended to the file name into the "remap_backup" folder in case something goes wrong.
     *
     * Backup the following:
     * - All schematic (prj-name.sch -> remap_backup/prj-name-time-stamp.sch ) files.
     * - The project (prj-name.pro) -> remap_backup/prj-name-time-stamp.pro) file.
     * - The cache library (prj-name-cache.lib -> remap_backup/prj-name.-cache-time-stamp.lib)
     *   file.
     * - The rescue library (prj-name-rescue.lib -> remap_backup/prj-name.rescue-time-stamp.lib)
     *   file.
     * - The rescue library (prj-name-rescue.dcm -> remap_backup/prj-name.rescue-time-stamp.dcm)
     *   file.
     *
     * @param aReporter is the #REPORTER object in which to write information messages.
     * @return true to continue rescue or false to abort rescue.
     */
    bool backupProject( REPORTER& aReporter );

    bool m_remapped;

    SCH_EDIT_FRAME* m_frame;
};

#endif  // _DIALOG_SYMBOL_REMAP_H_
