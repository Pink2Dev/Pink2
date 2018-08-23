// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_INIT_H
#define BITCOIN_INIT_H

#include "wallet.h"
namespace boost {
    class thread_group;
} // namespace boost

extern CWallet* pwalletMain;
extern CWallet* pstakeDB;
void StartShutdown();
void Shutdown(void* parg);
bool AppInit2(boost::thread_group& threadGroup);
std::string HelpMessage();

#endif
