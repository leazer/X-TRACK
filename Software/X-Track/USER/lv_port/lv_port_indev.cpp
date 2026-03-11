/**
 * @file lv_port_indev_templ.c
 *
 */

 /*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port.h"
#include "lvgl/lvgl.h"
#include "HAL/HAL.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void encoder_init(void);
static void encoder_read(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);
static void touchpad_read(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /*------------------
     * Encoder
     * -----------------*/

    /*Initialize your encoder if you have*/
    encoder_init();


    /* Register encoder input device */
    static lv_indev_drv_t indev_drv_enc;
    lv_indev_drv_init(&indev_drv_enc);
    indev_drv_enc.type = LV_INDEV_TYPE_ENCODER;
    indev_drv_enc.read_cb = encoder_read;
    lv_indev_t* indev_encoder = lv_indev_drv_register(&indev_drv_enc);
    lv_group_t* group = lv_group_create();
    lv_indev_set_group(indev_encoder, group);
    lv_group_set_default(group);

    HAL::TouchPad_Init();
    /* Register touch screen input device */
    static lv_indev_drv_t indev_drv_touch;
    lv_indev_drv_init(&indev_drv_touch);
    indev_drv_touch.type = LV_INDEV_TYPE_POINTER;
    indev_drv_touch.read_cb = touchpad_read;
    (void)lv_indev_drv_register(&indev_drv_touch);


    /* Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     * add objects to the group with `lv_group_add_obj(group, obj)`
     * and assign this input device to group to navigate in it:
     * `lv_indev_set_group(indev_encoder, group);` */
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Encoder
 * -----------------*/

/* Initialize your keypad */
static void encoder_init(void)
{
    /*Your code comes here*/
}


/* Will be called by the library to read the encoder */
static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    LV_UNUSED(indev_drv);

    static bool lastState;
    data->enc_diff = HAL::Encoder_GetDiff();
    bool isPush = HAL::Encoder_GetIsPush();
    data->state = isPush ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    if(isPush != lastState && HAL::Encoder_GetEnable())
    {
        HAL::Buzz_Tone(isPush ? 500 : 700, 20);
        lastState = isPush;
    }
}

/* Will be called by the library to read the touch screen */
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    LV_UNUSED(indev_drv);

    uint16_t x, y;
    static bool isPush = false;

    if(HAL::TouchPad_GetPoint(&x, &y))
    {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;

        // Serial.printf("CST836U: touch point: x=%d, y=%d\r\n", x, y);
        if(!isPush)
            HAL::Buzz_Tone(500, 20);
        isPush = true;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
        isPush = false;
    }
}

#else /* Enable this file at the top */

/* This dummy typedef exists purely to silence -Wpedantic. */
typedef int keep_pedantic_happy;
#endif
