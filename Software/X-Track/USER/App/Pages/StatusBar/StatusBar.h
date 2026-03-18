/*
 * MIT License
 * Copyright (c) 2021 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __STATUS_BAR_H
#define __STATUS_BAR_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
namespace Page
{

class StatusBar
{
public:
    static lv_obj_t* Create(lv_obj_t* par);

    struct UI_t
    {
        lv_obj_t *cont;

        struct
        {
            lv_obj_t *img;
            lv_obj_t *label;
        } satellite;

        lv_obj_t *imgSD;

        lv_obj_t *labelClock;

        lv_obj_t *labelRec;

        struct
        {
            lv_obj_t *img;
            lv_obj_t *objUsage;
            lv_obj_t *label;
        } battery;

        lv_obj_t *configWindow;
    };

    UI_t ui;
};

lv_obj_t* StatusBar_Create(lv_obj_t* par);

}
#else
/* C 兼容：仅暴露创建接口（模拟器/部分工具可能用 C 解析头文件） */
lv_obj_t* StatusBar_Create(lv_obj_t* par);
#endif

#endif
