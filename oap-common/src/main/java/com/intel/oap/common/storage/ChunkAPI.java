package com.intel.oap.common.storage;

/**
 * expose methods for high level stream to write/read data from different backend
 */
public interface ChunkAPI {
    /**
     * trunkID: logicalID + trunkIndex
     * @param id  unique trunkID, mapping to physical pmem address
     * @param value value to write on pmem
     */
    void write(byte[] id, byte[] value);

    /**
     * check whether this trunk exists
     * @param id trunkID
     * @return
     */
    boolean contains(byte[] id);

    /**
     *
     * @param id trunkID
     * @return base address of trunk with id
     */
    public long getChunk(byte[] id);


    /**
     * update pMemManager.pMemDataStore with <trunkID, pMemBlock>   ?????
     * @param id
     * @param pMemBlock
     */
    public void putChunk(byte[] id, UnsafeMemoryBlock pMemBlock);

    /**
     *
     * @param id unique trunkID
     * @param offset start position of read
     * @param len  length of bytes to read
     * @return
     */
    public boolean read(byte[] id, int offset, int len);

    /**
     * action when read done of current Chunk
     */
    public void release();

    /**
     * free pmem space when chunk life cycle end
     * @param pMemChunk
     */
    public void free(PMemChunk pMemChunk);
}