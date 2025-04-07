/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams, mike@mikebwilliams.com
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

#ifndef PCB_FIELD_H
#define PCB_FIELD_H

#include <pcb_text.h>
#include <template_fieldnames.h>

class BOARD_DESIGN_SETTINGS;

class PCB_FIELD : public PCB_TEXT
{
public:
    PCB_FIELD( FOOTPRINT* aParent, int aFieldId, const wxString& aName = wxEmptyString );

    PCB_FIELD( const PCB_TEXT& aText, int aFieldId, const wxString& aName = wxEmptyString );

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_FIELD_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "PCB_FIELD" ); }

    bool IsType( const std::vector<KICAD_T>& aScanTypes ) const override
    {
        if( BOARD_ITEM::IsType( aScanTypes ) )
            return true;

        for( KICAD_T scanType : aScanTypes )
        {
            if( scanType == PCB_FIELD_LOCATE_REFERENCE_T && m_id == REFERENCE_FIELD )
                return true;
            else if( scanType == PCB_FIELD_LOCATE_VALUE_T && m_id == VALUE_FIELD )
                return true;
            else if( scanType == PCB_FIELD_LOCATE_DATASHEET_T && m_id == DATASHEET_FIELD )
                return true;
        }

        return false;
    }

    bool IsReference() const { return m_id == REFERENCE_FIELD; }
    bool IsValue() const { return m_id == VALUE_FIELD; }
    bool IsDatasheet() const { return m_id == DATASHEET_FIELD; }
    bool IsComponentClass() const { return GetName() == wxT( "Component Class" ); }

    bool IsMandatory() const;

    bool IsHypertext() const;

    wxString GetTextTypeDescription() const override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

    EDA_ITEM* Clone() const override;

    /**
     * Same as Clone, but returns a PCB_FIELD item.
     *
     * Useful mainly for python scripts, because Clone returns an EDA_ITEM.
     */
    PCB_FIELD* CloneField() const { return (PCB_FIELD*) Clone(); }

    /**
     * Return the field name (not translated)..
     *
     * @param aUseDefaultName When true return the default field name if the field name is
     *                        empty.  Otherwise the default field name is returned.
     * @return the name of the field.
     */
    wxString GetName( bool aUseDefaultName = true ) const;

    /**
     * Get a non-language-specific name for a field which can be used for storage, variable
     * look-up, etc.
     */
    wxString GetCanonicalName() const;

    void SetName( const wxString& aName ) { m_name = aName; }

    int GetId() const { return m_id; }
    void SetId( int aId ) { m_id = aId; }

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const PCB_FIELD& aOther ) const;
    bool operator==( const BOARD_ITEM& aOther ) const override;

protected:
    void swapData( BOARD_ITEM* aImage ) override;

private:
    void setId( int aId ) { m_id = aId; }

    int m_id; ///< Field index, @see enum MANDATORY_FIELD_T

    wxString m_name;
};

#endif
