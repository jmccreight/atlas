#include <iostream>

#include "atlas/array.h"
#include "atlas/array/ArrayUtil.h"
#include "atlas/array/MakeView.h"
#include "atlas/array/helpers/ArrayInitializer.h"
#include "atlas/array/helpers/ArrayWriter.h"
#include "atlas/array/native/NativeDataStore.h"
#include "atlas/runtime/Exception.h"

using namespace atlas::array::helpers;

namespace atlas {
namespace array {

template <typename Value>
Array* Array::create( idx_t dim0 ) {
    return new ArrayT<Value>( dim0 );
}
template <typename Value>
Array* Array::create( idx_t dim0, idx_t dim1 ) {
    return new ArrayT<Value>( dim0, dim1 );
}
template <typename Value>
Array* Array::create( idx_t dim0, idx_t dim1, idx_t dim2 ) {
    return new ArrayT<Value>( dim0, dim1, dim2 );
}
template <typename Value>
Array* Array::create( idx_t dim0, idx_t dim1, idx_t dim2, idx_t dim3 ) {
    return new ArrayT<Value>( dim0, dim1, dim2, dim3 );
}
template <typename Value>
Array* Array::create( idx_t dim0, idx_t dim1, idx_t dim2, idx_t dim3, idx_t dim4 ) {
    return new ArrayT<Value>( dim0, dim1, dim2, dim3, dim4 );
}
template <typename Value>
Array* Array::create( const ArrayShape& shape ) {
    return new ArrayT<Value>( shape );
}
template <typename Value>
Array* Array::create( const ArrayShape& shape, const ArrayLayout& layout ) {
    return new ArrayT<Value>( shape, layout );
}
template <typename Value>
Array* Array::wrap( Value* data, const ArrayShape& shape ) {
    return new ArrayT<Value>( new native::WrappedDataStore<Value>( data ), shape );
}
template <typename Value>
Array* Array::wrap( Value* data, const ArraySpec& spec ) {
    return new ArrayT<Value>( new native::WrappedDataStore<Value>( data ), spec );
}

Array::~Array() {}

Array* Array::create( DataType datatype, const ArrayShape& shape ) {
    switch ( datatype.kind() ) {
        case DataType::KIND_REAL64:
            return new ArrayT<double>( shape );
        case DataType::KIND_REAL32:
            return new ArrayT<float>( shape );
        case DataType::KIND_INT32:
            return new ArrayT<int>( shape );
        case DataType::KIND_INT64:
            return new ArrayT<long>( shape );
        case DataType::KIND_UINT64:
            return new ArrayT<unsigned long>( shape );
        default: {
            std::stringstream err;
            err << "data kind " << datatype.kind() << " not recognised.";
            throw_NotImplemented( err.str(), Here() );
        }
    }
}

template <typename Value>
ArrayT<Value>::ArrayT( ArrayDataStore* ds, const ArraySpec& spec ) {
    data_store_ = std::unique_ptr<ArrayDataStore>( ds );
    spec_       = spec;
}

template <typename Value>
ArrayT<Value>::ArrayT( idx_t dim0 ) {
    spec_       = ArraySpec( make_shape( dim0 ) );
    data_store_ = std::unique_ptr<ArrayDataStore>( new native::DataStore<Value>( spec_.size() ) );
}
template <typename Value>
ArrayT<Value>::ArrayT( idx_t dim0, idx_t dim1 ) {
    spec_       = ArraySpec( make_shape( dim0, dim1 ) );
    data_store_ = std::unique_ptr<ArrayDataStore>( new native::DataStore<Value>( spec_.size() ) );
}
template <typename Value>
ArrayT<Value>::ArrayT( idx_t dim0, idx_t dim1, idx_t dim2 ) {
    spec_       = ArraySpec( make_shape( dim0, dim1, dim2 ) );
    data_store_ = std::unique_ptr<ArrayDataStore>( new native::DataStore<Value>( spec_.size() ) );
}
template <typename Value>
ArrayT<Value>::ArrayT( idx_t dim0, idx_t dim1, idx_t dim2, idx_t dim3 ) {
    spec_       = ArraySpec( make_shape( dim0, dim1, dim2, dim3 ) );
    data_store_ = std::unique_ptr<ArrayDataStore>( new native::DataStore<Value>( spec_.size() ) );
}
template <typename Value>
ArrayT<Value>::ArrayT( idx_t dim0, idx_t dim1, idx_t dim2, idx_t dim3, idx_t dim4 ) {
    spec_       = ArraySpec( make_shape( dim0, dim1, dim2, dim3, dim4 ) );
    data_store_ = std::unique_ptr<ArrayDataStore>( new native::DataStore<Value>( spec_.size() ) );
}

template <typename Value>
ArrayT<Value>::ArrayT( const ArrayShape& shape ) {
    ATLAS_ASSERT( shape.size() > 0 );
    idx_t size = 1;
    for ( idx_t j = 0; j < shape.size(); ++j )
        size *= shape[j];
    data_store_ = std::unique_ptr<ArrayDataStore>( new native::DataStore<Value>( size ) );
    spec_       = ArraySpec( shape );
}

template <typename Value>
ArrayT<Value>::ArrayT( const ArrayShape& shape, const ArrayLayout& layout ) {
    spec_       = ArraySpec( shape );
    data_store_ = std::unique_ptr<ArrayDataStore>( new native::DataStore<Value>( spec_.size() ) );
    for ( idx_t j = 0; j < layout.size(); ++j )
        ATLAS_ASSERT( spec_.layout()[j] == layout[j] );
}

template <typename Value>
ArrayT<Value>::ArrayT( const ArraySpec& spec ) {
    if ( not spec.contiguous() ) ATLAS_NOTIMPLEMENTED;
    spec_       = spec;
    data_store_ = std::unique_ptr<ArrayDataStore>( new native::DataStore<Value>( spec_.size() ) );
}

template <typename Value>
void ArrayT<Value>::resize( const ArrayShape& _shape ) {
    if ( rank() != _shape.size() ) {
        std::stringstream msg;
        msg << "Cannot resize existing Array with rank " << rank() << " with a shape of rank " << _shape.size();
        throw_Exception( msg.str(), Here() );
    }

    Array* resized = Array::create<Value>( _shape );

    switch ( rank() ) {
        case 1:
            array_initializer<1>::apply( *this, *resized );
            break;
        case 2:
            array_initializer<2>::apply( *this, *resized );
            break;
        case 3:
            array_initializer<3>::apply( *this, *resized );
            break;
        case 4:
            array_initializer<4>::apply( *this, *resized );
            break;
        case 5:
            array_initializer<5>::apply( *this, *resized );
            break;
        case 6:
            array_initializer<6>::apply( *this, *resized );
            break;
        case 7:
            array_initializer<7>::apply( *this, *resized );
            break;
        case 8:
            array_initializer<8>::apply( *this, *resized );
            break;
        case 9:
            array_initializer<9>::apply( *this, *resized );
            break;
        default:
            ATLAS_NOTIMPLEMENTED;
    }

    replace( *resized );
    delete resized;
}

template <typename Value>
void ArrayT<Value>::insert( idx_t idx1, idx_t size1 ) {
    ArrayShape nshape = shape();
    if ( idx1 > nshape[0] ) { throw_Exception( "Cannot insert into an array at a position beyond its size", Here() ); }
    nshape[0] += size1;

    Array* resized = Array::create<Value>( nshape );

    array_initializer_partitioned<0>::apply( *this, *resized, idx1, size1 );
    replace( *resized );
    delete resized;
}

template <typename Value>
void ArrayT<Value>::resize( idx_t size1 ) {
    resize( make_shape( size1 ) );
}

template <typename Value>
void ArrayT<Value>::resize( idx_t size1, idx_t size2 ) {
    resize( make_shape( size1, size2 ) );
}

template <typename Value>
void ArrayT<Value>::resize( idx_t size1, idx_t size2, idx_t size3 ) {
    resize( make_shape( size1, size2, size3 ) );
}

template <typename Value>
void ArrayT<Value>::resize( idx_t size1, idx_t size2, idx_t size3, idx_t size4 ) {
    resize( make_shape( size1, size2, size3, size4 ) );
}

template <typename Value>
void ArrayT<Value>::resize( idx_t size1, idx_t size2, idx_t size3, idx_t size4, idx_t size5 ) {
    resize( make_shape( size1, size2, size3, size4, size5 ) );
}

template <typename Value>
void ArrayT<Value>::dump( std::ostream& out ) const {
    switch ( rank() ) {
        case 1:
            make_host_view<Value, 1, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 2:
            make_host_view<Value, 2, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 3:
            make_host_view<Value, 3, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 4:
            make_host_view<Value, 4, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 5:
            make_host_view<Value, 5, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 6:
            make_host_view<Value, 6, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 7:
            make_host_view<Value, 7, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 8:
            make_host_view<Value, 8, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 9:
            make_host_view<Value, 9, Intent::ReadOnly>( *this ).dump( out );
            break;
        default:
            ATLAS_NOTIMPLEMENTED;
    }
}

//------------------------------------------------------------------------------

template <typename Value>
size_t ArrayT<Value>::footprint() const {
    size_t size = sizeof( *this );
    size += bytes();
    if ( not contiguous() ) ATLAS_NOTIMPLEMENTED;
    return size;
}

template <typename Value>
bool ArrayT<Value>::accMap() const {
    return false;
}

//------------------------------------------------------------------------------

template Array* Array::create<int>( idx_t );
template Array* Array::create<long>( idx_t );
template Array* Array::create<float>( idx_t );
template Array* Array::create<double>( idx_t );
template Array* Array::create<long unsigned>( idx_t );

template Array* Array::create<int>( idx_t, idx_t );
template Array* Array::create<long>( idx_t, idx_t );
template Array* Array::create<float>( idx_t, idx_t );
template Array* Array::create<double>( idx_t, idx_t );
template Array* Array::create<long unsigned>( idx_t, idx_t );

template Array* Array::create<int>( idx_t, idx_t, idx_t );
template Array* Array::create<long>( idx_t, idx_t, idx_t );
template Array* Array::create<float>( idx_t, idx_t, idx_t );
template Array* Array::create<double>( idx_t, idx_t, idx_t );
template Array* Array::create<long unsigned>( idx_t, idx_t, idx_t );

template Array* Array::create<int>( idx_t, idx_t, idx_t, idx_t );
template Array* Array::create<long>( idx_t, idx_t, idx_t, idx_t );
template Array* Array::create<float>( idx_t, idx_t, idx_t, idx_t );
template Array* Array::create<double>( idx_t, idx_t, idx_t, idx_t );
template Array* Array::create<long unsigned>( idx_t, idx_t, idx_t, idx_t );

template Array* Array::create<int>( idx_t, idx_t, idx_t, idx_t, idx_t );
template Array* Array::create<long>( idx_t, idx_t, idx_t, idx_t, idx_t );
template Array* Array::create<float>( idx_t, idx_t, idx_t, idx_t, idx_t );
template Array* Array::create<double>( idx_t, idx_t, idx_t, idx_t, idx_t );
template Array* Array::create<long unsigned>( idx_t, idx_t, idx_t, idx_t, idx_t );

template Array* Array::create<int>( const ArrayShape& );
template Array* Array::create<long>( const ArrayShape& );
template Array* Array::create<float>( const ArrayShape& );
template Array* Array::create<double>( const ArrayShape& );
template Array* Array::create<long unsigned>( const ArrayShape& );

template Array* Array::create<int>( const ArrayShape&, const ArrayLayout& );
template Array* Array::create<long>( const ArrayShape&, const ArrayLayout& );
template Array* Array::create<float>( const ArrayShape&, const ArrayLayout& );
template Array* Array::create<double>( const ArrayShape&, const ArrayLayout& );
template Array* Array::create<long unsigned>( const ArrayShape&, const ArrayLayout& );

template Array* Array::wrap<int>( int*, const ArrayShape& );
template Array* Array::wrap<long>( long*, const ArrayShape& );
template Array* Array::wrap<float>( float*, const ArrayShape& );
template Array* Array::wrap<double>( double*, const ArrayShape& );
template Array* Array::wrap<long unsigned>( long unsigned*, const ArrayShape& );

template Array* Array::wrap<int>( int*, const ArraySpec& );
template Array* Array::wrap<long>( long*, const ArraySpec& );
template Array* Array::wrap<float>( float*, const ArraySpec& );
template Array* Array::wrap<double>( double*, const ArraySpec& );
template Array* Array::wrap<long unsigned>( long unsigned*, const ArraySpec& );

template class ArrayT<int>;
template class ArrayT<long>;
template class ArrayT<float>;
template class ArrayT<double>;
template class ArrayT<unsigned long>;

}  // namespace array
}  // namespace atlas
