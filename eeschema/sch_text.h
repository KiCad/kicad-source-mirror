/********************************************/
/* Definitions for the EESchema program:    */
/********************************************/

#ifndef CLASS_TEXT_LABEL_H
#define CLASS_TEXT_LABEL_H


#include "macros.h"
#include "sch_item_struct.h"


class LINE_READER;


/* Type of SCH_HIERLABEL and SCH_GLOBALLABEL
 * mainly used to handle the graphic associated shape
 */
typedef enum {
    NET_INPUT,
    NET_OUTPUT,
    NET_BIDI,
    NET_TRISTATE,
    NET_UNSPECIFIED,
    NET_TMAX        /* Last value */
} TypeSheetLabel;


extern const char* SheetLabelType[];    /* names of types of labels */

class SCH_TEXT : public SCH_ITEM, public EDA_TextStruct
{
public:
    int  m_Shape;
    bool m_IsDangling;          // true if not connected (used to draw the "not
                                // connected" symbol
protected:
    int  m_SchematicOrientation;    /* orientation of texts (comments) and
                                     * labels in schematic
                                     *  0 = normal (horizontal, left
                                     * justified).
                                     *  1 = up (vertical)
                                     *  2 =  (horizontal, right justified).
                                     * This can be seen as the mirrored
                                     * position of 0
                                     *  3 = bottom . This can be seen as the
                                     * mirrored position of up
                                     *  this is perhaps a duplicate of m_Orient
                                     * and m_HJustified or m_VJustified,
                                     *  but is more easy to handle that 3
                                     * parameters in editions, Reading and
                                     * Saving file
                                     */

public:
    SCH_TEXT( const wxPoint& pos = wxPoint( 0, 0 ),
              const wxString& text = wxEmptyString,
              KICAD_T aType = SCH_TEXT_T );
    ~SCH_TEXT() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_TEXT" );
    }


    /**
     * Function SetTextOrientAndJustifyParmeters
     * Set m_SchematicOrientation, and initialize
     * m_orient,m_HJustified and m_VJustified, according to the value of
     * m_SchematicOrientation (for a text )
     * must be called after changing m_SchematicOrientation
     * @param aSchematicOrientation =
     *  0 = normal (horizontal, left justified).
     *  1 = up (vertical)
     *  2 =  (horizontal, right justified). This can be seen as the mirrored
     * position of 0
     *  3 = bottom . This can be seen as the mirrored position of up
     */
    virtual void SetSchematicTextOrientation( int aSchematicOrientation );

    int GetSchematicTextOrientation() { return m_SchematicOrientation; }

    /**
     * Function GetSchematicTextOffset (virtual)
     * @return the offset between the SCH_TEXT position and the text itself
     * position
     * This offset depend on orientation, and the type of text
     * (room to draw an associated graphic symbol, or put the text above a
     * wire)
     */
    virtual wxPoint GetSchematicTextOffset();

    SCH_TEXT* GenCopy();

    virtual void Draw( WinEDA_DrawPanel* panel,
                       wxDC*             DC,
                       const wxPoint&    offset,
                       int               draw_mode,
                       int               Color = -1 );

    /**
     * Function CreateGraphicShape
     * Calculates the graphic shape (a polygon) associated to the text
     * @param aCorner_list = a buffer to fill with polygon corners coordinates
     * @param Pos = Postion of the shape
     * for texts and labels: do nothing
     * Mainly for derived classes (SCH_SHEET_PIN and Hierarchical labels)
     */
    virtual void CreateGraphicShape( std::vector <wxPoint>& aCorner_list, const wxPoint& Pos )
    {
        aCorner_list.clear();
    }

    void SwapData( SCH_TEXT* copyitem );

    void Place( SCH_EDIT_FRAME* frame, wxDC* DC );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_Rect GetBoundingBox() const;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic text entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic text from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic text.
     * @return True if the schematic text loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    int GetPenSize() const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Rotate( wxPoint rotationPoint );
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );


    /**
     * Compare schematic text entry against search string.
     *
     * @param aSearchData - Criterial to search against.
     * @param aAuxData - a pointer on auxiliary data, if needed. Can be null
     * @param aFindLocation - a wxPoint where to put the location of matched item. can be NULL.
     * @return True if this schematic text item matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation );

    virtual void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsDangling() const { return m_IsDangling; }

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

#if defined(DEBUG)
    void         Show( int nestLevel, std::ostream& os );
#endif

private:
    virtual bool DoHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual bool DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const;
};


class SCH_LABEL : public SCH_TEXT
{
public:
    SCH_LABEL( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );
    ~SCH_LABEL() { }

    virtual void Draw( WinEDA_DrawPanel* panel,
                       wxDC*             DC,
                       const wxPoint&    offset,
                       int               draw_mode,
                       int               Color = -1 );

    virtual wxString GetClass() const
    {
        return wxT( "SCH_LABEL" );
    }


    /**
     * Function SetTextOrientAndJustifyParmeters
     * Set m_SchematicOrientation, and initialize
     * m_orient,m_HJustified and m_VJustified, according to the value of
     * m_SchematicOrientation (for a label)
     * must be called after changing m_SchematicOrientation
     * @param aSchematicOrientation =
     *  0 = normal (horizontal, left justified).
     *  1 = up (vertical)
     *  2 =  (horizontal, right justified). This can be seen as the mirrored
     * position of 0
     *  3 = bottom . This can be seen as the mirrored position of up
     */
    virtual void    SetSchematicTextOrientation( int aSchematicOrientation );

    /**
     * Function GetSchematicTextOffset (virtual)
     * @return the offset between the SCH_TEXT position and the text itself
     * position
     * This offset depend on orientation, and the type of text
     * (room to draw an associated graphic symbol, or put the text above a
     * wire)
     */
    virtual wxPoint GetSchematicTextOffset();
    virtual void    Mirror_X( int aXaxis_position );
    virtual void    Rotate( wxPoint rotationPoint );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_Rect        GetBoundingBox() const;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool            Save( FILE* aFile ) const;

    /**
     * Load schematic label entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic label from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic label.
     * @return True if the schematic label loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );
};


class SCH_GLOBALLABEL : public SCH_TEXT
{
public:
    SCH_GLOBALLABEL( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );
    ~SCH_GLOBALLABEL() { }

    virtual void Draw( WinEDA_DrawPanel* panel,
                       wxDC*             DC,
                       const wxPoint&    offset,
                       int               draw_mode,
                       int               Color = -1 );

    virtual wxString GetClass() const
    {
        return wxT( "SCH_GLOBALLABEL" );
    }


    /**
     * Function SetTextOrientAndJustifyParmeters
     * Set m_SchematicOrientation, and initialize
     * m_orient,m_HJustified and m_VJustified, according to the value of
     * m_SchematicOrientation
     * must be called after changing m_SchematicOrientation
     * @param aSchematicOrientation =
     *  0 = normal (horizontal, left justified).
     *  1 = up (vertical)
     *  2 = (horizontal, right justified). This can be seen as the mirrored
     *      position of 0
     *  3 = bottom . This can be seen as the mirrored position of up
     */
    virtual void    SetSchematicTextOrientation( int aSchematicOrientation );

    /**
     * Function GetSchematicTextOffset (virtual)
     * @return the offset between the SCH_TEXT position and the text itself
     * position
     * This offset depend on orientation, and the type of text
     * (room to draw an associated graphic symbol, or put the text above a
     * wire)
     */
    virtual wxPoint GetSchematicTextOffset();

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool            Save( FILE* aFile ) const;

    /**
     * Load schematic global label entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic global label from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic global label.
     * @return True if the schematic global label loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_Rect        GetBoundingBox() const;

    /**
     * Function CreateGraphicShape (virual)
     * Calculates the graphic shape (a polygon) associated to the text
     * @param aCorner_list = a buffer to fill with polygon corners coordinates
     * @param aPos = Position of the shape
     */
    virtual void    CreateGraphicShape( std::vector <wxPoint>& aCorner_list,
                                        const wxPoint&         aPos );

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void    Mirror_Y( int aYaxis_position );
    virtual void    Mirror_X( int aXaxis_position );
    virtual void    Rotate( wxPoint rotationPoint );
};


class SCH_HIERLABEL : public SCH_TEXT
{
public:
    SCH_HIERLABEL( const wxPoint& pos = wxPoint( 0, 0 ),
                   const wxString& text = wxEmptyString,
                   KICAD_T aType = SCH_HIERARCHICAL_LABEL_T );

    ~SCH_HIERLABEL() { }
    virtual void Draw( WinEDA_DrawPanel* panel,
                       wxDC*             DC,
                       const wxPoint&    offset,
                       int               draw_mode,
                       int               Color = -1 );

    virtual wxString GetClass() const
    {
        return wxT( "SCH_HIERLABEL" );
    }


    /**
     * Function SetTextOrientAndJustifyParmeters
     * Set m_SchematicOrientation, and initialize
     * m_orient,m_HJustified and m_VJustified, according to the value of
     * m_SchematicOrientation
     * must be called after changing m_SchematicOrientation
     * @param aSchematicOrientation =
     *  0 = normal (horizontal, left justified).
     *  1 = up (vertical)
     *  2 =  (horizontal, right justified). This can be seen as the mirrored
     * position of 0
     *  3 = bottom . This can be seen as the mirrored position of up
     */
    virtual void    SetSchematicTextOrientation( int aSchematicOrientation );

    /**
     * Function GetSchematicTextOffset (virtual)
     * @return the offset between the SCH_TEXT position and the text itself
     * position
     * This offset depend on orientation, and the type of text
     * (room to draw an associated graphic symbol, or put the text above a
     * wire)
     */
    virtual wxPoint GetSchematicTextOffset();

    /**
     * Function CreateGraphicShape
     * Calculates the graphic shape (a polygon) associated to the text
     * @param aCorner_list = a buffer to fill with polygon corners coordinates
     * @param Pos = Postion of the shape
     */
    virtual void CreateGraphicShape( std::vector <wxPoint>& aCorner_list,
                                        const wxPoint&         Pos );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool            Save( FILE* aFile ) const;

    /**
     * Load schematic hierarchical label entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic hierarchical label from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic hierarchical label.
     * @return True if the schematic hierarchical label loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_Rect        GetBoundingBox() const;

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void    Mirror_Y( int aYaxis_position );
    virtual void    Mirror_X( int aXaxis_position );
    virtual void    Rotate( wxPoint rotationPoint );
};

#endif /* CLASS_TEXT_LABEL_H */
