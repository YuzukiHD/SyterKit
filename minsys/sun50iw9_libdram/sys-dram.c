#include <timer.h>

extern int init_DRAM(int type, void *buff);

int dram()
{
    init_DRAM(0, (void*)0);
}

void printf(void* buff, ...)
{

}

int set_ddr_voltage(int set_vol)
{
    
}

void __usdelay(uint32_t loop)
{
    sdelay(loop);
}