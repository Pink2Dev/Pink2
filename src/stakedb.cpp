// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stakedb.h"
#include "wallet.h"

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;

//
// CStakeDB
//

bool CStakeDB::WriteStake(const string& strAddress, const string& strName, const string& sPercent)
{
    nStakeDBUpdated++;
    bool success = true;

    if (!Write(make_pair(string("name"), strAddress), strName))
        success = false;
    if (!Write(make_pair(string("percent"), strAddress), sPercent))
        success = false;
    return success;
}

bool CStakeDB::EraseStake(const string& strAddress)
{
    nStakeDBUpdated++;
    bool success=true;

    if (!Erase(make_pair(string("name"), strAddress)))
        success = false;
    if (!Erase(make_pair(string("percent"), strAddress)))
        success = false;

    return success;
}

bool CStakeDB::ReadStake(const string& strAddress, string& strName, string& sPercent)
{
    bool success = true;

    if (!Read(make_pair(string("name"), strAddress), strName))
        success = false;
    if (!Read(make_pair(string("percent"), strAddress), sPercent))
        success = false;

    return success;
}

bool
ReadKeyValue(CWallet* pwallet, CDataStream& ssKey, CDataStream& ssValue, string& strType, string& strErr)
{
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization
        // is just the two items serialized one after the other
        ssKey >> strType;
        if (strType == "name")
        {
            string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet->mapAddressBook[CBitcoinAddress(strAddress).Get()];
        }
        else if (strType == "percent")
        {
            string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet->mapAddressPercent[CBitcoinAddress(strAddress).Get()];
        }
    } catch (...)
    {
        return false;
    }

    return true;

}

SDBErrors CStakeDB::LoadWallet(CWallet* pwallet)
{
    pwallet->vchDefaultKey = CPubKey();

    bool fNoncriticalErrors = false;
    SDBErrors result = SDB_LOAD_OK;

    try {
        LOCK(pwallet->cs_wallet);
        int nMinVersion = 0;
        if (Read((string)"minversion", nMinVersion))
        {
            if (nMinVersion > CLIENT_VERSION)
                return SDB_TOO_NEW;
            pwallet->LoadMinVersion(nMinVersion);
        }

        // Get cursor
        Dbc* pcursor = GetCursor();
        if (!pcursor)
        {
            printf("Error getting wallet database cursor\n");
            return SDB_CORRUPT;
        }

        while (true)
        {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                printf("Error reading next record from wallet database\n");
                return SDB_CORRUPT;
            }

            // Try to be tolerant of single corrupt records:
            string strType, strErr;
            if (!ReadKeyValue(pwallet, ssKey, ssValue, strType, strErr))
            {
                printf("\n\nError: Debug ReadKeyValue for StakeDB \n\n");
            }
            if (!strErr.empty())
                printf("%s\n", strErr.c_str());

        }
        pcursor->close();
    }
    catch (...)
    {
        result = SDB_CORRUPT;
    }

    if (fNoncriticalErrors && result == SDB_LOAD_OK)
        result = SDB_NONCRITICAL_ERROR;

    // Any stakedb corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != SDB_LOAD_OK)
        return result;

    return result;
}

void ThreadFlushStakeDB(void* parg)
{
    // Make this thread recognisable as the wallet flushing thread
    RenameThread("pinkcoin-stake");

    const string& strFile = ((const string*)parg)[0];
    static bool fOneThread;
    if (fOneThread)
        return;
    fOneThread = true;
    if (!GetBoolArg("-flushwallet", true))
        return;

    unsigned int nLastSeen = nStakeDBUpdated;
    unsigned int nLastFlushed = nStakeDBUpdated;
    int64_t nLastStakeUpdate = GetTime();
    while (!fShutdown)
    {
        MilliSleep(500);

        if (nLastSeen != nStakeDBUpdated)
        {
            nLastSeen = nStakeDBUpdated;
            nLastStakeUpdate = GetTime();
        }

        if (nLastFlushed != nStakeDBUpdated && GetTime() - nLastStakeUpdate >= 1)
        {
            TRY_LOCK(bitdb.cs_db,lockDb);
            if (lockDb)
            {
                // Don't do this if any databases are in use
                int nRefCount = 0;
                map<string, int>::iterator mi = bitdb.mapFileUseCount.begin();
                while (mi != bitdb.mapFileUseCount.end())
                {
                    nRefCount += (*mi).second;
                    mi++;
                }

                if (nRefCount == 0 && !fShutdown)
                {
                    map<string, int>::iterator mi = bitdb.mapFileUseCount.find(strFile);
                    if (mi != bitdb.mapFileUseCount.end())
                    {
                        printf("Flushing stake.dat\n");
                        nLastFlushed = nStakeDBUpdated;
                        int64_t nStart = GetTimeMillis();

                        // Flush stake.dat so it's self contained
                        bitdb.CloseDb(strFile);
                        bitdb.CheckpointLSN(strFile);

                        bitdb.mapFileUseCount.erase(mi++);
                        printf("Flushed stake.dat %" PRId64 "ms\n", GetTimeMillis() - nStart);
                    }
                }
            }
        }
    }
}

bool BackupStakeDB(const CWallet& stakeDB, const string& strDest)
{
    if (!stakeDB.fFileBacked)
        return false;
    while (!fShutdown)
    {
        {
            LOCK(bitdb.cs_db);
            if (!bitdb.mapFileUseCount.count(stakeDB.strWalletFile) || bitdb.mapFileUseCount[stakeDB.strWalletFile] == 0)
            {
                // Flush log data to the dat file
                bitdb.CloseDb(stakeDB.strWalletFile);
                bitdb.CheckpointLSN(stakeDB.strWalletFile);
                bitdb.mapFileUseCount.erase(stakeDB.strWalletFile);

                // Copy wallet.dat
                filesystem::path pathSrc = GetDataDir() / stakeDB.strWalletFile;
                filesystem::path pathDest(strDest);
                if (filesystem::is_directory(pathDest))
                    pathDest /= stakeDB.strWalletFile;

                try {
#if BOOST_VERSION >= 104000
                    filesystem::copy_file(pathSrc, pathDest, filesystem::copy_option::overwrite_if_exists);
#else
                    filesystem::copy_file(pathSrc, pathDest);
#endif
                    printf("copied wallet.dat to %s\n", pathDest.string().c_str());
                    return true;
                } catch(const filesystem::filesystem_error &e) {
                    printf("error copying wallet.dat to %s - %s\n", pathDest.string().c_str(), e.what());
                    return false;
                }
            }
        }
        MilliSleep(100);
    }
    return false;
}

//
// Try to (very carefully!) recover stake.dat if there is a problem.
//
bool CStakeDB::Recover(CDBEnv& dbenv, std::string filename, bool fOnlyKeys)
{
    // Recovery procedure:
    // move stake.dat to stake.timestamp.bak
    // Call Salvage with fAggressive=true to
    // get as much data as possible.
    // Rewrite salvaged data to wallet.dat
    // Set -rescan so any missing transactions will be
    // found.
    int64_t now = GetTime();
    std::string newFilename = strprintf("stake.%" PRId64 ".bak", now);

    int result = dbenv.dbenv.dbrename(NULL, filename.c_str(), NULL,
                                      newFilename.c_str(), DB_AUTO_COMMIT);
    if (result == 0)
        printf("Renamed %s to %s\n", filename.c_str(), newFilename.c_str());
    else
    {
        printf("Failed to rename %s to %s\n", filename.c_str(), newFilename.c_str());
        return false;
    }

    std::vector<CDBEnv::KeyValPair> salvagedData;
    bool allOK = dbenv.Salvage(newFilename, true, salvagedData);
    if (salvagedData.empty())
    {
        printf("Salvage(aggressive) found no records in %s.\n", newFilename.c_str());
        return false;
    }
    printf("Salvage(aggressive) found %" PRIszu " records\n", salvagedData.size());

    bool fSuccess = allOK;
    Db* pdbCopy = new Db(&dbenv.dbenv, 0);
    int ret = pdbCopy->open(NULL,                 // Txn pointer
                            filename.c_str(),   // Filename
                            "main",    // Logical db name
                            DB_BTREE,  // Database type
                            DB_CREATE,    // Flags
                            0);
    if (ret > 0)
    {
        printf("Cannot create database file %s\n", filename.c_str());
        return false;
    }
    CWallet dummyStakeDB;

    DbTxn* ptxn = dbenv.TxnBegin();
    BOOST_FOREACH(CDBEnv::KeyValPair& row, salvagedData)
    {
        if (fOnlyKeys)
        {
            CDataStream ssKey(row.first, SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(row.second, SER_DISK, CLIENT_VERSION);
            string strType, strErr;
            bool fReadOK = ReadKeyValue(&dummyStakeDB, ssKey, ssValue, strType, strErr);
            if (!fReadOK)
            {
                printf("WARNING: CStakeDB::Recover skipping %s: %s\n", strType.c_str(), strErr.c_str());
                continue;
            }
        }
        Dbt datKey(&row.first[0], row.first.size());
        Dbt datValue(&row.second[0], row.second.size());
        int ret2 = pdbCopy->put(ptxn, &datKey, &datValue, DB_NOOVERWRITE);
        if (ret2 > 0)
            fSuccess = false;
    }
    ptxn->commit(0);
    pdbCopy->close(0);
    delete pdbCopy;

    return fSuccess;
}

bool CStakeDB::Recover(CDBEnv& dbenv, std::string filename)
{
    return CStakeDB::Recover(dbenv, filename, false);
}
