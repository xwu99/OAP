#!/usr/bin/env bash

export SPARK_HOME=/home/xiaochang/opt/spark-3.0.0-preview-bin-hadoop2.7
export HADOOP_HOME=/home/xiaochang/opt/hadoop-2.7.3
export HADOOP_CONF_DIR=$HADOOP_HOME/etc/hadoop

# Set OneCCL Root
CCLROOT=/home/xiaochang/Works/mlsl2/build/_install

# Check env
if [[ -z $SPARK_HOME ]]; then
    echo SPARK_HOME not defined!
    exit 1
fi

if [[ -z $HADOOP_HOME ]]; then
    echo HADOOP_HOME not defined!
    exit 1
fi

if [[ -z $DAALROOT ]]; then
    echo DAALROOT not defined!
    exit 1
fi

if [[ -z $TBBROOT ]]; then
    echo TBBROOT not defined!
    exit 1
fi

if [[ -z $CCLROOT ]]; then
    echo CCLROOT not defined!
    exit 1
fi

# Set SPARK_NUM_EXECUTORS for how many executors used
SPARK_MASTER=yarn
SPARK_NUM_EXECUTORS=2
SPARK_DEFAULT_PARALLELISM=$SPARK_NUM_EXECUTORS
SPARK_DRIVER_MEMORY=1G
SPARK_EXECUTOR_CORES=1
SPARK_EXECUTOR_MEMORY=1G

# Set user OAP MLlib Root directory
OAP_MLLIB_ROOT=/home/xiaochang/Works/OAP/oap-mllib
# Target jar built
OAP_MLLIB_JAR=$OAP_MLLIB_ROOT/mllib-dal/target/oap-mllib-1.0-SNAPSHOT-jar-with-dependencies.jar
DAAL_JAR=$DAALROOT/lib/daal.jar
# Comma-separated list of jars
SHARED_JARS=$OAP_MLLIB_JAR,$DAAL_JAR
# Comma-separated list of so
SHARED_LIBS=${DAALROOT}/lib/intel64/libJavaAPI.so,${TBBROOT}/lib/intel64/gcc4.8/libtbb.so.2,${TBBROOT}/lib/intel64/gcc4.8/libtbbmalloc.so.2
# All files to be uploaded
SPARK_FILES=$SHARED_JARS,$SHARED_LIBS

# Use absolute path
SPARK_DRIVER_CLASSPATH=$OAP_MLLIB_JAR:$DAAL_JAR
# Use relative path
SPARK_EXECUTOR_CLASSPATH=./oap-mllib-1.0-SNAPSHOT-jar-with-dependencies.jar:./daal.jar

# Set IP Port to one of the executors
CCL_KVS_IP_PORT=10.239.44.22_3000

APP_JAR=target/oap-mllib-examples-1.0-SNAPSHOT-jar-with-dependencies.jar
APP_CLASS=org.apache.spark.examples.ml.KMeansExample

# Data file is from Spark Examples (data/mllib/sample_kmeans_data.txt), the data file should be copied to HDFS
DATA_FILE=data/sample_kmeans_data.txt

/usr/bin/time -p $SPARK_HOME/bin/spark-submit --master $SPARK_MASTER -v \
    --num-executors $SPARK_NUM_EXECUTORS \
    --driver-memory $SPARK_DRIVER_MEMORY \
    --executor-cores $SPARK_EXECUTOR_CORES \
    --executor-memory $SPARK_EXECUTOR_MEMORY \
    --conf "spark.serializer=org.apache.spark.serializer.KryoSerializer" \
    --conf "spark.default.parallelism=$SPARK_DEFAULT_PARALLELISM" \
    --conf "spark.sql.shuffle.partitions=$SPARK_DEFAULT_PARALLELISM" \
    --conf "spark.executorEnv.CCL_ATL_TRANSPORT=ofi" \
    --conf "spark.executorEnv.CCL_PM_TYPE=resizable" \
    --conf "spark.executorEnv.CCL_KVS_IP_EXCHANGE=env" \
    --conf "spark.executorEnv.CCL_KVS_IP_PORT=$CCL_KVS_IP_PORT" \
    --conf "spark.executorEnv.CCL_WORLD_SIZE=$SPARK_NUM_EXECUTORS" \
    --conf "spark.driver.extraClassPath=$SPARK_DRIVER_CLASSPATH" \
    --conf "spark.executor.extraClassPath=$SPARK_EXECUTOR_CLASSPATH" \
    --files $SPARK_FILES \
    --class $APP_CLASS \
    $APP_JAR $DATA_FILE \
    2>&1 | tee KMeans-$(date +%m%d_%H_%M_%S).log
