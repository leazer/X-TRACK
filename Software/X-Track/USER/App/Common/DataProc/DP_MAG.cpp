#include "DataProc.h"
#include "../HAL/HAL.h"

DATA_PROC_INIT_DEF(MAG)
{
    /* NOTE: KEIL (non-C++11) doesn't support lambda. */
    extern bool DP_MAG_CommitThunk(void* info, void* userData);
    HAL::MAG_SetCommitCallback(DP_MAG_CommitThunk, account);
}

bool DP_MAG_CommitThunk(void* info, void* userData)
{
    Account* account = (Account*)userData;
    return account->Commit(info, sizeof(HAL::MAG_Info_t));
}
