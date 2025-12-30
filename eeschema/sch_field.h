/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef CLASS_SCH_FIELD_H
#define CLASS_SCH_FIELD_H


#include <eda_text.h>
#include <sch_item.h>
#include <template_fieldnames.h>
#include <general.h>
#include <string_utils.h>
#include "scintilla_tricks.h"
#include <algorithm>

class SCH_EDIT_FRAME;
class SCH_TEXT;


class SCH_FIELD : public SCH_ITEM, public EDA_TEXT
{
public:
    SCH_FIELD();    // For std::map::operator[]

    SCH_FIELD( SCH_ITEM* aParent, FIELD_T aFieldId = FIELD_T::USER,
               const wxString& aName = wxEmptyString );

    SCH_FIELD( SCH_ITEM* aParent, SCH_TEXT* aText );

    SCH_FIELD( const SCH_FIELD& aText );

    ~SCH_FIELD() override
    { }

    SCH_FIELD& operator=( const SCH_FIELD& aField );

    static bool ClassOf( const EDA_ITEM* aItem )
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
            if( scanType == SCH_FIELD_LOCATE_REFERENCE_T && m_id == FIELD_T::REFERENCE )
                return true;
            else if ( scanType == SCH_FIELD_LOCATE_VALUE_T && m_id == FIELD_T::VALUE )
                return true;
            else if ( scanType == SCH_FIELD_LOCATE_FOOTPRINT_T && m_id == FIELD_T::FOOTPRINT )
                return true;
            else if ( scanType == SCH_FIELD_LOCATE_DATASHEET_T && m_id == FIELD_T::DATASHEET )
                return true;
        }

        return false;
    }

    wxString GetFriendlyName() const override
    {
        return _( "Field" );
    }

    bool HasHypertext() const override;

    void DoHypertextAction( EDA_DRAW_FRAME* aFrame, const VECTOR2I& aMousePos ) const override;

    /**
     * Return the field name (not translated).
     *
     * @param aUseDefaultName If true return the default field name if the field name is empty.
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

    void SetText( const wxString& aText, const SCH_SHEET_PATH* aPath, const wxString& aVariantName = wxEmptyString );

    virtual const wxString& GetText() const override { return EDA_TEXT::GetText(); }

    wxString GetText( const SCH_SHEET_PATH* aPath, const wxString& aVariantName = wxEmptyString ) const;

    FIELD_T GetId() const { return m_id; }

    int GetOrdinal() const
    {
        return IsMandatory() ? (int) m_id : m_ordinal;
    }
    void SetOrdinal( int aOrdinal )
    {
        m_id = FIELD_T::USER;
        m_ordinal = aOrdinal;
    }

    /**
     * Get the field's name as displayed on the schematic or in the symbol fields table. This is
     * either the same as GetName() or if the field has a variable for name, the variable name
     * with the ${} stripped.
     */
    wxString GetShownName() const;
    wxString GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText, int aDepth = 0,
                           const wxString& aVariantName = wxEmptyString ) const;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override;

    /**
     * Return the text of a field.
     *
     * If the field is the reference field, the unit number is used to create a pseudo reference
     * text.  If the base reference field is U, the string U?A will be returned for unit = 1.
     *
     * @param unit - The package unit number.  Only effects reference field.
     * @return Field text.
     */
    wxString GetFullText( int unit = 1 ) const;

    /**
     * Return true if both the name and value of the field are empty.
     *
     * Whitespace does not count as non-empty.
     */
    bool IsEmpty()
    {
        wxString name( m_name );
        wxString value( GetText() );

        return name.Trim().empty() && value.Trim().empty();
    }

    int GetSchTextSize() const { return GetTextWidth(); }
    void SetSchTextSize( int aSize ) { SetTextSize( VECTOR2I( aSize, aSize ) ); }

    COLOR4D GetFieldColor() const;

    void SetLastResolvedState( const SCH_ITEM* aItem ) override
    {
        const SCH_FIELD* aField = dynamic_cast<const SCH_FIELD*>( aItem );

        if( aField )
            m_lastResolvedColor = aField->m_lastResolvedColor;
    }

    std::vector<int> ViewGetLayers() const override;

    SCH_LAYER_ID GetDefaultLayer() const;

    /**
     * Adjusters to allow EDA_TEXT to draw/print/etc. text in absolute coords.
     */
    EDA_ANGLE GetDrawRotation() const override;

    KIFONT::FONT* GetDrawFont( const RENDER_SETTINGS* aSettings ) const override;

    const BOX2I GetBoundingBox() const override;

    /**
     * Return whether the field will be rendered with the horizontal justification
     * inverted due to rotation or mirroring of the parent.
     */
    bool IsHorizJustifyFlipped() const;
    bool IsVertJustifyFlipped() const;

    GR_TEXT_H_ALIGN_T GetEffectiveHorizJustify() const;
    GR_TEXT_V_ALIGN_T GetEffectiveVertJustify() const;

    void SetEffectiveHorizJustify( GR_TEXT_H_ALIGN_T );
    void SetEffectiveVertJustify( GR_TEXT_V_ALIGN_T );

    bool IsNameShown() const { return m_showName; }
    void SetNameShown( bool aShown = true ) { m_showName = aShown; }

    /**
     * Generated fields are fields whose names are variables like ${VAR}.
     *
     * The shown name of these fields is VAR and the value is resolved from
     * ${VAR}
     */
    bool IsGeneratedField() const { return m_isGeneratedField; }

    bool CanAutoplace() const { return m_allowAutoPlace; }
    void SetCanAutoplace( bool aCanPlace ) { m_allowAutoPlace = aCanPlace; }

    void swapData( SCH_ITEM* aItem ) override;

    int GetPenWidth() const override;

    bool IsAutoAdded() const { return m_autoAdded; }
    void SetAutoAdded( bool aAutoAdded ) { m_autoAdded = aAutoAdded; }

    bool ShowInChooser() const { return m_showInChooser; }
    void SetShowInChooser( bool aShow = true ) { m_showInChooser = aShow; }

    void ClearCaches() override;
    void ClearRenderCache() override;

    std::vector<std::unique_ptr<KIFONT::GLYPH>>*
    GetRenderCache( const wxString& forResolvedText, const VECTOR2I& forPosition,
                    TEXT_ATTRIBUTES& aAttrs ) const;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        Offset( aMoveVector );
    }

    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    void MirrorVertically( int aCenter ) override;
    void MirrorHorizontally( int aCenter ) override;

    void BeginEdit( const VECTOR2I& aStartPoint ) override;
    void CalcEdit( const VECTOR2I& aPosition ) override;

    void OnScintillaCharAdded( SCINTILLA_TRICKS* aScintillaTricks,
                               wxStyledTextEvent &aEvent ) const;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData = nullptr ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    BITMAPS GetMenuImage() const override;

    bool IsReplaceable() const override;

    VECTOR2I GetLibPosition() const { return EDA_TEXT::GetTextPos(); }

    VECTOR2I GetPosition() const override;
    void     SetPosition( const VECTOR2I& aPosition ) override;

    VECTOR2I GetParentPosition() const;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    EDA_ITEM* Clone() const override;

    /**
     * Copy parameters from a SCH_FIELD source.
     *
     * @note Pointers and specific values (position) are not copied.
     *
     * @param aSource is the SCH_FIELD to read.
     */
    void ImportValues( const SCH_FIELD& aSource );

    /**
     * Copy parameters of this field to another field.
     *
     * @note Pointers are not copied.
     *
     * @param aTarget Target field to copy values to.
     */
    void Copy( SCH_FIELD* aTarget ) const;

    bool IsMandatory() const;

    bool operator <( const SCH_ITEM& aItem ) const override;

    double Similarity( const SCH_ITEM& aItem ) const override;

    bool operator==( const SCH_ITEM& aItem ) const override;
    bool operator==( const SCH_FIELD& aItem ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    friend class SCH_IO_KICAD_SEXPR_PARSER;

    const KIFONT::METRICS& getFontMetrics() const override { return GetFontMetrics(); }

    /**
     * @copydoc SCH_ITEM::compare()
     *
     * The field specific sort order is as follows:
     *
     *      - Field ID, REFERENCE, VALUE, etc.
     *      - Field string, case insensitive compare.
     *      - Field horizontal (X) position.
     *      - Field vertical (Y) position.
     *      - Field width.
     *      - Field height.
     */
    int compare( const SCH_ITEM& aOther, int aCompareFlags = 0 ) const override;

    void setId( FIELD_T aId );

    wxString getUnescapedText( const SCH_SHEET_PATH* aPath = nullptr,
                               const wxString& aVariantName = wxEmptyString ) const;

private:
    FIELD_T  m_id;               ///< Field id, @see enum FIELD_T
    int      m_ordinal;          ///< Sort order for non-mandatory fields
    wxString m_name;

    bool     m_showName;         ///< Render the field name in addition to its value
    bool     m_allowAutoPlace;   ///< This field can be autoplaced
    bool     m_isGeneratedField; ///< If the field name is a variable name (e.g. ${DNP}) then
                                 ///<   the value field is forced to be the same as the name

    bool     m_autoAdded;        ///< Was this field automatically added to a LIB_SYMBOL?
    bool     m_showInChooser;    ///< This field is available as a data column for the chooser

    mutable bool                                        m_renderCacheValid;
    mutable VECTOR2I                                    m_renderCachePos;
    mutable std::vector<std::unique_ptr<KIFONT::GLYPH>> m_renderCache;

    mutable COLOR4D                                     m_lastResolvedColor;
};


inline int NextFieldOrdinal( const std::vector<SCH_FIELD>& aFields )
{
    int ordinal = 42;     // Arbitrarily larger than any mandatory FIELD_T id

    for( const SCH_FIELD& field : aFields )
        ordinal = std::max( ordinal, field.GetOrdinal() + 1 );

    return ordinal;
}


inline const SCH_FIELD* FindField( const std::vector<SCH_FIELD>& aFields, FIELD_T aFieldId )
{
    for( const SCH_FIELD& field : aFields )
    {
        if( field.GetId() == aFieldId )
            return &field;
    }

    return nullptr;
}


inline SCH_FIELD* FindField( std::vector<SCH_FIELD>& aFields, FIELD_T aFieldId )
{
    auto& constFields = const_cast<const std::vector<SCH_FIELD>&>( aFields );
    return const_cast<SCH_FIELD*>( FindField( constFields, aFieldId ) );
}


inline const SCH_FIELD* FindField( const std::vector<SCH_FIELD>& aFields,
                                   const wxString& aFieldName )
{
    for( const SCH_FIELD& field : aFields )
    {
        if( field.GetName() == aFieldName )
            return &field;
    }

    return nullptr;
}


inline SCH_FIELD* FindField( std::vector<SCH_FIELD>& aFields, const wxString& aFieldName )
{
    auto& constFields = const_cast<const std::vector<SCH_FIELD>&>( aFields );
    return const_cast<SCH_FIELD*>( FindField( constFields, aFieldName ) );
}


inline wxString GetFieldValue( const std::vector<SCH_FIELD>* aFields, FIELD_T aFieldType )
{
    if( !aFields )
        return wxEmptyString;

    if( const SCH_FIELD* field = FindField( *aFields, aFieldType ) )
        return field->GetText();

    return wxEmptyString;
}


inline std::string GetFieldValue( const std::vector<SCH_FIELD>* aFields,
                                  const wxString& aFieldName, bool aResolve, int aDepth )
{
    if( !aFields )
        return "";

    if( const SCH_FIELD* field = FindField( *aFields, aFieldName ) )
        return ( aResolve ? field->GetShownText( false, aDepth ) : field->GetText() ).ToStdString();

    return "";
}


inline void SetFieldValue( std::vector<SCH_FIELD>& aFields, const wxString& aFieldName,
                           const std::string& aValue, bool aIsVisible = true )
{
    if( aValue == "" )
    {
        std::erase_if( aFields, [&]( const SCH_FIELD& field )
                                 {
                                     return field.GetName() == aFieldName;
                                 } );
        return;
    }

    if( SCH_FIELD* field = FindField( aFields, aFieldName ) )
    {
        field->SetText( aValue );
        return;
    }

    SCH_ITEM* parent = static_cast<SCH_ITEM*>( aFields.at( 0 ).GetParent() );
    aFields.emplace_back( parent, FIELD_T::USER, aFieldName );
    aFields.back().SetText( aValue );
    aFields.back().SetVisible( aIsVisible );
}



#endif /* CLASS_SCH_FIELD_H */
