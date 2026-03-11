#include "HAL/HAL.h"
#include "lvgl/lvgl.h"

#define BATT_ADC                    ADC1
#define BATT_MIN_VOLTAGE            3300
#define BATT_MAX_VOLTAGE            4200

#if CONFIG_POWER_BATT_CHG_DET_PULLUP
#  define BATT_CHG_DET_PIN_MODE     INPUT_PULLUP
#  define BATT_CHG_DET_STATUS       (!digitalRead(CONFIG_BAT_CHG_DET_PIN))
#else
#  define BATT_CHG_DET_PIN_MODE     INPUT_PULLDOWN
#  define BATT_CHG_DET_STATUS       ((usage == 100) ? false : digitalRead(CONFIG_BAT_CHG_DET_PIN))
#endif


// 定义查找表：电压必须从高到低排列
// 格式：{电压(mV), 电量(%)}
static const struct {
    uint16_t voltage;
    uint8_t soc;
} batt_curve[] = {
    {4150, 100},
    {4050, 95},
    {3950, 85},
    {3900, 70},
    {3800, 55},
    {3700, 40},
    {3600, 25},
    {3500, 15},
    {3400, 5},
    {3300, 0}
};

#define CURVE_POINTS (sizeof(batt_curve) / sizeof(batt_curve[0]))

typedef struct
{
    uint32_t LastHandleTime;
    uint16_t AutoLowPowerTimeout;
    bool AutoLowPowerEnable;
    bool ShutdownReq;
    uint16_t ADCValue;
    HAL::Power_CallbackFunction_t EventCallback;
} Power_t;

static Power_t Power;

static void Power_ADC_Init(adc_type* ADCx)
{
    adc_common_config_type adc_common_struct;
    adc_base_config_type adc_base_struct;

    crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, TRUE);

    adc_common_default_para_init(&adc_common_struct);
    adc_common_struct.combine_mode = ADC_INDEPENDENT_MODE;
    adc_common_struct.div = ADC_HCLK_DIV_16;
    adc_common_struct.common_dma_mode = ADC_COMMON_DMAMODE_DISABLE;
    adc_common_struct.common_dma_request_repeat_state = FALSE;
    adc_common_struct.sampling_interval = ADC_SAMPLING_INTERVAL_10CYCLES;
    adc_common_struct.tempervintrv_state = FALSE;
    adc_common_struct.vbat_state = FALSE;
    adc_common_config(&adc_common_struct);

    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.sequence_mode = FALSE;
    adc_base_struct.repeat_mode = FALSE;
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 1;
    adc_base_config(ADCx, &adc_base_struct);
    adc_resolution_set(ADCx, ADC_RESOLUTION_12B);

    adc_ordinary_conversion_trigger_set(ADCx, ADC_ORDINARY_TRIG_TMR1CH1, ADC_ORDINARY_TRIG_EDGE_NONE);

    adc_dma_mode_enable(ADCx, TRUE);
    adc_dma_request_repeat_enable(ADCx, FALSE);
    adc_interrupt_enable(ADCx, ADC_OCCO_INT, FALSE);

    adc_enable(ADCx, TRUE);
    while(adc_flag_get(ADCx, ADC_RDY_FLAG) == RESET);

    /* adc calibration */
    adc_calibration_init(ADCx);
    while(adc_calibration_init_status_get(ADCx));
    adc_calibration_start(ADCx);
    while(adc_calibration_status_get(ADCx));
}

static uint16_t Power_ADC_GetValue()
{
    uint16_t retval = 0;
    if(adc_flag_get(BATT_ADC, ADC_OCCE_FLAG))
    {
        retval = adc_ordinary_conversion_data_get(BATT_ADC);
    }
    return retval;
}

static void Power_ADC_Update()
{
    static bool isStartConv = false;

    if(!isStartConv)
    {
        adc_ordinary_channel_set(
            BATT_ADC,
            (adc_channel_select_type)PIN_MAP[CONFIG_BAT_DET_PIN].ADC_Channel,
            1,
            ADC_SAMPLETIME_247_5
        );

        adc_ordinary_software_trigger_enable(BATT_ADC, TRUE);
        isStartConv = true;
    }
    else
    {
        Power.ADCValue = Power_ADC_GetValue();
        isStartConv = false;
    }
}

static uint8_t Get_SOC_From_Voltage(int voltage_mv) {
    // 1. 边界处理
    if (voltage_mv >= batt_curve[0].voltage) return 100;
    if (voltage_mv <= batt_curve[CURVE_POINTS - 1].voltage) return 0;

    // 2. 查找区间
    for (int i = 0; i < CURVE_POINTS - 1; i++) {
        if (voltage_mv >= batt_curve[i+1].voltage && voltage_mv < batt_curve[i].voltage) {
            // 3. 线性插值计算
            // y = y0 + (x - x0) * (y1 - y0) / (x1 - x0)
            // 注意：因为数组是降序，x1 < x0，所以分母是负数，或者交换顺序计算
            
            uint16_t v_high = batt_curve[i].voltage;
            uint16_t v_low  = batt_curve[i+1].voltage;
            uint8_t  s_high = batt_curve[i].soc;
            uint8_t  s_low  = batt_curve[i+1].soc;

            // 防止除以零
            if (v_high == v_low) return s_high;

            // 计算比例 (0.0 ~ 1.0)，使用整数运算避免浮点
            // ratio = (v_high - current_v) / (v_high - v_low)
            int32_t delta_v_total = v_high - v_low;
            int32_t delta_v_curr  = v_high - voltage_mv;
            
            // 计算当前电量 = 高电量 - (比例 * 电量差)
            int32_t soc_delta = ((s_high - s_low) * delta_v_curr) / delta_v_total;
            
            return s_high - soc_delta;
        }
    }
    return 0; // 默认
}

void HAL::Power_Init()
{
    memset(&Power, 0, sizeof(Power));
    Power.AutoLowPowerTimeout = 60;

    Serial.printf("Power: Waiting[%dms]...\r\n", CONFIG_POWER_WAIT_TIME);
    pinMode(CONFIG_POWER_EN_PIN, OUTPUT);
    digitalWrite(CONFIG_POWER_EN_PIN, LOW);
    delay(CONFIG_POWER_WAIT_TIME);
    digitalWrite(CONFIG_POWER_EN_PIN, HIGH);
    Serial.println("Power: ON");

    Power_ADC_Init(BATT_ADC);
    pinMode(CONFIG_BAT_DET_PIN, INPUT_ANALOG);
    pinMode(CONFIG_BAT_CHG_DET_PIN, BATT_CHG_DET_PIN_MODE);

//    Power_SetAutoLowPowerTimeout(5 * 60);
//    Power_HandleTimeUpdate();
    Power_SetAutoLowPowerEnable(false);
}

void HAL::Power_HandleTimeUpdate()
{
    Power.LastHandleTime = millis();
}

void HAL::Power_SetAutoLowPowerTimeout(uint16_t sec)
{
    Power.AutoLowPowerTimeout = sec;
}

uint16_t HAL::Power_GetAutoLowPowerTimeout()
{
    return Power.AutoLowPowerTimeout;
}

void HAL::Power_SetAutoLowPowerEnable(bool en)
{
    Power.AutoLowPowerEnable = en;
    Power_HandleTimeUpdate();
}

void HAL::Power_Shutdown()
{
    CM_EXECUTE_ONCE(Power.ShutdownReq = true);
    CM_EXECUTE_ONCE(Audio_PlayMusic("Shutdown"));
    HAL::Encoder_SetEnable(false);
}

void HAL::Power_Update()
{
    CM_EXECUTE_INTERVAL(Power_ADC_Update(), 1000);

    if(!Power.AutoLowPowerEnable)
        return;

    if(Power.AutoLowPowerTimeout == 0)
        return;

    if(millis() - Power.LastHandleTime >= (Power.AutoLowPowerTimeout * 1000))
    {
        Power_Shutdown();
    }
}

void HAL::Power_EventMonitor()
{
    if(Power.ShutdownReq)
    {
        LED_Red_On();
        if(Power.EventCallback)
        {
            Power.EventCallback();
        }
        Backlight_SetGradual(0, 500);
        while(Audio_IsPlaying() || Backlight_IsGradualBusy())
        {
            lv_task_handler();
        }
        Serial.println("Power: OFF");
        digitalWrite(CONFIG_POWER_EN_PIN, LOW);
        Power.ShutdownReq = false;
    }
}

void HAL::Power_GetInfo(Power_Info_t* info)
{

    int voltage = map(
                      Power.ADCValue,
                      0, 4095,
                      0, 3300
                  );

    voltage *= 2;

    CM_VALUE_LIMIT(voltage, BATT_MIN_VOLTAGE, BATT_MAX_VOLTAGE);

    int usage = Get_SOC_From_Voltage(voltage);

    CM_VALUE_LIMIT(usage, 0, 100);

    info->usage = usage;
    info->isCharging = BATT_CHG_DET_STATUS;
    info->voltage = voltage;
}

void HAL::Power_SetEventCallback(Power_CallbackFunction_t callback)
{
    Power.EventCallback = callback;
}
