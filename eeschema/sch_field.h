/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2022 CERN
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef CLASS_SCH_FIELD_H
#define CLASS_SCH_FIELD_H


#include <eda_text.h>
#include <sch_item.h>
#include <template_fieldnames.h>
#include <general.h>

class SCH_EDIT_FRAME;
class LIB_FIELD;


/**
 * Instances are attached to a symbol or sheet and provide a place for the symbol's value,
 * reference designator, footprint, , a sheet's name, filename, and user definable name-value
 * pairs of arbitrary purpose.
 *
 *  - Field 0 is reserved for the symbol reference.
 *  - Field 1 is reserved for the symbol value.
 *  - Field 2 is reserved for the symbol footprint.
 *  - Field 3 is reserved for the symbol data sheet file.
 *  - Field 4 and higher are user definable.
 */
class SCH_FIELD : public SCH_ITEM, public EDA_TEXT
{
public:
    SCH_FIELD( const VECTOR2I& aPos, int aFieldId, SCH_ITEM* aParent,
               const wxString& aName = wxEmptyString );

    SCH_FIELD( SCH_ITEM* aParent, int aFieldId, const wxString& aName = wxEmptyString );

    SCH_FIELD( const SCH_FIELD& aText );

    ~SCH_FIELD()
    { }

    SCH_FIELD& operator=( const SCH_FIELD& aField );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_FIELD_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_FIELD" );
    }

    bool IsType( const std::vector<KICAD_T>& aScanTypes ) const override
    {
        if( SCH_ITEM::IsType( aScanTypes ) )
            return true;

        for( KICAD_T scanType : aScanTypes )
        {
            if( scanType == SCH_FIELD_LOCATE_REFERENCE_T && m_id == REFERENCE_FIELD )
                return true;
            else if ( scanType == SCH_FIELD_LOCATE_VALUE_T && m_id == VALUE_FIELD )
                return true;
            else if ( scanType == SCH_FIELD_LOCATE_FOOTPRINT_T && m_id == FOOTPRINT_FIELD )
                return true;
            else if ( scanType == SCH_FIELD_LOCATE_DATASHEET_T && m_id == DATASHEET_FIELD )
                return true;
        }

        return false;
    }

    bool IsHypertext() const override
    {
        return GetCanonicalName() == wxT( "Intersheetrefs" );
    }

    void DoHypertextAction( EDA_DRAW_FRAME* aFrame ) const override;

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

    void SetName( const wxString& aName );

    void SetText( const wxString& aText ) override;

    /**
     * Get the initial name of the field set at creation (or set by SetName()).
     * This is the raw field name with no translation and no change.
     */
    const wxString& GetInternalName() { return m_name; }

    int GetId() const { return m_id; }

    void SetId( int aId );

    /**
     * Gets the fields name as displayed on the schematic or
     * in the symbol fields table. This is either the same as GetName() or
     * if the field has a variable for name, the variable namer with the ${} stripped.
     */
    wxString GetShownName() const;
    wxString GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                           int aDepth = 0 ) const;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override;

    COLOR4D GetFieldColor() const;

    void SetLastResolvedState( const SCH_ITEM* aItem ) override
    {
        const SCH_FIELD* aField = dynamic_cast<const SCH_FIELD*>( aItem );

        if( aField )
            m_lastResolvedColor = aField->m_lastResolvedColor;
    }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /**
     * Adjusters to allow EDA_TEXT to draw/print/etc. text in absolute coords.
     */
    EDA_ANGLE GetDrawRotation() const override;

    const BOX2I GetBoundingBox() const override;

    /**
     * Return whether the field will be rendered with the horizontal justification
     * inverted due to rotation or mirroring of the parent.
     */
    bool IsHorizJustifyFlipped() const;
    bool IsVertJustifyFlipped() const;

    GR_TEXT_H_ALIGN_T GetEffectiveHorizJustify() const;
    GR_TEXT_V_ALIGN_T GetEffectiveVertJustify() const;

    bool IsNameShown() const { return m_showName; }
    void SetNameShown( bool aShown = true ) { m_showName = aShown; }

    /**
     * Named variables are fields whose names are variables like ${VAR}.
     *
     * The shown name of these fields is VAR and the value is resolved from
     * ${VAR}
     */
    bool IsNamedVariable() const { return m_isNamedVariable; }

    bool CanAutoplace() const { return m_allowAutoPlace; }
    void SetCanAutoplace( bool aCanPlace ) { m_allowAutoPlace = aCanPlace; }

    void SwapData( SCH_ITEM* aItem ) override;

    /**
     * Copy parameters from a LIB_FIELD source.
     *
     * Pointers and specific values (position) are not copied.
     *
     * @param aSource is the LIB_FIELD to read.
     */
    void ImportValues( const LIB_FIELD& aSource );

    int GetPenWidth() const override;

    void ClearCaches() override;
    void ClearRenderCache() override;

    std::vector<std::unique_ptr<KIFONT::GLYPH>>*
    GetRenderCache( const wxString& forResolvedText, const VECTOR2I& forPosition,
                    TEXT_ATTRIBUTES& aAttrs ) const;

    void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        Offset( aMoveVector );
    }

    void Rotate( const VECTOR2I& aCenter ) override;

    /**
     * @copydoc SCH_ITEM::MirrorVertically()
     *
     * This overload does nothing.  Fields are never mirrored alone.  They are moved
     * when the parent symbol is mirrored.  This function is only needed by the
     * pure function of the master class.
     */
    void MirrorVertically( int aCenter ) override
    {
    }

    /**
     * @copydoc SCH_ITEM::MirrorHorizontally()
     *
     * This overload does nothing.  Fields are never mirrored alone.  They are moved
     * when the parent symbol is mirrored.  This function is only needed by the
     * pure function of the master class.
     */
    void MirrorHorizontally( int aCenter ) override
    {
    }

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData = nullptr ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    BITMAPS GetMenuImage() const override;

    bool IsReplaceable() const override;

    VECTOR2I GetLibPosition() const { return EDA_TEXT::GetTextPos(); }

    VECTOR2I GetPosition() const override;
    void     SetPosition( const VECTOR2I& aPosition ) override;

    VECTOR2I GetParentPosition() const;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground ) const override;

    EDA_ITEM* Clone() const override;

    bool operator <( const SCH_ITEM& aItem ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    KIFONT::FONT* getDrawFont() const override;

private:
    int      m_id;         ///< Field index, @see enum MANDATORY_FIELD_T

    wxString m_name;

    bool     m_showName;   ///< Render the field name in addition to its value
    bool     m_allowAutoPlace;  ///< This field can be autoplaced
    bool     m_isNamedVariable; ///< If the field name is a variable name, e.g. ${DNP}
                                ///< then the value field is forced to be the same as the name

    mutable bool                                        m_renderCacheValid;
    mutable VECTOR2I                                    m_renderCachePos;
    mutable std::vector<std::unique_ptr<KIFONT::GLYPH>> m_renderCache;

    mutable COLOR4D                                     m_lastResolvedColor;
};


#endif /* CLASS_SCH_FIELD_H */
