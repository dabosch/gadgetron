#include <gtest/gtest.h>
// Created by dchansen on 4/12/19.
//
#include "hoNDArray.h"
#include "hoNDArray_math.h"
using namespace Gadgetron;
TEST(hoNDArrayView,copy){

    hoNDArray<float> x(127,49,3);
    std::fill(x.begin(),x.end(),0.0f);

    hoNDArray<float> y(127,3);
    std::fill(y.begin(),y.end(),1.0f);

    x(slice,2,slice) = y;

    ASSERT_EQ(asum(x),asum(y));


}

TEST(hoNDArrayView,copy2){

    hoNDArray<float> x(127,49,3);
    std::fill(x.begin(),x.end(),0.0f);

    hoNDArray<float> y(127,3);
    std::fill(y.begin(),y.end(),1.0f);

    x(slice,1,slice) = y;

    hoNDArray<float> z(127,3);
    std::fill(z.begin(),z.end(),2.0f);

    x(slice,2,slice) = z;
    ASSERT_EQ(asum(x),asum(y)*3);


}


TEST(hoNDArrayView,copy3){

    hoNDArray<float> x(2,4,3);
    std::fill(x.begin(),x.end(),0.0f);

    hoNDArray<float> y(2,4);
    std::fill(y.begin(),y.end(),1.0f);

    x(slice,slice,1) = y;

    hoNDArray<float> z(2,4);
    std::fill(z.begin(),z.end(),2.0f);

    x(slice,slice,2) = z;
    ASSERT_EQ(asum(x),asum(y)*3);
}

TEST(hoNDArrayView,copy4){

    hoNDArray<float> x(2,4,3);
    std::fill(x.begin(),x.end(),0.0f);

    hoNDArray<float> y(2);
    std::fill(y.begin(),y.end(),1.0f);

    x(slice,1,2) = y;

    hoNDArray<float> z(2);
    std::fill(z.begin(),z.end(),2.0f);

    x(slice,3,2) = z;
    ASSERT_EQ(asum(x),asum(y)*3);
}


TEST(hoNDArrayView,assignment){
    hoNDArray<float> x(2,4,3);
    std::fill(x.begin(),x.end(),0.0f);


    hoNDArray<float> y(27,4,3);

    std::fill(y.begin(),y.end(),1.0f);

    x(1,slice,2) = y(15,slice,0);

    ASSERT_FLOAT_EQ(asum(x),4.0f);

}
