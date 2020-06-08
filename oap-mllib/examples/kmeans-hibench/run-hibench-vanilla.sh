#!/usr/bin/env bash

# for log suffix
SUFFIX=$( basename -s .sh "${BASH_SOURCE[0]}" )

export SPARK_HOME=/home/xiaochang/opt/spark-3.0.0-preview-bin-hadoop2.7
export HADOOP_HOME=/home/xiaochang/opt/hadoop-2.7.3
export HADOOP_CONF_DIR=$HADOOP_HOME/etc/hadoop

SPARK_MASTER=yarn

SPARK_NUM_EXECUTORS=2
SPARK_EXECUTOR_CORES=2
SPARK_EXECUTOR_MEMORY=2G
SPARK_DRIVER_MEMORY=1G

SPARK_DEFAULT_PARALLELISM=$(expr $SPARK_NUM_EXECUTORS '*' $SPARK_EXECUTOR_CORES '*' 2)

APP_JAR=target/oap-mllib-examples-1.0-SNAPSHOT-jar-with-dependencies.jar
APP_CLASS=com.intel.hibench.sparkbench.ml.DenseKMeansDS

K=200
MAX_ITERATION=10
INPUT_HDFS=hdfs://localhost:8020/HiBench/Kmeans/Input/samples

/usr/bin/time -p $SPARK_HOME/bin/spark-submit --master $SPARK_MASTER -v \
    --num-executors $SPARK_NUM_EXECUTORS \
    --driver-memory $SPARK_DRIVER_MEMORY \
    --executor-cores $SPARK_EXECUTOR_CORES \
    --executor-memory $SPARK_EXECUTOR_MEMORY \
    --conf "spark.serializer=org.apache.spark.serializer.KryoSerializer" \
    --conf "spark.default.parallelism=$SPARK_DEFAULT_PARALLELISM" \
    --conf "spark.sql.shuffle.partitions=$SPARK_DEFAULT_PARALLELISM" \
    --class $APP_CLASS \
    $APP_JAR \
    -k $K --numIterations $MAX_ITERATION $INPUT_HDFS \
    2>&1 | tee KMeansHiBench-$SUFFIX-$(date +%m%d_%H_%M_%S).log
