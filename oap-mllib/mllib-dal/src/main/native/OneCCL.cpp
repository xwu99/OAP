#include <iostream>
#include <ccl.h>
#include "org_apache_spark_ml_util_OneCCL__.h"

static void test_allreduce(int rank, int size) {
    
    #define COUNT 128

    int i = 0;

    int sendbuf[COUNT];
    int recvbuf[COUNT];

    ccl_request_t request;
    ccl_stream_t stream;

    /* create CPU stream */
    ccl_stream_create(ccl_stream_cpu, NULL, &stream);

    /* initialize sendbuf */
    for (i = 0; i < COUNT; i++) {
        sendbuf[i] = rank;
    }

    /* modify sendbuf */
    for (i = 0; i < COUNT; i++) {
        sendbuf[i] += 1;
    }

    /* invoke ccl_allreduce */
    ccl_allreduce(sendbuf,
                  recvbuf,
                  COUNT,
                  ccl_dtype_int,
                  ccl_reduction_sum,
                  NULL, /* attr */
                  NULL, /* comm */
                  stream,
                  &request);

    ccl_wait(request);

    /* check correctness of recvbuf */
    for (i = 0; i < COUNT; i++) {
       if (recvbuf[i] != size * (size + 1) / 2) {
           recvbuf[i] = -1;
       }
    }

    /* print out the result of the test */
    if (rank == 0) {
        for (i = 0; i < COUNT; i++) {
            if (recvbuf[i] == -1) {
                printf("FAILED\n");
                break;
            }
        }
        if (i == COUNT) {
            printf("PASSED\n");
        }
    }

    ccl_stream_free(stream);
}

JNIEXPORT jint JNICALL Java_org_apache_spark_ml_util_OneCCL_00024_c_1init
  (JNIEnv *env, jobject obj, jobject param) {
  
  std::cout << "\noneCCL: init" << std::endl;

  ccl_init();  

  jclass cls = env->GetObjectClass(param);
  jfieldID fid_comm_size = env->GetFieldID(cls, "commSize", "J");
  jfieldID fid_rank_id = env->GetFieldID(cls, "rankId", "J");

  size_t comm_size;
  size_t rank_id;

  ccl_get_comm_size(NULL, &comm_size);
  ccl_get_comm_rank(NULL, &rank_id);

//   test_allreduce(rank_id, comm_size);

  env->SetLongField(param, fid_comm_size, comm_size);
  env->SetLongField(param, fid_rank_id, rank_id);    

  return 1;
}

/*
 * Class:     org_apache_spark_ml_util_OneCCL__
 * Method:    c_cleanup
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_apache_spark_ml_util_OneCCL_00024_c_1cleanup
  (JNIEnv *env, jobject obj) {

  std::cout << "\noneCCL: cleanup" << std::endl;   

  ccl_finalize();
}

/*
 * Class:     org_apache_spark_ml_util_OneCCL__
 * Method:    isRoot
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_apache_spark_ml_util_OneCCL_00024_isRoot
  (JNIEnv *env, jobject obj) {

    size_t rank_id;
    ccl_get_comm_rank(NULL, &rank_id);

    return (rank_id == 0);
}

/*
 * Class:     org_apache_spark_ml_util_OneCCL__
 * Method:    rankID
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_apache_spark_ml_util_OneCCL_00024_rankID
  (JNIEnv *env, jobject obj) {

    size_t rank_id;
    ccl_get_comm_rank(NULL, &rank_id);

    return rank_id;

}