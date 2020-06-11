package org.apache.spark.ml.util

object Utils {

  def profile[R](title: String, block: => R): R = {
    val start = System.nanoTime()
    val result = block
    val end = System.nanoTime()
    println(s"${title} elapsed: ${end - start} ns")
    result
  }

}
