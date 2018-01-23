/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "atlas/trans/ifs/TransIFS.h"

#include "eckit/parser/JSON.h"
#include "atlas/array.h"
#include "atlas/functionspace/NodeColumns.h"
#include "atlas/functionspace/Spectral.h"
#include "atlas/functionspace/StructuredColumns.h"
#include "atlas/mesh/IsGhostNode.h"
#include "atlas/mesh/Nodes.h"
#include "atlas/runtime/ErrorHandling.h"
#include "atlas/runtime/Log.h"
#include "atlas/parallel/mpi/mpi.h"

using Topology = atlas::mesh::Nodes::Topology;
using atlas::mesh::IsGhostNode;
using atlas::functionspace::StructuredColumns;
using atlas::functionspace::NodeColumns;
using atlas::functionspace::Spectral;
using atlas::Field;
using atlas::FunctionSpace;
using atlas::array::ArrayView;
using atlas::array::make_view;

namespace atlas {
namespace trans {

namespace {
static TransBuilderGrid<TransIFS> builder("ifs");
}

class TransParameters {
public:
  TransParameters( const eckit::Configuration& config ) : config_(config) {}
  ~TransParameters() {}

  bool scalar_derivatives() const {
    return config_.getBool("scalar_derivatives",false);
  }

  bool wind_EW_derivatives() const {
    return config_.getBool("wind_EW_derivatives",false);
  }

  bool vorticity_divergence_fields() const {
    return config_.getBool("vorticity_divergence_fields",false);
  }

  bool split_latitudes() const {
    return config_.getBool("split_latitudes",true);
  }

  int fft() const {
    static const std::map<std::string,int> string_to_FFT = { {"FFT992",TRANS_FFT992},{"FFTW",TRANS_FFTW} };
    return string_to_FFT.at( config_.getString("fft","FFTW") );
  }

  bool flt() const {
    return config_.getBool("flt",false);
  }

  std::string read_legendre() const {
    return config_.getString("read_legendre","");
  }

  std::string write_legendre() const {
    return config_.getString("write_legendre","");
  }

  bool global() const {
    return config_.getBool("global",false);
  }

private:
  const eckit::Configuration& config_;
};

namespace {
std::string fieldset_functionspace( const FieldSet& fields ) {
  std::string functionspace("undefined");
  for( size_t jfld = 0; jfld < fields.size(); ++jfld) {
    if( functionspace == "undefined" )
      functionspace = fields[jfld].functionspace().type();
    if( fields[jfld].functionspace().type() != functionspace ) {
      throw eckit::SeriousBug(": fielset has fields with different functionspaces",Here());
    }
  }
  return functionspace;
}
void assert_spectral_functionspace( const FieldSet& fields ){
  for( size_t jfld = 0; jfld < fields.size(); ++jfld ) {
    ASSERT( functionspace::Spectral( fields[jfld].functionspace() ) );
  }
}

void trans_check(const int code, const char* msg, const eckit::CodeLocation& location) {
  if(code != TRANS_SUCCESS) {
    std::stringstream errmsg;
    errmsg << "atlas::trans ERROR: " << msg << " failed: \n";
    errmsg << ::trans_error_msg(code);
    throw eckit::Exception(errmsg.str(),location);
  }
}
#define TRANS_CHECK( CALL ) trans_check(CALL, #CALL, Here() )

}

void TransIFS::dirtrans( const Field& gpfield,
                               Field& spfield,
                         const eckit::Configuration& config ) const {
  ASSERT( functionspace::Spectral(spfield.functionspace()) );
  if( functionspace::StructuredColumns( gpfield.functionspace() ) ) {
    __dirtrans( functionspace::StructuredColumns( gpfield.functionspace() ), gpfield , functionspace::Spectral( spfield.functionspace() ), spfield, config );
  } else if( functionspace::NodeColumns( gpfield.functionspace() ) ) {
    __dirtrans( functionspace::NodeColumns( gpfield.functionspace() ), gpfield, functionspace::Spectral( spfield.functionspace() ), spfield, config );
  } else {
    NOTIMP;
  }
}


void TransIFS::dirtrans( const FieldSet& gpfields,
                               FieldSet& spfields,
                         const eckit::Configuration& config ) const {
  assert_spectral_functionspace(spfields);
  std::string functionspace( fieldset_functionspace(gpfields) );

  if( functionspace == StructuredColumns::type() ) {
    __dirtrans( StructuredColumns( gpfields[0].functionspace() ), gpfields,
              Spectral( spfields[0].functionspace()), spfields, config );
  } else if ( functionspace == NodeColumns::type() ) {
    __dirtrans( NodeColumns( gpfields[0].functionspace() ), gpfields,
              Spectral( spfields[0].functionspace()), spfields, config );
  } else {
    NOTIMP;
  }
}


void TransIFS::invtrans( const Field& spfield,
                               Field& gpfield,
                         const eckit::Configuration& config ) const
{
  ASSERT( Spectral( spfield.functionspace()) );
  if( StructuredColumns( gpfield.functionspace()) ) {
    __invtrans( Spectral( spfield.functionspace() ), spfield, StructuredColumns( gpfield.functionspace() ), gpfield, config );
  } else if( NodeColumns( gpfield.functionspace()) ) {
    __invtrans( Spectral( spfield.functionspace() ), spfield, NodeColumns( gpfield.functionspace() ), gpfield, config );
  } else {
    NOTIMP;
  }
}

void TransIFS::invtrans( const FieldSet& spfields,
                               FieldSet& gpfields,
                         const eckit::Configuration& config ) const
{
  assert_spectral_functionspace( spfields );
  std::string functionspace( fieldset_functionspace(gpfields) );

  if( functionspace == StructuredColumns::type() ) {
    __invtrans( Spectral(spfields[0].functionspace()), spfields, StructuredColumns(gpfields[0].functionspace()), gpfields, config );
  } else if ( functionspace == NodeColumns::type() ) {
    __invtrans( Spectral(spfields[0].functionspace()), spfields, NodeColumns(gpfields[0].functionspace()), gpfields, config );
  } else {
    NOTIMP;
  }
}

// --------------------------------------------------------------------------------------------

void TransIFS::invtrans_grad(
    const Field& spfield,
          Field& gradfield,
    const eckit::Configuration& config ) const
{
  ASSERT( Spectral(spfield.functionspace()) );
  ASSERT( NodeColumns(gradfield.functionspace()) );
  __invtrans_grad( Spectral(spfield.functionspace()), spfield, NodeColumns(gradfield.functionspace()), gradfield, config );
}

void TransIFS::invtrans_grad(
    const FieldSet& spfields,
          FieldSet& gradfields,
    const eckit::Configuration& config ) const
{
  assert_spectral_functionspace(spfields);
  std::string functionspace( fieldset_functionspace(gradfields) );

  if ( functionspace == NodeColumns::type() ) {
    __invtrans_grad( Spectral( spfields[0].functionspace()), spfields,
                   NodeColumns( gradfields[0].functionspace() ), gradfields, config );
  } else {
    NOTIMP;
  }
}

void TransIFS::dirtrans_wind2vordiv(
    const Field& gpwind,
    Field& spvor, Field& spdiv,
    const eckit::Configuration& config ) const {
  ASSERT( Spectral( spvor.functionspace()) );
  ASSERT( Spectral( spdiv.functionspace()) );
  ASSERT( NodeColumns( gpwind.functionspace()) );
  __dirtrans_wind2vordiv( NodeColumns(gpwind.functionspace()), gpwind, Spectral(spvor.functionspace()), spvor, spdiv, config );
}

void TransIFS::invtrans_vordiv2wind( const Field& spvor, const Field& spdiv,
                                           Field& gpwind,
                                     const eckit::Configuration& config ) const {
  ASSERT( Spectral( spvor.functionspace() ) );
  ASSERT( Spectral( spdiv.functionspace() ) );
  ASSERT( NodeColumns( gpwind.functionspace() ) );
  __invtrans_vordiv2wind( Spectral(spvor.functionspace()), spvor, spdiv, NodeColumns(gpwind.functionspace()), gpwind, config );
}


void TransIFS::invtrans( const int nb_scalar_fields, const double scalar_spectra[],
                         const int nb_vordiv_fields, const double vorticity_spectra[], const double divergence_spectra[],
                         double gp_fields[],
                         const eckit::Configuration& config ) const
{
  TransParameters params(config);
  struct ::InvTrans_t args = new_invtrans(trans_.get());
    args.nscalar = nb_scalar_fields;
    args.rspscalar = scalar_spectra;
    args.nvordiv = nb_vordiv_fields;
    args.rspvor = vorticity_spectra;
    args.rspdiv = divergence_spectra;
    args.rgp = gp_fields;
    args.lglobal     = params.global();
    args.lscalarders = params.scalar_derivatives();
    args.luvder_EW   = params.wind_EW_derivatives();
    args.lvordivgp   = params.vorticity_divergence_fields();
  TRANS_CHECK( ::trans_invtrans(&args) );
}

///////////////////////////////////////////////////////////////////////////////

void TransIFS::invtrans( const int nb_scalar_fields, const double scalar_spectra[],
                         double gp_fields[],
                         const eckit::Configuration& config ) const
{
  TransParameters params(config);
  struct ::InvTrans_t args = new_invtrans(trans_.get());
    args.nscalar     = nb_scalar_fields;
    args.rspscalar   = scalar_spectra;
    args.rgp         = gp_fields;
    args.lglobal     = params.global();
    args.lscalarders = params.scalar_derivatives();
  TRANS_CHECK( ::trans_invtrans(&args) );
}

///////////////////////////////////////////////////////////////////////////////

void TransIFS::invtrans( const int nb_vordiv_fields, const double vorticity_spectra[], const double divergence_spectra[],
                         double gp_fields[],
                         const eckit::Configuration& config ) const
{
  TransParameters params(config);
  struct ::InvTrans_t args = new_invtrans(trans_.get());
    args.nvordiv = nb_vordiv_fields;
    args.rspvor  = vorticity_spectra;
    args.rspdiv  = divergence_spectra;
    args.rgp     = gp_fields;
    args.lglobal = params.global();
    args.luvder_EW = params.wind_EW_derivatives();
    args.lvordivgp = params.vorticity_divergence_fields();
  TRANS_CHECK( ::trans_invtrans(&args) );
}

///////////////////////////////////////////////////////////////////////////////

void TransIFS::dirtrans( const int nb_fields, const double scalar_fields[], double scalar_spectra[],
                         const eckit::Configuration& config ) const
{
  TransParameters params(config);
  struct ::DirTrans_t args = new_dirtrans(trans_.get());
    args.nscalar = nb_fields;
    args.rgp = scalar_fields;
    args.rspscalar = scalar_spectra;
    args.lglobal   = params.global();
  TRANS_CHECK( ::trans_dirtrans(&args) );
}

///////////////////////////////////////////////////////////////////////////////

void TransIFS::dirtrans( const int nb_fields, const double wind_fields[], double vorticity_spectra[], double divergence_spectra[],
                         const eckit::Configuration& config ) const
{
  TransParameters params(config);
  struct ::DirTrans_t args = new_dirtrans(trans_.get());
    args.nvordiv = nb_fields;
    args.rspvor = vorticity_spectra;
    args.rspdiv = divergence_spectra;
    args.rgp    = wind_fields;
    args.lglobal = params.global();
  TRANS_CHECK( ::trans_dirtrans(&args) );
}

}
}


// anonymous namespace
namespace {

struct PackNodeColumns
{
  ArrayView<double,2>& rgpview_;
  IsGhostNode is_ghost;
  size_t f;

  PackNodeColumns( ArrayView<double,2>& rgpview, const NodeColumns& fs ) :
    rgpview_(rgpview), is_ghost( fs.nodes() ), f(0) {}

  void operator()(const Field& field, int components = 0) {
    switch (field.rank()) {
      case 1:
        pack_1(field,components);
        break;
      case 2:
        pack_2(field,components);
        break;
      case 3:
        pack_3(field,components);
        break;
      default:
        ATLAS_DEBUG_VAR(field.rank());
        NOTIMP;
      break;
    }
  }

  void pack_1(const Field& field, int)
  {
    const ArrayView<double,1> gpfield = make_view<double,1>( field );
    size_t n=0;
    for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
    {
      if( !is_ghost(jnode) )
      {
        rgpview_(f,n) = gpfield(jnode);
        ++n;
      }
    }
    ++f;
  }
  void pack_2(const Field& field, int)
  {
    const ArrayView<double,2> gpfield = make_view<double,2>( field );
    const size_t nvars = gpfield.shape(1);
    for( size_t jvar=0; jvar<nvars; ++jvar )
    {
      size_t n=0;
      for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
      {
        if( !is_ghost(jnode) )
        {
          rgpview_(f,n) = gpfield(jnode,jvar);
          ++n;
        }
      }
      ++f;
    }
  }
  void pack_3(const Field& field, int components)
  {
    const ArrayView<double,3> gpfield = make_view<double,3>( field );
    if( not components ) components = gpfield.shape(2);
    for( size_t jcomp=0; jcomp<size_t(components); ++jcomp )
    {
      for( size_t jlev=0; jlev<gpfield.shape(1); ++jlev )
      {
        size_t n = 0;
        for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
        {
          if( !is_ghost(jnode) )
          {
            rgpview_(f,n) = gpfield(jnode,jlev,jcomp);
            ++n;
          }
        }
        ++f;
      }
    }
  }
};


struct PackStructuredColumns
{
  ArrayView<double,2>& rgpview_;
  size_t f;

  PackStructuredColumns( ArrayView<double,2>& rgpview ) :
    rgpview_(rgpview), f(0) {}

  void operator()(const Field& field) {
    switch (field.rank()) {
      case 1:
        pack_1(field);
        break;
      case 2:
        pack_2(field);
        break;
      default:
        ATLAS_DEBUG_VAR(field.rank());
        NOTIMP;
        break;
    }
  }

  void pack_1(const Field& field)
  {
    const ArrayView<double,1> gpfield = make_view<double,1>( field );
    size_t n=0;
    for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
    {
      rgpview_(f,n) = gpfield(jnode);
      ++n;
    }
    ++f;
  }
  void pack_2(const Field& field)
  {
    const ArrayView<double,2> gpfield = make_view<double,2>( field );
    const size_t nvars = gpfield.shape(1);
    for( size_t jvar=0; jvar<nvars; ++jvar )
    {
      size_t n=0;
      for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
      {
        rgpview_(f,n) = gpfield(jnode,jvar);
        ++n;
      }
      ++f;
    }
  }
};

struct PackSpectral
{
  ArrayView<double,2>& rspecview_;
  size_t f;
  PackSpectral( ArrayView<double,2>& rspecview ) :
    rspecview_(rspecview), f(0) {}

  void operator()(const Field& field) {
    switch (field.rank()) {
      case 1:
        pack_1(field);
        break;
      case 2:
        pack_2(field);
        break;
      default:
        ATLAS_DEBUG_VAR(field.rank());
        NOTIMP;
        break;
    }
  }

  void pack_1(const Field& field)
  {
    const ArrayView<double,1> spfield = make_view<double,1>( field );

    for( size_t jwave=0; jwave<spfield.shape(0); ++jwave )
    {
      rspecview_(jwave,f) = spfield(jwave);
    }
    ++f;
  }
  void pack_2(const Field& field)
  {
    const ArrayView<double,2> spfield = make_view<double,2>( field );

    const size_t nvars = spfield.shape(1);

    for( size_t jvar=0; jvar<nvars; ++jvar )
    {
      for( size_t jwave=0; jwave<spfield.shape(0); ++jwave )
      {
        rspecview_(jwave,f) = spfield(jwave,jvar);
      }
      ++f;
    }
  }

};

struct UnpackNodeColumns
{
  const ArrayView<double,2>& rgpview_;
  IsGhostNode is_ghost;
  size_t f;

  UnpackNodeColumns( const ArrayView<double,2>& rgpview, const NodeColumns& fs ) :
    rgpview_(rgpview), is_ghost( fs.nodes() ), f(0) {}

  void operator()(Field& field, int components = 0) {
    switch (field.rank()) {
      case 1:
        unpack_1(field,components);
        break;
      case 2:
        unpack_2(field,components);
        break;
      case 3:
        unpack_3(field,components);
        break;
      default:
        ATLAS_DEBUG_VAR(field.rank());
        NOTIMP;
        break;
    }
  }

  void unpack_1(Field& field, int)
  {
    ArrayView<double,1> gpfield = make_view<double,1>( field );
    size_t n(0);
    for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
    {
      if( !is_ghost(jnode) )
      {
        gpfield(jnode) = rgpview_(f,n);
        ++n;
      }
    }
    ++f;
  }
  void unpack_2(Field& field, int)
  {
    ArrayView<double,2> gpfield = make_view<double,2>( field );
    const size_t nvars = gpfield.shape(1);
    for( size_t jvar=0; jvar<nvars; ++jvar )
    {
      int n=0;
      for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
      {
        if( !is_ghost(jnode) )
        {
          gpfield(jnode,jvar) = rgpview_(f,n);
          ++n;
        }
      }
      ++f;
    }
  }
  void unpack_3(Field& field, int components)
  {
    ArrayView<double,3> gpfield = make_view<double,3>( field );
    if( not components ) components = gpfield.shape(2);
    for( size_t jcomp=0; jcomp<size_t(components); ++jcomp )
    {
      for( size_t jlev=0; jlev<gpfield.shape(1); ++jlev )
      {
        size_t n = 0;
        for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
        {
          if( !is_ghost(jnode) )
          {
            gpfield(jnode,jlev,jcomp) = rgpview_(f,n);
            ++n;
          }
        }
        ++f;
      }
    }
  }
};

struct UnpackStructuredColumns
{
  const ArrayView<double,2>& rgpview_;
  size_t f;

  UnpackStructuredColumns( const ArrayView<double,2>& rgpview ) :
    rgpview_(rgpview), f(0) {}

  void operator()(Field& field) {
    switch (field.rank()) {
      case 1:
        unpack_1(field);
        break;
      case 2:
        unpack_2(field);
        break;
      default:
        ATLAS_DEBUG_VAR(field.rank());
        NOTIMP;
        break;
    }
  }

  void unpack_1(Field& field)
  {
    ArrayView<double,1> gpfield = make_view<double,1>( field );
    size_t n=0;
    for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
    {
      gpfield(jnode) = rgpview_(f,n);
      ++n;
    }
    ++f;
  }
  void unpack_2(Field& field)
  {
    ArrayView<double,2> gpfield = make_view<double,2>( field );
    const size_t nvars = gpfield.shape(1);
    for( size_t jvar=0; jvar<nvars; ++jvar )
    {
      size_t n=0;
      for( size_t jnode=0; jnode<gpfield.shape(0); ++jnode )
      {
        gpfield(jnode,jvar) = rgpview_(f,n);
        ++n;
      }
      ++f;
    }
  }
};

struct UnpackSpectral
{
  const ArrayView<double,2>& rspecview_;
  size_t f;
  UnpackSpectral( const ArrayView<double,2>& rspecview ) :
    rspecview_(rspecview), f(0) {}

  void operator()(Field& field) {
    switch (field.rank()) {
      case 1:
        unpack_1(field);
        break;
      case 2:
        unpack_2(field);
        break;
      default:
        ATLAS_DEBUG_VAR(field.rank());
        NOTIMP;
        break;
    }
  }

  void unpack_1(Field& field)
  {
    ArrayView<double,1> spfield = make_view<double,1>( field );

    for( size_t jwave=0; jwave<spfield.shape(0); ++jwave )
    {
      spfield(jwave) = rspecview_(jwave,f);
    }
    ++f;
  }
  void unpack_2(Field& field)
  {
    ArrayView<double,2> spfield = make_view<double,2>( field );

    const size_t nvars = spfield.shape(1);

    for( size_t jvar=0; jvar<nvars; ++jvar )
    {
      for( size_t jwave=0; jwave<spfield.shape(0); ++jwave )
      {
        spfield(jwave,jvar) = rspecview_(jwave,f);
      }
      ++f;
    }
  }

};


} // end anonymous namespace



namespace atlas {
namespace trans {


void TransIFS::assertCompatibleDistributions( const FunctionSpace& gp, const FunctionSpace& /*sp*/ ) const {
  std::string gp_dist = gp.distribution();
  if( gp_dist != "trans"  &&  // distribution computed by TransPartitioner
      gp_dist != "serial" &&  // serial distribution always works
      gp_dist != "custom" ) { // trust user that he knows what he is doing
    throw eckit::Exception(
          gp.type() + " functionspace has unsupported distribution ("+gp_dist+") "
          "to do spectral transforms. Please partition grid with TransPartitioner",
          Here());
  }
}

TransIFS::TransIFS( const Cache& cache, const Grid& grid, const long truncation, const eckit::Configuration& config ) :
  grid_(grid),
  cache_( cache.legendre().data() ),
  cachesize_( cache.legendre().size() ) {
  ASSERT( grid.domain().global() );
  ASSERT( not grid.projection() );
  ctor( grid, truncation, config );
}

TransIFS::TransIFS( const Grid& grid, const long truncation, const eckit::Configuration& config ) :
  TransIFS( Cache(), grid, truncation, config ) {
  ASSERT( grid.domain().global() );
  ASSERT( not grid.projection() );
  ctor( grid, truncation, config );
}

TransIFS::TransIFS(const Grid& grid, const eckit::Configuration& config ) :
  TransIFS( grid, /*grid-only*/-1, config ) {
}


TransIFS::~TransIFS()
{
}

void TransIFS::ctor( const Grid& grid, long truncation, const eckit::Configuration& config ) {
  trans_ = std::shared_ptr<::Trans_t>( new ::Trans_t, [](::Trans_t* p) {
    ::trans_delete(p);
    delete p;
  });

  if( auto gg = grid::GaussianGrid(grid) ) {
    ctor_rgg(gg.ny(), gg.nx().data(), truncation, config );
    return;
  }
  if( auto ll = grid::RegularLonLatGrid(grid) ) {
    if( ll.standard() || ll.shifted() ) {
      ctor_lonlat( ll.nx(), ll.ny(), truncation, config );
      return;
    }
  }
  throw eckit::NotImplemented("Grid type not supported for Spectral Transforms",Here());
}

void TransIFS::ctor_spectral_only(long truncation, const eckit::Configuration& )
{
  trans_ = std::shared_ptr<::Trans_t>( new ::Trans_t, [](::Trans_t* p) {
    ::trans_delete(p);
    delete p;
  });
  TRANS_CHECK(::trans_new(trans_.get()));
  TRANS_CHECK(::trans_set_trunc(trans_.get(),truncation));
  TRANS_CHECK(::trans_use_mpi(parallel::mpi::comm().size()>1));
  TRANS_CHECK(::trans_setup(trans_.get()));
}

void TransIFS::ctor_rgg(const long nlat, const long pl[], long truncation, const eckit::Configuration& config )
{
  TransParameters p(config);
  std::vector<int> nloen(nlat);
  for( long jlat=0; jlat<nlat; ++jlat )
    nloen[jlat] = pl[jlat];
  TRANS_CHECK(::trans_new(trans_.get()));
  TRANS_CHECK(::trans_set_resol(trans_.get(),nlat,nloen.data()));
  if( truncation >= 0 )
    TRANS_CHECK(::trans_set_trunc(trans_.get(),truncation));

  TRANS_CHECK(::trans_set_cache(trans_.get(),cache_,cachesize_));

  if( p.read_legendre().size() && parallel::mpi::comm().size() == 1 )
  {
    eckit::PathName file( p.read_legendre() );
    if( not file.exists() )
    {
      std::stringstream msg; msg << "File " << file << " doesn't exist";
      throw eckit::CantOpenFile(msg.str(),Here());
    }
    TRANS_CHECK(::trans_set_read(trans_.get(),file.asString().c_str()));
  }
  if( p.write_legendre().size() && parallel::mpi::comm().size() == 1 ) {
    eckit::PathName file( p.write_legendre() );
    TRANS_CHECK(::trans_set_write(trans_.get(),file.asString().c_str()));
  }

  trans_->fft = p.fft();
  trans_->lsplit = p.split_latitudes();
  trans_->flt = p.flt();

  TRANS_CHECK(::trans_use_mpi(parallel::mpi::comm().size()>1));
  TRANS_CHECK(::trans_setup(trans_.get()));
}

void TransIFS::ctor_lonlat(const long nlon, const long nlat, long truncation, const eckit::Configuration& config )
{
  TransParameters p(config);
  TRANS_CHECK(::trans_new(trans_.get()));
  TRANS_CHECK(::trans_set_resol_lonlat(trans_.get(),nlon,nlat));
  if( truncation >= 0 )
    TRANS_CHECK(::trans_set_trunc(trans_.get(),truncation));
  TRANS_CHECK(::trans_set_cache(trans_.get(),cache_,cachesize_));

  if( p.read_legendre().size() && parallel::mpi::comm().size() == 1 ) {
    eckit::PathName file( p.read_legendre() );
    if( not file.exists() )
    {
      std::stringstream msg; msg << "File " << file << " doesn't exist";
      throw eckit::CantOpenFile(msg.str(),Here());
    }
    TRANS_CHECK(::trans_set_read(trans_.get(),file.asString().c_str()));
  }
  if( p.write_legendre().size() && parallel::mpi::comm().size() == 1 ) {
    eckit::PathName file( p.write_legendre() );
    TRANS_CHECK(::trans_set_write(trans_.get(),file.asString().c_str()));
  }

  trans_->fft = p.fft();
  trans_->lsplit = p.split_latitudes();
  trans_->flt = p.flt();

  TRANS_CHECK(::trans_use_mpi(parallel::mpi::comm().size()>1));
  TRANS_CHECK(::trans_setup(trans_.get()));
}


// --------------------------------------------------------------------------------------------



void TransIFS::__dirtrans( const functionspace::NodeColumns& gp, const Field& gpfield,
                           const Spectral& sp, Field& spfield, const eckit::Configuration& config ) const
{
  FieldSet gpfields; gpfields.add(gpfield);
  FieldSet spfields; spfields.add(spfield);
  __dirtrans(gp,gpfields,sp,spfields,config);
}


// --------------------------------------------------------------------------------------------

void TransIFS::__dirtrans( const functionspace::NodeColumns& gp,const FieldSet& gpfields,
                           const Spectral& sp, FieldSet& spfields, const eckit::Configuration& ) const
{
  assertCompatibleDistributions( gp, sp );

  // Count total number of fields and do sanity checks
  int nfld(0);
  for(size_t jfld = 0; jfld < gpfields.size(); ++jfld)
  {
    const Field& f = gpfields[jfld];
    nfld += f.stride(0);
  }

  int trans_spnfld(0);
  for(size_t jfld = 0; jfld < spfields.size(); ++jfld)
  {
    const Field& f = spfields[jfld];
    trans_spnfld += f.stride(0);
  }

  if( nfld != trans_spnfld )
  {
    throw eckit::SeriousBug("dirtrans: different number of gridpoint fields than spectral fields",Here());
  }
  // Arrays Trans expects
  array::ArrayT<double> rgp(nfld,ngptot());
  array::ArrayT<double> rspec(nspec2(),nfld);

  array::ArrayView<double,2> rgpview   = array::make_view<double,2>(rgp);
  array::ArrayView<double,2> rspecview = array::make_view<double,2>(rspec);

  // Pack gridpoints
  {
    PackNodeColumns pack(rgpview,gp);
    for( size_t jfld=0; jfld<gpfields.size(); ++jfld )
      pack( gpfields[jfld]);
  }

  // Do transform
  {
    struct ::DirTrans_t transform = ::new_dirtrans(trans_.get());
    transform.nscalar    = nfld;
    transform.rgp        = rgp.data<double>();
    transform.rspscalar  = rspec.data<double>();

    TRANS_CHECK( ::trans_dirtrans(&transform) );
  }

  // Unpack the spectral fields
  {
    UnpackSpectral unpack(rspecview);
    for( size_t jfld=0; jfld<spfields.size(); ++jfld )
      unpack(spfields[jfld]);
  }

}

// --------------------------------------------------------------------------------------------

void TransIFS::__dirtrans(
    const StructuredColumns& gp, const Field& gpfield,
    const Spectral& sp,                Field& spfield,
    const eckit::Configuration& ) const
{
  ASSERT( gpfield.functionspace() == 0 ||
          functionspace::StructuredColumns(gpfield.functionspace()) );
  ASSERT( spfield.functionspace() == 0 ||
          functionspace::Spectral(spfield.functionspace()) );

  assertCompatibleDistributions( gp, sp );

  if ( gpfield.stride(0) != spfield.stride(0) )
  {
    throw eckit::SeriousBug("dirtrans: different number of gridpoint fields than spectral fields",Here());
  }
  if ( (int)gpfield.shape(0) != ngptot() )
  {
    throw eckit::SeriousBug("dirtrans: slowest moving index must be ngptot",Here());
  }
  const int nfld = gpfield.stride(0);

  // Do transform
  {
    struct ::DirTrans_t transform = ::new_dirtrans(trans_.get());
    transform.nscalar    = nfld;
    transform.rgp        = gpfield.data<double>();
    transform.rspscalar  = spfield.data<double>();
    transform.ngpblks    = gpfield.shape(0);
    transform.nproma     = 1;
    TRANS_CHECK( ::trans_dirtrans(&transform) );
  }
}

void TransIFS::__dirtrans(
    const StructuredColumns& gp, const FieldSet& gpfields,
    const Spectral& sp,                FieldSet& spfields,
    const eckit::Configuration& ) const
{
  assertCompatibleDistributions( gp, sp );

  // Count total number of fields and do sanity checks
  int nfld(0);
  for(size_t jfld = 0; jfld < gpfields.size(); ++jfld)
  {
    const Field& f = gpfields[jfld];
    nfld += f.stride(0);
    ASSERT( f.functionspace() == 0 ||
            functionspace::StructuredColumns(f.functionspace()) );
  }

  int trans_spnfld(0);
  for(size_t jfld = 0; jfld < spfields.size(); ++jfld)
  {
    const Field& f = spfields[jfld];
    trans_spnfld += f.stride(0);
  }

  if( nfld != trans_spnfld )
  {
    throw eckit::SeriousBug("dirtrans: different number of gridpoint fields than spectral fields",Here());
  }
  // Arrays Trans expects
  array::ArrayT<double> rgp(nfld,ngptot());
  array::ArrayT<double> rspec(nspec2(),nfld);

  array::ArrayView<double,2> rgpview   = array::make_view<double,2>(rgp);
  array::ArrayView<double,2> rspecview = array::make_view<double,2>(rspec);

  // Pack gridpoints
  {
    PackStructuredColumns pack(rgpview);
    for( size_t jfld=0; jfld<gpfields.size(); ++jfld )
      pack(gpfields[jfld]);
  }

  // Do transform
  {
    struct ::DirTrans_t transform = ::new_dirtrans(trans_.get());
    transform.nscalar    = nfld;
    transform.rgp        = rgp.data<double>();
    transform.rspscalar  = rspec.data<double>();

    TRANS_CHECK( ::trans_dirtrans(&transform) );
  }

  // Unpack the spectral fields
  {
    UnpackSpectral unpack(rspecview);
    for( size_t jfld=0; jfld<spfields.size(); ++jfld )
      unpack(spfields[jfld]);
  }
}

// --------------------------------------------------------------------------------------------

void TransIFS::__invtrans_grad(
    const Spectral& sp, const Field& spfield,
    const functionspace::NodeColumns& gp, Field& gradfield,
    const eckit::Configuration& config ) const
{
  FieldSet spfields;   spfields.  add(spfield);
  FieldSet gradfields; gradfields.add(gradfield);
  __invtrans_grad(sp,spfields,gp,gradfields, config);
}

void TransIFS::__invtrans_grad(
    const Spectral& sp, const FieldSet& spfields,
    const functionspace::NodeColumns& gp, FieldSet& gradfields,
    const eckit::Configuration& config ) const
{
  assertCompatibleDistributions( gp, sp );

  // Count total number of fields and do sanity checks
  int nb_gridpoint_field(0);
  for(size_t jfld = 0; jfld < gradfields.size(); ++jfld)
  {
    const Field& f = gradfields[jfld];
    nb_gridpoint_field += f.stride(0);
  }

  int nfld(0);
  for(size_t jfld = 0; jfld < spfields.size(); ++jfld)
  {
    const Field& f = spfields[jfld];
    nfld += f.stride(0);
    ASSERT( std::max<size_t>(1,f.levels()) == f.stride(0) );
  }

  if( nb_gridpoint_field != 2*nfld ) // factor 2 because N-S and E-W derivatives
    throw eckit::SeriousBug("invtrans_grad: different number of gridpoint fields than spectral fields",Here());

  // Arrays Trans expects
  // Allocate space for
  array::ArrayT<double> rgp(3*nfld,ngptot()); // (scalars) + (NS ders) + (EW ders)
  array::ArrayT<double> rspec(nspec2(),nfld);

  array::ArrayView<double,2> rgpview   = array::make_view<double,2>(rgp);
  array::ArrayView<double,2> rspecview = array::make_view<double,2>(rspec);

  // Pack spectral fields
  {
    PackSpectral pack(rspecview);
    for(size_t jfld = 0; jfld < spfields.size(); ++jfld)
      pack(spfields[jfld]);
  }

  // Do transform
  {
    struct ::InvTrans_t transform = ::new_invtrans(trans_.get());
    transform.nscalar     = nfld;
    transform.rgp         = rgp.data<double>();
    transform.rspscalar   = rspec.data<double>();
    transform.lscalarders = true;

    TRANS_CHECK(::trans_invtrans(&transform));
  }

  // Unpack the gridpoint fields
  {
    mesh::IsGhostNode is_ghost( gp.nodes());
    int f=nfld; // skip to where derivatives start
    for(size_t dim=0; dim<2; ++dim) {
      for(size_t jfld = 0; jfld < gradfields.size(); ++jfld)
      {
        const size_t nlev = std::max<size_t>(1,gradfields[jfld].levels());
        const size_t nb_nodes  = gradfields[jfld].shape(0);

        array::LocalView<double,3> field ( gradfields[jfld].data<double>(),
           array::make_shape(nb_nodes, nlev, 2 ) );

        for( size_t jlev=0; jlev<nlev; ++jlev )
        {
          int n=0;
          for( size_t jnode=0; jnode<nb_nodes; ++jnode )
          {
            if( !is_ghost(jnode) )
            {
              field(jnode,jlev,1-dim) = rgpview(f,n);
              ++n;
            }
          }
          ASSERT( n == ngptot() );
          ++f;
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------------------

void TransIFS::__invtrans( const Spectral& sp, const Field& spfield,
                           const functionspace::NodeColumns& gp, Field& gpfield,
                           const eckit::Configuration& config ) const
{
  FieldSet spfields; spfields.add(spfield);
  FieldSet gpfields; gpfields.add(gpfield);
  __invtrans(sp,spfields,gp,gpfields,config);
}


// --------------------------------------------------------------------------------------------


void TransIFS::__invtrans( const Spectral& sp, const FieldSet& spfields,
                           const functionspace::NodeColumns& gp, FieldSet& gpfields,
                           const eckit::Configuration& config ) const
{
  assertCompatibleDistributions( gp, sp );

  // Count total number of fields and do sanity checks
  int nfld(0);
  for(size_t jfld = 0; jfld < gpfields.size(); ++jfld)
  {
    const Field& f = gpfields[jfld];
    nfld += f.stride(0);
  }

  int nb_spectral_fields(0);
  for(size_t jfld = 0; jfld < spfields.size(); ++jfld)
  {
    const Field& f = spfields[jfld];
    nb_spectral_fields += f.stride(0);
  }

  if( nfld != nb_spectral_fields )
    throw eckit::SeriousBug("invtrans: different number of gridpoint fields than spectral fields",Here());

  // Arrays Trans expects
  array::ArrayT<double> rgp(nfld,ngptot());
  array::ArrayT<double> rspec(nspec2(),nfld);

  array::ArrayView<double,2> rgpview   = array::make_view<double,2>(rgp);
  array::ArrayView<double,2> rspecview = array::make_view<double,2>(rspec);

  // Pack spectral fields
  {
    PackSpectral pack(rspecview);
    for(size_t jfld = 0; jfld < spfields.size(); ++jfld)
      pack(spfields[jfld]);
  }

  // Do transform
  {
    struct ::InvTrans_t transform = ::new_invtrans(trans_.get());
    transform.nscalar    = nfld;
    transform.rgp        = rgp.data<double>();
    transform.rspscalar  = rspec.data<double>();

    TRANS_CHECK(::trans_invtrans(&transform));
  }

  // Unpack the gridpoint fields
  {
    UnpackNodeColumns unpack(rgpview,gp);
    for(size_t jfld = 0; jfld < gpfields.size(); ++jfld)
      unpack(gpfields[jfld]);
  }

}

// --------------------------------------------------------------------------------------------


void TransIFS::__invtrans( const functionspace::Spectral& sp, const Field& spfield,
                           const functionspace::StructuredColumns& gp, Field& gpfield,
                           const eckit::Configuration& config ) const
{
  assertCompatibleDistributions( gp, sp );

  ASSERT( gpfield.functionspace() == 0 ||
          functionspace::StructuredColumns( gpfield.functionspace() ) );
  ASSERT( spfield.functionspace() == 0 ||
          functionspace::Spectral( spfield.functionspace() ) );
  if ( gpfield.stride(0) != spfield.stride(0) )
  {
    throw eckit::SeriousBug("dirtrans: different number of gridpoint fields than spectral fields",Here());
  }
  if ( (int)gpfield.shape(0) != ngptot() )
  {
    throw eckit::SeriousBug("dirtrans: slowest moving index must be ngptot",Here());
  }
  const int nfld = gpfield.stride(0);

  // Do transform
  {
    struct ::InvTrans_t transform = ::new_invtrans(trans_.get());
    transform.nscalar    = nfld;
    transform.rgp        = gpfield.data<double>();
    transform.rspscalar  = spfield.data<double>();
    transform.ngpblks    = gpfield.shape(0);
    transform.nproma     = 1;
    TRANS_CHECK( ::trans_invtrans(&transform) );
  }
}


// --------------------------------------------------------------------------------------------

void TransIFS::__invtrans( const functionspace::Spectral& sp, const FieldSet& spfields,
                           const functionspace::StructuredColumns& gp, FieldSet& gpfields,
                           const eckit::Configuration& config ) const
{
  assertCompatibleDistributions( gp, sp );

  // Count total number of fields and do sanity checks
  int nfld(0);
  for(size_t jfld = 0; jfld < gpfields.size(); ++jfld)
  {
    const Field& f = gpfields[jfld];
    nfld += f.stride(0);
    ASSERT( f.functionspace() == 0 ||
            functionspace::StructuredColumns( f.functionspace() ) );
  }

  int nb_spectral_fields(0);
  for(size_t jfld = 0; jfld < spfields.size(); ++jfld)
  {
    const Field& f = spfields[jfld];
    nb_spectral_fields += f.stride(0);
  }

  if( nfld != nb_spectral_fields ) {
    std::stringstream msg;
    msg << "invtrans: different number of gridpoint fields than spectral fields"
        << "[ " << nfld << " != " << nb_spectral_fields << " ]";
    throw eckit::SeriousBug(msg.str(),Here());
  }

  // Arrays Trans expects
  array::ArrayT<double> rgp(nfld,ngptot());
  array::ArrayT<double> rspec(nspec2(),nfld);

  array::ArrayView<double,2> rgpview   = array::make_view<double,2>(rgp);
  array::ArrayView<double,2> rspecview = array::make_view<double,2>(rspec);

  // Pack spectral fields
  {
    PackSpectral pack(rspecview);
    for(size_t jfld = 0; jfld < spfields.size(); ++jfld)
      pack(spfields[jfld]);
  }

  // Do transform
  {
    struct ::InvTrans_t transform = ::new_invtrans(trans_.get());
    transform.nscalar    = nfld;
    transform.rgp        = rgp.data<double>();
    transform.rspscalar  = rspec.data<double>();

    TRANS_CHECK(::trans_invtrans(&transform));
  }

  // Unpack the gridpoint fields
  {
    UnpackStructuredColumns unpack(rgpview);
    for(size_t jfld = 0; jfld < gpfields.size(); ++jfld)
      unpack(gpfields[jfld]);
  }
}

// -----------------------------------------------------------------------------------------------

void TransIFS::__dirtrans_wind2vordiv( const functionspace::NodeColumns& gp, const Field& gpwind,
                                       const Spectral& sp, Field& spvor, Field&spdiv,
                                       const eckit::Configuration& ) const
{
  assertCompatibleDistributions( gp, sp );

  // Count total number of fields and do sanity checks
  size_t nfld = spvor.stride(0);
  if( spdiv.shape(0) != spvor.shape(0) ) throw eckit::SeriousBug("invtrans: vorticity not compatible with divergence.",Here());
  if( spdiv.shape(1) != spvor.shape(1) ) throw eckit::SeriousBug("invtrans: vorticity not compatible with divergence.",Here());
  size_t nwindfld = gpwind.stride(0);
  if (nwindfld != 2*nfld && nwindfld != 3*nfld) throw eckit::SeriousBug("dirtrans: wind field is not compatible with vorticity, divergence.",Here());

  if( spdiv.shape(0) != size_t(nspec2()) ) {
    std::stringstream msg;
    msg << "dirtrans: Spectral vorticity and divergence have wrong dimension: nspec2 "<<spdiv.shape(0)<<" should be "<<nspec2();
    throw eckit::SeriousBug(msg.str(),Here());
  }

  if( spvor.size() == 0 ) throw eckit::SeriousBug("dirtrans: spectral vorticity field is empty.");
  if( spdiv.size() == 0 ) throw eckit::SeriousBug("dirtrans: spectral divergence field is empty.");

  // Arrays Trans expects
  array::ArrayT<double> rgp(2*nfld,size_t(ngptot()));
  array::ArrayView<double,2> rgpview = array::make_view<double,2>(rgp);

  // Pack gridpoints
  {
    PackNodeColumns pack( rgpview, gp );
    int wind_components = 2;
    pack(gpwind, wind_components);
  }

  // Do transform
  {
    struct ::DirTrans_t transform = ::new_dirtrans(trans_.get());
    transform.nvordiv = nfld;
    transform.rgp     = rgp.data<double>();
    transform.rspvor  = spvor.data<double>();
    transform.rspdiv  = spdiv.data<double>();

    ASSERT( transform.rspvor );
    ASSERT( transform.rspdiv );
    TRANS_CHECK( ::trans_dirtrans(&transform) );
  }
}


void TransIFS::__invtrans_vordiv2wind( const Spectral& sp, const Field& spvor, const Field& spdiv,
                                       const functionspace::NodeColumns& gp, Field& gpwind,
                                       const eckit::Configuration& config ) const
{
  assertCompatibleDistributions( gp, sp );

  // Count total number of fields and do sanity checks
  size_t nfld = spvor.stride(0);
  if( spdiv.shape(0) != spvor.shape(0) ) throw eckit::SeriousBug("invtrans: vorticity not compatible with divergence.",Here());
  if( spdiv.shape(1) != spvor.shape(1) ) throw eckit::SeriousBug("invtrans: vorticity not compatible with divergence.",Here());
  size_t nwindfld = gpwind.stride(0);
  if (nwindfld != 2*nfld && nwindfld != 3*nfld) throw eckit::SeriousBug("invtrans: wind field is not compatible with vorticity, divergence.",Here());

  if( spdiv.shape(0) != size_t(nspec2()) ) {
    std::stringstream msg;
    msg << "invtrans: Spectral vorticity and divergence have wrong dimension: nspec2 "<<spdiv.shape(0)<<" should be "<<nspec2();
    throw eckit::SeriousBug(msg.str(),Here());
  }

  ASSERT( spvor.rank() == 2 );
  ASSERT( spdiv.rank() == 2 );
  if( spvor.size() == 0 ) throw eckit::SeriousBug("invtrans: spectral vorticity field is empty.");
  if( spdiv.size() == 0 ) throw eckit::SeriousBug("invtrans: spectral divergence field is empty.");

  // Arrays Trans expects
  array::ArrayT<double> rgp(2*nfld,size_t(ngptot()));
  array::ArrayView<double,2> rgpview = array::make_view<double,2>(rgp);

  // Do transform
  {
    struct ::InvTrans_t transform = ::new_invtrans(trans_.get());
    transform.nvordiv = nfld;
    transform.rgp     = rgp.data<double>();
    transform.rspvor  = spvor.data<double>();
    transform.rspdiv  = spdiv.data<double>();

    ASSERT( transform.rspvor );
    ASSERT( transform.rspdiv );
    TRANS_CHECK(::trans_invtrans(&transform));
  }

  // Unpack the gridpoint fields
  {
    UnpackNodeColumns unpack( rgpview, gp );
    int wind_components = 2;
    unpack(gpwind,wind_components);
  }

}

///////////////////////////////////////////////////////////////////////////////


void TransIFS::distspec( const int nb_fields, const int origin[], const double global_spectra[], double spectra[] ) const
{
  struct ::DistSpec_t args = new_distspec(trans_.get());
    args.nfld = nb_fields;
    args.rspecg = global_spectra;
    args.nfrom = origin;
    args.rspec = spectra;
  TRANS_CHECK( ::trans_distspec(&args) );
}

/////////////////////////////////////////////////////////////////////////////

void TransIFS::gathspec( const int nb_fields, const int destination[], const double spectra[], double global_spectra[] ) const
{
  struct ::GathSpec_t args = new_gathspec(trans_.get());
    args.nfld = nb_fields;
    args.rspecg = global_spectra;
    args.nto = destination;
    args.rspec = spectra;
  TRANS_CHECK( ::trans_gathspec(&args) );
}

/////////////////////////////////////////////////////////////////////////////

void TransIFS::distgrid( const int nb_fields, const int origin[], const double global_fields[], double fields[] ) const
{
  struct ::DistGrid_t args = new_distgrid(trans_.get());
    args.nfld  = nb_fields;
    args.nfrom = origin;
    args.rgpg  = global_fields;
    args.rgp   = fields;
  TRANS_CHECK( ::trans_distgrid(&args) );
}

/////////////////////////////////////////////////////////////////////////////

void TransIFS::gathgrid( const int nb_fields, const int destination[], const double fields[], double global_fields[] ) const
{
  struct ::GathGrid_t args = new_gathgrid(trans_.get());
    args.nfld = nb_fields;
    args.nto  = destination;
    args.rgp  = fields;
    args.rgpg = global_fields;
  TRANS_CHECK( ::trans_gathgrid(&args) );
}

///////////////////////////////////////////////////////////////////////////////


void TransIFS::specnorm( const int nb_fields, const double spectra[], double norms[], int rank ) const
{
  struct ::SpecNorm_t args = new_specnorm(trans_.get());
    args.nfld = nb_fields;
    args.rspec = spectra;
    args.rnorm = norms;
    args.nmaster = rank+1;
  TRANS_CHECK( ::trans_specnorm(&args) );
}

///////////////////////////////////////////////////////////////////////////////

extern "C" {

TransIFS* atlas__Trans__new (const Grid::Implementation* grid, int nsmax)
{
  TransIFS* trans(0);
  ATLAS_ERROR_HANDLING(
    ASSERT( grid );
    trans = new TransIFS( Grid(grid) ,nsmax);
  );
  return trans;
}

void atlas__Trans__delete (TransIFS* This)
{
  ASSERT( This );
  ATLAS_ERROR_HANDLING( delete This );
}

int atlas__Trans__handle (const TransIFS* This)
{
  ASSERT( This );
  ATLAS_ERROR_HANDLING(
    ::Trans_t* t = *This;
    return t->handle;
  );
  return 0;
}

void atlas__Trans__distspec( const TransIFS* t, int nb_fields, int origin[], double global_spectra[], double spectra[] )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    struct ::DistSpec_t args = new_distspec( t->trans() );
      args.nfld = nb_fields;
      args.rspecg = global_spectra;
      args.nfrom = origin;
      args.rspec = spectra;
    TRANS_CHECK( ::trans_distspec(&args) );
  );
}

void atlas__Trans__gathspec( const TransIFS* t, int nb_fields, int destination[], double spectra[], double global_spectra[] )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    struct ::GathSpec_t args = new_gathspec( t->trans() );
      args.nfld = nb_fields;
      args.rspecg = global_spectra;
      args.nto = destination;
      args.rspec = spectra;
    TRANS_CHECK( ::trans_gathspec(&args) );
  );
}

void atlas__Trans__distgrid( const TransIFS* t, int nb_fields, int origin[], double global_fields[], double fields[] )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    struct ::DistGrid_t args = new_distgrid( t->trans() );
      args.nfld  = nb_fields;
      args.nfrom = origin;
      args.rgpg  = global_fields;
      args.rgp   = fields;
    TRANS_CHECK( ::trans_distgrid(&args) );
  );
}

void atlas__Trans__gathgrid( const TransIFS* t, int nb_fields, int destination[], double fields[], double global_fields[] )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    struct ::GathGrid_t args = new_gathgrid( t->trans() );
      args.nfld = nb_fields;
      args.nto  = destination;
      args.rgp  = fields;
      args.rgpg = global_fields;
    TRANS_CHECK( ::trans_gathgrid(&args) );
  );
}

void atlas__Trans__invtrans_scalar( const TransIFS* t, int nb_fields, double scalar_spectra[], double scalar_fields[] )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    return t->invtrans(nb_fields,scalar_spectra,scalar_fields);
  );
}

void atlas__Trans__invtrans_vordiv2wind( const TransIFS* t, int nb_fields, double vorticity_spectra[], double divergence_spectra[], double wind_fields[] )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    return t->invtrans(nb_fields,vorticity_spectra,divergence_spectra,wind_fields);
  );
}

void atlas__Trans__dirtrans_scalar( const TransIFS* t, int nb_fields, double scalar_fields[], double scalar_spectra[] )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    return t->dirtrans(nb_fields,scalar_fields,scalar_spectra);
  );
}

void atlas__Trans__dirtrans_wind2vordiv( const TransIFS* t, int nb_fields, double wind_fields[], double vorticity_spectra[], double divergence_spectra[] )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    return t->dirtrans(nb_fields,wind_fields,vorticity_spectra,divergence_spectra);
  );
}

void atlas__Trans__specnorm (const TransIFS* t, int nb_fields, double spectra[], double norms[], int rank)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( t );
    return t->specnorm(nb_fields, spectra, norms, rank);
  );
}

int atlas__Trans__nspec2 (const TransIFS* This)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    return This->trans()->nspec2;
  );
  return 0;
}

int atlas__Trans__nspec2g (const TransIFS* This)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    return This->trans()->nspec2g;
  );
  return 0;
}

int atlas__Trans__ngptot (const TransIFS* This)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    return This->trans()->ngptot;
  );
  return 0;
}

int atlas__Trans__ngptotg (const TransIFS* This)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
      return This->trans()->ngptotg;
  );
  return 0;
}

int atlas__Trans__truncation (const TransIFS* This)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    return This->truncation();
  );
  return 0;
}

const Grid::Implementation* atlas__Trans__grid(const TransIFS* This)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    ASSERT( This->grid() );
  ATLAS_DEBUG_VAR( This->grid().get()->owners() );
    return This->grid().get();
  );
  return nullptr;
}

void atlas__Trans__dirtrans_fieldset (const TransIFS* This, const field::FieldSetImpl* gpfields, field::FieldSetImpl* spfields, const eckit::Configuration* parameters)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    ASSERT( gpfields );
    ASSERT( spfields );
    ASSERT( parameters );
    FieldSet fspfields(spfields);
    This->dirtrans(gpfields,fspfields,*parameters);
  );
}

void atlas__Trans__dirtrans_field (const TransIFS* This, const field::FieldImpl* gpfield, field::FieldImpl* spfield, const eckit::Configuration* parameters)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    ASSERT( spfield );
    ASSERT( gpfield );
    ASSERT( parameters );
    Field fspfield(spfield);
    This->dirtrans(gpfield,fspfield,*parameters);
  );
}

void atlas__Trans__invtrans_fieldset (const TransIFS* This, const field::FieldSetImpl* spfields, field::FieldSetImpl* gpfields, const eckit::Configuration* parameters)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    ASSERT( spfields );
    ASSERT( gpfields );
    ASSERT( parameters );
    FieldSet fgpfields(gpfields);
    This->invtrans(spfields,fgpfields,*parameters);
  );
}

void atlas__Trans__invtrans_field (const TransIFS* This, const field::FieldImpl* spfield, field::FieldImpl* gpfield, const eckit::Configuration* parameters)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    ASSERT( spfield );
    ASSERT( gpfield );
    ASSERT( parameters );
    Field fgpfield(gpfield);
    This->invtrans(spfield,fgpfield,*parameters);
  );
}

void atlas__Trans__dirtrans_wind2vordiv_field (const TransIFS* This, const field::FieldImpl* gpwind, field::FieldImpl* spvor, field::FieldImpl* spdiv, const eckit::Configuration* parameters)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    ASSERT( gpwind );
    ASSERT( spvor );
    ASSERT( spdiv );
    ASSERT( parameters );
    Field fspvor(spvor);
    Field fspdiv(spdiv);
    This->dirtrans_wind2vordiv(gpwind,fspvor,fspdiv,*parameters);
  );
}

void atlas__Trans__invtrans_vordiv2wind_field (const TransIFS* This, const field::FieldImpl* spvor, const field::FieldImpl* spdiv, field::FieldImpl* gpwind, const eckit::Configuration* parameters)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    ASSERT( spvor );
    ASSERT( spdiv );
    ASSERT( gpwind );
    ASSERT( parameters );
    Field fgpwind(gpwind);
    This->invtrans_vordiv2wind(spvor,spdiv,fgpwind,*parameters);
  );
}

void atlas__Trans__invtrans (const TransIFS* This, int nb_scalar_fields, double scalar_spectra[], int nb_vordiv_fields, double vorticity_spectra[], double divergence_spectra[], double gp_fields[], const eckit::Configuration* parameters)
{
  ATLAS_ERROR_HANDLING(
    ASSERT(This);
      This->invtrans( nb_scalar_fields, scalar_spectra,
                      nb_vordiv_fields, vorticity_spectra, divergence_spectra,
                      gp_fields,
                      *parameters );
  );
}

void atlas__Trans__invtrans_grad_field (const TransIFS* This, const field::FieldImpl* spfield, field::FieldImpl* gpfield, const eckit::Configuration* config )
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    ASSERT( spfield );
    ASSERT( gpfield );
    Field fgpfield(gpfield);
    This->invtrans_grad(spfield,fgpfield,*config);
  );
}

}

} // namespace trans
} // namespace atlas
