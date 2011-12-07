/**
 * @file  class_board_item.h
 * @brief Classes BOARD_ITEM and BOARD_CONNECTED_ITEM.
 */

#ifndef BOARD_ITEM_STRUCT_H
#define BOARD_ITEM_STRUCT_H


#include "base_struct.h"

#include <boost/ptr_container/ptr_vector.hpp>


class BOARD;
class EDA_DRAW_PANEL;


/**
 * Enum STROKE_T
 * is the set of shapes for segments (graphic segments and tracks) which are often
 * in the .m_Shape member
 */
enum STROKE_T
{
    S_SEGMENT = 0,  ///< usual segment : line with rounded ends
    S_RECT,         ///< segment with non rounded ends
    S_ARC,          ///< Arcs (with rounded ends)
    S_CIRCLE,       ///< ring
    S_POLYGON,      ///< polygon (not yet used for tracks, but could be in microwave apps)
    S_CURVE,        ///< Bezier Curve
    S_LAST          ///< last value for this list
};


/**
 * Class BOARD_ITEM
 * is a base class for any item which can be embedded within the BOARD
 * container class, and therefore instances of derived classes should only be
 * found in Pcbnew or other programs that use class BOARD and its contents.
 * The corresponding class in Eeschema is SCH_ITEM.
 */
class BOARD_ITEM : public EDA_ITEM
{
    // These are made private here so they may not be used.
    // Instead everything derived from BOARD_ITEM is handled via DLIST<>'s
    // use of DHEAD's member functions.
    void SetNext( EDA_ITEM* aNext )       { Pnext = aNext; }
    void SetBack( EDA_ITEM* aBack )       { Pback = aBack; }

protected:
    int m_Layer;

public:

    BOARD_ITEM( BOARD_ITEM* aParent, KICAD_T idtype ) :
        EDA_ITEM( aParent, idtype )
        , m_Layer( 0 )
    {
    }

    BOARD_ITEM( const BOARD_ITEM& src ) :
        EDA_ITEM( src.m_Parent, src.Type() )
        , m_Layer( src.m_Layer )
    {
        m_Flags = src.m_Flags;
    }

    /**
     * A value of wxPoint(0,0) which can be passed to the Draw() functions.
     */
    static wxPoint ZeroOffset;

    BOARD_ITEM* Next() const { return (BOARD_ITEM*) Pnext; }
    BOARD_ITEM* Back() const { return (BOARD_ITEM*) Pback; }
    BOARD_ITEM* GetParent() const { return (BOARD_ITEM*) m_Parent; }

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return const wxPoint - The position of this object
     */
    virtual const wxPoint GetPosition() const = 0;

     /**
      * Function SetPosition
      * sets the position of this object.
      * @param aPos is the new position of this object
      */
    virtual void SetPosition( const wxPoint& aPos ) = 0;

    /**
     * Function GetLayer
     * returns the layer this item is on.
     */
    int GetLayer() const { return m_Layer; }

    /**
     * Function SetLayer
     * sets the layer this item is on.
     * @param aLayer The layer number.
     * is virtual because some items (in fact: class DIMENSION)
     * have a slightly different initialization
     */
    virtual void SetLayer( int aLayer )  { m_Layer = aLayer; }

    /**
     * Function Draw
     * BOARD_ITEMs have their own color information.
     */
    virtual void Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
                       int aDrawMode, const wxPoint& offset = ZeroOffset ) = 0;

    /**
     * Function IsOnLayer
     * tests to see if this object is on the given layer.  Is virtual so
     * objects like D_PAD, which reside on multiple layers can do their own
     * form of testing.
     * @param aLayer The layer to test for.
     * @return bool - true if on given layer, else false.
     */
    virtual bool IsOnLayer( int aLayer ) const
    {
        return m_Layer == aLayer;
    }

    /**
     * Function IsTrack
     * tests to see if this object is a track or via (or microvia).
     * form of testing.
     * @return bool - true if a track or via, else false.
     */
    bool IsTrack() const
    {
        return ( Type() == PCB_TRACE_T ) || ( Type() == PCB_VIA_T );
    }

    /**
     * Function IsLocked
     * @return bool - true if the object is locked, else false
     */
    virtual bool IsLocked() const
    {
        return false;   // only MODULEs can be locked at this time.
    }


    /**
     * Function UnLink
     * detaches this object from its owner.  This base class implementation
     * should work for all derived classes which are held in a DLIST<>.
     */
    virtual void UnLink();

    /**
     * Function DeleteStructure
     * deletes this object after UnLink()ing it from its owner.
     */
    void DeleteStructure()
    {
        UnLink();
        delete this;
    }


    /**
     * Function ShowShape
     * converts the enum STROKE_T integer value to a wxString.
     */
    static wxString ShowShape( STROKE_T aShape );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const = 0;

    // Some geometric transforms, that must be rewritten for derived classes
    /**
     * Function Move
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        wxMessageBox( wxT( "virtual BOARD_ITEM::Move used, should not occur" ), GetClass() );
    }

    /**
     * Function Rotate
     * Rotate this object.
     * @param aRotCentre - the rotation point.
     * @param aAngle - the rotation angle in 0.1 degree.
     */
    virtual void Rotate( const wxPoint& aRotCentre, int aAngle )
    {
        wxMessageBox( wxT( "virtual BOARD_ITEM::Rotate used, should not occur" ), GetClass() );
    }

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param aCentre - the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre )
    {
        wxMessageBox( wxT( "virtual BOARD_ITEM::Flip used, should not occur" ), GetClass() );
    }

    /**
     * Function GetBoard
     * returns the BOARD in which this BOARD_ITEM resides, or NULL if none.
     */
    virtual BOARD* GetBoard() const;

    /**
     * Function GetLayerName
     * returns the name of the PCB layer on which the item resides.
     *
     * @return wxString containing the layer name associated with this item.
     */
    wxString GetLayerName() const;
};

#endif /* BOARD_ITEM_STRUCT_H */
