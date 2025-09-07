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

#include <geometry/corner_operations.h>

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
    * need the old state to be known, so this allows the caller to
    * inject the dependencies for how to handle the changes.
    */
    class CHANGE_HANDLER
    {
    public:
        virtual ~CHANGE_HANDLER() = default;

        /**
         * @brief Report that the tools wants to add a new item to the board
         *
         * @param aItem the new item
         */
        virtual void AddNewItem( std::unique_ptr<BOARD_ITEM> aItem ) = 0;

        /**
         * @brief Report that the tool has modified an item on the board
         *
         * @param aItem the modified item
         */
        virtual void MarkItemModified( BOARD_ITEM& aItem ) = 0;

        /**
         * @brief Report that the tool has deleted an item on the board
         *
         * @param aItem the deleted item
         */
        virtual void DeleteItem( BOARD_ITEM& aItem ) = 0;
    };

    /**
     * @brief A handler that is based on a set of callbacks provided
     * by the user of the ITEM_MODIFICATION_ROUTINE
     */
    class CALLABLE_BASED_HANDLER : public CHANGE_HANDLER
    {
    public:
        /**
         * Handler for creating a new item on the board
         *
         * @param BOARD_ITEM& the item to add
         */
        using CREATION_HANDLER = std::function<void( std::unique_ptr<BOARD_ITEM> )>;

        /**
         * Handler for modifying or deleting an existing item on the board
         *
         * @param BOARD_ITEM& the item to modify
         */
        using MODIFICATION_HANDLER = std::function<void( BOARD_ITEM& )>;

        /**
         * Handler for modifying or deleting an existing item on the board
         *
         * @param BOARD_ITEM& the item to delete
         */
        using DELETION_HANDLER = std::function<void( BOARD_ITEM& )>;

        CALLABLE_BASED_HANDLER( CREATION_HANDLER     aCreationHandler,
                                MODIFICATION_HANDLER aModificationHandler,
                                DELETION_HANDLER     aDeletionHandler ) :
                m_creationHandler( std::move( aCreationHandler ) ),
                m_modificationHandler( std::move( aModificationHandler ) ),
                m_deletionHandler( std::move( aDeletionHandler ) )
        {
        }

        /**
         * @brief Report that the tools wants to add a new item to the board
         *
         * @param aItem the new item
         */
        void AddNewItem( std::unique_ptr<BOARD_ITEM> aItem ) override
        {
            m_creationHandler( std::move( aItem ) );
        }

        /**
         * @brief Report that the tool has modified an item on the board
         *
         * @param aItem the modified item
         */
        void MarkItemModified( BOARD_ITEM& aItem ) override { m_modificationHandler( aItem ); }

        /**
         * @brief Report that the tool has deleted an item on the board
         *
         * @param aItem the deleted item
         */
        void DeleteItem( BOARD_ITEM& aItem ) override { m_deletionHandler( aItem ); }

        CREATION_HANDLER     m_creationHandler;
        MODIFICATION_HANDLER m_modificationHandler;
        DELETION_HANDLER     m_deletionHandler;
    };

    ITEM_MODIFICATION_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler ) :
            m_board( aBoard ),
            m_handler( aHandler ),
            m_numSuccesses( 0 ),
            m_numFailures( 0 )
    {
    }

    virtual ~ITEM_MODIFICATION_ROUTINE() = default;

    unsigned GetSuccesses() const { return m_numSuccesses; }

    unsigned GetFailures() const { return m_numFailures; }

    virtual wxString GetCommitDescription() const = 0;

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
     *  @brief Helper function useful for multiple tools: modify a line or delete
     * it if it has zero length
     *
     * @param aItem the line to modify
     * @param aSeg the new line geometry
     */
    bool ModifyLineOrDeleteIfZeroLength( PCB_SHAPE& aItem, const std::optional<SEG>& aSeg );

    /**
     * @brief Access the handler for making changes to the board
     */
    CHANGE_HANDLER& GetHandler() { return m_handler; }

private:
    BOARD_ITEM*          m_board;
    CHANGE_HANDLER&      m_handler;

    unsigned m_numSuccesses;
    unsigned m_numFailures;
};

/**
 * A tool that acts on a pair of lines. For example, fillets, chamfers, extensions, etc
 */
class PAIRWISE_LINE_ROUTINE : public ITEM_MODIFICATION_ROUTINE
{
public:
    PAIRWISE_LINE_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler ) :
            ITEM_MODIFICATION_ROUTINE( aBoard, aHandler )
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

    /**
     * @brief Get a status message to show when the routine is complete
     *
     * Usually this will be an error or nothing.
     */
    virtual std::optional<wxString> GetStatusMessage( int aSegmentCount ) const = 0;
};

/**
 * Pairwise line tool that adds a fillet to the lines.
 */
class LINE_FILLET_ROUTINE : public PAIRWISE_LINE_ROUTINE
{
public:
    LINE_FILLET_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler, int filletRadiusIU ) :
            PAIRWISE_LINE_ROUTINE( aBoard, aHandler ),
            m_filletRadiusIU( filletRadiusIU )
    {
    }

    wxString GetCommitDescription() const override;
    std::optional<wxString> GetStatusMessage( int aSegmentCount ) const override;

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
    LINE_CHAMFER_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler,
                          CHAMFER_PARAMS aChamferParams ) :
            PAIRWISE_LINE_ROUTINE( aBoard, aHandler ),
            m_chamferParams( std::move( aChamferParams ) )
    {
    }

    wxString GetCommitDescription() const override;
    std::optional<wxString> GetStatusMessage( int aSegmentCount ) const override;

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
    LINE_EXTENSION_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler ) :
            PAIRWISE_LINE_ROUTINE( aBoard, aHandler )
    {
    }

    wxString GetCommitDescription() const override;
    std::optional<wxString> GetStatusMessage( int aSegmentCount ) const override;

    void ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB ) override;
};

/**
 * Pairwise add dogbone corners to an internal corner.
 */
class DOGBONE_CORNER_ROUTINE : public PAIRWISE_LINE_ROUTINE
{
public:
    struct PARAMETERS
    {
        int  DogboneRadiusIU;
        bool AddSlots;
    };

    DOGBONE_CORNER_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler, PARAMETERS aParams ) :
            PAIRWISE_LINE_ROUTINE( aBoard, aHandler ),
            m_params( std::move( aParams ) ),
            m_haveNarrowMouths( false )
    {
    }

    wxString                GetCommitDescription() const override;
    std::optional<wxString> GetStatusMessage( int aSegmentCount ) const override;

    void ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB ) override;

private:
    // Lazily load board outline polygons (outer outline + any holes)
    bool EnsureBoardOutline() const;

    PARAMETERS m_params;
    bool m_haveNarrowMouths;
    mutable bool m_boardOutlineCached = false;
    mutable SHAPE_POLY_SET m_boardOutline; ///< Cached board outline polygons
};


/**
 * A routine that modifies polygons using boolean operations
 */
class POLYGON_BOOLEAN_ROUTINE : public ITEM_MODIFICATION_ROUTINE
{
public:
    POLYGON_BOOLEAN_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler ) :
            ITEM_MODIFICATION_ROUTINE( aBoard, aHandler )
    {
    }

    /**
     * @brief False if the order of the polygons matters
     */
    virtual bool IsCommutative() const = 0;

    void ProcessShape( PCB_SHAPE& aPcbShape );

    /**
     * Clear up any outstanding work
     */
    void Finalize();

    /**
     * @brief Get a status message to show when the routine is complete
     *
     * Usually this will be an error or nothing.
     */
    virtual std::optional<wxString> GetStatusMessage() const = 0;

protected:
    SHAPE_POLY_SET& GetWorkingPolygons() { return m_workingPolygons; }

    virtual bool ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon ) = 0;

private:
    /// This can be disjoint, which will be fixed at the end
    SHAPE_POLY_SET m_workingPolygons;

    bool         m_firstPolygon = true;
    int          m_width = 0;
    PCB_LAYER_ID m_layer = PCB_LAYER_ID::UNDEFINED_LAYER;
    FILL_T       m_fillMode = FILL_T::NO_FILL;
};

class POLYGON_MERGE_ROUTINE : public POLYGON_BOOLEAN_ROUTINE
{
public:
    POLYGON_MERGE_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler ) :
            POLYGON_BOOLEAN_ROUTINE( aBoard, aHandler )
    {
    }

    bool IsCommutative() const override { return true; }

    wxString                GetCommitDescription() const override;
    std::optional<wxString> GetStatusMessage() const override;

private:
    bool ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon ) override;
};


class POLYGON_SUBTRACT_ROUTINE : public POLYGON_BOOLEAN_ROUTINE
{
public:
    POLYGON_SUBTRACT_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler ) :
            POLYGON_BOOLEAN_ROUTINE( aBoard, aHandler )
    {
    }

    bool IsCommutative() const override { return false; }

    wxString                GetCommitDescription() const override;
    std::optional<wxString> GetStatusMessage() const override;

private:
    bool ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon ) override;
};


class POLYGON_INTERSECT_ROUTINE : public POLYGON_BOOLEAN_ROUTINE
{
public:
    POLYGON_INTERSECT_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler ) :
            POLYGON_BOOLEAN_ROUTINE( aBoard, aHandler )
    {
    }

    bool IsCommutative() const override { return true; }

    wxString                GetCommitDescription() const override;
    std::optional<wxString> GetStatusMessage() const override;

private:
    bool ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon ) override;
};


class OUTSET_ROUTINE : public ITEM_MODIFICATION_ROUTINE
{
public:
    struct PARAMETERS
    {
        int                outsetDistance;
        bool               roundCorners;
        bool               useSourceLayers;
        bool               useSourceWidths;
        PCB_LAYER_ID       layer;
        int                lineWidth;
        std::optional<int> gridRounding;
        bool               deleteSourceItems;
    };

    OUTSET_ROUTINE( BOARD_ITEM* aBoard, CHANGE_HANDLER& aHandler, const PARAMETERS& aParams ) :
            ITEM_MODIFICATION_ROUTINE( aBoard, aHandler ),
            m_params( aParams )
    {
    }

    wxString GetCommitDescription() const override;

    std::optional<wxString> GetStatusMessage() const;

    void ProcessItem( BOARD_ITEM& aItem );

private:
    const PARAMETERS m_params;
};

#endif /* ITEM_MODIFICATION_ROUTINE_H_ */
