#define BT1_H
#include <stdint.h>

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
    uint32_t ue_id;
} RRCSetupComplete;

// Serialization
void packi16(unsigned char *buf, unsigned int i);
void packi32(unsigned char *buf, unsigned long int i);
void packi64(unsigned char *buf, unsigned long long int i);
int unpacki16(unsigned char *buf);
unsigned int unpacku16(unsigned char *buf);
long int unpacki32(unsigned char *buf);
unsigned long int unpacku32(unsigned char *buf);
long long int unpacki64(unsigned char *buf);
unsigned long long int unpacku64(unsigned char *buf);

// Message Serialization
unsigned int pack_msg1(unsigned char *buf, const RRCSetupRequest *msg1);
void unpack_msg1(unsigned char *buf, RRCSetupRequest *msg1);
unsigned int pack_msg2(unsigned char *buf, const RRCSetup *msg2);
void unpack_msg2(unsigned char *buf, RRCSetup *msg2);
unsigned int pack_msg3(unsigned char *buf, const RRCSetupComplete *msg3);
void unpack_msg3(unsigned char *buf, RRCSetupComplete *msg3);
