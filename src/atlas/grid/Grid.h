/*
 * (C) Copyright 1996-2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Peter Bispham
/// @author Tiago Quintino
/// @date Oct 2013

#ifndef atlas_grid_Grid_H
#define atlas_grid_Grid_H

#include <cstddef>
#include <vector>
#include <cmath>

#include "eckit/memory/NonCopyable.h"
#include "eckit/exception/Exceptions.h"

#include "eckit/geometry/Point2.h"

#include "atlas/Field.hpp"

//------------------------------------------------------------------------------------------------------

namespace atlas {
namespace grid {

//------------------------------------------------------------------------------------------------------

/// Interface to a grid of points in a 2d cartesian space
/// For example a LatLon grid or a Reduced Graussian grid

class Grid : private eckit::NonCopyable {

public: // types

    typedef eckit::geometry::LLPoint            Point;     ///< point type
    typedef eckit::geometry::BoundBox2<Point>   BoundBox;  ///< boundbox type

public: // methods

    Grid();

    virtual ~Grid();

    virtual std::string hash() const = 0;

    virtual BoundBox boundingBox() const = 0;

    virtual size_t nbPoints() const = 0;

    virtual const std::vector<Point>& coordinates() const = 0;

protected:

};

//------------------------------------------------------------------------------------------------------

} // namespace grid
} // namespace atlas

#endif