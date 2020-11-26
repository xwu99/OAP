package org.apache.spark.ml.recommendation

import com.intel.daal.data_management.data.CSRNumericTable.Indexing
import org.apache.spark.rdd.{ExecutorInProcessCoalescePartitioner, RDD}

import scala.reflect.ClassTag
import com.intel.daal.data_management.data.{CSRNumericTable, HomogenNumericTable, RowMergedNumericTable, Matrix => DALMatrix}
import com.intel.daal.services.DaalContext
import org.apache.spark.Partitioner
import org.apache.spark.internal.Logging
import org.apache.spark.ml.recommendation.ALS.Rating
import org.apache.spark.ml.util._

import scala.collection.mutable.ArrayBuffer
//import java.nio.DoubleBuffer
import java.nio.FloatBuffer

class ALSDataPartitioner(partitions: Int, itemsInBlock: Long)
  extends Partitioner {
  def numPartitions: Int = partitions
  def getPartition(key: Any): Int = {
    val k = key.asInstanceOf[Long]
    (k / itemsInBlock).toInt
//    if (k < 15)
//      (k / 5).toInt
//    else
//      3
  }
}

class ALSDALImpl[@specialized(Int, Long) ID: ClassTag](
  data: RDD[Rating[ID]],
  rank: Int,
  maxIter: Int,
  regParam: Double,
  alpha: Double,
  seed: Long,
) extends Serializable with Logging {

  // Return Map partitionId -> (ratingsNum, csrRowNum, rowOffset)
  private def getRatingsPartitionInfo(data: RDD[Rating[ID]]): Map[Int, (Int, Int, Int)] = {
    val collectd = data.mapPartitionsWithIndex { case (index: Int, it: Iterator[Rating[ID]]) =>
      var ratingsNum = 0
      var s = Set[ID]()
      it.foreach { v =>
        s += v.user
        ratingsNum += 1
      }
      Iterator((index, (ratingsNum, s.count(_ => true))))
    }.collect

    var ret = Map[Int, (Int, Int, Int)]()
    var rowOffset = 0
    collectd.foreach { v =>
      val partitionId = v._1
      val ratingsNum = v._2._1
      val csrRowNum = v._2._2
      ret += ( partitionId -> (ratingsNum, csrRowNum, rowOffset))
      rowOffset = rowOffset + csrRowNum
    }

    ret
  }

  private def ratingsToCSRNumericTables(ratings: RDD[Rating[ID]],
    nRatings: Long, nVectors: Long, nFeatures: Long, nBlocks: Long): RDD[CSRNumericTable] = {

//    val rowSortedRatings = ratings.sortBy(_.user.toString.toLong)

    val itemsInBlock = (nFeatures + nBlocks - 1) / nBlocks
//    val itemsInBlock = (nFeatures + nBlocks - 1) / nBlocks
//    val rowSortedGrouped = rowSortedRatings.groupBy(value => value.user.toString.toLong / itemsInBlock).flatMap(_._2)
    val rowSortedGrouped = ratings
      // Transpose the dataset
      .map { p =>
        Rating(p.item, p.user, p.rating)
      }
      .groupBy(value => value.user.toString.toLong)
      .partitionBy(new ALSDataPartitioner(nBlocks.toInt, itemsInBlock))
      .flatMap(_._2).mapPartitions { p =>
        p.toArray.sortBy(_.user.toString.toLong).toIterator
      }

//    rowSortedGrouped.mapPartitionsWithIndex { case (partitionId, partition) =>
//        println("partitionId", partitionId)
//        partition.foreach { p =>
//          println(p.user, p.item, p.rating) }
//        Iterator(partitionId)
//    }.collect()

    val ratingsPartitionInfo = getRatingsPartitionInfo(rowSortedGrouped)
    println("ratingsPartitionInfo:",  ratingsPartitionInfo)

    rowSortedGrouped.mapPartitionsWithIndex { case (partitionId, partition) =>
      val ratingsNum = ratingsPartitionInfo(partitionId)._1
      val csrRowNum = ratingsPartitionInfo(partitionId)._2
      val values = Array.fill(ratingsNum) { 0.0f }
      val columnIndices = Array.fill(ratingsNum) { 0L }
      val rowOffsets = ArrayBuffer[Long](1L)


      var index = 0
      var curRow = 0L
      // Each partition converted to one CSRNumericTable
      partition.foreach { p =>
        // Modify row index for each partition (start from 0)
        val row = p.user.toString.toLong - ratingsPartitionInfo(partitionId)._3
        val column = p.item.toString.toLong
        val rating = p.rating

        values(index) = rating
        // one-based index
        columnIndices(index) = column + 1

        if (row > curRow) {
          curRow = row
          // one-based index
          rowOffsets += index + 1
        }

        index = index + 1
      }
      // one-based row index
      rowOffsets += index+1

      println("PartitionId:", partitionId)
      println("csrRowNum", csrRowNum)
      println("rowOffsets", rowOffsets.mkString(","))
      println("columnIndices", columnIndices.mkString(","))
      println("values", values.mkString(","))

      val contextLocal = new DaalContext()

      println("ALSDALImpl: Loading native libraries ..." )
      LibLoader.loadLibraries()

      val cTable = OneDAL.cNewCSRNumericTable(values, columnIndices, rowOffsets.toArray, nVectors, csrRowNum)
      val table = new CSRNumericTable(contextLocal, cTable)
//      table.pack()

      println("Input dimensions:", table.getNumberOfRows, table.getNumberOfColumns)

      // There is a bug https://github.com/oneapi-src/oneDAL/pull/1288,
      // printNumericTable can't print correct result for CSRNumericTable, use C++ printNumericTable
      // Service.printNumericTable("Input: ", table)

      Iterator(table)
    }.cache()
  }

  def factorsToRDD(cUsersFactorsNumTab: Long, cItemsFactorsNumTab: Long)
    :(RDD[(ID, Array[Float])], RDD[(ID, Array[Float])]) = {
    val usersFactorsNumTab = OneDAL.makeNumericTable(cUsersFactorsNumTab)
    val itemsFactorsNumTab = OneDAL.makeNumericTable(cItemsFactorsNumTab)

    Service.printNumericTable("usersFactorsNumTab", usersFactorsNumTab)
    Service.printNumericTable("itemsFactorsNumTab", itemsFactorsNumTab)

    null
  }

  def run(): (RDD[(ID, Array[Float])], RDD[(ID, Array[Float])]) = {
    val executorNum = Utils.sparkExecutorNum()
    val executorCores = Utils.sparkExecutorCores()

    val largestItems = data.sortBy(_.item.toString.toLong, ascending = false).take(1)
    val nFeatures = largestItems(0).item.toString.toLong + 1

    val largestUsers = data.sortBy(_.user.toString.toLong, ascending = false).take(1)
    val nVectors = largestUsers(0).user.toString.toLong + 1

    val nBlocks = executorNum

    val nRatings = data.count()

    logInfo(s"ALSDAL fit $nRatings ratings using $executorNum Executors for $nVectors vectors and $nFeatures features")

    val executorIPAddress = Utils.sparkFirstExecutorIP(data.sparkContext)

//    val dataForConversion = if (data.getNumPartitions < executorNum) {
//      data.repartition(executorNum).setName("Repartitioned for conversion").cache()
//    } else {
//      data
//    }
//    println("data.getNumPartitions", data.getNumPartitions)

//    val numericTables = ratingsToCSRNumericTables(dataForConversion, nRatings, nVectors, nFeatures, nBlocks)
    val numericTables = ratingsToCSRNumericTables(data, nRatings, nVectors, nFeatures, nBlocks)
    val results = numericTables.mapPartitions { iter =>
      val table = iter.next()
      val context = new DaalContext()
//      table.unpack(context)

//      Service.printNumericTable("Converted Input:", table, 10)
//
//    }

//    numericTables.foreachPartition(() => _)

//    val coalescedRdd = numericTables.coalesce(1,
//      partitionCoalescer = Some(new ExecutorInProcessCoalescePartitioner()))
//
//    coalescedRdd.count()

//    val coalescedTables = coalescedRdd.mapPartitions { iter =>
//      val context = new DaalContext()
//      val mergedData = new RowMergedNumericTable(context)
//
//      println("ALSDALImpl: Loading libMLlibDAL.so" )
//      // oneDAL libs should be loaded by now, extract libMLlibDAL.so to temp file and load
//      LibLoader.loadLibraries()
//
//      iter.foreach { curIter =>
//        OneDAL.cAddNumericTable(mergedData.getCNumericTable, curIter.getCNumericTable)
//      }
//      Iterator(mergedData.getCNumericTable)
//
//    }.cache()

//    val coalescedTables = numericTables
//
//    val results = coalescedTables.mapPartitions { tableIter =>
//      val tableArr = tableIter.next()
//
//      val contextLocal = new DaalContext()
//      tableArr.unpack(contextLocal)
//
      println("ALSDALImpl: Loading libMLlibDAL.so" )
      LibLoader.loadLibraries()

      OneCCL.init(executorNum, executorIPAddress, OneCCL.KVS_PORT)

      println("nUsers", nVectors, "nItems", nFeatures)

      val result = new ALSResult()
      cDALImplictALS(
        table.getCNumericTable, nUsers = nVectors,
        rank, maxIter, regParam, alpha,
        executorNum,
        executorCores,
        result
      )
      Iterator(result)
    }.cache()

//    results.foreach { p =>
////      val usersFactorsNumTab = OneDAL.makeNumericTable(p.cUsersFactorsNumTab)
////      println("foreach", p.cUsersFactorsNumTab, p.cItemsFactorsNumTab)
//      println("result", p.rankId, p.cUserOffset, p.cItemOffset);
//    }

//    val usersFactorsRDD = results.mapPartitionsWithIndex { (index: Int, partiton: Iterator[ALSResult]) =>
//      partiton.foreach { p =>
//        val usersFactorsNumTab = OneDAL.makeNumericTable(p.cUsersFactorsNumTab)
//        Service.printNumericTable("usersFactorsNumTab", usersFactorsNumTab)
//      }
//      Iterator()
//    }.collect()

    val usersFactorsRDD = results.mapPartitionsWithIndex { (index: Int, partiton: Iterator[ALSResult]) =>
      val ret = partiton.flatMap { p =>
        val userOffset = p.cUserOffset
        val usersFactorsNumTab = OneDAL.makeNumericTable(p.cUsersFactorsNumTab)
        val nRows = usersFactorsNumTab.getNumberOfRows.toInt
        val nCols = usersFactorsNumTab.getNumberOfColumns.toInt
        var buffer = FloatBuffer.allocate(nCols * nRows)
        // should use returned buffer
        buffer = usersFactorsNumTab.getBlockOfRows(0, nRows, buffer)
        (0 until nRows).map { index =>
          val array = Array.fill(nCols){0.0f}
          buffer.get(array, 0, nCols)
          ((index+userOffset).asInstanceOf[ID], array)
        }.toIterator
      }
      ret
    }.cache()

    val itemsFactorsRDD = results.mapPartitionsWithIndex { (index: Int, partiton: Iterator[ALSResult]) =>
      val ret = partiton.flatMap { p =>
        val itemOffset = p.cItemOffset
        val itemsFactorsNumTab = OneDAL.makeNumericTable(p.cItemsFactorsNumTab)
        val nRows = itemsFactorsNumTab.getNumberOfRows.toInt
        val nCols = itemsFactorsNumTab.getNumberOfColumns.toInt
        var buffer = FloatBuffer.allocate(nCols * nRows)
        // should use returned buffer
        buffer = itemsFactorsNumTab.getBlockOfRows(0, nRows, buffer)
        (0 until nRows).map { index =>
          val array = Array.fill(nCols){0.0f}
          buffer.get(array, 0, nCols)
          ((index+itemOffset).asInstanceOf[ID], array)
        }.toIterator
      }
      ret
    }.cache()

    usersFactorsRDD.count()
    itemsFactorsRDD.count()

    usersFactorsRDD.foreach { case (id, array) =>
        println("usersFactorsRDD", id, array.mkString(", "))
    }

    itemsFactorsRDD.foreach { case (id, array) =>
      println("itemsFactorsRDD", id, array.mkString(", "))
    }

    (usersFactorsRDD, itemsFactorsRDD)
  }

  // Single entry to call Implict ALS DAL backend
  @native private def cDALImplictALS(data: Long, 
                                     nUsers: Long,
                                     rank: Int,
                                     maxIter: Int,
                                     regParam: Double,
                                     alpha: Double,
                                     executor_num: Int,
                                     executor_cores: Int,
                                     result: ALSResult): Long

}