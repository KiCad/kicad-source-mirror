// definition of ID structure used by FreePCB
//
#pragma once

// struct id : this structure is used to identify PCB design elements
// such as instances of parts or nets, and their subelements
// Each element will have its own id.
// An id is attached to each item of the Display List so that it can
// be linked back to the PCB design element which drew it.
// These are mainly used to identify items selected by clicking the mouse
//
// In general: 
//		id.type	= type of PCB element (e.g. part, net, text)
//		id.st	= subelement type (e.g. part pad, net connection)
//		id.i	= subelement index (zero-based)
//		id.sst	= subelement of subelement (e.g. net connection segment)
//		id.ii	= subsubelement index (zero-based)
//
// For example, the id for segment 0 of connection 4 of net 12 would be
//	id = { ID_NET, 12, ID_CONNECT, 4, ID_SEG, 0 };
//
//
class id {
public:
	// constructor
	id( int qt=0, int qst=0, int qis=0, int qsst=0, int qiis=0 ) 
	{ type=qt; st=qst; i=qis; sst=qsst; ii=qiis; } 
	// operators
	friend int operator ==(id id1, id id2)
	{ return (id1.type==id2.type 
			&& id1.st==id2.st 
			&& id1.sst==id2.sst 
			&& id1.i==id2.i 
			&& id1.ii==id2.ii ); 
	}
	// member functions
	void Clear() 
	{ type=0; st=0; i=0; sst=0; ii=0; } 
	void Set( int qt, int qst=0, int qis=0, int qsst=0, int qiis=0 ) 
	{ type=qt; st=qst; i=qis; sst=qsst; ii=qiis; } 
	// member variables
	unsigned int type;	// type of element
	unsigned int st;	// type of subelement
	unsigned int i;		// index of subelement
	unsigned int sst;	// type of subsubelement
	unsigned int ii;	// index of subsubelement
};


// these are constants used in ids
// root types
enum {
	ID_NONE = 0,	// an undefined type or st (or an error)
	ID_BOARD,		// board outline
	ID_PART,		// part
	ID_NET,			// net
	ID_TEXT,		// free-standing text
	ID_DRC,			// DRC error
	ID_SM_CUTOUT,	// cutout for solder mask
	ID_MULTI		// if multiple selections
};

// subtypes of ID_PART
enum {
	ID_PAD = 1,		// pad_stack in a part
	ID_SEL_PAD,		// selection rectangle for pad_stack in a part
	ID_OUTLINE,		// part outline
	ID_REF_TXT,		// text showing ref num for part
	ID_ORIG,		// part origin
	ID_SEL_RECT,	// selection rectangle for part
	ID_SEL_REF_TXT	// selection rectangle for ref text
};

// subtypes of ID_TEXT
enum {
	ID_SEL_TXT = 1,		// selection rectangle
	ID_STROKE			// stroke for text
};

// subtypes of ID_NET
enum {
	ID_ENTIRE_NET = 0,
	ID_CONNECT,		// connection
	ID_AREA			// copper area
};

// subtypes of ID_BOARD
enum {
	ID_BOARD_OUTLINE = 1,
};

// subsubtypes of ID_NET.ID_CONNECT
enum {
	ID_ENTIRE_CONNECT = 0,
	ID_SEG,
	ID_SEL_SEG,	
	ID_VERTEX,	
	ID_SEL_VERTEX,
	ID_VIA
};

// subsubtypes of ID_NET.ID_AREA, ID_BOARD.ID_BOARD_OUTLINE, ID_SM_CUTOUT
enum {
	ID_SIDE = 1,
	ID_SEL_SIDE,
	ID_SEL_CORNER,
	ID_HATCH,
	ID_PIN_X,	// only used by ID_AREA
	ID_STUB_X	// only used by ID_AREA
};

// subtypes of ID_DRC
// for subsubtypes, use types in DesignRules.h
enum {
	ID_DRE = 1,
	ID_SEL_DRE
};

