#!/bin/bash
p=$(pwd);
cd "incremental/ubuntu_1804_cuda92_cudnn7"
docker build --no-cache -t gadgetron .

cd "$p"
