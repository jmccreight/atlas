/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#pragma once

#include "atlas/library/config.h"
#include "atlas/parallel/omp/copy.h"
#include "atlas/parallel/omp/fill.h"
#include "atlas/runtime/Exception.h"

namespace atlas {

template <typename T>
class vector {
public:
    using value_type     = T;
    using iterator       = T*;
    using const_iterator = T const*;

public:
    vector() = default;

    vector( idx_t size ) { resize( size ); }

    vector( idx_t size, const value_type& value ) : vector( size ) { assign( size, value ); }

    vector( vector&& other ) : data_( other.data_ ), size_( other.size_ ) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    ~vector() {
        if ( data_ ) {
            delete[] data_;
        }
    }

    T& at( idx_t i ) noexcept( false ) {
        if ( i >= size_ ) {
            throw_OutOfRange( "atlas::vector", i, size_ );
        }
        return data_[i];
    }

    T const& at( idx_t i ) const noexcept( false ) {
        if ( i >= size_ ) {
            throw_OutOfRange( "atlas::vector", i, size_ );
        }
        return data_[i];
    }

    T& operator[]( idx_t i ) {
#if ATLAS_VECTOR_BOUNDS_CHECKING
        return at( i );
#else
        return data_[i];
#endif
    }

    T const& operator[]( idx_t i ) const {
#if ATLAS_VECTOR_BOUNDS_CHECKING
        return at( i );
#else
        return data_[i];
#endif
    }

    const T* data() const { return data_; }

    T* data() { return data_; }

    idx_t size() const { return size_; }

    void assign( idx_t n, const value_type& value ) {
        resize( n );
        omp::fill( begin(), begin() + n, value );
    }

    template <typename Iter>
    void assign( const Iter& first, const Iter& last ) {
        size_t size = std::distance( first, last );
        resize( size );
        omp::copy( first, last, begin() );
    }

    void reserve( idx_t size ) {
        if ( capacity_ != 0 )
            ATLAS_NOTIMPLEMENTED;
        data_     = new T[size];
        capacity_ = size;
    }
    void resize( idx_t size ) {
        if ( capacity_ == 0 ) {
            reserve( size );
        }
        if ( size > capacity_ ) {
            ATLAS_NOTIMPLEMENTED;
        }
        size_ = size;
    }
    const_iterator begin() const { return data_; }
    const_iterator end() const { return data_ + size_; }
    iterator begin() { return data_; }
    iterator end() { return data_ + size_; }
    const_iterator cbegin() const { return data_; }
    const_iterator cend() const { return data_ + size_; }

private:
    value_type* data_{nullptr};
    idx_t size_{0};
    idx_t capacity_{0};
};

}  // namespace atlas
