#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include "bt1.h"

/*
** packi16() -- store a 16-bit int into a char buffer (like htons())
*/
void packi16(unsigned char *buf, unsigned int i)
{
    *buf++ = i >> 8;
    *buf++ = i;
}

/*
** packi32() -- store a 32-bit int into a char buffer (like htonl())
*/
void packi32(unsigned char *buf, unsigned long int i)
{
    *buf++ = i >> 24;
    *buf++ = i >> 16;
    *buf++ = i >> 8;
    *buf++ = i;
}

/*
** packi64() -- store a 64-bit int into a char buffer
*/
void packi64(unsigned char *buf, unsigned long long int i)
{
    *buf++ = i >> 56;
    *buf++ = i >> 48;
    *buf++ = i >> 40;
    *buf++ = i >> 32;
    *buf++ = i >> 24;
    *buf++ = i >> 16;
    *buf++ = i >> 8;
    *buf++ = i;
}

/*
** unpacki16() -- unpack a 16-bit int from a char buffer (like ntohs())
*/
int unpacki16(unsigned char *buf)
{
    unsigned int i2 = ((unsigned int)buf[0] << 8) | buf[1];
    int i;

    // change unsigned numbers to signed
    if (i2 <= 0x7fffu)
    {
        i = i2;
    }
    else
    {
        i = -1 - (unsigned int)(0xffffu - i2);
    }

    return i;
}

/*
** unpacku16() -- unpack a 16-bit unsigned from a char buffer (like ntohs())
*/
unsigned int unpacku16(unsigned char *buf)
{
    return ((unsigned int)buf[0] << 8) | buf[1];
}

/*
** unpacki32() -- unpack a 32-bit int from a char buffer (like ntohl())
*/
long int unpacki32(unsigned char *buf)
{
    unsigned long int i2 = ((unsigned long int)buf[0] << 24) |
                           ((unsigned long int)buf[1] << 16) |
                           ((unsigned long int)buf[2] << 8) |
                           buf[3];
    long int i;

    // change unsigned numbers to signed
    if (i2 <= 0x7fffffffu)
    {
        i = i2;
    }
    else
    {
        i = -1 - (long int)(0xffffffffu - i2);
    }

    return i;
}

/*
** unpacku32() -- unpack a 32-bit unsigned from a char buffer (like ntohl())
*/
unsigned long int unpacku32(unsigned char *buf)
{
    return ((unsigned long int)buf[0] << 24) |
           ((unsigned long int)buf[1] << 16) |
           ((unsigned long int)buf[2] << 8) |
           buf[3];
}

/*
** unpacki64() -- unpack a 64-bit int from a char buffer (like ntohl())
*/
long long int unpacki64(unsigned char *buf)
{
    unsigned long long int i2 = ((unsigned long long int)buf[0] << 56) |
                                ((unsigned long long int)buf[1] << 48) |
                                ((unsigned long long int)buf[2] << 40) |
                                ((unsigned long long int)buf[3] << 32) |
                                ((unsigned long long int)buf[4] << 24) |
                                ((unsigned long long int)buf[5] << 16) |
                                ((unsigned long long int)buf[6] << 8) |
                                buf[7];
    long long int i;

    // change unsigned numbers to signed
    if (i2 <= 0x7fffffffffffffffu)
    {
        i = i2;
    }
    else
    {
        i = -1 - (long long int)(0xffffffffffffffffu - i2);
    }

    return i;
}

/*
** unpacku64() -- unpack a 64-bit unsigned from a char buffer (like ntohl())
*/
unsigned long long int unpacku64(unsigned char *buf)
{
    return ((unsigned long long int)buf[0] << 56) |
           ((unsigned long long int)buf[1] << 48) |
           ((unsigned long long int)buf[2] << 40) |
           ((unsigned long long int)buf[3] << 32) |
           ((unsigned long long int)buf[4] << 24) |
           ((unsigned long long int)buf[5] << 16) |
           ((unsigned long long int)buf[6] << 8) |
           buf[7];
}

unsigned int pack_msg1(unsigned char *buf, const RRCSetupRequest *msg1)
{
    int size = 0;
    size += 1;
    // pack msg1.type1
    *buf++ = msg1->type1;

    // pack msg1.ue-id
    size += 4;
    packi32(buf, msg1->ue_id);
    buf += 4;

    // pack msg1.cause
    size += 2;
    packi16(buf, msg1->cause);
    buf += 2;

    return size;
}

void unpack_msg1(unsigned char *buf, RRCSetupRequest *msg1)
{
    // Unpack msg1.type
    msg1->type1 = *buf++;

    // Unpack msg1.ue-id
    msg1->ue_id = unpacku32(buf);
    buf += 4;

    // Unpack msg1.cause
    msg1->cause = unpacku16(buf);
    buf += 2;
}

unsigned int pack_msg2(unsigned char *buf, const RRCSetup *msg2)
{
    int size = 0;
    size += 1;
    // pack msg1.type1
    *buf++ = msg2->type2;

    return size;
}

void unpack_msg2(unsigned char *buf, RRCSetup *msg2)
{
    // Unpack msg1.type
    msg2->type2 = *buf++;
}

unsigned int pack_msg3(unsigned char *buf, const RRCSetupComplete *msg3)
{
    int size = 0;
    size += 1;
    // pack msg3.type3
    *buf++ = msg3->type3;

    // pack msg3.ue-id
    size += 4;
    packi32(buf, msg3->ue_id);
    buf += 4;

    return size;
}

void unpack_msg3(unsigned char *buf, RRCSetupComplete *msg3)
{
    // Unpack msg3.type
    msg3->type3 = *buf++;

    // Unpack msg3.ue-id
    msg3->ue_id = unpacku32(buf);
    buf += 4;
}