// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_STAKEDB_H
#define BITCOIN_STAKEDB_H

#include "db.h"
#include "base58.h"
#include "stealth.h"

//class CKeyPool;
//class CAccount;
//class CAccountingentry;

/** Error statuses for the stake database */
enum SDBErrors
{
    SDB_LOAD_OK,
    SDB_CORRUPT,
    SDB_NONCRITICAL_ERROR,
    SDB_TOO_NEW,
    SDB_LOAD_FAIL,
    SDB_NEED_REWRITE
};

/** Access to the wallet database (stake.dat) */
class CStakeDB : public CDB
{
public:
    CStakeDB(std::string strFilename, const char* pszMode="r+") : CDB(strFilename.c_str(), pszMode)
    {
    }
private:
    CStakeDB(const CStakeDB&);
    void operator=(const CStakeDB&);
public:
    Dbc* GetAtCursor()
    {
        return GetCursor();
    }
    
    Dbc* GetTxnCursor()
    {
        if (!pdb)
            return NULL;
        
        DbTxn* ptxnid = activeTxn; // call TxnBegin first
        
        Dbc* pcursor = NULL;
        int ret = pdb->cursor(ptxnid, &pcursor, 0);
        if (ret != 0)
            return NULL;
        return pcursor;
    }
    
    DbTxn* GetAtActiveTxn()
    {
        return activeTxn;
    }
    
    bool WriteStake(const std::string& strAddress, const std::string& strName, const std::string& sPercent);

    bool EraseStake(const std::string& strAddress);

    bool ReadStake(const std::string& strAddress, std::string& strName, std::string& sPercent);

    bool WriteMinVersion(int nVersion)
    {
        return Write(std::string("minversion"), nVersion);
    }
public:
    SDBErrors ReorderTransactions(CWallet*);
    SDBErrors LoadWallet(CWallet* pwallet);
    static bool Recover(CDBEnv& dbenv, std::string filename, bool fOnlyKeys);
    static bool Recover(CDBEnv& dbenv, std::string filename);
};

#endif // BITCOIN_WALLETDB_H
