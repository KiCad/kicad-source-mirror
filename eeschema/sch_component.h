/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file sch_component.h
 * @brief Definition the SCH_COMPONENT class for Eeschema.
 */

#ifndef COMPONENT_CLASS_H
#define COMPONENT_CLASS_H

#include <lib_id.h>

#include <sch_field.h>
#include <transform.h>
#include <general.h>
#include <vector>
#include <lib_draw_item.h>

class SCH_SCREEN;
class SCH_SHEET_PATH;
class LIB_ITEM;
class LIB_PIN;
class LIB_PART;
class NETLIST_OBJECT_LIST;
class PART_LIB;
class PART_LIBS;
class SCH_COLLECTOR;
class SCH_SCREEN;
class SYMBOL_LIB_TABLE;


/// A container for several SCH_FIELD items
typedef std::vector<SCH_FIELD>    SCH_FIELDS;

typedef std::weak_ptr<LIB_PART>   PART_REF;


extern std::string toUTFTildaText( const wxString& txt );


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

    ///< Name and library where symbol was loaded from, i.e. "74xx:74LS00".
    LIB_ID      m_lib_id;

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
    std::vector<wxPoint> m_Pins;

    AUTOPLACED  m_fieldsAutoplaced; ///< indicates status of field autoplacement

    /**
     * A temporary sheet path is required to generate the correct reference designator string
     * in complex hierarchies.  Hopefully this is only a temporary hack to decouple schematic
     * objects from the drawing window until a better design for handling complex hierarchies
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
     * Clones \a aComponent into a new schematic symbol object.
     *
     * All fields are copied as is except for the linked list management pointers
     * which are set to NULL, and the SCH_FIELD's m_Parent pointers which are set
     * to the new object.
     *
     * @param aComponent is the schematic symbol to clone.
     */
    SCH_COMPONENT( const SCH_COMPONENT& aComponent );

    ~SCH_COMPONENT() { }

    wxString GetClass() const override
    {
        return wxT( "SCH_COMPONENT" );
    }

    const wxArrayString& GetPathsAndReferences() const { return m_PathsAndReferences; }

    /**
     * Return true for items which are moved with the anchor point at mouse cursor
     * and false for items moved with no reference to anchor.
     *
     * Usually return true for small items (labels, junctions) and false for items which can
     * be large (hierarchical sheets, components).
     *
     * @return false for a component
     */
    bool IsMovableFromAnchorPoint() override { return false; }

    void SetLibId( const LIB_ID& aName, PART_LIBS* aLibs=NULL );
    void SetLibId( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aSymLibTable, PART_LIB* aCacheLib );

    const LIB_ID& GetLibId() const        { return m_lib_id; }

    PART_REF& GetPartRef() { return m_part; }

    /**
     * Return information about the aliased parts
     */
    wxString GetAliasDescription() const;

    /**
     * Return the documentation text for the given part alias
     */
    wxString GetAliasDocumentation() const;

    /**
     * Assigns the current #LIB_PART from \a aLibs which this symbol is based on.
     *
     * @param aLibs is the current set of LIB_PARTs to choose from.
     */
    bool Resolve( PART_LIBS* aLibs );

    bool Resolve( SYMBOL_LIB_TABLE& aLibTable, PART_LIB* aCacheLib = NULL );

    static void ResolveAll( const SCH_COLLECTOR& aComponents, PART_LIBS* aLibs );

    static void ResolveAll( const SCH_COLLECTOR& aComponents, SYMBOL_LIB_TABLE& aLibTable,
                            PART_LIB* aCacheLib = NULL );

    int GetUnit() const { return m_unit; }

    /**
     * Updates the local cache of pin positions
     */
    void UpdatePinCache();

    /**
     * Update the pin cache for all components in \a aComponents
     *
     * @param aComponents collector of components in screen
     */
    static void UpdateAllPinCaches( const SCH_COLLECTOR& aComponents );

    /**
     * Change the unit number to \a aUnit
     *
     * This has meaning only for symbols made up of multiple units per package.
     *
     * @note This also set the modified flag bit
     *
     * @param aUnit is the new unit to select.
     */
    void SetUnit( int aUnit );

    /**
     * Change the unit number to \a aUnit without setting any internal flags.
     * This has meaning only for symbols made up of multiple units per package.
     *
     * @note This also set the modified flag bit
     *
     * @param aUnit is the new unit to select.
     */
    void UpdateUnit( int aUnit );

    int GetConvert() const { return m_convert; }

    void SetConvert( int aConvert );

    wxString GetPrefix() const { return m_prefix; }

    void SetPrefix( const wxString& aPrefix ) { m_prefix = aPrefix; }

    TRANSFORM& GetTransform() const { return const_cast< TRANSFORM& >( m_transform ); }

    void SetTransform( const TRANSFORM& aTransform );

    /**
     * Return the number of units per package of the symbol.
     *
     * @return the number of units per package or zero if the library entry cannot be found.
     */
    int GetUnitCount() const;

    /**
     * Compute the new transform matrix based on \a aOrientation for the symbol which is
     * applied to the current transform.
     *
     * @param aOrientation is the orientation to apply to the transform.
     */
    void SetOrientation( int aOrientation );

    /**
     * Get the display symbol orientation.
     *
     * Because there are different ways to have a given orientation/mirror,
     * the orientation/mirror is not necessary what the user does.  For example:
     * a mirrorX then a mirrorY returns no mirror but a rotate.  This function finds
     * a rotation and a mirror value #CMP_MIRROR_X because this is the first mirror
     * option tested.  This can differs from the orientation made by an user.  A
     * #CMP_MIRROR_Y is returned as a #CMP_MIRROR_X with an orientation 180 because
     * they are equivalent.
     *
     * @sa COMPONENT_ORIENTATION_T
     *
     * @return the orientation and mirror of the symbol.
     */
    int GetOrientation();

    /**
     * Returns the coordinate points relative to the orientation of the symbol to \a aPoint.
     *
     * The coordinates are always relative to the anchor position of the component.
     *
     * @param aPoint The coordinates to transform.
     *
     * @return The transformed point.
     */
    wxPoint GetScreenCoord( const wxPoint& aPoint );

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList ) override;

    /**
     * Clear exiting component annotation.
     *
     * For example, IC23 would be changed to IC? and unit number would be reset.
     *
     * @param aSheetPath is the hierarchical path of the symbol to clear or remove all
     *                   annotations for this symbol if NULL.
     */
    void ClearAnnotation( SCH_SHEET_PATH* aSheetPath );

    /**
     * Change the time stamp to \a aNewTimeStamp and updates the reference path.
     *
     * @see m_PathsAndReferences
     *
     * @param aNewTimeStamp = new time stamp
     */
    void SetTimeStamp( timestamp_t aNewTimeStamp );

    const EDA_RECT GetBoundingBox() const override;

    /**
     * Return a bounding box for the symbol body but not the fields.
     */
    EDA_RECT GetBodyBoundingBox() const;


    //-----<Fields>-----------------------------------------------------------

    /**
     * Returns a field in this symbol.
     *
     * @param aFieldNdx is the index into the array of fields, not a field id.
     *
     * @return is the field at \a aFieldNdx or NULL if the field does not exist.
     */
    SCH_FIELD* GetField( int aFieldNdx ) const;

    /**
     * Search for a field named \a aFieldName and returns text associated with this field.
     *
     * @param aFieldName is the name of the field
     * @param aIncludeDefaultFields is used to search the default library symbol fields in the
     *                              search.
     */
    wxString GetFieldText( wxString aFieldName, bool aIncludeDefaultFields = true ) const;

    /**
     * Populates a std::vector with SCH_FIELDs.
     *
     * @param aVector is the vector to populate.
     * @param aVisibleOnly is used to add only the fields that are visible and contain text.
     */
    void GetFields( std::vector<SCH_FIELD*>& aVector, bool aVisibleOnly );

    /**
     * Add a field to the symbol.
     *
     * @param aField is the field to add to this symbol.
     *
     * @return the newly inserted field.
     */
    SCH_FIELD* AddField( const SCH_FIELD& aField );

    /**
     * Search for a #SCH_FIELD with \a aFieldName
     *
     * @param aFieldName is the name of the field to find.
     * @param aIncludeDefaultFields searches the library symbol default fields if true.
     *
     * @return the field if found or NULL if the field was not found.
     */
    SCH_FIELD* FindField( const wxString& aFieldName, bool aIncludeDefaultFields = true );

    /**
     * Set multiple schematic fields.
     *
     * @param aFields are the fields to set in this symbol.
     */
    void SetFields( const SCH_FIELDS& aFields )
    {
        m_Fields = aFields;     // vector copying, length is changed possibly
    }

    /**
     * Restores fields to the original library values.
     * @param aResetStyle selects whether fields should reset the position and text attribute.
     * @param aResetRef selects whether the reference field should be restored.
     */
    void UpdateFields( bool aResetStyle, bool aResetRef = false );

    /**
     * Return the number of fields in this symbol.
     */
    int GetFieldCount() const { return (int)m_Fields.size(); }

    /**
     * Return whether the fields have been automatically placed.
     */
    AUTOPLACED GetFieldsAutoplaced() const { return m_fieldsAutoplaced; }

    /**
     * Set fields automatically placed flag false.
     */
    void ClearFieldsAutoplaced() { m_fieldsAutoplaced = AUTOPLACED_NO; }

    /**
     * Automatically orient all the fields in the component.
     *
     * @param aScreen is the SCH_SCREEN associated with the current instance of the
     * component. This can be NULL when aManual is false.
     * @param aManual should be true if the autoplace was manually initiated (e.g. by a hotkey
     *  or a menu item). Some more 'intelligent' routines will be used that would be
     *  annoying if done automatically during moves.
     */
    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual );

    /**
     * Autoplace fields only if correct to do so automatically.
     *
     * Fields that have been moved by hand are not automatically placed.
     *
     * @param aScreen is the SCH_SCREEN associated with the current instance of the
     *                component.
     */
    void AutoAutoplaceFields( SCH_SCREEN* aScreen )
    {
        if( GetFieldsAutoplaced() )
            AutoplaceFields( aScreen, GetFieldsAutoplaced() == AUTOPLACED_MANUAL );
    }


    //-----</Fields>----------------------------------------------------------


    /**
     * Find a symbol pin by number.
     *
     * @param number is the number of the pin to find.
     *
     * @return Pin object if found, otherwise NULL.
     */
    LIB_PIN* GetPin( const wxString& number );

    /**
     * Populate a vector with all the pins.
     *
     * @param aPinsList is the list to populate with all of the pins.
     */
    void GetPins( std::vector<LIB_PIN*>& aPinsList );

    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
               GR_DRAWMODE aDrawMode, COLOR4D aColor = COLOR4D::UNSPECIFIED ) override
    {
        Draw( aPanel, aDC, aOffset, aDrawMode, aColor, true );
    }

    /**
     * Draw a component with or without pin text
     *
     * @param aPanel is the panel to use (can be null) mainly used for clipping purposes.
     * @param aDC is the device context (can be null)
     * @param aOffset is the drawing offset (usually wxPoint(0,0),
     *  but can be different when moving an object)
     * @param aDrawMode is the drawing mode, GR_OR, GR_XOR, ...
     * @param aColor use COLOR4D::UNSPECIFIED for the normal body item color or use this
     *               color if >= 0
     * @param aDrawPinText use true to draw pin texts, false to draw only the pin shape
     *  usually false to draw a component when moving it and true otherwise.
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
               GR_DRAWMODE aDrawMode, COLOR4D aColor,
               bool aDrawPinText );

    void SwapData( SCH_ITEM* aItem ) override;

    // returns a unique ID, in the form of a path.
    wxString GetPath( const SCH_SHEET_PATH* sheet ) const;

    /**
     * Tests for an acceptable reference string.
     *
     * An acceptable reference string must support unannotation i.e starts by letter
     *
     * @param aReferenceString is the reference string to validate
     *
     * @return true if reference string is valid.
     */
    static bool IsReferenceStringValid( const wxString& aReferenceString );

    void SetCurrentSheetPath( const SCH_SHEET_PATH* aSheetPath )
    {
        m_currentSheetPath = aSheetPath;
    }

    /**
     * Return the reference for the given sheet path.
     *
     * @return the reference for the sheet.
     */
    const wxString GetRef( const SCH_SHEET_PATH* aSheet );

    /**
     * Set the reference for the given sheet path for this symbol.
     *
     * @param aSheet is the hierarchical path of the reference.
     * @param aReference is the new reference for the symbol.
     */
    void SetRef( const SCH_SHEET_PATH* aSheet, const wxString& aReference );

    /**
     * Add a full hierarchical reference to this symbol.
     *
     * @param aPath is the hierarchical path (/&ltsheet timestamp&gt/&ltcomponent
     *              timestamp&gt like /05678E50/A23EF560)
     * @param aRef is the local reference like C45, R56
     * @param aMulti is the unit selection used for symbols with multiple units per package.
     */
    void AddHierarchicalReference( const wxString& aPath,
                                   const wxString& aRef,
                                   int             aMulti );

    // returns the unit selection, for the given sheet path.
    int GetUnitSelection( SCH_SHEET_PATH* aSheet );

    // Set the unit selection, for the given sheet path.
    void SetUnitSelection( SCH_SHEET_PATH* aSheet, int aUnitSelection );

    // Geometric transforms (used in block operations):

    void Move( const wxPoint& aMoveVector ) override
    {
        if( aMoveVector == wxPoint( 0, 0 ) )
            return;

        m_Pos += aMoveVector;

        for( int ii = 0; ii < GetFieldCount(); ii++ )
            GetField( ii )->Move( aMoveVector );

        SetModified();
    }

    void MirrorY( int aYaxis_position ) override;

    void MirrorX( int aXaxis_position ) override;

    void Rotate( wxPoint aPosition ) override;

    bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation ) override;

    void GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList ) override;

    /**
     * Test if the component's dangling state has changed for all pins.
     *
     * As a side effect, actually update the dangling status for all pins.
     *
     * @note This does not test for  short circuits.
     *
     * @param aItemList is list of all #DANGLING_END_ITEM items to be tested.
     *
     * @return true if any pin's state has changed.
     */
    bool IsDanglingStateChanged( std::vector<DANGLING_END_ITEM>& aItemList ) override;

    /**
     * Return whether any pin in this symbol is dangling.
     *
     * @note This does not update the internal status.  It only checks the existing status.
     *
     * @return true if any pins of this symbol are not connect otherwise false.
     */
    bool IsDangling() const override;

    wxPoint GetPinPhysicalPosition( LIB_PIN* Pin );

    bool IsSelectStateChanged( const wxRect& aRect ) override;

    bool IsConnectable() const override { return true; }

    /**
     * @return true if the component is in netlist
     * which means this is not a power component, or something
     * like a component reference starting by #
     */
    bool IsInNetlist() const;

    void GetConnectionPoints( std::vector<wxPoint>& aPoints ) const override;

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    /**
     * Return the component library item at \a aPosition that is part of this component.
     *
     * @param aPosition is the schematic position of the component library object.
     * @param aType is the type of symbol library object to find or any if set to TYPE_NOT_INIT.
     *
     * @return is the symbol library object if found otherwise NULL.
     */
    LIB_ITEM* GetDrawItem( const wxPoint& aPosition, KICAD_T aType = TYPE_NOT_INIT );

    wxString GetSelectMenuText() const override;

    BITMAP_DEF GetMenuImage() const override;

    void GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                         SCH_SHEET_PATH*      aSheetPath ) override;

    bool operator <( const SCH_ITEM& aItem ) const override;

    bool operator==( const SCH_COMPONENT& aComponent) const;
    bool operator!=( const SCH_COMPONENT& aComponent) const;

    SCH_ITEM& operator=( const SCH_ITEM& aItem );

    bool IsReplaceable() const override { return true; }

    wxPoint GetPosition() const override { return m_Pos; }

    void SetPosition( const wxPoint& aPosition ) override { Move( aPosition - m_Pos ); }

    bool HitTest( const wxPoint& aPosition, int aAccuracy ) const override;

    bool HitTest( const EDA_RECT& aRect, bool aContained = false, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) override;

    EDA_ITEM* Clone() const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

private:
    bool doIsConnected( const wxPoint& aPosition ) const override;
};


#endif /* COMPONENT_CLASS_H */
