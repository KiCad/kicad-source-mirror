/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2022 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCH_SYMBOL_H
#define SCH_SYMBOL_H

#include <eda_item.h>
#include <core/typeinfo.h>
#include <layer_ids.h>
#include <lib_id.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wx/arrstr.h>
#include <wx/chartype.h>
#include <wx/string.h>

#include <schematic.h>
#include <symbol.h>
#include <sch_field.h>
#include <sch_pin.h>
#include <sch_sheet_path.h>    // SCH_SYMBOL_INSTANCE
#include <transform.h>

struct PICKED_SYMBOL;
class KIID_PATH;
class SCH_SCREEN;
class LIB_SYMBOL;
class MSG_PANEL_ITEM;
class LEGACY_SYMBOL_LIB;
class LEGACY_SYMBOL_LIBS;
class SCH_SCREEN;
class SCH_COMMIT;
class SCH_SHAPE;


/// A container for several SCH_FIELD items
typedef std::vector<SCH_FIELD>    SCH_FIELDS;

typedef std::weak_ptr<LIB_SYMBOL> PART_REF;


extern std::string toUTFTildaText( const wxString& txt );


/**
 * Schematic symbol object.
 */
class SCH_SYMBOL : public SYMBOL
{
public:
    SCH_SYMBOL();

    /**
     * Create schematic symbol from library symbol object.
     *
     * @param aSymbol is the library symbol to create schematic symbol from.
     * @param aLibId is the #LIB_ID of alias to create.
     * @param aSheet is the schematic sheet the symbol is place into.
     * @param aUnit is unit for symbols that have multiple parts per package.
     * @param aBodyStyle is the alternate body style for the schematic symbols.
     * @param aPosition is the position of the symbol.
     */
    SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const LIB_ID& aLibId, const SCH_SHEET_PATH* aSheet,
                int aUnit, int aBodyStyle = 0, const VECTOR2I& aPosition = VECTOR2I( 0, 0 ),
                EDA_ITEM* aParent = nullptr );

    SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const SCH_SHEET_PATH* aSheet, const PICKED_SYMBOL& aSel,
                const VECTOR2I& aPosition = VECTOR2I( 0, 0 ), EDA_ITEM* aParent = nullptr );

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

    ~SCH_SYMBOL();

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_SYMBOL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_SYMBOL" );
    }

    /**
     * Check to see if the library symbol is set to the dummy library symbol.
     *
     * When the library symbol is missing (which technically should not happen now that the
     * library symbols are cached in the schematic file), a dummy library symbol is substituted
     * for the missing symbol as an indicator that something is amiss.  The dummy symbol cannot
     * be edited so a check for this symbol must be performed before attempting to edit the
     * library symbol with the library editor or it will crash KiCad.
     *
     * @see LIB_SYMBOL::GetDummy()
     *
     * @return true if the library symbol is missing or false if it is valid.
     */
    bool IsMissingLibSymbol() const;

    const std::vector<SCH_SYMBOL_INSTANCE>& GetInstances() const
    {
        return m_instances;
    }

    bool GetInstance( SCH_SYMBOL_INSTANCE& aInstance,
                      const KIID_PATH& aSheetPath, bool aTestFromEnd = false ) const;

    void RemoveInstance( const SCH_SHEET_PATH& aInstancePath );

    void RemoveInstance( const KIID_PATH& aInstancePath );

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
     * @note We now use this to keep poorly-formed symbols from getting dragged off-grid.  If
     *       the symbol contains off-grid pins we will not allow it to be moved from its anchor.
     */
    bool IsMovableFromAnchorPoint() const override;

    void SetLibId( const LIB_ID& aName );

    const LIB_ID& GetLibId() const override { return m_lib_id; }

    wxString GetSymbolIDAsString() const { return m_lib_id.Format(); }

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
     * @return the associated LIB_SYMBOL's description field (or wxEmptyString).
     */
    wxString GetDescription() const override;
    wxString GetShownDescription( int aDepth = 0 ) const override;

    /**
     * @return the associated LIB_SYMBOL's keywords field (or wxEmptyString).
     */
    wxString GetKeyWords() const override;
    wxString GetShownKeyWords( int aDepth = 0 ) const override;

    /**
     * Return the documentation text for the given part alias
     */
    wxString GetDatasheet() const;

    /**
     * Updates the cache of SCH_PIN objects for each pin
     */
    void UpdatePins();

    /**
     * Return the display name for a given unit \a aUnit.
     *
     * @return the display name of a unit if set, or the ordinal name of the unit otherwise.
     */
    wxString GetUnitDisplayName( int aUnit, bool aLabel ) const override;

    wxString GetBodyStyleDescription( int aBodyStyle, bool aLabel ) const override;

    void SetBodyStyle( int aBodyStyle ) override;

    wxString GetPrefix() const { return m_prefix; }
    void SetPrefix( const wxString& aPrefix ) { m_prefix = aPrefix; }

    /**
     * Set the prefix based on the current reference designator.
     */
    void UpdatePrefix();

    wxString SubReference( int aUnit, bool aAddSeparator = true ) const;

    /**
     * Return the number of units per package of the symbol.
     *
     * @return the number of units per package or zero if the library entry cannot be found.
     */
    int GetUnitCount() const override;

    bool IsMultiUnit() const override { return GetUnitCount() > 1; }

    /**
     * Return the number of body styles of the symbol.
     *
     * @return the number of body styles or zero if the library entry cannot be found.
     */
    int GetBodyStyleCount() const override;

    bool IsMultiBodyStyle() const override { return GetBodyStyleCount() > 1; }

    bool HasDeMorganBodyStyles() const override;

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
    int GetOrientation() const override;

    /**
     * Orientation/mirroring access for property manager.
     */
    void SetOrientationProp( SYMBOL_ORIENTATION_PROP aAngle )
    {
        int mirroring = GetOrientation();

        mirroring &= ( SYMBOL_ORIENTATION_T::SYM_MIRROR_X | SYMBOL_ORIENTATION_T::SYM_MIRROR_Y );

        SetOrientation( aAngle | mirroring );
    }

    SYMBOL_ORIENTATION_PROP GetOrientationProp() const
    {
        int orientation = GetOrientation();

        orientation &= ~( SYMBOL_ORIENTATION_T::SYM_MIRROR_X | SYMBOL_ORIENTATION_T::SYM_MIRROR_Y );

        switch( orientation )
        {
        default:
        case SYM_NORMAL:
        case SYM_ORIENT_0:   return SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_0;
        case SYM_ORIENT_90:  return SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_90;
        case SYM_ORIENT_180: return SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_180;
        case SYM_ORIENT_270: return SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_270;
        }
    }

    void SetMirrorX( bool aMirror )
    {
        int orientation = GetOrientation();

        if( aMirror )
            orientation |= SYMBOL_ORIENTATION_T::SYM_MIRROR_X;
        else
            orientation &= ~SYMBOL_ORIENTATION_T::SYM_MIRROR_X;

        SetOrientation( orientation );
    }

    bool GetMirrorX() const
    {
        return GetOrientation() & SYMBOL_ORIENTATION_T::SYM_MIRROR_X;
    }

    void SetMirrorY( bool aMirror )
    {
        int orientation = GetOrientation();

        if( aMirror )
            orientation |= SYMBOL_ORIENTATION_T::SYM_MIRROR_Y;
        else
            orientation &= ~SYMBOL_ORIENTATION_T::SYM_MIRROR_Y;

        SetOrientation( orientation );
    }

    bool GetMirrorY() const
    {
        return GetOrientation() & SYMBOL_ORIENTATION_T::SYM_MIRROR_Y;
    }

    /**
     * Return the list of system text vars & fields for this symbol.
     */
    void GetContextualTextVars( wxArrayString* aVars ) const;

    /**
     * Resolve any references to system tokens supported by the symbol.
     *
     * @param aPath the sheet path for context.
     * @param token the token to resolve (modified in place with the result).
     * @param aDepth a counter to limit recursion and circular references.
     */
    bool ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token, int aDepth = 0 ) const;

    /**
     * Resolve any references to system tokens supported by the symbol with variant support.
     *
     * @param aPath the sheet path for context.
     * @param token the token to resolve (modified in place with the result).
     * @param aVariantName optional variant name to resolve field values for a specific variant.
     * @param aDepth a counter to limit recursion and circular references.
     */
    bool ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token,
                         const wxString& aVariantName, int aDepth = 0 ) const;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * Clear exiting symbol annotation.
     *
     * For example, IC23 would be changed to IC? and unit number would be reset.
     *
     * @param aSheetPath is the hierarchical path of the symbol to clear or remove all
     *                   annotations for this symbol if NULL.
     * @param[in] aResetPrefix The annotation prefix ('R', 'U', etc.) should be reset to the
     *                         symbol library prefix.
     */
    void ClearAnnotation( const SCH_SHEET_PATH* aSheetPath, bool aResetPrefix );

    /**
     * Add an instance to the alternate references list (m_instances), if this entry
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


    const BOX2I GetBoundingBox() const override;

    /**
     * Return a bounding box for the symbol body but not the pins or fields.
     */
    BOX2I GetBodyBoundingBox() const override;

    /**
     * Return a bounding box for the symbol body and pins but not the fields.
     */
    BOX2I GetBodyAndPinsBoundingBox() const override;


    //-----<Fields>-----------------------------------------------------------

    /**
     * Return a mandatory field in this symbol.  The const version will return nullptr if the
     * field is not found; non-const version will create the field.
     */
    SCH_FIELD* GetField( FIELD_T aFieldType );
    const SCH_FIELD* GetField( FIELD_T aFieldNdx ) const;

    /**
     * Return a field in this symbol.
     *
     * @param aFieldName is the canonical name of the field.
     *
     * @return Both non-const and const versions return nullptr if the field is not found.
     */
    SCH_FIELD* GetField( const wxString& aFieldName );
    const SCH_FIELD* GetField( const wxString& aFieldName ) const;

    /**
     * Populate a std::vector with SCH_FIELDs, sorted in ordinal order.
     *
     * @param aVector is the vector to populate.
     * @param aVisibleOnly is used to add only the fields that are visible and contain text.
     */
    void GetFields( std::vector<SCH_FIELD*>& aVector, bool aVisibleOnly ) const override;

    /**
     * Return a reference to the vector holding the symbol's fields
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
     *
     * @param aFieldName is the user fieldName to remove.
     */
    void RemoveField( const wxString& aFieldName );

    void RemoveField( SCH_FIELD* aField ) { RemoveField( aField->GetName() ); }

    /**
     * Search for a #SCH_FIELD with \a aFieldName
     *
     * @param aFieldName is the name of the field to find.
     *
     * @return the field if found or NULL if the field was not found.
     */
    SCH_FIELD* FindFieldCaseInsensitive( const wxString& aFieldName );

    /**
     * @return the reference for the instance on the given sheet.
     */
    const wxString GetRef( const SCH_SHEET_PATH* aSheet,
                           bool aIncludeUnit = false ) const override;

    /**
     * @return the value for the instance on the given sheet.
     */
    const wxString GetValue( bool aResolve, const SCH_SHEET_PATH* aPath,
                             bool aAllowExtraText, const wxString& aVariantName = wxEmptyString ) const override;

    void SetValueFieldText( const wxString& aValue, const SCH_SHEET_PATH* aInstance = nullptr,
                            const wxString& aVariantName = wxEmptyString );

    const wxString GetFootprintFieldText( bool aResolve, const SCH_SHEET_PATH* aPath,
                                          bool aAllowExtraText ) const;
    void SetFootprintFieldText( const wxString& aFootprint );

    /*
     * Field access for property manager
     */
    wxString GetRefProp() const
    {
        return GetRef( &Schematic()->CurrentSheet() );
    }

    void SetRefProp( const wxString& aRef );

    wxString GetValueProp() const
    {
        return GetValue( false, &Schematic()->CurrentSheet(), false, Schematic()->GetCurrentVariant() );
    }

    void SetValueProp( const wxString& aValue );  // Implemented in sch_symbol.cpp for tracing

    int GetUnitProp() const
    {
        return GetUnitSelection( &Schematic()->CurrentSheet() );
    }

    void SetFieldText( const wxString& aFieldName, const wxString& aFieldText, const SCH_SHEET_PATH* aPath = nullptr,
                       const wxString& aVariantName = wxEmptyString );

    wxString GetFieldText( const wxString& aFieldName, const SCH_SHEET_PATH* aPath = nullptr,
                           const wxString& aVariantName = wxEmptyString ) const;

    void SetUnitProp( int aUnit )
    {
        SetUnitSelection( &Schematic()->CurrentSheet(), aUnit );
        SetUnit( aUnit );
    }

    wxString GetBodyStyleProp() const override
    {
        return GetBodyStyleDescription( GetBodyStyle(), false );
    }

    void SetBodyStyleProp( const wxString& aBodyStyle ) override
    {
        for( int bodyStyle = 1; bodyStyle <= GetBodyStyleCount(); bodyStyle++ )
        {
            if( GetBodyStyleDescription( bodyStyle, false ) == aBodyStyle )
            {
                SetBodyStyle( bodyStyle );
                return;
            }
        }
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
     * Keep fields other than the reference, include/exclude flags, and alternate pin assignments
     * in sync in multi-unit parts.
     *
     * @param aSourceSheet the sheet instance of the unit to sync to
     * @param aProperty [optional] if present, the single property to sync.  (Otherwise the
     *                  include/exclude flags, alternate pin assignments, and all fields bar the
     *                  reference will be synced.)
     */
    void SyncOtherUnits( const SCH_SHEET_PATH& aSourceSheet, SCH_COMMIT& aCommit,
                         PROPERTY_BASE* aProperty );

    /**
     * Return the next ordinal for a user field for this symbol
     */
    int GetNextFieldOrdinal() const;

    /**
     * Automatically orient all the fields in the symbol.
     *
     * @param aScreen is the SCH_SCREEN associated with the current instance of the symbol.
     *                Required when \a aAlgo is AUTOPLACE_MANUAL; optional otherwise.
     */
    void AutoplaceFields( SCH_SCREEN* aScreen, AUTOPLACE_ALGO aAlgo ) override;

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction, RECURSE_MODE aMode ) override;


    //-----</Fields>----------------------------------------------------------


    /**
     * Find a symbol pin by number.
     *
     * @param number is the number of the pin to find.
     * @return Pin object if found, otherwise NULL.
     */
    SCH_PIN* GetPin( const wxString& number ) const;

    /**
     * Populate a vector with all the pins from the library object that match the current unit
     * and bodyStyle.
     *
     * @param aPinsList is the list to populate with all of the pins.
     */
    std::vector<SCH_PIN*> GetLibPins() const;

    /**
     * @return a list of pin pointers for all units / bodyStyles.  Used primarily for SPICE where
     * we want to treat all units together as a single SPICE element.
     */
    std::vector<SCH_PIN*> GetAllLibPins() const;

    /**
     * @return a count of pins for all units.
     */
    size_t GetFullPinCount() const;

    /**
     * @return the instance SCH_PIN associated with a particular SCH_PIN from the LIB_SYMBOL.
     */
    SCH_PIN* GetPin( SCH_PIN* aLibPin ) const;

    /**
     * Return the #SCH_PIN object found at \a aPosition.
     *
     * @param aPosition is the position of the pin to fetch.
     *
     * @return the #SCH_PIN object found at \a aPosition or nullptr.
     */
    const SCH_PIN* GetPin( const VECTOR2I& aPosition ) const;

    /**
     * Retrieve a list of the SCH_PINs for the given sheet path.
     *
     * Since a symbol can have a different unit on a different instance of a sheet,
     * this list returns the subset of pins that exist on a given sheet.
     *
     * @return a vector of pointers (non-owning) to SCH_PINs
     */
    std::vector<SCH_PIN*> GetPins( const SCH_SHEET_PATH* aSheet ) const;

    std::vector<SCH_PIN*> GetPins() const override;


    std::vector<std::unique_ptr<SCH_PIN>>& GetRawPins() { return m_pins; }

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
    bool IsAnnotated( const SCH_SHEET_PATH* aSheet ) const;

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
                                   int              aUnit );

    void AddHierarchicalReference( const SCH_SYMBOL_INSTANCE& aInstance );

    /// Return the instance-specific unit selection for the given sheet path.
    int GetUnitSelection( const SCH_SHEET_PATH* aSheet ) const;

    /// Set the selected unit of this symbol on one sheet.
    void SetUnitSelection( const SCH_SHEET_PATH* aSheet, int aUnitSelection );

    /// Set the selected unit of this symbol for all sheets.
    void SetUnitSelection( int aUnitSelection );

    virtual void SetDNP( bool aEnable, const SCH_SHEET_PATH* aInstance = nullptr,
                         const wxString& aVariantName = wxEmptyString ) override;
    virtual bool GetDNP( const SCH_SHEET_PATH* aInstance = nullptr,
                         const wxString& aVariantName = wxEmptyString ) const override;

    bool GetDNPProp() const { return GetDNP( &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() ); }

    void SetDNPProp( bool aEnable ) { SetDNP( aEnable, &Schematic()->CurrentSheet(),
                                              Schematic()->GetCurrentVariant() ); }

    void SetExcludedFromBOM( bool aEnable, const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) override;
    bool GetExcludedFromBOM( const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) const override;

    bool GetExcludedFromBOMProp() const
    {
        return GetExcludedFromBOM( &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromBOMProp( bool aEnable )
    {
        SetExcludedFromBOM( aEnable, &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromSim( bool aEnable, const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) override;
    bool GetExcludedFromSim( const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) const override;

    bool GetExcludedFromSimProp() const
    {
        return GetExcludedFromSim( &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromSimProp( bool aEnable )
    {
        SetExcludedFromSim( aEnable, &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromBoard( bool aEnable, const SCH_SHEET_PATH* aInstance = nullptr,
                               const wxString& aVariantName = wxEmptyString ) override;
    bool GetExcludedFromBoard( const SCH_SHEET_PATH* aInstance = nullptr,
                               const wxString& aVariantName = wxEmptyString ) const override;

    bool GetExcludedFromBoardProp() const
    {
        return GetExcludedFromBoard( &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromBoardProp( bool aEnable )
    {
        SetExcludedFromBoard( aEnable, &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromPosFiles( bool aEnable, const SCH_SHEET_PATH* aInstance = nullptr,
                                  const wxString& aVariantName = wxEmptyString ) override;
    bool GetExcludedFromPosFiles( const SCH_SHEET_PATH* aInstance = nullptr,
                                  const wxString& aVariantName = wxEmptyString ) const override;

    bool GetExcludedFromPosFilesProp() const
    {
        return GetExcludedFromPosFiles( &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromPosFilesProp( bool aEnable )
    {
        SetExcludedFromPosFiles( aEnable, &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    /**
     * SCH_SYMBOLs don't currently support embedded files, but their LIB_SYMBOL counterparts
     * do.
     */
    EMBEDDED_FILES* GetEmbeddedFiles() override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        if( aMoveVector == VECTOR2I( 0, 0 ) )
            return;

        m_pos += aMoveVector;

        for( SCH_FIELD& field : m_fields )
            field.Move( aMoveVector );
    }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

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
    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                              std::vector<DANGLING_END_ITEM>& aItemListByPos,
                              const SCH_SHEET_PATH*           aPath = nullptr ) override;

    VECTOR2I GetPinPhysicalPosition( const SCH_PIN* Pin ) const;

    bool IsConnectable() const override { return true; }

    bool HasConnectivityChanges( const SCH_ITEM* aItem,
                                 const SCH_SHEET_PATH* aInstance = nullptr ) const override;

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return ( aItem->Type() == SCH_LINE_T && aItem->GetLayer() == LAYER_WIRE ) ||
               ( aItem->Type() == SCH_NO_CONNECT_T ) ||
               ( aItem->Type() == SCH_JUNCTION_T ) ||
               ( aItem->Type() == SCH_SYMBOL_T ) ||
               ( aItem->Type() == SCH_DIRECTIVE_LABEL_T ) ||
               ( aItem->Type() == SCH_LABEL_T ) ||
               ( aItem->Type() == SCH_HIER_LABEL_T ) ||
               ( aItem->Type() == SCH_GLOBAL_LABEL_T );
    }

    /**
     * @return true if the symbol is in netlist.
     */
    bool IsInNetlist() const;

    std::vector<VECTOR2I> GetConnectionPoints() const override;

    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

    /**
     * Return the symbol library item at \a aPosition that is part of this symbol.
     *
     * @param aPosition is the schematic position of the symbol library object.
     * @param aType is the type of symbol library object to find or any if set to TYPE_NOT_INIT.
     * @return is the symbol library object if found otherwise NULL.
     */
    SCH_ITEM* GetDrawItem( const VECTOR2I& aPosition, KICAD_T aType = TYPE_NOT_INIT );

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    bool operator <( const SCH_ITEM& aItem ) const override;

    bool operator==( const SCH_SYMBOL& aSymbol) const;
    bool operator!=( const SCH_SYMBOL& aSymbol) const;

    SCH_SYMBOL& operator=( const SCH_SYMBOL& aItem );

    bool IsReplaceable() const override { return true; }

    VECTOR2I GetPosition() const override { return m_pos; }
    void    SetPosition( const VECTOR2I& aPosition ) override { Move( aPosition - m_pos ); }

    int GetX() const { return GetPosition().x; };
    void SetX( int aX ) { SetPosition( VECTOR2I( aX, GetY() ) ); }

    int GetY() const { return GetPosition().y; }
    void SetY( int aY ) { SetPosition( VECTOR2I( GetX(), aY ) ); }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    /**
     * Plot just the symbol pins.  This is separated to match the GAL display order.  The pins
     * are ALSO plotted with the symbol group.  This replotting allows us to ensure that they
     * are shown above other elements in the schematic.
     *
     * @param aPlotter is the #PLOTTER object used to plot pins.
     */
    void PlotPins( PLOTTER* aPlotter ) const;

    /**
     * Plot the local power pin indicator icon shape
     *
     * @param aPlotter the #PLOTTER object used to draw  the icon
     */
    void PlotLocalPowerIconShape( PLOTTER* aPlotter ) const;

    /**
     * Plot the red 'X' over the symbol.  This is separated to allow it being used from the
     * screen plot function, overlapping the pins
     *
     * @param aPlotter the #PLOTTER object used to draw the X
     */
    void PlotDNP( PLOTTER* aPlotter ) const;

    /**
     * Build the local power pin indicator icon shape, at coordinate aPos.
     * it is currently a set of 3 bezier curves + one filled cirle
     * @param aShapeList is a std::vector<SCH_SHAPE> to store the set of SCH_SHAPE
     * @param aPos is the position of icon
     * @param aSize is the size of icon
     * @param aLineWidth is the line width used to build shapes
     * @param aHorizontal = false for a vertical icon, true for a horizontal icon shape
     */
    static void BuildLocalPowerIconShape( std::vector<SCH_SHAPE>& aShapeList, const VECTOR2D& aPos,
                                          double aSize, double aLineWidth, bool aHorizontal );

    EDA_ITEM* Clone() const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    void ClearBrightenedPins();

    bool HasBrightenedPins();

    bool IsPointClickableAnchor( const VECTOR2I& aPos ) const override;

    /**
     * @return true if the symbol is equivalent to a global label:
     * It is a Power symbol
     * It has only one pin type Power input
     */
    bool IsSymbolLikePowerGlobalLabel() const;
    bool IsSymbolLikePowerLocalLabel() const;

    bool IsGlobalPower() const override;
    bool IsLocalPower() const override;
    bool IsPower() const override;
    bool IsNormal() const override;

    bool GetShowPinNames() const override;
    void SetShowPinNames( bool aShow ) override;

    bool GetShowPinNumbers() const override;
    void SetShowPinNumbers( bool aShow ) override;

    double Similarity( const SCH_ITEM& aOther ) const override;

    /// Return the component classes this symbol belongs in.
    std::unordered_set<wxString> GetComponentClassNames( const SCH_SHEET_PATH* aPath ) const;

    void DeleteVariant( const KIID_PATH& aPath, const wxString& aVariantName );

    void RenameVariant( const KIID_PATH& aPath, const wxString& aOldName, const wxString& aNewName );

    void CopyVariant( const KIID_PATH& aPath, const wxString& aSourceVariant,
                      const wxString& aNewVariant );

    std::optional<SCH_SYMBOL_VARIANT> GetVariant( const SCH_SHEET_PATH& aInstance, const wxString& aVariantName ) const;
    void AddVariant( const SCH_SHEET_PATH& aInstance, const SCH_SYMBOL_VARIANT& aVariant );

    void DeleteVariant( const SCH_SHEET_PATH& aInstance, const wxString& aVariantName )
    {
        DeleteVariant( aInstance.Path(), aVariantName );
    }

    void RenameVariant( const SCH_SHEET_PATH& aInstance, const wxString& aOldName,
                        const wxString& aNewName )
    {
        RenameVariant( aInstance.Path(), aOldName, aNewName );
    }

    void CopyVariant( const SCH_SHEET_PATH& aInstance, const wxString& aSourceVariant,
                      const wxString& aNewVariant )
    {
        CopyVariant( aInstance.Path(), aSourceVariant, aNewVariant );
    }

    bool operator==( const SCH_ITEM& aOther ) const override;

protected:
    void swapData( SCH_ITEM* aItem ) override;

private:
    BOX2I doGetBoundingBox( bool aIncludePins, bool aIncludeFields ) const;

    bool doIsConnected( const VECTOR2I& aPosition ) const override;

    void Init( const VECTOR2I& pos = VECTOR2I( 0, 0 ) );

    SCH_SYMBOL_INSTANCE* getInstance( const KIID_PATH& aPath );
    const SCH_SYMBOL_INSTANCE* getInstance( const KIID_PATH& aPath ) const;

    SCH_SYMBOL_INSTANCE* getInstance( const SCH_SHEET_PATH& aPath ) { return getInstance( aPath.Path() ); }
    const SCH_SYMBOL_INSTANCE* getInstance( const SCH_SHEET_PATH& aPath ) const { return getInstance( aPath.Path() ); }

private:
    VECTOR2I    m_pos;
    LIB_ID      m_lib_id;       ///< Name and library the symbol was loaded from, i.e. 74xx:74LS00.
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
    wxString                    m_schLibSymbolName;

    std::vector<SCH_FIELD>      m_fields;        ///< Variable length list of fields.

    std::unique_ptr<LIB_SYMBOL> m_part;          ///< A flattened copy of the #LIB_SYMBOL from the
                                                 ///< #PROJECT object's libraries.
    bool                        m_isInNetlist;   ///< True if the symbol should appear in netlist

    std::vector<std::unique_ptr<SCH_PIN>>  m_pins;     ///< A #SCH_PIN for every #LIB_PIN.
    std::unordered_map<SCH_PIN*, SCH_PIN*> m_pinMap;   ///< Library pin pointer : #SCH_PIN indices.

    /**
     * Define the hierarchical path and reference of the symbol.
     *
     * This allows support for multiple references to a single sub-sheet.
     */
    std::vector<SCH_SYMBOL_INSTANCE>       m_instances;

    /// @see SCH_SYMBOL::GetOrientation
    static std::unordered_map<TRANSFORM, int> s_transformToOrientationCache;
};


#endif /* SCH_SYMBOL_H */
