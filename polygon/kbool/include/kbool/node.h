/*! \file node.h
    \brief Holds a GDSII node structure (Header)
    \author Klaas Holwerda

    Copyright: 2001-2004 (C) Klaas Holwerda

    Licence: see kboollicense.txt

    RCS-ID: $Id: node.h,v 1.7 2009/09/14 16:50:12 titato Exp $
*/

#ifndef NODE_H
#define NODE_H

#include "kbool/booleng.h"

#include "kbool/lpoint.h"

#include "kbool/link.h"
#include "kbool/_lnk_itr.h" // LinkBaseIter
#include <math.h>

enum NodePosition { N_LEFT, N_ON, N_RIGHT};

class A2DKBOOLDLLEXP kbNode : public kbLPoint
{
protected:
    Bool_Engine* _GC;
public:
    // friend must be deleted in the final version!
    friend class Debug_driver;

    // constructors and destructors
    kbNode( Bool_Engine* GC );

    kbNode( const B_INT, const B_INT, Bool_Engine* GC );

    kbNode( kbLPoint* const a_point, Bool_Engine* GC );
    kbNode( kbNode * const, Bool_Engine* GC );
    kbNode& operator=( const kbNode &other_node );
    ~kbNode();

    //public member functions
    void AddLink( kbLink* );
    DL_List<void*>* GetLinklist();

    //! check two link for its operation flags to be the same when coming from the prev link.
    bool SameSides( kbLink* const prev , kbLink* const link, BOOL_OP operation );

    //! get the link most right or left to the current link, but with the specific operation
    /*! flags the same on the sides of the new link.
    */
    kbLink*  GetMost( kbLink* const prev , LinkStatus whatside, BOOL_OP operation );

    //! get link that is leading to a hole ( hole segment or linking segment )
    kbLink* GetMostHole( kbLink* const prev , LinkStatus whatside, BOOL_OP operation,
                         bool searchholelink = true );

    //! get link that is not vertical.
    kbLink*  GetNotFlat();

    //! get a link to a hole or from a hole.
    kbLink* GetHoleLink( kbLink* const prev, LinkStatus whatside,
                         bool checkbin, BOOL_OP operation );

    int Merge( kbNode* );
    void RemoveLink( kbLink* );
    bool Simplify( kbNode* First, kbNode* Second, B_INT Marge );

    //  memberfunctions for maximum performance
    void        RoundInt( B_INT grid );
    kbLink* GetIncomingLink();

    int    GetNumberOfLinks();
    kbLink* GetNextLink();
    kbLink* GetOtherLink( kbLink* );
    kbLink* GetOutgoingLink();
    kbLink* GetPrevLink();

    kbLink* Follow( kbLink* const prev );
    kbLink* GetBinHighest( bool binset );

protected:
    DL_List<void*>*  _linklist;
};

#endif
