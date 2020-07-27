#!/usr/bin/env bash

WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd $WORK_DIR

which java

./config-ssh.sh

cd /opt
wget https://archive.apache.org/dist/spark/spark-3.0.0/spark-3.0.0-bin-hadoop2.7.tgz
tar -xzf spark-3.0.0-bin-hadoop2.7.tgz
wget https://archive.apache.org/dist/hadoop/core/hadoop-2.7.7/hadoop-2.7.7.tar.gz
tar -xzf hadoop-2.7.7.tar.gz

cd $WORK_DIR

cp ./core-site.xml /opt/hadoop-2.7.7/etc/hadoop/
cp ./hdfs-site.xml /opt/hadoop-2.7.7/etc/hadoop/
cp ./yarn-site.xml /opt/hadoop-2.7.7/etc/hadoop/
cp ./hadoop-env.sh /opt/hadoop-2.7.7/etc/hadoop/
cp ./spark-defaults.conf /opt/spark-3.0.0-bin-hadoop2.7/conf

/opt/hadoop-2.7.7/sbin/stop-yarn.sh
/opt/hadoop-2.7.7/sbin/stop-dfs.sh

# create directories
mkdir -p /var/run/hdfs/namenode
mkdir -p /var/run/hdfs/datanode

# hdfs format
/opt/hadoop-2.7.7/bin/hdfs namenode -format

# start hdfs and yarn
/opt/hadoop-2.7.7/sbin/start-dfs.sh
/opt/hadoop-2.7.7/sbin/start-yarn.sh

export HADOOP_HOME=/opt/hadoop-2.7.7
export HADOOP_CONF_DIR=$HADOOP_HOME/etc/hadoop
export SPARK_HOME=/opt/spark-3.0.0-bin-hadoop2.7

export PATH=$HADOOP_HOME/bin:$SPARK_HOME/bin:$PATH

hadoop fs -ls /
yarn node -list
