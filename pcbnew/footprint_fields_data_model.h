/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <fields_table_data_model.h>

#include <vector>


class BOARD_COMMIT;
class FOOTPRINT;
class TEMPLATES;

/**
 * This is the minimal equivalent to the SCH_REFERENCE so we can provide
 * similar non-null guarantees throughout the data model. Otherwise we'll end up passing
 * in a raw FOOTPRINT* the data model row and we'll have to deal with the possibility
 * of null pointers.
 */
class FOOTPRINT_REF
{
public:
    explicit FOOTPRINT_REF( FOOTPRINT& aFootprint ) :
            m_footprint( &aFootprint )
    {
    }

    FOOTPRINT& GetFootprint() const { return *m_footprint; }
    bool       operator==( const FOOTPRINT_REF& aOther ) const { return m_footprint == aOther.m_footprint; }

private:
    FOOTPRINT* m_footprint;
};

using FOOTPRINT_REFERENCE_LIST = std::vector<FOOTPRINT_REF>;

using FOOTPRINT_FIELDS_TABLE_DATA_MODEL_ROW = DATA_MODEL_ROW<FOOTPRINT_REF>;

class FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL : public FIELDS_TABLE_DATA_MODEL<FOOTPRINT_REF>
{
public:
    enum SCOPE : int
    {
        SCOPE_ALL = 0,
        SCOPE_SHEET,
        SCOPE_SHEET_RECURSIVE,
        SCOPE_SELECTION
    };

    FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL( const FOOTPRINT_REFERENCE_LIST& aFootprintReferenceList,
                                             wxGridCellAttr*                 aURLEditor ) :
            m_footprintsList( aFootprintReferenceList ),
            m_scope( SCOPE_ALL ),
            m_urlEditor( aURLEditor ),
            m_textVarRenderer( nullptr )
    {
    }

    ~FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL() override
    {
        wxSafeDecRef( m_urlEditor );
        wxSafeDecRef( m_textVarRenderer );
    }

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser ) override;

    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;

    void RebuildRows() override;

    void ApplyData( BOARD_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames, const wxString& aVariantName );

    void  SetScope( SCOPE aScope ) { m_scope = aScope; }
    SCOPE GetScope() { return m_scope; }

    void             SetPath( const KIID_PATH& aPath ) { m_path = aPath; }
    const KIID_PATH& GetPath() { return m_path; }

    void AddReferences( const FOOTPRINT_REFERENCE_LIST& aRefs );
    void RemoveReferences( const FOOTPRINT_REFERENCE_LIST& aRefs );
    void RemoveFootprint( const FOOTPRINT_REF& aRef );
    void UpdateReferences( const FOOTPRINT_REFERENCE_LIST& aRefs );

    bool DeleteRows( size_t aPosition = 0, size_t aNumRows = 1 ) override;

    const FOOTPRINT_REFERENCE_LIST& GetReferenceList() const { return m_footprintsList; }

    bool IsCellReadOnly( int aRow, int aCol ) override;

private:
    bool unitMatch( const FOOTPRINT_REF& lhItem, const FOOTPRINT_REF& rhItem ) override;

    /**
     * Footprint attributes are don't currently track when they are inherited from the sheet,
     * so this function always returns false. That probably needs to change.
     */
    bool attributeForcedOnBySheet( const FOOTPRINT_REF& aRef, const wxString& aAttributeName ) const override;

    wxString getAttributeValue( const FOOTPRINT_REF& aRef, const wxString& aAttributeName,
                                const wxString& aVariantNames );
    wxString getFieldValueForVariant( const FOOTPRINT_REF& aRef, const wxString& aFieldName,
                                      const wxString& aVariantName );

    /**
     * Get the default (non-variant) value for a field.
     *
     * This retrieves the field value as it would appear without any variant override.
     *
     * @param aRef The footprint reference.
     * @param aFieldName The name of the field.
     * @return The default field value.
     */
    wxString getDefaultFieldValue( const FOOTPRINT_REF& aRef, const wxString& aFieldName );

    /**
     * Set the attribute value.
     *
     * @param aReference is a reference to the footprint to set the attribute.
     * @param aAttributeName is the name of the footprint attribute.
     * @param aValue is the value to set the attribute.
     * @param aVariantName is an optional variant name to set the variant attribute.
     * @retval true if the footprint attribute value has changed.
     * @retval false if the footprint attribute has **not** changed.
     */
    bool setAttributeValue( const FOOTPRINT_REF& aRef, const wxString& aAttributeName, const wxString& aValue,
                            const wxString& aVariantName = wxEmptyString );

    bool getLiveFieldValue( const FOOTPRINT_REF& aRef, const wxString& aFieldName,
                            wxString& aValue ) override;
    std::vector<FOOTPRINT_REF> getAllItems() const override;

    wxString getFieldResolvedLiveValue( const FOOTPRINT_REF& aRef, const wxString& aFieldName ) override;
    wxString resolveTextVars( const FOOTPRINT_REF& aRef, const wxString& aText ) override;

    KIID_PATH getDataStoreKey( const FOOTPRINT_REF& aItem ) const override;
    wxString  getItemIdentifier( const FOOTPRINT_REF& aItem ) const override;

protected:
    FOOTPRINT_REFERENCE_LIST m_footprintsList;
    SCOPE                    m_scope;
    KIID_PATH                m_path;
    wxGridCellAttr*          m_urlEditor;
    wxGridCellRenderer*      m_textVarRenderer; ///< Renderer for cells with text variable references
};
