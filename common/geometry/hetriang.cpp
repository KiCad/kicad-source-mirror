/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <ttl/halfedge/hetriang.h>
#include <ttl/halfedge/hetraits.h>
#include <ttl/ttl.h>
#include <algorithm>
#include <fstream>
#include <limits>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>


using namespace hed;
using namespace std;


#ifdef TTL_USE_NODE_ID
  int Node::id_count = 0;
#endif


//#define DEBUG_HE
#ifdef DEBUG_HE
#include <iostream>
  static void errorAndExit(char* message) {
    cout << "\n!!! ERROR: "<< message << " !!!\n" << endl; exit(-1);
  }
#endif


//--------------------------------------------------------------------------------------------------
static EdgePtr getLeadingEdgeInTriangle(const EdgePtr& e) {
  EdgePtr edge = e;
  
  // Code: 3EF (assumes triangle)
  if (!edge->isLeadingEdge()) {
    edge = edge->getNextEdgeInFace();
    if (!edge->isLeadingEdge())
      edge = edge->getNextEdgeInFace();
  }
  
  if (!edge->isLeadingEdge()) {
    return EdgePtr();
  }
  
  return edge;
}


//--------------------------------------------------------------------------------------------------
static void getLimits(NodesContainer::iterator first,
                      NodesContainer::iterator last,
                      int& xmin, int& ymin,
                      int& xmax, int& ymax) {
  
  xmin = ymin = std::numeric_limits<int>::min();
  xmax = ymax = std::numeric_limits<int>::max();
  
  NodesContainer::iterator it;
  for (it = first; it != last; ++it) {
    xmin = min(xmin, (*it)->GetX());
    ymin = min(ymin, (*it)->GetY());
    xmax = max(xmax, (*it)->GetX());
    ymax = max(ymax, (*it)->GetY());
  }
}


//--------------------------------------------------------------------------------------------------
EdgePtr Triangulation::initTwoEnclosingTriangles(NodesContainer::iterator first,
                                                 NodesContainer::iterator last) {
   
  int xmin, ymin, xmax, ymax;
  getLimits(first, last, xmin, ymin, xmax, ymax);
  
  // Add 10% of range:  
  double fac = 10.0;
  double dx  = (xmax-xmin)/fac;
  double dy  = (ymax-ymin)/fac;
  
  NodePtr n1 = boost::make_shared<Node>(xmin-dx, ymin-dy);
  NodePtr n2 = boost::make_shared<Node>(xmax+dx, ymin-dy);
  NodePtr n3 = boost::make_shared<Node>(xmax+dx, ymax+dy);
  NodePtr n4 = boost::make_shared<Node>(xmin-dx, ymax+dy);
  
  // diagonal
  EdgePtr e1d = boost::make_shared<Edge>();
  EdgePtr e2d = boost::make_shared<Edge>();
  
  // lower triangle
  EdgePtr e11 = boost::make_shared<Edge>();
  EdgePtr e12 = boost::make_shared<Edge>();
  
  // upper triangle
  EdgePtr e21 = boost::make_shared<Edge>();
  EdgePtr e22 = boost::make_shared<Edge>();
  
  // lower triangle
  e1d->setSourceNode(n3);
  e1d->setNextEdgeInFace(e11);
  e1d->setTwinEdge(e2d);
  addLeadingEdge(e1d);
  
  e11->setSourceNode(n1);
  e11->setNextEdgeInFace(e12);
  
  e12->setSourceNode(n2);
  e12->setNextEdgeInFace(e1d);
  
  // upper triangle
  e2d->setSourceNode(n1);
  e2d->setNextEdgeInFace(e21);
  e2d->setTwinEdge(e1d);
  addLeadingEdge(e2d);
  
  e21->setSourceNode(n3);
  e21->setNextEdgeInFace(e22);
  
  e22->setSourceNode(n4);
  e22->setNextEdgeInFace(e2d);
  
  return e11;
}


//--------------------------------------------------------------------------------------------------
Triangulation::Triangulation() {
  helper = new ttl::TriangulationHelper( *this );
}


//--------------------------------------------------------------------------------------------------
Triangulation::Triangulation(const Triangulation& tr) {
  std::cout << "Triangulation: Copy constructor not present - EXIT.";
  exit(-1);
}


//--------------------------------------------------------------------------------------------------
Triangulation::~Triangulation() {
  cleanAll();
  delete helper;
}


//--------------------------------------------------------------------------------------------------
void Triangulation::createDelaunay(NodesContainer::iterator first,
                                   NodesContainer::iterator last) {

  cleanAll();
  
  EdgePtr bedge = initTwoEnclosingTriangles(first, last);
  Dart dc(bedge);
  
  Dart d_iter = dc;
  
  NodesContainer::iterator it;
  for (it = first; it != last; ++it) {
      helper->insertNode<TTLtraits>(d_iter, *it);
  }

  // In general (e.g. for the triangle based data structure), the initial dart
  // may have been changed.
  // It is the users responsibility to get a valid boundary dart here.
  // The half-edge data structure preserves the initial dart.
  // (A dart at the boundary can also be found by trying to locate a
  // triangle "outside" the triangulation.)

  // Assumes rectangular domain
  helper->removeRectangularBoundary<TTLtraits>(dc);
}


//--------------------------------------------------------------------------------------------------
void Triangulation::removeTriangle(EdgePtr& edge) {
  
  EdgePtr e1 = getLeadingEdgeInTriangle(edge);

#ifdef DEBUG_HE
  if (!e1)
    errorAndExit("Triangulation::removeTriangle: could not find leading edge");
#endif
  
  removeLeadingEdgeFromList(e1);
  // cout << "No leading edges = " << leadingEdges_.size() << endl;  
  // Remove the triangle
  EdgePtr e2(e1->getNextEdgeInFace());
  EdgePtr e3(e2->getNextEdgeInFace());

  e1->clear();
  e2->clear();
  e3->clear();
}


//--------------------------------------------------------------------------------------------------
void Triangulation::reverse_splitTriangle(EdgePtr& edge) {
  
  // Reverse operation of splitTriangle
  
  EdgePtr e1(edge->getNextEdgeInFace());
  EdgePtr le(getLeadingEdgeInTriangle(e1));
#ifdef DEBUG_HE
  if (!le)
    errorAndExit("Triangulation::removeTriangle: could not find leading edge");
#endif
  removeLeadingEdgeFromList(le);
  
  EdgePtr e2(e1->getNextEdgeInFace()->getTwinEdge()->getNextEdgeInFace());
  le = getLeadingEdgeInTriangle(e2);
#ifdef DEBUG_HE
  if (!le)
    errorAndExit("Triangulation::removeTriangle: could not find leading edge");
#endif
  removeLeadingEdgeFromList(le);
    
  EdgePtr e3(edge->getTwinEdge()->getNextEdgeInFace()->getNextEdgeInFace());
  le = getLeadingEdgeInTriangle(e3);
#ifdef DEBUG_HE
  if (!le)
    errorAndExit("Triangulation::removeTriangle: could not find leading edge");
#endif
  removeLeadingEdgeFromList(le);
  
  // The three triangles at the node have now been removed
  // from the triangulation, but the arcs have not been deleted.
  // Next delete the 6 half edges radiating from the node
  // The node is maintained by handle and need not be deleted explicitly
  EdgePtr estar = edge;
  EdgePtr enext = estar->getTwinEdge()->getNextEdgeInFace();
  estar->getTwinEdge()->clear();
  estar->clear();

  estar = enext;
  enext = estar->getTwinEdge()->getNextEdgeInFace();
  estar->getTwinEdge()->clear();
  estar->clear();

  enext->getTwinEdge()->clear();
  enext->clear();


  // Create the new triangle
  e1->setNextEdgeInFace(e2);
  e2->setNextEdgeInFace(e3);
  e3->setNextEdgeInFace(e1);
  addLeadingEdge(e1);
}


//--------------------------------------------------------------------------------------------------
Dart Triangulation::createDart() { 
  
  // Return an arbitrary CCW dart
  return Dart(*leadingEdges_.begin());
}


//--------------------------------------------------------------------------------------------------
bool Triangulation::removeLeadingEdgeFromList(EdgePtr& leadingEdge) {
  
  // Remove the edge from the list of leading edges,
  // but don't delete it.
  // Also set flag for leading edge to false.
  // Must search from start of list. Since edges are added to the
  // start of the list during triangulation, this operation will
  // normally be fast (when used in the triangulation algorithm)
  list<EdgePtr>::iterator it;
  for (it = leadingEdges_.begin(); it != leadingEdges_.end(); ++it) {
    
    EdgePtr edge = *it;
    if (edge == leadingEdge) {
      
      edge->setAsLeadingEdge(false);
      it = leadingEdges_.erase(it);
      
      return true;
    }
  }
  
  return false;
}


//--------------------------------------------------------------------------------------------------
void Triangulation::cleanAll() {
  BOOST_FOREACH(EdgePtr& edge, leadingEdges_)
      edge->setNextEdgeInFace(EdgePtr());
}


//--------------------------------------------------------------------------------------------------
void Triangulation::swapEdge(Dart& dart) {
  swapEdge(dart.getEdge());
}


//--------------------------------------------------------------------------------------------------
void Triangulation::splitTriangle(Dart& dart, const NodePtr& point) {
  EdgePtr edge = splitTriangle(dart.getEdge(), point);
  dart.init(edge);
}


//--------------------------------------------------------------------------------------------------
void Triangulation::reverse_splitTriangle(Dart& dart) {
  reverse_splitTriangle(dart.getEdge());
}


//--------------------------------------------------------------------------------------------------
void Triangulation::removeBoundaryTriangle(Dart& d) {
  removeTriangle(d.getEdge());
}


#ifdef TTL_USE_NODE_FLAG
//--------------------------------------------------------------------------------------------------
// This is a "template" for accessing all nodes (but multiple tests)
void Triangulation::flagNodes(bool flag) const {
  
  list<EdgePtr>::const_iterator it;
  for (it = leadingEdges_.begin(); it != leadingEdges_.end(); ++it) {
    EdgePtr edge = *it;
    
    for (int i = 0; i < 3; ++i) {
      edge->getSourceNode()->SetFlag(flag);
      edge = edge->getNextEdgeInFace();
    }
  }
}


//--------------------------------------------------------------------------------------------------
list<NodePtr>* Triangulation::getNodes() const {
  
  flagNodes(false);
  list<NodePtr>* nodeList = new list<NodePtr>;
  
  list<EdgePtr>::const_iterator it;
  for (it = leadingEdges_.begin(); it != leadingEdges_.end(); ++it) {
    EdgePtr edge = *it;
    
    for (int i = 0; i < 3; ++i) {
      const NodePtr& node = edge->getSourceNode();
      
      if (node->GetFlag() == false) {
        nodeList->push_back(node);
        node->SetFlag(true);
      }
      edge = edge->getNextEdgeInFace();
    }
  }
  return nodeList;
}
#endif


//--------------------------------------------------------------------------------------------------
list<EdgePtr>* Triangulation::getEdges(bool skip_boundary_edges) const {
  
  // collect all arcs (one half edge for each arc)
  // (boundary edges are also collected).
  
  list<EdgePtr>::const_iterator it;
  list<EdgePtr>* elist = new list<EdgePtr>;
  for (it = leadingEdges_.begin(); it != leadingEdges_.end(); ++it) {
    EdgePtr edge = *it;
    for (int i = 0; i < 3; ++i) {
      EdgePtr twinedge = edge->getTwinEdge();
      // only one of the half-edges
      
      if ( (!twinedge && !skip_boundary_edges) ||
           (twinedge && ((size_t)edge.get() > (size_t)twinedge.get())) )
        elist->push_front(edge);
      
      edge = edge->getNextEdgeInFace();
    }
  }
  return elist;
}


//--------------------------------------------------------------------------------------------------
EdgePtr Triangulation::splitTriangle(EdgePtr& edge, const NodePtr& point) {
  
  // Add a node by just splitting a triangle into three triangles
  // Assumes the half edge is located in the triangle
  // Returns a half edge with source node as the new node
  
//    double x, y, z;
//    x = point.x();
//    y = point.y();
//    z = point.z();
  
  // e#_n are new edges
  // e# are existing edges
  // e#_n and e##_n are new twin edges
  // e##_n are edges incident to the new node
  
  // Add the node to the structure
  //NodePtr new_node(new Node(x,y,z));

  NodePtr n1(edge->getSourceNode());
  EdgePtr e1(edge);
  
  EdgePtr e2(edge->getNextEdgeInFace());
  NodePtr n2(e2->getSourceNode());
  
  EdgePtr e3(e2->getNextEdgeInFace());
  NodePtr n3(e3->getSourceNode());
  
  EdgePtr e1_n = boost::make_shared<Edge>();
  EdgePtr e11_n = boost::make_shared<Edge>();
  EdgePtr e2_n = boost::make_shared<Edge>();
  EdgePtr e22_n = boost::make_shared<Edge>();
  EdgePtr e3_n = boost::make_shared<Edge>();
  EdgePtr e33_n = boost::make_shared<Edge>();
  
  e1_n->setSourceNode(n1);
  e11_n->setSourceNode(point);
  e2_n->setSourceNode(n2);
  e22_n->setSourceNode(point);
  e3_n->setSourceNode(n3);
  e33_n->setSourceNode(point);
  
  e1_n->setTwinEdge(e11_n);
  e11_n->setTwinEdge(e1_n);
  e2_n->setTwinEdge(e22_n);
  e22_n->setTwinEdge(e2_n);
  e3_n->setTwinEdge(e33_n);
  e33_n->setTwinEdge(e3_n);
  
  e1_n->setNextEdgeInFace(e33_n);
  e2_n->setNextEdgeInFace(e11_n);
  e3_n->setNextEdgeInFace(e22_n);
  
  e11_n->setNextEdgeInFace(e1);
  e22_n->setNextEdgeInFace(e2);
  e33_n->setNextEdgeInFace(e3);
  
  // and update old's next edge
  e1->setNextEdgeInFace(e2_n);
  e2->setNextEdgeInFace(e3_n);
  e3->setNextEdgeInFace(e1_n);
  
  // add the three new leading edges, 
  // Must remove the old leading edge from the list.
  // Use the field telling if an edge is a leading edge
  // NOTE: Must search in the list!!!
  
  if (e1->isLeadingEdge())
    removeLeadingEdgeFromList(e1);
  else if (e2->isLeadingEdge())
    removeLeadingEdgeFromList(e2);
  else if(e3->isLeadingEdge())
    removeLeadingEdgeFromList(e3);
  else
    assert( false );        // one of the edges should be leading
  
  addLeadingEdge(e1_n);
  addLeadingEdge(e2_n);
  addLeadingEdge(e3_n);
  
  // Return a half edge incident to the new node (with the new node as source node)
  
  return e11_n;
}


//--------------------------------------------------------------------------------------------------
void Triangulation::swapEdge(EdgePtr& diagonal) {
  
  // Note that diagonal is both input and output and it is always
  // kept in counterclockwise direction (this is not required by all 
  // functions in TriangulationHelper now)
  
  // Swap by rotating counterclockwise
  // Use the same objects - no deletion or new objects
  EdgePtr eL(diagonal);
  EdgePtr eR(eL->getTwinEdge());
  EdgePtr eL_1(eL->getNextEdgeInFace());
  EdgePtr eL_2(eL_1->getNextEdgeInFace());
  EdgePtr eR_1(eR->getNextEdgeInFace());
  EdgePtr eR_2(eR_1->getNextEdgeInFace());
  
  // avoid node to be dereferenced to zero and deleted
  NodePtr nR(eR_2->getSourceNode());
  NodePtr nL(eL_2->getSourceNode());
  
  eL->setSourceNode(nR);
  eR->setSourceNode(nL);
  
  // and now 6 1-sewings
  eL->setNextEdgeInFace(eL_2);
  eL_2->setNextEdgeInFace(eR_1);
  eR_1->setNextEdgeInFace(eL);
  
  eR->setNextEdgeInFace(eR_2);
  eR_2->setNextEdgeInFace(eL_1);
  eL_1->setNextEdgeInFace(eR);
  
  if (eL->isLeadingEdge())
    removeLeadingEdgeFromList(eL);
  else if (eL_1->isLeadingEdge())
    removeLeadingEdgeFromList(eL_1);
  else if (eL_2->isLeadingEdge())
    removeLeadingEdgeFromList(eL_2);
  
  if (eR->isLeadingEdge())
    removeLeadingEdgeFromList(eR);
  else if (eR_1->isLeadingEdge())
    removeLeadingEdgeFromList(eR_1);
  else if (eR_2->isLeadingEdge())
    removeLeadingEdgeFromList(eR_2);
  
  addLeadingEdge(eL);
  addLeadingEdge(eR);
}


////--------------------------------------------------------------------------
//static void printEdge(const Dart& dart, ostream& ofile) {
//
//  Dart d0 = dart;
//  d0.alpha0();
//
//  ofile << dart.x() << " " << dart.y() << endl;
//  ofile << d0.x() << " " << d0.y() << endl;
//}


//--------------------------------------------------------------------------
bool Triangulation::checkDelaunay() const {
  
  // ???? outputs !!!!
  // ofstream os("qweND.dat");
  const list<EdgePtr>& leadingEdges = getLeadingEdges();
  
  list<EdgePtr>::const_iterator it;
  bool ok = true;
  int noNotDelaunay = 0;
  
  for (it = leadingEdges.begin(); it != leadingEdges.end(); ++it) {
    EdgePtr edge = *it;
    
    for (int i = 0; i < 3; ++i) {
      EdgePtr twinedge = edge->getTwinEdge();
      
      // only one of the half-edges
      if (!twinedge || (size_t)edge.get() > (size_t)twinedge.get()) {
        Dart dart(edge);
        if (helper->swapTestDelaunay<TTLtraits>(dart)) {
          noNotDelaunay++;
          
          //printEdge(dart,os); os << "\n";
          ok = false;
          //cout << "............. not Delaunay .... " << endl;
        }
      }
      edge = edge->getNextEdgeInFace();
    }
  }
  
#ifdef DEBUG_HE
  cout << "!!! Triangulation is NOT Delaunay: " << noNotDelaunay << " edges\n" << endl;
#endif
  
  return ok;
}


//--------------------------------------------------------------------------------------------------
void Triangulation::optimizeDelaunay() {

  // This function is also present in ttl where it is implemented
  // generically.
  // The implementation below is tailored for the half-edge data structure,
  // and is thus more efficient

  // Collect all interior edges (one half edge for each arc)
  bool skip_boundary_edges = true;
  list<EdgePtr>* elist = getEdges(skip_boundary_edges);
  
  // Assumes that elist has only one half-edge for each arc.
  bool cycling_check = true;
  bool optimal = false;
  list<EdgePtr>::const_iterator it;
  while(!optimal) {
    optimal = true;
    for (it = elist->begin(); it != elist->end(); ++it) {
      EdgePtr edge = *it;
      
      Dart dart(edge);
      // Constrained edges should not be swapped
      if (helper->swapTestDelaunay<TTLtraits>(dart, cycling_check)) {
        optimal = false;
        swapEdge(edge);
      }
    }
  }
  delete elist;
}


//--------------------------------------------------------------------------------------------------
EdgePtr Triangulation::getInteriorNode() const {
  
  const list<EdgePtr>& leadingEdges = getLeadingEdges();
  list<EdgePtr>::const_iterator it;
  for (it = leadingEdges.begin(); it != leadingEdges.end(); ++it) {
    EdgePtr edge = *it;
    
    // multiple checks, but only until found
    for (int i = 0; i < 3; ++i) {
      if (edge->getTwinEdge()) {
        
        if (!helper->isBoundaryNode(Dart(edge)))
          return edge;
      }
      edge = edge->getNextEdgeInFace();
    }
  }
  return EdgePtr(); // no boundary nodes
}


//--------------------------------------------------------------------------------------------------
EdgePtr Triangulation::getBoundaryEdgeInTriangle(const EdgePtr& e) const {
  EdgePtr edge = e;
  
  if (helper->isBoundaryEdge(Dart(edge)))
    return edge;

  edge = edge->getNextEdgeInFace();
  if (helper->isBoundaryEdge(Dart(edge)))
    return edge;

  edge = edge->getNextEdgeInFace();
  if (helper->isBoundaryEdge(Dart(edge)))
    return edge;
  
  return EdgePtr();
}


//--------------------------------------------------------------------------------------------------
EdgePtr Triangulation::getBoundaryEdge() const {

  // Get an arbitrary (CCW) boundary edge
  // If the triangulation is closed, NULL is returned

  const list<EdgePtr>& leadingEdges = getLeadingEdges();
  list<EdgePtr>::const_iterator it;
  EdgePtr edge;
  
  for (it = leadingEdges.begin(); it != leadingEdges.end(); ++it) {
    edge = getBoundaryEdgeInTriangle(*it);
    
    if (edge)
      return edge;
  }
  return EdgePtr();
}


//--------------------------------------------------------------------------------------------------
void Triangulation::printEdges(ofstream& os) const {
      
  // Print source node and target node for each edge face by face,
  // but only one of the half-edges.
  
  const list<EdgePtr>& leadingEdges = getLeadingEdges();
  list<EdgePtr>::const_iterator it;
  for (it = leadingEdges.begin(); it != leadingEdges.end(); ++it) {
    EdgePtr edge = *it;
    
    for (int i = 0; i < 3; ++i) {
      EdgePtr twinedge = edge->getTwinEdge();
      
      // Print only one edge (the highest value of the pointer)
      if (!twinedge || (size_t)edge.get() > (size_t)twinedge.get()) {
        // Print source node and target node
        NodePtr node = edge->getSourceNode();
        os << node->GetX() << " " << node->GetY() << endl;
        node = edge->getTargetNode();
        os << node->GetX() << " " << node->GetY() << endl;
        os << '\n'; // blank line
      }
      edge = edge->getNextEdgeInFace();
    }
  }
}
