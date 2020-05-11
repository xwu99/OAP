package com.intel.oap.common.storage;

import java.util.Iterator;

//FIXME should new this by parameter instead of passing in by Spark
/**
 * store memta info such as map between chunkID and physical baseAddr in pmem.
 * provide methods to get chunks iterator with logicalID provided.
 */
public interface PMemMetaStore {
    /**
     *
     * @param id logical ID
     * @return whether chunks exist for this logicalID
     */
    boolean contains(byte[] id);

    /**
     *
     * @param id logical ID
     * @return
     */
    Iterator<PMemBlock> getInputChunkIterator();

    /**
     * provide trunk for output stream write, need update metadata for
     * this stream, like chunkID++, totalsize, etc. need implement methods next()
     * @param id
     * @param chunkSize
     * @return
     */
    Iterator<PMemBlock> getOutputChunkIterator();

    /**
     * get metadata for this logical stream with format <Long + Int + boolean>
     * @param id logical ID
     * @return StreamMeta
     */
    StreamMeta getStreamMeta(byte[] id);

    /**
     * put metadata info in pmem? or HashMap?
     * @param id logical ID
     * @param streamMeta
     */
    void putStreamMeta(byte[] id, StreamMeta streamMeta);

}