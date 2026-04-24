/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef DS_PROXY_VIEW_ITEM_H
#define DS_PROXY_VIEW_ITEM_H

#include <eda_item.h>
#include <eda_units.h>
#include <functional>
#include <vector>

class BOARD;
class PAGE_INFO;
class PROJECT;
class TITLE_BLOCK;
class DS_DRAW_ITEM_LINE;
class DS_DRAW_ITEM_RECT;
class DS_DRAW_ITEM_TEXT;
class DS_DRAW_ITEM_BITMAP;
class DS_DRAW_ITEM_LIST;
class TEXT_VAR_TRACKER;
struct TEXT_VAR_REF_KEY;

namespace KIGFX
{
class VIEW;
class GAL;
}

class DS_PROXY_VIEW_ITEM : public EDA_ITEM
{
public:
    DS_PROXY_VIEW_ITEM( const EDA_IU_SCALE& aIuScale, const PAGE_INFO* aPageInfo,
                        const PROJECT* aProject, const TITLE_BLOCK* aTitleBlock,
                        const std::map<wxString, wxString>* aProperties );

    /**
     * Set the file name displayed in the title block.
     */
    void SetFileName( const std::string& aFileName ) { m_fileName = aFileName; }

    /**
     * Set the sheet name displayed in the title block.
     */
    void SetSheetName( const std::string& aSheetName ) { m_sheetName = aSheetName; }

    /**
     * Set the sheet path displayed in the title block.
     */
    void SetSheetPath( const std::string& aSheetPath ) { m_sheetPath = aSheetPath; }

    /**
     * Change the page number displayed in the title block.
     */
    void SetPageNumber( const std::string& aPageNumber ) { m_pageNumber = aPageNumber; }

    /**
     * Change the sheet-count number displayed in the title block.
     */
    void SetSheetCount( int aSheetCount ) { m_sheetCount = aSheetCount; }

    /**
     * Change if this is first page.
     *
     * Title blocks have an option to allow all subsequent pages to not display a title
     * block.  This needs to be set to false when displaying any page but the first page.
     */
    void SetIsFirstPage( bool aIsFirstPage ) { m_isFirstPage = aIsFirstPage; }

    /**
     * Set the current variant name and description to be shown on the drawing sheet.
     */
    void SetVariantName( const std::string& aVariant ) { m_variantName = aVariant; }
    void SetVariantDesc( const std::string& aVariantDesc ) { m_variantDesc = aVariantDesc; }

    /**
     * Can be used to override which layer ID is used for drawing sheet item colors
     * @param aLayerId is the color to use (defaults to LAYER_DRAWINGSHEET if this is not called)
     */
    void SetColorLayer( int aLayerId ) { m_colorLayer = aLayerId; }

    /**
     * Override the layer used to pick the color of the page border (normally LAYER_GRID)
     *
     * @param aLayerId is the layer to use
     */
    void SetPageBorderColorLayer( int aLayerId ) { m_pageBorderColorLayer = aLayerId; }

    const PAGE_INFO& GetPageInfo() { return *m_pageInfo; }
    const TITLE_BLOCK& GetTitleBlock() { return *m_titleBlock; }

    /// @copydoc VIEW_ITEM::ViewBBox()
    const BOX2I ViewBBox() const override;

    /// @copydoc VIEW_ITEM::ViewDraw()
    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    std::vector<int> ViewGetLayers() const override;

#if defined(DEBUG)
    /// @copydoc EDA_ITEM::Show()
    void Show( int x, std::ostream& st ) const override { }
#endif

    /**
     * Get class name.
     *
     * @return  string "DS_PROXY_VIEW_ITEM"
     */
    virtual wxString GetClass() const override
    {
        return wxT( "DS_PROXY_VIEW_ITEM" );
    }

    bool HitTestDrawingSheetItems( KIGFX::VIEW* aView, const VECTOR2I& aPosition );

    /**
     * Walk the current drawing-sheet definition and collect every `${...}`
     * reference encountered in its text items. Used by reactive integrations
     * (BOARD / SCHEMATIC text-var trackers) to register the proxy as a single
     * dependent covering the whole title block — any source change fans out
     * to one repaint rather than per-text-item.
     */
    std::vector<TEXT_VAR_REF_KEY> CollectTextVarKeys() const;

    /**
     * Register this proxy with @p aTracker as a dependent on every
     * title-block source variable its current template references. The
     * proxy retains a pointer to @p aTracker and unregisters automatically
     * in its destructor.
     *
     * Callers are responsible for attaching a single long-lived invalidate
     * listener to @p aTracker that routes per-proxy invalidations (e.g.,
     * look up the canvas's current drawing sheet and call VIEW::Update on
     * it). This avoids listener accumulation when the proxy is replaced on
     * page-setup changes.
     *
     * Passing @p aTracker as nullptr detaches.
     */
    void AttachToTracker( TEXT_VAR_TRACKER* aTracker );

    ~DS_PROXY_VIEW_ITEM() override;

protected:
    void buildDrawList( KIGFX::VIEW* aView, const std::map<wxString, wxString>* aProperties,
                        DS_DRAW_ITEM_LIST* aDrawList ) const;

protected:
    const EDA_IU_SCALE& m_iuScale;

    std::string         m_fileName;
    std::string         m_sheetName;
    std::string         m_sheetPath;
    const TITLE_BLOCK*  m_titleBlock;
    const PAGE_INFO*    m_pageInfo;
    std::string         m_pageNumber;
    int                 m_sheetCount;
    bool                m_isFirstPage;
    std::string         m_variantName;
    std::string         m_variantDesc;
    const PROJECT*      m_project;

    const std::map<wxString, wxString>* m_properties;

    /**
     * Layer that is used for drawing sheet color (LAYER_DRAWINGSHEET is always used
     * for visibility).
     */
    int                 m_colorLayer;

    /// Layer that is used for page border color
    int                 m_pageBorderColorLayer;

    /// Tracker this proxy is currently registered with (null if detached).
    /// Used by the destructor to unregister cleanly.
    TEXT_VAR_TRACKER*   m_attachedTracker = nullptr;
};

#endif /* DS_PROXY_VIEW_ITEM_H */
