// rc4.h
//
// c++ header-only implementation of the rc4 pseudorandom keystream generator


#ifndef RC4_H
#define RC4_H

class rc4 {
    static constexpr size_t N = 256;
    uint8_t S[N];
    int i = 0, j = 0;

    void swap(uint8_t *a, uint8_t *b) {
        int tmp = *a;
        *a = *b;
        *b = tmp;
    }

    int KSA(const uint8_t *key, ssize_t key_len, unsigned char *S) {
        int j = 0;

        for(size_t i = 0; i < N; i++) {
            S[i] = i;
        }
        for(size_t i = 0; i < N; i++) {
            j = (j + S[i] + key[i % key_len]) % N;
            swap(&S[i], &S[j]);
        }

        return 0;
    }

    static void fprintf_raw_as_hex(FILE *f, const uint8_t *data, unsigned int len) {
        const unsigned char *x = data;
        const unsigned char *end = data + len;
        size_t i=0;
        while (x < end) {
            fprintf(f, "%02x", *x++);
            if (++i % 32 == 0) {
                fprintf(f, "\n");
            }
        }
    }

public:

    rc4(const uint8_t *key, size_t key_len) {
        if (key_len != 16) {
            fprintf(stderr, "warning: %s not yet tested with key length %zu\n", __func__, key_len);
        }
        KSA(key, key_len, S);
    }

    void write_keystream(uint8_t *keystream, size_t keystream_len) {
        for(size_t n = 0; n < keystream_len; n++) {
            i = (i + 1) % N;
            j = (j + S[i]) % N;
            swap(&S[i], &S[j]);
            keystream[n] = S[(S[i] + S[j]) % N];
        }
    }

    void advance(size_t keystream_len) {
        for(size_t n = 0; n < keystream_len; n++) {
            i = (i + 1) % N;
            j = (j + S[i]) % N;
            swap(&S[i], &S[j]);
        }
    }

    static bool test() {

        // test cases are from RFC 6229 (https://tools.ietf.org/html/rfc6229)
        //
        // note: the keystream data presented in that RFC represent
        // *non-contiguous* segments of the keystream.  The offsets
        // associated with the 16-octet segments are indicated in the
        // comments below, and those offsets are used in the
        // rc4::advance() function to advance to the correct
        // position in the keystream.

        // Test Case
        //
        //    Key length: 128 bits.
        //    key: 0x0102030405060708090a0b0c0d0e0f10
        //
        uint8_t key[16] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
        };
        uint8_t keystream[288] = {
            0x9a, 0xc7, 0xcc, 0x9a, 0x60, 0x9d, 0x1e, 0xf7, 0xb2, 0x93, 0x28, 0x99, 0xcd, 0xe4, 0x1b, 0x97,  // offset    0
            0x52, 0x48, 0xc4, 0x95, 0x90, 0x14, 0x12, 0x6a, 0x6e, 0x8a, 0x84, 0xf1, 0x1d, 0x1a, 0x9e, 0x1c,  // offset   16
            0x06, 0x59, 0x02, 0xe4, 0xb6, 0x20, 0xf6, 0xcc, 0x36, 0xc8, 0x58, 0x9f, 0x66, 0x43, 0x2f, 0x2b,  // offset  240
            0xd3, 0x9d, 0x56, 0x6b, 0xc6, 0xbc, 0xe3, 0x01, 0x07, 0x68, 0x15, 0x15, 0x49, 0xf3, 0x87, 0x3f,  // offset  256
            0xb6, 0xd1, 0xe6, 0xc4, 0xa5, 0xe4, 0x77, 0x1c, 0xad, 0x79, 0x53, 0x8d, 0xf2, 0x95, 0xfb, 0x11,  // offset  496
            0xc6, 0x8c, 0x1d, 0x5c, 0x55, 0x9a, 0x97, 0x41, 0x23, 0xdf, 0x1d, 0xbc, 0x52, 0xa4, 0x3b, 0x89,  // offset  512
            0xc5, 0xec, 0xf8, 0x8d, 0xe8, 0x97, 0xfd, 0x57, 0xfe, 0xd3, 0x01, 0x70, 0x1b, 0x82, 0xa2, 0x59,  // offset  752
            0xec, 0xcb, 0xe1, 0x3d, 0xe1, 0xfc, 0xc9, 0x1c, 0x11, 0xa0, 0xb2, 0x6c, 0x0b, 0xc8, 0xfa, 0x4d,  // offset  768
            0xe7, 0xa7, 0x25, 0x74, 0xf8, 0x78, 0x2a, 0xe2, 0x6a, 0xab, 0xcf, 0x9e, 0xbc, 0xd6, 0x60, 0x65,  // offset 1008
            0xbd, 0xf0, 0x32, 0x4e, 0x60, 0x83, 0xdc, 0xc6, 0xd3, 0xce, 0xdd, 0x3c, 0xa8, 0xc5, 0x3c, 0x16,  // offset 1024
            0xb4, 0x01, 0x10, 0xc4, 0x19, 0x0b, 0x56, 0x22, 0xa9, 0x61, 0x16, 0xb0, 0x01, 0x7e, 0xd2, 0x97,  // offset 1520
            0xff, 0xa0, 0xb5, 0x14, 0x64, 0x7e, 0xc0, 0x4f, 0x63, 0x06, 0xb8, 0x92, 0xae, 0x66, 0x11, 0x81,  // offset 1536
            0xd0, 0x3d, 0x1b, 0xc0, 0x3c, 0xd3, 0x3d, 0x70, 0xdf, 0xf9, 0xfa, 0x5d, 0x71, 0x96, 0x3e, 0xbd,  // offset 2032
            0x8a, 0x44, 0x12, 0x64, 0x11, 0xea, 0xa7, 0x8b, 0xd5, 0x1e, 0x8d, 0x87, 0xa8, 0x87, 0x9b, 0xf5,  // offset 2048
            0xfa, 0xbe, 0xb7, 0x60, 0x28, 0xad, 0xe2, 0xd0, 0xe4, 0x87, 0x22, 0xe4, 0x6c, 0x46, 0x15, 0xa3,  // offset 3056
            0xc0, 0x5d, 0x88, 0xab, 0xd5, 0x03, 0x57, 0xf9, 0x35, 0xa6, 0x3c, 0x59, 0xee, 0x53, 0x76, 0x23,  // offset 3072
            0xff, 0x38, 0x26, 0x5c, 0x16, 0x42, 0xc1, 0xab, 0xe8, 0xd3, 0xc2, 0xfe, 0x5e, 0x57, 0x2b, 0xf8,  // offset 4080
            0xa3, 0x6a, 0x4c, 0x30, 0x1a, 0xe8, 0xac, 0x13, 0x61, 0x0c, 0xcb, 0xc1, 0x22, 0x56, 0xca, 0xcc   // offset 4096
        };
        uint8_t keystream2[288] = { 0 };

        rc4 rc4{key, sizeof(key)};
        uint8_t *k = keystream2;
        size_t index = 0;
        rc4.write_keystream(k, 32);  k += 32;   index += 32;

        rc4.advance(240 - index);
        rc4.write_keystream(k, 32);  k += 32;   index = (240 + 32);

        rc4.advance(496 - index);
        rc4.write_keystream(k, 32);  k += 32;   index = (496 + 32);

        rc4.advance(752 - index);
        rc4.write_keystream(k, 32);  k += 32;   index = (752 + 32);

        rc4.advance(1008 - index);
        rc4.write_keystream(k, 32);  k += 32;   index = (1008 + 32);

        rc4.advance(1520 - index);
        rc4.write_keystream(k, 32);  k += 32;   index = (1520 + 32);

        rc4.advance(2032 - index);
        rc4.write_keystream(k, 32);  k += 32;   index = (2032 + 32);

        rc4.advance(3056 - index);
        rc4.write_keystream(k, 32);  k += 32;   index = (3056 + 32);

        rc4.advance(4080 - index );
        rc4.write_keystream(k, 32);  k += 32;   index = (4080 + 32);

        if (memcmp(keystream, keystream2, sizeof(keystream)) != 0) {
            fprintf(stderr, "error: rc4 output did not match reference keystream in static test\n");
            fputs("key:        ", stdout);
            fprintf_raw_as_hex(stdout, key, sizeof(key));
            fputs("\nkeystream:  ", stdout);
            fprintf_raw_as_hex(stdout, keystream, sizeof(keystream));
            fputs("\nkeystream2: ", stdout);
            fprintf_raw_as_hex(stdout, keystream2, sizeof(keystream2));
            fputc('\n', stdout);
            return false;
        }

        return true;
    }
};

#endif // RC4_H
