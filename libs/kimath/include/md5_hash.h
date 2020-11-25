// Code by: B-Con (http://b-con.us)
// Released under the GNU GPL
// MD5 Hash Digest implementation (little endian byte order)

#ifndef __MD5_HASH_H
#define __MD5_HASH_H

#include <cstdint>
#include <string>

class MD5_HASH
{

public:
    MD5_HASH();
    MD5_HASH( const MD5_HASH& aOther );

    ~MD5_HASH();

    void Init();
    void Hash ( uint8_t *data, uint32_t length );
    void Hash ( int value );
    void Finalize();
    bool IsValid() const { return m_valid; };

    void SetValid( bool aValid ) { m_valid = aValid; }

    MD5_HASH& operator=( const MD5_HASH& aOther );

    bool operator==( const MD5_HASH& aOther ) const;
    bool operator!=( const MD5_HASH& aOther ) const;

    /** @return Build a hexadecimal string from the 16 bytes of MD5_HASH
     *  Mainly for debug purposes.
     * @param aCompactForm = false to generate a string with spaces between each byte (2 chars)
     * = true to generate a string filled with 32 hexadecimal chars
     */
    std::string Format( bool aCompactForm = false );

private:
    struct MD5_CTX {
       uint8_t data[64];
       uint32_t datalen;
       uint32_t bitlen[2];
       uint32_t state[4];
    };

    void md5_transform(MD5_CTX *ctx, uint8_t data[]);
    void md5_init(MD5_CTX *ctx);
    void md5_update(MD5_CTX *ctx, uint8_t data[], uint32_t len);
    void md5_final(MD5_CTX *ctx, uint8_t hash[]);

    bool m_valid;
    MD5_CTX m_ctx;
    uint8_t m_hash[16];
};

#endif
