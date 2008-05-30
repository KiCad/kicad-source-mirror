/*! \file ../include/../line.h
    \brief Mainy used for calculating crossings
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: line.h,v 1.2 2005/06/12 00:03:11 kbluck Exp $
*/

#ifndef LINE_H
#define LINE_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface
#endif

#include "../include/booleng.h"
#include "../include/link.h"

class A2DKBOOLDLLEXP Bool_Engine;

// Status of a point to a line
enum PointStatus {LEFT_SIDE, RIGHT_SIDE, ON_AREA, IN_AREA};

class A2DKBOOLDLLEXP Graph;

class A2DKBOOLDLLEXP KBoolLine
{
   protected:
      Bool_Engine* m_GC;
	public:

		// constructors and destructor
		KBoolLine(Bool_Engine* GC);
		KBoolLine(KBoolLink*,Bool_Engine* GC);
		~KBoolLine();

		void			Set(KBoolLink *);
		KBoolLink*		GetLink();

      //! Get the beginnode from a line
		Node*		GetBeginNode();		
      
      //! Get the endnode from a line
		Node*		GetEndNode();   										  	

      //! Check if two lines intersects
		int					CheckIntersect(KBoolLine*, double Marge);	

      //! Intersects two lines
		int					Intersect(KBoolLine*, double Marge);    	 
		int               Intersect_simple(KBoolLine * lijn);
		bool				   Intersect2(Node* crossing,KBoolLine * lijn);

      //!For an infinite line
		PointStatus			PointOnLine(Node* a_node, double& Distance, double Marge ); 

      //!For a non-infinite line
		PointStatus			PointInLine(Node* a_node, double& Distance, double Marge ); 

      //! Caclulate Y if X is known
      B_INT					Calculate_Y(B_INT X); 								
		B_INT         		Calculate_Y_from_X(B_INT X);
		void              Virtual_Point( LPoint *a_point, double distance);

      //! assignment operator
		KBoolLine& 			operator=(const KBoolLine&); 					

		Node* 				OffsetContour(KBoolLine* const nextline,Node* last_ins, double factor,Graph *shape);
		Node* 				OffsetContour_rounded(KBoolLine* const nextline,Node* _last_ins, double factor,Graph *shape);
		bool 				   OkeForContour(KBoolLine* const nextline,double factor,Node* LastLeft,Node* LastRight, LinkStatus& _outproduct);
		bool				   Create_Ring_Shape(KBoolLine* nextline,Node** _last_ins_left,Node** _last_ins_right,double factor,Graph *shape);
		void 					Create_Begin_Shape(KBoolLine* nextline,Node** _last_ins_left,Node** _last_ins_right,double factor,Graph *shape);
		void 					Create_End_Shape(KBoolLine* nextline,Node* _last_ins_left,Node* _last_ins_right,double factor,Graph *shape);

      //! Calculate the parameters if nessecary
		void  CalculateLineParameters(); 								

      //! Adds a crossing between the intersecting lines
		void  AddLineCrossing(B_INT , B_INT , KBoolLine *); 		

		void  AddCrossing(Node *a_node);
		Node* AddCrossing(B_INT X, B_INT Y);
		bool  ProcessCrossings(TDLI<KBoolLink>* _LI);

// Linecrosslist
		void	SortLineCrossings();
		bool	CrossListEmpty();
		DL_List<void*>*		GetCrossList();
//		bool	HasInCrossList(Node*);

	private:

      //! Function needed for Intersect
		int   ActionOnTable1(PointStatus,PointStatus); 							
      //! Function needed for Intersect
		int   ActionOnTable2(PointStatus,PointStatus); 						

		double 		m_AA;
		double		m_BB;
		double		m_CC;
		KBoolLink*	m_link;
		bool		m_valid_parameters;

      //! List with crossings through this link
		DL_List<void*>	 *linecrosslist;
};

#endif
