/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCB_PICKER_TOOL_H
#define PCB_PICKER_TOOL_H

#include <layer_ids.h>
#include <tool/picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_tool_base.h>

/**
 *Generic tool for picking an item.
 */
class PCB_PICKER_TOOL : public PCB_TOOL_BASE, public PICKER_TOOL_BASE
{
public:
    /**
     * Interface class for something that receives picked points
     * or items from this tool. Examples could be a dialog that's
     * asking the user to pick a point.
     */
    class RECEIVER
    {
    public:
        virtual void UpdatePickedPoint( const std::optional<VECTOR2I>& aPoint ) = 0;
        virtual void UpdatePickedItem( const EDA_ITEM* aItem ) = 0;

    protected:
        ~RECEIVER() = default;
    };

    struct INTERACTIVE_PARAMS
    {
        RECEIVER* m_Receiver = nullptr;
        wxString  m_Prompt;
        std::function<bool(EDA_ITEM*)> m_ItemFilter = nullptr;
    };

    PCB_PICKER_TOOL();
    virtual ~PCB_PICKER_TOOL() = default;

    ///< @copydoc TOOL_BASE::Init()
    bool Init() override;

    ///< Main event loop.
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Set the tool's snap layer set.
     */
    inline void SetLayerSet( const LSET& aLayerSet ) { m_layerMask = aLayerSet; }

    int SelectPointInteractively( const TOOL_EVENT& aEvent );
    int SelectItemInteractively( const TOOL_EVENT& aEvent );

    /**
     * Set a handler called with the drag box each time it changes, before anything is selected.
     *
     * Only the rubber-band drag uses it, so it lives here rather than on the generic picker.
     * Pairs with SetAreaHandler(), which runs once the drag is over.
     */
    inline void SetAreaPreviewHandler( PCB_SELECTION_TOOL::AREA_PREVIEW aHandler )
    {
        m_areaPreviewHandler = aHandler;
    }

    /// Whether the snap system may draw its explanatory geometry over the board this run.
    inline void SetConstructionGeometry( bool aEnable ) { m_constructionGeometry = aEnable; }

    /// Handlers only.  A caller sets this run's options and then clears the handlers, so
    /// anything reset here would go before Main() read it.  reset() puts the options back.
    inline void ClearHandlers()
    {
        m_areaPreviewHandler = nullptr;
        PICKER_TOOL_BASE::ClearHandlers();
    }

protected:
    ///< @copydoc TOOL_INTERACTIVE::setTransitions();
    void setTransitions() override;

    ///< Applies the requested VIEW_CONTROLS settings.
    void setControls();

    ///< Reinitialize tool to its initial state.
    void reset() override;

private:
    ///< The layer set to use for optional snapping.
    LSET                             m_layerMask;

    PCB_SELECTION_TOOL::AREA_PREVIEW  m_areaPreviewHandler;

    bool                              m_constructionGeometry = true;
};

#endif /* PCB_PICKER_TOOL_H */
