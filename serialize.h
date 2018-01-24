// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <map>
#include <boost/type_traits/is_fundamental.hpp>
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64  int64;
typedef unsigned __int64  uint64;
#else
typedef long long  int64;
typedef unsigned long long  uint64;
#endif
#if defined(_MSC_VER) && _MSC_VER < 1300
#define for  if (false) ; else for
#endif
class CScript;
class CDataStream;
class CAutoFile;

static const int VERSION = 106;





/////////////////////////////////////////////////////////////////
//
// Templates for serializing to anything that looks like a stream,
// i.e. anything that supports .read(char*, int) and .write(char*, int)
//

enum
{
    // primary actions
    SER_NETWORK         = (1 << 0),
    SER_DISK            = (1 << 1),
    SER_GETHASH         = (1 << 2),

    // modifiers
    SER_SKIPSIG         = (1 << 16),
    SER_BLOCKHEADERONLY = (1 << 17),
};

/*
    @up4dev
    这段宏定义会展开成3个函数
    GetSerializeSize    获取序列化后的大小
    Serialize           序列化
    Unserialize         反序列化
*/

#define IMPLEMENT_SERIALIZE(statements)    \
    unsigned int GetSerializeSize(int nType=0, int nVersion=VERSION) const  \
    {                                           \
        CSerActionGetSerializeSize ser_action;  \
        const bool fGetSize = true;             \
        const bool fWrite = false;              \
        const bool fRead = false;               \
        unsigned int nSerSize = 0;              \
        ser_streamplaceholder s;                \
        s.nType = nType;                        \
        s.nVersion = nVersion;                  \
        {statements}                            \
        return nSerSize;                        \
    }                                           \
    template<typename Stream>                   \
    void Serialize(Stream& s, int nType=0, int nVersion=VERSION) const  \
    {                                           \
        CSerActionSerialize ser_action;         \
        const bool fGetSize = false;            \
        const bool fWrite = true;               \
        const bool fRead = false;               \
        unsigned int nSerSize = 0;              \
        {statements}                            \
    }                                           \
    template<typename Stream>                   \
    void Unserialize(Stream& s, int nType=0, int nVersion=VERSION)  \
    {                                           \
        CSerActionUnserialize ser_action;       \
        const bool fGetSize = false;            \
        const bool fWrite = false;              \
        const bool fRead = true;                \
        unsigned int nSerSize = 0;              \
        {statements}                            \
    }

/*
    @up4dev
    READWRITE宏中最有意思的是参数ser_action，它用于标识这个用用来做什么操作(计算大小/序列化/反序列化)，
    可以看到下文中根据ser_action类型的不同有三个不同的SerReadWrite函数的重载实现，
    而这个宏的调用是包含在上边的IMPLEMENT_SERIALIZE的使用场景中的，而IMPLEMENT_SERIALIZE宏的三个不同的函数
    在一开始就分别定义了三个不同类型的ser_action，这就使得在不同的场景下可以调用不同的SerReadWrite函数。
*/
#define READWRITE(obj)      (nSerSize += ::SerReadWrite(s, (obj), nType, nVersion, ser_action))





/*
    @up4dev
    基础类型的(计算大小/序列化/反序列化函数)
*/

//
// Basic types
//
#define WRITEDATA(s, obj)   s.write((char*)&(obj), sizeof(obj))
#define READDATA(s, obj)    s.read((char*)&(obj), sizeof(obj))

inline unsigned int GetSerializeSize(char a,           int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(signed char a,    int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(unsigned char a,  int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(signed short a,   int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(unsigned short a, int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(signed int a,     int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(unsigned int a,   int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(signed long a,    int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(unsigned long a,  int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(int64 a,          int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(uint64 a,         int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(float a,          int, int=0) { return sizeof(a); }
inline unsigned int GetSerializeSize(double a,         int, int=0) { return sizeof(a); }

template<typename Stream> inline void Serialize(Stream& s, char a,           int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, signed char a,    int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, unsigned char a,  int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, signed short a,   int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, unsigned short a, int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, signed int a,     int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, unsigned int a,   int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, signed long a,    int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, unsigned long a,  int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int64 a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint64 a,         int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, float a,          int, int=0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, double a,         int, int=0) { WRITEDATA(s, a); }

template<typename Stream> inline void Unserialize(Stream& s, char& a,           int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, signed char& a,    int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, unsigned char& a,  int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, signed short& a,   int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, unsigned short& a, int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, signed int& a,     int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, unsigned int& a,   int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, signed long& a,    int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, unsigned long& a,  int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, int64& a,          int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, uint64& a,         int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, float& a,          int, int=0) { READDATA(s, a); }
template<typename Stream> inline void Unserialize(Stream& s, double& a,         int, int=0) { READDATA(s, a); }

inline unsigned int GetSerializeSize(bool a, int, int=0)                          { return sizeof(char); }
template<typename Stream> inline void Serialize(Stream& s, bool a, int, int=0)    { char f=a; WRITEDATA(s, f); }
template<typename Stream> inline void Unserialize(Stream& s, bool& a, int, int=0) { char f; READDATA(s, f); a=f; }



/*
    @up4dev
    对string/vector/map/set类型的对象做序列化时，需要先记录元素的个数(string/vector的长度，map键值对的个数，set元素的个数)，
    GetSizeOfCompactSize/WriteCompactSize/ReadCompactSize 这三个函数分别用来计算/写入/读取这个 元素个数 的数值。
    之所以存在这组函数，是为了能够尽可能少的占用磁盘空间。
    考虑到不同的情况，要记录的大小为255字节以下时，需要1个字节来记录这个数值；要记录64k(65535)字节以下时，需要2个字节来记录这个数字；
    要记录4G(0xffffffff)字节以下时，需要4个字节来记录这个数字；要记录4G(0xffffffff)字节以上时，需要8个字节来记录这个数字；
    实际情况是，这个数字大部分时间是小于255的，少数时候会大于255小于64k，而大于64k就非常少见了，大于4G就基本上没有了。
    为了能够记录可能出现的最大数据块，假设是64k以上4G以下，就至少就需要4字节，然而这4字节在大部分时候却只会用到1个2个字节，
    当要记录的数据个数很多的时候，这无疑造成了极大的浪费，所以就有了下边描述的压缩形式的数据大小的记录方式。
    1. 对于小于UCHAR_MAX-2(253)的数，直接记录这个数值，耗费1字节
    2. 对于大于UCHAR_MAX-2(253)小于等于USHRT_MAX(65535)的数，
        用首个字节记录UCHAR_MAX-2(253)表示类型(类型即UCHAR_MAX-2<=数字<=USHRT_MAX)，
        用接下来的2个字节记录实际的数值，总体耗费3字节
    3. 对于大于USHRT_MAX(65535)小于等于UINT_MAX(0xffffffff)的数，
        用首个字节记录UCHAR_MAX-1(254)表示类型(USHRT_MAX<=数字<=UINT_MAX)，
        用接下来的4个字节记录实际的数值，总体耗费5字节
    4. 对于大于UINT_MAX(0xffffffff)的数字，
        用首个字节记录UCHAR_MAX(255)表示类型(数字>UINT_MAX)，
        用接下来的8个字节记录实际的数值，总体耗费9字节
    读取的时候，先读取第一个字节，如果小于253，那么它本身代表的就是数据的大小，如果大于等于253则它代表的是一个类型，
    根据不同的情况再从他后边读取2/4/8字节作为真实的数据大小。

    如果进一步细化，其实可以对大小数值为0xff/0xffff/0xffffff/0xffffffff/0xffffffffff/0xffffffffffff/0xffffffffffffff/0xffffffffffffffff
    这些区间的数值分别进行分类处理，占用的字节数分别是2/3/4/5/6/7/8/9，但是对于比特币的应用来说大于0xffff的已经基本不存在了，则还不如简化为2/3/5/9四个类别，
    以减少存储类别对首字节的浪费。
*/


//
// Compact size
//  size <  253        -- 1 byte
//  size <= USHRT_MAX  -- 3 bytes  (253 + 2 bytes)
//  size <= UINT_MAX   -- 5 bytes  (254 + 4 bytes)
//  size >  UINT_MAX   -- 9 bytes  (255 + 8 bytes)
//
inline unsigned int GetSizeOfCompactSize(uint64 nSize)
{
    if (nSize < UCHAR_MAX-2)     return sizeof(unsigned char);
    else if (nSize <= USHRT_MAX) return sizeof(unsigned char) + sizeof(unsigned short);
    else if (nSize <= UINT_MAX)  return sizeof(unsigned char) + sizeof(unsigned int);
    else                         return sizeof(unsigned char) + sizeof(uint64);
}

template<typename Stream>
void WriteCompactSize(Stream& os, uint64 nSize)
{
    if (nSize < UCHAR_MAX-2)
    {
        unsigned char chSize = nSize;
        WRITEDATA(os, chSize);
    }
    else if (nSize <= USHRT_MAX)
    {
        unsigned char chSize = UCHAR_MAX-2;
        unsigned short xSize = nSize;
        WRITEDATA(os, chSize);
        WRITEDATA(os, xSize);
    }
    else if (nSize <= UINT_MAX)
    {
        unsigned char chSize = UCHAR_MAX-1;
        unsigned int xSize = nSize;
        WRITEDATA(os, chSize);
        WRITEDATA(os, xSize);
    }
    else
    {
        unsigned char chSize = UCHAR_MAX;
        WRITEDATA(os, chSize);
        WRITEDATA(os, nSize);
    }
    return;
}

template<typename Stream>
uint64 ReadCompactSize(Stream& is)
{
    unsigned char chSize;
    READDATA(is, chSize);
    if (chSize < UCHAR_MAX-2)
    {
        return chSize;
    }
    else if (chSize == UCHAR_MAX-2)
    {
        unsigned short nSize;
        READDATA(is, nSize);
        return nSize;
    }
    else if (chSize == UCHAR_MAX-1)
    {
        unsigned int nSize;
        READDATA(is, nSize);
        return nSize;
    }
    else
    {
        uint64 nSize;
        READDATA(is, nSize);
        return nSize;
    }
}


/*
    @up4dev
    数组和基础数据类型(POD Plain Old Data)的序列化封装类和宏
    (参考)[https://zh.wikipedia.org/wiki/POD_(%E7%A8%8B%E5%BA%8F%E8%AE%BE%E8%AE%A1)]
*/
//
// Wrapper for serializing arrays and POD
// There's a clever template way to make arrays serialize normally, but MSVC6 doesn't support it
//
#define FLATDATA(obj)   REF(CFlatData((char*)&(obj), (char*)&(obj) + sizeof(obj)))
class CFlatData
{
protected:
    char* pbegin;
    char* pend;
public:
    CFlatData(void* pbeginIn, void* pendIn) : pbegin((char*)pbeginIn), pend((char*)pendIn) { }
    char* begin() { return pbegin; }
    const char* begin() const { return pbegin; }
    char* end() { return pend; }
    const char* end() const { return pend; }

    unsigned int GetSerializeSize(int, int=0) const
    {
        return pend - pbegin;
    }

    template<typename Stream>
    void Serialize(Stream& s, int, int=0) const
    {
        s.write(pbegin, pend - pbegin);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int, int=0)
    {
        s.read(pbegin, pend - pbegin);
    }
};


/*
    @up4dev
    定长字符串的序列化操作封装
*/
//
// string stored as a fixed length field
//
template<std::size_t LEN>
class CFixedFieldString
{
protected:
    const string* pcstr;
    string* pstr;
public:
    explicit CFixedFieldString(const string& str) : pcstr(&str), pstr(NULL) { }
    explicit CFixedFieldString(string& str) : pcstr(&str), pstr(&str) { }

    unsigned int GetSerializeSize(int, int=0) const
    {
        return LEN;
    }

    template<typename Stream>
    void Serialize(Stream& s, int, int=0) const
    {
        char pszBuf[LEN];
        strncpy(pszBuf, pcstr->c_str(), LEN);
        s.write(pszBuf, LEN);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int, int=0)
    {
        if (pstr == NULL)
            throw std::ios_base::failure("CFixedFieldString::Unserialize : trying to unserialize to const string");
        char pszBuf[LEN+1];
        s.read(pszBuf, LEN);
        pszBuf[LEN] = '\0';
        *pstr = pszBuf;
    }
};





//
// Forward declarations
//

/*
    @up4dev
    string/vector/pair/map/set的序列化操作封装
*/

// string
template<typename C> unsigned int GetSerializeSize(const basic_string<C>& str, int, int=0);
template<typename Stream, typename C> void Serialize(Stream& os, const basic_string<C>& str, int, int=0);
template<typename Stream, typename C> void Unserialize(Stream& is, basic_string<C>& str, int, int=0);

// vector
template<typename T, typename A> unsigned int GetSerializeSize_impl(const std::vector<T, A>& v, int nType, int nVersion, const boost::true_type&);
template<typename T, typename A> unsigned int GetSerializeSize_impl(const std::vector<T, A>& v, int nType, int nVersion, const boost::false_type&);
template<typename T, typename A> inline unsigned int GetSerializeSize(const std::vector<T, A>& v, int nType, int nVersion=VERSION);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nType, int nVersion, const boost::true_type&);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nType, int nVersion, const boost::false_type&);
template<typename Stream, typename T, typename A> inline void Serialize(Stream& os, const std::vector<T, A>& v, int nType, int nVersion=VERSION);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nType, int nVersion, const boost::true_type&);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nType, int nVersion, const boost::false_type&);
template<typename Stream, typename T, typename A> inline void Unserialize(Stream& is, std::vector<T, A>& v, int nType, int nVersion=VERSION);

// others derived from vector
extern inline unsigned int GetSerializeSize(const CScript& v, int nType, int nVersion=VERSION);
template<typename Stream> void Serialize(Stream& os, const CScript& v, int nType, int nVersion=VERSION);
template<typename Stream> void Unserialize(Stream& is, CScript& v, int nType, int nVersion=VERSION);

// pair
template<typename K, typename T> unsigned int GetSerializeSize(const std::pair<K, T>& item, int nType, int nVersion=VERSION);
template<typename Stream, typename K, typename T> void Serialize(Stream& os, const std::pair<K, T>& item, int nType, int nVersion=VERSION);
template<typename Stream, typename K, typename T> void Unserialize(Stream& is, std::pair<K, T>& item, int nType, int nVersion=VERSION);

// map
template<typename K, typename T, typename Pred, typename A> unsigned int GetSerializeSize(const std::map<K, T, Pred, A>& m, int nType, int nVersion=VERSION);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, int nType, int nVersion=VERSION);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, int nType, int nVersion=VERSION);

// set
template<typename K, typename Pred, typename A> unsigned int GetSerializeSize(const std::set<K, Pred, A>& m, int nType, int nVersion=VERSION);
template<typename Stream, typename K, typename Pred, typename A> void Serialize(Stream& os, const std::set<K, Pred, A>& m, int nType, int nVersion=VERSION);
template<typename Stream, typename K, typename Pred, typename A> void Unserialize(Stream& is, std::set<K, Pred, A>& m, int nType, int nVersion=VERSION);




/*
    @up4dev
    如果上面定义版本的GetSerializeSize/Serialize/Unserialize均无法本匹配，
    下边这组泛型函数将会被匹配，前提是T实现了GetSerializeSize/Serialize/Unserialize成员函数，
    调用IMPLEMENT_SERIALIZE的类其实是通过这个宏实现了这三个函数
*/
//
// If none of the specialized versions above matched, default to calling member function.
// "int nType" is changed to "long nType" to keep from getting an ambiguous overload error.
// The compiler will only cast int to long if none of the other templates matched.
// Thanks to Boost serialization for this idea.
//
template<typename T>
inline unsigned int GetSerializeSize(const T& a, long nType, int nVersion=VERSION)
{
    return a.GetSerializeSize((int)nType, nVersion);
}

template<typename Stream, typename T>
inline void Serialize(Stream& os, const T& a, long nType, int nVersion=VERSION)
{
    a.Serialize(os, (int)nType, nVersion);
}

template<typename Stream, typename T>
inline void Unserialize(Stream& is, T& a, long nType, int nVersion=VERSION)
{
    a.Unserialize(is, (int)nType, nVersion);
}





//
// string
//
template<typename C>
unsigned int GetSerializeSize(const basic_string<C>& str, int, int)
{
    return GetSizeOfCompactSize(str.size()) + str.size() * sizeof(str[0]);
}

template<typename Stream, typename C>
void Serialize(Stream& os, const basic_string<C>& str, int, int)
{
    WriteCompactSize(os, str.size());
    if (!str.empty())
        os.write((char*)&str[0], str.size() * sizeof(str[0]));
}

template<typename Stream, typename C>
void Unserialize(Stream& is, basic_string<C>& str, int, int)
{
    unsigned int nSize = ReadCompactSize(is);
    str.resize(nSize);
    if (nSize != 0)
        is.read((char*)&str[0], nSize * sizeof(str[0]));
}



//
// vector
//
/*
    @up4dev
    这里有一个小技巧，利用boost::is_fundamental来区分vector中的元素是基础类型(字符/整数/浮点数等)还是对象，
    如果是基础类型，则调用boost::true_type的版本，否则调用boost::false_type的版本。
    以下的序列化反序列化函数都是按照这个套路来实现的，不再进一步说明
*/
template<typename T, typename A>
unsigned int GetSerializeSize_impl(const std::vector<T, A>& v, int nType, int nVersion, const boost::true_type&)
{
    return (GetSizeOfCompactSize(v.size()) + v.size() * sizeof(T));
}

template<typename T, typename A>
unsigned int GetSerializeSize_impl(const std::vector<T, A>& v, int nType, int nVersion, const boost::false_type&)
{
    unsigned int nSize = GetSizeOfCompactSize(v.size());
    for (typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
        nSize += GetSerializeSize((*vi), nType, nVersion);
    return nSize;
}

template<typename T, typename A>
inline unsigned int GetSerializeSize(const std::vector<T, A>& v, int nType, int nVersion)
{
    return GetSerializeSize_impl(v, nType, nVersion, boost::is_fundamental<T>());
}


template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nType, int nVersion, const boost::true_type&)
{
    WriteCompactSize(os, v.size());
    if (!v.empty())
        os.write((char*)&v[0], v.size() * sizeof(T));
}

template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nType, int nVersion, const boost::false_type&)
{
    WriteCompactSize(os, v.size());
    for (typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
        ::Serialize(os, (*vi), nType, nVersion);
}

template<typename Stream, typename T, typename A>
inline void Serialize(Stream& os, const std::vector<T, A>& v, int nType, int nVersion)
{
    Serialize_impl(os, v, nType, nVersion, boost::is_fundamental<T>());
}


template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nType, int nVersion, const boost::true_type&)
{
    //unsigned int nSize = ReadCompactSize(is);
    //v.resize(nSize);
    //is.read((char*)&v[0], nSize * sizeof(T));

    /*
        @up4dev
        这里需要关注v.resize，
        这里没有一次性的读取所有数组元素(这样就不用调用v.resize了，也就是上边被注释掉的3行代码)，
        而是blk个元素一次，并且显示的调用v.resize，是因为大部分编译器的vector实现中内存分配
        策略是以加倍的形式分配的，这有可能会造成内存的不必要浪费。
    */
    // Limit size per read so bogus size value won't cause out of memory
    v.clear();
    unsigned int nSize = ReadCompactSize(is);
    unsigned int i = 0;
    while (i < nSize)
    {
        unsigned int blk = min(nSize - i, 1 + 4999999 / sizeof(T));
        v.resize(i + blk);
        is.read((char*)&v[i], blk * sizeof(T));
        i += blk;
    }
}

template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nType, int nVersion, const boost::false_type&)
{
    //unsigned int nSize = ReadCompactSize(is);
    //v.resize(nSize);
    //for (std::vector<T, A>::iterator vi = v.begin(); vi != v.end(); ++vi)
    //    Unserialize(is, (*vi), nType, nVersion);

    v.clear();
    unsigned int nSize = ReadCompactSize(is);
    unsigned int i = 0;
    unsigned int nMid = 0;
    while (nMid < nSize)
    {
        nMid += 5000000 / sizeof(T);
        if (nMid > nSize)
            nMid = nSize;
        v.resize(nMid);
        for (; i < nMid; i++)
            Unserialize(is, v[i], nType, nVersion);
    }
}

template<typename Stream, typename T, typename A>
inline void Unserialize(Stream& is, std::vector<T, A>& v, int nType, int nVersion)
{
    Unserialize_impl(is, v, nType, nVersion, boost::is_fundamental<T>());
}



//
// others derived from vector
//
inline unsigned int GetSerializeSize(const CScript& v, int nType, int nVersion)
{
    return GetSerializeSize((const vector<unsigned char>&)v, nType, nVersion);
}

template<typename Stream>
void Serialize(Stream& os, const CScript& v, int nType, int nVersion)
{
    Serialize(os, (const vector<unsigned char>&)v, nType, nVersion);
}

template<typename Stream>
void Unserialize(Stream& is, CScript& v, int nType, int nVersion)
{
    Unserialize(is, (vector<unsigned char>&)v, nType, nVersion);
}



//
// pair
//
template<typename K, typename T>
unsigned int GetSerializeSize(const std::pair<K, T>& item, int nType, int nVersion)
{
    return GetSerializeSize(item.first, nType, nVersion) + GetSerializeSize(item.second, nType, nVersion);
}

template<typename Stream, typename K, typename T>
void Serialize(Stream& os, const std::pair<K, T>& item, int nType, int nVersion)
{
    Serialize(os, item.first, nType, nVersion);
    Serialize(os, item.second, nType, nVersion);
}

template<typename Stream, typename K, typename T>
void Unserialize(Stream& is, std::pair<K, T>& item, int nType, int nVersion)
{
    Unserialize(is, item.first, nType, nVersion);
    Unserialize(is, item.second, nType, nVersion);
}



//
// map
//
template<typename K, typename T, typename Pred, typename A>
unsigned int GetSerializeSize(const std::map<K, T, Pred, A>& m, int nType, int nVersion)
{
    unsigned int nSize = GetSizeOfCompactSize(m.size());
    for (typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        nSize += GetSerializeSize((*mi), nType, nVersion);
    return nSize;
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, int nType, int nVersion)
{
    WriteCompactSize(os, m.size());
    for (typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        Serialize(os, (*mi), nType, nVersion);
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, int nType, int nVersion)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    typename std::map<K, T, Pred, A>::iterator mi = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        pair<K, T> item;
        Unserialize(is, item, nType, nVersion);
        mi = m.insert(mi, item);
    }
}



//
// set
//
template<typename K, typename Pred, typename A>
unsigned int GetSerializeSize(const std::set<K, Pred, A>& m, int nType, int nVersion)
{
    unsigned int nSize = GetSizeOfCompactSize(m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        nSize += GetSerializeSize((*it), nType, nVersion);
    return nSize;
}

template<typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream& os, const std::set<K, Pred, A>& m, int nType, int nVersion)
{
    WriteCompactSize(os, m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        Serialize(os, (*it), nType, nVersion);
}

template<typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream& is, std::set<K, Pred, A>& m, int nType, int nVersion)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    typename std::set<K, Pred, A>::iterator it = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        K key;
        Unserialize(is, key, nType, nVersion);
        it = m.insert(it, key);
    }
}



//
// Support for IMPLEMENT_SERIALIZE and READWRITE macro
//
class CSerActionGetSerializeSize { };
class CSerActionSerialize { };
class CSerActionUnserialize { };

template<typename Stream, typename T>
inline unsigned int SerReadWrite(Stream& s, const T& obj, int nType, int nVersion, CSerActionGetSerializeSize ser_action)
{
    return ::GetSerializeSize(obj, nType, nVersion);
}

template<typename Stream, typename T>
inline unsigned int SerReadWrite(Stream& s, const T& obj, int nType, int nVersion, CSerActionSerialize ser_action)
{
    ::Serialize(s, obj, nType, nVersion);
    return 0;
}

template<typename Stream, typename T>
inline unsigned int SerReadWrite(Stream& s, T& obj, int nType, int nVersion, CSerActionUnserialize ser_action)
{
    ::Unserialize(s, obj, nType, nVersion);
    return 0;
}

struct ser_streamplaceholder
{
    int nType;
    int nVersion;
};








/*
    @up4dev
    重载的内存管理器，区别是在析构的时候会将内存全部置0
*/
//
// Allocator that clears its contents before deletion
//
template<typename T>
struct secure_allocator : public std::allocator<T>
{
    // MSVC8 default copy constructor is broken
    typedef std::allocator<T> base;
    typedef typename base::size_type size_type;
    typedef typename base::difference_type  difference_type;
    typedef typename base::pointer pointer;
    typedef typename base::const_pointer const_pointer;
    typedef typename base::reference reference;
    typedef typename base::const_reference const_reference;
    typedef typename base::value_type value_type;
    secure_allocator() throw() {}
    secure_allocator(const secure_allocator& a) throw() : base(a) {}
    ~secure_allocator() throw() {}
    template<typename _Other> struct rebind
    { typedef secure_allocator<_Other> other; };

    void deallocate(T* p, std::size_t n)
    {
        if (p != NULL)
            memset(p, 0, sizeof(T) * n);
        allocator<T>::deallocate(p, n);
    }
};


/*
    @up4dev
    CDataStream整合了vector和stream两类接口
    填入数据耗时是线性时间复杂度的
*/
//
// Double ended buffer combining vector and stream-like interfaces.
// >> and << read and write unformatted data using the above serialization templates.
// Fills with data in linear time; some stringstream implementations take N^2 time.
//
class CDataStream
{
protected:
    typedef vector<char, secure_allocator<char> > vector_type;
    vector_type vch;
    unsigned int nReadPos;
    short state;
    short exceptmask;
public:
    int nType;
    int nVersion;

    typedef vector_type::allocator_type   allocator_type;
    typedef vector_type::size_type        size_type;
    typedef vector_type::difference_type  difference_type;
    typedef vector_type::reference        reference;
    typedef vector_type::const_reference  const_reference;
    typedef vector_type::value_type       value_type;
    typedef vector_type::iterator         iterator;
    typedef vector_type::const_iterator   const_iterator;
    typedef vector_type::reverse_iterator reverse_iterator;

    explicit CDataStream(int nTypeIn=0, int nVersionIn=VERSION)
    {
        Init(nTypeIn, nVersionIn);
    }

    CDataStream(const_iterator pbegin, const_iterator pend, int nTypeIn=0, int nVersionIn=VERSION) : vch(pbegin, pend)
    {
        Init(nTypeIn, nVersionIn);
    }

#if !defined(_MSC_VER) || _MSC_VER >= 1300
    CDataStream(const char* pbegin, const char* pend, int nTypeIn=0, int nVersionIn=VERSION) : vch(pbegin, pend)
    {
        Init(nTypeIn, nVersionIn);
    }
#endif

    CDataStream(const vector_type& vchIn, int nTypeIn=0, int nVersionIn=VERSION) : vch(vchIn.begin(), vchIn.end())
    {
        Init(nTypeIn, nVersionIn);
    }

    CDataStream(const vector<char>& vchIn, int nTypeIn=0, int nVersionIn=VERSION) : vch(vchIn.begin(), vchIn.end())
    {
        Init(nTypeIn, nVersionIn);
    }

    CDataStream(const vector<unsigned char>& vchIn, int nTypeIn=0, int nVersionIn=VERSION) : vch((char*)&vchIn.begin()[0], (char*)&vchIn.end()[0])
    {
        Init(nTypeIn, nVersionIn);
    }

    void Init(int nTypeIn=0, int nVersionIn=VERSION)
    {
        nReadPos = 0;
        nType = nTypeIn;
        nVersion = nVersionIn;
        state = 0;
        exceptmask = ios::badbit | ios::failbit;
    }

    CDataStream& operator+=(const CDataStream& b)
    {
        vch.insert(vch.end(), b.begin(), b.end());
        return *this;
    }

    friend CDataStream operator+(const CDataStream& a, const CDataStream& b)
    {
        CDataStream ret = a;
        ret += b;
        return (ret);
    }

    string str() const
    {
        return (string(begin(), end()));
    }


    //
    // Vector subset
    //
    const_iterator begin() const                     { return vch.begin() + nReadPos; }
    iterator begin()                                 { return vch.begin() + nReadPos; }
    const_iterator end() const                       { return vch.end(); }
    iterator end()                                   { return vch.end(); }
    size_type size() const                           { return vch.size() - nReadPos; }
    bool empty() const                               { return vch.size() == nReadPos; }
    void resize(size_type n, value_type c=0)         { vch.resize(n + nReadPos, c); }
    void reserve(size_type n)                        { vch.reserve(n + nReadPos); }
    const_reference operator[](size_type pos) const  { return vch[pos + nReadPos]; }
    reference operator[](size_type pos)              { return vch[pos + nReadPos]; }
    void clear()                                     { vch.clear(); nReadPos = 0; }
    iterator insert(iterator it, const char& x=char()) { return vch.insert(it, x); }
    void insert(iterator it, size_type n, const char& x) { vch.insert(it, n, x); }

    void insert(iterator it, const_iterator first, const_iterator last)
    {
        if (it == vch.begin() + nReadPos && last - first <= nReadPos)
        {
            // special case for inserting at the front when there's room
            nReadPos -= (last - first);
            memcpy(&vch[nReadPos], &first[0], last - first);
        }
        else
            vch.insert(it, first, last);
    }

#if !defined(_MSC_VER) || _MSC_VER >= 1300
    void insert(iterator it, const char* first, const char* last)
    {
        if (it == vch.begin() + nReadPos && last - first <= nReadPos)
        {
            // special case for inserting at the front when there's room
            nReadPos -= (last - first);
            memcpy(&vch[nReadPos], &first[0], last - first);
        }
        else
            vch.insert(it, first, last);
    }
#endif

    iterator erase(iterator it)
    {
        if (it == vch.begin() + nReadPos)
        {
            // special case for erasing from the front
            if (++nReadPos >= vch.size())
            {
                // whenever we reach the end, we take the opportunity to clear the buffer
                nReadPos = 0;
                return vch.erase(vch.begin(), vch.end());
            }
            return vch.begin() + nReadPos;
        }
        else
            return vch.erase(it);
    }

    iterator erase(iterator first, iterator last)
    {
        if (first == vch.begin() + nReadPos)
        {
            // special case for erasing from the front
            if (last == vch.end())
            {
                nReadPos = 0;
                return vch.erase(vch.begin(), vch.end());
            }
            else
            {
                nReadPos = (last - vch.begin());
                return last;
            }
        }
        else
            return vch.erase(first, last);
    }

    inline void Compact()
    {
        vch.erase(vch.begin(), vch.begin() + nReadPos);
        nReadPos = 0;
    }

    bool Rewind(size_type n)
    {
        // Rewind by n characters if the buffer hasn't been compacted yet
        if (n > nReadPos)
            return false;
        nReadPos -= n;
        return true;
    }


    //
    // Stream subset
    //
    void setstate(short bits, const char* psz)
    {
        state |= bits;
        if (state & exceptmask)
            throw std::ios_base::failure(psz);
    }

    bool eof() const             { return size() == 0; }
    bool fail() const            { return state & (ios::badbit | ios::failbit); }
    bool good() const            { return !eof() && (state == 0); }
    void clear(short n)          { state = n; }  // name conflict with vector clear()
    short exceptions()           { return exceptmask; }
    short exceptions(short mask) { short prev = exceptmask; exceptmask = mask; setstate(0, "CDataStream"); return prev; }
    CDataStream* rdbuf()         { return this; }
    int in_avail()               { return size(); }

    void SetType(int n)          { nType = n; }
    int GetType()                { return nType; }
    void SetVersion(int n)       { nVersion = n; }
    int GetVersion()             { return nVersion; }
    void ReadVersion()           { *this >> nVersion; }
    void WriteVersion()          { *this << nVersion; }

    CDataStream& read(char* pch, int nSize)
    {
        // Read from the beginning of the buffer
        assert(nSize >= 0);
        unsigned int nReadPosNext = nReadPos + nSize;
        if (nReadPosNext >= vch.size())
        {
            if (nReadPosNext > vch.size())
            {
                setstate(ios::failbit, "CDataStream::read() : end of data");
                memset(pch, 0, nSize);
                nSize = vch.size() - nReadPos;
            }
            memcpy(pch, &vch[nReadPos], nSize);
            nReadPos = 0;
            vch.clear();
            return (*this);
        }
        memcpy(pch, &vch[nReadPos], nSize);
        nReadPos = nReadPosNext;
        return (*this);
    }

    CDataStream& ignore(int nSize)
    {
        // Ignore from the beginning of the buffer
        assert(nSize >= 0);
        unsigned int nReadPosNext = nReadPos + nSize;
        if (nReadPosNext >= vch.size())
        {
            if (nReadPosNext > vch.size())
            {
                setstate(ios::failbit, "CDataStream::ignore() : end of data");
                nSize = vch.size() - nReadPos;
            }
            nReadPos = 0;
            vch.clear();
            return (*this);
        }
        nReadPos = nReadPosNext;
        return (*this);
    }

    CDataStream& write(const char* pch, int nSize)
    {
        // Write to the end of the buffer
        assert(nSize >= 0);
        vch.insert(vch.end(), pch, pch + nSize);
        return (*this);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=VERSION) const
    {
        // Special case: stream << stream concatenates like stream += stream
        if (!vch.empty())
            s.write((char*)&vch[0], vch.size() * sizeof(vch[0]));
    }

    template<typename T>
    unsigned int GetSerializeSize(const T& obj)
    {
        // Tells the size of the object if serialized to this stream
        return ::GetSerializeSize(obj, nType, nVersion);
    }

    template<typename T>
    CDataStream& operator<<(const T& obj)
    {
        // Serialize to this stream
        ::Serialize(*this, obj, nType, nVersion);
        return (*this);
    }

    template<typename T>
    CDataStream& operator>>(T& obj)
    {
        // Unserialize from this stream
        ::Unserialize(*this, obj, nType, nVersion);
        return (*this);
    }
};

#ifdef TESTCDATASTREAM
// VC6sp6
// CDataStream:
// n=1000       0 seconds
// n=2000       0 seconds
// n=4000       0 seconds
// n=8000       0 seconds
// n=16000      0 seconds
// n=32000      0 seconds
// n=64000      1 seconds
// n=128000     1 seconds
// n=256000     2 seconds
// n=512000     4 seconds
// n=1024000    8 seconds
// n=2048000    16 seconds
// n=4096000    32 seconds
// stringstream:
// n=1000       1 seconds
// n=2000       1 seconds
// n=4000       13 seconds
// n=8000       87 seconds
// n=16000      400 seconds
// n=32000      1660 seconds
// n=64000      6749 seconds
// n=128000     27241 seconds
// n=256000     109804 seconds
#include <iostream>
int main(int argc, char *argv[])
{
    vector<unsigned char> vch(0xcc, 250);
    printf("CDataStream:\n");
    for (int n = 1000; n <= 4500000; n *= 2)
    {
        CDataStream ss;
        time_t nStart = time(NULL);
        for (int i = 0; i < n; i++)
            ss.write((char*)&vch[0], vch.size());
        printf("n=%-10d %d seconds\n", n, time(NULL) - nStart);
    }
    printf("stringstream:\n");
    for (int n = 1000; n <= 4500000; n *= 2)
    {
        stringstream ss;
        time_t nStart = time(NULL);
        for (int i = 0; i < n; i++)
            ss.write((char*)&vch[0], vch.size());
        printf("n=%-10d %d seconds\n", n, time(NULL) - nStart);
    }
}
#endif









/*
    @up4dev
    对FILE文件指针的封装
    主要是当类析构的时候会自动的关闭文件指针，避免泄露
*/
//
// Automatic closing wrapper for FILE*
//  - Will automatically close the file when it goes out of scope if not null.
//  - If you're returning the file pointer, return file.release().
//  - If you need to close the file early, use file.fclose() instead of fclose(file).
//
class CAutoFile
{
protected:
    FILE* file;
    short state;
    short exceptmask;
public:
    int nType;
    int nVersion;

    typedef FILE element_type;

    CAutoFile(FILE* filenew=NULL, int nTypeIn=SER_DISK, int nVersionIn=VERSION)
    {
        file = filenew;
        nType = nTypeIn;
        nVersion = nVersionIn;
        state = 0;
        exceptmask = ios::badbit | ios::failbit;
    }

    ~CAutoFile()
    {
        fclose();
    }

    void fclose()
    {
        if (file != NULL && file != stdin && file != stdout && file != stderr)
            ::fclose(file);
        file = NULL;
    }

    FILE* release()             { FILE* ret = file; file = NULL; return ret; }
    operator FILE*()            { return file; }
    FILE* operator->()          { return file; }
    FILE& operator*()           { return *file; }
    FILE** operator&()          { return &file; }
    FILE* operator=(FILE* pnew) { return file = pnew; }
    bool operator!()            { return (file == NULL); }


    //
    // Stream subset
    //
    void setstate(short bits, const char* psz)
    {
        state |= bits;
        if (state & exceptmask)
            throw std::ios_base::failure(psz);
    }

    bool fail() const            { return state & (ios::badbit | ios::failbit); }
    bool good() const            { return state == 0; }
    void clear(short n = 0)      { state = n; }
    short exceptions()           { return exceptmask; }
    short exceptions(short mask) { short prev = exceptmask; exceptmask = mask; setstate(0, "CAutoFile"); return prev; }

    void SetType(int n)          { nType = n; }
    int GetType()                { return nType; }
    void SetVersion(int n)       { nVersion = n; }
    int GetVersion()             { return nVersion; }
    void ReadVersion()           { *this >> nVersion; }
    void WriteVersion()          { *this << nVersion; }

    CAutoFile& read(char* pch, int nSize)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::read : file handle is NULL");
        if (fread(pch, 1, nSize, file) != nSize)
            setstate(ios::failbit, feof(file) ? "CAutoFile::read : end of file" : "CAutoFile::read : fread failed");
        return (*this);
    }

    CAutoFile& write(const char* pch, int nSize)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::write : file handle is NULL");
        if (fwrite(pch, 1, nSize, file) != nSize)
            setstate(ios::failbit, "CAutoFile::write : write failed");
        return (*this);
    }

    template<typename T>
    unsigned int GetSerializeSize(const T& obj)
    {
        // Tells the size of the object if serialized to this stream
        return ::GetSerializeSize(obj, nType, nVersion);
    }

    template<typename T>
    CAutoFile& operator<<(const T& obj)
    {
        // Serialize to this stream
        if (!file)
            throw std::ios_base::failure("CAutoFile::operator<< : file handle is NULL");
        ::Serialize(*this, obj, nType, nVersion);
        return (*this);
    }

    template<typename T>
    CAutoFile& operator>>(T& obj)
    {
        // Unserialize from this stream
        if (!file)
            throw std::ios_base::failure("CAutoFile::operator>> : file handle is NULL");
        ::Unserialize(*this, obj, nType, nVersion);
        return (*this);
    }
};
