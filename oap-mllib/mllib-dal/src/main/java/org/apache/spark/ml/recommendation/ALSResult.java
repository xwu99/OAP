package org.apache.spark.ml.recommendation;

public class ALSResult {
    public long rankId = -1;
    public long cUsersFactorsNumTab;
    public long cItemsFactorsNumTab;
    public long cUserOffsetsOnMaster[];
    public long cItemOffsetsOnMaster[];

    ALSResult(int nBlocks) {
        cUserOffsetsOnMaster = new long[nBlocks];
        cItemOffsetsOnMaster = new long[nBlocks];
    }
}
