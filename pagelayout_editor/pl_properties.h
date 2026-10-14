/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <base_units.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <eda_item.h>

/**
 * Exposes a drawing-sheet definition to the shared properties panel.
 *
 * One definition can produce many selected draw items.  Editing the definition, rather than a
 * rendered copy, preserves its corner anchors, repeat parameters and zero-as-default values.
 * Distances are converted from the file model's millimetres to drawing-sheet internal units.
 */
class PL_ITEM_PROPERTIES : public EDA_ITEM
{
public:
    explicit PL_ITEM_PROPERTIES( DS_DATA_ITEM* aItem ) :
            EDA_ITEM( TYPE_NOT_INIT ),
            m_item( aItem )
    {
    }

    wxString      GetClass() const override { return wxS( "PL_ITEM_PROPERTIES" ); }
    wxString      GetFriendlyName() const override { return m_item->GetClassName(); }
    DS_DATA_ITEM* GetItem() const { return m_item; }

    wxString GetComment() const { return m_item->m_Info; }
    void     SetComment( wxString aValue ) { m_item->m_Info = aValue; }

    PAGE_OPTION GetPageOption() const { return m_item->GetPage1Option(); }
    void        SetPageOption( PAGE_OPTION aValue ) { m_item->SetPage1Option( aValue ); }

    int  GetStartX() const { return KiROUND( m_item->m_Pos.m_Pos.x * drawSheetIUScale.IU_PER_MM ); }
    void SetStartX( int aValue ) { m_item->m_Pos.m_Pos.x = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetStartY() const { return KiROUND( m_item->m_Pos.m_Pos.y * drawSheetIUScale.IU_PER_MM ); }
    void SetStartY( int aValue ) { m_item->m_Pos.m_Pos.y = aValue / drawSheetIUScale.IU_PER_MM; }

    CORNER_ANCHOR GetStartAnchor() const { return static_cast<CORNER_ANCHOR>( m_item->m_Pos.m_Anchor ); }
    void          SetStartAnchor( CORNER_ANCHOR aValue ) { m_item->m_Pos.m_Anchor = aValue; }

    int  GetEndX() const { return KiROUND( m_item->m_End.m_Pos.x * drawSheetIUScale.IU_PER_MM ); }
    void SetEndX( int aValue ) { m_item->m_End.m_Pos.x = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetEndY() const { return KiROUND( m_item->m_End.m_Pos.y * drawSheetIUScale.IU_PER_MM ); }
    void SetEndY( int aValue ) { m_item->m_End.m_Pos.y = aValue / drawSheetIUScale.IU_PER_MM; }

    CORNER_ANCHOR GetEndAnchor() const { return static_cast<CORNER_ANCHOR>( m_item->m_End.m_Anchor ); }
    void          SetEndAnchor( CORNER_ANCHOR aValue ) { m_item->m_End.m_Anchor = aValue; }

    int  GetLineWidth() const { return KiROUND( m_item->m_LineWidth * drawSheetIUScale.IU_PER_MM ); }
    void SetLineWidth( int aValue ) { m_item->m_LineWidth = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetRepeatCount() const { return m_item->m_RepeatCount; }
    void SetRepeatCount( int aValue ) { m_item->m_RepeatCount = aValue; }

    int  GetStepX() const { return KiROUND( m_item->m_IncrementVector.x * drawSheetIUScale.IU_PER_MM ); }
    void SetStepX( int aValue ) { m_item->m_IncrementVector.x = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetStepY() const { return KiROUND( m_item->m_IncrementVector.y * drawSheetIUScale.IU_PER_MM ); }
    void SetStepY( int aValue ) { m_item->m_IncrementVector.y = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetLabelIncrement() const { return m_item->m_IncrementLabel; }
    void SetLabelIncrement( int aValue ) { m_item->m_IncrementLabel = aValue; }

    wxString GetText() const { return text()->m_TextBase; }
    void     SetText( wxString aValue ) { text()->m_TextBase = aValue; }

    int  GetTextWidth() const { return KiROUND( text()->m_TextSize.x * drawSheetIUScale.IU_PER_MM ); }
    void SetTextWidth( int aValue ) { text()->m_TextSize.x = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetTextHeight() const { return KiROUND( text()->m_TextSize.y * drawSheetIUScale.IU_PER_MM ); }
    void SetTextHeight( int aValue ) { text()->m_TextSize.y = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetConstraintWidth() const { return KiROUND( text()->m_BoundingBoxSize.x * drawSheetIUScale.IU_PER_MM ); }
    void SetConstraintWidth( int aValue ) { text()->m_BoundingBoxSize.x = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetConstraintHeight() const { return KiROUND( text()->m_BoundingBoxSize.y * drawSheetIUScale.IU_PER_MM ); }
    void SetConstraintHeight( int aValue ) { text()->m_BoundingBoxSize.y = aValue / drawSheetIUScale.IU_PER_MM; }

    GR_TEXT_H_ALIGN_T GetHAlign() const { return text()->m_Hjustify; }
    void              SetHAlign( GR_TEXT_H_ALIGN_T aValue ) { text()->m_Hjustify = aValue; }

    GR_TEXT_V_ALIGN_T GetVAlign() const { return text()->m_Vjustify; }
    void              SetVAlign( GR_TEXT_V_ALIGN_T aValue ) { text()->m_Vjustify = aValue; }

    KIGFX::COLOR4D GetTextColor() const { return text()->m_TextColor; }
    void           SetTextColor( KIGFX::COLOR4D aValue ) { text()->m_TextColor = aValue; }

    int  GetPPI() const { return static_cast<DS_DATA_ITEM_BITMAP*>( m_item )->GetPPI(); }
    void SetPPI( int aValue ) { static_cast<DS_DATA_ITEM_BITMAP*>( m_item )->SetPPI( aValue ); }

    double GetRotation() const;
    void   SetRotation( double aValue );

    wxString GetFont() const;
    void     SetFont( wxString aValue );
    bool     GetBold() const { return text()->m_Bold; }
    void     SetBold( bool aValue );
    bool     GetItalic() const { return text()->m_Italic; }
    void     SetItalic( bool aValue );

private:
    DS_DATA_ITEM_TEXT* text() const { return static_cast<DS_DATA_ITEM_TEXT*>( m_item ); }
    DS_DATA_ITEM*      m_item;
};


/** Drawing-sheet defaults and margins, shown when no item is selected. */
class PL_PAGE_PROPERTIES : public EDA_ITEM
{
public:
    explicit PL_PAGE_PROPERTIES( DS_DATA_MODEL& aModel ) :
            EDA_ITEM( TYPE_NOT_INIT ),
            m_model( aModel )
    {
    }

    wxString GetClass() const override { return wxS( "PL_PAGE_PROPERTIES" ); }
    wxString GetFriendlyName() const override { return _( "Drawing Sheet Defaults" ); }

    int  GetDefaultTextWidth() const { return KiROUND( m_model.m_DefaultTextSize.x * drawSheetIUScale.IU_PER_MM ); }
    void SetDefaultTextWidth( int aValue ) { m_model.m_DefaultTextSize.x = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetDefaultTextHeight() const { return KiROUND( m_model.m_DefaultTextSize.y * drawSheetIUScale.IU_PER_MM ); }
    void SetDefaultTextHeight( int aValue ) { m_model.m_DefaultTextSize.y = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetDefaultLineWidth() const { return KiROUND( m_model.m_DefaultLineWidth * drawSheetIUScale.IU_PER_MM ); }
    void SetDefaultLineWidth( int aValue ) { m_model.m_DefaultLineWidth = aValue / drawSheetIUScale.IU_PER_MM; }

    int GetDefaultTextThickness() const
    {
        return KiROUND( m_model.m_DefaultTextThickness * drawSheetIUScale.IU_PER_MM );
    }
    void SetDefaultTextThickness( int aValue ) { m_model.m_DefaultTextThickness = aValue / drawSheetIUScale.IU_PER_MM; }

    int  GetLeftMargin() const { return KiROUND( m_model.GetLeftMargin() * drawSheetIUScale.IU_PER_MM ); }
    void SetLeftMargin( int aValue ) { m_model.SetLeftMargin( aValue / drawSheetIUScale.IU_PER_MM ); }

    int  GetRightMargin() const { return KiROUND( m_model.GetRightMargin() * drawSheetIUScale.IU_PER_MM ); }
    void SetRightMargin( int aValue ) { m_model.SetRightMargin( aValue / drawSheetIUScale.IU_PER_MM ); }

    int  GetTopMargin() const { return KiROUND( m_model.GetTopMargin() * drawSheetIUScale.IU_PER_MM ); }
    void SetTopMargin( int aValue ) { m_model.SetTopMargin( aValue / drawSheetIUScale.IU_PER_MM ); }

    int  GetBottomMargin() const { return KiROUND( m_model.GetBottomMargin() * drawSheetIUScale.IU_PER_MM ); }
    void SetBottomMargin( int aValue ) { m_model.SetBottomMargin( aValue / drawSheetIUScale.IU_PER_MM ); }

private:
    DS_DATA_MODEL& m_model;
};
