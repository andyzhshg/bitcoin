// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"


map<string, string> mapArgs;
map<string, vector<string> > mapMultiArgs;
bool fDebug = false;
bool fPrintToDebugger = false;
bool fPrintToConsole = false;
char pszSetDataDir[MAX_PATH] = "";





// Init openssl library multithreading support
static wxMutex** ppmutexOpenSSL;
void locking_callback(int mode, int i, const char* file, int line)
{
    if (mode & CRYPTO_LOCK)
        ppmutexOpenSSL[i]->Lock();
    else
        ppmutexOpenSSL[i]->Unlock();
}

/*
    @up4dev
    CInit的定义最后紧跟着声明了一个全局变量instance_of_cinit，也就使得程序启动时必然会执行一次构造函数的代码，
    在构造函数中做了几个初始化的操作：
    - 初始化openssl的多线程锁
    - 初始化随机数发生器
*/
// Init
class CInit
{
public:
    CInit()
    {
        /*
            @up4dev
            根据openssl的多线程锁的机制做初始化
            (参考)[http://popozhu.github.io/2013/08/15/openssl%E5%92%8C%E5%A4%9A%E7%BA%BF%E7%A8%8B/]
        */
        // Init openssl library multithreading support
        ppmutexOpenSSL = (wxMutex**)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(wxMutex*));
        for (int i = 0; i < CRYPTO_num_locks(); i++)
            ppmutexOpenSSL[i] = new wxMutex();
        CRYPTO_set_locking_callback(locking_callback);

#ifdef __WXMSW__
        // Seed random number generator with screen scrape and other hardware sources
        RAND_screen();
#endif

        // Seed random number generator with performance counter
        RandAddSeed();
    }
    ~CInit()
    {
        // Shutdown openssl library multithreading support
        CRYPTO_set_locking_callback(NULL);
        for (int i = 0; i < CRYPTO_num_locks(); i++)
            delete ppmutexOpenSSL[i];
        OPENSSL_free(ppmutexOpenSSL);

        // Close sockets
        foreach(CNode* pnode, vNodes)
            closesocket(pnode->hSocket);
        if (closesocket(hListenSocket) == SOCKET_ERROR)
            printf("closesocket(hListenSocket) failed with error %d\n", WSAGetLastError());

#ifdef __WXMSW__
        // Shutdown Windows Sockets
        WSACleanup();
#endif
    }
}
instance_of_cinit;








void RandAddSeed()
{
    // Seed with CPU performance counter
    int64 nCounter = PerformanceCounter();
    RAND_add(&nCounter, sizeof(nCounter), 1.5);
    memset(&nCounter, 0, sizeof(nCounter));
}

void RandAddSeedPerfmon()
{
#ifdef __WXMSW__
    // Don't need this on Linux, OpenSSL automatically uses /dev/urandom
    // This can take up to 2 seconds, so only do it every 10 minutes
    static int64 nLastPerfmon;
    if (GetTime() < nLastPerfmon + 10 * 60)
        return;
    nLastPerfmon = GetTime();

    // Seed with the entire set of perfmon data
    unsigned char pdata[250000];
    memset(pdata, 0, sizeof(pdata));
    unsigned long nSize = sizeof(pdata);
    long ret = RegQueryValueEx(HKEY_PERFORMANCE_DATA, "Global", NULL, NULL, pdata, &nSize);
    RegCloseKey(HKEY_PERFORMANCE_DATA);
    if (ret == ERROR_SUCCESS)
    {
        uint256 hash;
        SHA256(pdata, nSize, (unsigned char*)&hash);
        RAND_add(&hash, sizeof(hash), min(nSize/500.0, (double)sizeof(hash)));
        hash = 0;
        memset(pdata, 0, nSize);

        printf("%s RandAddSeed() %d bytes\n", DateTimeStrFormat("%x %H:%M:%S", GetTime()).c_str(), nSize);
    }
#endif
}









// Safer snprintf
//  - prints up to limit-1 characters
//  - output string is always null terminated even if limit reached
//  - return value is the number of characters actually printed
int my_snprintf(char* buffer, size_t limit, const char* format, ...)
{
    if (limit == 0)
        return 0;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    int ret = _vsnprintf(buffer, limit, format, arg_ptr);
    va_end(arg_ptr);
    if (ret < 0 || ret >= limit)
    {
        ret = limit - 1;
        buffer[limit-1] = 0;
    }
    return ret;
}


string strprintf(const char* format, ...)
{
    char buffer[50000];
    char* p = buffer;
    int limit = sizeof(buffer);
    int ret;
    loop
    {
        va_list arg_ptr;
        va_start(arg_ptr, format);
        ret = _vsnprintf(p, limit, format, arg_ptr);
        va_end(arg_ptr);
        if (ret >= 0 && ret < limit)
            break;
        if (p != buffer)
            delete p;
        limit *= 2;
        p = new char[limit];
        if (p == NULL)
            throw std::bad_alloc();
    }
#ifdef _MSC_VER
    // msvc optimisation
    if (p == buffer)
        return string(p, p+ret);
#endif
    string str(p, p+ret);
    if (p != buffer)
        delete p;
    return str;
}


bool error(const char* format, ...)
{
    char buffer[50000];
    int limit = sizeof(buffer);
    va_list arg_ptr;
    va_start(arg_ptr, format);
    int ret = _vsnprintf(buffer, limit, format, arg_ptr);
    va_end(arg_ptr);
    if (ret < 0 || ret >= limit)
    {
        ret = limit - 1;
        buffer[limit-1] = 0;
    }
    printf("ERROR: %s\n", buffer);
    return false;
}


void ParseString(const string& str, char c, vector<string>& v)
{
    unsigned int i1 = 0;
    unsigned int i2;
    do
    {
        i2 = str.find(c, i1);
        v.push_back(str.substr(i1, i2-i1));
        i1 = i2+1;
    }
    while (i2 != str.npos);
}

/*
    @up4dev
    将钱的数字格式化成更易于阅读的形式
*/
string FormatMoney(int64 n, bool fPlus)
{
    n /= CENT;  //@up4dev 归一化到分
    /*
        @up4dev 
        保留两位小数(估计中本聪自己都没有想到比特币的汇率会涨到1000没劲，
        即使显示到'分'的最小零头也是价值10美金，放在今天，肯定不会只保留两位小数了)
    */
    string str = strprintf("%"PRI64d".%02"PRI64d, (n > 0 ? n : -n)/100, (n > 0 ? n : -n)%100);
    /*
        @up4dev
        从小数点前三位起，每隔三位数插入一个逗号
    */
    for (int i = 6; i < str.size(); i += 4)
        if (isdigit(str[str.size() - i - 1]))
            str.insert(str.size() - i, 1, ',');
    /*
        @up4dev
        插入正负符号
    */
    if (n < 0)
        str.insert((unsigned int)0, 1, '-');
    else if (fPlus && n > 0)
        str.insert((unsigned int)0, 1, '+');
    return str;
}

/*
    @up4dev
    上面函数的逆过程，将文字形式的钱数值转乘整数形式
*/
bool ParseMoney(const char* pszIn, int64& nRet)
{
    string strWhole;
    int64 nCents = 0;
    const char* p = pszIn;
    while (isspace(*p))
        p++;
    for (; *p; p++)
    {
        if (*p == ',' && p > pszIn && isdigit(p[-1]) && isdigit(p[1]) && isdigit(p[2]) && isdigit(p[3]) && !isdigit(p[4]))
            continue;
        if (*p == '.')
        {
            p++;
            if (isdigit(*p))
            {
                nCents = 10 * (*p++ - '0');
                if (isdigit(*p))
                    nCents += (*p++ - '0');
            }
            break;
        }
        if (isspace(*p))
            break;
        if (!isdigit(*p))
            return false;
        strWhole.insert(strWhole.end(), *p);
    }
    for (; *p; p++)
        if (!isspace(*p))
            return false;
    if (strWhole.size() > 14)
        return false;
    if (nCents < 0 || nCents > 99)
        return false;
    int64 nWhole = atoi64(strWhole);
    int64 nPreValue = nWhole * 100 + nCents;
    int64 nValue = nPreValue * CENT;
    if (nValue / CENT != nPreValue)
        return false;
    if (nValue / COIN != nWhole)
        return false;
    nRet = nValue;
    return true;
}

/*
    @up4dev
    16进制数转化成10进制
    貌似并没有地方用到这个函数
*/
vector<unsigned char> ParseHex(const char* psz)
{
    vector<unsigned char> vch;
    while (isspace(*psz))
        psz++;
    vch.reserve((strlen(psz)+1)/3);

    static char phexdigit[256] =
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
      -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

    while (*psz)
    {
        char c = phexdigit[(unsigned char)*psz++];
        if (c == -1)
            break;
        unsigned char n = (c << 4);
        if (*psz)
        {
            char c = phexdigit[(unsigned char)*psz++];
            if (c == -1)
                break;
            n |= c;
            vch.push_back(n);
        }
        while (isspace(*psz))
            psz++;
    }

    return vch;
}

vector<unsigned char> ParseHex(const std::string& str)
{
    return ParseHex(str.c_str());
}

/*
    @up4dev
    解析命令行参数
    解析的结果存入全局变量mapArgs和mapMultiArgs中
*/
void ParseParameters(int argc, char* argv[])
{
    mapArgs.clear();
    mapMultiArgs.clear();
    for (int i = 0; i < argc; i++)
    {
        char psz[10000];
        strlcpy(psz, argv[i], sizeof(psz));
        char* pszValue = "";
        if (strchr(psz, '='))
        {
            pszValue = strchr(psz, '=');
            *pszValue++ = '\0';
        }
        #ifdef __WXMSW__
        _strlwr(psz);
        if (psz[0] == '/')
            psz[0] = '-';
        #endif
        mapArgs[psz] = pszValue;
        mapMultiArgs[psz].push_back(pszValue);
    }
}







void FormatException(char* pszMessage, std::exception* pex, const char* pszThread)
{
#ifdef __WXMSW__
    char pszModule[MAX_PATH];
    pszModule[0] = '\0';
    GetModuleFileName(NULL, pszModule, sizeof(pszModule));
#else
    // might not be thread safe, uses wxString
    //const char* pszModule = wxStandardPaths::Get().GetExecutablePath().mb_str();
    const char* pszModule = "bitcoin";
#endif
    if (pex)
        snprintf(pszMessage, 1000,
            "EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, pszThread);
    else
        snprintf(pszMessage, 1000,
            "UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
}

void LogException(std::exception* pex, const char* pszThread)
{
    char pszMessage[1000];
    FormatException(pszMessage, pex, pszThread);
    printf("\n%s", pszMessage);
}

void PrintException(std::exception* pex, const char* pszThread)
{
    char pszMessage[1000];
    FormatException(pszMessage, pex, pszThread);
    printf("\n\n************************\n%s\n", pszMessage);
    if (wxTheApp)
        wxMessageBox(pszMessage, "Error", wxOK | wxICON_ERROR);
    throw;
    //DebugBreak();
}






/*
    @up4dev
    获取文件的大小
*/
int GetFilesize(FILE* file)
{
    int nSavePos = ftell(file);
    int nFilesize = -1;
    if (fseek(file, 0, SEEK_END) == 0)
        nFilesize = ftell(file);
    fseek(file, nSavePos, SEEK_SET);
    return nFilesize;
}

/*
    @up4dev
    取得数据文件目录
    如果配置了该参数，取得配置的位置；如果没有配置，则在用户的home目录下
*/
void GetDataDir(char* pszDir)
{
    // pszDir must be at least MAX_PATH length.
    if (pszSetDataDir[0] != 0)
    {
        strlcpy(pszDir, pszSetDataDir, MAX_PATH);
        static bool fMkdirDone;
        if (!fMkdirDone)
        {
            fMkdirDone = true;
            _mkdir(pszDir);
        }
    }
    else
    {
        // This can be called during exceptions by printf, so we cache the
        // value so we don't have to do memory allocations after that.
        // wxStandardPaths::GetUserDataDir
        //  Return the directory for the user-dependent application data files:
        //  Unix: ~/.appname
        //  Windows: C:\Documents and Settings\username\Application Data\appname
        //  Mac: ~/Library/Application Support/appname
        static char pszCachedDir[MAX_PATH];
        if (pszCachedDir[0] == 0)
        {
            strlcpy(pszCachedDir, wxStandardPaths::Get().GetUserDataDir().c_str(), sizeof(pszCachedDir));
            _mkdir(pszCachedDir);
        }
        strlcpy(pszDir, pszCachedDir, MAX_PATH);
    }
}

string GetDataDir()
{
    char pszDir[MAX_PATH];
    GetDataDir(pszDir);
    return pszDir;
}







/*
    @up4dev
    取得大小在 0~nMac 之间的随机数
    实际是调用openssl的函数RAND_bytes来生成的
*/
uint64 GetRand(uint64 nMax)
{
    if (nMax == 0)
        return 0;

    // The range of the random source must be a multiple of the modulus
    // to give every possible output value an equal possibility
    uint64 nRange = (_UI64_MAX / nMax) * nMax;
    uint64 nRand = 0;
    do
        RAND_bytes((unsigned char*)&nRand, sizeof(nRand));
    while (nRand >= nRange);
    return (nRand % nMax);
}










//
// "Never go to sea with two chronometers; take one or three."
// Our three chronometers are:
//  - System clock
//  - Median of other server's clocks
//  - NTP servers
//
// note: NTP isn't implemented yet, so until then we just use the median
//  of other nodes clocks to correct ours.
//

int64 GetTime()
{
    return time(NULL);
}

static int64 nTimeOffset = 0;

int64 GetAdjustedTime()
{
    return GetTime() + nTimeOffset;
}

/*
    @up4dev
    根据其他服务器传来的时间校准自身的时钟
    在这个版本还没有实现NTP，所以只有这一个比较粗略的方式
*/
void AddTimeData(unsigned int ip, int64 nTime)
{
    int64 nOffsetSample = nTime - GetTime();

    // Ignore duplicates
    static set<unsigned int> setKnown;
    if (!setKnown.insert(ip).second)
        return;

    // Add data
    static vector<int64> vTimeOffsets;
    if (vTimeOffsets.empty())
        vTimeOffsets.push_back(0);
    vTimeOffsets.push_back(nOffsetSample);
    printf("Added time data, samples %d, offset %+"PRI64d" (%+"PRI64d" minutes)\n", vTimeOffsets.size(), vTimeOffsets.back(), vTimeOffsets.back()/60);
    if (vTimeOffsets.size() >= 5 && vTimeOffsets.size() % 2 == 1)
    {
        sort(vTimeOffsets.begin(), vTimeOffsets.end());
        int64 nMedian = vTimeOffsets[vTimeOffsets.size()/2];
        nTimeOffset = nMedian;
        if ((nMedian > 0 ? nMedian : -nMedian) > 5 * 60)
        {
            // Only let other nodes change our clock so far before we
            // go to the NTP servers
            /// todo: Get time from NTP servers, then set a flag
            ///    to make sure it doesn't get changed again
        }
        foreach(int64 n, vTimeOffsets)
            printf("%+"PRI64d"  ", n);
        printf("|  nTimeOffset = %+"PRI64d"  (%+"PRI64d" minutes)\n", nTimeOffset, nTimeOffset/60);
    }
}
