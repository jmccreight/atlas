/*
 * (C) Copyright 1996-2014 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <cassert>
#include <iostream>
#include <sstream>
#include <limits>

#include "eckit/exception/Exceptions.h"
#include "eckit/types/Types.h"

#include "atlas/atlas_config.h"
#include "atlas/runtime/ErrorHandling.h"
#include "atlas/FunctionSpace.h"
#include "atlas/Field.h"
#include "atlas/actions/BuildParallelFields.h"
#include "atlas/util/Debug.h"
#include "atlas/util/Bitflags.h"

using atlas::util::Topology;

#ifdef ATLAS_HAVE_FORTRAN
#define REMOTE_IDX_BASE 1
#else
#define REMOTE_IDX_BASE 0
#endif


namespace atlas {

FunctionSpace::FunctionSpace(const std::string& name, const std::string& shape_func, const std::vector<size_t>& shape , Mesh& mesh) :
	name_(name),
	shape_(shape),
	gather_scatter_(new mpl::GatherScatter()),
	fullgather_(new mpl::GatherScatter()),
	halo_exchange_(new mpl::HaloExchange()),
	checksum_(new mpl::Checksum()),
	mesh_(mesh)
{
	//std::cout << "C++ : shape Constructor" << std::endl;
	dof_ = 1;
	size_t extsize = shape_.size();
	shapef_.resize(extsize);
	for (size_t i=0; i<extsize; ++i)
	{
		shapef_[extsize-1-i] = shape_[i];
#warning "SERIOUS ALERT -- UNDEF_VARS is -1 and we compare to size_t !??"
        if( shape_[i] != Field::UNDEF_VARS )
			dof_ *= shape_[i];
	}
	glb_dof_ = dof_;
}

FunctionSpace::~FunctionSpace()
{
}

void FunctionSpace::resize(const std::vector<size_t>& shape)
{
	if (shape.size() != shape_.size() )
		throw eckit::BadParameter("Cannot resize shape: shape sizes don't match.",Here());

	size_t extsize = shape_.size();

	for (size_t i=1; i<extsize; ++i)
	{
		if (shape[i] != shape_[i])
			throw eckit::BadParameter("Only the first extent can be resized for now!",Here());
	}

	shape_ = shape;
	shapef_.resize(extsize);
	for (size_t i=0; i<extsize; ++i)
	{
		shapef_[extsize-1-i] = shape_[i];
	}

	dof_ = 1;
	for (size_t i=0; i<extsize; ++i)
	{
		if( shape_[i] != Field::UNDEF_VARS )
			dof_ *= shape_[i];
	}

    for( size_t f=0; f<fields_.size(); ++f)
	{
		std::vector< size_t > field_shape(extsize);
		for (size_t i=0; i<extsize; ++i)
		{
			if( shape_[i] == Field::UNDEF_VARS )
				field_shape[i] = fields_[f]->nb_vars();
			else
				field_shape[i] = shape_[i];
		}
		fields_[f]->allocate(field_shape);
	}
}

template<>
FieldT<double>& FunctionSpace::field(const std::string& name) const
{
	if( has_field(name) )
	{
		return dynamic_cast< FieldT<double>& >( *fields_[ name ] );
	}
	else
	{
		std::stringstream msg;
		msg << "Could not find field \"" << name << "\" in FunctionSpace \"" << name_ << "\"";
		throw eckit::OutOfRange(msg.str(),Here());
	}
}

template<>
FieldT<float>& FunctionSpace::field(const std::string& name) const
{
	if( has_field(name) )
	{
		return dynamic_cast< FieldT<float>& >( *fields_[ name ] );
	}
	else
	{
		std::stringstream msg;
		msg << "Could not find field \"" << name << "\" in FunctionSpace \"" << name_ << "\"";
		throw eckit::OutOfRange(msg.str(),Here());
	}
}

template<>
FieldT<long>& FunctionSpace::field(const std::string& name) const
{
	if( has_field(name) )
	{
		return dynamic_cast< FieldT<long>& >( *fields_[ name ] );
	}
	else
	{
		std::stringstream msg;
		msg << "Could not find field \"" << name << "\" in FunctionSpace \"" << name_ << "\"";
		throw eckit::OutOfRange(msg.str(),Here());
	}
}

template<>
FieldT<int>& FunctionSpace::field(const std::string& name) const
{
	if( has_field(name) )
	{
		return dynamic_cast< FieldT<int>& >( *fields_[ name ] );
	}
	else
	{
		std::stringstream msg;
		msg << "Could not find field \"" << name << "\" in FunctionSpace \"" << name_ << "\"";
		throw eckit::OutOfRange(msg.str(),Here());
	}
}


namespace {

	template < typename T >
	FieldT<T>* check_if_exists( FunctionSpace* fs,
								const std::string& name,
								const std::vector<size_t>&  shape,
								size_t nb_vars,
								CreateBehavior b )
	{
		using namespace eckit;

		if( fs->has_field(name) )
		{
			if( b == IF_EXISTS_FAIL )
			{
				std::ostringstream msg; msg << "field with name " << name << " already exists" << std::endl;
				throw eckit::Exception( msg.str(), Here() );
			}

			FieldT<T>& f= fs->field<T>(name);
			if( f.nb_vars() != nb_vars )
			{
				std::ostringstream msg; msg << "field exists with name " << name << " has unexpected nb vars " << f.nb_vars() << " instead of " << nb_vars << std::endl;
				throw eckit::Exception(msg.str(),Here());
			}

			if( f.shape() != shape )
			{
				std::ostringstream msg; msg << "field exists with name " << name << " has unexpected shape ";
				__print_list(msg, f.shape());
				msg << " instead of ";
				__print_list(msg, shape);
				msg << std::endl;
				throw eckit::Exception(msg.str(),Here());
			}

			return &f;
		}

		return NULL;
	}

}

template <>
FieldT<double>& FunctionSpace::create_field(const std::string& name, size_t nb_vars, CreateBehavior b )
{
	FieldT<double>* field = NULL;

	size_t rank = shape_.size();
	std::vector< size_t > field_shape(rank);
	for (size_t i=0; i<rank; ++i)
	{
		if( shape_[i] == Field::UNDEF_VARS )
			field_shape[i] = nb_vars;
		else
			field_shape[i] = shape_[i];
	}

	if( (field = check_if_exists<double>(this, name, field_shape, nb_vars, b )) )
		return *field;

	field = new FieldT<double>(name,nb_vars);
	fields_.insert( name, Field::Ptr(field) );
	fields_.sort();

  field->set_function_space(*this);
	field->allocate(field_shape);
	return *field;
}

template <>
FieldT<float>& FunctionSpace::create_field(const std::string& name, size_t nb_vars, CreateBehavior b )
{
	FieldT<float>* field = NULL;

	size_t rank = shape_.size();
	std::vector< size_t > field_shape(rank);
	for (size_t i=0; i<rank; ++i)
	{
		if( shape_[i] == Field::UNDEF_VARS )
			field_shape[i] = nb_vars;
		else
			field_shape[i] = shape_[i];
	}

	if( (field = check_if_exists<float>(this, name, field_shape, nb_vars, b )) )
		return *field;

	field = new FieldT<float>(name,nb_vars);
	fields_.insert( name, Field::Ptr(field) );
	fields_.sort();
  field->set_function_space(*this);
	field->allocate(field_shape);
	return *field;
}

template <>
FieldT<int>& FunctionSpace::create_field(const std::string& name, size_t nb_vars, CreateBehavior b )
{
	FieldT<int>* field = NULL;

	size_t rank = shape_.size();
	std::vector< size_t > field_shape(rank);
	for (size_t i=0; i<rank; ++i)
	{
		if( shape_[i] == Field::UNDEF_VARS )
			field_shape[i] = nb_vars;
		else
			field_shape[i] = shape_[i];
	}

	if( (field = check_if_exists<int>(this, name, field_shape, nb_vars, b )) )
		return *field;

	field = new FieldT<int>(name,nb_vars);

	fields_.insert( name, Field::Ptr(field) );
	fields_.sort();

  field->set_function_space(*this);
	field->allocate(field_shape);
	return *field;
}

template <>
FieldT<long>& FunctionSpace::create_field(const std::string& name, size_t nb_vars, CreateBehavior b )
{
	FieldT<long>* field = NULL;

	size_t rank = shape_.size();
	std::vector< size_t > field_shape(rank);
	for (size_t i=0; i<rank; ++i)
	{
		if( shape_[i] == Field::UNDEF_VARS )
			field_shape[i] = nb_vars;
		else
			field_shape[i] = shape_[i];
	}

	if( (field = check_if_exists<long>(this, name, field_shape, nb_vars, b )) )
		return *field;

	field = new FieldT<long>(name,nb_vars);

	fields_.insert( name, Field::Ptr(field) );
	fields_.sort();

	field->allocate(field_shape);
	return *field;
}


void FunctionSpace::remove_field(const std::string& name)
{
	NOTIMP; ///< @todo DenseMap needs to have erase() function

//	if( has_field(name) )
//	{
//		fields_.erase(name);
//	}
//	else
//	{
//		std::stringstream msg;
//		msg << "Could not find field \"" << name << "\" in FunctionSpace \"" << name_ << "\"";
//		throw eckit::OutOfRange(msg.str(),Here());
//	}
}

Field& FunctionSpace::field( size_t idx ) const
{
	return *fields_.at( idx );
}

Field& FunctionSpace::field(const std::string& name) const
{
	if( has_field(name) )
	{
		return *fields_.get(name);
	}
	else
	{
		std::stringstream msg;
		msg << "Could not find field \"" << name << "\" in FunctionSpace \"" << name_ << "\"";
		throw eckit::OutOfRange(msg.str(),Here());
	}
}

void FunctionSpace::parallelise(const int part[], const int remote_idx[], const gidx_t glb_idx[], size_t parsize)
{
	halo_exchange_->setup(part,remote_idx,REMOTE_IDX_BASE,parsize);
	gather_scatter_->setup(part,remote_idx,REMOTE_IDX_BASE,glb_idx,-1,parsize);
	fullgather_->setup(part,remote_idx,REMOTE_IDX_BASE,glb_idx,-1,parsize,true);
	checksum_->setup(part,remote_idx,REMOTE_IDX_BASE,glb_idx,-1,parsize);
	glb_dof_ = gather_scatter_->glb_dof();
	for( int b=shapef_.size()-2; b>=0; --b)
	{
		if( shapef_[b] != Field::UNDEF_VARS )
			glb_dof_ *= shapef_[b];
	}
}

void FunctionSpace::parallelise(FunctionSpace& other_shape)
{
	halo_exchange_ = mpl::HaloExchange::Ptr( &other_shape.halo_exchange() );
	gather_scatter_ = mpl::GatherScatter::Ptr( &other_shape.gather_scatter() );
}

void FunctionSpace::parallelise()
{
  FieldT<int>& ridx = field<int>("remote_idx");
  FieldT<int>& part = field<int>("partition");
  FieldT<gidx_t>& gidx = field<gidx_t>("glb_idx");

  if( name() == "nodes")
  {
    ArrayView<int,1> flags ( field<int>("flags") );
    std::vector<int> mask(shape(0));
    for( size_t j=0; j<mask.size(); ++j )
    {
      mask[j] = Topology::check(flags(j),Topology::GHOST) ? 1 : 0;
    }
    halo_exchange_->setup(part.data(),ridx.data(),REMOTE_IDX_BASE,shape(0));
    gather_scatter_->setup(part.data(),ridx.data(),REMOTE_IDX_BASE,gidx.data(),mask.data(),shape(0));
    fullgather_->setup(part.data(),ridx.data(),REMOTE_IDX_BASE,gidx.data(),-1,shape(0),true);
    checksum_->setup(part.data(),ridx.data(),REMOTE_IDX_BASE,gidx.data(),mask.data(),shape(0));
    glb_dof_ = gather_scatter_->glb_dof();
    for( int b=shapef_.size()-2; b>=0; --b)
    {
      if( shapef_[b] != Field::UNDEF_VARS )
        glb_dof_ *= shapef_[b];
    }
  }
  else
  {
    halo_exchange_->setup(part.data(),ridx.data(),REMOTE_IDX_BASE,shape(0));
    gather_scatter_->setup(part.data(),ridx.data(),REMOTE_IDX_BASE,gidx.data(),-1,shape(0));
    fullgather_->setup(part.data(),ridx.data(),REMOTE_IDX_BASE,gidx.data(),-1,shape(0),true);
    checksum_->setup(part.data(),ridx.data(),REMOTE_IDX_BASE,gidx.data(),-1,shape(0));
  }
  glb_dof_ = gather_scatter_->glb_dof();
  for( int b=shapef_.size()-2; b>=0; --b)
  {
    if( shapef_[b] != Field::UNDEF_VARS )
      glb_dof_ *= shapef_[b];
  }
}

//------------------------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& s,const FunctionSpace& fs)
{
	s << "FunctionSpace [" << fs.name() << "]" << std::endl;
	for( size_t i = 0; i < fs.nb_fields() ; ++i )
	{
		s << "  Field [ " << fs.field(i).name() << " ] <" << fs.field(i).data_type() << ">" << std::endl;
	}
	return s;
}

// ------------------------------------------------------------------
// C wrapper interfaces to C++ routines

Metadata* atlas__FunctionSpace__metadata (FunctionSpace* This)
{
  ASSERT( This != NULL );
  return &This->metadata();
}

int atlas__FunctionSpace__dof (FunctionSpace* This) {
  ASSERT( This != NULL );
  return This->dof();
}

int atlas__FunctionSpace__glb_dof (FunctionSpace* This) {
  ASSERT( This != NULL );
  return This->glb_dof();
}

void atlas__FunctionSpace__create_field_double (FunctionSpace* This, char* name, int nb_vars) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( This->create_field<double>( std::string(name), nb_vars ) );
}

void atlas__FunctionSpace__create_field_float (FunctionSpace* This, char* name, int nb_vars) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( This->create_field<float>( std::string(name), nb_vars ) );
}

void atlas__FunctionSpace__create_field_int (FunctionSpace* This, char* name, int nb_vars) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( This->create_field<int>( std::string(name), nb_vars ) );
}

void atlas__FunctionSpace__create_field_long (FunctionSpace* This, char* name, int nb_vars) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( This->create_field<long>( std::string(name), nb_vars ) );
}

void atlas__FunctionSpace__remove_field (FunctionSpace* This, char* name ) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( This->remove_field( std::string(name) ) );
}

int atlas__FunctionSpace__has_field (FunctionSpace* This, char* name) {
  ASSERT( This != NULL );
  return This->has_field( std::string(name) );
}

const char* atlas__FunctionSpace__name (FunctionSpace* This) {
  ASSERT( This != NULL );
  return This->name().c_str();
}

void atlas__FunctionSpace__shapef (FunctionSpace* This, int* &shape, int &rank) {
  ASSERT( This != NULL );
  shape = const_cast<int*>(&(This->shapef()[0]));
  rank = This->shapef().size();
}

Field* atlas__FunctionSpace__field (FunctionSpace* This, char* name) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( return &This->field( std::string(name) ) );
  return NULL;
}

void atlas__FunctionSpace__parallelise (FunctionSpace* This) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( This->parallelise() );
}

void atlas__FunctionSpace__halo_exchange_int (FunctionSpace* This, int field_data[], int field_size) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING(
    This->halo_exchange(field_data,field_size) );
}

void atlas__FunctionSpace__halo_exchange_float (FunctionSpace* This, float field_data[], int field_size) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( This->halo_exchange(field_data,field_size) );
}

void atlas__FunctionSpace__halo_exchange_double (FunctionSpace* This, double field_data[], int field_size) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( This->halo_exchange(field_data,field_size) );
}

void atlas__FunctionSpace__gather_int (FunctionSpace* This, int field_data[], int field_size, int glbfield_data[], int glbfield_size) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING(
    This->gather(field_data,field_size, glbfield_data,glbfield_size) );
}

void atlas__FunctionSpace__gather_float (FunctionSpace* This, float field_data[], int field_size, float glbfield_data[], int glbfield_size) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING(
    This->gather(field_data,field_size, glbfield_data,glbfield_size) );
}

void atlas__FunctionSpace__gather_double (FunctionSpace* This, double field_data[], int field_size, double glbfield_data[], int glbfield_size) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING(
    This->gather(field_data,field_size, glbfield_data,glbfield_size) );
}

mpl::HaloExchange* atlas__FunctionSpace__halo_exchange (FunctionSpace* This) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( return &This->halo_exchange() );
  return NULL;
}

mpl::GatherScatter* atlas__FunctionSpace__gather (FunctionSpace* This) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( return &This->gather_scatter() );
  return NULL;
}

mpl::Checksum* atlas__FunctionSpace__checksum (FunctionSpace* This) {
  ASSERT( This != NULL );
  ATLAS_ERROR_HANDLING( return &This->checksum() );
  return NULL;
}

void atlas__FunctionSpace__delete (FunctionSpace* This) {
  ASSERT( This != NULL );
  delete This;
}
// ------------------------------------------------------------------

} // namespace atlas

