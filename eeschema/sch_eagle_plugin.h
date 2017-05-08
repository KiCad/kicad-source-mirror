/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2017 CERN
* @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SCH_EAGLE_PLUGIN_H_
#define _SCH_EAGLE_PLUGIN_H_

#include <wx/xml/xml.h>

#include <sch_line.h>
#include <sch_io_mgr.h>

// class KIWAY;
// class LINE_READER;
// class SCH_SCREEN;
// class SCH_SHEET;
// class SCH_BITMAP;
// class SCH_JUNCTION;
// class SCH_NO_CONNECT;
// class SCH_LINE;
// class SCH_BUS_ENTRY_BASE;s
// class SCH_TEXT;
// class SCH_COMPONENT;
// class SCH_FIELD;
// class PROPERTIES;
// class SCH_EAGLE_PLUGIN_CACHE;
// class LIB_PART;
// class PART_LIB;
// class LIB_ALIAS;


/**
 * Class SCH_EAGLE_PLUGIN
 * is a #SCH_PLUGIN derivation for loading 6.x+ Eagle schematic files.
 *
 *
 * As with all SCH_PLUGINs there is no UI dependencies i.e. windowing
 * calls allowed.
 */
class SCH_EAGLE_PLUGIN : public SCH_PLUGIN
{
public:

    SCH_EAGLE_PLUGIN();
    ~SCH_EAGLE_PLUGIN();

    const wxString GetName() const override;

    const wxString GetFileExtension() const override;

    int GetModifyHash() const override;

    void SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties = NULL ) override;

    SCH_SHEET* Load( const wxString& aFileName, KIWAY* aKiway, SCH_SHEET* aAppendToMe = NULL,
                     const PROPERTIES* aProperties = NULL ) override;

    void Save( const wxString& aFileName, SCH_SCREEN* aSchematic, KIWAY* aKiway,
               const PROPERTIES* aProperties = NULL ) override;

    size_t GetSymbolLibCount( const wxString& aLibraryPath,
                              const PROPERTIES* aProperties = NULL ) override;

    void EnumerateSymbolLib( wxArrayString& aAliasNameList, const wxString& aLibraryPath,
                             const PROPERTIES* aProperties = NULL ) override;

    LIB_ALIAS* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                           const PROPERTIES* aProperties = NULL ) override;

    void SaveSymbol( const wxString& aLibraryPath, const LIB_PART* aSymbol,
                     const PROPERTIES* aProperties = NULL ) override;

    void DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
                      const PROPERTIES* aProperties = NULL ) override;

    void DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                       const PROPERTIES* aProperties = NULL ) override;

    void CreateSymbolLib( const wxString& aLibraryPath,
                          const PROPERTIES* aProperties = NULL ) override;

    bool DeleteSymbolLib( const wxString& aLibraryPath,
                          const PROPERTIES* aProperties = NULL ) override;

    bool IsSymbolLibWritable( const wxString& aLibraryPath ) override;

    void SymbolLibOptions( PROPERTIES* aListToAppendTo ) const override;


private:
    void loadDrawing( wxXmlNode* aDrawingNode );
    void loadSchematic( wxXmlNode* aSchematicNode );
    void loadSheet( wxXmlNode* aSheetNode );
    void loadSegments( wxXmlNode* aSegmentsNode );
    SCH_LINE* loadWire( wxXmlNode* aWireNode );
    void loadInstance( wxXmlNode* aInstanceNode );
    void loadModuleinst( wxXmlNode* aModuleinstNode );
    void loadLibrary( wxXmlNode* aLibraryNode );
    LIB_PART* loadSymbol( wxXmlNode* aSymbolNode );


    SCH_SHEET* m_rootSheet; ///< The root sheet of the schematic being loaded..
    wxString m_version; ///< Eagle file version.
protected:
};

#endif  // _SCH_EAGLE_PLUGIN_H_
