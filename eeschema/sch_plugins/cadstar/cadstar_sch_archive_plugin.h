/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file cadstar_pcb_archive_plugin.h
 * @brief Pcbnew PLUGIN for CADSTAR PCB Archive (*.cpa) format: an ASCII format
 *        based on S-expressions.
 */

#ifndef CADSTAR_SCH_ARCHIVE_PLUGIN_H_
#define CADSTAR_SCH_ARCHIVE_PLUGIN_H_


#include <sch_io_mgr.h>
#include <reporter.h>


class SCH_SHEET;
class SCH_SCREEN;

class CADSTAR_SCH_ARCHIVE_PLUGIN : public SCH_PLUGIN
{
public:
    //-----<PUBLIC SCH_PLUGIN API>-------------------------------------------------

    const wxString GetName() const override;

    void SetReporter( REPORTER* aReporter ) override { m_reporter = aReporter; }

    const wxString GetFileExtension() const override;

    const wxString GetLibraryFileExtension() const override;

    int GetModifyHash() const override;

    SCH_SHEET* Load( const wxString& aFileName, SCHEMATIC* aSchematic,
            SCH_SHEET* aAppendToMe = NULL, const PROPERTIES* aProperties = NULL ) override;

    bool CheckHeader( const wxString& aFileName ) override;


    // unimplemented functions. Will trigger a not_implemented IO error.
    //void SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties = NULL ) override;

    //void Save( const wxString& aFileName, SCH_SCREEN* aSchematic, KIWAY* aKiway,
    //           const PROPERTIES* aProperties = NULL ) override;

    //void EnumerateSymbolLib( wxArrayString& aAliasNameList, const wxString& aLibraryPath,
    //                         const PROPERTIES* aProperties = NULL ) override;

    //LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
    //                      const PROPERTIES* aProperties = NULL ) override;

    //void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
    //                 const PROPERTIES* aProperties = NULL ) override;

    //void DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
    //                  const PROPERTIES* aProperties = NULL ) override;

    //void DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
    //                   const PROPERTIES* aProperties = NULL ) override;

    //void CreateSymbolLib( const wxString& aLibraryPath,
    //                      const PROPERTIES* aProperties = NULL ) override;

    // bool DeleteSymbolLib( const wxString& aLibraryPath,
    //                      const PROPERTIES* aProperties = NULL ) override;

    //bool IsSymbolLibWritable( const wxString& aLibraryPath ) override;

    //void SymbolLibOptions( PROPERTIES* aListToAppendTo ) const override;

    //-----</PUBLIC SCH_PLUGIN API>------------------------------------------------


    CADSTAR_SCH_ARCHIVE_PLUGIN()
    {
        m_reporter = &WXLOG_REPORTER::GetInstance();
    }

    ~CADSTAR_SCH_ARCHIVE_PLUGIN()
    {
    }

    REPORTER* m_reporter;          // current reporter for warnings/errors
};

#endif // CADSTAR_SCH_ARCHIVE_PLUGIN_H_
