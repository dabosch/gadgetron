#ifndef MRI_CORE_DATA_H
#define MRI_CORE_DATA_H

#include "ismrmrd/ismrmrd.h"
#include "ismrmrd/waveform.h"
#include "ismrmrd/meta.h"
#include <vector>
#include <set>
#include "hoNDArray.h"
#include <boost/optional.hpp>
#include "Types.h"

namespace Gadgetron 
{

    /**
      This is a list of lables of the coordinates described in the ISMRMRD acquisition header.

      It is useful for accumulators and triggers and for labeling the storage used in
      the @IsmrmrdAcquisitionBucket and @IsmrmrdDataBuffered structures. 

   */
    enum IsmrmrdCONDITION {
	KSPACE_ENCODE_STEP_1,
	KSPACE_ENCODE_STEP_2,
	AVERAGE,
	SLICE,
	CONTRAST,
	PHASE,
	REPETITION,
	SET,
	SEGMENT,
	USER_0,
	USER_1,
	USER_2,
	USER_3,
	USER_4,
	USER_5,
	USER_6,
	USER_7,
    N_ACQUISITIONS,
	NONE
      };

    IsmrmrdCONDITION from_string(const std::string&, IsmrmrdCONDITION& condition );


    // define the dimensions of ISMRMRD
    enum IsmrmrdDIM
    {
        DIM_ReadOut = 32,
        DIM_Encoding1,
        DIM_Channel,
        DIM_Slice,
        DIM_Encoding2,
        DIM_Contrast,
        DIM_Phase,
        DIM_Repetition,
        DIM_Set,
        DIM_Segment,
        DIM_Average,
        DIM_other1,
        DIM_other2,
        DIM_other3,
        DIM_NONE
    };


    // --------------------------------------------------------------------------
    /// define the calibration mode of ISMRMRD
    // --------------------------------------------------------------------------
    enum ismrmrdCALIBMODE
    {
        ISMRMRD_embedded,
        ISMRMRD_interleaved,
        ISMRMRD_separate,
        ISMRMRD_external,
        ISMRMRD_other,
        ISMRMRD_noacceleration
    };

  struct SamplingLimit
  {
    uint16_t min_ = 0;
    uint16_t center_ = 0;
    uint16_t max_ = 0;
  };
  
  struct SamplingDescription
  {
    // encoding FOV
    float encoded_FOV_[3] = {0,0,0};
    // recon FOV
    float recon_FOV_[3] ={0,0,0};
    
    uint16_t encoded_matrix_[3] = {0,0,0};
    uint16_t recon_matrix_[3] = {0,0,0};
    
    // sampled range along RO, E1, E2 (for asymmetric echo and partial fourier)
    // min, max and center
    SamplingLimit sampling_limits_[3];
  };
  
  struct IsmrmrdDataBuffered
  {
  public:
    //7D, fixed order [E0, E1, E2, CHA, N, S, LOC]
    hoNDArray< std::complex<float> > data_;
    
    //7D, fixed order [TRAJ, E0, E1, E2, N, S, LOC]
    boost::optional<hoNDArray<float>> trajectory_;

    // 6D, density weights [E0, E1, E2, N, S, LOC]
    boost::optional<hoNDArray<float> > density_;

    //5D, fixed order [E1, E2, N, S, LOC]
    hoNDArray< ISMRMRD::AcquisitionHeader > headers_;

    SamplingDescription sampling_;

    IsmrmrdDataBuffered() {}
    IsmrmrdDataBuffered(const IsmrmrdDataBuffered& obj)
    {
        this->data_.copyFrom(obj.data_);

        if (obj.trajectory_)
        {
            if (this->trajectory_) {
                if (this->trajectory_->delete_data_on_destruct()) this->trajectory_->clear();
            } else {
                this->trajectory_ = hoNDArray<float>();
            }

            this->trajectory_->copyFrom(*obj.trajectory_);
        }
        
        this->headers_.copyFrom(obj.headers_);
        this->sampling_ = obj.sampling_;
    }

    ~IsmrmrdDataBuffered() {}

    void clear()
    {
        if (this->data_.delete_data_on_destruct()) this->data_.clear();
        if (this->trajectory_)
        {
            if (this->trajectory_->delete_data_on_destruct()) this->trajectory_->clear();
        }

        if (this->headers_.delete_data_on_destruct()) headers_.clear();
    }
  };
  

  /**
     This class is used to group a sub-unit of the data that would feed into a reconstruction. 
   */
  struct IsmrmrdReconBit
  {
  public:
    IsmrmrdDataBuffered data_;
    boost::optional<IsmrmrdDataBuffered> ref_;

    IsmrmrdReconBit() {}
    IsmrmrdReconBit(const IsmrmrdReconBit& obj)
    {
        this->data_= obj.data_;

        if (obj.ref_)
        {
            if (!this->ref_)
                this->ref_.reset(IsmrmrdDataBuffered());

            *this->ref_ = *obj.ref_;
        }
    }

    ~IsmrmrdReconBit() {}
  };

  /**
     This class is used to store a unit of data that would feed into a reconstruction. 
   */
  struct IsmrmrdReconData
  {
  public:
    std::vector<IsmrmrdReconBit> rbit_;
  };

  
  /**
     This class is used to store an array of reconstructed data. 
   */
  struct IsmrmrdImageArray
  {
    //7D, fixed order [X, Y, Z, CHA, N, S, LOC]
    hoNDArray< std::complex<float> > data_;
    
    //3D, fixed order [N, S, LOC]
    hoNDArray< ISMRMRD::ImageHeader > headers_;
    
    //3D, fixed order [N, S, LOC]
    //This element is optional (length is 0 if not present)
    std::vector< ISMRMRD::MetaContainer > meta_;

    // wave form
    Core::optional<std::vector< ISMRMRD::Waveform>> waveform_;

    // acquisition header, [Y, Z, N, S, LOC]
    Core::optional<hoNDArray< ISMRMRD::AcquisitionHeader >> acq_headers_;

  };



}
#endif //MRI_CORE_DATA_H
