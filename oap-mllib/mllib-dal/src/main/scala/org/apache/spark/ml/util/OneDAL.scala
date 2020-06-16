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

package org.apache.spark.ml.util

import com.intel.daal.data_management.data.{HomogenNumericTable, NumericTable, Matrix => DALMatrix}
import com.intel.daal.services.DaalContext
//import com.intel.daal.services.Environment
import org.apache.spark.ml.linalg.{Vector, Vectors}
import org.apache.spark.mllib.linalg.{Vector => OldVector, Vectors => OldVectors}
import org.apache.spark.rdd.RDD
import org.apache.spark.sql.{Dataset, Row}
import org.apache.spark.storage.StorageLevel

import scala.collection.mutable.ArrayBuffer

object OneDAL {

  println("oneDAL: Loading libMLlibDAL.so")

  // extract libMLlibDAL.so to temp file and load, TODO: remove duplicate loads
  LibUtils.loadLibrary()

//  def setNumberOfThread(thread_num: Int): Unit = {
//    println(s"oneDAL: setNumberOfThread: $thread_num")
//    Environment.setNumberOfThreads(thread_num)
//  }

  // Convert DAL numeric table to array of vectors, TODO: improve perf
  def numericTableToVectors(table: NumericTable): Array[Vector] = {
    val numRows = table.getNumberOfRows.toInt
    val numCols = table.getNumberOfColumns.toInt

    val resArray = new Array[Vector](numRows.toInt)

    for (row <- 0 until numRows) {
      val internArray = new Array[Double](numCols)
      for (col <- 0 until numCols) {
        internArray(col) = table.getDoubleValue(col, row)
      }
      resArray(row) = Vectors.dense(internArray)
    }

    resArray
  }

  // Convert all dataset rows and columns into HomogenNumericTable
  def datasetToRDDNumericTable(dataset: Dataset[_]): RDD[HomogenNumericTable] = {
    val data = dataset.toDF()
    val numCols: Int = data.schema.fields.length
    val tablesRDD = data.rdd.mapPartitions(
      (it: Iterator[Row]) => {

        val tables = new ArrayBuffer[HomogenNumericTable]()
        // here we need get the numRows, so convert the iterator to array.
        val arrayData = it.toArray
        val numRows: Int = arrayData.size

        val context = new DaalContext()
        val matrix = new DALMatrix(context, classOf[java.lang.Double],
          numCols.toLong, numRows.toLong, NumericTable.AllocationFlag.DoAllocate)

        arrayData.zipWithIndex.foreach {
          case (row, rowIndex) =>
            for (colIndex <- 0 until numCols)
//            matrix.set(rowIndex, colIndex, row.getString(colIndex).toDouble)
              setNumericTableValue(matrix.getCNumericTable, rowIndex, colIndex, row.getString(colIndex).toDouble)
        }

        matrix.pack()
        tables += matrix

        context.dispose()
        tables.iterator
      }
    ).persist(StorageLevel.MEMORY_AND_DISK)
    tablesRDD
  }

  // Convert a single Vector column into HomogenNumericTable
  def rddVectorToRDDNumericTable(instances: RDD[Vector]): RDD[HomogenNumericTable] = {
    val tablesRDD = instances.mapPartitions(
      (it: Iterator[Vector]) => {
        println("oneDAL: rddVectorToRDDNumericTable")
        val tables = new ArrayBuffer[HomogenNumericTable]()
        // here we need get the numRows, so convert the iterator to array.
        val arrayData = it.toArray
        val numCols = arrayData.head.size
        val numRows: Int = arrayData.size

        val context = new DaalContext()
        val matrix = new DALMatrix(context, classOf[java.lang.Double],
          numCols.toLong, numRows.toLong, NumericTable.AllocationFlag.DoAllocate)

        arrayData.zipWithIndex.foreach {
          case (v, rowIndex) =>
            for (colIndex <- 0 until numCols)
            //            matrix.set(rowIndex, colIndex, row.getString(colIndex).toDouble)
              setNumericTableValue(matrix.getCNumericTable, rowIndex, colIndex, v(colIndex))
        }

        Service.printNumericTable("oneDAL: First 10 rows of input matrix :", matrix, 10)

        matrix.pack()
        tables += matrix

        context.dispose()
        tables.iterator
      }
    )
//      .persist(StorageLevel.MEMORY_AND_DISK)
    tablesRDD
  }

  def makeNumericTable (cData: Long) : NumericTable = {

    val context = new DaalContext()
    val table = new HomogenNumericTable(context, cData)

    table
  }

  def makeNumericTable (arrayVectors: Array[OldVector]): NumericTable = {

    val numCols = arrayVectors.head.size
    val numRows: Int = arrayVectors.size

    val context = new DaalContext()
    val matrix = new DALMatrix(context, classOf[java.lang.Double],
      numCols.toLong, numRows.toLong, NumericTable.AllocationFlag.DoAllocate)

    arrayVectors.zipWithIndex.foreach {
      case (v, rowIndex) =>
        for (colIndex <- 0 until numCols)
        //            matrix.set(rowIndex, colIndex, row.getString(colIndex).toDouble)
          setNumericTableValue(matrix.getCNumericTable, rowIndex, colIndex, v(colIndex))
    }

    matrix
  }

  @native def setNumericTableValue(numTableAddr: Long, rowIndex: Int, colIndex: Int, value: Double)
}
