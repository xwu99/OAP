
./travis/config-ssh.sh

cd /opt
wget https://archive.apache.org/dist/spark/spark-3.0.0/spark-3.0.0-bin-hadoop2.7.tgz
tar -xzf spark-3.0.0-bin-hadoop2.7.tgz
wget https://archive.apache.org/dist/hadoop/core/hadoop-2.7.7/hadoop-2.7.7.tar.gz
tar -xzf hadoop-2.7.7.tar.gz

cp ./travis/core-site.xml /opt/hadoop-2.7.7/etc/hadoop/
cp ./travis/hdfs-site.xml /opt/hadoop-2.7.7/etc/hadoop/
cp ./travis/yarn-site.xml /opt/hadoop-2.7.7/etc/hadoop/
cp ./travis/hadoop-env.sh /opt/hadoop-2.7.7/etc/hadoop/
cp ./travis/spark-defaults.conf /opt/spark-3.0.0-bin-hadoop2.7/conf

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
