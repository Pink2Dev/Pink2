#include <iostream>
#include <vector>

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <chrono>
#include <thread>

#include "util.h"
#include "ntp.h"


// #include "ntp.h"


// This is here to help us know what bytes to pull for what with our bTimeReq[48]
/*
  uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.

  uint8_t stratum;         // Eight bits. Stratum level of the local clock.
  uint8_t poll;            // Eight bits. Maximum interval between successive messages.
  uint8_t precision;       // Eight bits. Precision of the local clock.

  uint32_t rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
  uint32_t refId;          // 32 bits. Reference clock identifier.

  uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
  uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

  uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
  uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

  uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
  uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

  uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
  uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

  48 bits total, hence bTimeReq[48]
*/

using namespace std;
static uint64_t nNTPUnix = 2208988800ull;

bool GetNTPTime(const char *addrConnect, uint64_t& timeRet)
{

    uint64_t startMicros = 0;
    uint64_t endMicros = 0;
    uint64_t diffMicros = 0;

    // We're doing a lot of this by hand instead of using our classes in netbase.cpp
    // because that hasn't been set up for UDP requests. This can be moved into netbase
    // should we ever need UDP support for any other reason, but for now this is fine.

    // Gotta use BigEndian for networks. NTP uses UDP port 123.
    int udpPort = htons(123);

    // Standard UDP socket.
    int socketNTP = socket( AF_INET , SOCK_DGRAM, IPPROTO_UDP);
    if (socketNTP < 0)
        return false;

    struct sockaddr_in sockAddr;
    struct hostent *hNTPServ;

    // Zero out our socket address.
    memset(&sockAddr, 0, sizeof(sockAddr));

    // Get our host from DNS.
    hNTPServ = gethostbyname(addrConnect);

    if (hNTPServ == nullptr)
        return false;

    // Set our connection information.
    sockAddr.sin_family = AF_INET;
    memcpy(&sockAddr.sin_addr.s_addr, hNTPServ->h_addr_list[0], hNTPServ->h_length);
    sockAddr.sin_port = udpPort;


    // Set our timeout to 2 seconds.
#ifdef WIN32
    DWORD timeout = 2000;
    setsockopt(socketNTP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(socketNTP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif

    // Connect to NTP
    if (connect(socketNTP, (struct sockaddr*) &sockAddr, sizeof(sockAddr)) < 0)
        return false;

    // Our buffer for our request from NTP. See note above for its structure.
    // Note: we're not bothering with the struct. We're just grabbing what we need.
    char bTimeReq[48];

    // Clear it.
    memset(&bTimeReq, 0, 48);

    // Set the first byte so NTP knows what to send us.
    // Leap Indicator = 0
    // Version = 3
    // Mode 3
    bTimeReq[0] = 0x1b;

    // Time we send our request
    startMicros = boost::chrono::duration_cast<boost::chrono::microseconds>(boost::chrono::system_clock::now().time_since_epoch()).count();
    int n = send(socketNTP, (char*)&bTimeReq, 48, 0);

    if (n < 0)
        return false;


    n = recv(socketNTP, (char*)&bTimeReq, 48, 0);
    // Time we got it
    endMicros = boost::chrono::duration_cast<boost::chrono::microseconds>(boost::chrono::system_clock::now().time_since_epoch()).count();

    if (n < 0)
        return false;

    uint32_t tSeconds = 0;
    uint32_t tMicros = 0;

    // Grab the time in seconds &[40] (Note: NTP is from Jan 1, 1900)
    // Grab the fraction of second &[48] (Note: Fraction is N/UINT32(MAX))

    memcpy(&tSeconds, &bTimeReq[40], 4);
    memcpy(&tMicros, &bTimeReq[44], 4);

    // Convert back to LittleEndian (everything Pink runs on atm uses LE)
    tSeconds = ntohl(tSeconds);
    tMicros = ntohl(tMicros);

    // Subtract the UNIX Epoch from NTP Time (Jan 1, 1970 - Jan 1, 1900)
    int64_t nEpoch = tSeconds - nNTPUnix;

    if (nEpoch < 0)
        nEpoch += std::numeric_limits<uint32_t>::max(); // NTP Rolled over int32 limit. Happens in Y2036.

    // There is no reason this should *still* be < 0;
    if (nEpoch < 0)
        return false;

    // Get our NTP time down to the microsecond.
    // We have to convert NTP fractional time to Micros.
    uint64_t ntpMicros = 0;
    ntpMicros = (nEpoch * 1000000);
    ntpMicros += ((double)tMicros / std::numeric_limits<uint32_t>::max()) * 1000000;

    // Split the difference between when we requested the time
    // and when we got it to account for the network round trip.
    diffMicros = (endMicros - startMicros) / 2;

    // Add the difference to our time so we can better estimate
    // the time when we actually received it from NTP.
    timeRet = (ntpMicros + diffMicros);

    return true;

}

static uint64_t ntpTime[4];
void *threadGetNTPTime(int nServer, const string strPool, uint64_t startMicros)
{
    std::string strAddress = strPool;
    if (nServer > 3)
        return nullptr;

    // User set :// or tried to use a port (org:###)
    // or is trying to use something other than ntp.org.
    // Drop whatever they set and just use the default.
    if (strAddress.find(":") != std::string::npos || strAddress.find("pool.ntp.org") == std::string::npos)
        strAddress = "pool.ntp.org";

    // Prepend pool server number.
    strAddress = to_string(nServer) + "." + strAddress;

    uint64_t myTime = 0;
    if (GetNTPTime(strAddress.c_str(), myTime))
    {
        uint64_t nowMicros = boost::chrono::duration_cast<boost::chrono::microseconds>(boost::chrono::system_clock::now().time_since_epoch()).count();

        // Subtract how long it took our thread to get the time.
        myTime -= (nowMicros - startMicros);
    }

    ntpTime[nServer] = myTime;

    return nullptr;
}

bool SetNTPOffset(const string &strPool)
{
    int nWait = 0;
    static int tCount = 0;

    uint64_t nowMicros = 0;
    uint64_t avMicros;
    int64_t nTimeOffset = 0;

    vector<uint64_t> ntpMicros;

    while (tCount < 4)
    {
        // Start with a fresh vector to hold our NTP Times
        ntpMicros.clear();
        avMicros = 0;

        // We didn't get what we needed last time, so we're going to wait longer this time.
        nWait++;

        // If we don't have 4 good times after 4 iterations and waiting
        // 2 seconds for a response on our last try then we give up.
        if (nWait == 5)
            return false;

        for (int i = 0; i < 4; i++)
            ntpTime[i] = 0;

        // Let our threads know when we're starting from so they can give
        // us times that are consistent with what we need.
        nowMicros = boost::chrono::duration_cast<boost::chrono::microseconds>(boost::chrono::system_clock::now().time_since_epoch()).count();

        // Any regional ntp pool can be used. But we prepend the number for NTP's randomized selection.
        // By default we use pool.ntp.org - which generally selects pools that are appropriate for your ip.
        // https://www.ntppool.org/en/use.html
        // https://www.pool.ntp.org/zone/@
        boost::thread nGet1(threadGetNTPTime, 0, strPool, nowMicros);
        boost::thread nGet2(threadGetNTPTime, 1, strPool, nowMicros);
        boost::thread nGet3(threadGetNTPTime, 2, strPool, nowMicros);
        boost::thread nGet4(threadGetNTPTime, 3, strPool, nowMicros);

        // Ideally we want times from NTP servers that can respond to us in < 500ms.
        // We'll wait longer if we don't get what we need.
        boost::this_thread::sleep_for(boost::chrono::milliseconds(500 * nWait));

        // Get this now so we know exactly when we woke up again.
        // Helps us get an accurate offset.
        nowMicros = boost::chrono::duration_cast<boost::chrono::microseconds>(boost::chrono::system_clock::now().time_since_epoch()).count();

        for (int i = 0; i < 4; i++)
        {
            // Make sure we actually got the time. NTP servers that respond but aren't able
            // to give us an accurate time as requested instead send us the uint32 max.
            // We tolerate 10 seconds here to accomodate our internal adjustments.
            if (ntpTime[i] > 0 && abs((int64_t)(ntpTime[i] / 1000000) + (int64_t)nNTPUnix - (int64_t)std::numeric_limits<uint32_t>::max()) > 10)
            {
                // Account for our wait time.
                ntpTime[i] += (500000 * nWait);
                ntpMicros.push_back(ntpTime[i]);
            }
        }

        // Add it up.
        for (vector<uint64_t>::iterator it = ntpMicros.begin(); it != ntpMicros.end(); it++)
             avMicros += *it;

        // Average of what we got.
        avMicros /= ntpMicros.size();

        // Set our offset based on the difference and maintain an average.
        nTimeOffset += (avMicros - nowMicros);
        if (nWait > 1)
            nTimeOffset /= 2;

        // Add these to our successful NTP Request count.
        tCount += ntpMicros.size();

        // Clear up our threads
        nGet1.join();
        nGet2.join();
        nGet3.join();
        nGet4.join();
    }

    tCount = 0;

    // nTimeOffset precision is in seconds, so convert from Micros.
    nTimeOffset /= 1000000;

    SetTimeOffset(nTimeOffset);

    return true;

}

void *threadNTPUpdate(const string strNTPool)
{

    static int nRefreshTime = 0;
    static int nFailCount = 0;

    // If SetNTPOffset fails, we fall back on local/peer time, depending on
    // user settings. What we're using is visible from the rpc command 'getinfo'
    fNTPSuccess = SetNTPOffset(strNTPool);

    // We use steady_clock here so we can be completely agnostic about the system
    // time while we sleep. Protects user against a local clock manipulation attack
    // and allows us to adjust our offset internally if the system time does change.
    std::this_thread::sleep_for(std::chrono::steady_clock::duration(1000000000));

    // Time Tracker tTrack helps us monitor for system clock changes each second.
    uint64_t tTrack = GetAdjustedTime();

    while (!fShutdown)
    {
        std::this_thread::sleep_for(std::chrono::steady_clock::duration(1000000000));

        nRefreshTime++;

        uint64_t tNow = GetAdjustedTime();

        // Our clock changed in the last second.
        if (abs((int64_t)tNow - ((int64_t)tTrack + 1)) > 1)
        {
            // We'll do this internally so we don't have to bug NTP
            int64_t nOffset = GetTimeOffset();
            nOffset += ((int64_t)tTrack + 1) - (int64_t)tNow;
            SetTimeOffset(nOffset);

            // Update tNow directly instead of from GetTimeOffset
            // in case the system time changes since we set it.
            tNow += ((int64_t)tTrack + 1) - (int64_t)tNow;

            // Update our refresh time sooner from NTP though, just in case
            nRefreshTime = (nRefreshTime < 3600) ? nRefreshTime + 400 : 3600;
        }

        // Update our Time Tracker.
        tTrack = tNow;


        // Update against NTP every hour.
        if (nRefreshTime == 3600)
        {
            nRefreshTime = 0;

            // Only set fNTPSuccess to false after 24 hours of
            // not having a successful update from NTP.
            if (!SetNTPOffset(strNTPool))
            {
                nFailCount++;
            } else {
                fNTPSuccess = true;
                nFailCount = 0;
            }

            if (nFailCount > 23)
                fNTPSuccess = false;

            // Start our next loop with a fresh tTrack;
            tTrack = GetAdjustedTime();

        }

    }

    return nullptr;
}

