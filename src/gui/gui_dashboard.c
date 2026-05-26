#include "gui/gui_dashboard.h"
/* Standard includes. */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

/* Hardware includes. */
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "drivers/rtos_hw_drivers.h"
#include "utils/uartstdio.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/flash.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib.h"
#include "widget.h"
#include "canvas.h"
#include "checkbox.h"
#include "container.h"
#include "pushbutton.h"
#include "radiobutton.h"
#include "slider.h"
#include "utils/ustdlib.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"
#include "drivers/images.h"
#include "motor/motor_control.h"
#include "sensors/sensor_manager.h"
#include "motor/motor_state_machine.h"
#include "common/project_config.h"

tContext sContext;
uint32_t g_ui32Panel;
uint32_t g_ui32LastPanel;
TaskHandle_t xDisplayTaskHandle = NULL; 
#define PLOT_SIZE 200
#define DISPLAY_TASK_STACK_WORDS 768U
uint32_t graphX = 1;
uint32_t graphY[6];
uint32_t lastGraphY[6];
bool plotToggle = false;
bool faultLatched = false;
uint32_t g_thresholds[4] = {250, 250, 250, 25};
uint32_t g_temp_thresholds[4] = {250, 250, 250, 25};
uint32_t g_max_values[6] = {500, 500, 1000, 50, 100, 500};
volatile uint32_t g_elapsed_seconds = 40500;
char clockBuf[20];
char plotBuf[20];
char configBuf[20];
char configBuf2[20];
char configBuf3[20];


static void prvGUITask( void *pvParameters );
void OnPrevious(tWidget *psWidget);
void OnNext(tWidget *psWidget);
void OnIntroPaint(tWidget *psWidget, tContext *psContext);
void OnCanvasPaint(tWidget *psWidget, tContext *psContext);
void OnSensorPlotsPaint(tWidget *psWidget, tContext *psContext);
void OnConfigurationPaint(tWidget *psWidget, tContext *psContext);
void OnStartStopButtonPress(tWidget *psWidget);
void OnSliderChange(tWidget *psWidget, int32_t i32Value);
void UpdateBanner();
void ClockUpdate();
void OnSensorButtonPress(tWidget *psWidget);
void OnFaultAcknowledgeButtonPress(tWidget *psWidget);
void OnConfigButtonPress(tWidget *psWidget);
void OnThresholdButtonPress(tWidget *psWidget);
extern tCanvasWidget g_psPanels[];
uint32_t g_ui32CurrentPlot = 0;
uint32_t g_ui32LastPlot = 6;
uint32_t g_ui32CurrentConfig = 0;
uint32_t g_ui32LastConfig = 0;
EvMotorState_t g_currentState = EV_MOTOR_STATE_IDLE;
EvMotorState_t g_lastState = EV_MOTOR_STATE_IDLE;
volatile bool configUpdateRequest = false;
volatile bool clockUpdateRequest = true;
bool nightConditions = false;
TickType_t g_lastTimerTicks;
static uint32_t g_lastUserSpeed = 250U;

typedef enum
{
    GUI_REQ_NONE             = 0x00,
    GUI_REQ_PREV_PANEL       = 0x01,
    GUI_REQ_NEXT_PANEL       = 0x02,
    GUI_REQ_START_STOP       = 0x04,
    GUI_REQ_FAULT_ACK        = 0x08,
    GUI_REQ_APPLY_THRESHOLDS = 0x10,
    GUI_REQ_CONFIG_TAB       = 0x20,
    GUI_REQ_PLOT_BUTTON      = 0x40
} GuiRequest_t;

static volatile uint32_t g_ui32GuiRequests = GUI_REQ_NONE;
static uint32_t g_ui32PendingConfig = 0;
static uint32_t g_ui32PendingPlot = 0;

static void RequestUiWork(uint32_t request)
{
    g_ui32GuiRequests |= request;

    if (xDisplayTaskHandle != NULL)
    {
        xTaskNotifyGive(xDisplayTaskHandle);
    }
}

//for testing
enum MotorStates {
    IDLE,
    STARTING,
    RUNNING,
    ESTOP,
    FAULT
} MotorState = IDLE;


tPushButtonWidget g_psFaultAcknowledgeButton[] = 
{
    RectangularButtonStruct(g_psPanels, 0, 0,
                            &g_sKentec320x240x16_SSD2119, 60, 100, 200, 40,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrYellow, ClrYellow, 0, ClrBlack,
                            &g_sFontCm22, "ACKNOWLEDGE", 0, 0, 0, 0, OnFaultAcknowledgeButtonPress),
};

tPushButtonWidget g_psStartStopButton[] = {
    RectangularButtonStruct(g_psPanels, 0, 0,
                            &g_sKentec320x240x16_SSD2119, 90, 100, 140, 40,
                            PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrLightGreen, 0, 0, ClrBlack,
                            &g_sFontCm22, "Start Motor", 0, 0, 0, 0, OnStartStopButtonPress),
};

Canvas(g_sIntroduction, g_psPanels, g_psStartStopButton, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 216, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);

tPushButtonWidget g_psClock[] = {
    RectangularButtonStruct(g_psPanels + 1, 0, 0,
                        &g_sKentec320x240x16_SSD2119, 0, 220, 120, 20,
                        PB_STYLE_FILL | PB_STYLE_TEXT,
                        0, 0, 0, ClrWhite,
                        &g_sFontCm18, "27/05/26 11:15:00", 0, 0, 0, 0, 0),
};

tSliderWidget g_psSliders[] = {
    SliderStruct(g_psPanels + 1, 0, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 129, 140, 18, 0, 1000, 250,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm16, "250", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 1, 0, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 149, 140, 18, 0, 2500, 250,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm16, "250", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 1, 0, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 149, 140, 18, 0, 1000, 250,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm16, "250", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 1, 0, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 149, 140, 18, 0, 50, 25,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm16, "250", 0, 0, OnSliderChange),
};
#define NUM_SLIDERS (sizeof(g_psSliders) / sizeof(g_psSliders[0]))

tPushButtonWidget g_psThresholdButton[] = {
    RectangularButtonStruct(g_psPanels + 1, 0, 0,
                        &g_sKentec320x240x16_SSD2119, 100, 200, 120, 33,
                        PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                        ClrGray, ClrMidnightBlue, ClrDarkGray, ClrWhite,
                        &g_sFontFixed6x8, "Update System", 0, 0, 0, 0, OnThresholdButtonPress),
};

tPushButtonWidget g_psConfigButtons[] = {
    RectangularButtonStruct(g_psPanels + 1, g_psConfigButtons + 1, 0,
                            &g_sKentec320x240x16_SSD2119, 0, 23, 107, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrAqua, ClrMidnightBlue, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "Motor", 0, 0, 0, 0, OnConfigButtonPress),
    RectangularButtonStruct(g_psPanels + 1, g_psConfigButtons + 2, 0,
                            &g_sKentec320x240x16_SSD2119, 107, 23, 107, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrMidnightBlue, ClrMidnightBlue, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "VL53L0X", 0, 0, 0, 0, OnConfigButtonPress),
    RectangularButtonStruct(g_psPanels + 1, 0, 0,
                            &g_sKentec320x240x16_SSD2119, 213, 23, 107, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrMidnightBlue, ClrMidnightBlue, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "SHT31", 0, 0, 0, 0, OnConfigButtonPress),
};
#define NUM_CONFIG_BUTTONS (sizeof(g_psConfigButtons) / sizeof(g_psConfigButtons[0]))

tPushButtonWidget g_psPlotButtons[] = {
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 1, 0,
                            &g_sKentec320x240x16_SSD2119, 0, 23, 53, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrMidnightBlue, 0, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "M.Speed", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 2, 0,
                            &g_sKentec320x240x16_SSD2119, 53, 23, 53, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrMidnightBlue, 0, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "M.Power", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 3, 0,
                            &g_sKentec320x240x16_SSD2119, 106, 23, 53, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrMidnightBlue, 0, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "Light", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 4, 0,
                            &g_sKentec320x240x16_SSD2119, 159, 23, 53, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrMidnightBlue, 0, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "Temp.", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 5, 0,
                            &g_sKentec320x240x16_SSD2119, 212, 23, 53, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrMidnightBlue, 0, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "Humi.", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, 0, 0,
                            &g_sKentec320x240x16_SSD2119, 265, 23, 55, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY,
                            ClrMidnightBlue, 0, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "Dist.", 0, 0, 0, 0, OnSensorButtonPress),
};

#define NUM_PLOT_BUTTONS (sizeof(g_psPlotButtons) / sizeof(g_psPlotButtons[0]))

Canvas(g_sCanvas3, g_psPanels + 2, g_psPlotButtons, 0, &g_sKentec320x240x16_SSD2119, 60,
       62, 200, 140, CANVAS_STYLE_OUTLINE | CANVAS_STYLE_APP_DRAWN, 0, ClrGray,
       0, 0, 0, 0, OnCanvasPaint);

Canvas(g_sSensorPlots, g_psPanels + 2, &g_sCanvas3, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 216, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnSensorPlotsPaint);

Canvas(g_sConfiguration, g_psPanels + 1, g_psConfigButtons, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 216, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnConfigurationPaint);

tCanvasWidget g_psPanels[] = {
    CanvasStruct(0, 0, &g_sIntroduction, &g_sKentec320x240x16_SSD2119, 0, 24,
                 320, 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sConfiguration, &g_sKentec320x240x16_SSD2119, 0, 24,
                 320, 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sSensorPlots, &g_sKentec320x240x16_SSD2119, 0, 24, 320,
                 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};

#define NUM_PANELS (sizeof(g_psPanels) / sizeof(g_psPanels[0]))

char *g_pcPanei32Names[] =
{
    "     Motor Control     ",
    "     System Configuration     ",
    "     Sensor Data     ",
};

RectangularButton(g_sPrevious, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 190,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY, ClrBlack, ClrBlack, 0, ClrSilver,
                  &g_sFontCm20, "-", g_pui8Blue50x50, g_pui8Blue50x50Press, 0, 0,
                  OnPrevious);
RectangularButton(g_sNext, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 270, 190,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY, ClrBlack, ClrBlack, 0,
                  ClrSilver, &g_sFontCm20, "+", g_pui8Blue50x50,
                  g_pui8Blue50x50Press, 0, 0, OnNext);

static void HandlePreviousPanel(void) {
    if(MotorState == ESTOP) return;
    if(g_ui32Panel == 0) {
        return;
    }

    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    g_ui32Panel--;
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));
    // UpdateBanner();

    if(g_ui32Panel == 0)
    {
        PushButtonImageOff(&g_sPrevious);
        PushButtonTextOff(&g_sPrevious);
        PushButtonFillOn(&g_sPrevious);
    }

    if(g_ui32Panel == (NUM_PANELS - 2)) {
        PushButtonImageOn(&g_sNext);
        PushButtonTextOn(&g_sNext);
        PushButtonFillOff(&g_sNext);
    }
    WidgetPaint((tWidget *)&g_sPrevious);
    WidgetPaint((tWidget *)&g_sNext);
}

void OnPrevious(tWidget *psWidget) {
    (void)psWidget;
    RequestUiWork(GUI_REQ_PREV_PANEL);
}

static void HandleNextPanel(void) {
    if(MotorState == ESTOP) return;
    if(g_ui32Panel == (NUM_PANELS - 1)) {
        return;
    }

    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    g_ui32Panel++;
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));
    // UpdateBanner();

    if(g_ui32Panel == 1) {
        PushButtonImageOn(&g_sPrevious);
        PushButtonTextOn(&g_sPrevious);
        PushButtonFillOff(&g_sPrevious);
    }

    if(g_ui32Panel == (NUM_PANELS - 1)) {
        PushButtonImageOff(&g_sNext);
        PushButtonTextOff(&g_sNext);
        PushButtonFillOn(&g_sNext);
    }
    
    WidgetPaint((tWidget *)&g_sPrevious);
    WidgetPaint((tWidget *)&g_sNext);
}

void OnNext(tWidget *psWidget) {
    (void)psWidget;
    RequestUiWork(GUI_REQ_NEXT_PANEL);
}

void OnIntroPaint(tWidget *psWidget, tContext *psContext) {
    EvMotorState_t motorState = MotorControl_GetState();
    if(motorState == EV_MOTOR_STATE_ESTOP_BRAKING || motorState == EV_MOTOR_STATE_FAULT_LATCHED) {
        GrContextFontSet(psContext, &g_sFontCm18);
        GrContextForegroundSet(psContext, ClrSilver);
        GrStringDrawCentered(psContext, "An Emergency Stop Has Been Triggered.", -1, 160, 32, 0);
        GrStringDrawCentered(psContext, "You must acknowledge the fault before", -1, 160, 55, 0);
        GrStringDrawCentered(psContext, "starting the motor.", -1, 160, 75, 0);
        return;
    }
    ClockUpdate();

}

void UpdatePlotAxes() {
    if (g_ui32CurrentPlot >= NUM_PLOT_BUTTONS)
    {
        return;
    }

    tRectangle limitRect = {260, 58, 320, 72};
    tRectangle labelRect = {260, 130, 320, 140};
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &limitRect);
    GrRectFill(&sContext, &labelRect);

    usprintf(plotBuf, "%d", g_max_values[g_ui32CurrentPlot]);
    GrContextFontSet(&sContext, &g_sFontCm16);
    GrContextForegroundSet(&sContext, ClrSilver);
    GrStringDrawCentered(&sContext, plotBuf, -1, 273, 62, 0);
    GrStringDrawCentered(&sContext, "0", -1, 265, 200, 0);
    GrStringDrawCentered(&sContext, "0", -1, 60, 210, 0);
    GrStringDrawCentered(&sContext, "10", -1, 260, 210, 0);

    GrContextFontSet(&sContext, &g_sFontFixed6x8);
    GrStringDrawCentered(&sContext, "Time (s)", -1, 160, 210, 0);
    switch(g_ui32CurrentPlot) {
        case 0:
            GrStringDraw(&sContext, "Speed(rpm)", -1, 260, 130, 0);
            break;
        case 1:
            GrStringDraw(&sContext, "Power(W)", -1, 260, 130, 0);
            break;
        case 2:
            GrStringDraw(&sContext, "Light(lux)", -1, 260, 130, 0);
            break;
        case 3:
            GrStringDraw(&sContext, "Temp.(C)", -1, 260, 130, 0);
            break;
        case 4:
            GrStringDraw(&sContext, "Humi.(%)", -1, 260, 130, 0);
            break;
        case 5:
            GrStringDraw(&sContext, "Dist.(mm)", -1, 260, 130, 0);
            break;
    }

    graphX = 1;
    tRectangle clearRect = {61, 63, 258, 200};
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &clearRect);
}

void OnSensorPlotsPaint(tWidget *psWidget, tContext *psContext) {
    UpdatePlotAxes();
}

void UpdateConfigPage(bool newOpen) {
    if(g_ui32Panel != 1) return;
    EvSensorData_t sensorData = SensorManager_GetFilteredData();
    GrContextFontSet(&sContext, &g_sFontCm16);
    if(g_ui32LastConfig != g_ui32CurrentConfig) {
        WidgetRemove((tWidget *)(g_psSliders));
        WidgetRemove((tWidget *)(g_psSliders + 1));
        WidgetRemove((tWidget *)(g_psSliders + 2));
        WidgetRemove((tWidget *)(g_psSliders + 3));
        tRectangle configClearRect = {0, 58, 320, 190};
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &configClearRect);
        WidgetAdd((tWidget *)(g_psPanels + 1), (tWidget *)g_psThresholdButton);
        WidgetPaint((tWidget *)g_psThresholdButton);
    }

    if(g_ui32CurrentConfig == 0) {
        if(g_ui32LastConfig != 0 || newOpen) {
            GrContextForegroundSet(&sContext, ClrWhite);
            GrStringDraw(&sContext, "Motor Speed: ", -1, 5, 62, 0);
            GrStringDraw(&sContext, "Motor State: ", -1, 5, 77, 0);
            GrStringDraw(&sContext, "Desired Speed", -1, 5, 130, 0);
            GrStringDraw(&sContext, "Power Threshold", -1, 5, 150, 0);
            WidgetAdd((tWidget *)(g_psPanels + 1), (tWidget *)g_psSliders);
            usprintf(configBuf2, "%d", g_thresholds[0]);
            SliderTextSet((tSliderWidget *)(g_psSliders), configBuf2);
            SliderValueSet((tSliderWidget *)(g_psSliders), g_thresholds[0]);
            WidgetAdd((tWidget *)(g_psPanels + 1), (tWidget *)(g_psSliders + 1));
            usprintf(configBuf3, "%d", g_thresholds[1]);
            SliderTextSet((tSliderWidget *)(g_psSliders + 1), configBuf3);
            SliderValueSet((tSliderWidget *)(g_psSliders + 1), g_thresholds[1]);
            g_ui32LastConfig = g_ui32CurrentConfig;
        }
        WidgetPaint((tWidget *)g_psSliders);
        WidgetPaint((tWidget *)(g_psSliders + 1)); 
        tRectangle configClearRect1 = {100, 62, 160, 78};
        tRectangle configClearRect2 = {100, 77, 200, 93};
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &configClearRect1);
        GrRectFill(&sContext, &configClearRect2);
        GrContextForegroundSet(&sContext, ClrWhite);
        usprintf(configBuf, "%drpm", sensorData.motor_speed_rpm);
        GrStringDraw(&sContext, configBuf, -1, 100, 62, 0);
        switch (g_currentState) {
            case EV_MOTOR_STATE_IDLE:
                GrStringDraw(&sContext, "Idle", -1, 100, 77, 0);
                break;
            case EV_MOTOR_STATE_STARTING: 
                GrStringDraw(&sContext, "Starting", -1, 100, 77, 0);
                break;
            case EV_MOTOR_STATE_RUNNING:
                GrStringDraw(&sContext, "Running", -1, 100, 77, 0);
                break;
            case EV_MOTOR_STATE_ESTOP_BRAKING:
                GrStringDraw(&sContext, "EStop Braking", -1, 100, 77, 0);
                break;
            case EV_MOTOR_STATE_FAULT_LATCHED:
                GrStringDraw(&sContext, "Fault Latched", -1, 100, 77, 0);
                break;
        }
    } 
    else if(g_ui32CurrentConfig == 1) {
        if(g_ui32LastConfig != 1 || newOpen) {
            GrContextForegroundSet(&sContext, ClrWhite);
            GrStringDraw(&sContext, "Nearest Object: ", -1, 5, 62, 0);
            GrStringDraw(&sContext, "Distance Threshold", -1, 5, 150, 0);
            WidgetAdd((tWidget *)(g_psPanels + 1), (tWidget *)(g_psSliders + 2));
            usprintf(configBuf2, "%d", g_thresholds[2]);
            SliderTextSet((tSliderWidget *)(g_psSliders + 2), configBuf2);
            SliderValueSet((tSliderWidget *)(g_psSliders + 2), g_thresholds[2]);
            g_ui32LastConfig = g_ui32CurrentConfig;
        }
        WidgetPaint((tWidget *)(g_psSliders + 2));
        tRectangle configClearRect1 = {110, 62, 165, 75};
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &configClearRect1);
        GrContextForegroundSet(&sContext, ClrWhite);
        usprintf(configBuf, "%dmm", sensorData.distance_mm);
        GrStringDraw(&sContext, configBuf, -1, 115, 62, 0);
    } 
    else if(g_ui32CurrentConfig == 2) {
        if(g_ui32LastConfig != 2 || newOpen) {
            GrContextForegroundSet(&sContext, ClrWhite);
            GrStringDraw(&sContext, "Current Temperature: ", -1, 5, 62, 0);
            GrStringDraw(&sContext, "Relative Humidity: ", -1, 5, 77, 0);
            GrStringDraw(&sContext, "Cooling: ", -1, 5, 92, 0);
            GrStringDraw(&sContext, "Cooling Threshold", -1, 5, 150, 0);
            WidgetAdd((tWidget *)(g_psPanels + 1), (tWidget *)(g_psSliders + 3));
            usprintf(configBuf2, "%d", g_thresholds[3]);
            SliderTextSet((tSliderWidget *)(g_psSliders + 3), configBuf2);
            SliderValueSet((tSliderWidget *)(g_psSliders + 3), g_thresholds[3]);
            g_ui32LastConfig = g_ui32CurrentConfig;
        }
        WidgetPaint((tWidget *)(g_psSliders + 3));
        tRectangle configClearRect1 = {160, 62, 190, 75};
        tRectangle configClearRect2 = {140, 77, 170, 90};
        tRectangle configClearRect3 = {63, 92, 85, 105};
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &configClearRect1);
        GrRectFill(&sContext, &configClearRect2);
        GrRectFill(&sContext, &configClearRect3);
        GrContextForegroundSet(&sContext, ClrWhite);
        usprintf(configBuf, "%dC", sensorData.cabin_temperature_c_x10);
        GrStringDraw(&sContext, configBuf, -1, 160, 62, 0);
        usprintf(configBuf, "%d%%", sensorData.cabin_humidity_rh_x10);
        GrStringDraw(&sContext, configBuf, -1, 140, 77, 0);
        GrStringDraw(&sContext, sensorData.cabin_temperature_c_x10 >= (int16_t)g_thresholds[3] ? "On" : "Off", -1, 63, 92, 0);
    }
}

static void HandleThresholdButtonPress(void) {
    if(g_ui32CurrentConfig == 0) {
        g_thresholds[0] = g_temp_thresholds[0];
        g_thresholds[1] = g_temp_thresholds[1];
        g_lastUserSpeed = g_thresholds[0];    /* save the user speed */
        MotorControl_SetSpeed(g_lastUserSpeed);
    }
    else if(g_ui32CurrentConfig == 1) {
        g_thresholds[2] = g_temp_thresholds[2];
    }
    else if(g_ui32CurrentConfig == 2) {
        g_thresholds[3] = g_temp_thresholds[3];
    }

    EvSensorThresholds_t thresholds = SensorManager_GetThresholds();
    thresholds.max_motor_power_w = g_thresholds[1];
    thresholds.min_distance_mm = (uint16_t)g_thresholds[2];
    thresholds.cooling_threshold_c_x10 = (int16_t)g_thresholds[3];
    SensorManager_SetThresholds(&thresholds);

    configUpdateRequest = true;
}

void OnThresholdButtonPress(tWidget *psWidget){
    (void)psWidget;
    RequestUiWork(GUI_REQ_APPLY_THRESHOLDS);
}

void OnConfigurationPaint(tWidget *psWidget, tContext *psContext) {
    UpdateConfigPage(true);
}

static void HandleConfigButtonPress(uint32_t ui32Idx) {
    if(ui32Idx >= NUM_CONFIG_BUTTONS || ui32Idx == g_ui32CurrentConfig) {
        return;
    }

    PushButtonFillColorSet((tPushButtonWidget *)(g_psConfigButtons + ui32Idx), ClrAqua);
    WidgetPaint((tWidget *)(g_psConfigButtons + ui32Idx));
    PushButtonFillColorSet((tPushButtonWidget *)(g_psConfigButtons + g_ui32CurrentConfig), ClrMidnightBlue);
    WidgetPaint((tWidget *)(g_psConfigButtons + g_ui32CurrentConfig));

    g_ui32CurrentConfig = ui32Idx;
}

void OnConfigButtonPress(tWidget *psWidget) {
    uint32_t ui32Idx;
    for(ui32Idx = 0; ui32Idx < NUM_CONFIG_BUTTONS; ui32Idx++) {
        if(psWidget == (tWidget *)(g_psConfigButtons + ui32Idx)) {
            break;
        }
    }

    if(ui32Idx == NUM_CONFIG_BUTTONS) {
        return;
    }

    g_ui32PendingConfig = ui32Idx;
    RequestUiWork(GUI_REQ_CONFIG_TAB);
}

void OnFaultLatched(void) {
    faultLatched = true;
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    g_ui32Panel = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);
    WidgetPaint((tWidget *)g_psPanels);
    WidgetRemove((tWidget *)&g_psStartStopButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    WidgetAdd((tWidget *)g_psPanels, (tWidget *)&g_psFaultAcknowledgeButton);
    WidgetPaint((tWidget *)&g_psFaultAcknowledgeButton);
    PushButtonImageOn(&g_sNext);
    PushButtonTextOn(&g_sNext);
    PushButtonFillOff(&g_sNext);
    WidgetPaint((tWidget *)&g_sNext);
    PushButtonImageOff(&g_sPrevious);
    PushButtonTextOff(&g_sPrevious);
    PushButtonFillOn(&g_sPrevious);
    WidgetPaint((tWidget *)&g_sPrevious);
    UpdateBanner();
}

static void HandleFaultAcknowledge(void) {
    faultLatched = false;
    MotorStateMachine_AcknowledgeFault();
    MotorControl_SetSpeed(g_lastUserSpeed);    /* restore previous speed */
    MotorControl_Start();
    MotorState = RUNNING;
    PushButtonTextSet((tPushButtonWidget *)&g_psStartStopButton, "Stop Motor");
    PushButtonFillColorSet((tPushButtonWidget *)&g_psStartStopButton, ClrRedMask);
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);
    WidgetAdd((tWidget *)g_psPanels, (tWidget *)&g_psStartStopButton);
    WidgetPaint((tWidget *)g_psPanels);
    WidgetRemove((tWidget *)&g_psFaultAcknowledgeButton);
    WidgetPaint((tWidget *)&g_psStartStopButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    PushButtonImageOn(&g_sNext);
    PushButtonTextOn(&g_sNext);
    PushButtonFillOff(&g_sNext);
    WidgetPaint((tWidget *)&g_sNext);
    UpdateBanner();
}

void OnFaultAcknowledgeButtonPress(tWidget *psWidget) {
    (void)psWidget;
    RequestUiWork(GUI_REQ_FAULT_ACK);
}

void SensorDataToArray() {
    EvSensorData_t filtered = SensorManager_GetFilteredData();
    graphY[0] = filtered.motor_speed_rpm;
    graphY[1] = filtered.motor_power_w;
    graphY[2] = filtered.ambient_light_lux;
    graphY[3] = filtered.cabin_temperature_c_x10;
    graphY[4] = filtered.cabin_humidity_rh_x10;
    graphY[5] = filtered.distance_mm;
    
    // for testing without sensor data
    // uint32_t rand_val = urand() % 500;
    // uint32_t add_val = rand_val % 3 ? rand_val % 10 : -(rand_val % 10);
    // graphY[0] = (lastGraphY[0] + add_val) % 500;


}

void OnCanvasPaint(tWidget *psWidget, tContext *psContext) {
    if(MotorState == ESTOP) return;
    SensorDataToArray();

    // define and draw the scale
    uint32_t xMax = PLOT_SIZE;
    uint32_t yMax = g_max_values[g_ui32CurrentPlot];
    uint32_t canvas_xMin = 60;
    uint32_t canvas_xRange = 200;
    uint32_t canvas_yMin = 62;
    uint32_t canvas_yRange = 140;

    if (!plotToggle || g_ui32Panel != 2) {
        return;
    }

    float display_y = graphY[g_ui32CurrentPlot];
    display_y = canvas_yMin + canvas_yRange - ((display_y + 1)/yMax)*canvas_yRange;
    float display_last_y = lastGraphY[g_ui32CurrentPlot];
    display_last_y = canvas_yMin + canvas_yRange - ((display_last_y + 1)/yMax)*canvas_yRange;
    float display_x = graphX;
    display_x = (display_x/xMax)*canvas_xRange + canvas_xMin;
    float display_x2 = graphX - 1;
    display_x2 = (display_x2/xMax)*canvas_xRange + canvas_xMin;

    GrContextForegroundSet(psContext, ClrBlue);
    GrLineDraw(psContext, display_x2, display_last_y, display_x, display_y);

    graphX++;

    if(display_x >= canvas_xMin + canvas_xRange) {
        graphX = 1;
        tRectangle clearRect = {canvas_xMin + 1, canvas_yMin + 1, canvas_xMin + canvas_xRange - 2, canvas_yMin + canvas_yRange - 2};
        GrContextForegroundSet(psContext, ClrBlack);
        GrRectFill(psContext, &clearRect);
    }

    lastGraphY[g_ui32CurrentPlot] = graphY[g_ui32CurrentPlot];
}

static void HandleStartStopButton(void) {
    EvMotorState_t motorState = MotorControl_GetState();
    if (motorState == EV_MOTOR_STATE_IDLE || MotorState == IDLE) {
        PushButtonTextSet((tPushButtonWidget *)&g_psStartStopButton, "Stop Motor");
        PushButtonFillColorSet((tPushButtonWidget *)&g_psStartStopButton, ClrRedMask);
        MotorControl_SetSpeed(g_lastUserSpeed);
        MotorControl_Start();
        MotorState = RUNNING;
    } else {
        PushButtonTextSet((tPushButtonWidget *)&g_psStartStopButton, "Start Motor");
        PushButtonFillColorSet((tPushButtonWidget *)&g_psStartStopButton, ClrLightGreen);
        MotorControl_Stop();
        MotorState = IDLE;
    }
    WidgetPaint((tWidget *)&g_psStartStopButton);
}

void OnStartStopButtonPress(tWidget *psWidget) {
    (void)psWidget;
    RequestUiWork(GUI_REQ_START_STOP);
}

static void HandleSensorButtonPress(uint32_t ui32Idx) {
    if(ui32Idx >= NUM_PLOT_BUTTONS) {
        return;
    }

    if (plotToggle && g_ui32CurrentPlot < NUM_PLOT_BUTTONS) {
        PushButtonFillColorSet((tPushButtonWidget *)(g_psPlotButtons + g_ui32CurrentPlot), ClrMidnightBlue);
        WidgetPaint((tWidget *)(g_psPlotButtons + g_ui32CurrentPlot));
    }

    graphX = 1;
    tRectangle clearRect = {61, 63, 258, 200};
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &clearRect);

    if(g_ui32CurrentPlot == ui32Idx && plotToggle) {
        plotToggle = !plotToggle;
        // return;
    } else {
        PushButtonFillColorSet((tPushButtonWidget *)(g_psPlotButtons + ui32Idx), ClrAqua);
        WidgetPaint((tWidget *)(g_psPlotButtons + ui32Idx));
        plotToggle = true;
    }

    g_ui32CurrentPlot = ui32Idx;
    UpdatePlotAxes();
}

void OnSensorButtonPress(tWidget *psWidget) {
    uint32_t ui32Idx;
    for(ui32Idx = 0; ui32Idx < NUM_PLOT_BUTTONS; ui32Idx++) {
        if(psWidget == (tWidget *)(g_psPlotButtons + ui32Idx)) {
            break;
        }
    }

    if(ui32Idx == NUM_PLOT_BUTTONS) {
        return;
    }

    g_ui32PendingPlot = ui32Idx;
    RequestUiWork(GUI_REQ_PLOT_BUTTON);
}

void UpdateBanner() {
    tRectangle sRect;
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 23;

    EvMotorState_t motorState = MotorControl_GetState();

    switch (motorState) {
        case EV_MOTOR_STATE_IDLE:
            GrContextForegroundSet(&sContext, ClrGold);
            GrRectFill(&sContext, &sRect);
            break;
        case EV_MOTOR_STATE_STARTING: 
            GrContextForegroundSet(&sContext, ClrGold);
            GrRectFill(&sContext, &sRect);
            break;
        case EV_MOTOR_STATE_RUNNING:
            GrContextForegroundSet(&sContext, ClrGreen);
            GrRectFill(&sContext, &sRect);
            break;
        case EV_MOTOR_STATE_ESTOP_BRAKING:
            GrContextForegroundSet(&sContext, ClrRedMask);
            GrRectFill(&sContext, &sRect);
            break;
        case EV_MOTOR_STATE_FAULT_LATCHED:
            GrContextForegroundSet(&sContext, ClrRedMask);
            GrRectFill(&sContext, &sRect);
            break;  
    }

    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);
    GrContextFontSet(&sContext, &g_sFontCmsc26);
    GrStringDrawCentered(&sContext, g_pcPanei32Names[g_ui32Panel], -1,
                         GrContextDpyWidthGet(&sContext) / 2, 10, 0);
}

void OnSliderChange(tWidget *psWidget, int32_t i32Value) {
    static char pcSliderText[4][5];
    uint32_t ui32Idx;
    for(ui32Idx = 0; ui32Idx < NUM_SLIDERS; ui32Idx++) {
        if(psWidget == (tWidget *)(g_psSliders + ui32Idx)) {
            break;
        }
    }
    if(ui32Idx == NUM_SLIDERS) return;
    g_temp_thresholds[ui32Idx] = i32Value;
    usprintf(pcSliderText[ui32Idx], "%3d", i32Value);
    SliderTextSet((tSliderWidget *)psWidget, pcSliderText[ui32Idx]);

    /* Btn/slider cbs just mark state. LCD drawing stays in the UI task. */
    // configUpdateRequest = true;
    // RequestUiWork(GUI_REQ_NONE);
}

void ClockUpdate() {
    EvMotorState_t motorState = MotorControl_GetState();
    if (motorState == EV_MOTOR_STATE_ESTOP_BRAKING || motorState == EV_MOTOR_STATE_FAULT_LATCHED || g_ui32Panel != 0) return;
    int32_t seconds = g_elapsed_seconds % 60;
    int32_t minutes = (g_elapsed_seconds / 60) % 60;
    int32_t hours = (g_elapsed_seconds / 3600) % 12;
    usprintf(clockBuf, "27/05/26 %02d:%02d:%02d", hours, minutes, seconds);
    PushButtonTextSet((tPushButtonWidget *)&g_psClock, clockBuf);
    WidgetPaint((tWidget *)&g_psClock);

}

static void ProcessUiRequests(void)
{
    uint32_t requests = g_ui32GuiRequests;
    g_ui32GuiRequests = GUI_REQ_NONE;

    if (requests == GUI_REQ_NONE)
    {
        return;
    }

    /* One place touches widgets, bc GrLib can lock up if paints overlap. */
    if (requests & GUI_REQ_FAULT_ACK)        { HandleFaultAcknowledge(); }
    if (requests & GUI_REQ_PREV_PANEL)       { HandlePreviousPanel(); }
    if (requests & GUI_REQ_NEXT_PANEL)       { HandleNextPanel(); }
    if (requests & GUI_REQ_START_STOP)       { HandleStartStopButton(); }
    if (requests & GUI_REQ_CONFIG_TAB)       { HandleConfigButtonPress(g_ui32PendingConfig); }
    if (requests & GUI_REQ_PLOT_BUTTON)      { HandleSensorButtonPress(g_ui32PendingPlot); }
    if (requests & GUI_REQ_APPLY_THRESHOLDS) { HandleThresholdButtonPress(); }
}

static void prvGUITask( void *pvParameters ) {
    FPUEnable();
    FPULazyStackingEnable();

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)){}
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    Kentec320x240x16_SSD2119Init(configCPU_CLOCK_HZ);
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    TouchScreenInit(configCPU_CLOCK_HZ);
    TouchScreenCallbackSet(WidgetPointerMessage);

    g_ui32Panel = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    PushButtonImageOff(&g_sPrevious);
    PushButtonTextOff(&g_sPrevious);
    PushButtonFillOn(&g_sPrevious);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);
    WidgetPaint(WIDGET_ROOT);
    UpdateBanner();

    for (;;)
    {
        uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5));
        if (notified) {
            if (plotToggle && g_ui32Panel == 2) {
                WidgetPaint((tWidget *)&g_sCanvas3);
            }            
        }
        if(g_currentState != g_lastState) {
            UpdateBanner();
            g_lastState = g_currentState;
        }

        if(g_ui32Panel != g_ui32LastPanel) {
            UpdateBanner();
            g_ui32LastPanel = g_ui32Panel;
            g_ui32LastConfig = 3;
        }

        if(!faultLatched && g_currentState == EV_MOTOR_STATE_FAULT_LATCHED) {
            OnFaultLatched();
        }

        if(configUpdateRequest) {
            UpdateConfigPage(false);
            configUpdateRequest = false;
        }

        if(clockUpdateRequest) {
            ClockUpdate();
            clockUpdateRequest = false;
        }


        ProcessUiRequests();

        // EvMotorState_t motorState = MotorControl_GetState();
        // if(motorState != EV_MOTOR_STATE_ESTOP_BRAKING) {
        WidgetMessageQueueProcess();
        ProcessUiRequests();

        // vTaskDelay(10);
    }
}

void GuiDashboard_Init(void) {
    EvSensorThresholds_t thresholds = SensorManager_GetThresholds();
    g_thresholds[1] = thresholds.max_motor_power_w;
    g_thresholds[2] = thresholds.min_distance_mm;
    g_thresholds[3] = (uint32_t)thresholds.cooling_threshold_c_x10;
    for (uint32_t idx = 0; idx < 4U; idx++)
    {
        g_temp_thresholds[idx] = g_thresholds[idx];
    }

     xTaskCreate( prvGUITask,
                 "display",
                 DISPLAY_TASK_STACK_WORDS,
                 NULL,
                 tskIDLE_PRIORITY + 1,
                 &xDisplayTaskHandle );

    g_lastTimerTicks = xTaskGetTickCount();
}

void GuiDashboard_Update(void)
{
    EvMotorState_t motorState = MotorControl_GetState();
    g_currentState = motorState;

    TickType_t currentTicks = xTaskGetTickCount();
    if((currentTicks - g_lastTimerTicks) >= pdMS_TO_TICKS(1000)) {
        g_elapsed_seconds++;
        clockUpdateRequest = true;
        g_lastTimerTicks = currentTicks;
        RequestUiWork(GUI_REQ_NONE);
    }

    EvSensorData_t tempSensorData = SensorManager_GetFilteredData();
    if(tempSensorData.ambient_light_lux >= 5) {
        if(nightConditions) {
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0x0);
            nightConditions = false;
        }        
    } else {
        if(!nightConditions) {
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
            nightConditions = true;
        }
    }

    if(g_ui32Panel == 1) {
        configUpdateRequest = true;
        RequestUiWork(GUI_REQ_NONE);
    }
}
