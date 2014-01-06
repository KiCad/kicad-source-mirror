/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 *
 * Contact information: E-mail: tor.dokken@sintef.no                      
 * SINTEF ICT, Department of Applied Mathematics,                         
 * P.O. Box 124 Blindern,                                                 
 * 0314 Oslo, Norway.                                                     
 *
 * This file is part of TTL.
 *
 * TTL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version. 
 *
 * TTL is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with TTL. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In accordance with Section 7(b) of the GNU Affero General Public
 * License, a covered work must retain the producer line in every data
 * file that is created or manipulated using TTL.
 *
 * Other Usage
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial activities involving the TTL library without
 * disclosing the source code of your own applications.
 *
 * This file may be used in accordance with the terms contained in a
 * written agreement between you and SINTEF ICT. 
 */

#ifndef _TTL_CONSTR_H_
#define _TTL_CONSTR_H_


#include <list>
#include <cmath>


// Debugging
#ifdef DEBUG_TTL_CONSTR_PLOT
  #include <fstream>
  static ofstream ofile_constr("qweCons.dat");
#endif


//using namespace std;

/** \brief Constrained Delaunay triangulation
*
*   Basic generic algorithms in TTL for inserting a constrained edge between two existing nodes.\n
*
*   See documentation for the namespace ttl for general requirements and assumptions.
*
*   \author 
*   Øyvind Hjelle, oyvindhj@ifi.uio.no
*/

namespace ttl_constr {

  //  ??? A constant used to evluate a numerical expression against a user spesified
  //  roundoff-zero number
#ifdef DEBUG_TTL_CONSTR
  static const double ROUNDOFFZERO = 0.0; // 0.1e-15;
#endif


  //------------------------------------------------------------------------------------------------
  /* Checks if \e dart has start and end points in \e dstart and \e dend.
  *
  *   \param dart 
  *   The dart that should be controlled to see if it's the constraint
  *
  *   \param dstart 
  *   A CCW dart with the startnode of the constraint as the startnode
  *
  *   \param dend 
  *   A CCW dart with the endnode of the constraint as the startnode
  *
  *   \retval bool 
  *   A bool confirming that it's the constraint or not 
  *
  *   \using
  *   ttl::same_0_orbit
  */
  template <class DartType>
    bool isTheConstraint(const DartType& dart, const DartType& dstart, const DartType& dend) {
    DartType d0 = dart;
    d0.alpha0(); // CW
    if ((ttl::same_0_orbit(dstart, dart) && ttl::same_0_orbit(dend, d0)) ||
      (ttl::same_0_orbit(dstart, d0)  && ttl::same_0_orbit(dend, dart))) {
      return true;
    }
    return false;
  }


  //------------------------------------------------------------------------------------------------
  /* Checks if \e d1 and \e d2 are on the same side of the line between \e dstart and \e dend.
  *   (The start nodes of \e d1 and \e d2 represent an edge).
  *
  *   \param dstart
  *   A CCW dart with the start node of the constraint as the source node of the dart.
  *
  *   \param dend
  *    A CCW dart with the end node of the constraint as the source node of the dart.
  * 
  *   \param d1
  *    A CCW dart with the first node as the start node of the dart.
  * 
  *   \param d2
  *   A CCW dart with the other node as the start node of the dart.
  *
  *   \using
  *   TraitsType::orient2d
  */
  template <class TraitsType, class DartType>
    bool crossesConstraint(DartType& dstart, DartType& dend, DartType& d1, DartType& d2) {
    
    typename TraitsType::real_type orient_1 = TraitsType::orient2d(dstart,d1,dend);
    typename TraitsType::real_type orient_2 = TraitsType::orient2d(dstart,d2,dend);
    // ??? Should we refine this? e.g. find if (dstart,dend) (d1,d2) represent the same edge
    if ((orient_1 <= 0 && orient_2 <= 0) || (orient_1 >= 0 && orient_2 >= 0))
      return false;
    
    return true;
  }


  //------------------------------------------------------------------------------------------------
  /* Return the dart \e d making the smallest non-negative angle,
  *   as calculated with: orient2d(dstart, d.alpha0(), dend),
  *   at the 0-orbit of dstart.
  *   If (dstart,dend) is a CCW boundary edge \e d will be CW, otherwise CCW (since CCW in)
  *   at the 0-orbit of dstart.
  * 
  *   \par Assumes:
  *   - CCW dstart and dend, but returned dart can be CW at the boundary.
  *   - Boundary is convex?
  *
  *   \param dstart
  *   A CCW dart dstart
  *
  *   \param dend 
  *   A CCW dart dend
  *
  *   \retval DartType
  *   The dart \e d making the smallest positive (or == 0) angle
  *
  *   \using
  *   ttl::isBoundaryNode
  *   ttl::positionAtNextBoundaryEdge
  *   TraitsType::orient2d
  */
  template <class TraitsType, class DartType>
    DartType getAtSmallestAngle(const DartType& dstart, const DartType& dend) {
    
    // - Must boundary be convex???
    // - Handle the case where the constraint is already present???
    // - Handle dstart and/or dend at the boundary
    //   (dstart and dend may define a boundary edge)
    
    DartType d_iter = dstart;
    if (ttl::isBoundaryNode(d_iter)) {
      d_iter.alpha1(); // CW
      ttl::positionAtNextBoundaryEdge(d_iter); // CCW (was rotated CW to the boundary)
    }
    
    // assume convex boundary; see comments
    
    DartType d0 = d_iter;
    d0.alpha0();
    bool ccw = true; // the rotation later
    typename TraitsType::real_type o_iter = TraitsType::orient2d(d_iter, d0, dend);
    if (o_iter == 0) { // collinear BUT can be on "back side"
      d0.alpha1().alpha0(); // CW
      if (TraitsType::orient2d(dstart, dend, d0) > 0)
        return d_iter; //(=dstart) collinear
      else {
        // collinear on "back side"
        d_iter.alpha1().alpha2(); // assume convex boundary
        ccw = true;
      }
    }
    else if (o_iter < 0) {
      // Prepare for rotating CW and with d_iter CW
      d_iter.alpha1();
      ccw = false;
    }
    
    // Set first angle
    d0 = d_iter; d0.alpha0();
    o_iter = TraitsType::orient2d(dstart, d0, dend);
    
    typename TraitsType::real_type o_next;
    
    // Rotate towards the constraint CCW or CW.
    // Here we assume that the boundary is convex.
    DartType d_next = d_iter;
    for (;;) {
      d_next.alpha1(); // CW !!! (if ccw == true)
      d0 = d_next; d0.alpha0();
      o_next = TraitsType::orient2d(dstart, d0, dend);
      
      if (ccw && o_next < 0) // and o_iter > 0
        return d_iter;
      else if (!ccw && o_next > 0)
        return d_next; // CCW
      else if (o_next == 0) {
        if (ccw)
          return d_next.alpha2(); // also ok if boundary
        else
          return d_next;
      }
      
      // prepare next
      d_next.alpha2(); // CCW if ccw
      d_iter = d_next; // also ok if boundary CCW if ccw == true
    }
  }


  //------------------------------------------------------------------------------------------------
  /* This function finds all the edges in the triangulation crossing
  *   the spesified constraint and puts them in a list.
  *   In the case of collinearity, an attempt is made to detect this.
  *   The first collinear node between dstart and dend is then returned.
  *
  *   Strategy:
  *   - Iterate such that \e d_iter is always strictly "below" the constraint
  *     as seen with \e dstart to the left and \e dend to the right.
  *   - Add CCW darts, whose edges intersect the constrait, to a list.
  *     These edges are found by the orient2d predicate:
  *     If two nodes of an edge are on opposite sides of the constraint,
  *     the edge between them intersect.
  *   - Must handle collinnear cases, i.e., if a node falls on the constraint,
  *     and possibly restarting collection of edges. Detecting collinearity
  *     heavily relies on the orient2d predicate which is provided by the
  *     traits class.
  *
  *   Action:
  *   1) Find cone/opening angle containing \e dstart and \e dend
  *   2) Find first edge from the first 0-orbit that intersects
  *   3) Check which of the two opposite that intersects
  *
  *   1)  
  *   Rotate CCW and find the (only) case where \e d_iter and \e d_next satisfy:
  *   - orient2d(d_iter, d_iter.alpha0(), dend) > 0
  *   - orient2d(d_next, d_next.alpha0(), dend) < 0
  *      
  *   - check if we are done, i.e., if (d_next.alpha0() == my_dend)
  *   - Note also the situation if, e.g., the constraint is a boundary edge in which case
  *     \e my_dend wil be CW
  *
  *   \param dstart 
  *   A CCW dart with the startnode of the constraint as the startnode
  *
  *   \param dend
  *   A CCW dart with the endnode of the constraint as the startnode
  *
  *   \param elist
  *   A list where all the edges crossing the spesified constraint will be put
  * 
  *   \retval dartType 
  *   Returns the next "collinear" starting node such that dend is returned when done.
  */
  template <class TraitsType, class DartType, class ListType>
    DartType findCrossingEdges(const DartType& dstart, const DartType& dend, ListType& elist) {
    
    const DartType my_start = getAtSmallestAngle<TraitsType>(dstart, dend);
    DartType my_end   = getAtSmallestAngle<TraitsType>(dend, dstart);
    
    DartType d_iter = my_start;
    if (d_iter.alpha0().alpha2() == my_end)
      return d_iter; // The constraint is an existing edge and we are done
    
    // Facts/status so far:
    // - my_start and my_end are now both CCW and the constraint is not a boundary edge.
    // - Further, the constraint is not one single existing edge, but it might be a collection
    //   of collinear edges in which case we return the current collinear edge
    //   and calling this function until all are collected.
    
    my_end.alpha1(); // CW! // ??? this is probably ok for testing now?
    
    d_iter = my_start;
    d_iter.alpha0().alpha1(); // alpha0 is downwards or along the constraint
    
    // Facts:
    // - d_iter is guaranteed to intersect, but can be in start point.
    // - d_iter.alpha0() is not at dend yet
    typename TraitsType::real_type orient = TraitsType::orient2d(dstart, d_iter, dend);

    // Use round-off error/tolerance or rely on the orient2d predicate ???
    // Make a warning message if orient != exact 0
    if (orient == 0)
      return d_iter;

#ifdef DEBUG_TTL_CONSTR
    else if (fabs(orient) <= ROUNDOFFZERO) {
      cout << "The darts are not exactly colinear, but |d1 x d2| <= " << ROUNDOFFZERO << endl;
      return d_iter; // collinear, not done (and not collect in the list)
    }
#endif

    // Collect intersecting edges
    // --------------------------
    elist.push_back(d_iter); // The first with interior intersection point
    
    // Facts, status so far:
    // - The first intersecting edge is now collected
    // (- d_iter.alpha0() is still not at dend)
    
    // d_iter should always be the edge that intersects and be below or on the constraint
    // One of the two edges opposite to d_iter must intersect, or we have collinearity
    
    // Note: Almost collinear cases can be handled on the
    // application side with orient2d. We should probably
    // return an int and the application will set it to zero
    for(;;) {
      // assume orient have been calc. and collinearity has been tested,
      // above the first time and below later
      d_iter.alpha2().alpha1(); // 2a same node
      
      DartType d0 = d_iter;
      d0.alpha0(); // CW
      if (d0 == my_end)
        return dend; // WE ARE DONE (but can we enter an endless loop???)
      
      // d_iter or d_iter.alpha0().alpha1() must intersect
      orient = TraitsType::orient2d(dstart, d0, dend);
      
      if (orient == 0) 
        return d0.alpha1();

#ifdef DEBUG_TTL_CONSTR
      else if (fabs(orient) <= ROUNDOFFZERO) {
        return d0.alpha1(); // CCW, collinear
      }
#endif

      else if (orient > 0) { // orient > 0 and still below
        // This one must intersect!
        d_iter = d0.alpha1();
      }
      elist.push_back(d_iter);
    }
  }


  //------------------------------------------------------------------------------------------------
  /* This function recives a constrained edge and a list of all the edges crossing a constraint.
  *   It then swaps the crossing edges away from the constraint. This is done according to a
  *   scheme suggested by Dyn, Goren & Rippa (slightly modified).
  *   The resulting triangulation is a constrained one, but not necessarily constrained Delaunay.
  *   In other to run optimization later to obtain a constrained Delaunay triangulation,
  *   the swapped edges are maintained in a list.
  *
  *   Strategy :
  *   - Situation A: Run through the list and swap crossing edges away from the constraint.
  *     All the swapped edges are moved to the end of the list, and are "invisible" to this procedure.
  *   - Situation B: We may come in a situation where none of the crossing edges can be swapped away
  *     from the constraint.
  *     Then we follow the strategy of Dyn, Goren & Rippa and allow edges to be swapped,
  *     even if they are not swapped away from the constraint.
  *     These edges are NOT moved to the end of the list. They are later swapped to none-crossing
  *     edges when the locked situation is solved.
  *   - We keep on swapping edges in Situation B until we have iterated trough the list.
  *     We then resume Situation A.
  *   - This is done until the list is virtualy empty. The resulting \c elist has the constraint
  *     as the last element.
  *
  *   \param dstart 
  *   A CCW dart dstart
  *
  *   \param dend
  *   A CCW dart dend
  *
  *   \param elist
  *   A list containing all the edges crossing the spesified constraint
  *
  *   \using
  *   ttl::swappableEdge
  *   ttl::swapEdgeInList
  *   ttl::crossesConstraint
  *   ttl::isTheConstraint
  */
  template <class TraitsType, class DartType>
    void transformToConstraint(DartType& dstart, DartType& dend, std::list<DartType>& elist) {
    
    typename list<DartType>::iterator it, used;
    
    // We may enter in a situation where dstart and dend are altered because of a swap.
    // (The general rule is that darts inside the actual quadrilateral can be changed,
    // but not those outside.)
    // So, we need some look-ahead strategies for dstart and dend and change these
    // after a swap if necessary.
    
    int dartsInList = (int)elist.size();
    if (dartsInList == 0)
      return;

    bool erase; // indicates if an edge is swapped away from the constraint such that it can be
                // moved to the back of the list
    bool locked = false;
    do {
      int noswap = 0;
      it = elist.begin();

      // counts how many edges that have been swapped per list-cycle
      int counter = 1;
      while(it != elist.end()) { // ??? change this test with counter > dartsInList
        erase = false;
        // Check if our virtual end of the list has been crossed. It breaks the
        // while and starts all over again in the do-while loop
        if (counter > dartsInList)
          break;
        
        if (ttl::swappableEdge<TraitsType, DartType>(*it, true)) {
          // Dyn & Goren & Rippa 's notation:
          // The node assosiated with dart *it is denoted u_m. u_m has edges crossing the constraint
          // named w_1, ... , w_r . The other node to the edge assosiated with dart *it is w_s.
          // We want to swap from edge u_m<->w_s to edge w_{s-1}<->w_{s+1}.
          DartType op1 = *it;
          DartType op2 = op1;
          op1.alpha1().alpha0(); //finds dart with node w_{s-1}
          op2.alpha2().alpha1().alpha0(); // (CW) finds dart with node w_{s+1}
          DartType tmp = *it; tmp.alpha0(); // Dart with assosiated node opposite to node of *it allong edge
          // If there is a locked situation we swap, even if the result is crossing the constraint
          // If there is a looked situation, but we do an ordinary swap, it should be treated as
          // if we were not in a locked situation!!

          // The flag swap_away indicates if the edge is swapped away from the constraint such that
          // it does not cross the constraint.
          bool swap_away = (crossesConstraint<TraitsType>(dstart, dend, *it, tmp) &&
                            !crossesConstraint<TraitsType>(dstart, dend, op1, op2));
          if (swap_away || locked) {
            // Do a look-ahead to see if dstart and/or dend are in the quadrilateral
            // If so, we mark it with a flag to make sure we update them after the swap
            // (they may have been changed during the swap according to the general rule!)
            bool start = false;
            bool end = false;
            
            DartType d = *it;
            if (d.alpha1().alpha0() == dstart)
              start = true;
            d = *it;
            if (d.alpha2().alpha1().alpha0().alpha1() == dend)
              end = true;
            
            // This is the only place swapping is called when inserting a constraint
            ttl::swapEdgeInList<TraitsType, DartType>(it,elist);
            
            // If we, during look-ahead, found that dstart and/or dend were in the quadrilateral,
            // we update them.
            if (end)
              dend = *it;
            if (start) {
              dstart = *it;
              dstart.alpha0().alpha2();
            }
            
            if (swap_away) { // !locked || //it should be sufficient with swap_away ???
              noswap++;
              erase = true;
            }
            
            if (isTheConstraint(*it, dstart, dend)) {
              // Move the constraint to the end of the list
              DartType the_constraint = *it;
              elist.erase(it);
              elist.push_back(the_constraint);
              return;
            } //endif
          } //endif
        } //endif "swappable edge"
        
                
        // Move the edge to the end of the list if it was swapped away from the constraint
        if (erase) {
          used = it;
          elist.push_back(*it);
          ++it;
          elist.erase(used);
          --dartsInList;
        } 
        else {
          ++it;
          ++counter;
        }
        
      } //end while
      
      if (noswap == 0)
        locked = true;
      
    } while (dartsInList != 0);
    
    
#ifdef DEBUG_TTL_CONSTR
    // We will never enter here. (If elist is empty, we return above).
    cout << "??????? ERROR 2, should never enter here ????????????????????????? SKIP ???? " << endl;
    exit(-1);
#endif

  }

}; // End of ttl_constr namespace scope


namespace ttl { // (extension)
  
  /** @name Constrained (Delaunay) Triangulation */
  //@{

  //------------------------------------------------------------------------------------------------
  /** Inserts a constrained edge between two existing nodes in a triangulation.
  *   If the constraint falls on one or more existing nodes and this is detected by the
  *   predicate \c TraitsType::orient2d, which should return zero in this case, the
  *   constraint is split. Otherwise a degenerate triangle will be made along
  *   the constraint. 
  *
  *   \param dstart 
  *   A CCW dart with the start node of the constraint as the source node
  *
  *   \param dend 
  *   A CCW dart with the end node of the constraint as the source node
  *
  *   \param optimize_delaunay
  *   If set to \c true, the resulting triangulation will be
  *   a \e constrained \e Delaunay \e triangulation. If set to \c false, the resulting
  *   triangulation will not necessarily be of constrained Delaunay type.
  *
  *   \retval DartType 
  *   A dart representing the constrained edge.
  *
  *   \require
  *   - \ref hed::TTLtraits::orient2d "TraitsType::orient2d" (DartType&, DartType&, PointType&)
  *   - \ref hed::TTLtraits::swapEdge "TraitsType::swapEdge" (DartType&)
  *
  *   \using
  *   - ttl::optimizeDelaunay if \e optimize_delaunay is set to \c true
  *
  *   \par Assumes:
  *   - The constrained edge must be inside the existing triangulation (and it cannot
  *     cross the boundary of the triangulation).
  */
  template <class TraitsType, class DartType>
    DartType insertConstraint(DartType& dstart, DartType& dend, bool optimize_delaunay) {
    
    // Assumes:
    // - It is the users responsibility to avoid crossing constraints
    // - The constraint cannot cross the boundary, i.e., the boundary must be
    //   convex in the area of crossing edges.
    // - dtart and dend are preserved (same node associated.)
    

    // Find edges crossing the constraint and put them in elist.
    // If findCrossingEdges reaches a Node lying on the constraint, this function 
    // calls itself recursively. 

    // RECURSION
    list<DartType> elist;
    DartType next_start = ttl_constr::findCrossingEdges<TraitsType>(dstart, dend, elist);

    // If there are no crossing edges (elist is empty), we assume that the constraint
    // is an existing edge.
    // In this case, findCrossingEdges returns the constraint.
    // Put the constraint in the list to fit with the procedures below
    // (elist can also be empty in the case of invalid input data (the constraint is in
    // a non-convex area) but this is the users responsibility.)

    //by Thomas Sevaldrud   if (elist.size() == 0)
    //by Thomas Sevaldrud   elist.push_back(next_start);

    // findCrossingEdges stops if it finds a node lying on the constraint.
    // A dart with this node as start node is returned
    // We call insertConstraint recursivly until the received dart is dend
    if (!ttl::same_0_orbit(next_start, dend)) {

#ifdef DEBUG_TTL_CONSTR_PLOT
      cout << "RECURSION due to collinearity along constraint" << endl;
#endif

      insertConstraint<TraitsType,DartType>(next_start, dend, optimize_delaunay);
    }
    
    // Swap edges such that the constraint edge is present in the transformed triangulation.
    if (elist.size() > 0) // by Thomas Sevaldrud
       ttl_constr::transformToConstraint<TraitsType>(dstart, next_start, elist);
    
#ifdef DEBUG_TTL_CONSTR_PLOT
    cout << "size of elist = " << elist.size() << endl;
    if (elist.size() > 0) {
      DartType the_constraint = elist.back();
      ofile_constr << the_constraint.x() << " " << the_constraint.y() << " " << 0 << endl;
      the_constraint.alpha0();
      ofile_constr << the_constraint.x() << " " << the_constraint.y() << " " << 0 << endl << endl;
    }
#endif

    // Optimize to constrained Delaunay triangulation if required.
    typename list<DartType>::iterator end_opt = elist.end();
    if (optimize_delaunay) {

      // Indicate that the constrained edge, which is the last element in the list,
      // should not be swapped
      --end_opt;
      ttl::optimizeDelaunay<TraitsType, DartType>(elist, end_opt);
    }

    if(elist.size() == 0) // by Thomas Sevaldrud
       return next_start; // by Thomas Sevaldrud
     
    // Return the constraint, which is still the last element in the list
    end_opt = elist.end();
    --end_opt;
    return *end_opt;
  }

  //@} // End of Constrained Triangulation Group

}; // End of ttl namespace scope (extension)

#endif // _TTL_CONSTR_H_
