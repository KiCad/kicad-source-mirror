/**********************************/
/* classes to handle copper zones */
/**********************************/

#ifndef CLASS_ZONE_H
#define CLASS_ZONE_H

/************************/
/* class ZONE_CONTAINER */
/************************/
/* handle a list of polygons delimiting a copper zone
 * a zone is described by a main polygon, a time stamp, a layer and a net name.
 * others polygons inside this main polygon are holes.
*/

class ZONE_CONTAINER : public BOARD_ITEM	// Not sure BOARD_ITEM is better than EDA_BaseStruct
{
public:
    wxString m_Netname;             /* Net Name */

private:
    int     m_NetCode;              // Net number for fast comparisons

public:
	ZONE_CONTAINER(BOARD * parent);
	~ZONE_CONTAINER();

    bool Save( FILE* aFile ) const;
	
};

/*******************/
/* class EDGE_ZONE */
/*******************/

class EDGE_ZONE : public DRAWSEGMENT
{
public:
    EDGE_ZONE( BOARD_ITEM* StructFather );
    EDGE_ZONE( const EDGE_ZONE& edgezone );
    ~EDGE_ZONE();

    EDGE_ZONE* Next() { return (EDGE_ZONE*) Pnext; }

    EDGE_ZONE* Back() { return (EDGE_ZONE*) Pback; }
    
    
    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */ 
    bool Save( FILE* aFile ) const;
};

#endif	// #ifndef CLASS_ZONE_H
