/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

#include "eckit/config/Resource.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/log/Bytes.h"
#include "eckit/log/Log.h"
#include "eckit/parser/JSON.h"
#include "eckit/runtime/Main.h"
#include "eckit/types/FloatCompare.h"

#include "atlas/grid.h"
#include "atlas/grid/detail/grid/GridFactory.h"
#include "atlas/library/Library.h"
#include "atlas/runtime/AtlasTool.h"
#include "atlas/runtime/Exception.h"
#include "atlas/runtime/Log.h"

using namespace atlas;
using namespace atlas::grid;
using eckit::JSON;
using eckit::PathName;

//----------------------------------------------------------------------------------------------------------------------

class AtlasGrids : public AtlasTool {
    virtual bool serial() { return true; }
    virtual int execute( const Args& args );
    virtual std::string briefDescription() { return "Catalogue of available built-in grids"; }
    virtual std::string usage() { return name() + " GRID [OPTION]... [--help,-h]"; }
    virtual std::string longDescription() {
        return "Catalogue of available built-in grids\n"
               "\n"
               "       Browse catalogue of grids\n"
               "\n"
               "       GRID: unique identifier for grid \n"
               "           Example values: N80, F40, O24, L32\n";
    }

public:
    AtlasGrids( int argc, char** argv ) : AtlasTool( argc, argv ) {
        add_option(
            new SimpleOption<bool>( "list", "List all grids. The names are possible values for the GRID argument" ) );

        add_option( new SimpleOption<bool>( "info", "List information about GRID" ) );

        add_option( new SimpleOption<bool>( "json", "Export json" ) );

        add_option( new SimpleOption<bool>( "rtable", "Export IFS rtable" ) );

        add_option( new SimpleOption<bool>( "check", "Check grid" ) );
    }

private:
    bool list;
    std::string key;
    bool info;
    bool json;
    bool rtable;
    bool do_run;
    bool check;
};

//------------------------------------------------------------------------------------------------------

int AtlasGrids::execute( const Args& args ) {
    key = "";
    if ( args.count() ) key = args( 0 );

    info = false;
    args.get( "info", info );

    if ( info && !key.empty() ) do_run = true;

    json = false;
    args.get( "json", json );
    if ( json && !key.empty() ) do_run = true;

    rtable = false;
    args.get( "rtable", rtable );
    if ( rtable && !key.empty() ) do_run = true;

    list = false;
    args.get( "list", list );
    if ( list ) do_run = true;
    
    check = false;
    args.get( "check", check );
    if( check && !key.empty() ) do_run = true;

    if ( !key.empty() && do_run == false ) {
        Log::error() << "Option wrong or missing after '" << key << "'" << std::endl;
    }
    if ( list ) {
        Log::info() << "usage: atlas-grids GRID [OPTION]... [--help]\n" << std::endl;
        Log::info() << "Available grids:" << std::endl;
        for ( const auto& key : GridFactory::keys() ) {
            Log::info() << "  -- " << key << std::endl;
        }
    }

    if ( !key.empty() ) {
        StructuredGrid grid;

        PathName spec{ key };
        if( spec.exists() ) {
          grid = Grid( Grid::Spec{ spec } );
        } 
        else {
          grid = Grid( key );
        }

        if ( !grid ) return failed();

        if ( info ) {
            double deg, km;
            Log::info() << "Grid " << key << std::endl;
            Log::info() << "   name:                               " << grid.name() << std::endl;
            Log::info() << "   uid:                                " << grid.uid() << std::endl;
            if ( auto gaussian = GaussianGrid( grid ) ) {
                Log::info() << "   Gaussian N number:                  " << gaussian.N() << std::endl;
            }
            Log::info() << "   number of points:                   " << grid.size() << std::endl;
            

            size_t memsize = grid.size() * sizeof( double );
            
            Log::info() << "   memory footprint per field (dp):    " << eckit::Bytes( memsize ) << std::endl;
            
            
            if( not grid.projection() ) {
              Log::info() << "   number of latitudes (N-S):          " << grid.ny() << std::endl;
              Log::info() << "   number of longitudes (max):         " << grid.nxmax() << std::endl;

              deg = ( grid.y().front() - grid.y().back() ) / ( grid.ny() - 1 );
              km  = deg * 40075. / 360.;
              Log::info() << "   approximate resolution N-S:         " << std::setw( 10 ) << std::fixed << deg
                          << " deg   " << km << " km " << std::endl;

              deg = 360. / static_cast<double>( grid.nx( grid.ny() / 2 ) );
              km  = deg * 40075. / 360.;
              Log::info() << "   approximate resolution E-W equator: " << std::setw( 10 ) << std::fixed << deg
                          << " deg   " << km << " km " << std::endl;

              deg = 360. * std::cos( grid.y( grid.ny() / 4 ) * M_PI / 180. ) /
                    static_cast<double>( grid.nx( grid.ny() / 4 ) );
              km = deg * 40075. / 360.;
              Log::info() << "   approximate resolution E-W midlat:  " << std::setw( 10 ) << std::fixed << deg
                          << " deg   " << km << " km " << std::endl;

              deg = 360. * std::cos( grid.y().front() * M_PI / 180. ) / static_cast<double>( grid.nx().front() );
              km  = deg * 40075. / 360.;


              Log::info() << "   approximate resolution E-W pole:    " << std::setw( 10 ) << std::fixed << deg
                          << " deg   " << km << " km " << std::endl;

              Log::info() << "   spectral truncation -- linear:      " << grid.ny() - 1 << std::endl;
              Log::info() << "   spectral truncation -- quadratic:   "
                          << static_cast<int>( std::floor( 2. / 3. * grid.ny() + 0.5 ) ) - 1 << std::endl;
              Log::info() << "   spectral truncation -- cubic:       "
                          << static_cast<int>( std::floor( 0.5 * grid.ny() + 0.5 ) ) - 1 << std::endl;
            }

            auto precision = Log::info().precision(3);
            if( grid.projection().units() == "meters" ) {
              Log::info() << "   x : [ " << std::setw(10) << std::fixed << grid.xspace().min() / 1000.
                          << " , "       << std::setw(10) << std::fixed << grid.xspace().max() / 1000. << " ] km" << std::endl;
              Log::info() << "   y : [ " << std::setw(10) << std::fixed << grid.yspace().min() / 1000.
                          << " , "       << std::setw(10) << std::fixed << grid.yspace().max() / 1000. << " ] km" << std::endl;
              if( grid.xspace().nxmax() == grid.xspace().nxmin() ) {
                  Log::info() << "   dx : " << grid.xspace().dx()[0] / 1000. << " km" << std::endl;
              }
              Log::info() << "   dy : " << std::abs(grid.yspace()[1]-grid.yspace()[0]) / 1000. << " km" << std::endl;
              Log::info() << "   lonlat(centre)    : " << grid.projection().lonlat(
                { 0.5*(grid.xspace().max()+grid.xspace().min()),
                  0.5*(grid.yspace().max()+grid.yspace().min())} ) << std::endl;
              Log::info() << "   lonlat(xmin,ymax) : " << grid.projection().lonlat( { grid.xspace().min(), grid.yspace().max()} ) << std::endl;
              Log::info() << "   lonlat(xmin,ymin) : " << grid.projection().lonlat( { grid.xspace().min(), grid.yspace().min()} ) << std::endl;
              Log::info() << "   lonlat(xmax,ymin) : " << grid.projection().lonlat( { grid.xspace().max(), grid.yspace().min()} ) << std::endl;
              Log::info() << "   lonlat(xmax,ymax) : " << grid.projection().lonlat( { grid.xspace().max(), grid.yspace().max()} ) << std::endl;
           }
           if( grid.projection().units() == "degrees" ) {
              Log::info() << "   x : [ " << std::setw(10) << std::fixed << grid.xspace().min()
                          << " , "       << std::setw(10) << std::fixed << grid.xspace().max() << " ] deg" << std::endl;
              Log::info() << "   y : [ " << std::setw(10) << std::fixed << grid.yspace().min()
                          << " , "       << std::setw(10) << std::fixed << grid.yspace().max() << " ] deg" << std::endl;
            }
            PointLonLat first_point = *grid.lonlat().begin();
            PointLonLat last_point;
            for( const auto p : grid.lonlat() ) {
              last_point = p;
            }
            Log::info() << "   lonlat(first)     : " << first_point << std::endl;
            Log::info() << "   lonlat(last)      : " << last_point << std::endl;
            Log::info().precision(precision);
        }
        if ( json ) {
            std::stringstream stream;
            JSON js( stream );
            js.precision( 16 );
            js << grid.spec();
            std::cout << stream.str() << std::endl;
        }

        if ( rtable ) {
            std::stringstream stream;
            stream << "&NAMRGRI\n";
            for ( idx_t j = 0; j < grid.ny(); ++j )
                stream << " NRGRI(" << std::setfill( '0' ) << std::setw( 5 ) << 1 + j << ")=" << std::setfill( ' ' )
                       << std::setw( 5 ) << grid.nx( j ) << ",\n";
            stream << "/" << std::flush;
            std::cout << stream.str() << std::endl;
        }
        
        if( check ) {
            bool check_failed = false;
            Log::Channel out; out.setStream( Log::error() );
            PathName spec{ key };
            if( not spec.exists() ) {
              out << "Check failed:  " << key << " is not a file" << std::endl;
              return failed();
            }
            util::Config config_check;
            if( not util::Config{spec}.get( "check", config_check ) ) {
              out << "Check failed:  no \"check\" section in " << key << std::endl;
              return failed();
            }

            idx_t size;
            if( config_check.get("size",size) ) {
                if( grid.size() != size ) {
                  out << "Check failed: grid size " << grid.size() << " expected to be " << size << std::endl;
                  check_failed = true;
                }
            }
            else {
              Log::warning() << "Check for size skipped" << std::endl;
            }

            std::string uid;
            if( config_check.get("uid",uid) ) {
                if( grid.uid() != uid ) {
                  out << "Check failed: grid uid " << grid.uid() << " expected to be " << uid << std::endl;
                  check_failed = true;
                }
            }
            else {
              Log::warning() << "Check for uid skipped" << std::endl;
            }

            
            auto equal = []( const PointLonLat& p, const std::vector<double>& check ) -> bool {
              if( not eckit::types::is_approximately_equal( p.lon(), check[0], 5.e-4 ) ) return false;
              if( not eckit::types::is_approximately_equal( p.lat(), check[1], 5.e-4 ) ) return false;
              return true;
            };
            
            std::vector<double> first_point_lonlat;
            if( config_check.get("lonlat(first)",first_point_lonlat) ) {
                PointLonLat first_point = *grid.lonlat().begin();
                if( not equal( first_point, first_point_lonlat ) ) {
                  out << "Check failed: lonlat(first) " << first_point << " expected to be " << PointLonLat( first_point_lonlat.data() ) << std::endl;
                  check_failed = true;
                }
            }
            else {
              Log::warning() << "Check for lonlat(first) skipped" << std::endl;
            }

            std::vector<double> last_point_lonlat;
            if( config_check.get("lonlat(last)",last_point_lonlat) ) {
              PointLonLat last_point;
              for( const auto&& p : grid.lonlat() ) {
                last_point = p;
              }
                if( not equal( last_point, last_point_lonlat ) ) {
                  out << "Check failed: lonlat(last) " << last_point << " expected to be " << PointLonLat( last_point_lonlat.data() ) << std::endl;
                  check_failed = true;
                }
            }
            else {
              Log::warning() << "Check for lonlat(last) skipped" << std::endl;
            }
            if( check_failed ) return failed();
            Log::info() << "SUCCESS: All checks passed" << std::endl;
        }
    }
    return success();
}

//------------------------------------------------------------------------------------------------------

int main( int argc, char** argv ) {
    AtlasGrids tool( argc, argv );
    return tool.start();
}
