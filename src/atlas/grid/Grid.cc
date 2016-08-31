/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include "atlas/grid/Grid.h"

#include <vector>
#include "eckit/memory/Factory.h"
#include "atlas/grid/grids.h"
#include "atlas/mesh/Mesh.h"


namespace atlas {
namespace grid {


static void checkSizeOfPoint() {
    // compile time check support C++11
#if __cplusplus >= 201103L
    static_assert( sizeof(Grid::Point)==2*sizeof(double), "Grid requires size of Point to be 2*double" );
#endif

    // runtime check
    ASSERT( sizeof(Grid::Point) == 2*sizeof(double) );
}


std::string Grid::className() {
    return "atlas.Grid";
}


Grid* Grid::create(const eckit::Parametrisation& params) {
    eckit::Factory<Grid>& fact = eckit::Factory<Grid>::instance();

    std::string shortName;
    if (params.get("short_name",shortName) && fact.exists(shortName)) {
        return fact.get(shortName).create(params);
    }
    std::string gridType;
    if (params.get("grid_type", gridType) && fact.exists(gridType)) {
        return fact.get(gridType).create(params);
    }
    return NULL;
}


Grid* Grid::create(const Grid::uid_t& uid) {
    return grid::grid_from_uid(uid);
}


Grid::Grid()
{
    checkSizeOfPoint();
}

Grid::~Grid() {
}


Grid::uid_t Grid::uniqueId() const {
    if (uid_.empty()) {
        std::ostringstream s;
        s << shortName() << "." << hash();
        uid_ = s.str();
    }
    return uid_;
}


eckit::MD5::digest_t Grid::hash() const {
    if (hash_.empty()) {
        eckit::MD5 md5;
        hash(md5);
        hash_ = md5.digest();
    }
    return hash_;
}


void Grid::fillLonLat(double array[], size_t arraySize) const {
    const size_t size = npts()*2;
    ASSERT(arraySize >= size);
    copyLonLatMemory(array, size_t(sizeof(double)*size));
}


std::string Grid::getOptimalMeshGenerator() const {
    return "Delaunay";
}


void Grid::fillLonLat(std::vector<double>& v) const {
    v.resize(npts()*2);
    copyLonLatMemory(&v[0], size_t(sizeof(double)*v.size()));
}


size_t Grid::copyLonLatMemory(double* pts, size_t size) const {
    std::vector<Grid::Point> gpts;
    lonlat(gpts);

    size_t sizePts = 2*npts();

    ASSERT(size >= sizePts);

    for(size_t c = 0, i = 0; i < gpts.size(); ++i) {
        pts[c++] = gpts[i].lon();
        pts[c++] = gpts[i].lat();
    }

    return sizePts;
}


bool Grid::same(const grid::Grid& g) const {
    return uniqueId() == g.uniqueId();
}


}  // namespace grid
}  // namespace atlas
