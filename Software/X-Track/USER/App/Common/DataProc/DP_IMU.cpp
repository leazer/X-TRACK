#include "DataProc.h"
#include "../HAL/HAL.h"

DATA_PROC_INIT_DEF(IMU)
{
    /* NOTE: KEIL (non-C++11) doesn't support lambda. */
    extern bool DP_IMU_CommitThunk(void* info, void* userData);
    HAL::IMU_SetCommitCallback(DP_IMU_CommitThunk, account);
}

bool DP_IMU_CommitThunk(void* info, void* userData)
{
    Account* account = (Account*)userData;
    return account->Commit(info, sizeof(HAL::IMU_Info_t));
}
