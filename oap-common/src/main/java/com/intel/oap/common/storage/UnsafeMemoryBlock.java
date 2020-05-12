package com.intel.oap.common.storage;

public class UnsafeMemoryBlock {
    Object baseObj;
    long baseAdj;
    long size;

    public UnsafeMemoryBlock(){}

    public UnsafeMemoryBlock(Object baseObj, long baseAdj, long size){
        this.baseObj = baseObj;
        this.baseAdj = baseAdj;
        this.size = size;
    }
}
