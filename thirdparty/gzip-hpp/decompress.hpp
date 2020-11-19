#ifndef GZIP_HPP_DECOMPRESS_HPP
#define GZIP_HPP_DECOMPRESS_HPP

#ifndef ZLIB_CONST
#define ZLIB_CONST
#endif

// zlib
#include <zlib.h>

// std
#include <limits>
#include <stdexcept>
#include <string>

namespace gzip {

inline void decompress(const char* data,
                       std::size_t size,
                       std::string& output,
                       std::size_t max_uncompressed_size = 0,
                       std::size_t buffering_size = 0)
{
    if (buffering_size == 0)
    {
        buffering_size = (size * 2) - (size / 2) + 16;
    }
    z_stream inflate_s;
    inflate_s.zalloc = Z_NULL;
    inflate_s.zfree = Z_NULL;
    inflate_s.opaque = Z_NULL;
    inflate_s.avail_in = 0;
    inflate_s.next_in = Z_NULL;

    // The windowBits parameter is the base two logarithm of the window size (the size of the history buffer).
    // It should be in the range 8..15 for this version of the library.
    // Larger values of this parameter result in better compression at the expense of memory usage.
    // This range of values also changes the decoding type:
    //  -8 to -15 for raw deflate
    //  8 to 15 for zlib
    // (8 to 15) + 16 for gzip
    // (8 to 15) + 32 to automatically detect gzip/zlib header
    constexpr int window_bits = 15 + 32; // auto with windowbits of 15

    constexpr unsigned int max_uint = std::numeric_limits<unsigned int>::max();
    const unsigned int size_step = buffering_size > max_uint ? max_uint : static_cast<unsigned int>(buffering_size);
        if( max_uncompressed_size != 0 && size_step > max_uncompressed_size )
        {
            throw std::runtime_error(
                    "buffer size used during decompression of gzip will use more memory then allowed, "
                    "either increase the limit or reduce the buffer size" );
        }

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
    if (inflateInit2(&inflate_s, window_bits) != Z_OK)
    {
        throw std::runtime_error("inflate init failed");
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    inflate_s.next_in = reinterpret_cast<z_const Bytef*>(data);
    inflate_s.avail_in = static_cast<unsigned int>(size);
    std::string buffer(static_cast<std::size_t>(size_step), char());
    do
    {
        inflate_s.avail_out = size_step;
        inflate_s.next_out = reinterpret_cast<Bytef*>(&buffer[0]);
        const int ret = inflate(&inflate_s, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR)
        {
            std::string error_msg = inflate_s.msg;
            inflateEnd(&inflate_s);
            throw std::runtime_error(error_msg);
        }
        if (max_uncompressed_size != 0 && (output.size() + size_step - inflate_s.avail_out) > max_uncompressed_size)
        {
            inflateEnd(&inflate_s);
            throw std::runtime_error("size of output string will use more memory then intended when decompressing");
        }
        output.append(buffer, 0, size_step - inflate_s.avail_out);
    } while (inflate_s.avail_out == 0);
    const int ret2 = inflateEnd(&inflate_s);
    if (ret2 != Z_OK)
    {
        throw std::runtime_error("Unexpected gzip decompression error, state of stream was inconsistent");
    }
}

inline void decompress(std::string const& input,
                       std::string& output,
                       std::size_t max_uncompressed_size = 0,
                       std::size_t buffering_size = 0)
{
    return decompress(input.data(), input.size(), output, max_uncompressed_size, buffering_size);
}

inline std::string decompress(const char* data,
                              std::size_t size,
                              std::size_t max_uncompressed_size = 0,
                              std::size_t buffering_size = 0)
{
    std::string output;
    decompress(data, size, output, max_uncompressed_size, buffering_size);
    return output;
}

inline std::string decompress(std::string const& input,
                              std::size_t max_uncompressed_size = 0,
                              std::size_t buffering_size = 0)
{
    std::string output;
    decompress(input.data(), input.size(), output, max_uncompressed_size, buffering_size);
    return output;
}

} // namespace gzip

#endif // GZIP_HPP_DECOMPRESS_HPP