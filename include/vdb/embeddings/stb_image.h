/* stb_image - v2.29 - public domain image loader - http://nothings.org/stb
   
   This is a single-header library. To use it, do this in ONE C/C++ file:
      #define STB_IMAGE_IMPLEMENTATION
      #include "stb_image.h"
   
   DOCUMENTATION:
   
   Limitations:
      - no 12-bit-per-channel JPEG
      - no JPEGs with arithmetic coding
      - GIF always returns *comp=4
   
   Basic usage (see HDR discussion below for HDR usage):
      int x,y,n;
      unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
      // ... process data if not NULL ...
      // ... x = width, y = height, n = # 8-bit components per pixel ...
      // ... replace '0' with '1'..'4' to force that many components per pixel
      stbi_image_free(data);

   LICENSE: This software is in the public domain. Where that dedication is not
   recognized, you are granted a perpetual, irrevocable license to copy,
   modify and distribute this software.

   NOTE: This is a STUB header for the vector_database project.
   Download the full stb_image.h from: https://github.com/nothings/stb/blob/master/stb_image.h
   Or use vcpkg: vcpkg install stb
*/

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

// IMPORTANT: This is a minimal stub. For the actual implementation,
// download from: https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
// and replace this file.

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

// Primary API
STBIDEF stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF void     stbi_image_free(void *retval_from_stbi_load);
STBIDEF const char *stbi_failure_reason(void);

#ifdef __cplusplus
}
#endif

//
// IMPLEMENTATION
//
#ifdef STB_IMAGE_IMPLEMENTATION

static thread_local const char* stbi__g_failure_reason = "";

STBIDEF const char* stbi_failure_reason(void) {
    return stbi__g_failure_reason;
}

STBIDEF void stbi_image_free(void* retval_from_stbi_load) {
    free(retval_from_stbi_load);
}

// ============================================================================
// PNG Decoder - Minimal implementation for RGBA images
// ============================================================================

#include <zlib.h>  // For deflate decompression

// PNG signature
static const unsigned char png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

static unsigned int stbi__get32be(const unsigned char* p) {
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static int stbi__paeth(int a, int b, int c) {
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static void stbi__unfilter_row(unsigned char* out, const unsigned char* in, 
                                const unsigned char* prev, int filter, 
                                int width, int bpp) {
    switch (filter) {
        case 0: // None
            memcpy(out, in, width * bpp);
            break;
        case 1: // Sub
            for (int i = 0; i < width * bpp; i++) {
                int left = (i >= bpp) ? out[i - bpp] : 0;
                out[i] = in[i] + left;
            }
            break;
        case 2: // Up
            for (int i = 0; i < width * bpp; i++) {
                int up = prev ? prev[i] : 0;
                out[i] = in[i] + up;
            }
            break;
        case 3: // Average
            for (int i = 0; i < width * bpp; i++) {
                int left = (i >= bpp) ? out[i - bpp] : 0;
                int up = prev ? prev[i] : 0;
                out[i] = in[i] + ((left + up) >> 1);
            }
            break;
        case 4: // Paeth
            for (int i = 0; i < width * bpp; i++) {
                int left = (i >= bpp) ? out[i - bpp] : 0;
                int up = prev ? prev[i] : 0;
                int upleft = (prev && i >= bpp) ? prev[i - bpp] : 0;
                out[i] = in[i] + stbi__paeth(left, up, upleft);
            }
            break;
    }
}

STBIDEF stbi_uc* stbi_load_from_memory(stbi_uc const* buffer, int len, 
                                        int* x, int* y, 
                                        int* channels_in_file, 
                                        int desired_channels) {
    if (len < 8 || memcmp(buffer, png_signature, 8) != 0) {
        stbi__g_failure_reason = "not PNG";
        return NULL;
    }
    
    const unsigned char* p = buffer + 8;
    const unsigned char* end = buffer + len;
    
    int width = 0, height = 0, bit_depth = 0, color_type = 0;
    unsigned char* compressed = NULL;
    size_t compressed_len = 0;
    size_t compressed_cap = 0;
    
    // Parse chunks
    while (p + 8 <= end) {
        unsigned int chunk_len = stbi__get32be(p);
        const char* chunk_type = (const char*)(p + 4);
        p += 8;
        
        if (p + chunk_len + 4 > end) break;
        
        if (memcmp(chunk_type, "IHDR", 4) == 0 && chunk_len >= 13) {
            width = stbi__get32be(p);
            height = stbi__get32be(p + 4);
            bit_depth = p[8];
            color_type = p[9];
            // Compression, filter, interlace at p[10-12]
            
            if (bit_depth != 8 || (color_type != 2 && color_type != 6)) {
                stbi__g_failure_reason = "unsupported PNG format";
                return NULL;
            }
        }
        else if (memcmp(chunk_type, "IDAT", 4) == 0) {
            // Accumulate compressed data
            if (compressed_len + chunk_len > compressed_cap) {
                compressed_cap = (compressed_len + chunk_len) * 2;
                compressed = (unsigned char*)realloc(compressed, compressed_cap);
            }
            memcpy(compressed + compressed_len, p, chunk_len);
            compressed_len += chunk_len;
        }
        else if (memcmp(chunk_type, "IEND", 4) == 0) {
            break;
        }
        
        p += chunk_len + 4; // Skip data + CRC
    }
    
    if (width == 0 || height == 0 || compressed == NULL) {
        free(compressed);
        stbi__g_failure_reason = "invalid PNG";
        return NULL;
    }
    
    // Decompress
    int src_bpp = (color_type == 6) ? 4 : 3;
    size_t raw_size = (size_t)height * ((size_t)width * src_bpp + 1);
    unsigned char* raw = (unsigned char*)malloc(raw_size);
    
    z_stream strm = {};
    strm.next_in = compressed;
    strm.avail_in = (uInt)compressed_len;
    strm.next_out = raw;
    strm.avail_out = (uInt)raw_size;
    
    if (inflateInit(&strm) != Z_OK) {
        free(compressed);
        free(raw);
        stbi__g_failure_reason = "zlib init failed";
        return NULL;
    }
    
    int ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    free(compressed);
    
    if (ret != Z_STREAM_END) {
        free(raw);
        stbi__g_failure_reason = "zlib decompress failed";
        return NULL;
    }
    
    // Unfilter and convert
    int out_bpp = (desired_channels == 0) ? src_bpp : desired_channels;
    unsigned char* output = (unsigned char*)malloc((size_t)width * height * out_bpp);
    unsigned char* prev_row = NULL;
    unsigned char* cur_row = (unsigned char*)malloc((size_t)width * src_bpp);
    
    const unsigned char* src = raw;
    unsigned char* dst = output;
    
    for (int row = 0; row < height; row++) {
        int filter = *src++;
        stbi__unfilter_row(cur_row, src, prev_row, filter, width, src_bpp);
        src += width * src_bpp;
        
        // Convert to output format
        for (int col = 0; col < width; col++) {
            unsigned char r = cur_row[col * src_bpp + 0];
            unsigned char g = cur_row[col * src_bpp + 1];
            unsigned char b = cur_row[col * src_bpp + 2];
            unsigned char a = (src_bpp == 4) ? cur_row[col * src_bpp + 3] : 255;
            
            switch (out_bpp) {
                case 1: dst[col] = (r + g + b) / 3; break;
                case 2: dst[col*2+0] = (r+g+b)/3; dst[col*2+1] = a; break;
                case 3: dst[col*3+0] = r; dst[col*3+1] = g; dst[col*3+2] = b; break;
                case 4: dst[col*4+0] = r; dst[col*4+1] = g; dst[col*4+2] = b; dst[col*4+3] = a; break;
            }
        }
        
        dst += width * out_bpp;
        
        // Swap buffers for Paeth filter
        unsigned char* tmp = prev_row;
        prev_row = cur_row;
        cur_row = tmp;
        if (cur_row == NULL) cur_row = (unsigned char*)malloc((size_t)width * src_bpp);
    }
    
    free(raw);
    free(cur_row);
    free(prev_row);
    
    *x = width;
    *y = height;
    if (channels_in_file) *channels_in_file = src_bpp;
    
    return output;
}

STBIDEF stbi_uc* stbi_load(char const* filename, int* x, int* y, 
                           int* channels_in_file, int desired_channels) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        stbi__g_failure_reason = "can't open file";
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char* buffer = (unsigned char*)malloc(len);
    if (fread(buffer, 1, len, f) != (size_t)len) {
        free(buffer);
        fclose(f);
        stbi__g_failure_reason = "can't read file";
        return NULL;
    }
    fclose(f);
    
    stbi_uc* result = stbi_load_from_memory(buffer, (int)len, x, y, channels_in_file, desired_channels);
    free(buffer);
    return result;
}

#endif // STB_IMAGE_IMPLEMENTATION

#endif // STBI_INCLUDE_STB_IMAGE_H
