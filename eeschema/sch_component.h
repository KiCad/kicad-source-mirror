/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_component.h
 * @brief Definition the SCH_COMPONENT class for Eeschema.
 */

#ifndef COMPONENT_CLASS_H
#define COMPONENT_CLASS_H


#include <sch_field.h>
#include <transform.h>
#include <general.h>
#include <boost/weak_ptr.hpp>
#include <vector>
#include <lib_draw_item.h>

class SCH_SHEET_PATH;
class LIB_ITEM;
class LIB_PIN;
class LIB_PART;
class NETLIST_OBJECT_LIST;
class LIB_PART;
class PART_LIBS;
class SCH_COLLECTOR;


/// A container for several SCH_FIELD items
typedef std::vector<SCH_FIELD>      SCH_FIELDS;

typedef boost::weak_ptr<LIB_PART>   PART_REF;


/**
 * Class SCH_COMPONENT
 * describes a real schematic component
 */
class SCH_COMPONENT : public SCH_ITEM
{
    friend class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC;

public:
    enum AUTOPLACED { AUTOPLACED_NO = 0, AUTOPLACED_AUTO, AUTOPLACED_MANUAL };
private:

    wxPoint     m_Pos;
    wxString    m_part_name;    ///< Name to look for in the library, i.e. "74LS00".

    int         m_unit;         ///< The unit for multiple part per package components.
    int         m_convert;      ///< The alternate body style for components that have more than
                                ///< one body style defined.  Primarily used for components that
                                ///< have a De Morgan conversion.
    wxString    m_prefix;       ///< C, R, U, Q etc - the first character which typically indicates
                                ///< what the component is. Determined, upon placement, from the
                                ///< library component.  Created upon file load, by the first
                                ///<  non-digits in the reference fields.
    TRANSFORM   m_transform;    ///< The rotation/mirror transformation matrix.
    SCH_FIELDS  m_Fields;       ///< Variable length list of fields.

    PART_REF    m_part;         ///< points into the PROJECT's libraries to the LIB_PART for this component

    std::vector<bool> m_isDangling; ///< One isDangling per pin

    AUTOPLACED  m_fieldsAutoplaced; ///< indicates status of field autoplacement

    /**
     * A temporary sheet path is required to generate the correct reference designator string
     * in complex heirarchies.  Hopefully this is only a temporary hack to decouple schematic
     * objects from the drawing window until a better design for handling complex heirarchies
     * can be implemented.
     */
    const SCH_SHEET_PATH* m_currentSheetPath;

    /**
     * Defines the hierarchical path and reference of the component.  This allows support
     * for hierarchical sheets that reference the same schematic.  The format for the path
     * is /&ltsheet time stamp&gt/&ltsheet time stamp&gt/.../&lscomponent time stamp&gt.
     * A single / denotes the root sheet.
     */
    wxArrayString m_PathsAndReferences;

    void Init( const wxPoint& pos = wxPoint( 0, 0 ) );

public:
    SCH_COMPONENT( const wxPoint& pos = wxPoint( 0, 0 ), SCH_ITEM* aParent = NULL );

    /**
     * Create schematic component from library component object.
     *
     * @param aPart - library part to create schematic component from.
     * @param aSheet - Schematic sheet the component is place into.
     * @param unit - Part for components that have multiple parts per
     *               package.
     * @param convert - Use the alternate body style for the schematic
     *                  component.
     * @param pos - Position to place new component.
     * @param setNewItemFlag - Set the component IS_NEW and IS_MOVED flags.
     */
    SCH_COMPONENT( LIB_PART& aPart, SCH_SHEET_PATH* aSheet,
                   int unit = 0, int convert = 0,
                   const wxPoint& pos = wxPoint( 0, 0 ),
                   bool setNewItemFlag = false );

    /**
     * Copy Constructor
     * clones \a aComponent into a new object.  All fields are copied as is except
     * for the linked list management pointers which are set to NULL, and the
     * SCH_FIELD's m_Parent pointers which are set to the new parent,
     * i.e. this new object.
     */
    SCH_COMPONENT( const SCH_COMPONENT& aComponent );

    ~SCH_COMPONENT() { }

    wxString GetClass() const
    {
        return wxT( "SCH_COMPONENT" );
    }

    /**
     * Virtual function IsMovableFromAnchorPoint
     * Return true for items which are moved with the anchor point at mouse cursor
     *  and false for items moved with no reference to anchor
     * Usually return true for small items (labels, junctions) and false for
     * items which can be large (hierarchical sheets, compoments)
     * @return false for a componant
     */
    bool IsMovableFromAnchorPoint() { return false; }

    void SetPartName( const wxString& aName, PART_LIBS* aLibs=NULL );
    const wxString& GetPartName() const        { return m_part_name; }

    /**
     * Function Resolve
     * [re-]assigns the current LIB_PART from aLibs which this component
     * is based on.
     * @param aLibs is the current set of LIB_PARTs to choose from.
     */
    bool Resolve( PART_LIBS* aLibs );

    static void ResolveAll( const SCH_COLLECTOR& aComponents, PART_LIBS* aLibs );

    int GetUnit() const { return m_unit; }

    /**
     * change the unit id to aUnit
     * has maening only for multiple parts per package
     * Also set the modified flag bit
     * @param aUnit = the new unit Id
     */
    void SetUnit( int aUnit );

    /**
     * change the unit id to aUnit
     * has maening only for multiple parts per package
     * Do not change the modified flag bit, and should be used when
     * change is not due to an edition command
     * @param aUnit = the new unit Id
     */
    void UpdateUnit( int aUnit );

    int GetConvert() const { return m_convert; }

    void SetConvert( int aConvert );

    wxString GetPrefix() const { return m_prefix; }

    TRANSFORM& GetTransform() const { return const_cast< TRANSFORM& >( m_transform ); }

    void SetTransform( const TRANSFORM& aTransform );

    /**
     * Function GetUnitCount
     * returns the number of parts per package of the component.
     *
     * @return The number of parts per package or zero if the library entry cannot be found.
     */
    int GetUnitCount() const;

    bool Save( FILE* aFile ) const;

    bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function SetOrientation
     * computes the new transform matrix based on \a aOrientation for the component which is
     * applied to the current transform.
     * @param aOrientation The orientation to apply to the transform.
     */
    void SetOrientation( int aOrientation );

    /**
     * Function GetOrientation
     * Used to display component orientation (in dialog editor or info)
     * @return the orientation and mirror
     * Note: Because there are different ways to have a given orientation/mirror,
     * the orientation/mirror is not necessary what the used does
     * (example : a mirrorX then a mirrorY give no mirror but rotate the
     * component).
     * So this function find a rotation and a mirror value
     * ( CMP_MIRROR_X because this is the first mirror option tested)
     *  but can differs from the orientation made by an user
     * ( a CMP_MIRROR_Y is find as a CMP_MIRROR_X + orientation 180, because
     * they are equivalent)
     */
    int GetOrientation();

    /**
     * Function GetScreenCoord
     * Returns the coordinated point relative to the orientation of the component of \a aPoint.
     * The coordinates are always relative to the anchor position of the component.
     * @param aPoint The coordinates to transform.
     * @return The transformed point.
     */
    wxPoint GetScreenCoord( const wxPoint& aPoint );

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

    /**
     * Function ClearAnnotation
     * clears exiting component annotation ( i.i IC23 changed to IC? and part reset to 1)
     * @param aSheet: SCH_SHEET value: if NULL remove all annotations,
     *                else remove annotation relative to \a aSheet.
     */
    void ClearAnnotation( SCH_SHEET* aSheet );

    /**
     * Function SetTimeStamp
     * changes the time stamp to \a aNewTimeStamp updates the reference path.
     * @see m_PathsAndReferences
     * @param aNewTimeStamp = new time stamp
     */
    void SetTimeStamp( time_t aNewTimeStamp );

    const EDA_RECT GetBoundingBox() const;    // Virtual

    /**
     * Function GetBodyBoundingBox
     * Return a bounding box for the component body but not the fields.
     */
    EDA_RECT GetBodyBoundingBox() const;


    //-----<Fields>-----------------------------------------------------------

    /**
     * Function GetField
     * returns a field.
     * @param aFieldNdx An index into the array of fields, not a field id.
     * @return SCH_FIELD* - the field value or NULL if does not exist
     */
    SCH_FIELD* GetField( int aFieldNdx ) const;

    /**
     * Function GetFields
     * populates a std::vector with SCH_FIELDs.
     * @param aVector - vector to populate.
     * @param aVisibleOnly - if true, only get fields that are visible and contain text.
     */
    void GetFields( std::vector<SCH_FIELD*>& aVector, bool aVisibleOnly );

    /**
     * Function AddField
     * adds a field to the component.  The field is copied as it is put into
     * the component.
     * @param aField A const reference to the SCH_FIELD to add.
     * @return SCH_FIELD* - the newly inserted field.
     */
    SCH_FIELD* AddField( const SCH_FIELD& aField );

    /**
     * Function FindField
     * searches for SCH_FIELD with \a aFieldName and returns it if found, else NULL.
     */
    SCH_FIELD* FindField( const wxString& aFieldName );

    void SetFields( const SCH_FIELDS& aFields )
    {
        m_Fields = aFields;     // vector copying, length is changed possibly
    }
    /**
     * Function GetFieldCount
     * returns the number of fields in this component.
     */
    int GetFieldCount() const { return (int) m_Fields.size(); }

    /**
     * Function GetFieldsAutoplaced
     * returns whether the fields are autoplaced.
     */
    AUTOPLACED GetFieldsAutoplaced() const { return m_fieldsAutoplaced; }

    /**
     * Function ClearFieldsAutoplaced
     * Set fields autoplaced flag false.
     */
    void ClearFieldsAutoplaced() { m_fieldsAutoplaced = AUTOPLACED_NO; }

    /**
     * Function AutoplaceFields
     * Automatically orient all the fields in the component.
     * @param aScreen - the SCH_SCREEN associated with the current instance of the
     *  component. This can be NULL when aManual is false.
     * @param aManual - True if the autoplace was manually initiated (e.g. by a hotkey
     *  or a menu item). Some more 'intelligent' routines will be used that would be
     *  annoying if done automatically during moves.
     */
    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual );

    /**
     * Function AutoAutoplaceFields
     * Autoplace fields only if correct to do so automatically. That is, do not
     * autoplace if fields have been moved by hand.
     * @param aScreen - the SCH_SCREEN associated with the current instance of the
     *  component.
     */
    void AutoAutoplaceFields( SCH_SCREEN* aScreen )
    {
        if( GetFieldsAutoplaced() )
            AutoplaceFields( aScreen, GetFieldsAutoplaced() == AUTOPLACED_MANUAL );
    }


    //-----</Fields>----------------------------------------------------------


    /**
     * Function GetPin
     * finds a component pin by number.
     *
     * @param number - The number of the pin to find.
     * @return Pin object if found, otherwise NULL.
     */
    LIB_PIN* GetPin( const wxString& number );

    /**
     * Function GetPins
     * populate a vector with all the pins.
     */
    void GetPins( std::vector<LIB_PIN*>& aPinsList );

    /**
     * Virtual function, from the base class SCH_ITEM::Draw
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
               GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor = UNSPECIFIED_COLOR )
    {
        Draw( aPanel, aDC, aOffset, aDrawMode, aColor, true );
    }

    /**
     * Function Draw, specific to components.
     * Draw a component, with or without pin texts.
     * @param aPanel DrawPanel to use (can be null) mainly used for clipping purposes.
     * @param aDC Device Context (can be null)
     * @param aOffset drawing Offset (usually wxPoint(0,0),
     *  but can be different when moving an object)
     * @param aDrawMode GR_OR, GR_XOR, ...
     * @param aColor UNSPECIFIED_COLOR to use the normal body item color, or use this color if >= 0
     * @param aDrawPinText = true to draw pin texts, false to draw only the pin shape
     *  usually false to draw a component when moving it, and true otherwise.
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
               GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor,
               bool aDrawPinText );

    void SwapData( SCH_ITEM* aItem );

    // returns a unique ID, in the form of a path determined by \a aSheet.
    wxString GetPath( const SCH_SHEET* sheet ) const;

    /**
     * Function IsReferenceStringValid (static)
     * Tests for an acceptable reference string
     * An acceptable reference string must support unannotation
     * i.e starts by letter
     * @param aReferenceString = the reference string to validate
     * @return true if OK
     */
    static bool IsReferenceStringValid( const wxString& aReferenceString );

    void SetCurrentSheetPath( const SCH_SHEET_PATH* aSheetPath )
    {
        m_currentSheetPath = aSheetPath;
    }

    /**
     * Function GetRef
     * returns the reference, for the given sheet path.
     */
    const wxString GetRef( const SCH_SHEET* sheet );

    /**
     * Set the reference, for the given sheet path.
     */
    void SetRef( const SCH_SHEET* aSheet, const wxString& ref );

    /**
     * Function AddHierarchicalReference
     * adds a full hierarchical reference (path + local reference)
     * @param aPath Hierarchical path (/&ltsheet timestamp&gt/&ltcomponent
     *              timestamp&gt like /05678E50/A23EF560)
     * @param aRef :local reference like C45, R56
     * @param aMulti Part selection, used in multi part per package (0 or 1 for non multi)
     */
    void AddHierarchicalReference( const wxString& aPath,
                                   const wxString& aRef,
                                   int             aMulti );

    // returns the unit selection, for the given sheet.
    int GetUnitSelection( SCH_SHEET* aSheet );

    // Set the unit selection, for the given sheet.
    void SetUnitSelection( SCH_SHEET* aSheet, int aUnitSelection );

    // Geometric transforms (used in block operations):

    void Move( const wxPoint& aMoveVector )
    {
        if( aMoveVector == wxPoint( 0, 0 ) )
            return;

        m_Pos += aMoveVector;

        for( int ii = 0; ii < GetFieldCount(); ii++ )
            GetField( ii )->Move( aMoveVector );

        SetModified();
    }

    void MirrorY( int aYaxis_position );

    void MirrorX( int aXaxis_position );

    void Rotate( wxPoint aPosition );

    bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation );

    void GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList );

    /**
     * Test if the component's dangling state has changed for one given pin index. As
     * a side effect, actually update the dangling status for that pin.
     *
     * @param aItemList - list of all DANGLING_END_ITEMs to be tested
     * @param aLibPins - list of all the LIB_PIN items in this component's symbol
     * @param aPin - index into aLibPins that identifies the pin to test
     * @return true if the pin's state has changed.
     */
    bool IsPinDanglingStateChanged( std::vector<DANGLING_END_ITEM>& aItemList,
            LIB_PINS& aLibPins, unsigned aPin );

    /**
     * Test if the component's dangling state has changed for all pins. As a side
     * effect, actually update the dangling status for all pins (does not short-circuit).
     *
     * @param aItemList - list of all DANGLING_END_ITEMs to be tested
     * @return true if any pin's state has changed.
     */
    bool IsDanglingStateChanged( std::vector<DANGLING_END_ITEM>& aItemList );

    /**
     * Return whether any pin has dangling status. Does NOT update the internal status,
     * only checks the existing status.
     */
    bool IsDangling() const;

    wxPoint GetPinPhysicalPosition( LIB_PIN* Pin );

    bool IsSelectStateChanged( const wxRect& aRect );

    bool IsConnectable() const { return true; }

    /**
     * @return true if the component is in netlist
     * which means this is not a power component, or something
     * like a component reference starting by #
     */
    bool IsInNetlist() const;

    void GetConnectionPoints( std::vector<wxPoint>& aPoints ) const;

    SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                                 const KICAD_T scanTypes[] );

    /**
     * Function GetDrawItem().
     * Return the component library item at \a aPosition that is part of this component.
     *
     * @param aPosition - Schematic position of the component library object.
     * @param aType - Type of component library object to find or any if set to TYPE_NOT_INIT.
     * @return A pointer to the component library object if found, otherwise NULL.
     */
    LIB_ITEM* GetDrawItem( const wxPoint& aPosition, KICAD_T aType = TYPE_NOT_INIT );

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  add_component_xpm; }

    void GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                         SCH_SHEET_PATH*      aSheetPath );

    bool operator <( const SCH_ITEM& aItem ) const;

    bool operator==( const SCH_COMPONENT& aComponent) const;
    bool operator!=( const SCH_COMPONENT& aComponent) const;

    SCH_ITEM& operator=( const SCH_ITEM& aItem );

    bool IsReplaceable() const { return true; }

    wxPoint GetPosition() const { return m_Pos; }

    void SetPosition( const wxPoint& aPosition ) { Move( aPosition - m_Pos ); }

    bool HitTest( const wxPoint& aPosition, int aAccuracy ) const;

    bool HitTest( const EDA_RECT& aRect, bool aContained = false, int aAccuracy = 0 ) const;

    void Plot( PLOTTER* aPlotter );

    EDA_ITEM* Clone() const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // override
#endif

private:
    bool doIsConnected( const wxPoint& aPosition ) const;
};


#endif /* COMPONENT_CLASS_H */
