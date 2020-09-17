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

package org.apache.spark.ml.feature

import com.intel.daal.algorithms.PCAResult
import org.apache.spark.ml.linalg._
import org.apache.spark.ml.util.{OneCCL, OneDAL, Utils}
import org.apache.spark.mllib.feature.{PCAModel => MLlibPCAModel}
import org.apache.spark.mllib.linalg.{DenseMatrix => OldDenseMatrix, Vectors => OldVectors}
import org.apache.spark.rdd.RDD


class PCADALImpl (
    val k: Int,
    val executorNum: Int,
    val executorCores: Int) extends Serializable {

  def fitWithCorrelation(input: RDD[Vector]) : MLlibPCAModel = {

    val coalescedTables = OneDAL.rddVectorToNumericTables(input, executorNum)

    val executorIPAddress = Utils.sparkFirstExecutorIP(input.sparkContext)

    val results = coalescedTables.mapPartitions { table =>
      val tableArr = table.next()
      OneCCL.init(executorNum, executorIPAddress, OneCCL.KVS_PORT)

      var result = new PCAResult()
      cPCADALCorrelation(
        tableArr,
        k,
        executorNum,
        executorCores,
        result
      )

      val ret = if (OneCCL.isRoot()) {

        val pcNumericTable = OneDAL.makeNumericTable(result.pcNumericTable)
        val explainedVarianceNumericTable = OneDAL.makeNumericTable(result.explainedVarianceNumericTable)

        val principleComponents = OneDAL.numericTableToDenseMatrix(pcNumericTable)
        val explainedVariance = OneDAL.numericTable1xnToDenseVector(explainedVarianceNumericTable)

        Iterator((principleComponents, explainedVariance))
      } else {
        Iterator.empty
      }

//      OneCCL.cleanup()

      ret
    }.collect()

    // Make sure there is only one result from rank 0
    assert(results.length == 1)

    val pc = results(0)._1
    val explainedVariance = results(0)._2

    val parentModel = new MLlibPCAModel(k,
      OldDenseMatrix.fromML(pc),
      OldVectors.fromML(explainedVariance).toDense
    )

    parentModel
  }

  // Single entry to call Correlation PCA DAL backend with parameter K
  @native private def cPCADALCorrelation(data: Long,
                                         k: Int,
                                         executor_num: Int,
                                         executor_cores: Int,
                                         result: PCAResult): Long

}
