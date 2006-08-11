// // Copyright (c) 2002  Max Planck Institut fuer Informatik (Germany).
// All rights reserved.
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
// $URL: svn+ssh://fcacciola@scm.gforge.inria.fr/svn/cgal/trunk/Surface_mesh_simplification/test/Surface_mesh_simplification/LT_edge_collapse_test.cpp $
// $Id: LT_edge_collapse_test.cpp 32177 2006-07-03 11:55:13Z fcacciola $
// 
//
// Author(s)     : Fernando Cacciola <fernando.cacciola@gmail.com>


#include <iostream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <boost/tokenizer.hpp>

#define CGAL_CHECK_EXPENSIVE

#include <CGAL/Real_timer.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_BGL.h>
#include <CGAL/Polyhedron_extended_BGL.h>
#include <CGAL/Polyhedron_BGL_properties.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Constrained_triangulation_2.h>

//#define CGAL_SURFACE_SIMPLIFICATION_ENABLE_LT_TRACE 4
#define CGAL_SURFACE_SIMPLIFICATION_ENABLE_TRACE 1

//#define STATS
//#define AUDIT

void Surface_simplification_external_trace( std::string s )
{
  static std::ofstream lout("tsms_log.txt");
  lout << s << std::flush << std::endl ;
  std::printf("%s\n",s.c_str());
} 

int exit_code = 0 ;

#include <CGAL/Surface_mesh_simplification_vertex_pair_collapse.h>
#include <CGAL/Surface_mesh_simplification/Policies/Count_stop_pred.h>
#include <CGAL/Surface_mesh_simplification/Polyhedron_is_vertex_fixed_map.h>
#include <CGAL/Surface_mesh_simplification/Polyhedron_edge_cached_pointer_map.h>

#ifdef TEST_LT
#include <CGAL/Surface_mesh_simplification/Policies/LindstromTurk.h>
#else
#include <CGAL/Surface_mesh_simplification/Policies/Minimal_collapse_data.h>
#include <CGAL/Surface_mesh_simplification/Policies/Set_minimal_collapse_data.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_length_cost.h>
#include <CGAL/Surface_mesh_simplification/Policies/Midpoint_vertex_placement.h>
#endif

using namespace std ;
using namespace boost ;
using namespace CGAL ;

typedef Simple_cartesian<double> Kernel;
typedef Kernel::Vector_3         Vector;
typedef Kernel::Point_3          Point;

template <class Refs, class Traits>
struct My_vertex : public HalfedgeDS_vertex_base<Refs,Tag_true,Point> 
{
  typedef HalfedgeDS_vertex_base<Refs,Tag_true,Point> Base ;
 
  My_vertex() : ID(-1), IsFixed(false) {} 
  My_vertex( Point p ) : Base(p), ID(-1), IsFixed(false) {}

  int id() const { return ID ; }
    
  int  ID; 
  bool IsFixed ;
} ;

template <class Refs, class Traits>
struct My_halfedge : public HalfedgeDS_halfedge_base<Refs> 
{ 
  My_halfedge() 
   : 
     ID(-1) 
  {}
 
  int id() const { return ID ; }
  
  int ID; 
  
  void* cached_pointer ;
};

template <class Refs, class Traits>
struct My_face : public HalfedgeDS_face_base<Refs,Tag_true,typename Traits::Plane_3>
{
  typedef HalfedgeDS_face_base<Refs,Tag_true,typename Traits::Plane_3> Base ;
  
  My_face() : ID(-1) {}
  My_face( typename Traits::Plane_3 plane ) : Base(plane) {}
  
  int id() const { return ID ; }
  
  int ID; 
};

struct My_items : public Polyhedron_items_3 
{
    template < class Refs, class Traits>
    struct Vertex_wrapper {
        typedef My_vertex<Refs,Traits> Vertex;
    };
    template < class Refs, class Traits>
    struct Halfedge_wrapper { 
        typedef My_halfedge<Refs,Traits>  Halfedge;
    };
    template < class Refs, class Traits>
    struct Face_wrapper {
        typedef My_face<Refs,Traits> Face;
    };
};

typedef Polyhedron_3<Kernel,My_items> Polyhedron; 

typedef Polyhedron::Vertex                                   Vertex;
typedef Polyhedron::Vertex_iterator                          Vertex_iterator;
typedef Polyhedron::Vertex_handle                            Vertex_handle;
typedef Polyhedron::Vertex_const_handle                      Vertex_const_handle;
typedef Polyhedron::Halfedge_handle                          Halfedge_handle;
typedef Polyhedron::Halfedge_const_handle                    Halfedge_const_handle;
typedef Polyhedron::Edge_iterator                            Edge_iterator;
typedef Polyhedron::Facet_iterator                           Facet_iterator;
typedef Polyhedron::Halfedge_around_vertex_const_circulator  HV_circulator;
typedef Polyhedron::Halfedge_around_facet_circulator         HF_circulator;

CGAL_BEGIN_NAMESPACE

template<>
struct External_polyhedron_get_is_vertex_fixed<Polyhedron>
{
  bool operator() ( Polyhedron const&, Vertex_const_handle v ) const { return v->IsFixed; }
}  ;

template<>
struct External_polyhedron_access_edge_cached_pointer<Polyhedron>
{
  void*& operator() ( Polyhedron&, Halfedge_handle he ) const { return he->cached_pointer; }
}  ;

CGAL_END_NAMESPACE  


#ifdef AUDIT

typedef CGAL::Constrained_triangulation_2<> CT ;
CT ct ;

double sCostMatchThreshold   = 1e-2 ;
double sVertexMatchThreshold = 1e-2 ;

struct Audit_data
{
  Audit_data( Point p, Point q ) : P(p), Q(q) {}
  
  Point            P, Q ;
  optional<double> Cost ;
  optional<Point>  NewVertexPoint ;
} ;
typedef shared_ptr<Audit_data> Audit_data_ptr ;
typedef vector<Audit_data_ptr> Audit_data_vector ;

Audit_data_vector sAuditData ;

Audit_data_ptr lookup_audit ( Point p, Point q )
{ 
  
  for ( Audit_data_vector::iterator it = sAuditData.begin() ; it != sAuditData.end() ; ++ it )
  {
    Audit_data_ptr data = *it ;
    if ( data->P == p && data->Q == q )
      return data ;
  }
  return Audit_data_ptr() ;  
}

Audit_data_ptr get_or_create_audit ( Point p, Point q )
{ 
  Audit_data_ptr data = lookup_audit(p,q);
  if ( !data )
  {
    data = Audit_data_ptr(new Audit_data(p,q));
    sAuditData.push_back(data);
    ct.insert_constriant(p,q));
  }  
  return data ;
}

struct Audit_report
{
  Audit_report( int eid ) 
   : 
     HalfedgeID(eid)
   , CostMatched(false)
   , OrderMatched(false)
   , NewVertexMatched(false)
  {}
 
  
  int              HalfedgeID ;
  Audit_data_ptr   AuditData ; 
  optional<double> Cost ;
  optional<Point>  NewVertexPoint ;
  bool             CostMatched ;
  bool             OrderMatched ;
  bool             NewVertexMatched ; 
} ;

typedef shared_ptr<Audit_report> Audit_report_ptr ;

typedef map<int,Audit_report_ptr> Audit_report_map ;

Audit_report_map sAuditReport ;

typedef char_separator<char> Separator ;
typedef tokenizer<Separator> Tokenizer ;

Point ParsePoint( vector<string> tokens, int aIdx )
{
  double x = atof(tokens[aIdx+0].c_str());
  double y = atof(tokens[aIdx+1].c_str());
  double z = atof(tokens[aIdx+2].c_str());
  
  return Point(x,y,z);
}

void ParseAuditLine( string s )
{
  Tokenizer tk(s,Separator(","));
  vector<string> tokens(tk.begin(),tk.end());
  if ( tokens.size() >= 7 )
  {
    Point p = ParsePoint(tokens,0);
    Point q = ParsePoint(tokens,3);
    
    Audit_data_ptr lData = get_or_create_audit(p,q);
    
    double cost = atof(tokens[6].c_str()) ;
    
    if ( cost != std::numeric_limits<double>::max() )
      lData->Cost = cost ;
    
    if ( tokens.size() == 10 )
      lData->NewVertexPoint = ParsePoint(tokens,7);
  }
  else
    cerr << "WARNING: Invalid audit line: [" << s << "]" << endl ;
}


void ParseAudit ( string name )
{
  ifstream is(name.c_str());
  if ( is )
  {
    while ( is )
    {
      string line ;
      getline(is,line);
      if ( line.length() > 0 )
        ParseAuditLine(line);
    }
  }
  else cerr << "Warning: Audit file " << name << " doesn't exist." << endl ;
}

string to_string( optional<double> const& c )
{
  if ( c )
       return str( format("%1%") % (*c) ) ;
  else return "NONE" ;  
}

string to_string( optional<Point> const& p )
{
  if ( p )
       return str( format("(%1%,%2%,%3%)") % p->x() % p->y() % p->z() ) ;
  else return "NONE" ;  
}

void register_collected_edge( Vertex_handle    const& p 
                            , Vertex_handle    const& q
                            , Halfedge_handle  const& e
                            , optional<double> const& cost
                            , optional<Point>  const& newv
                            )
{
  Audit_report_ptr lReport( new Audit_report(e->ID) ) ;
  
  lReport->Cost           = cost ;
  lReport->NewVertexPoint = newv ;
  
  Audit_data_ptr lData ; //= lookup_audit(p->point(),q->point());
  if ( lData )
  {
    lReport->AuditData = lData ;
    
    if ( !!lData->Cost && !!cost )
    {
      double d = fabs(*lData->Cost - *cost) ;
      if ( d < sCostMatchThreshold )
        lReport->CostMatched = true ;
    }
    else if (!lData->Cost && !cost )
      lReport->CostMatched = true ;
      
    if ( !!lData->NewVertexPoint && !!newv )
    {
      double d = std::sqrt(squared_distance(*lData->NewVertexPoint,*newv));
      
      if ( d < sVertexMatchThreshold )
        lReport->NewVertexMatched = true ;
    }
    else if ( !lData->NewVertexPoint && !newv )
      lReport->NewVertexMatched = true ;
  }
  
  sAuditReport.insert( make_pair(e->ID,lReport) ) ;
} 
#endif

#ifdef STATS
int sInitial ;
int sCollected ;
int sProcessed ;
int sCollapsed ;
int sNonCollapsable ;
int sCostUncomputable ;
int sFixed ;
int sRemoved ;
#endif


struct Visitor
{
  void OnStarted( Polyhedron& aSurface ) 
  {
#ifdef STATS
    sInitial = aSurface.size_of_halfedges() / 2 ;
#endif
  } 
  
  void OnFinished ( Polyhedron& aSurface ) 
  {
#ifdef STATS
    printf("\n");
#endif
  } 
  
  void OnThetrahedronReached ( Polyhedron& aSurface ) {} 
  void OnStopConditionReached( Polyhedron& aSurface ) {} 
  
  void OnCollected( Polyhedron&            aSurface
                  , Vertex_handle const&   aP
                  , Vertex_handle const&   aQ
                  , bool                   aIsPFixed
                  , bool                   aIsQFixed
                  , Halfedge_handle const& aEdge
                  , optional<double>       aCost
                  , optional<Point>        aNewVertexPoint
                  )
  {
#ifdef AUDIT  
    register_collected_edge(aP,aQ,aEdge,aCost,aNewVertexPoint); 
#endif
#ifdef STATS
    ++sCollected ;
    printf("\rEdges collected %d",sCollected);
#endif
 }                
  
  void OnProcessed( Polyhedron&            aSurface
                  , Vertex_handle const&   aP
                  , Vertex_handle const&   aQ
                  , bool                   aIsPFixed
                  , bool                   aIsQFixed
                  , Halfedge_handle const& aEdge
                  , optional<double>       aCost
                  , optional<Point>        aNewVertexPoint
                  , bool                   aIsCollapsable
                  )
  {
#ifdef STATS
    if ( sProcessed == 0 )
      printf("\n");  
    ++ sProcessed ;
    if ( aIsPFixed && aIsQFixed )
      ++ sFixed ;
    else if ( !aIsCollapsable )
    {
      if ( !aCost )
           ++ sCostUncomputable ;
      else ++ sNonCollapsable ;
    }
    else 
      ++ sCollapsed;
#endif  
 
  }                
  
  void OnCollapsed( Polyhedron&            aSurface
                  , Vertex_handle const&   aP
                  , Halfedge_handle const& aPQ
                  , Halfedge_handle const& aPT
                  , Halfedge_handle const& aQB
                  )
  {
#ifdef STATS
    sRemoved += 3 ;
    printf("\r%d%%",((int)(100.0*((double)sRemoved/(double)sInitial))));
#endif  
  }                
 
} ;

// This is here only to allow a breakpoint to be placed so I can trace back the problem.
void error_handler ( char const* what, char const* expr, char const* file, int line, char const* msg )
{
  cerr << "CGAL error: " << what << " violation!" << endl 
       << "Expr: " << expr << endl
       << "File: " << file << endl 
       << "Line: " << line << endl;
  if ( msg != 0)
    cerr << "Explanation:" << msg << endl;
    
  throw std::logic_error(what);  
}

using namespace CGAL::Triangulated_surface_mesh::Simplification ;

char const* matched_alpha ( bool matched )
{
  return matched ? "matched" : "UNMATCHED" ; 
}

bool Test ( int aStop, string name )
{
  bool rSucceeded = false ;
  
  string off_name    = name+string(".off");
  string audit_name  = name+string(".audit");
  string result_name = name+string(".out.off");
  
  cout << "Testing simplification of surface " << off_name << " using "
#ifdef TEST_LT
       << " LindstromTurk method."
#else
       << " Squared-length/mid-point method."
#endif  
       << endl ;
  
  ifstream off_is(off_name.c_str());
  if ( off_is )
  {
    Polyhedron lP; 
    
    scan_OFF(off_is,lP,true);
    
    if ( lP.is_valid() )
    {
      cout << (lP.size_of_halfedges()/2) << " edges..." << endl ;
    
      cout << setprecision(19) ;
    
      int lVertexID = 0 ;
      for ( Polyhedron::Vertex_iterator vi = lP.vertices_begin(); vi != lP.vertices_end() ; ++ vi )
        vi->ID = lVertexID ++ ;    
      
      int lHalfedgeID = 0 ;
      for ( Polyhedron::Halfedge_iterator hi = lP.halfedges_begin(); hi != lP.halfedges_end() ; ++ hi )
        hi->ID = lHalfedgeID++ ;    
      
      int lFacetID = 0 ;
      for ( Polyhedron::Facet_iterator fi = lP.facets_begin(); fi != lP.facets_end() ; ++ fi )
        fi->ID = lFacetID ++ ;    
  
  #ifdef AUDIT
      sAuditData .clear();
      sAuditReport.clear();
      ParseAudit(audit_name);
      cout << "Audit data loaded." << endl ;
  #endif
      
  #ifdef TEST_LT
      typedef LindstromTurk_collapse_data<Polyhedron> Collapse_data ;
      
      Set_LindstromTurk_collapse_data<Collapse_data> Set_collapse_data ;
      LindstromTurk_cost             <Collapse_data> Get_cost ;
      LindstromTurk_vertex_placement <Collapse_data> Get_vertex_point ;
  #else
      typedef Minimal_collapse_data<Polyhedron> Collapse_data ;
      
      Set_minimal_collapse_data <Collapse_data> Set_collapse_data ;
      Edge_length_cost          <Collapse_data> Get_cost ;
      Midpoint_vertex_placement <Collapse_data> Get_vertex_point ;
  #endif
      
      Count_stop_condition           <Collapse_data> Should_stop(aStop);
          
      Collapse_data::Params lParams;
      
      Visitor lVisitor ;
  
      Real_timer t ; t.start();    
      int r = vertex_pair_collapse(lP,Set_collapse_data,&lParams,Get_cost,Get_vertex_point,Should_stop,&lVisitor);
      t.stop();
              
      ofstream off_out(result_name.c_str(),ios::trunc);
      off_out << lP ;
      
      cout << "\nFinished...\n"
           << "Ellapsed time: " << t.time() << " seconds.\n" 
           << r << " edges removed.\n"
           << endl
           << lP.size_of_vertices() << " final vertices.\n"
           << (lP.size_of_halfedges()/2) << " final edges.\n"
           << lP.size_of_facets() << " final triangles.\n" 
           << ( lP.is_valid() ? " valid\n" : " INVALID!!\n" ) ;
  
  #ifdef STATS
      cout << "\n------------STATS--------------\n"
           << sProcessed        << " edges processed.\n"
           << sCollapsed        << " edges collapsed.\n" 
           << sNonCollapsable   << " non-collapsable edges.\n"
           << sCostUncomputable << " non-computable edges.\n"
           << sFixed            << " fixed edges.\n"
           << sRemoved          << " edges removed.\n" 
           << (sRemoved/3)      << " vertices removed." ;
  #endif
  
  #ifdef AUDIT
      unsigned lMatches = 0 ;
                    
      cout << "Audit report:\n" ;
      for ( Audit_report_map::const_iterator ri = sAuditReport.begin() ; ri != sAuditReport.end() ; ++ ri )
      {
        Audit_report_ptr lReport = ri->second ;
        
        if ( lReport->AuditData )
        {
          if ( lReport->NewVertexMatched )
            ++ lMatches ;
             
          cout << "Collapsed Halfedge " << lReport->HalfedgeID << endl 
               << "  Cost: Actual=" << to_string(lReport->Cost) << ", Expected=" << to_string(lReport->AuditData->Cost)
                                    << ". " << matched_alpha(lReport->CostMatched) << endl
               << "  New vertex point: Actual=" << to_string(lReport->NewVertexPoint) << ", Expected=" 
                                                << to_string(lReport->AuditData->NewVertexPoint) 
                                                << ". " << matched_alpha(lReport->NewVertexMatched)
               << endl ;
        }
        else
        {
          cout << "No audit data for Halfedge " << lReport->HalfedgeID << endl ;
            ++ lMatches ;
        }  
      }
      
      rSucceeded = ( lMatches == sAuditReport.size() ) ;
  #else
      rSucceeded = true ;
  #endif
    }
    else
    {
      cerr << "Invalid surface: " << name << endl ;
    }

  }
  else
  {
    cerr << "Unable to open test file " << name << endl ;
  }              
  
  return rSucceeded ;
}

int main( int argc, char** argv ) 
{
  set_error_handler  (error_handler);
  set_warning_handler(error_handler);
  
  if ( argc > 3 )
  {
    vector<string> cases ;
  
    int lStop = atoi(argv[1]);
    
    string lFolder(argv[2]);
    
    for ( int i = 3 ; i < argc ; ++i )
      cases.push_back( string(argv[i]) ) ;
      
    unsigned lOK = 0 ;
    for ( vector<string>::const_iterator it = cases.begin(); it != cases.end() ; ++ it )
    {
      if ( Test( lStop, lFolder + *it) )
        ++ lOK ;
    }  
    
    cout << endl
         << lOK                   << " cases succedded." << endl
         << (cases.size() - lOK ) << " cases failed." << endl ;
          
    return lOK == cases.size() ? 0 : 1 ;
  }
  else
  {
    cout << "USAGE: LT_edge_collapse_test final_edge_count folder file0 file1 file2 ..." << endl ;
    
    return 1 ;
  } 
}

// EOF //
