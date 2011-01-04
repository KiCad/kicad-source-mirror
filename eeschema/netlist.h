/***************/
/*  netlist.h  */
/***************/

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 Kicad Developers, see change_log.txt for contributors.
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


#ifndef _NETLIST_H_
#define _NETLIST_H_


#include "macros.h"

#include "class_libentry.h"
#include "sch_sheet_path.h"


class SCH_COMPONENT;


#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"

#define ISBUS 1

/* Max pin number per component and footprint */
#define MAXPIN 5000

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


public:
    SCH_COMPONENT* m_RootCmp;           // the component in schematic
    LIB_COMPONENT* m_Entry;             // the source component in library
    int            m_Unit;              /* Selected part (For multi parts per
                                        * package) depending on sheet path */
    wxPoint        m_CmpPos;            // The physical position of the component in schematic
                                        // used to annotate by Y ou Y position
    SCH_SHEET_PATH m_SheetPath;         /* the sheet path for this component */
    int            m_SheetNum;          // the sheet num for this component
    unsigned long  m_TimeStamp;         /* unique identification number
                                         * depending on sheet path */
    bool           m_IsNew;             /* true for not yet annotated
                                         * components */
    wxString*      m_Value;             /* Component value (same for all
                                         * instances) */
    int            m_NumRef;            /* Reference number (for IC1, this is
                                         * 1) ) depending on sheet path*/
    int            m_Flag;              /* flag for computations */

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
        m_Ref =  CONV_TO_UTF8( aReference );
    }
    wxString GetRef() const
    {
        return CONV_FROM_UTF8( m_Ref.c_str() );
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
        return m_Value->CmpNoCase( *item.m_Value );
    }


    int CompareRef( const SCH_REFERENCE& item ) const
    {
        return m_Ref.compare( item.m_Ref );
    }


    bool IsPartsLocked()
    {
        return m_Entry->UnitsLocked();
    }
};

/* object used in annotation to handle a list of components in schematic
 * because in a complex hierarchy, a component is used more than once,
 * and its reference is depending on the sheet path
 * for the same component, we must create a flat list of components
 * used in nelist generation, BOM generation and annotation
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
    SCH_REFERENCE& GetItem(int aIdx)
    {
        return componentFlatList[aIdx];
    }

    /**
     * Function AddItem
     * Add a OBJ_CMP_TO_LIST object in aComponentsList for each component found
     * in sheet
     * @param aItem - a SCH_REFERENCE item to add
     */
    void AddItem( SCH_REFERENCE& aItem )
    {
        componentFlatList.push_back( aItem);
    }

    /**
     * Function RemoveSubComponentsFromList
     * Remove sub components from the list, when multiples parts per package are
     * found in this list.
     * Useful to create BOM, when a component must appear only once
     */
    void RemoveSubComponentsFromList( );

    /* Sort functions:
     * Sort functions are used to sort components for annotatioon or BOM generation.
     * Because sorting depend on we want to do, there are many sort functions.
     * Note:
     *    When creating BOM, components are fully annotated.
     *    references are somethink like U3, U5 or R4, R8
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
     * Update the reference components for the schematic project (or the current sheet)
     * Note: this function does not calculate the reference numbers
     * stored in m_NumRef
     * So, it must be called after calcultaion of new reference numbers
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
     * Function SortCmpByXCoordinate
     * sort the flat list by X coordinates.
     * The list is always sorted first by ref and sheet
     */
    void SortCmpByXCoordinate()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortBy_X_Position );
    }

    /**
     * Function SortCmpByYCoordinate
     * sort the flat list by Y coordinates.
     * The list is always sorted first by ref and sheet
     */
    void SortCmpByYCoordinate()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortBy_Y_Position );
    }

    /**
     * Function SortComponentsByTimeStamp
     * sort the flat list by Time Stamp.
     * Useful to detect duplicate Time Stamps
     */
    void SortComponentsByTimeStamp()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortByTimeStamp );
    }

    /**
     * Function SortComponentsByValue
     * sort the flat list by Value.
     * Values are sorted by numeric values, not by alpahbetic order
     * The list is always sorted first by ref
     */
    void SortComponentsByRefAndValue()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortByRefAndValue );
    }

    /**
     * Function SortComponentsByReferenceOnly
     * sort the flat list by references
     * For BOM, sorted by reference
     */
    void SortComponentsByReferenceOnly()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortComponentsByReferenceOnly );
    }

    /**
     * Function SortComponentsByValueOnly
     * sort the flat list by references
     * For BOM, sorted by values
     */
    void SortComponentsByValueOnly()
    {
        sort( componentFlatList.begin(), componentFlatList.end(), sortComponentsByValueOnly );
    }

private:
    /* sort functions used to sort componentFlatList
    */

    /**
     * Function sortByRefAndValue
     * sort function to annotate items by value
     *  Components are sorted
     *      by reference (when used, referenc is only U ot R, with no number)
     *      if same reference: by value
     *          if same value: by unit number
     *              if same unit number, by sheet
     *                  if same sheet, by position X, and Y
     * @param item1, item2 = SCH_REFERENCE items to compare
     */
    static bool sortByRefAndValue( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );
    /**
     * Function sortBy_X_Position
     * sort function to annotate items from their position.
     *  Components are sorted
     *      by reference (when used, referenc is only U ot R, with no number)
     *      if same reference: by sheet
     *          if same sheet, by X pos
     *                if same X pos, by Y pos
     *                  if same Y pos, by time stamp
     * @param item1, item2 = SCH_REFERENCE items to compare
     */
    static bool sortBy_X_Position( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    /**
     * Function sortBy_Y_Position
     * sort function to annotate items from their position.
     *  Components are sorted
     *      by reference (when used, referenc is only U ot R, with no number)
     *      if same reference: by sheet
     *          if same sheet, by Y pos
     *                if same Y pos, by X pos
     *                  if same X pos, by time stamp
     * @param item1, item2 = SCH_REFERENCE items to compare
     */
    static bool sortBy_Y_Position( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByTimeStamp( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    /**
     * Function sortComponentsByValueOnly
     * compare function for sorting in BOM creation.
     * components are sorted
     *     by value
     *     if same value: by reference
     *         if same reference: by unit number
     * @param item1, item2 = SCH_REFERENCE items to compare
     */

    static bool sortComponentsByValueOnly( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );
    /**
     * Function sortComponentsByReferenceOnly
     * compare function for sorting in BOM creation.
     * components are sorted
     *     by reference
     *     if same reference: by value (happens only for multi parts per package)
     *         if same value: by unit number
     * @param item1, item2 = SCH_REFERENCE items to compare
     */
    static bool sortComponentsByReferenceOnly( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );
};

/**
 * helper Class LABEL_OBJECT
 * is used in build BOM to handle the list of labels in schematic
 * because in a complex hierarchy, a label is used more than once,
 * and had more than one sheet path, so we must create a flat list of labels
 */
class LABEL_OBJECT
{
public:
    int            m_LabelType;
    SCH_ITEM*      m_Label;

    //have to store it here since the object references will be duplicated.
    SCH_SHEET_PATH m_SheetPath;  //composed of UIDs

public: LABEL_OBJECT()
    {
        m_Label     = NULL;
        m_LabelType = 0;
    }
};
typedef std::vector <LABEL_OBJECT> LABEL_OBJECT_LIST;

#endif
