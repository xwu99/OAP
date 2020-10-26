package org.apache.spark.ml.recommendation

import org.apache.spark.rdd.RDD

import scala.reflect.ClassTag
import com.intel.daal.data_management.data.{CSRNumericTable, HomogenNumericTable, Matrix => DALMatrix}
import org.apache.spark.ml.recommendation.ALS.Rating

class ALSDALImpl[ID: ClassTag](
  ratings: RDD[Rating[ID]],
  rank: Int,
  maxIter: Int,
  regParam: Double,
  alpha: Double,
  seed: Long
) extends Serializable {

  private def ratingsToCSRNumericTables(ratings: RDD[Rating[ID]]): RDD[CSRNumericTable] = {
    null
  }

//  private def initialize[ID](
//                              inBlocks: RDD[(Int, InBlock[ID])],
//                              rank: Int,
//                              seed: Long): RDD[(Int, FactorBlock)] = {
//    // Choose a unit vector uniformly at random from the unit sphere, but from the
//    // "first quadrant" where all elements are nonnegative. This can be done by choosing
//    // elements distributed as Normal(0,1) and taking the absolute value, and then normalizing.
//    // This appears to create factorizations that have a slightly better reconstruction
//    // (<1%) compared picking elements uniformly at random in [0,1].
//    inBlocks.mapPartitions({ iter =>
//      iter.map {
//        case (srcBlockId, inBlock) =>
//          val random = new XORShiftRandom(byteswap64(seed ^ srcBlockId))
//          val factors = Array.fill(inBlock.srcIds.length) {
//            val factor = Array.fill(rank)(random.nextGaussian().toFloat)
//            val nrm = blas.snrm2(rank, factor, 1)
//            blas.sscal(rank, 1.0f / nrm, factor, 1)
//            factor
//          }
//          (srcBlockId, factors)
//      }
//    }, preservesPartitioning = true)
//  }

  def run(): (RDD[(ID, Array[Float])], RDD[(ID, Array[Float])]) = {
    val numericTables = ratingsToCSRNumericTables(ratings)
    null
  }

  // Single entry to call Implict ALS DAL backend
  @native private def cDALImplictALS(data: Long, rank: Int,
                                     maxIter: Int,
                                     regParam: Double,
                                     alpha: Double,
                                     executor_num: Int,
                                     executor_cores: Int,
                                     result: ALSResult): Long

}