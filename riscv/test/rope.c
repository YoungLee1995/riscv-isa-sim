#include <string.h>
#include <stdint.h>

#define CALL_ROPE(A0, A1, A2, A3, A4, A5, A6)                                    \
    ({ __asm__ __volatile__("mv a0,%0 \n\t"                                      \
                            "mv a1,%1 \n\t"                                      \
                            "mv a2,%2 \n\t"                                      \
                            "mv a3,%3 \n\t"                                      \
                            "mv a4,%4 \n\t"                                      \
                            "mv a5,%5 \n\t"                                      \
                            "mv a6,%6 \n\t"                                      \
                            ".insu u 0x0b,a0,0x90\n\t"                           \
                            ".align 2 \n\t" ::"r"(A0),                           \
                            "r"(A1), "r"(A2), "r"(A3), "r"(A4), "r"(A5), "r"(A6) \
                            : "memory", "a0", "a1", "a2", "a3", "a4", "a5", "a6"); })

int g_busys[4] = 0;
#define saved_gp ((uint64_t *)(SM_MCU_SRAM_BASE + SM_MCU_SRAM_SIZE - 16));

int npu_proc(uint64_t arg0, uint64_t arg1)
{
    write_regl(gp, *saved_gp);
    unsigned long hartid;
    hartid = read_csr(0xf14);

    if (hartid == SM_HARTID_NPU0)
    {
        unsigned short input[32];
        void *start_addr = (void *)0x42000000;
        short tmp = 1000;
        for (int i = 0; i < 32; i++)
        {
            input[i] = tmp;
            tmp += 1000;
        }
        for (int i = 0; i < 32; i++)
        {
            memcpy(start_addr,input,32*sizeof(unsigned short));
            start_addr = (void *)(start_addr + sizeof(input));
        }
        unsigned short *psrc=(unsigned short *)(0x42000000UL+16*sizeof(input));
    }
    
}