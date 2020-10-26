#include <iostream>
#include <chrono>
#include <ccl.h>
#include <daal.h>
#include "service.h"
#include "org_apache_spark_ml_recommendation_ALSDALImpl.h"

using namespace std;
using namespace daal;
using namespace daal::algorithms;
using namespace daal::algorithms::implicit_als;

const int ccl_root = 0;

template <typename T>
void gather(size_t rankId, int nBlocks, const ByteBuffer & nodeResults,  T * result)
{
    size_t perNodeArchLengthMaster[nBlocks];
    size_t perNodeArchLength = nodeResults.size();
    ByteBuffer serializedData;
    ccl_request_t request;

    size_t recv_counts[nBlocks];
    for (int i = 0; i < nBlocks; i++)
        recv_counts[i] = sizeof(size_t);

    // MPI_Gather(&perNodeArchLength, sizeof(int), MPI_CHAR, perNodeArchLengthMaster, sizeof(int), MPI_CHAR, ccl_root, MPI_COMM_WORLD);
    ccl_allgatherv(&perNodeArchLength, sizeof(size_t), perNodeArchLengthMaster, recv_counts, ccl_dtype_char, NULL, NULL, NULL, &request);
    ccl_wait(request);
    
    // should resize for all ranks for ccl_allgatherv
    size_t memoryBuf = 0;
    for (size_t i = 0; i < nBlocks; i++)
    {
        memoryBuf += perNodeArchLengthMaster[i];
    }
    serializedData.resize(memoryBuf);

    std::vector<int> displs(nBlocks);
    if (rankId == ccl_root)
    {
        size_t shift = 0;
        for (size_t i = 0; i < nBlocks; i++)
        {
            displs[i] = shift;
            shift += perNodeArchLengthMaster[i];
        }
    }
    
    /* Transfer partial results to step 2 on the root node */
    // MPI_Gatherv(&nodeResults[0], perNodeArchLength, MPI_CHAR, &serializedData[0], perNodeArchLengthMaster, displs, MPI_CHAR, ccl_root,
    //             MPI_COMM_WORLD);    
    ccl_allgatherv(&nodeResults[0], perNodeArchLength, &serializedData[0], perNodeArchLengthMaster, ccl_dtype_char, NULL, NULL, NULL, &request);
    ccl_wait(request);    

    if (rankId == ccl_root)
    {
        for (size_t i = 0; i < nBlocks; i++)
        {
            /* Deserialize partial results from step 1 */
            result[i] = result[i]->cast(deserializeDAALObject(&serializedData[0] + displs[i], perNodeArchLengthMaster[i]));
        }
    }
    
}

void gatherItems(const ByteBuffer & nodeResults, training::DistributedPartialResultStep4Ptr itemsPartialResultsMaster[], int nBlocks)
{
    size_t perNodeArchLengthMaster[nBlocks];
    size_t perNodeArchLength = nodeResults.size();
    ByteBuffer serializedData;
    size_t recv_counts[nBlocks];
    for (int i = 0; i < nBlocks; i++) {
        recv_counts[i] = sizeof(size_t);
    }    

    ccl_request_t request;    
    // MPI_Allgather(&perNodeArchLength, sizeof(int), MPI_CHAR, perNodeArchLengthMaster, sizeof(int), MPI_CHAR, MPI_COMM_WORLD);
    ccl_allgatherv(&perNodeArchLength, sizeof(size_t), perNodeArchLengthMaster, recv_counts, ccl_dtype_char, NULL, NULL, NULL, &request);
    ccl_wait(request);

    size_t memoryBuf = 0;
    for (int i = 0; i < nBlocks; i++)
    {
        memoryBuf += perNodeArchLengthMaster[i];
    }
    serializedData.resize(memoryBuf);

    size_t shift = 0;
    std::vector<int> displs(nBlocks);
    for (int i = 0; i < nBlocks; i++)
    {
        displs[i] = shift;
        shift += perNodeArchLengthMaster[i];
    }

    /* Transfer partial results to step 2 on the root node */
    // MPI_Allgatherv(&nodeResults[0], perNodeArchLength, MPI_CHAR, &serializedData[0], perNodeArchLengthMaster, displs, MPI_CHAR, MPI_COMM_WORLD);    
    ccl_allgatherv(&nodeResults[0], perNodeArchLength, &serializedData[0], perNodeArchLengthMaster, ccl_dtype_char, NULL, NULL, NULL, &request);
    ccl_wait(request);

    for (int i = 0; i < nBlocks; i++)
    {
        /* Deserialize partial results from step 4 */
        itemsPartialResultsMaster[i] =
            training::DistributedPartialResultStep4::cast(deserializeDAALObject(&serializedData[0] + displs[i], perNodeArchLengthMaster[i]));
    }
}

template <typename T>
void all2all(ByteBuffer * nodeResults, int nBlocks, KeyValueDataCollectionPtr result)
{
    size_t memoryBuf = 0;
    size_t shift  = 0;
    size_t perNodeArchLengths[nBlocks];
    size_t perNodeArchLengthsRecv[nBlocks];
    std::vector<size_t> sdispls(nBlocks);
    ByteBuffer serializedSendData;
    ByteBuffer serializedRecvData;

    for (int i = 0; i < nBlocks; i++)
    {
        perNodeArchLengths[i] = nodeResults[i].size();        
        memoryBuf += perNodeArchLengths[i];
        sdispls[i] = shift;
        shift += perNodeArchLengths[i];
    }      
    serializedSendData.resize(memoryBuf);

    /* memcpy to avoid double compute */
    memoryBuf = 0;
    for (int i = 0; i < nBlocks; i++)
    {
        for (int j = 0; j < perNodeArchLengths[i]; j++) serializedSendData[memoryBuf + j] = nodeResults[i][j];
        memoryBuf += perNodeArchLengths[i];
    }

    ccl_request_t request;    
    // MPI_Alltoall(perNodeArchLengths, sizeof(int), MPI_CHAR, perNodeArchLengthsRecv, sizeof(int), MPI_CHAR, MPI_COMM_WORLD);
    ccl_alltoall(perNodeArchLengths, perNodeArchLengthsRecv, sizeof(size_t), ccl_dtype_char, NULL, NULL, NULL, &request);
    ccl_wait(request);    

    memoryBuf = 0;
    shift     = 0;
    std::vector<size_t> rdispls(nBlocks);
    for (int i = 0; i < nBlocks; i++)
    {
        memoryBuf += perNodeArchLengthsRecv[i];
        rdispls[i] = shift;
        shift += perNodeArchLengthsRecv[i];
    }
    
    serializedRecvData.resize(memoryBuf);

    /* Transfer partial results to step 2 on the root node */
    // MPI_Alltoallv(&serializedSendData[0], perNodeArchLengths, sdispls, MPI_CHAR, &serializedRecvData[0], perNodeArchLengthsRecv, rdispls, MPI_CHAR,
    //               MPI_COMM_WORLD);    
    ccl_alltoallv(&serializedSendData[0], perNodeArchLengths, &serializedRecvData[0], perNodeArchLengthsRecv, ccl_dtype_char, NULL, NULL, NULL, &request);    
    ccl_wait(request); 

    for (size_t i = 0; i < nBlocks; i++)
    {
        (*result)[i] = T::cast(deserializeDAALObject(&serializedRecvData[rdispls[i]], perNodeArchLengthsRecv[i]));
    }
}