package com.intel.oap.common.storage;

public class PMemChunk {
    ChunkAPI chunkAPI;
    byte[] id;

    public PMemChunk(ChunkAPI chunkAPI, byte[] id){
        this.chunkAPI = chunkAPI;
        this.id = id;
    }

    public void writeDataToStore(Object baseObj, long baseAddress, long offset){
        chunkAPI.putChunk(id, new UnsafeMemoryBlock(baseObj, baseAddress, offset));
    }

}
