#define SERIALIZE_H
#include <stdint.h>

#define pack754_16(f) (pack754((f), 16, 5))
#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_16(i) (unpack754((i), 16, 5))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

// Typedef Message Structure
typedef struct
{
    uint8_t type1;
    uint32_t ue_id;
    uint16_t cause;
} RRCSetupRequest;

typedef struct
{
    uint8_t type2;
} RRCSetup;
typedef struct
{
    uint8_t type3;
} RRCSetupComplete;

// Serialization
unsigned long long int pack754(long double f, unsigned bits, unsigned expbits);
long double unpack754(unsigned long long int i, unsigned bits, unsigned expbits);
void packi16(unsigned char *buf, unsigned int i);
void packi32(unsigned char *buf, unsigned long int i);
void packi64(unsigned char *buf, unsigned long long int i);
int unpacki16(unsigned char *buf);
unsigned int unpacku16(unsigned char *buf);
long int unpacki32(unsigned char *buf);
unsigned long int unpacku32(unsigned char *buf);
long long int unpacki64(unsigned char *buf);
unsigned long long int unpacku64(unsigned char *buf);
// Case sensitive
unsigned int pack(unsigned char *buf, char *format, ...);
void unpack(unsigned char *buf, char *format, ...);

// Message Serialization
unsigned int pack_msg1(unsigned char *buf, const RRCSetupRequest *msg1);
void unpack_msg1(unsigned char *buf, RRCSetupRequest *msg1);
unsigned int pack_msg2(unsigned char *buf, const RRCSetup *msg2);
void unpack_msg2(unsigned char *buf, RRCSetup *msg2);
unsigned int pack_msg3(unsigned char *buf, const RRCSetupComplete *msg3);
void unpack_msg3(unsigned char *buf, RRCSetupComplete *msg3);