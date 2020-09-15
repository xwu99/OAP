#include <ccl.h>
#include <daal.h>

#include "org_apache_spark_ml_feature_PCADALImpl.h"
#include <iostream>
#include <chrono>

/*
 * Class:     org_apache_spark_ml_feature_PCADALImpl
 * Method:    cPCADALCorrelation
 * Signature: (JIIILcom/intel/daal/algorithms/PCAResult;)J
 */
JNIEXPORT jlong JNICALL Java_org_apache_spark_ml_feature_PCADALImpl_cPCADALCorrelation
  (JNIEnv *, jobject,
  jlong pNumTabData, jint k,
  jint executor_num, jint executor_cores,
  jobject resultObj) {

}