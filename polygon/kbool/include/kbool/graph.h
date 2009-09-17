/*! \file include/graph.h
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: graph.h,v 1.5 2009/09/10 17:04:09 titato Exp $
*/

/* @@(#) $Source: /cvsroot/wxart2d/wxArt2D/thirdparty/kbool/include/kbool/graph.h,v $ $Revision: 1.5 $ $Date: 2009/09/10 17:04:09 $ */

/*
Program GRAPH.H
Purpose Used to Intercect and other process functions
Last Update 03-04-1996
*/

#ifndef GRAPH_H
#define GRAPH_H

#include "kbool/booleng.h"
#include "kbool/_lnk_itr.h"
#include "kbool/link.h"
#include "kbool/line.h"
#include "kbool/scanbeam.h"

class kbNode;

class kbGraphList;

//! one graph containing links that cab be connected.
class A2DKBOOLDLLEXP kbGraph
{
protected:
    Bool_Engine* _GC;
public:

    kbGraph( Bool_Engine* GC );
    kbGraph( kbLink*, Bool_Engine* GC );

    kbGraph( kbGraph* other );

    ~kbGraph();

    bool   GetBin()        { return _bin; };
    void   SetBin( bool b )      { _bin = b; };

    void    Prepare( int intersectionruns );
    void      RoundInt( B_INT grid );
    void    Rotate( bool plus90 );

    //! adds a link to the linklist
    void   AddLink( kbNode *begin, kbNode *end );

    //! adds a link to the linklist
    void   AddLink( kbLink *a_link );

    bool   AreZeroLines( B_INT Marge );

    //! Delete parallel lines
    void   DeleteDoubles();

    //! delete zerolines
    bool   DeleteZeroLines( B_INT Marge );
    bool    RemoveNullLinks();

    //! Process found intersections
    void   ProcessCrossings();
    //! set flags for operations based on group
    void      Set_Operation_Flags();

    //! Left Right values
    void   Remove_IN_Links();

    //! reset bin and mark flags in links.
    void ResetBinMark();

    // Remove unused links
    void   ReverseAllLinks();

    //! Simplify the kbGraph
    bool   Simplify( B_INT Marge );


    //! Takes over all links of the argument
    bool    Smoothen( B_INT Marge );

    void   TakeOver( kbGraph* );

    //! function for maximum performance

    //! Get the First link from the kbGraph
    kbLink* GetFirstLink();
    kbNode*   GetTopNode();
    void   SetBeenHere( bool );
    void    Reset_flags();

    //! Set the group of a kbGraph
    void   SetGroup( GroupType );

    //! Set the number of the kbGraph
    void   SetNumber( int );
    void    Reset_Mark_and_Bin();
    bool     GetBeenHere();
    int   GetGraphNum();
    int   GetNumberOfLinks();

    void        Boolean( BOOL_OP operation, kbGraphList* Result );
    void        Correction( kbGraphList* Result, double factor );
    void        MakeRing( kbGraphList* Result, double factor );
    void        CreateRing( kbGraphList *ring, double factor );
    void        CreateRing_fast( kbGraphList *ring, double factor );
    void        CreateArc( kbNode* center, kbLine* incoming, kbNode* end, double radius, double aber );
    void        CreateArc( kbNode* center, kbNode* begin, kbNode* end, double radius, bool clock, double aber );
    void        MakeOneDirection();
    void        Make_Rounded_Shape( kbLink* a_link, double factor );
    bool      MakeClockWise();
    bool      writegraph( bool linked );
    bool      writeintersections();
    void        WriteKEY( Bool_Engine* GC,  FILE* file = NULL );
    void        WriteGraphKEY( Bool_Engine* GC );

protected:

    //! Extracts partical polygons from the graph
    /*
       Links are sorted in XY at beginpoint. Bin and mark flag are reset.
       Next start to collect subparts from the graph, setting the links bin for all found parts.
       The parts are searched starting at a topleft corner NON set bin flag link.
       Found parts are numbered, to be easily split into to real sperate graphs by Split()

       \param operation operation to collect for.
       \param detecthole if you want holes detected, influences also way of extraction.
       \param foundholes when holes are found this flag is set true, but only if detecthole is set true.
    */
    void  Extract_Simples( BOOL_OP operation, bool detecthole, bool& foundholes );

    //! split graph into small graph, using the numbers in links.
    void  Split( kbGraphList* partlist );

    //! Collect a graph by starting at argument link
    /*
       Called from Extract_Simples, and assumes sorted links with bin flag unset for non extarted piece

       Collect graphs pieces from a total graph, by following links set to a given boolean operation.
       \param current_node start node to collect
       \param operation operation to collect for.
       \param detecthole if you want holes detected, influences also way of extraction.
       \param graphnumber number to be given to links in the extracted graph piece
       \param foundholes when holes are found this flag is set true.
    */
    void  CollectGraph( kbNode *current_node, BOOL_OP operation, bool detecthole, int graphnumber, bool& foundholes );

    void  CollectGraphLast( kbNode *current_node, BOOL_OP operation, bool detecthole, int graphnumber, bool& foundholes );

    //! find a link not bin in the top left corner ( links should be sorted already )
    /*!
       Last found position is used to find it quickly.
       Used in ExtractSimples()
    */
    kbNode* GetMostTopLeft( TDLI<kbLink>* _LI );

    //! calculates crossing for all links in a graph, and add those as part of the graph.
    /*
       It is not just crossings calculation, snapping close nodes is part of it.
       This is not done at maximum stability in economic time.
       There are faster ways, but hardly ever they solve the problems, and they do not snap.
       Here it is on purpose split into separate steps, to get a better result in snapping, and
       to reach a better stability.

       \param Marge nodes and lines closer to eachother then this, are merged.
    */
    bool CalculateCrossings( B_INT Marge );

    //! equal nodes in position are merged into one.
    int Merge_NodeToNode( B_INT Marge );

    //! basic scan algorithm with a sweeping beam are line.
    /*!
        \param scantype a different face in the algorithm.
        \param holes to detect hole when needed.
    */
    int ScanGraph2( SCANTYPE scantype, bool& holes );

    //! links not used for a certain operation can be deleted, simplifying extraction
    void  DeleteNonCond( BOOL_OP operation );

    //! links not used for a certain operation can be set bin, simplifying extraction
    void HandleNonCond( BOOL_OP operation );

    //! debug
    bool  checksort();

    //! used in correction/offset algorithm
    bool  Small( B_INT howsmall );


    bool _bin;

    DL_List<void*>* _linklist;

};

#endif


