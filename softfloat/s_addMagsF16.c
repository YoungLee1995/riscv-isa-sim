
/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3d, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2015, 2016, 2017 The Regents of the
University of California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#include <stdbool.h>
#include <stdint.h>
#include "platform.h"
#include "internals.h"
#include "specialize.h"
#include "softfloat.h"

float16_t softfloat_addMagsF16(uint_fast16_t uiA, uint_fast16_t uiB)
{
    int_fast8_t expA;         // 浮点数 A 的指数
    uint_fast16_t sigA;       // 浮点数 A 的尾数
    int_fast8_t expB;         // 浮点数 B 的指数
    uint_fast16_t sigB;       // 浮点数 B 的尾数
    int_fast8_t expDiff;      // 指数差值
    uint_fast16_t uiZ;        // 最终结果的二进制表示
    bool signZ;               // 最终结果的符号位
    int_fast8_t expZ;         // 最终结果的指数
    uint_fast16_t sigZ;       // 最终结果的尾数
    uint_fast16_t sigX, sigY; // 用于对齐后的尾数
    int_fast8_t shiftDist;    // 尾数对齐时的位移量
    uint_fast32_t sig32Z;     // 32 位临时变量，用于尾数相加
    int_fast8_t roundingMode; // 舍入模式
    union ui16_f16 uZ;        // 用于将结果转换为 float16_t

    /*------------------------------------------------------------------------
     *------------------------------------------------------------------------*/
    expA = expF16UI(uiA);  // 取出 uiA 的指数部分
    sigA = fracF16UI(uiA); // 取出 uiA 的尾数部分
    expB = expF16UI(uiB);  // 取出 uiB 的指数部分
    sigB = fracF16UI(uiB); // 取出 uiB 的尾数部分
    /*------------------------------------------------------------------------
     *------------------------------------------------------------------------*/
    expDiff = expA - expB; // 计算两个浮点数的指数部分之差
    if (!expDiff)
    { // 如果两个浮点数的指数部分相等
        /*--------------------------------------------------------------------
         *--------------------------------------------------------------------*/
        if (!expA)
        {                     // 如果两个浮点数的指数部分都为 0
            uiZ = uiA + sigB; // 将 uiA 和 sigB 相加
            goto uiZ;         // 跳转到 uiZ
        }
        if (expA == 0x1F)
        { // 如果 uiA 的指数部分为 31
            if (sigA | sigB)
                goto propagateNaN; // 如果 sigA 或 sigB 不为 0，则调用 propagateNaN 函数
            uiZ = uiA;             // 将 uiA 赋值给 uiZ
            goto uiZ;
        }
        signZ = signF16UI(uiA);      // 取出 uiA 的符号位
        expZ = expA;                 // 将 expA 赋值给 expZ
        sigZ = 0x0800 + sigA + sigB; // 将 0x0800、sigA 和 sigB 相加,一个隐藏位是0x0400，两个是0x0800
        if (!(sigZ & 1) && (expZ < 0x1E))
        {               // 如果 sigZ 的最低位为 0 且 expZ 小于 30
            sigZ >>= 1; // 右移 1 位，之前增加的0x0800正好成为expZ的增加值 1
            goto pack;
        }
        sigZ <<= 3;
    }
    else // 如果两个浮点数的指数部分不相等
    {
        /*--------------------------------------------------------------------
         *--------------------------------------------------------------------*/
        signZ = signF16UI(uiA); // 取出 uiA 的符号位
        if (expDiff < 0) //expA < expB
        {
            /*----------------------------------------------------------------
             *----------------------------------------------------------------*/
            if (expB == 0x1F)
            { // 如果 uiB 的指数部分为 31
                if (sigB)
                    goto propagateNaN; // 如果 sigB 不为 0，则调用 propagateNaN 函数
                uiZ = packToF16UI(signZ, 0x1F, 0); // 将 signZ、0x1F 和 0 合并成一个 16 位无符号整数表示的浮点数
                goto uiZ;
            }
            if (expDiff <= -13) // 如果 expDiff 小于等于 -13
            {
                uiZ = packToF16UI(signZ, expB, sigB); // 将 signZ、expB 和 sigB 合并成一个 16 位无符号整数表示的浮点数
                if (expA | sigA)                      // 如果 expA 或 sigA 不为 0
                    goto addEpsilon;                  // 调用 addEpsilon 函数
                goto uiZ;
            }
            expZ = expB;
            sigX = sigB | 0x0400; // 将 sigB 和 0x0400 相或
            sigY = sigA + (expA ? 0x0400 : sigA); // 如果 expA 不为 0，则将 sigA 和 0x0400 相加
            shiftDist = 19 + expDiff;            // 计算位移量
        }
        else // 如果 expDiff 大于 0
        {
            /*----------------------------------------------------------------
             *----------------------------------------------------------------*/
            uiZ = uiA;
            if (expA == 0x1F)
            {
                if (sigA)
                    goto propagateNaN;
                goto uiZ;
            }
            if (13 <= expDiff)
            {
                if (expB | sigB)
                    goto addEpsilon;
                goto uiZ;
            }
            expZ = expA;
            sigX = sigA | 0x0400;
            sigY = sigB + (expB ? 0x0400 : sigB);
            shiftDist = 19 - expDiff;
        }
        sig32Z =
            ((uint_fast32_t)sigX << 19) + ((uint_fast32_t)sigY << shiftDist); // 将 sigX 左移 19 位和 sigY 左移 shiftDist 位相加
        if (sig32Z < 0x40000000)
        {
            --expZ;
            sig32Z <<= 1;
        } // 如果 sig32Z 小于 0x40000000，则将 expZ 减 1，sig32Z 左移 1 位
        sigZ = sig32Z >> 16; // 将 sig32Z 右移 16 位
        if (sig32Z & 0xFFFF)
        {
            sigZ |= 1;
        } // 如果 sig32Z 的低 16 位不为 0，则将 sigZ 的最低位设为 1
        else
        {
            if (!(sigZ & 0xF) && (expZ < 0x1E)) // 如果 sigZ 的低 4 位为 0 且 expZ 小于 30
            {
                sigZ >>= 4;
                goto pack;
            }
        }
    }
    return softfloat_roundPackToF16(signZ, expZ, sigZ);
    /*------------------------------------------------------------------------
     *------------------------------------------------------------------------*/
propagateNaN:
    uiZ = softfloat_propagateNaNF16UI(uiA, uiB); // 调用 softfloat_propagateNaNF16UI 函数
    goto uiZ;
    /*------------------------------------------------------------------------
     *------------------------------------------------------------------------*/
addEpsilon:
    roundingMode = softfloat_roundingMode; // 获取舍入模式
    if (roundingMode != softfloat_round_near_even)
    {
        if (
            roundingMode == (signF16UI(uiZ) ? softfloat_round_min
                                            : softfloat_round_max)) // 如果舍入模式为 round_min 或 round_max
        {
            ++uiZ; // 将 uiZ 加 1
            if ((uint16_t)(uiZ << 1) == 0xF800) // 如果 uiZ 的最高位为 1
            {
                softfloat_raiseFlags(
                    softfloat_flag_overflow | softfloat_flag_inexact); // 将 overflow 和 inexact 状态位置位
            }
        }
#ifdef SOFTFLOAT_ROUND_ODD
        else if (roundingMode == softfloat_round_odd)
        {
            uiZ |= 1;
        }
#endif
    } // 如果舍入模式不为 round_near_even，则根据舍入模式设置 uiZ
    softfloat_exceptionFlags |= softfloat_flag_inexact; // 将 inexact 状态位置位
    goto uiZ;
    /*------------------------------------------------------------------------
     *------------------------------------------------------------------------*/
pack:
    uiZ = packToF16UI(signZ, expZ, sigZ);
uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}

