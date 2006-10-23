// Copyright (c) 2005, 2006 Fernando Luis Cacciola Carballal. All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
// Author(s)     : Fernando Cacciola <fernando_cacciola@ciudad.com.ar>
//
#ifndef CGAL_SURFACE_MESH_SIMPLIFICATION_DETAIL_EDGE_COLLAPSE_H
#define CGAL_SURFACE_MESH_SIMPLIFICATION_DETAIL_EDGE_COLLAPSE_H 1

#include <vector>
#include <set>

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <CGAL/Surface_mesh_simplification/Detail/Common.h>

CGAL_BEGIN_NAMESPACE

namespace Surface_mesh_simplification
{

//
// Implementation of the vertex-pair collapse triangulated surface mesh simplification algorithm
//
template<class ECM_
        ,class ShouldStop_
        ,class VertexPointMap_
        ,class VertexIsFixedMap_
        ,class EdgeIndexMap_
        ,class EdgeIsBorderMap_
        ,class SetCache_
        ,class GetCost_
        ,class GetPlacement_
        ,class CostParams_ 
        ,class PlacementParams_ 
        ,class VisitorT_
        >
class EdgeCollapse
{
public:

  typedef ECM_              ECM ;
  typedef ShouldStop_       ShouldStop ;
  typedef VertexPointMap_   VertexPointMap ;
  typedef VertexIsFixedMap_ VertexIsFixedMap ;
  typedef EdgeIndexMap_     EdgeIndexMap ;
  typedef EdgeIsBorderMap_  EdgeIsBorderMap ;
  typedef SetCache_         SetCache ;
  typedef GetCost_          GetCost ;
  typedef GetPlacement_     GetPlacement ;
  typedef CostParams_       CostParams ;
  typedef PlacementParams_  PlacementParams ;
  typedef VisitorT_         VisitorT ;
  
  typedef EdgeCollapse Self ;
  
  typedef boost::graph_traits  <ECM>       GraphTraits ;
  typedef boost::graph_traits  <ECM const> ConstGraphTraits ;
  typedef halfedge_graph_traits<ECM>       HalfedgeGraphTraits ; 
  
  typedef typename GraphTraits::vertex_descriptor      vertex_descriptor ;
  typedef typename GraphTraits::vertex_iterator        vertex_iterator ;
  typedef typename GraphTraits::edge_descriptor        edge_descriptor ;
  typedef typename GraphTraits::edge_iterator          edge_iterator ;
  typedef typename GraphTraits::out_edge_iterator      out_edge_iterator ;
  typedef typename GraphTraits::in_edge_iterator       in_edge_iterator ;
  typedef typename GraphTraits::traversal_category     traversal_category ;
  typedef typename GraphTraits::edges_size_type        size_type ;
  
  typedef typename ConstGraphTraits::vertex_descriptor const_vertex_descriptor ;
  typedef typename ConstGraphTraits::edge_descriptor   const_edge_descriptor ;
  typedef typename ConstGraphTraits::in_edge_iterator  const_in_edge_iterator ;
  
  typedef typename HalfedgeGraphTraits::undirected_edge_iterator undirected_edge_iterator ;
  typedef typename HalfedgeGraphTraits::Point                    Point ;

  typedef typename GetCost     ::result_type Optional_cost_type ;
  typedef typename GetPlacement::result_type Optional_placement_type ;
  
  typedef typename SetCache::Cache Cache ;

  typedef typename Kernel_traits<Point>::Kernel Kernel ;
  
  typedef typename Kernel::Equal_3 Equal_3 ;

  struct Compare_id
  {
    Compare_id() : mAlgorithm(0) {}
    
    Compare_id( Self const* aAlgorithm ) : mAlgorithm(aAlgorithm) {}
    
    bool operator() ( edge_descriptor const& a, edge_descriptor const& b ) const 
    {
      return mAlgorithm->get_directed_edge_id(a) < mAlgorithm->get_directed_edge_id(b);
    }
    
    Self const* mAlgorithm ;
  } ;
  
  struct Compare_cost
  {
    Compare_cost() : mAlgorithm(0) {}
    
    Compare_cost( Self const* aAlgorithm ) : mAlgorithm(aAlgorithm) {}
    
    bool operator() ( edge_descriptor const& a, edge_descriptor const& b ) const
    { 
      // NOTE: A cost is an optional<> value.
      // Absent optionals are ordered first; that is, "none < T" and "T > none" for any defined T != none.
      // In consequence, edges with undefined costs will be promoted to the top of the priority queue and poped out first.
      return mAlgorithm->get_cost(a) < mAlgorithm->get_cost(b);
    }
    
    Self const* mAlgorithm ;
  } ;
  
  struct Undirected_edge_id : boost::put_get_helper<size_type, Undirected_edge_id>
  {
    typedef boost::readable_property_map_tag category;
    typedef size_type                        value_type;
    typedef size_type                        reference;
    typedef edge_descriptor                  key_type;
    
    Undirected_edge_id() : mAlgorithm(0) {}
    
    Undirected_edge_id( Self const* aAlgorithm ) : mAlgorithm(aAlgorithm) {}
    
    size_type operator[] ( edge_descriptor const& e ) const { return mAlgorithm->get_undirected_edge_id(e); }
    
    Self const* mAlgorithm ;
  } ;
  
  
  typedef Modifiable_priority_queue<edge_descriptor,Compare_cost,Undirected_edge_id> PQ ;
  typedef typename PQ::handle pq_handle ;
  
  // An Edge_data is associated with EVERY _undirected_ edge in the mesh (collapsable or not).
  // It relates the edge with the PQ-handle needed to update the priority queue
  // It also relates the edge with a policy-based cache
  class Edge_data
  {
  public :
  
    Edge_data() : mPQHandle() {}
    
    Cache const& cache() const { return mCache ; }
    Cache &      cache()       { return mCache ; }
    
    pq_handle PQ_handle() const { return mPQHandle ;}
    
    bool is_in_PQ() const { return mPQHandle != PQ::null_handle() ; }
    
    void set_PQ_handle( pq_handle h ) { mPQHandle = h ; }
    
    void reset_PQ_handle() { mPQHandle = PQ::null_handle() ; }
    
  private:  
    
    Cache     mCache ;
    pq_handle mPQHandle ;
  } ;
  typedef Edge_data* Edge_data_ptr ;
  typedef boost::scoped_array<Edge_data> Edge_data_array ;
  
  
public:

  EdgeCollapse( ECM&                    aSurface
              , ShouldStop       const& aShouldStop 
              , VertexPointMap   const& aVertex_point_map 
              , VertexIsFixedMap const& aVertex_is_fixed_map 
              , EdgeIndexMap     const& aEdge_index_map 
              , EdgeIsBorderMap  const& aEdge_is_border_map 
              , SetCache         const& aSetCache
              , GetCost          const& aGetCost
              , GetPlacement     const& aGetPlacement
              , CostParams       const* aCostParams       // Can be NULL
              , PlacementParams  const* aPlacementParams  // Can be NULL
              , VisitorT*               aVisitor          // Can be NULL
              ) ;
  
  int run() ;
  
private:
  
  void Collect();
  void Loop();
  bool Is_collapsable( edge_descriptor const& aEdge ) ;
  bool Is_tetrahedron( edge_descriptor const& h1 ) ;
  bool Is_open_triangle( edge_descriptor const& h1 ) ;
  void Collapse( edge_descriptor const& aEdge ) ;
  void Update_neighbors( vertex_descriptor const& aKeptV ) ;
  
  size_type get_directed_edge_id   ( const_edge_descriptor const& aEdge ) const { return Edge_index_map[aEdge]; }
  size_type get_undirected_edge_id ( const_edge_descriptor const& aEdge ) const { return get_directed_edge_id(aEdge) / 2 ; }

  bool is_primary_edge ( const_edge_descriptor const& aEdge ) const { return ( get_directed_edge_id(aEdge) % 2 ) == 0 ; }
  
  edge_descriptor primary_edge ( edge_descriptor const& aEdge ) 
  { 
    return is_primary_edge(aEdge) ? aEdge : opposite_edge(aEdge,mSurface) ;
  }  
    
  bool is_vertex_fixed ( const_vertex_descriptor const& aV ) const { return Vertex_is_fixed_map[aV] ; }
  
  bool is_border ( const_edge_descriptor const& aEdge ) const { return Edge_is_border_map[aEdge] ; }    
  
  bool is_undirected_edge_a_border ( const_edge_descriptor const& aEdge ) const
  {
    return is_border(aEdge) || is_border(opposite_edge(aEdge,mSurface)) ;
  }    
  
  bool is_border ( const_vertex_descriptor const& aV ) const ;
  
  Edge_data& get_data ( edge_descriptor const& aEdge ) const 
  { 
    CGAL_assertion( is_primary_edge(aEdge) ) ;
    return mEdgeDataArray[get_undirected_edge_id(aEdge)];
  }
  
  Point get_point ( const_vertex_descriptor const aV ) const { return get(Vertex_point_map,aV); }
  
  tuple<const_vertex_descriptor,const_vertex_descriptor> get_vertices( const_edge_descriptor const& aEdge ) const
  {
    const_vertex_descriptor p,q ;
    p = source(aEdge,mSurface);
    q = target(aEdge,mSurface);
    return make_tuple(p,q);
  }
  
  tuple<vertex_descriptor,vertex_descriptor> get_vertices( edge_descriptor const& aEdge ) 
  {
    vertex_descriptor p,q ;
    p = source(aEdge,mSurface);
    q = target(aEdge,mSurface);
    return make_tuple(p,q);
  }
  
  std::string vertex_to_string( const_vertex_descriptor const& v ) const
  {
    Point const& p = get_point(v);
    return boost::str( boost::format("[V%1%:%2%]") % v->ID % xyz_to_string(p) ) ;
  }
    
  std::string edge_to_string ( const_edge_descriptor const& aEdge ) const
  {
    const_vertex_descriptor p,q ; tie(p,q) = get_vertices(aEdge);
    return boost::str( boost::format("{E%1% %2%->%3%}") % aEdge->ID % vertex_to_string(p) % vertex_to_string(q) ) ;
  }
  
  Optional_cost_type get_cost ( edge_descriptor const& aEdge ) const
  {
    return Get_cost(aEdge,mSurface,get_data(aEdge).cache(),mCostParams);
  }
  
  Optional_placement_type get_placement( edge_descriptor const& aEdge ) const
  {
    return Get_placement(aEdge,mSurface,get_data(aEdge).cache(),mPlacementParams);
  }
  
  void insert_in_PQ( edge_descriptor const& aEdge, Edge_data& aData ) 
  {
    CGAL_precondition( is_primary_edge(aEdge) ) ;
    CGAL_precondition(!aData.is_in_PQ());
    aData.set_PQ_handle(mPQ->push(aEdge));
  }
  
  void update_in_PQ( edge_descriptor const& aEdge, Edge_data& aData )
  {
    CGAL_precondition( is_primary_edge(aEdge) ) ;
    CGAL_precondition(aData.is_in_PQ());
    aData.set_PQ_handle(mPQ->update(aEdge,aData.PQ_handle())) ; 
  }   
  
  void remove_from_PQ( edge_descriptor const& aEdge, Edge_data& aData )
  {
    CGAL_precondition( is_primary_edge(aEdge) ) ;
    CGAL_precondition(aData.is_in_PQ());
    aData.set_PQ_handle(mPQ->erase(aEdge,aData.PQ_handle()));
  }   
  
  optional<edge_descriptor> pop_from_PQ() 
  {
    optional<edge_descriptor> rEdge = mPQ->extract_top();
    if ( rEdge )
    {
      CGAL_precondition( is_primary_edge(*rEdge) ) ;
      get_data(*rEdge).reset_PQ_handle();
    }  
    return rEdge ;  
  }
   
private:

  ECM&                    mSurface ;
  ShouldStop       const& Should_stop ;
  VertexPointMap   const& Vertex_point_map ;
  VertexIsFixedMap const& Vertex_is_fixed_map ;
  EdgeIndexMap     const& Edge_index_map ;
  EdgeIsBorderMap  const& Edge_is_border_map ;
  SetCache         const& Set_cache;   
  GetCost          const& Get_cost ;
  GetPlacement     const& Get_placement ;
  CostParams       const* mCostParams ;      // Can be NULL
  PlacementParams  const* mPlacementParams ; // Can be NULL
  VisitorT*               Visitor ;          // Can be NULL
  
private:

  Edge_data_array mEdgeDataArray ;
  
  boost::scoped_ptr<PQ> mPQ ;
    
  std::size_t mInitialEdgeCount ;
  std::size_t mCurrentEdgeCount ; 

  CGAL_ECMS_DEBUG_CODE ( unsigned mStep ; )
} ;

} // namespace Surface_mesh_simplification

CGAL_END_NAMESPACE

#include <CGAL/Surface_mesh_simplification/Detail/Edge_collapse_impl.h>

#endif // CGAL_SURFACE_MESH_SIMPLIFICATION_I_EDGE_COLLAPSE_H //
// EOF //
 
