/*******************************************************************************
* Copyright 2020 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <daal.h>
#include "org_apache_spark_ml_util_OneDAL__.h"

using namespace daal;
using namespace daal::data_management;

/*
 * Class:     org_apache_spark_ml_util_OneDAL__
 * Method:    setNumericTableValue
 * Signature: (JIID)V
 */
JNIEXPORT void JNICALL Java_org_apache_spark_ml_util_OneDAL_00024_setNumericTableValue
  (JNIEnv *, jobject, jlong numTableAddr, jint row, jint column, jdouble value) {

  HomogenNumericTable<double> * nt = static_cast<HomogenNumericTable<double> *>(((SerializationIfacePtr *)numTableAddr)->get());
  (*nt)[row][column]               = (double)value;

}

JNIEXPORT void JNICALL Java_org_apache_spark_ml_util_OneDAL_00024_cSetDoubleIterator
  (JNIEnv *env, jobject,jlong numTableAddr, jobject jiter) {
    
    jclass iterClass = env->FindClass("java/util/Iterator");
    jmethodID hasNext = env->GetMethodID(iterClass,
                                          "hasNext", "()Z");
    jmethodID next = env->GetMethodID(iterClass,
                                       "next", "()Ljava/lang/Object;");

    HomogenNumericTable<double> *nt = static_cast<HomogenNumericTable<double> *>(
                ((SerializationIfacePtr *)numTableAddr)->get());
    int totalRows = 0;
    while (env->CallBooleanMethod(jiter, hasNext)) {
         jobject batch = env->CallObjectMethod(jiter, next);
		 
         jclass batchClass = env->GetObjectClass(batch);
         jlongArray joffset = (jlongArray)env->GetObjectField(
              batch, env->GetFieldID(batchClass, "rowOffset", "[J"));
         jdoubleArray jvalue = (jdoubleArray)env->GetObjectField(
              batch, env->GetFieldID(batchClass, "values", "[D"));
         jint jcols = env->GetIntField(
              batch, env->GetFieldID(batchClass, "numCols", "I"));

         long numRows = env->GetArrayLength(joffset);

         jlong* rowOffset = env->GetLongArrayElements(joffset, 0);
		 
		 jdouble* values = env->GetDoubleArrayElements(jvalue, 0);

         long numValues = env->GetArrayLength(jvalue);
         for (int i = 0; i < numRows; i ++){
              jlong curRow = rowOffset[i] + totalRows;
            for(int j = 0; j < jcols; j ++) {

                (*nt)[curRow][j] = values[rowOffset[i] * jcols + j];
            }
         }
         totalRows += numRows;
         env->ReleaseLongArrayElements(joffset, rowOffset, 0);
         env->DeleteLocalRef(joffset);
         env->ReleaseDoubleArrayElements(jvalue, values, 0);
         env->DeleteLocalRef(jvalue);
         env->DeleteLocalRef(batch);
         env->DeleteLocalRef(batchClass);
  }
  env->DeleteLocalRef(iterClass);

  }

