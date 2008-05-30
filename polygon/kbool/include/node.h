/*! \file ../include/../node.h
    \brief Holds a GDSII node structure (Header)
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: node.h,v 1.1 2005/05/24 19:13:37 titato Exp $
*/

#ifndef NODE_H
#define NODE_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface
#endif

#include <math.h>
#include "../include/booleng.h"

#include "../include/lpoint.h"

#include "../include/link.h"
#include "../include/_lnk_itr.h" // LinkBaseIter

enum NodePosition { N_LEFT, N_ON, N_RIGHT};

class A2DKBOOLDLLEXP Node : public LPoint
{
	protected:
      Bool_Engine* _GC;
	public:
		// friend must be deleted in the final version!
		friend class Debug_driver;

		// constructors and destructors
		Node(Bool_Engine* GC);

		Node(const B_INT, const B_INT, Bool_Engine* GC);
	
		Node(LPoint* const a_point, Bool_Engine* GC);
		Node(Node * const, Bool_Engine* GC);
		Node& operator=(const Node &other_node);
		~Node();

		//public member functions
		void AddLink(KBoolLink*);
		DL_List<void*>* GetLinklist();

      //! check two link for its operation flags to be the same when coming from the prev link.
      bool SameSides( KBoolLink* const prev , KBoolLink* const link, BOOL_OP operation );

      //! get the link most right or left to the current link, but with the specific operation
      /*! flags the same on the sides of the new link.
      */
		KBoolLink*  GetMost( KBoolLink* const prev ,LinkStatus whatside, BOOL_OP operation );

      //! get link that is leading to a hole ( hole segment or linking segment )
      KBoolLink* GetMostHole( KBoolLink* const prev ,LinkStatus whatside, BOOL_OP operation );

      //! get link that is not vertical.
      KBoolLink* 	GetNotFlat();

      //! get a link to a hole or from a hole.
      KBoolLink* GetHoleLink( KBoolLink* const prev, bool checkbin, BOOL_OP operation );

		int Merge(Node*);
		void RemoveLink(KBoolLink*);
		bool Simplify(Node* First, Node* Second, B_INT Marge );

		//  memberfunctions for maximum performance
		void        RoundInt(B_INT grid);
		KBoolLink*	GetIncomingLink();

		int		  GetNumberOfLinks();
		KBoolLink* GetNextLink();
		KBoolLink* GetOtherLink(KBoolLink*);
		KBoolLink* GetOutgoingLink();
		KBoolLink* GetPrevLink();

		KBoolLink* Follow(KBoolLink* const prev );
		KBoolLink* GetBinHighest(bool binset);

	protected:
		DL_List<void*>*		_linklist;
};

#endif
