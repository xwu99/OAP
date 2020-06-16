package org.apache.spark.ml.util

import org.apache.spark.SparkConf
import org.apache.spark.rdd.RDD
import org.apache.spark.ml.linalg.Vector

object Utils {

  def profile[R](title: String, block: => R): R = {
    val start = System.nanoTime()
    val result = block
    val end = System.nanoTime()
    println(s"${title} elapsed: ${end - start} ns")
    result
  }

  // Return index -> (rows, cols) map
  def getPartitionDims(data: RDD[Vector]): Map[Int, (Int, Int)] = {
    var numCols: Int = 0
    // Collect the numRows and numCols
    val collected = data.mapPartitionsWithIndex { (index: Int, it: Iterator[Vector]) =>
      val numCols = it.next().size
      Iterator((index, it.size + 1, numCols))
    }.collect

    var ret = Map[Int, (Int, Int)]()

    // set numRows and numCols
    collected.foreach {
      case (index, rows, cols) =>
        ret += (index -> (rows, cols))
    }

    ret
  }

  // Run on Driver
  def sparkExecutorNum(): Int = {
    val conf = new SparkConf()

    val executorNum = conf.getInt("spark.executor.instances", -1)

    assert(executorNum != -1, message = "spark.executor.instances not set")

    executorNum
  }

  def sparkExecutorCores(): Int = {
    val conf = new SparkConf(true)

    val executorCores = conf.getInt("spark.executor.cores", -1)

    assert(executorCores != -1, message = "spark.executor.cores not set")

    executorCores
  }
}
