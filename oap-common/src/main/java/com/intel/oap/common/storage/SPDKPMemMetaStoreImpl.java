package com.intel.oap.common.storage;

import java.util.Iterator;

public class SPDKPMemMetaStoreImpl implements PMemMetaStore {
    @Override
    public boolean contains(byte[] id) {
        throw new RuntimeException("Unsupported operation");
    }

    @Override
    public Iterator<PMemBlock> getInputChunkIterator() {
        throw new RuntimeException("Unsupported operation");
    }

    @Override
    public Iterator<PMemBlock> getOutputChunkIterator() {
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
