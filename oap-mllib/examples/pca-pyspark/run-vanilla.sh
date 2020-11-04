#!/usr/bin/env bash

# == User to customize the following environments ======= #

# Set user Spark and Hadoop home directory
#export SPARK_HOME=/path/to/your/spark/home
#export HADOOP_HOME=/path/to/your/hadoop/home
# Set user HDFS Root
export HDFS_ROOT=hdfs://localhost:8020
# Set user Intel MLlib Root directory
export OAP_MLLIB_ROOT=/home/xiaochang/Works/OAP-xwu99-pca/oap-mllib
# Set IP and Port for oneCCL KVS, you can select any one of the worker nodes and set CCL_KVS_IP_PORT to its IP and Port
# IP can be got with `hostname -I`, if multiple IPs are returned, the first IP should be used. Port can be any available port.
# For example, if one of the worker IP is 192.168.0.1 and an available port is 51234. 
# CCL_KVS_IP_PORT can be set in the format of 192.168.0.1_51234
# Incorrectly setting this value will result in hanging when oneCCL initialize
#export CCL_KVS_IP_PORT=10.0.2.149_51234

# Data file is from Spark Examples (data/mllib/sample_kmeans_data.txt), the data file should be copied to HDFS

#  Turn off filename expanding
set -f
# DATA_FILE=data/pca_normalized_'['1-4']'.csv
DATA_FILE=data/pca_data.csv

# == User to customize Spark executor cores and memory == #

# User should check the requested resources are acturally allocated by cluster manager or Intel MLlib will behave incorrectly
SPARK_MASTER=yarn
SPARK_DRIVER_MEMORY=1G
SPARK_NUM_EXECUTORS=2
SPARK_EXECUTOR_CORES=1
SPARK_EXECUTOR_MEMORY=1G

SPARK_DEFAULT_PARALLELISM=$(expr $SPARK_NUM_EXECUTORS '*' $SPARK_EXECUTOR_CORES '*' 2)

# ======================================================= #

# Check env
if [[ -z $SPARK_HOME ]]; then
    echo SPARK_HOME not defined!
    exit 1
fi

if [[ -z $HADOOP_HOME ]]; then
    echo HADOOP_HOME not defined!
    exit 1
fi

export HADOOP_CONF_DIR=$HADOOP_HOME/etc/hadoop

APP_PY=pca-pyspark.py
K=3

/usr/bin/time -p $SPARK_HOME/bin/spark-submit --master $SPARK_MASTER -v \
    --num-executors $SPARK_NUM_EXECUTORS \
    --driver-memory $SPARK_DRIVER_MEMORY \
    --executor-cores $SPARK_EXECUTOR_CORES \
    --executor-memory $SPARK_EXECUTOR_MEMORY \
    --conf "spark.serializer=org.apache.spark.serializer.KryoSerializer" \
    --conf "spark.default.parallelism=$SPARK_DEFAULT_PARALLELISM" \
    --conf "spark.sql.shuffle.partitions=$SPARK_DEFAULT_PARALLELISM" \
    --conf "spark.shuffle.reduceLocality.enabled=false" \
    --conf "spark.network.timeout=1200s" \
    --conf "spark.task.maxFailures=1" \
    $APP_PY $DATA_FILE $K \
    2>&1 | tee PCA-vanilla-$(date +%m%d_%H_%M_%S).log
