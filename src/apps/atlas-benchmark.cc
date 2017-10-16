/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/**
 * @file atlas-benchmark.cc
 * @author Willem Deconinck
 *
 * Benchmark testing parallel performance of gradient computation using the
 * Green-Gauss Theorem on an edge-based median-dual mesh.
 *
 * Configurable is
 *   - Horizontal mesh resolution, which is unstructured and
 *     domain-decomposed,
 *   - Vertical resolution, which is structured, and is beneficial for caching
 *   - Number of iterations, so caches can warm up, and timings can be averaged
 *   - Number of OpenMP threads per MPI task
 *
 * Results should be bit-identical when changing number of OpenMP threads or MPI tasks.
 * A checksum on all bits is used to verify between scaling runs.
 *
 *
 */

#include <limits>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <memory>

#include "eckit/exception/Exceptions.h"
#include "eckit/log/Timer.h"

#include "atlas/library/Library.h"
#include "atlas/functionspace.h"
#include "atlas/grid.h"
#include "atlas/meshgenerator.h"
#include "atlas/mesh.h"
#include "atlas/mesh/IsGhostNode.h"
#include "atlas/mesh/actions/BuildDualMesh.h"
#include "atlas/mesh/actions/BuildEdges.h"
#include "atlas/mesh/actions/BuildHalo.h"
#include "atlas/mesh/actions/BuildParallelFields.h"
#include "atlas/mesh/actions/BuildPeriodicBoundaries.h"
#include "atlas/runtime/AtlasTool.h"
#include "atlas/runtime/Trace.h"
#include "atlas/util/CoordinateEnums.h"
#include "atlas/output/Gmsh.h"
#include "atlas/parallel/Checksum.h"
#include "atlas/parallel/HaloExchange.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/parallel/omp/omp.h"

//----------------------------------------------------------------------------------------------------------------------

using std::unique_ptr;
using std::string;
using std::stringstream;
using std::min;
using std::max;
using std::vector;
using std::setw;
using std::setprecision;
using std::scientific;
using std::fixed;
using std::cout;
using std::endl;
using std::numeric_limits;

using Topology = atlas::mesh::Nodes::Topology;
using atlas::mesh::IsGhostNode;

using namespace eckit::option;
using namespace atlas;
using namespace atlas::grid;
using namespace atlas::mesh::actions;
using namespace atlas::functionspace;
using namespace atlas::meshgenerator;
using atlas::AtlasTool;

namespace {
  void usage(const std::string& tool) {
    Log::info() << "Usage: "<<tool<<" [OPTIONS]..." << std::endl;
  }

}

//----------------------------------------------------------------------------------------------------------------------

struct TimerStats
{
  TimerStats(const string& _name = "timer")
  {
    max = -1;
    min = -1;
    avg = 0;
    cnt = 0;
    name = _name;
  }
  void update(Trace& timer)
  {
    double t = timer.elapsed();
    if( min < 0 ) min = t;
    if( max < 0 ) max = t;
    min = std::min(min, t);
    max = std::max(max, t);
    avg = (avg*cnt+t)/(cnt+1);
    ++cnt;
  }
  string str()
  {
    stringstream stream;
    stream << name << ": min, max, avg -- " << fixed << setprecision(5) << min << ", " << fixed << setprecision(5) << max << ", " << fixed << setprecision(5) << avg;
    return stream.str();
  }
  string name;
  double max;
  double min;
  double avg;
  int cnt;
};

//----------------------------------------------------------------------------------------------------------------------

class AtlasBenchmark: public AtlasTool {

  virtual void execute(const Args& args);

public:

  AtlasBenchmark(int argc,char **argv): AtlasTool(argc,argv)
  {
    add_option( new SimpleOption<std::string>("grid","Grid unique identifier") );
    add_option( new SimpleOption<size_t>("nlev","Vertical resolution: Number of levels") );
    add_option( new SimpleOption<size_t>("niter","Number of iterations") );
    add_option( new SimpleOption<size_t>("omp","Number of OpenMP threads per MPI task") );
    add_option( new SimpleOption<bool>("progress","Show progress bar instead of intermediate timings") );
    add_option( new SimpleOption<bool>("output","Write output in gmsh format") );
    add_option( new SimpleOption<long>("exclude","Exclude number of iterations in statistics (default=1)") );
    add_option( new SimpleOption<bool>("details","Show detailed timers (default=false)") );
  }

  void setup();

  void iteration();

  double result();

  int verify(const double&);

private:

  Mesh mesh;
  functionspace::NodeColumns nodes_fs;
  Field scalar_field;
  Field grad_field;

  vector<int> pole_edges;
  vector<bool> is_ghost;

  size_t nnodes;
  size_t nedges;
  size_t nlev;
  size_t niter;
  size_t exclude;
  bool output;
  long omp_threads;
  double dz;
  std::string gridname;

  TimerStats iteration_timer;
  TimerStats haloexchange_timer;
  size_t iter;
  bool progress;

public:
  int exit_code;

};

//----------------------------------------------------------------------------------------------------------------------

void AtlasBenchmark::execute(const Args& args)
{
  Trace timer( Here(),"atlas-benchmark");
  // Timer::Logging set_channel( Log::info() );

  nlev = 137;
  args.get("nlev",nlev);
  gridname = "N64";
  args.get("grid",gridname);
  niter = 100;
  args.get("niter",niter);
  omp_threads = -1;
  args.get("omp",omp_threads);
  progress = false;
  args.get("progress",progress);
  exclude = niter==1?0:1;
  args.get("exclude",exclude);
  output = false;
  args.get("output",output);
  bool help(false);
  args.get("help",help);

  iteration_timer = TimerStats("iteration");
  haloexchange_timer = TimerStats("halo-exchange");

  if( omp_threads > 0 )
    omp_set_num_threads(omp_threads);

  Log::info() << "atlas-benchmark\n" << endl;
  Log::info() << Library::instance().information() << endl;
  Log::info() << "Configuration:" << endl;
  Log::info() << "  grid: " << gridname << endl;
  Log::info() << "  nlev: " << nlev << endl;
  Log::info() << "  niter: " << niter << endl;
  Log::info() << endl;
  Log::info() << "  MPI tasks: "<<parallel::mpi::comm().size()<<endl;
  Log::info() << "  OpenMP threads per MPI task: " << omp_get_max_threads() << endl;
  Log::info() << endl;


  Log::info() << "Timings:" << endl;

  ATLAS_TRACE_SCOPE("setup",{"atlas-benchmark-setup"}) { setup(); }

  Log::info() << "  Executing " << niter << " iterations: \n";
  if( progress )
  {
    Log::info() << "      0%   10   20   30   40   50   60   70   80   90   100%\n";
    Log::info() << "      |----|----|----|----|----|----|----|----|----|----|\n";
    Log::info() << "      " << std::flush;
  }
  unsigned int tic=0;
  for( iter=0; iter<niter; ++iter )
  {
    if( progress )
    {
      unsigned int tics_needed = static_cast<unsigned int>(static_cast<double>(iter)/static_cast<double>(niter-1)*50.0);
      while( tic <= tics_needed )
      {
        Log::info() << '*' << std::flush;
        ++tic;
      }
      if ( iter == niter-1 )
      {
        if ( tic < 51 ) Log::info() << '*';
          Log::info() << endl;
      }
    }
    iteration();
  }
  timer.stop();


  Log::info() << "Iteration timer Statistics:\n"
              << "  min: " << setprecision(5) << fixed << iteration_timer.min
              << "  max: " << setprecision(5) << fixed << iteration_timer.max
              << "  avg: " << setprecision(5) << fixed << iteration_timer.avg << endl;
  Log::info() << "Communication timer Statistics:\n"
              << "  min: " << setprecision(5) << fixed << haloexchange_timer.min
              << "  max: " << setprecision(5) << fixed << haloexchange_timer.max
              << "  avg: " << setprecision(5) << fixed << haloexchange_timer.avg
              << " ( "<< setprecision(2) << haloexchange_timer.avg/iteration_timer.avg*100. << "% )" << endl;

  util::Config report_config;
  report_config.set("indent",4);
  if( not args.getBool("details",false) )
    report_config.set("exclude", std::vector<std::string>{
      "halo-exchange",
      "atlas-benchmark-setup/*"
    });
  Log::info() << timer.report( report_config ) << std::endl;

  Log::info() << endl;
  Log::info() << "Results:" << endl;

  double res = result();

  Log::info() << endl;
  exit_code = verify( res );
}

//----------------------------------------------------------------------------------------------------------------------

void AtlasBenchmark::setup()
{
  size_t halo = 1;

  StructuredGrid grid;
  ATLAS_TRACE_SCOPE( "Create grid" ) {  grid = Grid(gridname); }
  ATLAS_TRACE_SCOPE( "Create mesh" ) {  mesh = MeshGenerator( "structured", util::Config("partitioner","equal_regions") ).generate(grid); }

  ATLAS_TRACE_SCOPE( "Create node_fs") { nodes_fs = functionspace::NodeColumns(mesh,option::halo(halo)); }
  ATLAS_TRACE_SCOPE( "build_edges" )                     { build_edges(mesh); }
  ATLAS_TRACE_SCOPE( "build_pole_edges" )                { build_pole_edges(mesh); }
  ATLAS_TRACE_SCOPE( "build_edges_parallel_fiels" )      { build_edges_parallel_fields(mesh); }
  ATLAS_TRACE_SCOPE( "build_median_dual_mesh" )          { build_median_dual_mesh(mesh); }
  ATLAS_TRACE_SCOPE( "build_node_to_edge_connectivity" ) { build_node_to_edge_connectivity(mesh); }

  scalar_field = nodes_fs.createField<double>( option::name("field") | option::levels(nlev) );
  grad_field   = nodes_fs.createField<double>( option::name("grad")  | option::levels(nlev) | option::variables(3) );

  nnodes = mesh.nodes().size();
  nedges = mesh.edges().size();

  auto lonlat = array::make_view<double,2> ( mesh.nodes().xy() );
  auto V      = array::make_view<double,1> ( mesh.nodes().field("dual_volumes") );
  auto S      = array::make_view<double,2> ( mesh.edges().field("dual_normals") );
  auto field  = array::make_view<double,2> ( scalar_field );

  double radius = 6371.22e+03; // Earth's radius
  double height = 80.e+03;     // Height of atmosphere
  double deg2rad = M_PI/180.;
  atlas_omp_parallel_for( size_t jnode=0; jnode<nnodes; ++jnode )
  {
    lonlat(jnode,LON) = lonlat(jnode,LON) * deg2rad;
    lonlat(jnode,LAT) = lonlat(jnode,LAT) * deg2rad;
    double y  = lonlat(jnode,LAT);
    double hx = radius*std::cos(y);
    double hy = radius;
    double G  = hx*hy;
    V(jnode) *= std::pow(deg2rad,2) * G;

    for(size_t jlev = 0; jlev < nlev; ++jlev)
      field(jnode,jlev) = 100.+50.*std::cos(2*y);
  }
  atlas_omp_parallel_for( size_t jedge=0; jedge<nedges; ++jedge )
  {
    S(jedge,LON) *= deg2rad;
    S(jedge,LAT) *= deg2rad;
  }
  dz = height/static_cast<double>(nlev);

  auto edge_is_pole   = array::make_view<int,1> ( mesh.edges().field("is_pole_edge") );
  const mesh::Connectivity& node2edge = mesh.nodes().edge_connectivity();
  const mesh::MultiBlockConnectivity& edge2node = mesh.edges().node_connectivity();
  auto node2edge_sign = array::make_view<double,2> ( mesh.nodes().add(
      Field("to_edge_sign",array::make_datatype<double>(),array::make_shape(nnodes,node2edge.maxcols()) ) ) );

  atlas_omp_parallel_for( size_t jnode=0; jnode<nnodes; ++jnode )
  {
    for(size_t jedge = 0; jedge < node2edge.cols(jnode); ++jedge)
    {
      size_t iedge = node2edge(jnode,jedge);
      size_t ip1 = edge2node(iedge,0);
      if( jnode == ip1 )
        node2edge_sign(jnode,jedge) = 1.;
      else
        node2edge_sign(jnode,jedge) = -1.;
    }
  }

  vector<int> tmp(nedges);
  int c(0);
  for(size_t jedge = 0; jedge < nedges; ++jedge)
  {
    if( edge_is_pole(jedge) )
      tmp[c++] = jedge;
  }
  pole_edges.reserve(c);
  for( int jedge=0; jedge<c; ++jedge )
    pole_edges.push_back(tmp[jedge]);

  auto flags = array::make_view<int,1>( mesh.nodes().field("flags") );
  is_ghost.reserve(nnodes);
  for(size_t jnode = 0; jnode < nnodes; ++jnode)
  {
    is_ghost.push_back( Topology::check(flags(jnode),Topology::GHOST) );
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AtlasBenchmark::iteration()
{
  Trace t( Here() );
  Trace compute( Here(), "compute" );
  unique_ptr<array::Array> avgS_arr( array::Array::create<double>(nedges,nlev,2ul) );
  const auto& node2edge = mesh.nodes().edge_connectivity();
  const auto& edge2node = mesh.edges().node_connectivity();
  const auto field = array::make_view<double,2>( scalar_field );
  const auto S     = array::make_view<double,2>( mesh.edges().field("dual_normals"));
  const auto V     = array::make_view<double,1>( mesh.nodes().field("dual_volumes"));
  const auto node2edge_sign = array::make_view<double,2> ( mesh.nodes().field("to_edge_sign") );

  auto grad = array::make_view<double,3>( grad_field );
  auto avgS = array::make_view<double,3>( *avgS_arr );

  atlas_omp_parallel_for( size_t jedge=0; jedge<nedges; ++jedge )
  {
    int ip1 = edge2node(jedge,0);
    int ip2 = edge2node(jedge,1);

    for(size_t jlev = 0; jlev < nlev; ++jlev)
    {
      double avg = ( field(ip1,jlev) + field(ip2,jlev) ) * 0.5;
      avgS(jedge,jlev,LON) = S(jedge,LON)*avg;
      avgS(jedge,jlev,LAT) = S(jedge,LAT)*avg;
    }
  }

  atlas_omp_parallel_for( size_t jnode=0; jnode<nnodes; ++jnode )
  {
    for(size_t jlev = 0; jlev < nlev; ++jlev )
    {
      grad(jnode,jlev,LON) = 0.;
      grad(jnode,jlev,LAT) = 0.;
    }
    for( size_t jedge=0; jedge<node2edge.cols(jnode); ++jedge )
    {
      size_t iedge = node2edge(jnode,jedge);
      double add = node2edge_sign(jnode,jedge);
      for(size_t jlev = 0; jlev < nlev; ++jlev)
      {
        grad(jnode,jlev,LON) += add*avgS(iedge,jlev,LON);
        grad(jnode,jlev,LAT) += add*avgS(iedge,jlev,LAT);
      }
    }
    for(size_t jlev = 0; jlev < nlev; ++jlev)
    {
      grad(jnode,jlev,LON) /= V(jnode);
      grad(jnode,jlev,LAT) /= V(jnode);
    }
  }
  // special treatment for the north & south pole cell faces
  // Sx == 0 at pole, and Sy has same sign at both sides of pole
  for(size_t jedge = 0; jedge < pole_edges.size(); ++jedge)
  {
    int iedge = pole_edges[jedge];
    int ip2 = edge2node(iedge,1);
    // correct for wrong Y-derivatives in previous loop
    for(size_t jlev = 0; jlev < nlev; ++jlev)
      grad(ip2,jlev,LAT) += 2.*avgS(iedge,jlev,LAT)/V(ip2);
  }

  double dzi = 1./dz;
  double dzi_2 = 0.5*dzi;

  atlas_omp_parallel_for( size_t jnode=0; jnode<nnodes; ++jnode )
  {
    if( nlev > 2 )
    {
      for(size_t jlev = 1; jlev < nlev - 1; ++jlev)
      {
        grad(jnode,jlev,ZZ)   = (field(jnode,jlev+1)     - field(jnode,jlev-1))*dzi_2;
      }
    }
    if( nlev > 1 )
    {
      grad(jnode,  0   ,ZZ) = (field(jnode,  1   ) - field(jnode,  0   ))*dzi;
      grad(jnode,nlev-1,ZZ) = (field(jnode,nlev-2) - field(jnode,nlev-1))*dzi;
    }
    if( nlev == 1 )
      grad(jnode,0,ZZ) = 0.;
  }
  compute.stop();

  // halo-exchange
  Trace halo( Here(), "halo-exchange");
  nodes_fs.halo_exchange().execute(grad);
  halo.stop();

  t.stop();

  if( iter >= exclude )
  {
    haloexchange_timer.update(halo);
    iteration_timer.update(t);
  }

  if( !progress )
  {
    Log::info() << setw(6) << iter+1
                << "    total: " << fixed << setprecision(5) << t.elapsed()
                << "    communication: " << setprecision(5) << halo.elapsed()
                << " ( "<< setprecision(2) << fixed << setw(3)
                << halo.elapsed()/t.elapsed()*100 << "% )" << endl;
  }
}

//----------------------------------------------------------------------------------------------------------------------

template< typename DATA_TYPE >
DATA_TYPE vecnorm( DATA_TYPE vec[], size_t size )
{
  DATA_TYPE norm=0;
  for(size_t j=0; j < size; ++j)
    norm += std::pow(vec[j],2);
  return std::sqrt(norm);
}

double AtlasBenchmark::result()
{
  const auto grad = array::make_view<double,3>( grad_field );
  double maxval = numeric_limits<double>::min();
  double minval = numeric_limits<double>::max();;
  double norm = 0.;
  for(size_t jnode = 0; jnode < nnodes; ++jnode)
  {
    if( !is_ghost[jnode] )
    {
      for(size_t jlev = 0; jlev < nlev; ++jlev)
      {
        maxval = max(maxval,grad(jnode,jlev,LON));
        maxval = max(maxval,grad(jnode,jlev,LAT));
        maxval = max(maxval,grad(jnode,jlev,ZZ));
        minval = min(minval,grad(jnode,jlev,LON));
        minval = min(minval,grad(jnode,jlev,LAT));
        minval = min(minval,grad(jnode,jlev,ZZ));
//        norm += std::pow(vecnorm(grad[jnode][jlev].data(),3),2);
        norm += std::pow(vecnorm(grad.at(jnode).at(jlev).data(),3),2);
      }
    }
  }
  ATLAS_TRACE_MPI( ALLREDUCE ) {
    parallel::mpi::comm().allReduceInPlace(maxval, eckit::mpi::max());
    parallel::mpi::comm().allReduceInPlace(minval, eckit::mpi::min());
    parallel::mpi::comm().allReduceInPlace(norm,   eckit::mpi::sum());
  }

  norm = std::sqrt(norm);

  Log::info() << "  checksum: " << nodes_fs.checksum().execute( grad ) << endl;
  Log::info() << "  maxval: " << setw(13) << setprecision(6) << scientific << maxval << endl;
  Log::info() << "  minval: " << setw(13) << setprecision(6) << scientific << minval << endl;
  Log::info() << "  norm:   " << setw(13) << setprecision(6) << scientific << norm << endl;

  if( output )
  {
    std::vector<long> levels( 1, 0 );
    atlas::output::Output gmsh = atlas::output::Gmsh( "benchmark.msh", util::Config("levels",levels) );
    gmsh.write( mesh );
    gmsh.write( mesh.nodes().field("field") );
    gmsh.write( mesh.nodes().field("grad") );
  }
  return norm;
}

int AtlasBenchmark::verify(const double& norm)
{
  if( nlev != 137 )
  {
    Log::warning() << "Results cannot be verified with nlev != 137" << endl;
    return 1;
  }
  std::map<std::string,double> norms;
  norms[  "N16"] = 1.473937e-09;
  norms[  "N24"] = 2.090045e-09;
  norms[  "N32"] = 2.736576e-09;
  norms[  "N48"] = 3.980306e-09;
  norms[  "N64"] = 5.219642e-09;
  norms[  "N80"] = 6.451913e-09;
  norms[  "N96"] = 7.647690e-09;
  norms[ "N128"] = 1.009042e-08;
  norms[ "N160"] = 1.254571e-08;
  norms[ "N200"] = 1.557589e-08;
  norms[ "N256"] = 1.983944e-08;
  norms[ "N320"] = 2.469347e-08;
  norms[ "N400"] = 3.076775e-08;
  norms[ "N512"] = 3.924470e-08;
  norms[ "N576"] = 4.409003e-08;
  norms[ "N640"] = 4.894316e-08;
  norms[ "N800"] = 6.104009e-08;
  norms["N1024"] = 7.796900e-08;
  norms["N1280"] = 9.733947e-08;
  norms["N1600"] = 1.215222e-07;
  norms["N2000"] = 1.517164e-07;
  norms["N4000"] = 2.939562e-07;

  if( norms.count(gridname) == 0 )
  {
    Log::warning() << "Results cannot be verified with grid "<< gridname << '\n';
    Log::warning() << "Valid grids: \n";
    for( std::map<std::string,double>::const_iterator it=norms.begin(); it!=norms.end(); ++it)
    {
      Log::warning() << "    -  " << it->first << "\n";
    }
    Log::warning() << std::flush;

    return 1;
  }

  double diff = (norm-norms[gridname])/norms[gridname];
  if( diff < 0.01 )
  {
    Log::info() << "Results are verified and correct.\n  difference = " << setprecision(6) << fixed << diff*100 << "%" << endl;
    return 0;
  }

  Log::info() << "Results are wrong.\n  difference = " << setprecision(6) << fixed << diff*100 << "%" << endl;
  return 1;
}

//----------------------------------------------------------------------------------------------------------------------

int main( int argc, char **argv )
{
  AtlasBenchmark tool(argc,argv);
  return tool.start();
}
