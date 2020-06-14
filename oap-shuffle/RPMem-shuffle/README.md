# Remote Persistent Memory shuffle plugin for Spark
RPMem extension for Spark Shuffle (previously Spark-PMoF), is a Spark Shuffle Plugin which enables persistent memory and high performance fabric technology like RDMA for Spark shuffle to improve Spark performance in shuffle intensive scneario. 

## Contents
- [Introduction](#introduction)
- [Installation](#installation)
- [Benchmark](#benchmark)
- [Usage](#usage)
- [Contact](#contact)

## Introduction
Remote Persistent Memory shuffle plugin is a Spark shuffle plugin that leverage Persistent Memory as shuffle media, and high performance fabrics powered NICs like RDMA to improve Spark shuffle performance. it:

- Leverage high performance persistent memory as shuffle media as well as spill media,increased shuffle performance and reduced memory footprint
  
- Using PMDK libs to avoid inefficient context switches and memory copies with zerocopy remote access to persistent memory.
  
- Leveraging RDMA for network offloading


## Installation
Make sure you got [HPNL](https://github.com/Intel-bigdata/HPNL) installed.

```shell
git clone https://github.com/Intel-bigdata/Spark-PMoF.git
cd Spark-PMoF; mvn package
```
Besdies HPNL, RPMem shuffle depends on several other libraries, including PMDK, libcuckoo etc. Please refer to the enabling guide for detail instructions. 

## Benchmark

## Usage
This plugin current supports Spark 2.4.4 and works well on various Network fabrics, including Socket, **RDMA** and **Omni-Path**. Before runing Spark workload, please refer to the enabling guide in doc folder for details. 

```shell
spark.driver.extraClassPath Spark-PMoF-PATH/target/sso-0.1-jar-with-dependencies.jar
spark.executor.extraClassPath Spark-PMoF-PATH/target/sso-0.1-jar-with-dependencies.jar
spark.shuffle.manager org.apache.spark.shuffle.pmof.RdmaShuffleManager
```
