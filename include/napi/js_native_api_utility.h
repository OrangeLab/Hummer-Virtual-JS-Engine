//
// Created by didi on 2021/6/6.
//

#ifndef SKIA_JS_NATIVE_API_UTILITY_H
#define SKIA_JS_NATIVE_API_UTILITY_H

typedef enum
{
    ConversionOK,    /* conversion successful */
    SourceExhausted, /* partial character in source, but hit end */
    TargetExhausted, /* inSufficent. room in target for conversion */
    SourceIllegal    /* source sequence is illegal/malformed */
} ConversionResult;
static ConversionResult convertUTF16toUTF8(const uint16_t *sourceStart, const uint16_t *sourceEnd,
                                           uint8_t **targetStart, const uint8_t *targetEnd);
#endif // SKIA_JS_NATIVE_API_UTILITY_H
