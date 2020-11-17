#include <iostream>
#include <chrono>
#include <ccl.h>
#include <daal.h>
#include "service.h"
#include "org_apache_spark_ml_recommendation_ALSDALImpl.h"

using namespace std;
using namespace daal;
using namespace daal::algorithms::implicit_als;

/* Input data set parameters */
// string ccl_world_size = getenv("CCL_WORLD_SIZE");
const size_t nBlocks = 4;
// const size_t nBlocks = 2;
// const size_t nBlocks = stoi(ccl_world_size);

size_t rankId, comm_size;
#define ccl_root 0

/* Number of observations in transposed training data set blocks */
const string trainDatasetFileNames[] = { "/home/xiaochang/Works/onedal-experiment/als-oneccl/data/implicit_als_trans_csr_1.csv", 
                                         "/home/xiaochang/Works/onedal-experiment/als-oneccl/data/implicit_als_trans_csr_2.csv",
                                         "/home/xiaochang/Works/onedal-experiment/als-oneccl/data/implicit_als_trans_csr_3.csv",
                                         "/home/xiaochang/Works/onedal-experiment/als-oneccl/data/implicit_als_trans_csr_4.csv" };

static int usersPartition[1] = { nBlocks };

NumericTablePtr userOffset;
NumericTablePtr itemOffset;

KeyValueDataCollectionPtr userOffsetsOnMaster;
KeyValueDataCollectionPtr itemOffsetsOnMaster;

typedef float algorithmFPType; /* Algorithm floating-point type */

/* Algorithm parameters */
const size_t nUsers = 46; /* Full number of users */

const size_t nFactors      = 10; /* Number of factors */
const size_t maxIterations = 5;  /* Number of iterations in the implicit ALS training algorithm */
const double alpha = 40;         /* Confidence parameter of the implicit ALS training algorithm */
const double lambda = 0.01; 

int displs[nBlocks];
int sdispls[nBlocks];
int rdispls[nBlocks];

string colFileName;

CSRNumericTablePtr dataTable;
CSRNumericTablePtr transposedDataTable;

KeyValueDataCollectionPtr userStep3LocalInput;
KeyValueDataCollectionPtr itemStep3LocalInput;

training::DistributedPartialResultStep4Ptr itemsPartialResultLocal;
training::DistributedPartialResultStep4Ptr usersPartialResultLocal;
training::DistributedPartialResultStep4Ptr itemsPartialResultsMaster[nBlocks];

NumericTablePtr predictedRatingsLocal[nBlocks];
NumericTablePtr predictedRatingsMaster[nBlocks][nBlocks];

ByteBuffer serializedData;
ByteBuffer serializedSendData;
ByteBuffer serializedRecvData;

void initializeModel();
void readData();
void trainModel();
void testModel();
void predictRatings();

template <typename T>
void gather(const ByteBuffer & nodeResults, T * result);
void gatherItems(const ByteBuffer & nodeResults);
template <typename T>
void all2all(ByteBuffer * nodeResults, KeyValueDataCollectionPtr result);

// int main(int argc, char * argv[])
// {
//     // MPI_Init(&argc, &argv);
//     // MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
//     // MPI_Comm_rank(MPI_COMM_WORLD, &rankId);
//     ccl_init();
//     ccl_get_comm_size(NULL, &comm_size);
//     ccl_get_comm_rank(NULL, &rankId);

//     readData();
    
//     initializeModel();

//     trainModel();

//     testModel();

//     if (rankId == ccl_root)
//     {
//         for (size_t i = 0; i < nBlocks; i++)
//         {
//             for (size_t j = 0; j < nBlocks; j++)
//             {
//                 cout << "Ratings for users block " << i << ", items block " << j << " :" << endl;
//                 printALSRatings(NumericTable::cast((*userOffsetsOnMaster)[i]), NumericTable::cast((*itemOffsetsOnMaster)[j]),
//                                 predictedRatingsMaster[i][j]);
//             }
//         }
//     }

//     // MPI_Finalize();
//     ccl_finalize();

//     return 0;
// }

void readData()
{
    /* Read trainDatasetFileName from a file and create a numeric table to store the input data */
    dataTable.reset(createFloatSparseTable(trainDatasetFileNames[rankId]));
}

KeyValueDataCollectionPtr initializeStep1Local()
{
    /* Create an algorithm object to initialize the implicit ALS model with the default method */
    training::init::Distributed<step1Local, algorithmFPType, training::init::fastCSR> initAlgorithm;
    initAlgorithm.parameter.fullNUsers = nUsers;
    initAlgorithm.parameter.nFactors   = nFactors;
    initAlgorithm.parameter.seed += rankId;
    initAlgorithm.parameter.partition.reset(new HomogenNumericTable<int>((int *)usersPartition, 1, 1));
    /* Pass a training data set and dependent values to the algorithm */
    initAlgorithm.input.set(training::init::data, dataTable);

    /* Initialize the implicit ALS model */
    initAlgorithm.compute();

    training::init::PartialResultPtr partialResult = initAlgorithm.getPartialResult();
    itemStep3LocalInput                            = partialResult->get(training::init::outputOfInitForComputeStep3);
    userOffset                                     = partialResult->get(training::init::offsets, (size_t)rankId);
    if (rankId == ccl_root)
    {
        userOffsetsOnMaster = partialResult->get(training::init::offsets);
    }
    PartialModelPtr partialModelLocal = partialResult->get(training::init::partialModel);

    itemsPartialResultLocal.reset(new training::DistributedPartialResultStep4());
    itemsPartialResultLocal->set(training::outputOfStep4ForStep1, partialModelLocal);

    return partialResult->get(training::init::outputOfStep1ForStep2);
}

void initializeStep2Local(const KeyValueDataCollectionPtr & initStep2LocalInput)
{
    /* Create an algorithm object to perform the second step of the implicit ALS initialization algorithm */
    training::init::Distributed<step2Local, algorithmFPType, training::init::fastCSR> initAlgorithm;

    initAlgorithm.input.set(training::init::inputOfStep2FromStep1, initStep2LocalInput);

    /* Compute partial results of the second step on local nodes */
    initAlgorithm.compute();

    training::init::DistributedPartialResultStep2Ptr partialResult = initAlgorithm.getPartialResult();
    transposedDataTable                                            = CSRNumericTable::cast(partialResult->get(training::init::transposedData));
    userStep3LocalInput                                            = partialResult->get(training::init::outputOfInitForComputeStep3);
    itemOffset                                                     = partialResult->get(training::init::offsets, (size_t)rankId);
    if (rankId == ccl_root)
    {
        itemOffsetsOnMaster = partialResult->get(training::init::offsets);
    }
}

void initializeModel()
{
    KeyValueDataCollectionPtr initStep1LocalResult = initializeStep1Local();

    /* MPI_Alltoallv to populate initStep2LocalInput */
    ByteBuffer nodeCPs[nBlocks];
    for (size_t i = 0; i < nBlocks; i++)
    {
        serializeDAALObject((*initStep1LocalResult)[i].get(), nodeCPs[i]);
    }
    KeyValueDataCollectionPtr initStep2LocalInput(new KeyValueDataCollection());
    all2all<NumericTable>(nodeCPs, initStep2LocalInput);

    initializeStep2Local(initStep2LocalInput);
}

training::DistributedPartialResultStep1Ptr computeStep1Local(const training::DistributedPartialResultStep4Ptr & partialResultLocal)
{
    /* Create algorithm objects to compute implicit ALS algorithm in the distributed processing mode on the local node using the default method */
    training::Distributed<step1Local> algorithm;
    algorithm.parameter.nFactors = nFactors;

    /* Set input objects for the algorithm */
    algorithm.input.set(training::partialModel, partialResultLocal->get(training::outputOfStep4ForStep1));

    /* Compute partial estimates on local nodes */
    algorithm.compute();

    /* Get the computed partial estimates */
    return algorithm.getPartialResult();
}

NumericTablePtr computeStep2Master(const training::DistributedPartialResultStep1Ptr * step1LocalResultsOnMaster)
{
    /* Create algorithm objects to compute implicit ALS algorithm in the distributed processing mode on the master node using the default method */
    training::Distributed<step2Master> algorithm;
    algorithm.parameter.nFactors = nFactors;

    /* Set input objects for the algorithm */
    for (size_t i = 0; i < nBlocks; i++)
    {
        algorithm.input.add(training::inputOfStep2FromStep1, step1LocalResultsOnMaster[i]);
    }

    /* Compute a partial estimate on the master node from the partial estimates on local nodes */
    algorithm.compute();

    return algorithm.getPartialResult()->get(training::outputOfStep2ForStep4);
}

KeyValueDataCollectionPtr computeStep3Local(const NumericTablePtr & offset, const training::DistributedPartialResultStep4Ptr & partialResultLocal,
                                            const KeyValueDataCollectionPtr & step3LocalInput)
{
    training::Distributed<step3Local> algorithm;
    algorithm.parameter.nFactors = nFactors;

    algorithm.input.set(training::partialModel, partialResultLocal->get(training::outputOfStep4ForStep3));
    algorithm.input.set(training::inputOfStep3FromInit, step3LocalInput);
    algorithm.input.set(training::offset, offset);

    algorithm.compute();

    return algorithm.getPartialResult()->get(training::outputOfStep3ForStep4);
}

training::DistributedPartialResultStep4Ptr computeStep4Local(const CSRNumericTablePtr & dataTable, const NumericTablePtr & step2MasterResult,
                                                             const KeyValueDataCollectionPtr & step4LocalInput)
{
    training::Distributed<step4Local> algorithm;
    algorithm.parameter.nFactors = nFactors;

    algorithm.input.set(training::partialModels, step4LocalInput);
    algorithm.input.set(training::partialData, dataTable);
    algorithm.input.set(training::inputOfStep4FromStep2, step2MasterResult);

    algorithm.compute();

    return algorithm.getPartialResult();
}

void trainModel()
{
    training::DistributedPartialResultStep1Ptr step1LocalResultsOnMaster[nBlocks];
    training::DistributedPartialResultStep1Ptr step1LocalResult;
    NumericTablePtr step2MasterResult;
    KeyValueDataCollectionPtr step3LocalResult;
    KeyValueDataCollectionPtr step4LocalInput(new KeyValueDataCollection());

    ByteBuffer nodeCPs[nBlocks];
    ByteBuffer nodeResults;
    ByteBuffer crossProductBuf;
    int crossProductLen;

    for (size_t iteration = 0; iteration < maxIterations; iteration++)
    {
        step1LocalResult = computeStep1Local(itemsPartialResultLocal);

        serializeDAALObject(step1LocalResult.get(), nodeResults);

	// cout << "Gathering step1LocalResult on the master" << endl;
        /* Gathering step1LocalResult on the master */
        gather(nodeResults, step1LocalResultsOnMaster);
	// cout << "Finish Gathering step1LocalResult on the master" << endl;

        if (rankId == ccl_root)
        {
            step2MasterResult = computeStep2Master(step1LocalResultsOnMaster);
            serializeDAALObject(step2MasterResult.get(), crossProductBuf);
            crossProductLen = crossProductBuf.size();
        }

        ccl_request_t request;    
        
        // MPI_Bcast(&crossProductLen, sizeof(int), MPI_CHAR, ccl_root, MPI_COMM_WORLD);
        ccl_bcast(&crossProductLen, sizeof(int), ccl_dtype_char, ccl_root, NULL, NULL, NULL, &request);
        ccl_wait(request);

        if (rankId != ccl_root)
        {
            crossProductBuf.resize(crossProductLen);
        }
        // MPI_Bcast(&crossProductBuf[0], crossProductLen, MPI_CHAR, ccl_root, MPI_COMM_WORLD);
        ccl_bcast(&crossProductBuf[0], crossProductLen, ccl_dtype_char, ccl_root, NULL, NULL, NULL, &request);
        ccl_wait(request);

        step2MasterResult = NumericTable::cast(deserializeDAALObject(&crossProductBuf[0], crossProductLen));

        step3LocalResult = computeStep3Local(itemOffset, itemsPartialResultLocal, itemStep3LocalInput);

        /* MPI_Alltoallv to populate step4LocalInput */
        for (size_t i = 0; i < nBlocks; i++)
        {
            serializeDAALObject((*step3LocalResult)[i].get(), nodeCPs[i]);
        }
        all2all<PartialModel>(nodeCPs, step4LocalInput);

        usersPartialResultLocal = computeStep4Local(transposedDataTable, step2MasterResult, step4LocalInput);

        step1LocalResult = computeStep1Local(usersPartialResultLocal);

        serializeDAALObject(step1LocalResult.get(), nodeResults);

        /*Gathering step1LocalResult on the master*/
        gather(nodeResults, step1LocalResultsOnMaster);

        if (rankId == ccl_root)
        {
            step2MasterResult = computeStep2Master(step1LocalResultsOnMaster);
            serializeDAALObject(step2MasterResult.get(), crossProductBuf);
            crossProductLen = crossProductBuf.size();
        }

        // MPI_Bcast(&crossProductLen, sizeof(int), MPI_CHAR, ccl_root, MPI_COMM_WORLD);
        ccl_bcast(&crossProductLen, sizeof(int), ccl_dtype_char, ccl_root, NULL, NULL, NULL, &request);
        ccl_wait(request);

        if (rankId != ccl_root)
        {
            crossProductBuf.resize(crossProductLen);
        }

        // MPI_Bcast(&crossProductBuf[0], crossProductLen, MPI_CHAR, ccl_root, MPI_COMM_WORLD);
        ccl_bcast(&crossProductBuf[0], crossProductLen, ccl_dtype_char, ccl_root, NULL, NULL, NULL, &request);
        ccl_wait(request);
        
        step2MasterResult = NumericTable::cast(deserializeDAALObject(&crossProductBuf[0], crossProductLen));

        step3LocalResult = computeStep3Local(userOffset, usersPartialResultLocal, userStep3LocalInput);

        /* MPI_Alltoallv to populate step4LocalInput */
        for (size_t i = 0; i < nBlocks; i++)
        {
            serializeDAALObject((*step3LocalResult)[i].get(), nodeCPs[i]);
        }
        all2all<PartialModel>(nodeCPs, step4LocalInput);

        itemsPartialResultLocal = computeStep4Local(dataTable, step2MasterResult, step4LocalInput);
    }

    /*Gather all itemsPartialResultLocal to itemsPartialResultsMaster on the master and distributing the result over other ranks*/
    serializeDAALObject(itemsPartialResultLocal.get(), nodeResults);
    gatherItems(nodeResults);
}

void testModel()
{
    ByteBuffer nodeResults;
    /* Create an algorithm object to predict recommendations of the implicit ALS model */
    for (size_t i = 0; i < nBlocks; i++)
    {
        prediction::ratings::Distributed<step1Local> algorithm;
        algorithm.parameter.nFactors = nFactors;

        algorithm.input.set(prediction::ratings::usersPartialModel, usersPartialResultLocal->get(training::outputOfStep4ForStep1));
        algorithm.input.set(prediction::ratings::itemsPartialModel, itemsPartialResultsMaster[i]->get(training::outputOfStep4ForStep1));
        
        auto pUser = usersPartialResultLocal->get(training::outputOfStep4ForStep1)->getFactors();
        auto pUserIndices = usersPartialResultLocal->get(training::outputOfStep4ForStep1)->getIndices();
        auto pItem = itemsPartialResultsMaster[i]->get(training::outputOfStep4ForStep1)->getFactors();
        auto pItemIndices = itemsPartialResultsMaster[i]->get(training::outputOfStep4ForStep1)->getIndices();

        printf("For block %d\n", i);
        printNumericTable(pUser, "User Factors:");
        printNumericTable(pUserIndices, "User Indices:");
        printNumericTable(pItem, "Item Factors:");
        printNumericTable(pItemIndices, "Item Indices:");
        

        algorithm.compute();

        predictedRatingsLocal[i] = algorithm.getResult()->get(prediction::ratings::prediction);

        serializeDAALObject(predictedRatingsLocal[i].get(), nodeResults);
        gather(nodeResults, predictedRatingsMaster[i]);
    }
}

template <typename T>
void gather(const ByteBuffer & nodeResults, T * result)
{
    size_t perNodeArchLengthMaster[nBlocks];
    size_t perNodeArchLength = nodeResults.size();

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

void gatherItems(const ByteBuffer & nodeResults)
{
    size_t perNodeArchLengthMaster[nBlocks];
    size_t perNodeArchLength = nodeResults.size();

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
    for (size_t i = 0; i < nBlocks; i++)
    {
        displs[i] = shift;
        shift += perNodeArchLengthMaster[i];
    }

    /* Transfer partial results to step 2 on the root node */
    // MPI_Allgatherv(&nodeResults[0], perNodeArchLength, MPI_CHAR, &serializedData[0], perNodeArchLengthMaster, displs, MPI_CHAR, MPI_COMM_WORLD);    
    ccl_allgatherv(&nodeResults[0], perNodeArchLength, &serializedData[0], perNodeArchLengthMaster, ccl_dtype_char, NULL, NULL, NULL, &request);
    ccl_wait(request);

    for (size_t i = 0; i < nBlocks; i++)
    {
        /* Deserialize partial results from step 4 */
        itemsPartialResultsMaster[i] =
            training::DistributedPartialResultStep4::cast(deserializeDAALObject(&serializedData[0] + displs[i], perNodeArchLengthMaster[i]));
    }
}

template <typename T>
void all2all(ByteBuffer * nodeResults, KeyValueDataCollectionPtr result)
{
    size_t memoryBuf = 0;
    size_t shift  = 0;
    size_t perNodeArchLengths[nBlocks];
    size_t perNodeArchLengthsRecv[nBlocks];
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

JNIEXPORT jlong JNICALL Java_org_apache_spark_ml_recommendation_ALSDALImpl_cDALImplictALS
  (JNIEnv *env, jobject obj, jlong numTableAddr, jlong nUsers, jint nFactors, jint maxIter, jdouble regParam, jdouble alpha,
   jint executor_num, jint executor_cores, jobject resultObj)
{
    // size_t rankId;
    // ccl_get_comm_rank(NULL, &rankId);

    ccl_get_comm_size(NULL, &comm_size);
    ccl_get_comm_rank(NULL, &rankId);

    cout << "Started" << endl;

    readData();
    // dataTable = *((CSRNumericTablePtr *)numTableAddr);

    cout << "readData DONE" << endl;
    
    initializeModel();

    cout << "initializeModel DONE" << endl;

    trainModel();

    cout << "trainModel DONE" << endl;

    
    // dataTable.reset(createFloatSparseTable("/home/xiaochang/github/oneDAL-upstream/samples/daal/cpp/mpi/data/distributed/implicit_als_csr_1.csv"));

    // // printNumericTable(dataTable, "cDALImplictALS", 10);
    // cout << "getNumberOfRows: " << dataTable->getNumberOfRows() << endl;
    // cout << "getNumberOfColumns: " << dataTable->getNumberOfColumns() << endl;
    // cout << "getDataSize: " << dataTable->getDataSize() << endl;

    // // Set number of threads for oneDAL to use for each rank
    // // services::Environment::getInstance()->setNumberOfThreads(executor_cores);
    // // int nThreadsNew = services::Environment::getInstance()->getNumberOfThreads();
    // // cout << "oneDAL (native): Number of threads used: " << nThreadsNew << endl;

    // int nBlocks = executor_num;
    // initializeModel(rankId, nBlocks, nUsers, nFactors);
    // trainModel(rankId, executor_num, nFactors, maxIter);

    auto pUser = usersPartialResultLocal->get(training::outputOfStep4ForStep1)->getFactors();
    // auto pUserIndices = usersPartialResultLocal->get(training::outputOfStep4ForStep1)->getIndices();
    auto pItem = itemsPartialResultLocal->get(training::outputOfStep4ForStep1)->getFactors();
    // auto pItemIndices = itemsPartialResultsMaster[i]->get(training::outputOfStep4ForStep1)->getIndices();

//    printNumericTable(pUser, "User Factors:");
//    printNumericTable(pItem, "Item Factors:");
//    printf("native pUser %ld, pItem %ld", (jlong)&pUser, (jlong)&pItem);

    // Get the class of the input object
    jclass clazz = env->GetObjectClass(resultObj);
    // Get Field references
    jfieldID cUsersFactorsNumTabField = env->GetFieldID(clazz, "cUsersFactorsNumTab", "J");
    jfieldID cItemsFactorsNumTabField = env->GetFieldID(clazz, "cItemsFactorsNumTab", "J");

    // Set factors as result, should use heap memory
    NumericTablePtr *retUser = new NumericTablePtr(pUser);
    NumericTablePtr *retItem = new NumericTablePtr(pItem);

    env->SetLongField(resultObj, cUsersFactorsNumTabField, (jlong)retUser);
    env->SetLongField(resultObj, cItemsFactorsNumTabField, (jlong)retItem);

    return 0;
}
