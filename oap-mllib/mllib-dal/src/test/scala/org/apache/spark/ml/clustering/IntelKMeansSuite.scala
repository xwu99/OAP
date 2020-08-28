package org.apache.spark.ml.clustering

import org.scalatest.funsuite.AnyFunSuite
import org.scalatest.{Assertions, BeforeAndAfterAll}
import org.apache.spark.{SparkConf, SparkContext}
import org.apache.spark.sql.{DataFrame, SparkSession}

class IntelKMeansSuite extends AnyFunSuite with BeforeAndAfterAll {

  var spark: SparkSession = _
  var sc: SparkContext = _
  var dataset: DataFrame = _

  final val k = 5

  override def beforeAll(): Unit = {
    spark = SparkSession
      .builder
      .master("local")
      .config("spark.sql.testkey", "true")
      .appName(s"${this.getClass.getSimpleName}")
      .getOrCreate()

    sc = spark.sparkContext

    dataset = KMeansSuite.generateKMeansData(spark, 50, 3, k)
  }

  override def afterAll(): Unit = {
    spark.stop()
  }

  test("generateKMeansData") {
    dataset.show()
  }
}