/** \file vector_td_utilities.h
    \brief The class vector_td defines a D-dimensional vector of type T.

    The class vector_td defines a D-dimensional vector of type T.
    It is used in the Gadgetron to represent small (one- to four-dimensional) vectors only.
    For larger vectors consider using the NDArray class instead.
    The vector_td class can be used on both the cpu and gpu.
    The accompanying headers vector_td_opeators.h and vector_td_utilities.h define most of the functionality.
*/

#pragma once

#include "vector_td.h"
#include "vector_td_operators.h"
#include "real_utilities.h"
#include "core_defines.h"

#include <float.h>
#include <vector>
#include <iostream>
#include <algorithm>

#ifndef __CUDA_ARCH__
using std::min; // workaround for nvcc
using std::max; // workaround for nvcc
using std::ceil; // workaround for nvcc
using std::abs; // workaround for nvcc
using std::floor; // workaround for nvcc
using std::sqrt;
#endif

namespace Gadgetron{


  //
  // Get/set operations on vector_td<T,D>
  //

  template<class T, unsigned int D> __inline__ __host__ __device__ T 
  get( const vector_td<T,D>& vec, unsigned int dim ) { return vec[dim]; }

  template<class T, unsigned int D> __inline__ __host__ __device__ void 
  set( vector_td<T,D> &vec, unsigned int dim, T val ) { vec[dim] = val; }

  //
  // Component-wise math operations
  //

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  vector_td<T,D> abs( const vector_td<T,D>& vec )
  {
    vector_td<T,D> res;
    for (unsigned int i=0; i<D; i++) {
      res[i] = std::abs(vec[i]);
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__
  vector_td<int,D> sgn( const vector_td<T,D>& vec )
  {
    vector_td<int,D> res;
    for (unsigned int i=0; i<D; i++) {
      res[i] = sgn(vec[i]);
    }
    return res;
  }

  template<class REAL, unsigned int D> __inline__ __host__ __device__ 
  vector_td<REAL,D> ceil( const vector_td<REAL,D> vec )
  {
    vector_td<REAL,D> res;
    for (unsigned int i=0; i<D; i++) {
      res[i] = ::ceil(vec[i]);
    }
    return res;
  }

  template<class REAL, unsigned int D> __inline__ __host__ __device__ 
  vector_td<REAL,D> floor( const vector_td<REAL,D> vec )
  {
    vector_td<REAL,D> res;
    for (unsigned int i=0; i<D; i++) {
      res[i] = ::floor(vec[i]);
    }
    return res;
  }

  //
  // Vectorize a scalar value
  //

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  vector_td<T,D> to_vector_td( const T scalar )
  {
    vector_td<T,D> res;
    for (unsigned int i=0; i<D; i++) {
      res[i] = scalar;
    }
    return res;
  }

  //
  // Grid <-> index transformations
  //

  template<unsigned int D> __inline__ __host__ __device__ 
  typename uintd<D>::Type idx_to_co( unsigned int idx, const vector_td<unsigned int,D> dims )
  {
    typename uintd<D>::Type co;
    unsigned int idx_tmp = idx;
    for (unsigned int i=0; i<D; i++) {
      co[i] = idx_tmp%dims[i];
      idx_tmp -= co[i];
      idx_tmp /= dims[i];
    }
    return co;
  } 

  template<unsigned int D> __inline__ __host__ __device__ 
  typename intd<D>::Type idx_to_co( int idx, const vector_td<int,D> dims )
  {
    typename intd<D>::Type co;
    int idx_tmp = idx;
    for (unsigned int i=0; i<D; i++) {
      co[i] = idx_tmp%dims[i];
      idx_tmp -= co[i];
      idx_tmp /= dims[i];
    }
    return co;
  } 

  template<unsigned int D> __inline__ __host__ __device__ 
  unsigned int co_to_idx( const vector_td<unsigned int,D> co, const vector_td<unsigned int,D> dims )
  {
    unsigned int idx = 0;
    unsigned long block_size = 1;
    for (unsigned int i=0; i<D; i++) {
      idx += (block_size*co[i]);
      block_size *= dims[i];
    }
    return idx;
  } 

  template<unsigned int D> __inline__ __host__ __device__  
  int co_to_idx( const vector_td< int,D> co, const vector_td<unsigned int,D> dims )
  {
    int idx = 0;
    long block_size = 1;
    for (int i=0; i<D; i++) {
      idx += (block_size*co[i]);
      block_size *= dims[i];
    }
    return idx;
  }

  template<unsigned int D> __inline__ __host__ __device__
  int co_to_idx( const vector_td< int,D> co, const vector_td<int,D> dims )
  {
    int idx = 0;
    long block_size = 1;
    for (int i=0; i<D; i++) {
      idx += (block_size*co[i]);
      block_size *= dims[i];
    }
    return idx;
  }

  template<unsigned int D> __inline__ __host__ __device__ 
  unsigned int co_to_idx( const typename uintd<D>::Type co, const typename uintd<D>::Type dims, const typename uintd<D>::Type &order )
  {
    unsigned int idx = 0;
    unsigned long block_size = 1;
    for (unsigned int i=0; i<D; i++){
      idx += (block_size*co.d[order[i]]);
      block_size *= dims.d[order[i]];
    }
    return idx;
  } 

  template<unsigned int D> __inline__ __host__ __device__ 
  int co_to_idx( const typename intd<D>::Type co, const typename intd<D>::Type dims, const typename intd<D>::Type order )
  {
    int idx = 0;
    unsigned long block_size = 1;
    for (unsigned int i=0; i<D; i++){
      idx += (block_size*co.d[order[i]]);
      block_size *= dims.d[order[i]];
    }
    return idx;
  } 

  template<unsigned int D> __inline__ __host__ __device__ 
  typename uintd<D>::Type counting_vec()
  {
    typename uintd<D>::Type res;
    for(unsigned int i=0; i<D; i++) {
      res[i]=i;
    }
    return res;
  }

  //
  // Conversion between vector<unsigned int> and uintd
  //

  template<unsigned int D> 
  std::vector<unsigned int> uintd_to_vector( typename uintd<D>::Type _uintd )
  {
    std::vector<unsigned int> out(D);
    for( unsigned int i=0; i<D; i++ )
      out[i] = _uintd[i];
    return out;
  }

  template<unsigned int D> 
  typename uintd<D>::Type vector_to_uintd( std::vector<unsigned int> _vector )
  {
    typename uintd<D>::Type out;
    for( unsigned int i=0; i<D; i++ ){
      if( i<_vector.size() )
	out[i] = _vector[i];
      else 
	out[i] = 1;
    }
    return out;
  }

  template<unsigned int D>
  typename intd<D>::Type vector_to_intd( std::vector<unsigned int> _vector )
  {
    typename intd<D>::Type out;
    for( unsigned int i=0; i<D; i++ ){
      if( i<_vector.size() )
	out[i] = _vector[i];
      else
	out[i] = 1;
    }
    return out;
  }

  template<class T, unsigned int D>
  std::vector<T> to_std_vector( vector_td<T,D> vec )
  {
    std::vector<T> out(D);
    for(int i=0; i<D; i++ )
      out[i] = vec[i];
    return out;
  }

  template<class T, unsigned int D>
  vector_td<T,D> from_std_vector( std::vector<T> _vector )
  {
    vector_td<T,D> out;
    for( unsigned int i=0; i<D; i++ ){
      if( i<_vector.size() )
	out[i] = _vector[i];
      else
	out[i] = T(1);
    }
    return out;
  }

  //
  // Reductions on vector_td<T,D>
  //

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  T prod( const vector_td<T,D>& vec )
  {
    T res = vec[0];
    for (unsigned int i=1; i<D; i++){
      res *= vec[i];
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  T sum( const vector_td<T,D>& vec )
  {
    T res = vec[0];
    for (unsigned int i=1; i<D; i++){
      res += vec[i];
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  T dot( const vector_td<T,D>& vec1, const vector_td<T,D>& vec2 )
  {
    T res = (vec1[0]*vec2[0]);
    for (unsigned int i=1; i<D; i++){
      res += (vec1[i]*vec2[i]);
    }
    return res;
  }


  template<class T, unsigned int D> __inline__ __host__ __device__
  T max( const vector_td<T,D>& vec )
  {
    T res = vec[0];
    for (unsigned int i=1; i<D; i++){
      res = ::max(res,vec[i]);
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__
  T min( const vector_td<T,D>& vec )
  {
    T res = vec[0];
    for (unsigned int i=1; i<D; i++){
      res = ::min(res,vec[i]);
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__
  vector_td<T,D> amin( const vector_td<T,D>& vec1, const vector_td<T,D>& vec2)
  {
    vector_td<T,D> res;
    for (unsigned int i=0; i<D; i++){
      res[i] = ::min(vec1[i],vec2[i]);
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__
  vector_td<T,D> amax( const vector_td<T,D>& vec1, const vector_td<T,D>& vec2)
  {
    vector_td<T,D> res;
    for (unsigned int i=0; i<D; i++){
      res[i] = ::max(vec1[i],vec2[i]);
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__
   vector_td<T,D> amin( const vector_td<T,D>& vec1, T val)
   {
     vector_td<T,D> res;
     for (unsigned int i=0; i<D; i++){
       res[i] = ::min(vec1[i],val);
     }
     return res;
   }

   template<class T, unsigned int D> __inline__ __host__ __device__
   vector_td<T,D> amax( const vector_td<T,D>& vec1, T val )
   {
     vector_td<T,D> res;
     for (unsigned int i=0; i<D; i++){
       res[i] = ::max(vec1[i],val);
     }
     return res;
   }

  template<class T, unsigned int D> __inline__ __host__ __device__
  T max_not_nan( const vector_td<T,D>& vec )
  {
    int i=0;
    while (isnan(vec[i])) i++;
    if (i >= D) return 0;
    T res = vec[i];
    for (++i; i<D; i++){
      if (!isnan(vec[i])) res = ::max(res,vec[i]);
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__
  T min_not_nan( const vector_td<T,D>& vec )
  {
    int i=0;
    while (isnan(vec[i])) i++;
    T res = vec[i];
    for (++i; i<D; i++){
      if (!isnan(vec[i])) res = ::min(res,vec[i]);
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  unsigned int argmin( const vector_td<T,D>& vec )
  {
    unsigned int res= 0;
    for (unsigned int i=1; i<D; i++){
      if (vec[i] < vec[res] ) res = i;
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  unsigned int argmin_not_nan( const vector_td<T,D>& vec )
  {
    unsigned int res= 0;
    for (unsigned int i=1; i<D; i++){
      if (vec[i] < vec[res] && !isnan(vec[i])) res = i;
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  unsigned int argmax( const vector_td<T,D>& vec )
  {
    unsigned int res= 0;
    for (unsigned int i=1; i<D; i++){
      if (vec[i] > vec[res] ) res = i;
    }
    return res;
  }

  //
  // Reductions on reald<REAL,D>
  //

  template<class REAL, unsigned int D> __inline__ __host__ __device__ 
  REAL norm_squared( const vector_td<REAL,D> vec )
  {
    REAL res = REAL(0);
    for (unsigned int i=0; i<D; i++){
      res += (vec[i]*vec[i]);
    }
    return res;
  }

  template<class REAL, unsigned int D> __inline__ __host__ __device__ 
  REAL norm( const vector_td<REAL,D> vec )
  {
    return ::sqrt(norm_squared<REAL,D>(vec));
  }

  //
  // Type conversion
  //

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  vector_td<int,D> to_intd( const vector_td<T,D>& vec )
  {
    vector_td<int,D> res;
    for (unsigned int i=0; i<D; i++){
      res[i] = int(vec[i]);
    }
    return res;
  }

  template<class T, unsigned int D> __inline__ __host__ __device__ 
  vector_td<unsigned int,D> to_uintd( const vector_td<T,D>& vec )
  {
    vector_td<unsigned int,D> res;
    for (unsigned int i=0; i<D; i++){
      res[i] = (unsigned int) vec[i];
    }
    return res;
  }

  template<class REAL, class T, unsigned int D> __inline__ __host__ __device__ 
  vector_td<REAL,D> to_reald( const vector_td<T,D>& vec )
  {
    vector_td<REAL,D> res;
    for (unsigned int i=0; i<D; i++){
      res[i] = (REAL) vec[i];
    }
    return res;
  }
}
