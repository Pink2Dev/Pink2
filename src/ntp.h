#ifndef NTP_H
#define NTP_H
#include <string>

extern bool fNTPSuccess;
bool SetNTPOffset(const std::string &strPool);
void *threadNTPUpdate(const std::string &strNTPool);

#endif // NTP_H
