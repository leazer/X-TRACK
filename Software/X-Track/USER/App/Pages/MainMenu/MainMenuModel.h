#ifndef __MAIN_MENU_MODEL_H
#define __MAIN_MENU_MODEL_H

#include "Common/DataProc/DataProc.h"

namespace Page
{

class MainMenuModel
{
public:
    void Init();
    void Deinit();

    void SetStatusBarStyle(DataProc::StatusBar_Style_t style);

private:
    Account* account;
};

}

#endif

