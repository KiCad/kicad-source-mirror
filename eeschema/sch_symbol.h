/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <eda_item.h>
#include <core/typeinfo.h>
#include <layers_id_colors_and_visibility.h>
#include <lib_id.h>
#include <widgets/msgpanel.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wx/arrstr.h>
#include <wx/chartype.h>
#include <wx/fdrepdlg.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

#include <sch_field.h>
#include <sch_item.h>
#include <sch_pin.h>
#include <sch_sheet_path.h>    // SYMBOL_INSTANCE_REFERENCE
#include <symbol_lib_table.h>
#include <transform.h>

struct PICKED_SYMBOL;
class SCH_SCREEN;
class LIB_ITEM;
class LIB_PIN;
class LIB_SYMBOL;
class NETLIST_OBJECT_LIST;
class SYMBOL_LIB;
class SYMBOL_LIBS;
class EE_COLLECTOR;
class SCH_SCREEN;
class SYMBOL_LIB_TABLE;


/// A container for several SCH_FIELD items
typedef std::vector<SCH_FIELD>    SCH_FIELDS;

typedef std::weak_ptr<LIB_SYMBOL> PART_REF;


extern std::string toUTFTildaText( const wxString& txt );


/**
 * Schematic symbol object.
 */
class SCH_SYMBOL : public SCH_ITEM
{
public:
    SCH_SYMBOL( const wxPoint& pos = wxPoint( 0, 0 ), SCH_ITEM* aParent = nullptr );

    /**
     * Create schematic symbol from library symbol object.
     *
     * @param aSymbol is the library symbol to create schematic symbol from.
     * @param aLibId is the #LIB_ID of alias to create.
     * @param aSheet is the schematic sheet the symbol is place into.
     * @param unit is unit for symbols that have multiple parts per package.
     * @param convert is the alternate body style for the schematic symbols.
     * @param pos is the position of the symbol.
     * @param setNewItemFlag is used to set the symbol #IS_NEW and #IS_MOVING flags.
     */
    SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const LIB_ID& aLibId, const SCH_SHEET_PATH* aSheet,
                int unit = 0, int convert = 0, const wxPoint& pos = wxPoint( 0, 0 ) );

    SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const SCH_SHEET_PATH* aSheet, const PICKED_SYMBOL& aSel,
                const wxPoint& pos = wxPoint( 0, 0 ) );

    /**
     * Clone \a aSymbol into a new schematic symbol object.
     *
     * All fields are copied as is except for the linked list management pointers
     * which are set to NULL, and the SCH_FIELD's m_Parent pointers which are set
     * to the new object.
     *
     * @param aSymbol is the schematic symbol to clone.
     */
    SCH_SYMBOL( const SCH_SYMBOL& aSymbol );

    ~SCH_SYMBOL() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_SYMBOL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_SYMBOL" );
    }

    const std::vector<SYMBOL_INSTANCE_REFERENCE>& GetInstanceReferences()
    {
        return m_instanceReferences;
    }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /**
     * Return true for items which are moved with the anchor point at mouse cursor
     * and false for items moved with no reference to anchor.
     *
     * Usually return true for small items (labels, junctions) and false for items which can
     * be large (hierarchical sheets, symbols).
     *
     * @note We used to try and be smart about this and return false for symbols in case
     *       they are big.  However, this annoyed some users and we now have a preference which
     *       controls warping on move in general, so this was switched to true for symbols.
     *
     * @return true for a symbol.
     */
    bool IsMovableFromAnchorPoint() const override { return true; }

    void SetLibId( const LIB_ID& aName );

    const LIB_ID& GetLibId() const { return m_lib_id; }

    /**
     * The name of the symbol in the schematic library symbol list.
     *
     * @note See #SCH_SCREEN::m_libSymbols
     *
     * The name of the schematic symbol list entry can vary from the item name in the #LIB_ID
     * object because the library symbol may have changed so a new name has to be generated
     * but the original symbol library link has to be preserved in order to update it from
     * the library at some point in the future.  If this name is empty, then the library item
     * name from #LIB_ID is used.
     */
    void SetSchSymbolLibraryName( const wxString& aName ) { m_schLibSymbolName = aName; }
    wxString GetSchSymbolLibraryName() const;
    bool UseLibIdLookup() const { return m_schLibSymbolName.IsEmpty(); }

    std::unique_ptr< LIB_SYMBOL >& GetLibSymbolRef() { return m_part; }
    const std::unique_ptr< LIB_SYMBOL >& GetLibSymbolRef() const { return m_part; }

    /**
     * Set this schematic symbol library symbol reference to \a aLibSymbol
     *
     * The schematic symbol object owns \a aLibSymbol and the pin list will be updated
     * accordingly.  The #LIB_SYMBOL object can be null to clear the library symbol link
     * as well as the pin map.  If the #LIB_SYMBOL object is not null, it must be a root
     * symbol.  Otherwise an assertion will be raised in debug builds and the library
     * symbol will be cleared.  The new file format will no longer require a cache
     * library so all library symbols must be valid.
     *
     * @note This is the only way to publicly set the library symbol for a schematic
     *       symbol except for the ctors that take a LIB_SYMBOL reference.  All previous
     *       public resolvers have been deprecated.
     *
     * @param aLibSymbol is the library symbol to associate with this schematic symbol.
     */
    void SetLibSymbol( LIB_SYMBOL* aLibSymbol );

    /**
     * Return information about the aliased parts
     */
    wxString GetDescription() const;

    /**
     * Return the documentation text for the given part alias
     */
    wxString GetDatasheet() const;

    int GetUnit() const { return m_unit; }

    /**
     * Updates the cache of SCH_PIN objects for each pin
     */
    void UpdatePins();

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

    TRANSFORM& GetTransform() { return m_transform; }
    const TRANSFORM& GetTransform() const { return m_transform; }

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
     * a mirrorV then a mirrorH returns no mirror but a rotate.  This function finds
     * a rotation and a mirror value #SYM_MIRROR_X because this is the first mirror
     * option tested.  This can differs from the orientation made by an user.  A
     * #SYM_MIRROR_Y is returned as a #SYM_MIRROR_X with an orientation 180 because
     * they are equivalent.
     *
     * @sa SYMBOL_ORIENTATION_T
     *
     * @return the orientation and mirror of the symbol.
     */
    int GetOrientation();

    /**
     * Return the list of system text vars & fields for this symbol.
     */
    void GetContextualTextVars( wxArrayString* aVars ) const;

    /**
     * Resolve any references to system tokens supported by the symbol.
     *
     * @param aDepth a counter to limit recursion and circular references.
     */
    bool ResolveTextVar( wxString* token, int aDepth = 0 ) const;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * Clear exiting symbol annotation.
     *
     * For example, IC23 would be changed to IC? and unit number would be reset.
     *
     * @param aSheetPath is the hierarchical path of the symbol to clear or remove all
     *                   annotations for this symbol if NULL.
     */
    void ClearAnnotation( const SCH_SHEET_PATH* aSheetPath );

    /**
     * Add an instance to the alternate references list (m_instanceReferences), if this entry
     * does not already exist.
     *
     * Do nothing if already exists. In symbol lists shared by more than one sheet path, an
     * entry for each sheet path must exist to manage references.
     *
     * @param aSheetPath is the candidate sheet path of the sheet containing the symbol not the
     *                   full symbol sheet path.
     * @return false if the alternate reference was existing, true if added.
     */
    bool AddSheetPathReferenceEntryIfMissing( const KIID_PATH& aSheetPath );

    /**
     * Replace \a aOldSheetPath with \a aNewSheetPath in the instance list.
     *
     * @param aOldSheetPath is a #KIID_PATH object of an existing path in the instance list.
     * @param aNewSheetPath is a #KIID_PATH object of the path to replace the existing path.
     *
     * @return true if \a aOldSheetPath was found and replaced or false if \a aOldSheetPath was
     *         not found in the instance list.
     */
    bool ReplaceInstanceSheetPath( const KIID_PATH& aOldSheetPath, const KIID_PATH& aNewSheetPath );

    const EDA_RECT GetBoundingBox() const override;

    const EDA_RECT GetBoundingBox( bool aIncludeInvisibleText ) const;

    /**
     * Return a bounding box for the symbol body but not the fields.
     */
    EDA_RECT GetBodyBoundingBox() const;


    //-----<Fields>-----------------------------------------------------------

    /**
     * Return a mandatory field in this symbol.
     *
     * @note If you need to fetch a user field, use GetFieldById.
     *
     * @param aFieldType is one of the mandatory field types (REFERENCE_FIELD, VALUE_FIELD, etc.).
     * @return is the field at \a aFieldType or NULL if the field does not exist.
     */
    SCH_FIELD* GetField( MANDATORY_FIELD_T aFieldType );
    const SCH_FIELD* GetField( MANDATORY_FIELD_T aFieldNdx ) const;

    /**
     * Return a field in this symbol.
     *
     * @param aFieldId is the id of the field requested.  Note that this id ONLY SOMETIMES equates
     *                 to the field's position in the vector.
     * @return is the field at \a aFieldType or NULL if the field does not exist.
     */
    SCH_FIELD* GetFieldById( int aFieldId );

    /**
     * Search for a field named \a aFieldName and returns text associated with this field.
     *
     * @param aFieldName is the name of the field
     */
    wxString GetFieldText( const wxString& aFieldName, SCH_EDIT_FRAME* aFrame ) const;

    /**
     * Populate a std::vector with SCH_FIELDs.
     *
     * @param aVector is the vector to populate.
     * @param aVisibleOnly is used to add only the fields that are visible and contain text.
     */
    void GetFields( std::vector<SCH_FIELD*>& aVector, bool aVisibleOnly );

    /**
     * Return a vector of fields from the symbol
     */
    std::vector<SCH_FIELD>& GetFields() { return m_fields; }
    const std::vector<SCH_FIELD>& GetFields() const { return m_fields; }

    /**
     * Add a field to the symbol.
     *
     * @param aField is the field to add to this symbol.
     *
     * @return the newly inserted field.
     */
    SCH_FIELD* AddField( const SCH_FIELD& aField );

    /**
     * Remove a user field from the symbol.
     * @param aFieldName is the user fieldName to remove.  Attempts to remove a mandatory
     *                   field or a non-existant field are silently ignored.
     */
    void RemoveField( const wxString& aFieldName );

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
        m_fields = aFields;     // vector copying, length is changed possibly
    }

    /**
     * Restore fields to the original library values.
     *
     * @param aUpdateStyle selects whether fields should update the position and text attributes.
     * @param aUpdateRef selects whether the reference field should be updated.
     * @param aUpdateOtherFields selects whether non-reference fields should be updated.
     * @param aResetRef selects whether the reference should be reset to the library value.
     * @param aResetOtherFields selects whether non-reference fields should be reset to library
     *                          values.
     */
    void UpdateFields( const SCH_SHEET_PATH* aPath, bool aUpdateStyle, bool aUpdateRef,
                       bool aUpdateOtherFields, bool aResetRef, bool aResetOtherFields );

    /**
     * Return the number of fields in this symbol.
     */
    int GetFieldCount() const { return (int)m_fields.size(); }

    /**
     * Automatically orient all the fields in the symbol.
     *
     * @param aScreen is the SCH_SCREEN associated with the current instance of the
     *                symbol. This can be NULL when aManual is false.
     * @param aManual should be true if the autoplace was manually initiated (e.g. by a hotkey
     *                or a menu item). Some more 'intelligent' routines will be used that would be
     *                annoying if done automatically during moves.
     */
    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) override;

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction ) override;


    //-----</Fields>----------------------------------------------------------


    /**
     * Find a symbol pin by number.
     *
     * @param number is the number of the pin to find.
     * @return Pin object if found, otherwise NULL.
     */
    SCH_PIN* GetPin( const wxString& number ) const;

    /**
     * Populate a vector with all the pins from the library object.
     *
     * @param aPinsList is the list to populate with all of the pins.
     */
    void GetLibPins( std::vector<LIB_PIN*>& aPinsList ) const;

    SCH_PIN* GetPin( LIB_PIN* aLibPin );

    /**
     * Retrieve a list of the SCH_PINs for the given sheet path.
     *
     * Since a symbol can have a different unit on a different instance of a sheet,
     * this list returns the subset of pins that exist on a given sheet.
     *
     * @return a vector of pointers (non-owning) to SCH_PINs
     */
    std::vector<SCH_PIN*> GetPins( const SCH_SHEET_PATH* aSheet = nullptr ) const;

    std::vector<std::unique_ptr<SCH_PIN>>& GetRawPins() { return m_pins; }

    /**
     * Print a symbol.
     *
     * @param aDC is the device context (can be null).
     * @param aOffset is the drawing offset (usually wxPoint(0,0), but can be different when
     *                moving an object)
     */
    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    void SwapData( SCH_ITEM* aItem ) override;

    /**
     * Test for an acceptable reference string.
     *
     * An acceptable reference string must support unannotation i.e starts by letter
     *
     * @param aReferenceString is the reference string to validate
     * @return true if reference string is valid.
     */
    static bool IsReferenceStringValid( const wxString& aReferenceString );

    /**
     * Return the reference for the given sheet path.
     *
     * @return the reference for the sheet.
     */
    const wxString GetRef( const SCH_SHEET_PATH* aSheet, bool aIncludeUnit = false ) const;

    /**
     * Set the reference for the given sheet path for this symbol.
     *
     * @param aSheet is the hierarchical path of the reference.
     * @param aReference is the new reference for the symbol.
     */
    void SetRef( const SCH_SHEET_PATH* aSheet, const wxString& aReference );

    /**
     * Check if the symbol has a valid annotation (reference) for the given sheet path.
     *
     * @param aSheet is the sheet path to test.
     * @return true if the symbol exists on that sheet and has a valid reference.
     */
    bool IsAnnotated( const SCH_SHEET_PATH* aSheet );

    /**
     * Add a full hierarchical reference to this symbol.
     *
     * @param aPath is the hierarchical path (/&ltsheet timestamp&gt/&ltsymbol
     *              timestamp&gt like /05678E50/A23EF560).
     * @param aRef is the local reference like C45, R56.
     * @param aUnit is the unit selection used for symbols with multiple units per package.
     * @param aValue is the value used for this instance.
     * @param aFootprint is the footprint used for this instance (which might have different
     *                   hole spacing or other board-specific changes from other instances).
     */
    void AddHierarchicalReference( const KIID_PATH& aPath,
                                   const wxString&  aRef,
                                   int              aUnit,
                                   const wxString&  aValue = wxEmptyString,
                                   const wxString&  aFootprint = wxEmptyString );

    /// Return the instance-specific unit selection for the given sheet path.
    int GetUnitSelection( const SCH_SHEET_PATH* aSheet ) const;

    /// Set the selected unit of this symbol on one sheet.
    void SetUnitSelection( const SCH_SHEET_PATH* aSheet, int aUnitSelection );

    /// Set the selected unit of this symbol for all sheets.
    void SetUnitSelection( int aUnitSelection );

    /// Return the instance-specific value for the given sheet path.
    const wxString GetValue( const SCH_SHEET_PATH* sheet, bool aResolve ) const;
    void SetValue( const SCH_SHEET_PATH* sheet, const wxString& aValue );

    /// Set the value for all instances (the default GUI behavior).
    void SetValue( const wxString& aValue )
    {
        SetValue( nullptr, aValue );
    }

    /// Return the instance-specific footprint assignment for the given sheet path.
    const wxString GetFootprint( const SCH_SHEET_PATH* sheet, bool aResolve ) const;
    void SetFootprint( const SCH_SHEET_PATH* sheet, const wxString& aFootprint );

    /// Set the value for all instances (the default GUI behavior).
    void SetFootprint( const wxString& aFootprint )
    {
        SetFootprint( nullptr, aFootprint );
    }

    // Geometric transforms (used in block operations):

    void Move( const wxPoint& aMoveVector ) override
    {
        if( aMoveVector == wxPoint( 0, 0 ) )
            return;

        m_pos += aMoveVector;

        for( SCH_FIELD& field : m_fields )
            field.Move( aMoveVector );

        SetModified();
    }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const wxPoint& aCenter ) override;

    bool Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const override;

    void GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList ) override;

    /**
     * Test if the symbol's dangling state has changed for all pins.
     *
     * As a side effect, actually update the dangling status for all pins.
     *
     * @note This does not test for  short circuits.
     *
     * @param aItemList is list of all #DANGLING_END_ITEM items to be tested.
     * @return true if any pin's state has changed.
     */
    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                              const SCH_SHEET_PATH* aPath = nullptr ) override;

    wxPoint GetPinPhysicalPosition( const LIB_PIN* Pin ) const;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return ( aItem->Type() == SCH_LINE_T && aItem->GetLayer() == LAYER_WIRE ) ||
                ( aItem->Type() == SCH_NO_CONNECT_T ) ||
                ( aItem->Type() == SCH_JUNCTION_T ) ||
                ( aItem->Type() == SCH_SYMBOL_T ) ||
                ( aItem->Type() == SCH_LABEL_T ) ||
                ( aItem->Type() == SCH_HIER_LABEL_T ) ||
                ( aItem->Type() == SCH_GLOBAL_LABEL_T );
    }

    /**
     * @return true if the symbol is in netlist.
     */
    bool IsInNetlist() const;

    std::vector<wxPoint> GetConnectionPoints() const override;

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    /**
     * Return the symbol library item at \a aPosition that is part of this symbol.
     *
     * @param aPosition is the schematic position of the symbol library object.
     * @param aType is the type of symbol library object to find or any if set to TYPE_NOT_INIT.
     * @return is the symbol library object if found otherwise NULL.
     */
    LIB_ITEM* GetDrawItem( const wxPoint& aPosition, KICAD_T aType = TYPE_NOT_INIT );

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    bool operator <( const SCH_ITEM& aItem ) const override;

    bool operator==( const SCH_SYMBOL& aSymbol) const;
    bool operator!=( const SCH_SYMBOL& aSymbol) const;

    SCH_SYMBOL& operator=( const SCH_ITEM& aItem );

    bool IsReplaceable() const override { return true; }

    wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( const wxPoint& aPosition ) override { Move( aPosition - m_pos ); }

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) const override;

    EDA_ITEM* Clone() const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    void ClearBrightenedPins();

    bool HasBrightenedPins();

    bool GetIncludeInBom() const { return m_inBom; }
    void SetIncludeInBom( bool aIncludeInBom ) { m_inBom = aIncludeInBom; }

    bool GetIncludeOnBoard() const { return m_onBoard; }
    void SetIncludeOnBoard( bool aIncludeOnBoard ) { m_onBoard = aIncludeOnBoard; }

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override;

private:
    bool doIsConnected( const wxPoint& aPosition ) const override;

    void Init( const wxPoint& pos = wxPoint( 0, 0 ) );

    wxPoint     m_pos;
    LIB_ID      m_lib_id;       ///< Name and library the symbol was loaded from, i.e. 74xx:74LS00.
    int         m_unit;         ///< The unit for multiple part per package symbols.
    int         m_convert;      ///< The alternate body style for symbols that have more than
                                ///<   one body style defined.  Primarily used for symbols that
                                ///<   have a De Morgan conversion.
    wxString    m_prefix;       ///< C, R, U, Q etc - the first character(s) which typically
                                ///<   indicate what the symbol is. Determined, upon placement,
                                ///<   from the library symbol.  Created upon file load, by the
                                ///<   first non-digits in the reference fields.

    /**
     * The name used to look up a symbol in the symbol library embedded in a schematic.
     *
     * By default this is the same as #LIB_ID::GetLibItemName().  However, schematics allow for
     * multiple variants of the same library symbol.  Set this member in order to preserve the
     * link to the original symbol library.  If empty, #LIB_ID::GetLibItemName() should be used.
     */
    wxString    m_schLibSymbolName;

    TRANSFORM   m_transform;    ///< The rotation/mirror transformation matrix.
    SCH_FIELDS  m_fields;       ///< Variable length list of fields.

    std::unique_ptr< LIB_SYMBOL >          m_part;    // a flattened copy of the LIB_SYMBOL from
                                                      // the PROJECT's libraries.
    std::vector<std::unique_ptr<SCH_PIN>>  m_pins;    // a SCH_PIN for every LIB_PIN (all units)
    std::unordered_map<LIB_PIN*, unsigned> m_pinMap;  // library pin pointer to SCH_PIN's index

    bool        m_isInNetlist;  ///< True if the symbol should appear in the netlist
    bool        m_inBom;        ///< True to include in bill of materials export.
    bool        m_onBoard;      ///< True to include in netlist when updating board.

    // Defines the hierarchical path and reference of the symbol.  This allows support
    // for multiple references to a single sub-sheet.
    std::vector<SYMBOL_INSTANCE_REFERENCE> m_instanceReferences;
};

#endif /* __SYMBOL_H__ */
