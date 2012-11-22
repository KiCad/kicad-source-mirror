/**
 * @file BOM_lister.h
 */

/* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#ifndef _BOM_LISTER_H_
#define _BOM_LISTER_H_

#include <netlist.h>


// A helper class to build item lists for BOM,
// and write lists on files
class BOM_LISTER
{
    BOM_LABEL_LIST m_labelList;     // a list of global and hierarchical labels
    FILE * m_outFile;               // the output file for BOM generation
    char m_separatorSymbol;         // the separator used for csv files ( usually \t ; or , )
    bool m_outputFmtCsv;            // true to create Csv files, false to create text lists
    bool m_includeSubComponents;    // true to list each part
                                    // of a multiple part per package component
                                    // false to list only once this kind of component
    std::vector <int> m_fieldIDactive;  // list of field IDs to print

public:
    BOM_LISTER()
    {
        m_outFile = NULL;
        m_separatorSymbol = '\t';
        m_outputFmtCsv = false;
        m_includeSubComponents = false;
    }

    void SetIncludeSubCmp( bool aIncludeSubCmp )
        { m_includeSubComponents = aIncludeSubCmp; }

    void CreateCsvBOMList( char aSeparator, FILE * aFile );
    void PrintComponentsListByPart( SCH_REFERENCE_LIST& aList );
    void AddFieldIdToPrintList( int aFieldId );
    void ClearFieldIdPrintList() { m_fieldIDactive.clear(); }

    /**
     * Function PrintGlobalAndHierarchicalLabelsList
     * print the list of global and hierarchical labels bu sheet or by name
     * @param aSortBySheet = true to print by sheet name order
     *          false to print by label name order
     * @param aFile = the file to write to (will be NOT clesed)
     */
    void PrintGlobalAndHierarchicalLabelsList( FILE * aFile, bool aSortBySheet );

private:
    bool isFieldPrintable( int aFieldId );
    void buildGlobalAndHierarchicalLabelsList();
};

#endif  // _BOM_LISTER_H_
