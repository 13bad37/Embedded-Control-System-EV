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

tContext sContext;
uint32_t g_ui32Panel;
TaskHandle_t xDisplayTaskHandle = NULL; 
#define PLOT_SIZE 200
uint32_t graphX = 1;
uint32_t graphY[6];
uint32_t lastGraphY[6];
bool plotToggle = false;
bool faultLatched = false;
uint32_t g_thresholds[6];
volatile uint32_t g_elapsed_ticks = 405000;
char clockBuf[20];

static void prvGUITask( void *pvParameters );
void OnPrevious(tWidget *psWidget);
void OnNext(tWidget *psWidget);
void OnIntroPaint(tWidget *psWidget, tContext *psContext);
void OnCanvasPaint(tWidget *psWidget, tContext *psContext);
void OnSensorPlotsPaint(tWidget *psWidget, tContext *psContext);
void OnStartStopButtonPress(tWidget *psWidget);
void OnSliderChange(tWidget *psWidget, int32_t i32Value);
void UpdateBanner();
void OnSensorButtonPress(tWidget *psWidget);
void OnFaultAcknowledgeButtonPress(tWidget *psWidget);
extern tCanvasWidget g_psPanels[];

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
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrYellow, ClrYellow, 0, ClrBlack,
                            &g_sFontCm22, "ACKNOWLEDGE", 0, 0, 0, 0, OnFaultAcknowledgeButtonPress),
};

tPushButtonWidget g_psStartStopButton[] = {
    RectangularButtonStruct(g_psPanels, 0, 0,
                            &g_sKentec320x240x16_SSD2119, 90, 100, 140, 40,
                            PB_STYLE_FILL | PB_STYLE_TEXT,
                            ClrLightGreen, 0, 0, ClrBlack,
                            &g_sFontCm22, "Start Motor", 0, 0, 0, 0, OnStartStopButtonPress),
};

Canvas(g_sIntroduction, g_psPanels, g_psStartStopButton, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 216, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);

tPushButtonWidget g_psClock[] = {
    RectangularButtonStruct(g_psPanels + 1, 0, 0,
                        &g_sKentec320x240x16_SSD2119, 0, 30, 120, 40,
                        PB_STYLE_FILL | PB_STYLE_TEXT,
                        0, 0, 0, ClrWhite,
                        &g_sFontCm22, "11:15:00", 0, 0, 0, 0, 0),
};

tSliderWidget g_psSliders[] = {
    SliderStruct(g_psPanels + 1, g_psSliders + 1, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 30, 140, 30, 0, 1000, 250,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm20, "250", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 1, g_psSliders + 2, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 60, 140, 30, 0, 1000, 250,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm20, "250", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 1, g_psSliders + 3, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 90, 140, 30, 0, 1000, 250,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm20, "250", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 1, g_psSliders + 4, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 120, 140, 30, 0, 1000, 250,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm20, "250", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 1, g_psClock, 0,
                 &g_sKentec320x240x16_SSD2119, 150, 150, 140, 30, 0, 1000, 250,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 &g_sFontCm20, "250", 0, 0, OnSliderChange),
};

#define NUM_SLIDERS (sizeof(g_psSliders) / sizeof(g_psSliders[0]))

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
uint32_t g_ui32CurrentPlot = 10;

Canvas(g_sCanvas3, g_psPanels + 2, g_psPlotButtons, 0, &g_sKentec320x240x16_SSD2119, 60,
       62, 200, 140, CANVAS_STYLE_OUTLINE | CANVAS_STYLE_APP_DRAWN, 0, ClrGray,
       0, 0, 0, 0, OnCanvasPaint);

Canvas(g_sSensorPlots, g_psPanels + 2, &g_sCanvas3, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 216, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnSensorPlotsPaint);

tCanvasWidget g_psPanels[] = {
    CanvasStruct(0, 0, &g_sIntroduction, &g_sKentec320x240x16_SSD2119, 0, 24,
                 320, 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_psSliders, &g_sKentec320x240x16_SSD2119, 0, 24,
                 320, 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sSensorPlots, &g_sKentec320x240x16_SSD2119, 0, 24, 320,
                 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};

#define NUM_PANELS (sizeof(g_psPanels) / sizeof(g_psPanels[0]))

char *g_pcPanei32Names[] =
{
    "     Motor Control     ",
    "     System Status     ",
    "     Sensor Data     ",
};

RectangularButton(g_sPrevious, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 190,
                  50, 50, PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  &g_sFontCm20, "-", g_pui8Blue50x50, g_pui8Blue50x50Press, 0, 0,
                  OnPrevious);
RectangularButton(g_sNext, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 270, 190,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, &g_sFontCm20, "+", g_pui8Blue50x50,
                  g_pui8Blue50x50Press, 0, 0, OnNext);

void OnPrevious(tWidget *psWidget) {
    if(MotorState == ESTOP) return;
    if(g_ui32Panel == 0) {
        return;
    }

    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    g_ui32Panel--;
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));
    UpdateBanner();

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

void OnNext(tWidget *psWidget) {
    if(MotorState == ESTOP) return;
    if(g_ui32Panel == (NUM_PANELS - 1)) {
        return;
    }

    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    g_ui32Panel++;
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));
    UpdateBanner();

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

void OnIntroPaint(tWidget *psWidget, tContext *psContext) {
    if(MotorState == FAULT) {
        GrContextFontSet(psContext, &g_sFontCm18);
        GrContextForegroundSet(psContext, ClrSilver);
        GrStringDrawCentered(psContext, "An Emergency Stop Has Been Triggered.", -1, 160, 32, 0);
        GrStringDrawCentered(psContext, "You must acknowledge the fault before", -1, 160, 55, 0);
        GrStringDrawCentered(psContext, "starting the motor.", -1, 160, 75, 0);
    }
}

void OnSensorPlotsPaint(tWidget *psWidget, tContext *psContext) {
    GrContextFontSet(psContext, &g_sFontCm16);
    GrContextForegroundSet(psContext, ClrSilver);
    GrStringDrawCentered(psContext, "500", -1, 270, 62, 0);
    GrStringDrawCentered(psContext, "0", -1, 265, 200, 0);
    GrStringDrawCentered(psContext, "0s", -1, 60, 210, 0);
    GrStringDrawCentered(psContext, "10s", -1, 260, 210, 0);
    graphX = 1;
    tRectangle clearRect = {61, 63, 258, 200};
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &clearRect);
}

void OnFaultLatched(void) {
    faultLatched = true;
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    g_ui32Panel = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);
    WidgetPaint((tWidget *)g_psPanels);
    WidgetRemove((tWidget *)&g_psStartStopButton);
    WidgetRemove((tWidget *)&g_sNext);
    WidgetRemove((tWidget *)&g_sPrevious);
    WidgetAdd((tWidget *)g_psPanels, (tWidget *)&g_psFaultAcknowledgeButton);
    WidgetPaint((tWidget *)&g_psFaultAcknowledgeButton);
    PushButtonImageOff(&g_sNext);
    PushButtonTextOff(&g_sNext);
    PushButtonFillOn(&g_sNext);
    WidgetPaint((tWidget *)&g_sNext);
    PushButtonImageOff(&g_sPrevious);
    PushButtonTextOff(&g_sPrevious);
    PushButtonFillOn(&g_sPrevious);
    WidgetPaint((tWidget *)&g_sPrevious);
    UpdateBanner();
}

void OnFaultAcknowledgeButtonPress(tWidget *psWidget) {
    faultLatched = false;
    MotorStateMachine_AcknowledgeFault();
    MotorState = IDLE;
    PushButtonTextSet((tPushButtonWidget *)&g_psStartStopButton, "Start Motor");
    PushButtonFillColorSet((tPushButtonWidget *)&g_psStartStopButton, ClrLightGreen);
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
    uint32_t yMax = 500;
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

void OnStartStopButtonPress(tWidget *psWidget) {
    if (MotorState == IDLE) {
        PushButtonTextSet((tPushButtonWidget *)&g_psStartStopButton, "Stop Motor");
        PushButtonFillColorSet((tPushButtonWidget *)&g_psStartStopButton, ClrRedMask);
        MotorControl_Start();
        MotorState = RUNNING;
    } else {
        PushButtonTextSet((tPushButtonWidget *)&g_psStartStopButton, "Start Motor");
        PushButtonFillColorSet((tPushButtonWidget *)&g_psStartStopButton, ClrLightGreen);
        MotorControl_Stop();
        MotorState = IDLE;
    }
    WidgetPaint((tWidget *)&g_psStartStopButton);

    UpdateBanner();    
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

    if (plotToggle) {
        PushButtonFillColorSet((tPushButtonWidget *)(g_psPlotButtons + g_ui32CurrentPlot), ClrMidnightBlue);
        WidgetPaint((tWidget *)(g_psPlotButtons + g_ui32CurrentPlot));
    }

    graphX = 1;
    tRectangle clearRect = {61, 63, 258, 200};
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &clearRect);

    if(g_ui32CurrentPlot == ui32Idx && plotToggle) {
        plotToggle = !plotToggle;
        return;
    }
    PushButtonFillColorSet((tPushButtonWidget *)(g_psPlotButtons + ui32Idx), ClrAqua);
    WidgetPaint((tWidget *)(g_psPlotButtons + ui32Idx));
    plotToggle = true;

    g_ui32CurrentPlot = ui32Idx;
}

void UpdateBanner() {
    tRectangle sRect;
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 23;

    switch (MotorState) {
        case IDLE:
            GrContextForegroundSet(&sContext, ClrGold);
            GrRectFill(&sContext, &sRect);
            break;
        case STARTING: 
            GrContextForegroundSet(&sContext, ClrGold);
            GrRectFill(&sContext, &sRect);
            break;
        case RUNNING:
            GrContextForegroundSet(&sContext, ClrGreen);
            GrRectFill(&sContext, &sRect);
            break;
        case ESTOP:
            GrContextForegroundSet(&sContext, ClrRedMask);
            GrRectFill(&sContext, &sRect);
            break;
        case FAULT:
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
    static char pcSliderText[5];
    uint32_t ui32Idx;
    for(ui32Idx = 0; ui32Idx < NUM_SLIDERS; ui32Idx++) {
        if(psWidget == (tWidget *)(g_psSliders + ui32Idx)) {
            break;
        }
    }
    g_thresholds[ui32Idx] = i32Value;
    usprintf(pcSliderText, "%3d", i32Value);
    SliderTextSet(psWidget, pcSliderText);
    WidgetPaint(psWidget);
}

void ClockUpdate() {
    if (MotorState == ESTOP || g_ui32Panel != 1) return;
    int32_t seconds = (g_elapsed_ticks / 10) % 60;
    int32_t minutes = (g_elapsed_ticks / 600) % 60;
    int32_t hours = (g_elapsed_ticks / 36000) % 12;
    usprintf(clockBuf, "%02d:%02d:%02d", hours, minutes, seconds);
    PushButtonTextSet(&g_psClock, clockBuf);
    WidgetPaint((tWidget *)&g_psClock);
}

void vClockTimerCallback(TimerHandle_t xTimer) {
    g_elapsed_ticks++;
    if(g_elapsed_ticks % 10 == 9) {
        ClockUpdate();
    }
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
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);
    WidgetPaint(WIDGET_ROOT);
    UpdateBanner();

    for (;;)
    {
        if (ulTaskNotifyTake(pdTRUE, 0)) {
            if (plotToggle && g_ui32Panel == 2) {
                WidgetPaint((tWidget *)&g_sCanvas3);
            }            
        }
        EvMotorState_t motorState = MotorControl_GetState();
        if(motorState != EV_MOTOR_STATE_ESTOP_BRAKING) {
            WidgetMessageQueueProcess();
        }

        // vTaskDelay(10);
    }
}

void GuiDashboard_Init(void) {
     xTaskCreate( prvGUITask,
                 "Hello",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 tskIDLE_PRIORITY + 1,
                 &xDisplayTaskHandle );

    // for testing without sensor data
    TimerHandle_t xClockTimer;
    xClockTimer = xTimerCreate(
        "PlotTimer",
        pdMS_TO_TICKS(100),
        pdTRUE,
        NULL,
        vClockTimerCallback
    );
    xTimerStart(xClockTimer, 0);
}

void GuiDashboard_Update(void)
{
    EvMotorState_t motorState = MotorControl_GetState();
    if(!faultLatched && motorState == EV_MOTOR_STATE_FAULT_LATCHED) {
        OnFaultLatched();
    }
    /* TODO: show motor state, faults, speed input, thresholds etc */
}

