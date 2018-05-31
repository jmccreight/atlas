/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */


#include <cmath>
#include <iostream>
#include <stdexcept>

#include "eckit/exception/Exceptions.h"

#include "atlas/array.h"
#include "atlas/array/ArrayView.h"
#include "atlas/array/IndexView.h"
#include "atlas/field/Field.h"
#include "atlas/mesh/HybridElements.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/mesh/Nodes.h"
#include "atlas/mesh/actions/BuildParallelFields.h"
#include "atlas/mesh/detail/PeriodicTransform.h"
#include "atlas/parallel/GatherScatter.h"
#include "atlas/parallel/mpi/Buffer.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/runtime/ErrorHandling.h"
#include "atlas/runtime/Log.h"
#include "atlas/runtime/Trace.h"
#include "atlas/util/CoordinateEnums.h"
#include "atlas/util/Unique.h"

#define EDGE( jedge )                                                                                     \
    "Edge(" << node_gidx( edge_nodes( jedge, 0 ) ) << "[p" << node_part( edge_nodes( jedge, 0 ) ) << "] " \
            << node_gidx( edge_nodes( jedge, 1 ) ) << "[p" << node_part( edge_nodes( jedge, 1 ) ) << "])"

#ifdef DEBUGGING_PARFIELDS
#define own1 2419089
#define own2 2423185
#define OWNED_EDGE( jedge )                                                                   \
    ( ( gidx( edge_nodes( jedge, 0 ) ) == own1 && gidx( edge_nodes( jedge, 1 ) ) == own2 ) || \
      ( gidx( edge_nodes( jedge, 0 ) ) == own2 && gidx( edge_nodes( jedge, 1 ) ) == own1 ) )
#define per1 -1
#define per2 -1
#define PERIODIC_EDGE( jedge )                                                                \
    ( ( gidx( edge_nodes( jedge, 0 ) ) == per1 && gidx( edge_nodes( jedge, 1 ) ) == per2 ) || \
      ( gidx( edge_nodes( jedge, 0 ) ) == per2 && gidx( edge_nodes( jedge, 1 ) ) == per1 ) )
#define find1 -12
#define find2 -17
#define FIND_EDGE( jedge )                                                                      \
    ( ( gidx( edge_nodes( jedge, 0 ) ) == find1 && gidx( edge_nodes( jedge, 1 ) ) == find2 ) || \
      ( gidx( edge_nodes( jedge, 0 ) ) == find2 && gidx( edge_nodes( jedge, 1 ) ) == find1 ) )
#define ownuid 547124520
#define OWNED_UID( UID ) ( UID == ownuid )
#endif

using Topology = atlas::mesh::Nodes::Topology;
using atlas::mesh::detail::PeriodicTransform;
using atlas::util::UniqueLonLat;

namespace atlas {
namespace mesh {
namespace actions {

Field& build_nodes_partition( mesh::Nodes& nodes );
Field& build_nodes_remote_idx( mesh::Nodes& nodes );
Field& build_nodes_global_idx( mesh::Nodes& nodes );
Field& build_edges_partition( Mesh& mesh );
Field& build_edges_remote_idx( Mesh& mesh );
Field& build_edges_global_idx( Mesh& mesh );

//----------------------------------------------------------------------------------------------------------------------

typedef gidx_t uid_t;

namespace {

struct Node {
    Node( gidx_t gid, int idx ) {
        g = gid;
        i = idx;
    }
    gidx_t g;
    gidx_t i;
    bool operator<( const Node& other ) const { return ( g < other.g ); }
};

}  // namespace

//----------------------------------------------------------------------------------------------------------------------

void build_parallel_fields( Mesh& mesh ) {
    ATLAS_TRACE();
    build_nodes_parallel_fields( mesh.nodes() );
}

//----------------------------------------------------------------------------------------------------------------------

void build_nodes_parallel_fields( mesh::Nodes& nodes ) {
    ATLAS_TRACE();
    bool parallel = false;
    nodes.metadata().get( "parallel", parallel );
    if ( !parallel ) {
        build_nodes_partition( nodes );
        build_nodes_remote_idx( nodes );
        build_nodes_global_idx( nodes );
    }
    nodes.metadata().set( "parallel", true );
}

//----------------------------------------------------------------------------------------------------------------------

void build_edges_parallel_fields( Mesh& mesh ) {
    ATLAS_TRACE();
    build_edges_partition( mesh );
    build_edges_remote_idx( mesh );
    /*
 * We turn following off. It is expensive and we don't really care about a nice
 * contiguous
 * ordering.
 */
    // build_edges_global_idx( mesh );
}

//----------------------------------------------------------------------------------------------------------------------

Field& build_nodes_global_idx( mesh::Nodes& nodes ) {
    ATLAS_TRACE();

    array::ArrayView<gidx_t, 1> glb_idx = array::make_view<gidx_t, 1>( nodes.global_index() );

    UniqueLonLat compute_uid( nodes );

    for ( size_t jnode = 0; jnode < glb_idx.shape( 0 ); ++jnode ) {
        if ( glb_idx( jnode ) <= 0 ) { glb_idx( jnode ) = compute_uid( jnode ); }
    }
    return nodes.global_index();
}

void renumber_nodes_glb_idx( mesh::Nodes& nodes ) {
    bool human_readable( false );
    nodes.global_index().metadata().get( "human_readable", human_readable );
    if ( human_readable ) {
        /* nothing to be done */
        return;
    }

    ATLAS_TRACE();

    // TODO: ATLAS-14: fix renumbering of EAST periodic boundary points
    // --> Those specific periodic points at the EAST boundary are not checked for
    // uid,
    //     and could receive different gidx for different tasks

    UniqueLonLat compute_uid( nodes );

    // unused // int mypart = mpi::comm().rank();
    int nparts  = mpi::comm().size();
    size_t root = 0;

    array::ArrayView<gidx_t, 1> glb_idx = array::make_view<gidx_t, 1>( nodes.global_index() );

    /*
 * Sorting following gidx will define global order of
 * gathered fields. Special care needs to be taken for
 * pole edges, as their centroid might coincide with
 * other edges
 */
    int nb_nodes = glb_idx.shape( 0 );
    for ( int jnode = 0; jnode < nb_nodes; ++jnode ) {
        if ( glb_idx( jnode ) <= 0 ) { glb_idx( jnode ) = compute_uid( jnode ); }
    }

    // 1) Gather all global indices, together with location
    array::ArrayT<uid_t> loc_id_arr( nb_nodes );
    array::ArrayView<uid_t, 1> loc_id = array::make_view<uid_t, 1>( loc_id_arr );

    for ( int jnode = 0; jnode < nb_nodes; ++jnode ) {
        loc_id( jnode ) = glb_idx( jnode );
    }

    std::vector<int> recvcounts( mpi::comm().size() );
    std::vector<int> recvdispls( mpi::comm().size() );

    ATLAS_TRACE_MPI( GATHER ) { mpi::comm().gather( nb_nodes, recvcounts, root ); }

    recvdispls[0] = 0;
    for ( int jpart = 1; jpart < nparts; ++jpart ) {  // start at 1
        recvdispls[jpart] = recvcounts[jpart - 1] + recvdispls[jpart - 1];
    }
    int glb_nb_nodes = std::accumulate( recvcounts.begin(), recvcounts.end(), 0 );

    array::ArrayT<uid_t> glb_id_arr( glb_nb_nodes );
    array::ArrayView<uid_t, 1> glb_id = array::make_view<uid_t, 1>( glb_id_arr );

    ATLAS_TRACE_MPI( GATHER ) {
        mpi::comm().gatherv( loc_id.data(), loc_id.size(), glb_id.data(), recvcounts.data(), recvdispls.data(), root );
    }

    // 2) Sort all global indices, and renumber from 1 to glb_nb_edges
    std::vector<Node> node_sort;
    node_sort.reserve( glb_nb_nodes );
    ATLAS_TRACE_SCOPE( "sort global indices" ) {
        for ( size_t jnode = 0; jnode < glb_id.shape( 0 ); ++jnode ) {
            node_sort.push_back( Node( glb_id( jnode ), jnode ) );
        }
        std::sort( node_sort.begin(), node_sort.end() );
    }

    // Assume edge gid start
    uid_t gid = 0;
    for ( size_t jnode = 0; jnode < node_sort.size(); ++jnode ) {
        if ( jnode == 0 ) { ++gid; }
        else if ( node_sort[jnode].g != node_sort[jnode - 1].g ) {
            ++gid;
        }
        int inode       = node_sort[jnode].i;
        glb_id( inode ) = gid;
    }

    // 3) Scatter renumbered back
    ATLAS_TRACE_MPI( SCATTER ) {
        mpi::comm().scatterv( glb_id.data(), recvcounts.data(), recvdispls.data(), loc_id.data(), loc_id.size(), root );
    }

    for ( int jnode = 0; jnode < nb_nodes; ++jnode ) {
        glb_idx( jnode ) = loc_id( jnode );
    }
    nodes.global_index().metadata().set( "human_readable", true );
}

//----------------------------------------------------------------------------------------------------------------------

Field& build_nodes_remote_idx( mesh::Nodes& nodes ) {
    ATLAS_TRACE();
    size_t mypart = mpi::comm().rank();
    size_t nparts = mpi::comm().size();

    UniqueLonLat compute_uid( nodes );

    // This piece should be somewhere central ... could be NPROMA ?
    // ---------->
    std::vector<int> proc( nparts );
    for ( size_t jpart = 0; jpart < nparts; ++jpart )
        proc[jpart] = jpart;
    // <---------

    auto ridx       = array::make_indexview<int, 1>( nodes.remote_index() );
    auto part       = array::make_view<int, 1>( nodes.partition() );
    auto gidx       = array::make_view<gidx_t, 1>( nodes.global_index() );
    size_t nb_nodes = nodes.size();

    int varsize = 2;

    std::vector<std::vector<uid_t>> send_needed( mpi::comm().size() );
    std::vector<std::vector<uid_t>> recv_needed( mpi::comm().size() );
    int sendcnt = 0;
    std::map<uid_t, int> lookup;
    for ( size_t jnode = 0; jnode < nb_nodes; ++jnode ) {
        uid_t uid = compute_uid( jnode );

        if ( size_t( part( jnode ) ) == mypart ) {
            lookup[uid]   = jnode;
            ridx( jnode ) = jnode;
        }
        else {
            ASSERT( jnode < part.shape( 0 ) );
            if ( part( jnode ) >= (int)proc.size() ) {
                std::stringstream msg;
                msg << "Assertion [part(" << jnode << ") < proc.size()] failed\n"
                    << "part(" << jnode << ") = " << part( jnode ) << "\n"
                    << "proc.size() = " << proc.size();
                eckit::AssertionFailed( msg.str(), Here() );
            }
            ASSERT( part( jnode ) < (int)proc.size() );
            ASSERT( (size_t)proc[part( jnode )] < send_needed.size() );
            send_needed[proc[part( jnode )]].push_back( uid );
            send_needed[proc[part( jnode )]].push_back( jnode );
            sendcnt++;
        }
    }

    ATLAS_TRACE_MPI( ALLTOALL ) { mpi::comm().allToAll( send_needed, recv_needed ); }

    std::vector<std::vector<int>> send_found( mpi::comm().size() );
    std::vector<std::vector<int>> recv_found( mpi::comm().size() );

    for ( size_t jpart = 0; jpart < nparts; ++jpart ) {
        const std::vector<uid_t>& recv_node = recv_needed[proc[jpart]];
        const size_t nb_recv_nodes          = recv_node.size() / varsize;
        // array::ArrayView<uid_t,2> recv_node( make_view( Array::wrap(shape,
        // recv_needed[ proc[jpart] ].data()) ),
        //     array::make_shape(recv_needed[ proc[jpart] ].size()/varsize,varsize)
        //     );
        for ( size_t jnode = 0; jnode < nb_recv_nodes; ++jnode ) {
            uid_t uid = recv_node[jnode * varsize + 0];
            int inode = recv_node[jnode * varsize + 1];
            if ( lookup.count( uid ) ) {
                send_found[proc[jpart]].push_back( inode );
                send_found[proc[jpart]].push_back( lookup[uid] );
            }
            else {
                std::stringstream msg;
                msg << "[" << mpi::comm().rank() << "] "
                    << "Node requested by rank [" << jpart << "] with uid [" << uid
                    << "] that should be owned is not found";
                throw eckit::SeriousBug( msg.str(), Here() );
            }
        }
    }

    ATLAS_TRACE_MPI( ALLTOALL ) { mpi::comm().allToAll( send_found, recv_found ); }

    for ( size_t jpart = 0; jpart < nparts; ++jpart ) {
        const std::vector<int>& recv_node = recv_found[proc[jpart]];
        const size_t nb_recv_nodes        = recv_node.size() / 2;
        // array::ArrayView<int,2> recv_node( recv_found[ proc[jpart] ].data(),
        //     array::make_shape(recv_found[ proc[jpart] ].size()/2,2) );
        for ( size_t jnode = 0; jnode < nb_recv_nodes; ++jnode ) {
            ridx( recv_node[jnode * 2 + 0] ) = recv_node[jnode * 2 + 1];
        }
    }
    return nodes.remote_index();
}

//----------------------------------------------------------------------------------------------------------------------

Field& build_nodes_partition( mesh::Nodes& nodes ) {
    ATLAS_TRACE();
    return nodes.partition();
}

//----------------------------------------------------------------------------------------------------------------------

Field& build_edges_partition( Mesh& mesh ) {
    ATLAS_TRACE();

    const mesh::Nodes& nodes = mesh.nodes();

    UniqueLonLat compute_uid( mesh );

    size_t mypart = mpi::comm().rank();
    size_t nparts = mpi::comm().size();

    mesh::HybridElements& edges              = mesh.edges();
    array::ArrayView<int, 1> edge_part       = array::make_view<int, 1>( edges.partition() );
    array::ArrayView<gidx_t, 1> edge_glb_idx = array::make_view<gidx_t, 1>( edges.global_index() );

    const mesh::HybridElements::Connectivity& edge_nodes   = edges.node_connectivity();
    const mesh::HybridElements::Connectivity& edge_to_elem = edges.cell_connectivity();

    array::ArrayView<int, 1> node_part    = array::make_view<int, 1>( nodes.partition() );
    array::ArrayView<double, 2> xy        = array::make_view<double, 2>( nodes.xy() );
    array::ArrayView<int, 1> flags        = array::make_view<int, 1>( nodes.field( "flags" ) );
    array::ArrayView<gidx_t, 1> node_gidx = array::make_view<gidx_t, 1>( nodes.global_index() );

    array::ArrayView<int, 1> elem_part = array::make_view<int, 1>( mesh.cells().partition() );
    array::ArrayView<int, 1> elem_halo = array::make_view<int, 1>( mesh.cells().halo() );

    auto check_flags = [&]( idx_t jedge, int flag ) {
        idx_t ip1 = edge_nodes( jedge, 0 );
        idx_t ip2 = edge_nodes( jedge, 1 );
        return Topology::check( flags( ip1 ), flag ) && Topology::check( flags( ip2 ), flag );
    };
    auto domain_bdry = [&]( idx_t jedge ) {
        if ( check_flags( jedge, Topology::BC | Topology::NORTH ) ) { return true; }
        if ( check_flags( jedge, Topology::BC | Topology::SOUTH ) ) { return true; }
        return false;
    };

    PeriodicTransform transform;

    size_t nb_edges = edges.size();

    std::vector<int> periodic( nb_edges );

    std::vector<gidx_t> bdry_edges;
    bdry_edges.reserve( nb_edges );
    std::map<gidx_t, size_t> global_to_local;

    for ( size_t jedge = 0; jedge < nb_edges; ++jedge ) {
        global_to_local[edge_glb_idx( jedge )] = jedge;

        periodic[jedge] = 0;
        idx_t ip1       = edge_nodes( jedge, 0 );
        idx_t ip2       = edge_nodes( jedge, 1 );
        int pn1         = node_part( ip1 );
        int pn2         = node_part( ip2 );
        int y1          = util::microdeg( xy( ip1, YY ) );
        int y2          = util::microdeg( xy( ip2, YY ) );
        int p;
        if ( y1 == y2 ) {
            int x1 = util::microdeg( xy( ip1, XX ) );
            int x2 = util::microdeg( xy( ip2, XX ) );
            p      = ( x1 < x2 ) ? pn1 : pn2;
        }
        else {
            p = ( y1 > y2 ) ? pn1 : pn2;
        }

        idx_t elem1 = edge_to_elem( jedge, 0 );
        idx_t elem2 = edge_to_elem( jedge, 1 );
        if ( elem1 == edge_to_elem.missing_value() ) {
            bdry_edges.push_back( edge_glb_idx( jedge ) );
            p = pn1;
            // Log::error() << EDGE(jedge) << " is a pole edge with part " << p <<
            // std::endl;
        }
        else if ( elem2 == edge_to_elem.missing_value() ) {
            // if( not domain_bdry(jedge) ) {
            bdry_edges.push_back( edge_glb_idx( jedge ) );
            p = elem_part( elem1 );
            if( pn1 != p && pn2 == pn1 && elem_halo( elem1 ) > 0 ) {
                p = pn1;
            }
            // }
        }
        else if ( p != elem_part( elem1 ) && p != elem_part( elem2 ) ) {
            p = ( p == pn1 ) ? pn2 : pn1;

            if ( p != elem_part( elem1 ) and p != elem_part( elem2 ) ) {
                std::stringstream msg;
                msg << "[" << eckit::mpi::comm().rank() << "] " << EDGE( jedge )
                    << " has nodes and elements of different rank: elem1[p" << elem_part( elem1 ) << "] elem2[p"
                    << elem_part( elem2 ) << "]";
                throw eckit::SeriousBug( msg.str(), Here() );
            }
        }
        edge_part( jedge ) = p;
    }
    std::sort( bdry_edges.begin(), bdry_edges.end() );
    auto is_bdry_edge = [&bdry_edges]( gidx_t gid ) {
        std::vector<uid_t>::iterator it = std::lower_bound( bdry_edges.begin(), bdry_edges.end(), gid );
        bool found                      = !( it == bdry_edges.end() || gid < *it );
        return found;
    };

    int mpi_size = eckit::mpi::comm().size();
    mpi::Buffer<gidx_t, 1> recv_bdry_edges_from_parts( mpi_size );
    std::vector<std::vector<gidx_t>> send_gidx( mpi_size );
    std::vector<std::vector<int>> send_part( mpi_size );
    std::vector<std::vector<gidx_t>> recv_gidx( mpi_size );
    std::vector<std::vector<int>> recv_part( mpi_size );
    mpi::comm().allGatherv( bdry_edges.begin(), bdry_edges.end(), recv_bdry_edges_from_parts );
    for ( int p = 0; p < mpi_size; ++p ) {
        auto view = recv_bdry_edges_from_parts[p];
        for ( int j = 0; j < view.size(); ++j ) {
            gidx_t gidx = view[j];
            if ( global_to_local.count( gidx ) ) {
                if ( not is_bdry_edge( gidx ) ) {
                    int iedge = global_to_local[gidx];
                    send_gidx[p].push_back( gidx );
                    send_part[p].push_back( edge_part( iedge ) );
                }
            }
        }
    }
    mpi::comm().allToAll( send_gidx, recv_gidx );
    mpi::comm().allToAll( send_part, recv_part );
    for ( int p = 0; p < mpi_size; ++p ) {
        const auto& recv_gidx_p = recv_gidx[p];
        const auto& recv_part_p = recv_part[p];
        for ( int j = 0; j < recv_gidx_p.size(); ++j ) {
            idx_t iedge        = global_to_local[recv_gidx_p[j]];
            int prev           = edge_part( iedge );
            edge_part( iedge ) = recv_part_p[j];
            // if( edge_part(iedge) != prev )
            //   Log::error() << EDGE(iedge) << " part " << prev << " --> " <<
            //   edge_part(iedge) << std::endl;
        }
    }

    // Sanity check
    std::shared_ptr<array::ArrayView<int, 1>> is_pole_edge;
    bool has_pole_edges = false;
    if ( edges.has_field( "is_pole_edge" ) ) {
        has_pole_edges = true;
        is_pole_edge   = std::shared_ptr<array::ArrayView<int, 1>>(
            new array::ArrayView<int, 1>( array::make_view<int, 1>( edges.field( "is_pole_edge" ) ) ) );
    }
    int insane = 0;
    for ( size_t jedge = 0; jedge < nb_edges; ++jedge ) {
        idx_t ip1   = edge_nodes( jedge, 0 );
        idx_t ip2   = edge_nodes( jedge, 1 );
        idx_t elem1 = edge_to_elem( jedge, 0 );
        idx_t elem2 = edge_to_elem( jedge, 1 );
        int p       = edge_part( jedge );
        int pn1     = node_part( ip1 );
        int pn2     = node_part( ip2 );
        if ( has_pole_edges && ( *is_pole_edge )( jedge ) ) {
            if ( p != pn1 || p != pn2 ) {
                Log::error() << "pole edge " << EDGE( jedge ) << " [p" << p << "] is not correct" << std::endl;
                insane = 1;
            }
        }
        else {
            if ( elem1 == edge_to_elem.missing_value() && elem2 == edge_to_elem.missing_value() ) {
                Log::error() << EDGE( jedge ) << " has no neighbouring elements" << std::endl;
                insane = 1;
            }
        }
        bool edge_is_partition_boundary =
            ( elem1 == edge_to_elem.missing_value() || elem2 == edge_to_elem.missing_value() );
        bool edge_partition_is_same_as_one_of_nodes = ( p == pn1 || p == pn2 );
        if ( edge_is_partition_boundary ) {
            if ( not edge_partition_is_same_as_one_of_nodes ) {
                if ( elem1 != edge_to_elem.missing_value() ) {
                    Log::error() << "[" << mypart << "] " << EDGE( jedge ) << " [p" << p << "] is not correct elem1[p" << elem_part( elem1 )
                                 << "]" << std::endl;
                }
                else {
                    Log::error() << "[" << mypart << "] " << EDGE( jedge ) << " [p" << p << "] is not correct elem2[p" << elem_part( elem2 )
                                 << "]" << std::endl;
                }
                insane = 1;
            }
        }
        else {
            int pe1                                     = elem_part( elem1 );
            int pe2                                     = elem_part( elem2 );
            bool edge_partition_is_same_as_one_of_elems = ( p == pe1 || p == pe2 );
            if ( not edge_partition_is_same_as_one_of_elems and not edge_partition_is_same_as_one_of_nodes ) {
                Log::error() << EDGE( jedge ) << " is not correct elem1[p" << pe1 << "] elem2[p" << pe2 << "]"
                             << std::endl;
                insane = 1;
            }
        }
    }
    mpi::comm().allReduceInPlace( insane, eckit::mpi::max() );
    if ( insane && eckit::mpi::comm().rank() == 0 ) throw eckit::Exception( "Sanity check failed", Here() );

    //#ifdef DEBUGGING_PARFIELDS
    //        if( OWNED_EDGE(jedge) )
    //          DEBUG( EDGE(jedge) << " -->    part " << edge_part(jedge));
    //#endif

    //#ifdef DEBUGGING_PARFIELDS_disable
    //    if( PERIODIC_EDGE(jedge) )
    //      DEBUG_VAR( "           the part is " << edge_part(jedge) );
    //#endif
    //  }

    return edges.partition();
}

Field& build_edges_remote_idx( Mesh& mesh ) {
    ATLAS_TRACE();

    const mesh::Nodes& nodes = mesh.nodes();
    UniqueLonLat compute_uid( mesh );

    size_t mypart = mpi::comm().rank();
    size_t nparts = mpi::comm().size();

    mesh::HybridElements& edges = mesh.edges();

    array::IndexView<int, 1> edge_ridx = array::make_indexview<int, 1>( edges.remote_index() );

    const array::ArrayView<int, 1> edge_part             = array::make_view<int, 1>( edges.partition() );
    const mesh::HybridElements::Connectivity& edge_nodes = edges.node_connectivity();

    array::ArrayView<double, 2> xy = array::make_view<double, 2>( nodes.xy() );
    array::ArrayView<int, 1> flags = array::make_view<int, 1>( nodes.field( "flags" ) );
#ifdef DEBUGGING_PARFIELDS
    array::ArrayView<gidx_t, 1> node_gidx = array::make_view<gidx_t, 1>( nodes.global_index() );
    array::ArrayView<int, 1> node_part    = array::make_view<int, 1>( nodes.partition() );
#endif

    std::shared_ptr<array::ArrayView<int, 1>> is_pole_edge;
    bool has_pole_edges = false;
    if ( edges.has_field( "is_pole_edge" ) ) {
        has_pole_edges = true;
        is_pole_edge   = std::shared_ptr<array::ArrayView<int, 1>>(
            new array::ArrayView<int, 1>( array::make_view<int, 1>( edges.field( "is_pole_edge" ) ) ) );
    }

    const int nb_edges = edges.size();

    double centroid[2];
    std::vector<std::vector<uid_t>> send_needed( mpi::comm().size() );
    std::vector<std::vector<uid_t>> recv_needed( mpi::comm().size() );
    int sendcnt = 0;
    std::map<uid_t, int> lookup;

    PeriodicTransform transform;

    for ( int jedge = 0; jedge < nb_edges; ++jedge ) {
        int ip1      = edge_nodes( jedge, 0 );
        int ip2      = edge_nodes( jedge, 1 );
        centroid[XX] = 0.5 * ( xy( ip1, XX ) + xy( ip2, XX ) );
        centroid[YY] = 0.5 * ( xy( ip1, YY ) + xy( ip2, YY ) );
        if ( has_pole_edges && ( *is_pole_edge )( jedge ) ) { centroid[YY] = centroid[YY] > 0 ? 90. : -90.; }

        bool needed( false );

        if ( ( Topology::check( flags( ip1 ), Topology::PERIODIC ) &&
               !Topology::check( flags( ip1 ), Topology::BC | Topology::WEST ) &&
               Topology::check( flags( ip2 ), Topology::PERIODIC ) &&
               !Topology::check( flags( ip2 ), Topology::BC | Topology::WEST ) ) ||
             ( Topology::check( flags( ip1 ), Topology::PERIODIC ) &&
               !Topology::check( flags( ip1 ), Topology::BC | Topology::WEST ) &&
               Topology::check( flags( ip2 ), Topology::BC | Topology::WEST ) ) ||
             ( Topology::check( flags( ip1 ), Topology::BC | Topology::WEST ) &&
               Topology::check( flags( ip2 ), Topology::PERIODIC ) &&
               !Topology::check( flags( ip2 ), Topology::BC | Topology::WEST ) ) ) {
            needed = true;
            if ( Topology::check( flags( ip1 ), Topology::EAST ) )
                transform( centroid, -1 );
            else
                transform( centroid, +1 );
        }

        uid_t uid = util::unique_lonlat( centroid );
        if ( size_t( edge_part( jedge ) ) == mypart && !needed )  // All interior edges fall here
        {
            lookup[uid]        = jedge;
            edge_ridx( jedge ) = jedge;

#ifdef DEBUGGING_PARFIELDS
            if ( FIND_EDGE( jedge ) ) { DEBUG( "Found " << EDGE( jedge ) ); }
#endif
        }
        else  // All ghost edges PLUS the periodic edges identified edges above
              // fall here
        {
            send_needed[edge_part( jedge )].push_back( uid );
            send_needed[edge_part( jedge )].push_back( jedge );
#ifdef DEBUGGING_PARFIELDS
            send_needed[edge_part( jedge )].push_back( node_gidx( ip1 ) );
            send_needed[edge_part( jedge )].push_back( node_gidx( ip2 ) );
            send_needed[edge_part( jedge )].push_back( node_part( ip1 ) );
            send_needed[edge_part( jedge )].push_back( node_part( ip2 ) );
#endif
            sendcnt++;
        }
    }

    int varsize = 2;
#ifdef DEBUGGING_PARFIELDS
    varsize = 6;
#endif

    ATLAS_TRACE_MPI( ALLTOALL ) { mpi::comm().allToAll( send_needed, recv_needed ); }

    std::vector<std::vector<int>> send_found( mpi::comm().size() );
    std::vector<std::vector<int>> recv_found( mpi::comm().size() );

    std::map<uid_t, int>::iterator found;
    for ( size_t jpart = 0; jpart < nparts; ++jpart ) {
        const std::vector<uid_t>& recv_edge = recv_needed[jpart];
        const size_t nb_recv_edges          = recv_edge.size() / varsize;
        // array::ArrayView<uid_t,2> recv_edge( recv_needed[ jpart ].data(),
        //     array::make_shape(recv_needed[ jpart ].size()/varsize,varsize) );
        for ( size_t jedge = 0; jedge < nb_recv_edges; ++jedge ) {
            uid_t recv_uid = recv_edge[jedge * varsize + 0];
            int recv_idx   = recv_edge[jedge * varsize + 1];
            found          = lookup.find( recv_uid );
            if ( found != lookup.end() ) {
                send_found[jpart].push_back( recv_idx );
                send_found[jpart].push_back( found->second );
            }
            else {
                std::stringstream msg;
#ifdef DEBUGGING_PARFIELDS
                msg << "Edge(" << recv_edge[ jedge * varsize + 2 ] << "[p" << recv_edge[ jedge * varsize + 4 ] << "] "
                    << recv_edge[ jedge *varsize + 3 ] << "[p" << recv_edge[ jedge * varsize + 5 ] << "])";
#else
                msg << "Edge with uid " << recv_uid;
#endif
                msg << " requested by rank [" << jpart << "]";
                msg << " that should be owned by " << mpi::comm().rank() << " is not found. This could be because no "
                       "halo was built.";
                // throw eckit::SeriousBug(msg.str(),Here());
                Log::warning() << msg.str() << " @ " << Here() << std::endl;
            }
        }
    }

    ATLAS_TRACE_MPI( ALLTOALL ) { mpi::comm().allToAll( send_found, recv_found ); }

    for ( size_t jpart = 0; jpart < nparts; ++jpart ) {
        const std::vector<int>& recv_edge = recv_found[jpart];
        const size_t nb_recv_edges        = recv_edge.size() / 2;
        // array::ArrayView<int,2> recv_edge( recv_found[ jpart ].data(),
        //     array::make_shape(recv_found[ jpart ].size()/2,2) );
        for ( size_t jedge = 0; jedge < nb_recv_edges; ++jedge ) {
            edge_ridx( recv_edge[jedge * 2 + 0] ) = recv_edge[jedge * 2 + 1];
        }
    }
    return edges.remote_index();
}

Field& build_edges_global_idx( Mesh& mesh ) {
    ATLAS_TRACE();

    UniqueLonLat compute_uid( mesh );

    int nparts  = mpi::comm().size();
    size_t root = 0;

    mesh::HybridElements& edges = mesh.edges();

    array::make_view<gidx_t, 1>( edges.global_index() ).assign( -1 );
    array::ArrayView<gidx_t, 1> edge_gidx = array::make_view<gidx_t, 1>( edges.global_index() );

    const mesh::HybridElements::Connectivity& edge_nodes = edges.node_connectivity();
    array::ArrayView<double, 2> xy                       = array::make_view<double, 2>( mesh.nodes().xy() );
    std::shared_ptr<array::ArrayView<int, 1>> is_pole_edge;
    bool has_pole_edges = false;
    if ( edges.has_field( "is_pole_edge" ) ) {
        has_pole_edges = true;
        is_pole_edge   = std::shared_ptr<array::ArrayView<int, 1>>(
            new array::ArrayView<int, 1>( array::make_view<int, 1>( edges.field( "is_pole_edge" ) ) ) );
    }

    /*
 * Sorting following edge_gidx will define global order of
 * gathered fields. Special care needs to be taken for
 * pole edges, as their centroid might coincide with
 * other edges
 */
    double centroid[2];
    int nb_edges = edges.size();
    for ( int jedge = 0; jedge < nb_edges; ++jedge ) {
        if ( edge_gidx( jedge ) <= 0 ) {
            centroid[XX] = 0.5 * ( xy( edge_nodes( jedge, 0 ), XX ) + xy( edge_nodes( jedge, 1 ), XX ) );
            centroid[YY] = 0.5 * ( xy( edge_nodes( jedge, 0 ), YY ) + xy( edge_nodes( jedge, 1 ), YY ) );
            if ( has_pole_edges && ( *is_pole_edge )( jedge ) ) { centroid[YY] = centroid[YY] > 0 ? 90. : -90.; }
            edge_gidx( jedge ) = util::unique_lonlat( centroid );
        }
    }

    /*
 * REMOTE INDEX BASE = 1
 */

    // 1) Gather all global indices, together with location
    array::ArrayT<uid_t> loc_edge_id_arr( nb_edges );
    array::ArrayView<uid_t, 1> loc_edge_id = array::make_view<uid_t, 1>( loc_edge_id_arr );

    for ( int jedge = 0; jedge < nb_edges; ++jedge ) {
        loc_edge_id( jedge ) = edge_gidx( jedge );
    }

    std::vector<int> recvcounts( mpi::comm().size() );
    std::vector<int> recvdispls( mpi::comm().size() );

    ATLAS_TRACE_MPI( GATHER ) { mpi::comm().gather( nb_edges, recvcounts, root ); }

    recvdispls[0] = 0;
    for ( int jpart = 1; jpart < nparts; ++jpart )  // start at 1
    {
        recvdispls[jpart] = recvcounts[jpart - 1] + recvdispls[jpart - 1];
    }
    int glb_nb_edges = std::accumulate( recvcounts.begin(), recvcounts.end(), 0 );

    array::ArrayT<uid_t> glb_edge_id_arr( glb_nb_edges );
    array::ArrayView<uid_t, 1> glb_edge_id = array::make_view<uid_t, 1>( glb_edge_id_arr );

    ATLAS_TRACE_MPI( GATHER ) {
        mpi::comm().gatherv( loc_edge_id.data(), loc_edge_id.size(), glb_edge_id.data(), recvcounts.data(),
                             recvdispls.data(), root );
    }

    // 2) Sort all global indices, and renumber from 1 to glb_nb_edges
    std::vector<Node> edge_sort;
    edge_sort.reserve( glb_nb_edges );
    for ( size_t jedge = 0; jedge < glb_edge_id.shape( 0 ); ++jedge ) {
        edge_sort.emplace_back( Node( glb_edge_id( jedge ), jedge ) );
    }
    std::sort( edge_sort.begin(), edge_sort.end() );

    // Assume edge gid start
    uid_t gid( 0 );
    for ( size_t jedge = 0; jedge < edge_sort.size(); ++jedge ) {
        if ( jedge == 0 ) { ++gid; }
        else if ( edge_sort[jedge].g != edge_sort[jedge - 1].g ) {
            ++gid;
        }
        int iedge            = edge_sort[jedge].i;
        glb_edge_id( iedge ) = gid;
    }

    // 3) Scatter renumbered back
    ATLAS_TRACE_MPI( SCATTER ) {
        mpi::comm().scatterv( glb_edge_id.data(), recvcounts.data(), recvdispls.data(), loc_edge_id.data(),
                              loc_edge_id.size(), root );
    }

    for ( int jedge = 0; jedge < nb_edges; ++jedge ) {
        edge_gidx( jedge ) = loc_edge_id( jedge );
    }

    return edges.global_index();
}

//----------------------------------------------------------------------------------------------------------------------
// C wrapper interfaces to C++ routines

void atlas__build_parallel_fields( Mesh::Implementation* mesh ) {
    ATLAS_ERROR_HANDLING( Mesh m( mesh ); build_parallel_fields( m ); );
}
void atlas__build_nodes_parallel_fields( mesh::Nodes* nodes ) {
    ATLAS_ERROR_HANDLING( build_nodes_parallel_fields( *nodes ) );
}

void atlas__build_edges_parallel_fields( Mesh::Implementation* mesh ) {
    ATLAS_ERROR_HANDLING( Mesh m( mesh ); build_edges_parallel_fields( m ); );
}

void atlas__renumber_nodes_glb_idx( mesh::Nodes* nodes ) {
    ATLAS_ERROR_HANDLING( renumber_nodes_glb_idx( *nodes ) );
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace actions
}  // namespace mesh
}  // namespace atlas
