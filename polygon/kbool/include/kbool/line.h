/*! \file line.h
    \brief Mainy used for calculating crossings
    \author Klaas Holwerda

    Copyright: 2001-2004 (C) Klaas Holwerda

    Licence: see kboollicense.txt

    RCS-ID: $Id: line.h,v 1.5 2009/09/10 17:04:09 titato Exp $
*/

#ifndef LINE_H
#define LINE_H

#include "kbool/booleng.h"
#include "kbool/link.h"

class A2DKBOOLDLLEXP Bool_Engine;

// Status of a point to a line
enum PointStatus {LEFT_SIDE, RIGHT_SIDE, ON_AREA, IN_AREA};

class A2DKBOOLDLLEXP kbGraph;

class A2DKBOOLDLLEXP kbLine
{
protected:
    Bool_Engine* m_GC;
public:

    // constructors and destructor
    kbLine( Bool_Engine* GC );
    kbLine( kbLink*, Bool_Engine* GC );
    ~kbLine();

    void   Set( kbLink * );
    kbLink*  GetLink();

    //! Get the beginnode from a line
    kbNode*  GetBeginNode();

    //! Get the endnode from a line
    kbNode*  GetEndNode();

    //! Check if two lines intersects
    int     CheckIntersect( kbLine*, double Marge );

    //! Intersects two lines
    int     Intersect( kbLine*, double Marge );
    int               Intersect_simple( kbLine * lijn );
    bool       Intersect2( kbNode* crossing, kbLine * lijn );

    //!For an infinite line
    PointStatus   PointOnLine( kbNode* a_node, double& Distance, double Marge );

    //!For a non-infinite line
    PointStatus   PointInLine( kbNode* a_node, double& Distance, double Marge );

    //! Caclulate Y if X is known
    B_INT     Calculate_Y( B_INT X );
    B_INT           Calculate_Y_from_X( B_INT X );
    void              Virtual_Point( kbLPoint *a_point, double distance );

    //! assignment operator
    kbLine&    operator=( const kbLine& );

    kbNode*     OffsetContour( kbLine* const nextline, kbNode* last_ins, double factor, kbGraph *shape );
    kbNode*     OffsetContour_rounded( kbLine* const nextline, kbNode* _last_ins, double factor, kbGraph *shape );
    bool        OkeForContour( kbLine* const nextline, double factor, kbNode* LastLeft, kbNode* LastRight, LinkStatus& _outproduct );
    bool       Create_Ring_Shape( kbLine* nextline, kbNode** _last_ins_left, kbNode** _last_ins_right, double factor, kbGraph *shape );
    void      Create_Begin_Shape( kbLine* nextline, kbNode** _last_ins_left, kbNode** _last_ins_right, double factor, kbGraph *shape );
    void      Create_End_Shape( kbLine* nextline, kbNode* _last_ins_left, kbNode* _last_ins_right, double factor, kbGraph *shape );

    //! Calculate the parameters if nessecary
    void  CalculateLineParameters();

    //! Adds a crossing between the intersecting lines
    void  AddLineCrossing( B_INT , B_INT , kbLine * );

    void  AddCrossing( kbNode *a_node );
    kbNode* AddCrossing( B_INT X, B_INT Y );
    bool  ProcessCrossings( TDLI<kbLink>* _LI );

// Linecrosslist
    void SortLineCrossings();
    bool CrossListEmpty();
    DL_List<void*>*  GetCrossList();
//  bool HasInCrossList(kbNode*);

private:

    //! Function needed for Intersect
    int   ActionOnTable1( PointStatus, PointStatus );
    //! Function needed for Intersect
    int   ActionOnTable2( PointStatus, PointStatus );

    double  m_AA;
    double  m_BB;
    double  m_CC;
    kbLink* m_link;
    bool  m_valid_parameters;

    //! List with crossings through this link
    DL_List<void*>  *linecrosslist;
};

#endif
