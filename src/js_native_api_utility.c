//
// Created by didi on 2021/6/6.
//
#include <napi/js_native_utility.h>



static const uint32_t UNI_SUR_HIGH_START = 0xd800;

static const uint32_t UNI_SUR_HIGH_END = 0xdbff;

static const uint32_t UNI_SUR_LOW_START = 0xdc00;

static const uint32_t UNI_SUR_LOW_END = 0xdfff;

static const uint8_t halfShift = 10;

static const uint32_t halfBase = 0x10000;

static const uint32_t UNI_REPLACEMENT_CHAR = 0x0000fffd;

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const uint8_t firstByteMark[7] = {0x0, 0x0, 0xc0, 0xe0, 0xf0};

static ConversionResult convertUTF16toUTF8(const uint16_t *sourceStart, const uint16_t *sourceEnd,
                                           uint8_t **targetStart, const uint8_t *targetEnd)
{
    ConversionResult result = ConversionOK;
    uint8_t *target = *targetStart;
    while (sourceStart <= sourceEnd)
    {
        uint32_t unicodeValue;
        // 无效为 0xfffd，3 字节
        uint8_t bytesToWrite = 3;
        const uint32_t byteMask = 0xBF;
        const uint32_t byteMark = 0x80;
        unicodeValue = *sourceStart++;
        if (unicodeValue >= UNI_SUR_HIGH_START && unicodeValue <= UNI_SUR_HIGH_END)
        {
            // 代理平面
            if (sourceStart < sourceEnd)
            {
                uint16_t tail = *sourceStart;
                if (tail >= UNI_SUR_LOW_START && tail <= UNI_SUR_LOW_END)
                {
                    unicodeValue =
                        ((unicodeValue - UNI_SUR_HIGH_START) << halfShift) + (tail - UNI_SUR_LOW_START) + halfBase;
                    ++sourceStart;
                }
                else
                {
                    result = SourceIllegal;

                    break;
                }
            }
            else
            {
                result = SourceExhausted;

                break;
            }
        }
        else
        {
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (unicodeValue >= UNI_SUR_LOW_START && unicodeValue <= UNI_SUR_LOW_END)
            {
                result = SourceIllegal;

                break;
            }
        }
        /* Figure out how many bytes the result will require */
        if (unicodeValue < (uint32_t)0x80)
        {
            bytesToWrite = 1;
        }
        else if (unicodeValue < (uint32_t)0x800)
        {
            bytesToWrite = 2;
        }
        else if (unicodeValue < (uint32_t)0x10000)
        {
            bytesToWrite = 3;
        }
        else if (unicodeValue < (uint32_t)0x110000)
        {
            bytesToWrite = 4;
        }
        else
        {
            //            bytesToWrite = 3;
            unicodeValue = UNI_REPLACEMENT_CHAR;
        }

        if (target + bytesToWrite - 1 > targetEnd)
        {
            //            target -= bytesToWrite - 1;
            result = TargetExhausted;

            break;
        }
        // 从后往前写
        // offset 为 3，2，1，0
        // 但是 0 不考虑在循环做处理
        for (uint8_t offset = bytesToWrite - 1; offset > 0; --offset)
        {
            *(target + offset) = (uint8_t)((unicodeValue | byteMark) & byteMask);
        }
        *target = (uint8_t)(unicodeValue | firstByteMark[bytesToWrite]);
        target += bytesToWrite;
    }
    *targetStart = target;

    return result;
}
