/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_sheet_path.h
 * @brief Definition of the SCH_SHEET_PATH and SCH_SHEET_LIST classes for Eeschema.
 */

#ifndef CLASS_DRAWSHEET_PATH_H
#define CLASS_DRAWSHEET_PATH_H

#include <base_struct.h>

#include <map>


/** Info about complex hierarchies handling:
 * A hierarchical schematic uses sheets (hierarchical sheets) included in a
 * given sheet.  Each sheet corresponds to a schematic drawing handled by a
 * SCH_SCREEN structure.  A SCH_SCREEN structure contains drawings, and have
 * a filename to write it's data.  Also a SCH_SCREEN display a sheet number
 * and the name of the sheet.
 *
 * In simple (and flat) hierarchies a sheet is linked to a SCH_SCREEN,
 * and a SCH_SCREEN is used by only one hierarchical sheet.
 *
 * In complex hierarchies the same SCH_SCREEN (and its data) is shared between
 * more than one sheet.  Therefore subsheets (like subsheets in a SCH_SCREEN
 * shared by many sheets) can be also shared.  So the same SCH_SCREEN must
 * handle different components references and parts selection depending on
 * which sheet is currently selected, and how a given subsheet is selected.
 * 2 sheets share the same SCH_SCREEN (the same drawings) if they have the
 * same filename.
 *
 * In KiCad each component and sheet receives (when created) an unique
 * identification called Time Stamp.  So each sheet has 2 ids: its time stamp
 * (that cannot change) and its name ( that can be edited and therefore is
 * not reliable for strong identification).  KiCad uses Time Stamp ( a unique
 * 32 bit id), to identify sheets in hierarchies.
 * A given sheet in a hierarchy is fully labeled by its path (or sheet path)
 * that is the list of time stamp found to access it through the hierarchy
 * the root sheet is /.  All  other sheets have a path like /1234ABCD or
 * /4567FEDC/AA2233DD/.  This path can be displayed as human readable sheet
 * name like: / or /sheet1/include_sheet/ or /sheet2/include_sheet/
 *
 * So to know for a given SCH_SCREEN (a given schematic drawings) we must:
 *   1) Handle all references possibilities.
 *   2) When acceded by a given selected sheet, display (update) the
 *      corresponding references and sheet path
 *
 * The class SCH_SHEET_PATH handles paths used to access a sheet.  The class
 * SCH_SHEET_LIST allows one to handle the full (or partial) list of sheets and
 * their paths in a complex hierarchy.  The class EDA_ScreenList allows one
 * to handle the list of SCH_SCREEN. It is useful to clear or save data,
 * but is not suitable to handle the full complex hierarchy possibilities
 * (usable in flat and simple hierarchies).
 */


class wxFindReplaceData;
class SCH_SHEET;
class SCH_SCREEN;
class SCH_MARKER;
class SCH_ITEM;
class SCH_REFERENCE_LIST;


#define SHEET_NOT_FOUND          -1


/**
 * Type SCH_MULTI_UNIT_REFERENCE_MAP
 * is used to create a map of reference designators for multi-unit parts.
 */
typedef std::map<wxString, SCH_REFERENCE_LIST> SCH_MULTI_UNIT_REFERENCE_MAP;

/**
 * Class SCH_SHEET_PATH
 *
 * handles access to a stack of flattened #SCH_SHEET objects by way of a path for
 * creating a flattened schematic hierarchy.
 *
 * <p>
 * The #SCH_SHEET objects are stored in a list from first (usually the root sheet) to a
 * given sheet in last position.  The _last_ sheet is usually the sheet we want to select
 * or reach (which is what the function Last() returns).   Others sheets constitute the
 * "path" from the first to the last sheet.
 * </p>
 */
class SCH_SHEET_PATH
{
protected:
    std::vector< SCH_SHEET* > m_sheets;

    size_t m_current_hash;

    int m_pageNumber;              /// Page numbers are maintained by the sheet load order.

public:
    SCH_SHEET_PATH();

    /// Forwarded method from std::vector
    SCH_SHEET* at( size_t aIndex ) const { return m_sheets.at( aIndex ); }

    /// Forwarded method from std::vector
    void clear()
    {
        m_sheets.clear();
        Rehash();
    }

    /// Forwarded method from std::vector
    bool empty() const { return m_sheets.empty(); }

    /// Forwarded method from std::vector
    void pop_back()
    {
        m_sheets.pop_back();
        Rehash();
    }

    /// Forwarded method from std::vector
    void push_back( SCH_SHEET* aSheet )
    {
        m_sheets.push_back( aSheet );
        Rehash();
    }

    /// Forwarded method from std::vector
    size_t size() const { return m_sheets.size(); }

    void Rehash();

    size_t GetCurrentHash() const { return m_current_hash; }

    void SetPageNumber( int aPageNumber ) { m_pageNumber = aPageNumber; }

    int GetPageNumber() const { return m_pageNumber; }

    const SCH_SHEET* GetSheet( unsigned aIndex ) const
    {
        SCH_SHEET* retv = NULL;

        if( aIndex < size() )
            retv = at( aIndex );

        return const_cast< SCH_SHEET* >( retv );
    }

    SCH_SHEET* GetSheet( unsigned aIndex )
    {
        return const_cast< SCH_SHEET* >( static_cast< const SCH_SHEET_PATH& >( *this ).GetSheet( aIndex ) );
    }

    /**
     * Function Cmp
     * Compare if this is the same sheet path as aSheetPathToTest
     * @param aSheetPathToTest = sheet path to compare
     * @return 1 if this sheet path has more sheets than aSheetPathToTest,
     *   -1 if this sheet path has fewer sheets than aSheetPathToTest,
     *   or 0 if same
     */
    int Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const;

    /**
     * Function Last
     * returns a pointer to the last sheet of the list
     * One can see the others sheet as the "path" to reach this last sheet
     */
    SCH_SHEET* Last() const;

    /**
     * Function LastScreen
     * @return the SCH_SCREEN relative to the last sheet in list
     */
    SCH_SCREEN* LastScreen() const;

    /**
     * Function LastDrawList
     * @return a pointer to the first schematic item handled by the
     * SCH_SCREEN relative to the last sheet in list
     */
    SCH_ITEM* LastDrawList() const;

    /**
     * Get the last schematic item relative to the first sheet in the list.
     *
     * @return Last schematic item relative to the first sheet in the list if list
     *         is not empty.  Otherwise NULL.
     */
    SCH_ITEM* FirstDrawList() const;

    /**
     * Function Path
     * the path uses the time stamps which do not changes even when editing
     * sheet parameters
     * a path is something like / (root) or /34005677 or /34005677/00AE4523
     */
    wxString Path() const;

    /**
     * Function PathHumanReadable
     * returns the sheet path in a human readable form, i.e. as a path made
     * from sheet names.  The the "normal" path instead uses the time
     * stamps in the path.  (Time stamps do not change even when editing
     * sheet parameters).
     */
    wxString PathHumanReadable() const;

    /**
     * @return a PathName for the root sheet (like "/" or "<root>"
     * @param aUseShortName: true to return "/", false to return a longer name
     */
    static wxString GetRootPathName( bool aUseShortName = true );

    /**
     * Function UpdateAllScreenReferences
     * updates the reference and the m_Multi parameter (part selection) for all
     * components on a screen depending on the actual sheet path.
     * Mandatory in complex hierarchies because sheets use the same screen
     * (basic schematic)
     * but with different references and part selections according to the
     * displayed sheet
     */
    void UpdateAllScreenReferences();

    /**
     * Function GetComponents
     * adds a SCH_REFERENCE() object to \a aReferences for each component in the sheet.
     *
     * @param aReferences List of references to populate.
     * @param aIncludePowerSymbols : false to only get normal components.
     * @param aForceIncludeOrphanComponents : true to include components having no symbol found in lib.
     * ( orphan components)
     * The normal option is false, and set to true only to build the full list of components.
     */
    void GetComponents( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols = true,
                        bool aForceIncludeOrphanComponents = false );

    /**
     * Function GetMultiUnitComponents
     * adds a SCH_REFERENCE_LIST object to \a aRefList for each same-reference set of
     * multi-unit parts in the sheet. The map key for each element will be the
     * reference designator.
     *
     * @param aRefList Map of reference designators to reference lists
     * @param aIncludePowerSymbols : false to only get normal components.
     */
    void GetMultiUnitComponents( SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                                 bool aIncludePowerSymbols = true );

    /**
     * Function SetFootprintField
     * searches last sheet in the path for a component with \a aReference and set the footprint
     * field to \a aFootPrint if found.
     *
     * @param aReference The reference designator of the component.
     * @param aFootPrint The value to set the footprint field.
     * @param aSetVisible The value to set the field visibility flag.
     * @return true if \a aReference was found otherwise false.
     */
    bool SetComponentFootprint( const wxString& aReference, const wxString& aFootPrint,
                                bool aSetVisible );

    /**
     * Find the next schematic item in this sheet object.
     *
     * @param aType - The type of schematic item object to search for.
     * @param aLastItem - Start search from aLastItem.  If no aLastItem, search from
     *                    the beginning of the list.
     * @param aWrap - Wrap around the end of the list to find the next item if aLastItem
     *                is defined.
     * @return - The next schematic item if found.  Otherwise, NULL is returned.
     */
    SCH_ITEM* FindNextItem( KICAD_T aType, SCH_ITEM* aLastItem = NULL, bool aWrap = false ) const;

    /**
     * Find the previous schematic item in this sheet path object.
     *
     * @param aType - The type of schematic item object to search for.
     * @param aLastItem - Start search from aLastItem.  If no aLastItem, search from
     *                    the end of the list.
     * @param aWrap - Wrap around the beginning of the list to find the next item if aLastItem
     *                is defined.
     * @return - The previous schematic item if found.  Otherwise, NULL is returned.
     */
    SCH_ITEM* FindPreviousItem( KICAD_T aType, SCH_ITEM* aLastItem = NULL, bool aWrap = false ) const;

    /**
     * Function TestForRecursion
     *
     * test the SCH_SHEET_PATH file names to check adding the sheet stored in the file
     * \a aSrcFileName to the sheet stored in file \a aDestFileName  will cause a sheet
     * path recursion.
     *
     * @param aSrcFileName is the source file name of the sheet add to \a aDestFileName.
     * @param aDestFileName is the file name of the destination sheet for \a aSrcFileName.
     * @return true if \a aFileName will cause recursion in the sheet path.  Otherwise false.
     */
    bool TestForRecursion( const wxString& aSrcFileName, const wxString& aDestFileName ) const;

    int FindSheet( const wxString& aFileName ) const;

    /**
     * Function FindSheetByName
     *
     * searches the #SCH_SHEET_PATH for a sheet named \a aSheetName.
     *
     * @param aSheetName is the name of the sheet to find.
     * @return a pointer to the sheet named \a aSheetName if found or NULL if not found.
     */
    SCH_SHEET* FindSheetByName( const wxString& aSheetName );

    /**
     * Function FindSheetByPageNumber
     *
     * searches the #SCH_SHEET_LIST for a sheet with \a aPageNumber.
     *
     * @param aPageNumber is the number of the sheet to find.
     * @return a pointer to a #SCH_SHEET object page \a aPageNumber if found or NULL if not found.
     */
    SCH_SHEET* FindSheetByPageNumber( int aPageNumber );

    bool operator==( const SCH_SHEET_PATH& d1 ) const;

    bool operator!=( const SCH_SHEET_PATH& d1 ) const { return !( *this == d1 ) ; }

    bool operator<( const SCH_SHEET_PATH& d1 ) const { return m_sheets < d1.m_sheets; }

};


namespace std
{
    template<> struct hash<SCH_SHEET_PATH>
    {
        size_t operator()( const SCH_SHEET_PATH& path ) const;
    };
}


typedef std::vector< SCH_SHEET_PATH >            SCH_SHEET_PATHS;
typedef SCH_SHEET_PATHS::iterator                SCH_SHEET_PATHS_ITER;
typedef SCH_SHEET_PATHS::const_iterator          SCH_SHEET_PATHS_CITER;
typedef SCH_SHEET_PATHS::reverse_iterator        SCH_SHEET_PATHS_RITER;
typedef SCH_SHEET_PATHS::const_reverse_iterator  SCH_SHEET_PATHS_CRITER;


/**
 * Class SCH_SHEET_LIST
 *
 * handles a list of #SCH_SHEET_PATH objects in a flattened hierarchy.
 *
 * #SCH_SHEET objects are not unique, there can be many sheets with the same filename and
 * that share the same #SCH_SCREEN reference.   Each The schematic file (#SCH_SCREEN) may
 * be shared between these sheets and component references are specific to a sheet path.
 * When a sheet is entered, component references and sheet page number are updated.
 */
class SCH_SHEET_LIST : public SCH_SHEET_PATHS
{
private:
    bool            m_isRootSheet;
    SCH_SHEET_PATH  m_currentSheetPath;

public:

    /**
     * Constructor
     * build a flattened list of SCH_SHEET_PATH objects from \a aSheet.
     *
     * If aSheet == NULL, then this is an empty hierarchy which the user can populate.
     */
    SCH_SHEET_LIST( SCH_SHEET* aSheet = NULL );

    ~SCH_SHEET_LIST() {}

    /**
     * Function GetSheetByPath
     * returns a sheet matching the path name in \a aPath.
     *
     * @param aPath A wxString object containing path of the sheet to get.
     * @param aHumanReadable True uses the human readable path for comparison.
     *                       False uses the timestamp generated path.
     * @return The sheet that matches \a aPath or NULL if no sheet matching
     *         \a aPath is found.
     */
    SCH_SHEET_PATH* GetSheetByPath( const wxString& aPath, bool aHumanReadable = true );

    /**
     * Function IsModified
     * checks the entire hierarchy for any modifications.
     * @return True if the hierarchy is modified otherwise false.
     */
    bool IsModified();

    void ClearModifyStatus();

    /**
     * Function AnnotatePowerSymbols
     * Silently annotates the not yet annotated power symbols of the entire hierarchy
     * of the sheet path list.
     * It is called before creating a netlist, to annotate power symbols, without prompting
     * the user about not annotated or duplicate for these symbols, if only these symbols
     * need annotation ( a very frequent case ).
     */
    void AnnotatePowerSymbols();

    /**
     * Function GetComponents
     * adds a SCH_REFERENCE() object to \a aReferences for each component in the list
     * of sheets.
     *
     * @param aReferences List of references to populate.
     * @param aIncludePowerSymbols Set to false to only get normal components.
     * @param aForceIncludeOrphanComponents : true to include components having no symbol found in lib.
     * ( orphan components)
     * The normal option is false, and set to true only to build the full list of components.
     */
    void GetComponents( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols = true,
                        bool aForceIncludeOrphanComponents = false );

    /**
     * Function GetMultiUnitComponents
     * adds a SCH_REFERENCE_LIST object to \a aRefList for each same-reference set of
     * multi-unit parts in the list of sheets. The map key for each element will be the
     * reference designator.
     *
     * @param aRefList Map of reference designators to reference lists
     * @param aIncludePowerSymbols Set to false to only get normal components.
     */
    void GetMultiUnitComponents( SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                                 bool aIncludePowerSymbols = true );

    /**
     * Function FindNextItem
     * searches the entire schematic for the next schematic object.
     *
     * @param aType - The type of schematic item to find.
     * @param aSheetFound - The sheet the item was found in.  NULL if the next item
     *                      is not found.
     * @param aLastItem - Find next item after aLastItem if not NULL.
     * @param aWrap - Wrap past around the end of the list of sheets.
     * @return If found, Returns the next schematic item.  Otherwise, returns NULL.
     */
    SCH_ITEM* FindNextItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFound = NULL,
                            SCH_ITEM* aLastItem = NULL, bool aWrap = true );

    /**
     * Function FindPreviousItem
     * searches the entire schematic for the previous schematic item.
     *
     * @param aType - The type of schematic item to find.
     * @param aSheetFound - The sheet the item was found in.  NULL if the previous item
     *                      is not found.
     * @param aLastItem - Find the previous item before aLastItem if not NULL.
     * @param aWrap - Wrap past around the beginning of the list of sheets.
     * @return If found, the previous schematic item.  Otherwise, NULL.
     */
    SCH_ITEM* FindPreviousItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFound = NULL,
                                SCH_ITEM* aLastItem = NULL, bool aWrap = true );

    /**
     * Function SetFootprintField
     * searches all the sheets for a component with \a aReference and set the footprint
     * field to \a aFootPrint if found.
     *
     * @param aReference The reference designator of the component.
     * @param aFootPrint The value to set the footprint field.
     * @param aSetVisible The value to set the field visibility flag.
     * @return True if \a aReference was found otherwise false.
     */
    bool SetComponentFootprint( const wxString& aReference, const wxString& aFootPrint,
                                bool aSetVisible );

    /**
     * Function IsComplexHierarchy
     * searches all of the sheets for duplicate files names which indicates a complex
     * hierarchy.
     *
     * @return true if the #SCH_SHEET_LIST is a complex hierarchy.
     */
    bool IsComplexHierarchy() const;

    /**
     * Function TestForRecursion
     *
     * test every SCH_SHEET_PATH in the SCH_SHEET_LIST to verify if adding the sheets stored
     * in \a aSrcSheetHierarchy to the sheet stored in \a aDestFileName  will cause recursion.
     *
     * @param aSrcSheetHierarchy is the SCH_SHEET_LIST of the source sheet add to \a aDestFileName.
     * @param aDestFileName is the file name of the destination sheet for \a aSrcFileName.
     * @return true if \a aFileName will cause recursion in the sheet path.  Otherwise false.
     */
    bool TestForRecursion( const SCH_SHEET_LIST& aSrcSheetHierarchy,
                           const wxString& aDestFileName ) const;

    /**
     * Function FindSheetByName
     *
     * searches the entire #SCH_SHEET_LIST for a sheet named \a aSheetName.
     *
     * @param aSheetName is the name of the sheet to find.
     * @return a pointer to the sheet named \a aSheetName if found or NULL if not found.
     */
    SCH_SHEET* FindSheetByName( const wxString& aSheetName );

    /**
     * Function FindSheetForScreen
     *
     * returns the first sheetPath (not necessarily the only one) using a particular screen
     */
    SCH_SHEET_PATH* FindSheetForScreen( SCH_SCREEN* aScreen );

    /**
     * Function BuildSheetList
     * builds the list of sheets and their sheet path from \a aSheet.
     * If \a aSheet is the root sheet, the full sheet path and sheet list are built.
     *
     * @param aSheet is the starting sheet from which the list is built, or NULL
     *               indicating that g_RootSheet should be used.
     * @throw std::bad_alloc if the memory for the sheet path list could not be allocated.
     */
    void BuildSheetList( SCH_SHEET* aSheet );
};

#endif // CLASS_DRAWSHEET_PATH_H
