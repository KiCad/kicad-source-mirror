/**
 * @file dialog_symbol_remap.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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


class PART_LIB;
class SCH_COMPONENT;
class REPORTER;


class DIALOG_SYMBOL_REMAP : public DIALOG_SYMBOL_REMAP_BASE
{
public:
    DIALOG_SYMBOL_REMAP( wxWindow* aParent );

    void OnRemapSymbols( wxCommandEvent& aEvent ) override;

private:
    /**
     * Function getLibsNotInGlobalSymbolLibTable
     *
     * adds libraries found in the legacy library list to \a aLibs that are not found in
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
    size_t getLibsNotInGlobalSymbolLibTable( std::vector< PART_LIB* >& aLibs );

    void createProjectSymbolLibTable( REPORTER& aReporter );

    void remapSymbolsToLibTable( REPORTER& aReporter );

    bool remapSymbolToLibTable( SCH_COMPONENT* aSymbol );

    bool normalizeAbsolutePaths( const wxFileName& aPathA,
                                 const wxFileName& aPathB,
                                 wxString*         aResultPath );
};

#endif  // _DIALOG_SYMBOL_REMAP_H_
