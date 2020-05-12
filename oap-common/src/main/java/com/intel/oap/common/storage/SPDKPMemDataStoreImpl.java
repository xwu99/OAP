package com.intel.oap.common.storage;

import java.util.Iterator;

public class SPDKPMemDataStoreImpl implements PMemDataStore {
    @Override
    public boolean contains(byte[] id) {
        throw new RuntimeException("Unsupported operation");
    }

    @Override
    public Iterator<PMemChunk> getInputChunkIterator() {
        throw new RuntimeException("Unsupported operation");
    }

    @Override
    public Iterator<PMemChunk> getOutputChunkIterator() {
        return null;
    }

    @Override
    public StreamMeta getStreamMeta(byte[] id) {
        throw new RuntimeException("Unsupported operation");
    }

    @Override
    public void putStreamMeta(byte[] id, StreamMeta streamMeta) {
        throw new RuntimeException("Unsupported operation");
    }
}
