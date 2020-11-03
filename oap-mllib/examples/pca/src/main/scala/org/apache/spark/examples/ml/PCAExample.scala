/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// scalastyle:off println
package org.apache.spark.examples.ml

// $example on$
import org.apache.spark.ml.feature.{Normalizer, PCA, StandardScaler}
import org.apache.spark.ml.linalg.Vectors
import org.apache.spark.ml.linalg.Vector
import org.apache.spark.ml.util.DatasetUtils
import org.apache.spark.mllib.linalg.distributed.RowMatrix
import org.apache.spark.mllib.linalg.{Vectors => OldVectors}
import org.apache.spark.sql.Row
// $example off$
import org.apache.spark.sql.SparkSession

object PCAExample {

  val K = 3

  def test1() = {
    val spark = SparkSession
      .builder
      .appName("PCAExample")
      .getOrCreate()

    val sc = spark.sparkContext

    // $example on$
    val data = Array(
      //      Vectors.sparse(5, Seq((1, 1.0), (3, 7.0))),
      Vectors.dense(0.0, 1.0, 0.0, 7.0, 0.0),
      Vectors.dense(2.0, 0.0, 3.0, 4.0, 5.0),
      Vectors.dense(4.0, 0.0, 0.0, 6.0, 7.0),
      Vectors.dense(8.0, 0.0, 1.0, 5.0, 7.0)
    )

    val df = spark.createDataFrame(data.map(Tuple1.apply)).toDF("features")

    val rdd = df.select("features").rdd.map {
      case Row(point: Vector) => OldVectors.fromML(point)
    }
    val mat = new RowMatrix(rdd)

    val (pc, variance) = mat.computePrincipalComponentsAndExplainedVariance(K)

    println("\nNot Scaled ML Principal Components: \n", pc)
    println("\nNot Scaled ML Explained Variance: \n", variance)

    val pca = new PCA()
      .setInputCol("features")
      .setOutputCol("pcaFeatures")
      .setK(K)
      .fit(df)

    println("\nPrincipal Components: \n", pca.pc)
    println("\nExplained Variance: \n", pca.explainedVariance)


    spark.stop()
  }

  def test2() = {

    val spark = SparkSession
      .builder
      .appName("PCAExample")
      .getOrCreate()

    val sc = spark.sparkContext

    // $example on$
    val data = Array(
      //      Vectors.sparse(5, Seq((1, 1.0), (3, 7.0))),
      Vectors.dense(0.0, 1.0, 0.0, 7.0, 0.0),
      Vectors.dense(2.0, 0.0, 3.0, 4.0, 5.0),
      Vectors.dense(4.0, 0.0, 0.0, 6.0, 7.0),
      Vectors.dense(8.0, 0.0, 1.0, 5.0, 7.0)
    )

    val df = spark.createDataFrame(data.map(Tuple1.apply)).toDF("features")

    val scaler = new StandardScaler()
      .setInputCol("features")
      .setOutputCol("scaledFeatures")
      .setWithMean(true)
      .setWithStd(true)

    val scalerModel = scaler.fit(df)

    // Normalize each feature to have unit standard deviation.
    val scaledData = scalerModel.transform(df)
    scaledData.show(truncate = false)

    val scaledRDD = scaledData.select("scaledFeatures").rdd.map {
      case Row(point: Vector) => OldVectors.fromML(point)
    }
    val mat = new RowMatrix(scaledRDD)

    val (pc, variance) = mat.computePrincipalComponentsAndExplainedVariance(K)

    println("\nScaled ML Principal Components: \n", pc)
    println("\nScaled ML Explained Variance: \n", variance)

    val pca = new PCA()
      .setInputCol("scaledFeatures")
      .setOutputCol("pcaFeatures")
      .setK(K)
      .fit(scaledData)

    println("\nPrincipal Components: \n", pca.pc)
    println("\nExplained Variance: \n", pca.explainedVariance)

    spark.stop()

  }
  def main(args: Array[String]): Unit = {
    test1()
    test2()
  }
}
// scalastyle:on println
