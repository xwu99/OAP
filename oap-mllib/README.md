# OAP MLlib

## Overview

OAP MLlib is an optimized package to accelerate machine learning algorithms in  [Apache Spark MLlib](https://spark.apache.org/mllib).  It is compatible with Spark MLlib and leverages [Intel® oneAPI Data Analytics Library (oneDAL)](https://github.com/oneapi-src/oneDAL)  to provide highly optimized algorithms and get most out of CPU and GPU capabilities. It also take advantage of [Intel® oneAPI Collective Communications Library (oneCCL)](https://github.com/oneapi-src/oneCCL) to provide efficient communication patterns in multi-node multi-GPU clusters.

## Compatibility

OAP MLlib tried to maintain the same API interfaces and produce same results that are identical with Spark MLlib. However due to the nature of float point operation, there may be some small deviation from the original result, we will try our best to make sure the error is within acceptable range.
For those algorithms that are not accelerated by OAP MLlib, the original Spark MLlib one will be used. 

## Getting Started

You can use a pre-built package to get started, it can be downloaded from: [XXX]() .

After downloaded, you can refer to the following [Running](#Running) section to try out.

You can also build the package from source code, please refer to [Building](#Building) section.

## Running

### Prerequisites

* CentOS 7.0+
* Java 8.0+
* Apache Spark 3.0.0+

### Sanity Check
You need to change related variables in `run.sh` and run the following commands:
```
    $ cd OAP/oap-mllib/example/kmeans
    $ ./build.sh
    $ ./run.sh
```

### Benchmark with HiBench
Use HiBench to generate dataset with various profiles, and change related variables in `run-XXX.sh` script when applicable.  Then run the following commands:
```
    $ cd OAP/oap-mllib/example/kmeans-hibench
    $ ./build.sh
    $ ./run-hibench-oap-mllib.sh
```

### PySpark Support

As PySpark-based applications call their Scala couterparts, they shall be supported out-of-box. An example can be found in the [Examples](#Examples) section.

## Building

### Prerequisites

We use [Apache Maven](https://maven.apache.org/) to manage and build source code.  The following tools and libraries are also needed to build OAP MLlib:

* JDK 8.0+
* Apache Maven 3.6.2+
* GNU GCC 4.8.5+
* Intel® oneAPI Data Analytics Library 2021.1-beta06+
* Intel® oneAPI Threading Building Blocks 2021.1-beta06+
* Intel® oneAPI Collective Communications Library 2021.1-beta06+

Intel® oneAPI Toolkits (Beta) and its components can be downloaded from [here](https://software.intel.com/content/www/us/en/develop/tools/oneapi.html).

Scala and Java dependency descriptions are already included in Maven POM file. 

### Build

To clone and checkout source code, run the following commands:
```
    $ git clone https://github.com/Intel-bigdata/OAP
    $ git checkout -b origin/branch-intelmllib-spark-3.0.0
```
After installed the above Prerequisites, please make sure the following environment variables are set for building:

Environment | Description
------------| -----------
JAVA_HOME   | Path to JDK home directory
DAALROOT    | Path to oneDAL home directory
TBB_ROOT    | Path to oneTBB home directory
CCL_ROOT    | Path to oneCCL home directory

To build, run the following commands: 
```
    $ cd OAP/oap-mllib
    $ ./build.sh
```

The result jar package will be placed in `target` directory.

## Examples

Example         |  Description 
----------------|---------------------------
kmeans          |  K-means example for Scala
kmeans-pyspark  |  K-means example for PySpark
kmeans-hibench  |  Use HiBench-generated input dataset to benchmark K-means performance

## List of Accelerated Algorithms

* K-Means (CPU, Experimental)

