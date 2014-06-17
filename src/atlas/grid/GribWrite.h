#ifndef atlas_GribWrite_h
#define atlas_GribWrite_h
/*
 * (C) Copyright 1996-2014 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "atlas/grid/FieldSet.h"

//------------------------------------------------------------------------------------------------------

namespace eckit { class GribHandle; }
namespace eckit { class PathName; }
namespace eckit { class DataHandle; }

namespace atlas {

//------------------------------------------------------------------------------------------------------

class GribWrite {

public: // methods

   /// Given a Grid, this function will find the closest matching GRIB samples file.
   /// The cloned/new handle of the GRIB sample file is returned.
   /// If no match found a NULL handle is returned.
   static eckit::GribHandle* create_handle( const grid::Grid& );

   // Given a GridSpec return closest grib samples file.
   // If no match found returns an empty string
   static std::string grib_sample_file( const grid::GridSpec& );

   static void write( const atlas::grid::FieldSet& field, const eckit::PathName& opath  );

   static void clone( const atlas::grid::FieldSet& field, const eckit::PathName& src, const eckit::PathName& opath  );

private: // methods

    static void write( const atlas::grid::FieldHandle& field, const eckit::PathName& opath  );

    static void clone( const atlas::grid::FieldHandle& field, const eckit::PathName& gridsec, eckit::DataHandle& );

    static eckit::GribHandle* clone(const grid::FieldHandle &field, eckit::GribHandle& gridsec );
};

//---------------------------------------------------------------------------------------------------------

} // namespace atlas

#endif

