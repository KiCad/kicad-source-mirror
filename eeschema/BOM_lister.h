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
private:
    BOM_LABEL_LIST      m_labelList;            // a list of global and hierarchical labels
    SCH_REFERENCE_LIST  m_cmplist;              // a flat list of components in the full hierarchy
    FILE*               m_outFile;              // the output file for BOM generation
    char                m_separatorSymbol;      // the separator used for csv files ( usually \t ; or , )
    bool                m_outputFmtCsv;         // true to create Csv files, false to create text lists
    bool                m_includeSubComponents; // true to list each part
                                                // of a multiple part per package component
                                                // false to list only once this kind of component
    bool                m_csvForm;              // true to print less verbose component list
                                                // false to print more verbose component list
    bool                m_groupReferences;      // true to group  in list by reference (when possible,
                                                // i.e. when other fields have the same value
                                                // false to list one reference per line
    bool                m_printLocation;        // true to print component location in list by reference
    std::vector <int>   m_fieldIDactive;        // list of field IDs to print

public:
    BOM_LISTER()
    {
        m_outFile = NULL;
        m_separatorSymbol   = '\t';
        m_outputFmtCsv      = false;
        m_includeSubComponents = false;
        m_csvForm = true;
        m_printLocation     = false;
        m_groupReferences   = false;
    }

    // Accessors:
    void SetGroupReferences( bool aGroupRef )
    {
        m_groupReferences = aGroupRef;
    }

    void SetPrintLocation( bool aPrintLoc )
    {
        m_printLocation = aPrintLoc;
    }

    void SetIncludeSubCmp( bool aIncludeSubCmp )
    {
        m_includeSubComponents = aIncludeSubCmp;
    }

    /**
     * Function SetCvsFormOn
     * prepare parameters to create a BOM list in comma separated value (cvs)
     * @param aSeparator = the character used as "csv" separator
     */
    void SetCvsFormOn( char aSeparator )
    {
        m_csvForm = true;
        m_separatorSymbol = aSeparator;
    }

    /**
     * Function SetCvsFormOff
     * prepare parameters to create a BOM list in full text readable mode
     * (not csv format)
     */
    void SetCvsFormOff()
    {
        m_csvForm = false;
    }

    void            AddFieldIdToPrintList( int aFieldId );

    void ClearFieldIdPrintList() { m_fieldIDactive.clear(); }

    /**
     * Function CreateCsvBOMListByValues
     * print the list of components, grouped by values:
     * One line by value. The format is something like:
     *   value;quantity;references;other fields
     *   18pF;2;"C404 C405";SM0402
     *   22nF/25V;4;"C128 C168 C228 C268";SM0402
     * @param aFile = the file to write to (will be closed)
     */
    void            CreateCsvBOMListByValues( FILE* aFile );

    /**
     * Function PrintGlobalAndHierarchicalLabelsList
     * print the list of global and hierarchical labels by sheet or by name
     * @param aSortBySheet = true to print by sheet name order
     *          false to print by label name order
     * @param aFile = the file to write to (will be NOT closed)
     */
    void            PrintGlobalAndHierarchicalLabelsList( FILE* aFile, bool aSortBySheet );

    /**
     * Function PrintComponentsListByReferenceHumanReadable
     * print a BOM list in human readable form
     * @param aFile = the file to write to (will be NOT closed)
     */
    bool            PrintComponentsListByReferenceHumanReadable( FILE* aFile );

    /**
     * Function PrintComponentsListByReferenceCsvForm
     * print the list of components ordered by references. Generate 2 formats:
     * - full component list in csv form
     * - "short" component list in csv form, grouped by common fields values
     *          (mainly component value)
     * @param aFile = the file to write to (will be NOT closed)
     */
    bool            PrintComponentsListByReferenceCsvForm( FILE* aFile );

    /**
     * Function PrintComponentsListByValue
     * print the list of components, sorted by value, one line per component
     * not useable for csv format (use CreateCsvBOMListByValues instead)
     * @param aFile = the file to write to (will be NOT closed)
     */
    int             PrintComponentsListByValue( FILE* aFile );

private:

    /**
     * Helper function isFieldPrintable
     * @return true if the field aFieldId should be printed.
     * @param aFieldId = the field Id (FOOTPRIN, FIELD4 ...)
     */
    bool            isFieldPrintable( int aFieldId );

    /**
     * Helper function buildGlobalAndHierarchicalLabelsList
     * Populate m_labelList with global and hierarchical labels
     * and sheet pins labels
     */
    void            buildGlobalAndHierarchicalLabelsList();

    /**
     * Helper function returnFieldsString
     * @return a string containing all selected fields texts,
     * @param aComponent = the schematic component
     * separated by the csv separator symbol
     */
    const wxString  returnFieldsString( SCH_COMPONENT* aComponent );

    /**
     * Helper function returnURLItemLocation
     * @param aPathName = the full sheet name of item
     * @param aPosition = a position (in internal units) to print
     * @return a formated string to print the full location:
     * /sheet name/( X Y position)
     */
    const wxString  returnURLItemLocation( const wxString&  aPathName,
                                           wxPoint          aPosition );
};

#endif    // _BOM_LISTER_H_
