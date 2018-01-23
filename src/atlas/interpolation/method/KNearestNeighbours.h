/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#pragma once

#include "atlas/interpolation/method/KNearestNeighboursBase.h"


namespace atlas {
namespace interpolation {
namespace method {


class KNearestNeighbours : public KNearestNeighboursBase {
public:

    KNearestNeighbours(const Config& config);
    virtual ~KNearestNeighbours() {}

    /**
     * @brief Create an interpolant sparse matrix relating two (pre-partitioned) meshes,
     * using the k-nearest neighbours method
     * @param source functionspace containing source elements
     * @param target functionspace containing target points
     */
    virtual void setup(const FunctionSpace& source, const FunctionSpace& target) override;

protected:

    size_t k_;

};


}  // method
}  // interpolation
}  // atlas
