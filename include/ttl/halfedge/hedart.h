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

#ifndef _HALF_EDGE_DART_
#define _HALF_EDGE_DART_


#include <ttl/halfedge/hetriang.h>


namespace hed {


  //------------------------------------------------------------------------------------------------
  // Dart class for the half-edge data structure
  //------------------------------------------------------------------------------------------------

  /** \class Dart
  *   \brief \b %Dart class for the half-edge data structure.
  *
  *   See \ref api for a detailed description of how the member functions
  *   should be implemented.
  */

  class Dart {

    EdgePtr edge_;
    bool dir_; // true if dart is counterclockwise in face

  public:
    /// Default constructor
    Dart() { dir_ = true; }

    /// Constructor
    Dart(const EdgePtr& edge, bool dir = true) { edge_ = edge; dir_ = dir; }

    /// Copy constructor
    Dart(const Dart& dart) { edge_ = dart.edge_; dir_ = dart.dir_; }

    /// Destructor
    ~Dart() {}

    /// Assignment operator
    Dart& operator = (const Dart& dart) {
      if (this == &dart)
        return *this;
      edge_ = dart.edge_;
      dir_  = dart.dir_;
      return *this;
    }

    /// Comparing dart objects
    bool operator==(const Dart& dart) const {
      if (dart.edge_ == edge_ && dart.dir_ == dir_)
        return true;
      return false;
    }

    /// Comparing dart objects
    bool operator!=(const Dart& dart) const {
      return !(dart==*this);
    }

    /// Maps the dart to a different node
    Dart& alpha0() { dir_ = !dir_; return *this; }

    /// Maps the dart to a different edge
    Dart& alpha1() {
      if (dir_) {
        edge_ = edge_->getNextEdgeInFace()->getNextEdgeInFace();
        dir_ = false;
      }
      else {
        edge_ = edge_->getNextEdgeInFace();
        dir_ = true;
      }
      return *this;
    }

    /// Maps the dart to a different triangle. \b Note: the dart is not changed if it is at the boundary!
    Dart& alpha2() {
      if (edge_->getTwinEdge()) {
        edge_ = edge_->getTwinEdge();
        dir_ = !dir_;
      }
      // else, the dart is at the boundary and should not be changed
      return *this;
    }


    // Utilities not required by TTL
    // -----------------------------

    /** @name Utilities not required by TTL */
    //@{

    void init(const EdgePtr& edge, bool dir = true) { edge_ = edge; dir_ = dir; }

    double x() const { return getNode()->GetX(); } // x-coordinate of source node
    double y() const { return getNode()->GetY(); } // y-coordinate of source node

    bool isCounterClockWise() const { return dir_; }

    const NodePtr& getNode() const { return dir_ ? edge_->getSourceNode() : edge_->getTargetNode(); }
    const NodePtr& getOppositeNode() const { return dir_ ? edge_->getTargetNode() : edge_->getSourceNode(); }
    EdgePtr& getEdge() { return edge_; }

    //@} // End of Utilities not required by TTL

  };

}; // End of hed namespace

#endif
