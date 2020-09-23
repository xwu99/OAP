package org.apache.spark.ml.clustering

import org.apache.spark.SparkContext
import org.apache.spark.mllib.clustering.{DistanceMeasure, KMeans => MLlibKMeans}
import org.apache.spark.sql.{DataFrame, SparkSession}
import org.scalatest.BeforeAndAfterAll
import org.scalatest.funsuite.AnyFunSuite

class IntelKMeansSuite extends AnyFunSuite with BeforeAndAfterAll {

  var spark: SparkSession = _
  var sc: SparkContext = _
  var dataset: DataFrame = _

  final val k = 5

  override def beforeAll(): Unit = {
    spark = SparkSession
      .builder
      .master("local")
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

  test("default parameters") {
    val kmeans = new KMeans()

    assert(kmeans.getK === 2)
    assert(kmeans.getFeaturesCol === "features")
    assert(kmeans.getPredictionCol === "prediction")
    assert(kmeans.getMaxIter === 20)
    assert(kmeans.getInitMode === MLlibKMeans.K_MEANS_PARALLEL)
    assert(kmeans.getInitSteps === 2)
    assert(kmeans.getTol === 1e-4)
    assert(kmeans.getDistanceMeasure === DistanceMeasure.EUCLIDEAN)
    val model = kmeans.setMaxIter(1).fit(dataset)

//    val transformed = model.transform(dataset)
//    checkNominalOnDF(transformed, "prediction", model.clusterCenters.length)
//
//    MLTestingUtils.checkCopyAndUids(kmeans, model)
//    assert(model.hasSummary)
//    val copiedModel = model.copy(ParamMap.empty)
//    assert(copiedModel.hasSummary)
  }
}