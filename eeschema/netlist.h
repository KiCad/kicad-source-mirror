/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file netlist.h
 */

#ifndef _NETLIST_H_
#define _NETLIST_H_


#include <macros.h>

#include <class_libentry.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <sch_text.h>


/// netlist types
enum  NETLIST_TYPE_ID {
    NET_TYPE_UNINIT = 0,
    NET_TYPE_PCBNEW,
    NET_TYPE_ORCADPCB2,
    NET_TYPE_CADSTAR,
    NET_TYPE_SPICE,
    NET_TYPE_CUSTOM1,   /* NET_TYPE_CUSTOM1
                         * is the first id for user netlist format
                         * NET_TYPE_CUSTOM1+CUSTOMPANEL_COUNTMAX-1
                         * is the last id for user netlist format
                         */
    //NET_TYPE_CUSTOM_MAX = NET_TYPE_CUSTOM1 + CUSTOMPANEL_COUNTMAX - 1
};


/// Options for Spice netlist generation (OR'ed bits
enum netlistOptions {
    NET_PCBNEW_USE_NEW_FORMAT = 1,      // For Pcbnew use the new format (S expression and SWEET)
    NET_USE_X_PREFIX = 2,               // for Spice netlist : change "U" and "IC" reference prefix to "X"
    NET_USE_NETCODES_AS_NETNAMES = 4    // for Spice netlist : use netcode numbers as netnames
};


class SCH_COMPONENT;
class SCH_REFERENC_LIST;


#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"

// Max pin number per component and footprint
#define MAXPIN      5000


/**
 * Class SCH_REFERENCE
 * is used as a helper to define a component's reference designator in a schematic.  This
 * helper is required in a complex hierarchy because a component can be used more than
 * once and its reference depends on the sheet path.  This class is used to flatten the
 * schematic hierarchy for annotation, net list generation, and bill of material
 * generation.
 */
class SCH_REFERENCE
{
private:
    /// Component reference prefix, without number (for IC1, this is IC) )
    std::string    m_Ref;               // it's private, use the accessors please
    SCH_COMPONENT* m_RootCmp;           ///< The component associated the reference object.
    LIB_COMPONENT* m_Entry;             ///< The source component from a library.
    wxPoint        m_CmpPos;            ///< The physical position of the component in schematic
                                        ///< used to annotate by X or Y position
    int            m_Unit;              ///< The unit number for components with multiple parts
                                        ///< per package.
    SCH_SHEET_PATH m_SheetPath;         ///< The sheet path for this reference.
    bool           m_IsNew;             ///< True if not yet annotated.
    int            m_SheetNum;          ///< The sheet number for the reference.
    time_t         m_TimeStamp;         ///< The time stamp for the reference.
    EDA_TEXT*      m_Value;             ///< The component value of the refernce.  It is the
                                        ///< same for all instances.
    int            m_NumRef;            ///< The numeric part of the reference designator.
    int            m_Flag;

    friend class SCH_REFERENCE_LIST;

public:

    SCH_REFERENCE()
    {
        m_RootCmp      = NULL;
        m_Entry        = NULL;
        m_Unit         = 0;
        m_TimeStamp    = 0;
        m_IsNew        = false;
        m_Value        = NULL;
        m_NumRef       = 0;
        m_Flag         = 0;
        m_SheetNum     = 0;
    }

    SCH_REFERENCE( SCH_COMPONENT* aComponent, LIB_COMPONENT* aLibComponent,
                   SCH_SHEET_PATH& aSheetPath );

    SCH_COMPONENT* GetComponent() const { return m_RootCmp; }

    LIB_COMPONENT* GetLibComponent() const { return m_Entry; }

    SCH_SHEET_PATH GetSheetPath() const { return m_SheetPath; }

    int GetUnit() const { return m_Unit; }

    void SetSheetNumber( int aSheetNumber ) { m_SheetNum = aSheetNumber; }

    /**
     * Function Annotate
     * updates the annotation of the component according the the current object state.
     */
    void Annotate();

    /**
     * Function Split
     * attempts to split the reference designator into a name (U) and number (1).  If the
     * last character is '?' or not a digit, the reference is tagged as not annotated.
     * For components with multiple parts per package that are not already annotated, set
     * m_Unit to a max value (0x7FFFFFFF).
     */
    void Split();

    /*  Some accessors which hide the strategy of how the reference is stored,
        thereby making it easy to change that strategy.
    */

    void SetRef( const wxString& aReference )
    {
        m_Ref =  TO_UTF8( aReference );
    }

    wxString GetRef() const
    {
        return FROM_UTF8( m_Ref.c_str() );
    }
    void SetRefStr( const std::string& aReference )
    {
        m_Ref = aReference;
    }
    const char* GetRefStr() const
    {
        return m_Ref.c_str();
    }

    int CompareValue( const SCH_REFERENCE& item ) const
    {
        return m_Value->GetText().CmpNoCase( item.m_Value->GetText() );
    }

    int CompareRef( const SCH_REFERENCE& item ) const
    {
        return m_Ref.compare( item.m_Ref );
    }

    int CompareLibName( const SCH_REFERENCE& item ) const
    {
        return m_RootCmp->GetLibName().CmpNoCase( item.m_RootCmp->GetLibName() );
    }

    bool IsPartsLocked()
    {
        return m_Entry->UnitsLocked();
    }
};


/**
 * Class SCH_REFERENCE_LIST
 * is used create a flattened list of components because in a complex hierarchy, a component
 * can be used more than once and its reference designator is dependent on the sheet path for
 * the same component.  This flattened list is used for netlist generation, BOM generation,
 * and schematic annotation.
 */
class SCH_REFERENCE_LIST
{
private:
    std::vector <SCH_REFERENCE> componentFlatList;

public:
    /** Constructor
     */
    SCH_REFERENCE_LIST()
    {
    }

    SCH_REFERENCE& operator[]( int aIndex )
    {
        return componentFlatList[ aIndex ];
    }

    /**
     * Function GetCount
     * @return the number of items in the list
     */
    unsigned GetCount()
    {
        return componentFlatList.size();
    }

    /**
     * Function GetItem
     * @return the aIdx item
     */
    SCH_REFERENCE& GetItem( int aIdx )
    {
        return componentFlatList[aIdx];
    }

    /**
     * Function AddItem
     * adds a SCH_REFERENCE object to the list of references.
     * @param aItem - a SCH_REFERENCE item to add
     */
    void AddItem( SCH_REFERENCE& aItem )
    {
        componentFlatList.push_back( aItem );
    }

    /**
     * Function RemoveItem
     * removes an item from the list of references.
     *
     * @param aIndex is the index of the item to be removed.
     */
    void RemoveItem( unsigned int aIndex );

    /**
     * Function RemoveSubComponentsFromList
     * Remove sub components from the list, when multiples parts per package are
     * found in this list.
     * Useful to create BOM, when a component must appear only once
     */
    void RemoveSubComponentsFromList();

    /* Sort functions:
     * Sort functions are used to sort components for annotation or BOM generation.
     * Because sorting depend on we want to do, there are many sort functions.
     * Note:
     *    When creating BOM, components are fully annotated.
     *    references are something like U3, U5 or R4, R8
     *    When annotating,  some or all components are not annotated,
     *    i.e. ref is only U or R, with no number.
     */

    /**
     * Function SplitReferences
     * attempts to split all reference designators into a name (U) and number (1).  If the
     * last character is '?' or not a digit, the reference is tagged as not annotated.
     * For components with multiple parts per package that are not already annotated, set
     * m_Unit to a max value (0x7FFFFFFF).
     * @see SCH_REFERENCE::Split()
     */
    void SplitReferences()
    {
        for( unsigned ii = 0; ii < GetCount(); ii++ )
            componentFlatList[ii].Split();
    }

    /**
     * function UpdateAnnotation
     * Updates the reference components for the schematic project (or the current sheet)
     * Note: this function does not calculate the reference numbers stored in m_NumRef
     * So, it must be called after calculation of new reference numbers
     * @see SCH_REFERENCE::Annotate()
     */
    void UpdateAnnotation()
    {
        /* update the reference numbers */
        for( unsigned ii = 0; ii < GetCount(); ii++ )
        {
            componentFlatList[ii].Annotate();
        }
    }

    /**
     * Function Annotate
     * set the reference designators in the list that have not been annotated.
     * @param aUseSheetNum Set to true to start annotation for each sheet at the sheet number
     *                     times \a aSheetIntervalId.  Otherwise annotate incrementally.
     * @param aSheetIntervalId The per sheet reference designator multiplier.
     * <p>
     * If a the sheet number is 2 and \a aSheetIntervalId is 100, then the first reference
     * designator would be 201 and the last reference designator would be 299 when no overlap
     * occurs with sheet number 3.  If there are 150 items in sheet number 2, then items are
     * referenced U201 to U351, and items in sheet 3 start from U352
     * </p>
     */
    void Annotate( bool aUseSheetNum, int aSheetIntervalId );

    /**
     * Function CheckAnnotation
     * check for annotations errors.
     * <p>
     * The following annotation error conditions are tested:
     * <ul>
     * <li>Components not annotated.</li>
     * <li>Components having the same reference designator (duplicates).</li>
     * <li>Components with multiple parts per package having different reference designators.</li>
     * <li>Components with multiple parts per package with invalid part count.</li>
     * </ul>
     * </p>
     * @param aMessageList A wxArrayString to store error messages.
     * @return The number of errors found.
     */
    int CheckAnnotation( wxArrayString* aMessageList );

    /**
     * Function sortByXCoordinate
     * sorts the list of references by X position.
     * <p>
     * Components are sorted as follows:
     * <ul>
     * <li>Numeric value of reference designator.</li>
     * <li>Sheet number.</li>
     * <li>X coordinate position.</li>
     * <li>Y coordinate position.</li>
     * <li>Time stamp.</li>
     * </ul>
     * </p>
     */
    void SortByXCoordinate()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortByXPosition );
    }

    /**
     * Function sortByYCoordinate
     * sorts the list of references by Y position.
     * <p>
     * Components are sorted as follows:
     * <ul>
     * <li>Numeric value of reference designator.</li>
     * <li>Sheet number.</li>
     * <li>Y coordinate position.</li>
     * <li>X coordinate position.</li>
     * <li>Time stamp.</li>
     * </ul>
     * </p>
     */
    void SortByYCoordinate()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortByYPosition );
    }

    /**
     * Function SortComponentsByTimeStamp
     * sort the flat list by Time Stamp.
     * Useful to detect duplicate Time Stamps
     */
    void SortByTimeStamp()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortByTimeStamp );
    }

    /**
     * Function SortByRefAndValue
     * sorts the list of references by value.
     * <p>
     * Components are sorted in the following order:
     * <ul>
     * <li>Numeric value of reference designator.</li>
     * <li>Value of component.</li>
     * <li>Unit number when component has multiple parts.</li>
     * <li>Sheet number.</li>
     * <li>X coordinate position.</li>
     * <li>Y coordinate position.</li>
     * </ul>
     * </p>
     */
    void SortByRefAndValue()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortByRefAndValue );
    }

    /**
     * Function SortByReferenceOnly
     * sorts the list of references by reference.
     * <p>
     * Components are sorted in the following order:
     * <ul>
     * <li>Numeric value of reference designator.</li>
     * <li>Unit number when component has multiple parts.</li>
     * </ul>
     * </p>
     */
    void SortByReferenceOnly()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortByReferenceOnly );
    }

    /**
     * Function GetUnit
     * searches the sorted list of components for a another component with the same
     * reference and a given part unit.  Use this method to manage components with
     * multiple parts per package.
     * @param aIndex = index in aComponentsList for of given SCH_REFERENCE item to test.
     * @param aUnit = the given unit number to search
     * @return index in aComponentsList if found or -1 if not found
     */
    int FindUnit( size_t aIndex, int aUnit );

    /**
     * Function ResetHiddenReferences
     * clears the annotation for all references that have an invisible reference designator.
     * Invisible reference designators always have # as the first letter.
     */
    void ResetHiddenReferences();

    /**
     * Function GetRefsInUse
     * adds all the reference designator numbers greater than \a aMinRefId to \a aIdList
     * skipping the reference at \a aIndex.
     * @param aIndex = the current component index to use for reference prefix filtering.
     * @param aIdList = the buffer to fill
     * @param aMinRefId = the min id value to store. all values < aMinRefId are ignored
     */
    void GetRefsInUse( int aIndex, std::vector< int >& aIdList, int aMinRefId );

    /**
     * Function GetLastReference
     * returns the last used (greatest) reference number in the reference list
     * for the prefix reference given by \a aIndex.  The component list must be
     * sorted.
     *
     * @param aIndex The index of the reference item used for the search pattern.
     * @param aMinValue The minimum value for the current search.
     */
    int GetLastReference( int aIndex, int aMinValue );

private:
    /* sort functions used to sort componentFlatList
    */

    static bool sortByRefAndValue( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByXPosition( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByYPosition( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByTimeStamp( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByReferenceOnly( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    /**
     * Function CreateFirstFreeRefId
     * searches for the first free reference number in \a aListId of reference numbers in use.
     * This function just searches for a hole in a list of incremented numbers, this list must
     * be sorted by increasing values and each value can be stored only once.  The new value
     * is added to the list.
     * @see BuildRefIdInUseList to prepare this list
     * @param aIdList The buffer that contains the reference numbers in use.
     * @param aFirstValue The first expected free value
     * @return The first free (not yet used) value.
     */
    int CreateFirstFreeRefId( std::vector<int>& aIdList, int aFirstValue );
};


/**
 * Class BOM_LABEL
 * is used to build a List of Labels by handling the list of labels in schematic because in a
 * complex hierarchy, a label is used more than once and has more than one sheet path
 * so we must create a flat list of labels.
 */
class BOM_LABEL
{
    KICAD_T        m_type;
    SCH_ITEM*      m_label;

    // have to store it here since the object references will be duplicated.
    SCH_SHEET_PATH m_sheetPath;  //composed of UIDs

    static const SCH_SHEET_PATH emptySheetPath;

public:
    BOM_LABEL( KICAD_T aType = TYPE_NOT_INIT, SCH_ITEM* aLabel = NULL,
               const SCH_SHEET_PATH& aSheetPath = emptySheetPath )
        : m_type( aType )
        , m_label( aLabel )
        , m_sheetPath( aSheetPath )
    {
    }

    KICAD_T GetType() const { return m_type; }

    const SCH_ITEM* GetLabel() const { return m_label; }

    const SCH_SHEET_PATH& GetSheetPath() const { return m_sheetPath; }

    wxString GetText() const
    {
        const SCH_TEXT* tmp = (SCH_TEXT*) m_label;
        return tmp->GetText();
    }
};


typedef std::vector <BOM_LABEL> BOM_LABEL_LIST;

#endif    // _NETLIST_H_
