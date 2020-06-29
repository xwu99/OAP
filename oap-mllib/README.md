# OAP MLlib

## Overview

OAP MLlib is an optimized package to accelerate machine learning algorithms in  [Apache Spark MLlib](https://spark.apache.org/mllib).  It is compatible with Spark MLlib and leverages [Intel oneAPI Data Analytics Library (oneDAL)](https://github.com/oneapi-src/oneDAL)  to provide highly optimized algorithms and get most out of CPU and GPU capabilities.

## Compatibility

OAP MLlib tried to maintain the same API interfaces and produce same results that are identical with Spark MLlib. However due to the nature of float point operation, there may be some small deviation from the original result, we will try our best to make sure the error is within acceptable range.
For those algorithms that are not accelerated by OAP MLlib, the original Spark MLlib one will be used. 

## Getting Started

You can use a pre-built package to get started, it can be downloaded from: [XXX]() .

After downloaded, you can refer to the following [Running](#Running) section to try out.

You can also build the package from source code, please refer to [Building](#Building) section.

## Building

### Prerequisites

The following libraries and tools are needed to build OAP MLlib:

* JDK 8.0+
* Apache Maven 3.6.2+
* GNU GCC 4.8+
* Intel® oneAPI Collective Communications Library 2021.1-beta06+
* Intel® oneAPI Data Analytics Library 2021.1-beta06+
* Intel® oneAPI Threading Building Blocks 2021.1-beta06+

We use [Apache Maven](https://maven.apache.org/) to manage and build  source code.  To clone source code and build, please do as following:
```
    $ git clone https://github.com/Intel-bigdata/OAP
    $ git checkout -b origin/branch-intelmllib-spark-3.0.0
    $ cd OAP/oap-mllib
    $ ./build.sh
```

## Running

### Prerequisites

* CentOS 7.0+
* Apache Spark 3.0.0+

### Sanity Check
```
    $ cd OAP/oap-mllib/example/kmeans
    $ ./build.sh
    $ ./run.sh
```

### Benchmark with HiBench
Use HiBench to generate dataset, and change run scripts variables and configs when applicable.  Then run the following commands:
```
    $ cd OAP/oap-mllib/example/kmeans-hibench
    $ ./build.sh
    $ ./run-hibench-oap-mllib.sh
```

## List of Accelerated Algorithms

* K-Means (Experimental)