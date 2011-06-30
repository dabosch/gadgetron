#include "cuCG.h"
#include "real_utilities.h"
#include "vector_td_utilities.h"
#include "ndarray_vector_td_utilities.h"

template<class REAL, class T> bool 
cuCG<REAL,T>::set_device( int device )
{ 
  static bool handle_set = false;
  
  device_ = device;

  if ( handle_set && cublasDestroy(cublas_handle_) != CUBLAS_STATUS_SUCCESS) {
    std::cerr << "cuCG::set_device unable to destroy existing cublas handle" << std::endl;
    return false;
  }

  handle_set = false;
  
  int old_device;  
  if( cudaGetDevice( &old_device ) != cudaSuccess ){
    std::cerr << "cuCG::set_device: unable to get current device." << std::endl ;
    return false;
  }
    
  if( device<0 )
    device_ = old_device;

  if( device_ != old_device && cudaSetDevice(device_) != cudaSuccess) {
    std::cerr << "cuCG:set_device: unable to set device " << device_ << std::endl;
    return false;
  }
          
  if (cublasCreate(&cublas_handle_) != CUBLAS_STATUS_SUCCESS) {
    std::cerr << "cuCG::set_device: unable to create cublas handle" << std::endl;
    return false;
  }

  handle_set = true;
  
  cublasSetPointerMode( cublas_handle_, CUBLAS_POINTER_MODE_HOST );
  
  if( device_ != old_device && cudaSetDevice(old_device) != cudaSuccess) {
    std::cerr << "cuCG::set_device: unable to restore device no" << std::endl;
    return false;
  }

  return true;
}


template<class REAL, class T> boost::shared_ptr< cuNDArray<T> > 
cuCG<REAL,T>::solve(cuNDArray<T> *_rhs)
{
  int old_device;
  
  if( cudaGetDevice( &old_device ) != cudaSuccess ){
    std::cerr << "cuCG::solve: unable to get current device." << std::endl;
    return boost::shared_ptr< cuNDArray<T> >();
  }
  
  if( device_ != old_device && cudaSetDevice(device_) != cudaSuccess) {
    std::cerr << "cuCG:solve: unable to set device " << device_ << std::endl;
    return boost::shared_ptr< cuNDArray<T> >();
  }
  
  // Transfer arrays to compute device if necessary
  cuNDArray<T> *rhs;
  if( device_ != _rhs->get_device() )
    rhs = new cuNDArray<T>(*_rhs);
  else
    rhs = _rhs;
  
  // Result, rho
  cuNDArray<T> *rho = new cuNDArray<T>();
  
  if (!rho->create(rhs->get_dimensions().get())) {
    std::cerr << "cuCG::solve : Unable to allocate temp storage (rho)" << std::endl;
    cudaSetDevice(old_device);
    return boost::shared_ptr< cuNDArray<T> >(rho);
  }

  cuNDA_clear<T>(rho);
  
  // Calculate residual r
  cuNDArray<T> r;
  if (precond_.get()) {
    if (!r.create(rhs->get_dimensions().get())) {
      std::cerr << "cuCG::solve : Unable to allocate storage (r)" << std::endl;
      cudaSetDevice(old_device);
      return boost::shared_ptr< cuNDArray<T> >(rho);
    }
    if (precond_->apply(rhs,&r) < 0) {
      std::cerr << "cuCG::solve : Unable to apply preconditioning to rhs" << std::endl;
      cudaSetDevice(old_device);
      return boost::shared_ptr< cuNDArray<T> >(rho);
    }
  } else {
    r =  *rhs;
  }
  
  REAL rr_0    = real<REAL>(cuNDA_dot<T>(&r, &r, cublas_handle_));
  REAL rr_1    = rr_0;
  REAL rr      = get_zero<REAL>();
  REAL rr_last = get_max<REAL>();

  cuNDArray<T> p;
  if (!p.create(rhs->get_dimensions().get())) {
    std::cerr << "cuCG::solve : Unable to allocate temp storage (p)" << std::endl;
    cudaSetDevice(old_device);
    return boost::shared_ptr< cuNDArray<T> >(rho);
  }

  cuNDArray<T> p_precond;
  if (precond_.get()) { // We only need this additional storage if we are using a preconditioner
    if (!p_precond.create(rhs->get_dimensions().get())) {
      std::cerr << "cuCG::solve : Unable to allocate temp storage (p_precond)" << std::endl;
      cudaSetDevice(old_device);
      return boost::shared_ptr< cuNDArray<T> >(rho);
    }
  }

  cuNDArray<T> q;
  if (!q.create(rhs->get_dimensions().get())) {
    std::cerr << "cuCG::solve : Unable to allocate temp storage (q)" << std::endl;
    cudaSetDevice(old_device);
    return boost::shared_ptr< cuNDArray<T> >(rho);
  }

  cuNDArray<T> q2;
  if (!q2.create(rhs->get_dimensions().get())) {
    std::cerr << "cuCG::solve : Unable to allocate temp storage (q2)" << std::endl;
    cudaSetDevice(old_device);
    return boost::shared_ptr< cuNDArray<T> >(rho);
  }

  REAL rel_res;

  if (output_mode_ >= OUTPUT_VERBOSE) {
    std::cout << "Iterating..." << std::endl;
  }

  for (unsigned int it = 0; it < iterations_; it++) { //iterations_; it++) {

    rr_1 = rr;
    rr = real<REAL,T>(cuNDA_dot<T>(&r, &r, cublas_handle_));
    
    // Update p
    if (it == 0){
      p = r;
    } else {        
      T beta = mul<REAL>(rr/rr_1, get_one<T>());
      if (!cuNDA_scal<T>(beta,&p,cublas_handle_)) {
	std::cerr << "cuCG::solve : failed to scale p" << std::endl;
	cudaSetDevice(old_device);
	return boost::shared_ptr< cuNDArray<T> >(rho);
      }
      if (!cuNDA_axpy<T>(get_one<T>(),&r,&p,cublas_handle_)) {
	std::cerr << "cuCG::solve : failed to add r to scaled p" << std::endl;
	cudaSetDevice(old_device);
	return boost::shared_ptr< cuNDArray<T> >(rho);
      }
    }

    // Now we need to multiply with the system matrix
    cuNDA_clear<T>(&q);
    
    // Take care of preconditioning
    cuNDArray<T>* cur_p = &p;
    if (precond_.get()) {
      if (precond_->apply(&p,&p_precond) < 0) {
	std::cerr << "cuCG::solve : failed to apply preconditioner to p" << std::endl;
	cudaSetDevice(old_device);
	return boost::shared_ptr< cuNDArray<T> >(rho);
      }
      cur_p = &p_precond;
    }

    for (unsigned int i = 0; i < operators_->size(); i++) {

      if ((*operators_)[i]->mult_MH_M(cur_p, &q2, false) < 0) {
	std::cerr << "cuCG::solve : failed to apply operator number " << i << std::endl;
	cudaSetDevice(old_device);
	return boost::shared_ptr< cuNDArray<T> >(rho);
      }

      if (!cuNDA_axpy<T>(mul<REAL>((*operators_)[i]->get_weight(), get_one<T>()),&q2,&q,cublas_handle_)) {
	std::cerr << "cuCG::solve : failed to add q1 to q" << std::endl;
	cudaSetDevice(old_device);
	return boost::shared_ptr< cuNDArray<T> >(rho);
      }
    }
    
    if (precond_.get()) {
      if (precond_->apply(&q,&q) < 0) {
	std::cerr << "cuCG::solve : failed to apply preconditioner to q" << std::endl;
	cudaSetDevice(old_device);
	return boost::shared_ptr< cuNDArray<T> >(rho);
      }
    }

    T alpha = mul<REAL>(rr, reciprocal<T>(cuNDA_dot<T>(&p,&q,cublas_handle_)));
    
    // Update solution
    if (!cuNDA_axpy<T>(alpha,&p,rho,cublas_handle_)) {
      std::cerr << "cuCG::solve : failed to update solution" << std::endl;
      cudaSetDevice(old_device);
      return boost::shared_ptr< cuNDArray<T> >(rho);
    }
    
    // Update residual
    if (!cuNDA_axpy<T>(mul<REAL>(-get_one<REAL>(),alpha),&q,&r,cublas_handle_)) {
      std::cerr << "cuCG::solve : failed to update residual" << std::endl;
      cudaSetDevice(old_device);
      return boost::shared_ptr< cuNDArray<T> >(rho);
    }

    // Calculate relative residual norm
    rel_res = rr/rr_0;
    
    if (output_mode_ >= OUTPUT_WARNINGS) {
      if (output_mode_ >= OUTPUT_VERBOSE) {
	std::cout << "Iteration " << it+1 << ". rr/rr_0 = " << rel_res << std::endl;
      }
      if (rr_last-rel_res < get_zero<REAL>()) {
	std::cout << "----- Warning: CG residual increase. Stability problem! -----" << std::endl;
      }
    }

    if (rel_res < limit_) {
      break;
    } else {
      rr_last = rel_res;
    }
  }

  if (precond_.get()) {
    if (precond_->apply(rho,rho) < 0) {
      std::cerr << "cuCG::solve : failed to apply preconditioner to rho" << std::endl;
      cudaSetDevice(old_device);
      return boost::shared_ptr< cuNDArray<T> >(rho);
    }
  }

  // Free allocated array (if necessary)
  if( device_ != _rhs->get_device() )
    delete rhs;
  
  if( device_ != old_device && cudaSetDevice(old_device) != cudaSuccess) {
    std::cerr << "cuCG::solve: unable to restore device no" << std::endl;
    return boost::shared_ptr< cuNDArray<T> >(rho);
  }

  return boost::shared_ptr< cuNDArray<T> >(rho);
}

//
// Instantiation
//

template class EXPORTGPUCG cuCG<float, float>;
template class EXPORTGPUCG cuCG<float, float_complext::Type>;

template class EXPORTGPUCG cuCG<double, double>;
template class EXPORTGPUCG cuCG<double, double_complext::Type>;
