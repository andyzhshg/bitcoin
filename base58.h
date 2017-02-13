// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.


/*
    @up4dev
    为什么用base-58而不是base-64呢?中本聪给出了如下的理由
    - 0OIl在某些字体下看起来容易被混淆，这可能被别有用心的人用来制造钓鱼账号，
        比如1J7mdg5rbQyUHENYdx39WVWK7fsLpE0OIl和1J7mdg5rbQyUHENYdx39WVWK7fsLpEO0lI
        就可能看起来特别相像。
    - 非数字非字母的账号不容易被人所接受，所以字符'+/'没有包含进来。
    - 没有标点符号，当账号出现在邮件中时，通常不会被展示成换行的形式。
    - 多数软件双击选择文本时，会选中整个账号而不会被识别成多个词。

    可见中本聪对账号易用，实用都做了比较深入的考虑。

    [Base64](https://zh.wikipedia.org/wiki/Base64)
    [Base58](https://zh.wikipedia.org/wiki/Base58)
*/

//
// Why base-58 instead of standard base-64 encoding?
// - Don't want 0OIl characters that look the same in some fonts and
//      could be used to create visually identical looking account numbers.
// - A string with non-alphanumeric characters is not as easily accepted as an account number.
// - E-mail usually won't line-break if there's no punctuation to break at.
// - Doubleclicking selects the whole number as one word if it's all alphanumeric.
//


//@up4dev 这就base64字符集去掉了'0IOl+/'这六个字符
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";


inline string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
{
    /*
        @up4dev
        转换base的主要思想就是将源字符串是做一个大整数(BigNum)，
        然后对这个大整数连续除以58取余数，
        这一些列的余数就是转换后的结果。
    */
    CAutoBN_CTX pctx;
    CBigNum bn58 = 58;
    CBigNum bn0 = 0;

    /*
        @up4dev 
        将输入字符串反转，并在最后补零
        反转是因为CBigNum.setvch需要输入的是小端的数据(实际上setvch中调用的openssl的函数BN_mpi2bn需要的输入是大端的，
        但是在setvch中已经做了处理，具体实现可以参考CBigNum的实现)；
        末尾补零是因为字符串整体会被作为大整数处理，如果字符串的首字节首位是1，
        则这个数会被作为负数处理，末尾补零则是人为的将其设为正数
    */
    // Convert big endian data to little endian
    // Extra zero at the end make sure bignum will interpret as a positive number
    vector<unsigned char> vchTmp(pend-pbegin+1, 0);
    reverse_copy(pbegin, pend, vchTmp.begin());

    // Convert little endian data to bignum
    CBigNum bn;
    bn.setvch(vchTmp);

    // Convert bignum to string
    string str;
    /*
        @up4dev
        按预估的转换后的字符串长度来预先分配内存
        Base58编码可以表示的比特位数为以2为底58的对数，这个值约等于5.858，原始编码可以表达8比特位，
        则边编码后的长度大概为原来的 8/5.858=1.37 倍，安全起见，取1.38倍，并多一个字符。
        (参考)[https://zh.wikipedia.org/wiki/Base58]
    */
    str.reserve((pend - pbegin) * 138 / 100 + 1);
    CBigNum dv;
    CBigNum rem;
    while (bn > bn0)
    {
        /*
            @up4dev
            BN_div 对bn除以bn58得到商dv和余数rem，余数计入结果，商赋值为bn继续循环计算
            (参考)[https://wiki.openssl.org/index.php/Manual:BN_add(3)]
        */
        if (!BN_div(&dv, &rem, &bn, &bn58, pctx))
            throw bignum_error("EncodeBase58 : BN_div failed");
        bn = dv;
        unsigned int c = rem.getulong();
        str += pszBase58[c];
    }

    /*
        @up4dev
        由于前置的0全部在大整数转换的时候被忽略，这里对其做补全的特殊处理
    */
    // Leading zeroes encoded as base58 zeros
    for (const unsigned char* p = pbegin; p < pend && *p == 0; p++)
        str += pszBase58[0];

    // Convert little endian string to big endian
    reverse(str.begin(), str.end());
    return str;
}

inline string EncodeBase58(const vector<unsigned char>& vch)
{
    return EncodeBase58(&vch[0], &vch[0] + vch.size());
}

inline bool DecodeBase58(const char* psz, vector<unsigned char>& vchRet)
{
    CAutoBN_CTX pctx;
    vchRet.clear();
    CBigNum bn58 = 58;
    CBigNum bn = 0;
    CBigNum bnChar;

    /*
        @up4dev
        首先忽略到所有的前导空格
    */
    while (isspace(*psz))
        psz++;

    // Convert big endian string to bignum
    for (const char* p = psz; *p; p++)
    {
        /*
            @up4dev
            在base58词汇表中查找字符
        */
        const char* p1 = strchr(pszBase58, *p);
        if (p1 == NULL)
        {
            while (isspace(*p)) //@up4dev 过滤空格
                p++;
            if (*p != '\0')
                return false;
            break;
        }
        bnChar.setulong(p1 - pszBase58);    //@up4dev 这是base58之后的数值(不是字符值)
        if (!BN_mul(&bn, &bn, &bn58, pctx)) //@up4dev 进行编码时的逆运算，做乘法
            throw bignum_error("DecodeBase58 : BN_mul failed");
        bn += bnChar;
    }

    // Get bignum as little endian data
    vector<unsigned char> vchTmp = bn.getvch(); //@up4dev 从BigNum中取回编码前字符串

    // Trim off sign byte if present    //@up4dev 如果存在人为添加的符号字节，则去掉
    if (vchTmp.size() >= 2 && vchTmp.end()[-1] == 0 && vchTmp.end()[-2] >= 0x80)
        vchTmp.erase(vchTmp.end()-1);

    // Restore leading zeros
    int nLeadingZeros = 0;
    for (const char* p = psz; *p == pszBase58[0]; p++)
        nLeadingZeros++;
    vchRet.assign(nLeadingZeros + vchTmp.size(), 0);

    // Convert little endian data to big endian
    reverse_copy(vchTmp.begin(), vchTmp.end(), vchRet.end() - vchTmp.size());
    return true;
}

inline bool DecodeBase58(const string& str, vector<unsigned char>& vchRet)
{
    return DecodeBase58(str.c_str(), vchRet);
}




/*
    @up4dev
    首先计算字符串的hash
    将hash字符串的前4字节接至源字符串的末尾
    将带hash的字符串做base58编码
*/
inline string EncodeBase58Check(const vector<unsigned char>& vchIn)
{
    // add 4-byte hash check to the end
    vector<unsigned char> vch(vchIn);
    uint256 hash = Hash(vch.begin(), vch.end());
    vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
    return EncodeBase58(vch);
}

/*
    @up4dev
    上面函数的逆过程
    base58解码
    计算除最后4字节的字符串的hash
    将计算所得hash前4字节做验证
*/
inline bool DecodeBase58Check(const char* psz, vector<unsigned char>& vchRet)
{
    if (!DecodeBase58(psz, vchRet))
        return false;
    if (vchRet.size() < 4)
    {
        vchRet.clear();
        return false;
    }
    uint256 hash = Hash(vchRet.begin(), vchRet.end()-4);
    if (memcmp(&hash, &vchRet.end()[-4], 4) != 0)
    {
        vchRet.clear();
        return false;
    }
    vchRet.resize(vchRet.size()-4);
    return true;
}

inline bool DecodeBase58Check(const string& str, vector<unsigned char>& vchRet)
{
    return DecodeBase58Check(str.c_str(), vchRet);
}






static const unsigned char ADDRESSVERSION = 0;

/*
    @up4dev
    将hash160(160位大整数)转换为地址字符串
    首字节是版本号，最终的地址字符串是base58加上校验的字符串的形式。
    目前的版本号是0，编码后显示的结果即是1开头的字符串
*/
inline string Hash160ToAddress(uint160 hash160)
{
    // add 1-byte version number to the front
    vector<unsigned char> vch(1, ADDRESSVERSION);
    vch.insert(vch.end(), UBEGIN(hash160), UEND(hash160));
    return EncodeBase58Check(vch);
}

/*
    @up4dev
    地址转换为hash160
    会做版本和校验位的合法性判断
*/
inline bool AddressToHash160(const char* psz, uint160& hash160Ret)
{
    vector<unsigned char> vch;
    if (!DecodeBase58Check(psz, vch))
        return false;
    if (vch.empty())
        return false;
    unsigned char nVersion = vch[0];
    if (vch.size() != sizeof(hash160Ret) + 1)
        return false;
    memcpy(&hash160Ret, &vch[1], sizeof(hash160Ret));
    return (nVersion <= ADDRESSVERSION);
}

inline bool AddressToHash160(const string& str, uint160& hash160Ret)
{
    return AddressToHash160(str.c_str(), hash160Ret);
}

inline bool IsValidBitcoinAddress(const char* psz)
{
    uint160 hash160;
    return AddressToHash160(psz, hash160);
}

inline bool IsValidBitcoinAddress(const string& str)
{
    return IsValidBitcoinAddress(str.c_str());
}



/*
    @up4dev
    公钥到地址字符串的转换
*/
inline string PubKeyToAddress(const vector<unsigned char>& vchPubKey)
{
    return Hash160ToAddress(Hash160(vchPubKey));
}
