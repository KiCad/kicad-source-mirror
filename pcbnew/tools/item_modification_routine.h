/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ITEM_MODIFICATION_ROUTINE_H_
#define ITEM_MODIFICATION_ROUTINE_H_

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <board_item.h>
#include <pcb_shape.h>

#include <geometry/chamfer.h>

/**
 * @brief An object that has the ability to modify items on a board
 *
 * For example, such an object could take pairs of lines and fillet them,
 * or produce other modification to items.
 *
 * Deliberately not called a "tool" to distinguish it from true
 * tools that are used interactively by the user.
 */
class ITEM_MODIFICATION_ROUTINE
{
public:
    /*
     * Handlers for receiving changes from the tool
     *
     * These are used to allow the tool's caller to make changes to
     * affected board items using extra information that the tool
     * does not have access to (e.g. is this an FP editor, was
     * the line created from a rectangle and needs to be added, not
     * modified, etc).
     *
     * We can't store them up until the end, because modifications
     * need the old state to be known.
     */

    /**
     * Handler for creating a new item on the board
     *
     * @param PCB_SHAPE& the shape to add
     */
    using CREATION_HANDLER = std::function<void( std::unique_ptr<PCB_SHAPE> )>;

    /**
     * Handler for modifying an existing item on the board
     *
     * @param PCB_SHAPE& the shape to modify
     */
    using MODIFICATION_HANDLER = std::function<void( PCB_SHAPE& )>;

    ITEM_MODIFICATION_ROUTINE( BOARD_ITEM* aBoard, CREATION_HANDLER aCreationHandler,
                               MODIFICATION_HANDLER aModificationHandler ) :
            m_board( aBoard ),
            m_creationHandler( aCreationHandler ), m_modificationHandler( aModificationHandler ),
            m_numSuccesses( 0 ), m_numFailures( 0 )
    {
    }

    virtual ~ITEM_MODIFICATION_ROUTINE() = default;

    unsigned GetSuccesses() const { return m_numSuccesses; }

    unsigned GetFailures() const { return m_numFailures; }

    virtual wxString GetCommitDescription() const = 0;

    virtual wxString GetCompleteFailureMessage() const = 0;
    virtual wxString GetSomeFailuresMessage() const = 0;

protected:
    /**
     * The BOARD used when creating new shapes
     */
    BOARD_ITEM* GetBoard() const { return m_board; }

    /**
     * Mark that one of the actions succeeded.
     */
    void AddSuccess() { ++m_numSuccesses; }

    /**
     * Mark that one of the actions failed.
     */
    void AddFailure() { ++m_numFailures; }

    /**
     * @brief Report that the tools wants to add a new item to the board
     *
     * @param aItem the new item
     */
    void AddNewItem( std::unique_ptr<PCB_SHAPE> aItem ) { m_creationHandler( std::move( aItem ) ); }

    /**
     * @brief Report that the tool has modified an item on the board
     *
     * @param aItem the modified item
     */
    void MarkItemModified( PCB_SHAPE& aItem ) { m_modificationHandler( aItem ); }

private:
    BOARD_ITEM*          m_board;
    CREATION_HANDLER     m_creationHandler;
    MODIFICATION_HANDLER m_modificationHandler;

    unsigned m_numSuccesses;
    unsigned m_numFailures;
};

/**
 * A tool that acts on a pair of lines. For example, fillets, chamfers, extensions, etc
 */
class PAIRWISE_LINE_ROUTINE : public ITEM_MODIFICATION_ROUTINE
{
public:
    PAIRWISE_LINE_ROUTINE( BOARD_ITEM* aBoard, CREATION_HANDLER aCreationHandler,
                           MODIFICATION_HANDLER aModificationHandler ) :
            ITEM_MODIFICATION_ROUTINE( aBoard, aCreationHandler, aModificationHandler )
    {
    }

    /**
     * @brief Perform the action on the pair of lines given
     *
     * The routine will be called repeatedly with all possible pairs of lines
     * in the selection. The tools should handle the ones it's interested in.
     * This means that the same line can appear multiple times with different
     * partners.
     *
     * The routine can skip lines that it's not interested in by returning without
     * adding to the success or failure count.
     *
     * @param aLineA the first line
     * @param aLineB the second line
     * @return did the action succeed
     */
    virtual void ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB ) = 0;
};

/**
 * Pairwise line tool that adds a fillet to the lines.
 */
class LINE_FILLET_ROUTINE : public PAIRWISE_LINE_ROUTINE
{
public:
    LINE_FILLET_ROUTINE( BOARD_ITEM* aBoard, CREATION_HANDLER aCreationHandler,
                         MODIFICATION_HANDLER aModificationHandler, int filletRadiusIU ) :
            PAIRWISE_LINE_ROUTINE( aBoard, aCreationHandler, aModificationHandler ),
            m_filletRadiusIU( filletRadiusIU )
    {
    }

    wxString GetCommitDescription() const override;
    wxString GetCompleteFailureMessage() const override;
    wxString GetSomeFailuresMessage() const override;

    void ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB ) override;

private:
    int m_filletRadiusIU;
};

/**
 * Pairwise line tool that adds a chamfer between the lines.
 */
class LINE_CHAMFER_ROUTINE : public PAIRWISE_LINE_ROUTINE
{
public:
    LINE_CHAMFER_ROUTINE( BOARD_ITEM* aBoard, CREATION_HANDLER aCreationHandler,
                          MODIFICATION_HANDLER aModificationHandler,
                          CHAMFER_PARAMS       aChamferParams ) :
            PAIRWISE_LINE_ROUTINE( aBoard, aCreationHandler, aModificationHandler ),
            m_chamferParams( std::move( aChamferParams ) )
    {
    }

    wxString GetCommitDescription() const override;
    wxString GetCompleteFailureMessage() const override;
    wxString GetSomeFailuresMessage() const override;

    void ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB ) override;

private:
    const CHAMFER_PARAMS m_chamferParams;
};

/**
 * Pairwise extend to corner or meeting tool
 */
class LINE_EXTENSION_ROUTINE : public PAIRWISE_LINE_ROUTINE
{
public:
    LINE_EXTENSION_ROUTINE( BOARD_ITEM* aBoard, CREATION_HANDLER aCreationHandler,
                            MODIFICATION_HANDLER aModificationHandler ) :
            PAIRWISE_LINE_ROUTINE( aBoard, aCreationHandler, aModificationHandler )
    {
    }

    wxString GetCommitDescription() const override;
    wxString GetCompleteFailureMessage() const override;
    wxString GetSomeFailuresMessage() const override;

    void ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB ) override;
};

#endif /* ITEM_MODIFICATION_ROUTINE_H_ */
