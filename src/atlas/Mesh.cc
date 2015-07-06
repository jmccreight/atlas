/*
 * (C) Copyright 1996-2014 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "atlas/Mesh.h"

#include <sstream>
#include <stdexcept>

#include "eckit/exception/Exceptions.h"
#include "eckit/log/Log.h"

#include "atlas/runtime/ErrorHandling.h"
#include "atlas/FunctionSpace.h"
#include "atlas/Field.h"
#include "atlas/Grid.h"
#include "atlas/Parameters.h"

namespace atlas {

//----------------------------------------------------------------------------------------------------------------------

Mesh* Mesh::create( const eckit::Parametrisation& params )
{
  return new Mesh(params);
}

Mesh* Mesh::create( const Grid& grid, const eckit::Parametrisation& params )
{
  return new Mesh(grid,params);
}

Mesh::Mesh( const eckit::Parametrisation& ):
  grid_(NULL)
{
}

Mesh::Mesh(const Grid& grid, const eckit::Parametrisation& ) :
  grid_(&grid)
{
  add_nodes(grid);
}

Mesh::~Mesh()
{
}

bool Mesh::has_function_space(const std::string& name) const
{
	return function_spaces_.has(name);
}

FunctionSpace& Mesh::create_function_space(const std::string& name, const std::string& shape_func, const std::vector<size_t>& shape)
{
	if( has_function_space(name) )
	{
		throw eckit::Exception( "Functionspace '" + name + "' already exists", Here() );
	}

	FunctionSpace::Ptr fs( new FunctionSpace(name,shape_func,shape,*this) );

	function_spaces_.insert(name,fs);
	function_spaces_.sort();

	fs->set_index( function_spaces_.size() - 1 ); ///< @todo revisit this once we can remove functionspaces

	return *fs;
}

FunctionSpace& Mesh::function_space(const std::string& name) const
{
	if( ! has_function_space(name) )
	{
		std::stringstream msg;
		msg << "Could not find FunctionSpace '" << name << "' in mesh";
		throw eckit::OutOfRange(msg.str(),Here());
	}
	return *( function_spaces_[ name ] );
}

FunctionSpace& Mesh::function_space( size_t idx) const
{
	if( idx >= function_spaces_.size() )
		throw eckit::OutOfRange(idx,function_spaces_.size(),Here());
	return *function_spaces_[ idx ];
}


FunctionSpace& Mesh::add_nodes(const Grid& g)
{
  set_grid(g);
  size_t nb_nodes = g.npts();
  FunctionSpace& nodes = add_nodes(nb_nodes);

  g.fillLonLat(nodes.field("lonlat").data<double>(), nb_nodes*2);
  return nodes;
}


FunctionSpace& Mesh::add_nodes(size_t nb_nodes)
{
  if( has_function_space("nodes") )
    throw eckit::Exception("Nodes have already been added before", Here());

  ArrayShape shape = make_shape(nb_nodes,Field::UNDEF_VARS);

  FunctionSpace& nodes = create_function_space( "nodes","LagrangeP1",shape );
  nodes.metadata().set<long>("type",static_cast<int>(Entity::NODES));

  ArrayView<double,2> lonlat  ( nodes.create_field<double>("lonlat",   2, IF_EXISTS_RETURN) );
  ArrayView<gidx_t,1> glb_idx ( nodes.create_field<gidx_t>("glb_idx",  1, IF_EXISTS_RETURN) );
  ArrayView<int,   1> part    ( nodes.create_field<int   >("partition",1, IF_EXISTS_RETURN) );
  ArrayView<int,   1> flags   ( nodes.create_field<int   >("flags",    1, IF_EXISTS_RETURN) );

  for(size_t n=0; n<nb_nodes; ++n)
  {
    glb_idx(n) = 1+n;
    part(n) = eckit::mpi::rank();
    flags(n) = 0;
  }

  return nodes;
}

void Mesh::prettyPrint(std::ostream& os) const
{
    os << "Mesh:" << std::endl;
    for( size_t i = 0; i < nb_function_spaces(); ++i )
    {
        os << function_space(i) << std::endl;
    }
}
void Mesh::print(std::ostream& os) const
{
    os << "Mesh[";
    for( size_t i = 0; i < nb_function_spaces(); ++i )
    {
        os << function_space(i);
    }
    os << "]";
}

//----------------------------------------------------------------------------------------------------------------------

// C wrapper interfaces to C++ routines

Mesh* atlas__Mesh__new () {
	return new Mesh();
}

void atlas__Mesh__delete (Mesh* This) {
	delete This;
}

void atlas__Mesh__create_function_space(Mesh* This, char* name,char* shape_func,int shape[], int shape_size)
{
  ATLAS_ERROR_HANDLING(

    ASSERT( This != NULL );
    ASSERT( shape_size >= 0 );

    std::vector<size_t> vshape(shape_size);
    for(size_t n=0; n<vshape.size(); ++n)
        vshape[n]=shape[n];

    This->create_function_space(std::string(name), std::string(shape_func),vshape);
  );
}

FunctionSpace* atlas__Mesh__function_space (Mesh* This, char* name) {
  ATLAS_ERROR_HANDLING(
    ASSERT( This != NULL );
  	return &This->function_space( std::string(name) );
  );
  return NULL;
}

Grid* atlas__Mesh__grid (Mesh* This) {
  ATLAS_ERROR_HANDLING(
    ASSERT( This != NULL );
    return const_cast<Grid*>(&This->grid());
  );
  return NULL;
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace atlas

