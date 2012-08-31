// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "bitcoinrpc.h"

using namespace json_spirit;
using namespace std;

double GetDifficulty(const CBlockIndex* blockindex)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (blockindex == NULL)
    {
        if (pindexBest == NULL)
            return 1.0;
        else
            blockindex = pindexBest;
    }

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}


Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    Array txs;
    BOOST_FOREACH(const CTransaction&tx, block.vtx)
        txs.push_back(tx.GetHash().GetHex());
    result.push_back(Pair("tx", txs));
    result.push_back(Pair("time", (boost::int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (boost::uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex)));

    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));
    return result;
}


Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "Returns the number of blocks in the longest block chain.");

    return nBestHeight;
}


Value getblockbycount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockbycount height\n"
            "Dumps the block existing at specified height");

    int64 height = params[0].get_int64();
    if (height > nBestHeight)
        throw runtime_error(
            "getblockbycount height\n"
            "Dumps the block existing at specified height");

    string blkname = strprintf("blk%d", height);

    CBlockIndex* pindex;
    bool found = false;

    for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin();
         mi != mapBlockIndex.end(); ++mi)
    {
    	pindex = (*mi).second;
	if ((pindex->nHeight == height) && (pindex->IsInMainChain())) {
		found = true;
		break;
	}
    }

    if (!found)
        throw runtime_error(
            "getblockbycount height\n"
            "Dumps the block existing at specified height");

    CBlock block;
    block.ReadFromDisk(pindex);
    block.BuildMerkleTree();

    Object obj;
    obj.push_back(Pair("hash", block.GetHash().ToString().c_str()));
    obj.push_back(Pair("version", block.nVersion));
    obj.push_back(Pair("prev_block", block.hashPrevBlock.ToString().c_str()));
    obj.push_back(Pair("mrkl_root", block.hashMerkleRoot.ToString().c_str()));
    obj.push_back(Pair("time", (uint64_t)block.nTime));
    obj.push_back(Pair("bits", (uint64_t)block.nBits));
    obj.push_back(Pair("nonce", (uint64_t)block.nNonce));
    obj.push_back(Pair("n_tx", (int)block.vtx.size()));
    obj.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, CLIENT_VERSION)));
    //obj.push_back(Pair("size", (int)::GetSerializeSize(CBlock(), SER_DISK, CLIENT_VERSION)));

    Array tx;
    for (unsigned int i = 0; i < block.vtx.size(); i++) {
    	Object txobj;

	txobj.push_back(Pair("hash", block.vtx[i].GetHash().ToString().c_str()));
	txobj.push_back(Pair("version", block.vtx[i].nVersion));
	txobj.push_back(Pair("lock_time", (uint64_t)block.vtx[i].nLockTime));
	txobj.push_back(Pair("size",
		(int)::GetSerializeSize(block.vtx[i], SER_NETWORK, PROTOCOL_VERSION)));

	Array tx_vin;
	for (unsigned int j = 0; j < block.vtx[i].vin.size(); j++) {
	    Object vino;

	    Object vino_outpt;

	    vino_outpt.push_back(Pair("hash",
	    	block.vtx[i].vin[j].prevout.hash.ToString().c_str()));
	    vino_outpt.push_back(Pair("n", (uint64_t)block.vtx[i].vin[j].prevout.n));

	    vino.push_back(Pair("prev_out", vino_outpt));

	    if (block.vtx[i].vin[j].prevout.IsNull())
	    	vino.push_back(Pair("coinbase", HexStr(
			block.vtx[i].vin[j].scriptSig.begin(),
			block.vtx[i].vin[j].scriptSig.end(), false).c_str()));
	    else
	    	vino.push_back(Pair("scriptSig", 
			block.vtx[i].vin[j].scriptSig.ToString().c_str()));
	    if (block.vtx[i].vin[j].nSequence != UINT_MAX)
	    	vino.push_back(Pair("sequence", (uint64_t)block.vtx[i].vin[j].nSequence));

	    tx_vin.push_back(vino);
	}

	Array tx_vout;
	for (unsigned int j = 0; j < block.vtx[i].vout.size(); j++) {
	    Object vouto;

	    vouto.push_back(Pair("value",
	    	(double)block.vtx[i].vout[j].nValue / (double)COIN));
	    vouto.push_back(Pair("scriptPubKey", 
		block.vtx[i].vout[j].scriptPubKey.ToString().c_str()));

	    tx_vout.push_back(vouto);
	}

	txobj.push_back(Pair("in", tx_vin));
	txobj.push_back(Pair("out", tx_vout));

	tx.push_back(txobj);
    }

    obj.push_back(Pair("tx", tx));

    Array mrkl;
    for (unsigned int i = 0; i < block.vMerkleTree.size(); i++)
    	mrkl.push_back(block.vMerkleTree[i].ToString().c_str());

    obj.push_back(Pair("mrkl_tree", mrkl));

    return obj;
}




Value getdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "Returns the proof-of-work difficulty as a multiple of the minimum difficulty.");

    return GetDifficulty();
}


Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "settxfee <amount>\n"
            "<amount> is a real and is rounded to the nearest 0.00000001");

    // Amount
    int64 nAmount = 0;
    if (params[0].get_real() != 0.0)
        nAmount = AmountFromValue(params[0]);        // rejects 0.0 amounts

    nTransactionFee = nAmount;
    return true;
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getrawmempool\n"
            "Returns all transaction ids in memory pool.");

    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    Array a;
    BOOST_FOREACH(const uint256& hash, vtxid)
        a.push_back(hash.ToString());

    return a;
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash <index>\n"
            "Returns hash of block in best-block-chain at <index>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = FindBlockByHeight(nHeight);
    return pblockindex->phashBlock->GetHex();
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblock <hash>\n"
            "Returns details of a block with given block-hash.");

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(-5, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex);
}





