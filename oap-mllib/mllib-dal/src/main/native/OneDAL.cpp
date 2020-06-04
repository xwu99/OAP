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

