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
// #include "examples/boards/ek-tm4c1294xl-boostxl-kentec-s1/drivers/Kentec320x240x16_ssd2119_spi.h"
// #include "examples/boards/ek-tm4c1294xl-boostxl-kentec-s1/drivers/touch.h"
// #include "examples/boards/ek-tm4c1294xl-boostxl-kentec-s1/grlib_demo/images.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"
#include "drivers/images.h"

tContext sContext;
static TaskHandle_t xDisplayTaskHandle = NULL; 
#define PLOT_SIZE 200
uint32_t g_plot_data[5][PLOT_SIZE];
bool plotToggle = false;
uint32_t g_count = 0;
uint32_t g_thresholds[5];
uint32_t g_temp_thresholds[5];
volatile uint32_t g_elapsed_ticks = 405000;
char clockBuf[20];

static void prvDisplayDemoTask( void *pvParameters );
void OnPrevious(tWidget *psWidget);
void OnNext(tWidget *psWidget);
void OnIntroPaint(tWidget *psWidget, tContext *psContext);
void OnCanvasPaint(tWidget *psWidget, tContext *psContext);
// void OnCheckChange(tWidget *psWidget, uint32_t bSelected);
// void OnButtonPress(tWidget *psWidget);
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

//*****************************************************************************
//
// The first panel, which contains introductory text explaining the
// application.
//
//*****************************************************************************
tPushButtonWidget g_psFaultAcknowledgeButton[] = 
{
    RectangularButtonStruct(g_psPanels, 0, 0,
                            &g_sKentec320x240x16_SSD2119, 60, 100, 200, 40,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrYellow, ClrYellow, 0, ClrBlack,
                            &g_sFontCm22, "ACKNOWLEDGE", 0, 0, 0, 0, OnFaultAcknowledgeButtonPress),
};

tPushButtonWidget g_psStartStopButton[] = 
{
    RectangularButtonStruct(g_psPanels, 0, 0,
                            &g_sKentec320x240x16_SSD2119, 90, 100, 140, 40,
                            PB_STYLE_FILL | PB_STYLE_TEXT,
                            ClrLightGreen, 0, 0, ClrBlack,
                            &g_sFontCm22, "Start Motor", 0, 0, 0, 0, OnStartStopButtonPress),
};

Canvas(g_sIntroduction, g_psPanels, g_psStartStopButton, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 216, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);


//*****************************************************************************
//
// The second panel, which demonstrates the graphics primitives.
//
//*****************************************************************************

tPushButtonWidget g_psClock[] = 
{
    RectangularButtonStruct(g_psPanels + 1, 0, 0,
                        &g_sKentec320x240x16_SSD2119, 0, 30, 120, 40,
                        PB_STYLE_FILL | PB_STYLE_TEXT,
                        0, 0, 0, ClrWhite,
                        &g_sFontCm22, "11:15:00", 0, 0, 0, 0, 0),
};

tSliderWidget g_psSliders[] =
{
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

#define SLIDER_TEXT_VAL_INDEX   0
#define SLIDER_LOCKED_INDEX     2
#define SLIDER_CANVAS_VAL_INDEX 4

#define NUM_SLIDERS (sizeof(g_psSliders) / sizeof(g_psSliders[0]))

// tCanvasWidget g_psCheckBoxIndicators[] =
// {
//     CanvasStruct(g_psPanels + 1, g_psSliders, 0,
//                  &g_sKentec320x240x16_SSD2119, 91, 134, 50, 42,
//                  CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0)
// };
// tCheckBoxWidget g_psCheckBoxes[] =
// {
//     CheckBoxStruct(g_psPanels + 1, g_psCheckBoxIndicators, 0,
//                    &g_sKentec320x240x16_SSD2119, 40, 134, 50, 42,
//                    CB_STYLE_OUTLINE | CB_STYLE_TEXT, 16,
//                    0, ClrGray, ClrGreen, &g_sFontCm20, "2",
//                    0, OnCheckChange),
// };
// #define NUM_CHECK_BOXES         (sizeof(g_psCheckBoxes) /   \
//                                  sizeof(g_psCheckBoxes[0]))

// // Define push buttons
// tCanvasWidget g_psPushButtonIndicators[] =
// {
//     CanvasStruct(g_psPanels + 1, g_psPushButtonIndicators + 1, 0,
//                  &g_sKentec320x240x16_SSD2119, 40, 85, 20, 20,
//                  CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
//     CanvasStruct(g_psPanels + 1, g_psCheckBoxes, 0,
//                  &g_sKentec320x240x16_SSD2119, 90, 85, 20, 20,
//                  CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),

// };
// tPushButtonWidget g_psPushButtons[] =
// {
//     RectangularButtonStruct(g_psPanels + 1, g_psPushButtons + 1, 0,
//                             &g_sKentec320x240x16_SSD2119, 30, 35, 40, 40,
//                             PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
//                             ClrMidnightBlue, ClrBlack, ClrGray, ClrSilver,
//                             &g_sFontCm22, "1", 0, 0, 0, 0, OnButtonPress),
 
//     CircularButtonStruct(g_psPanels + 1, g_psPushButtonIndicators, 0,
//                          &g_sKentec320x240x16_SSD2119, 100, 55, 20,
//                          PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
//                          ClrMidnightBlue, ClrBlack, ClrGray, ClrSilver,
//                          &g_sFontCm22, "3", 0, 0, 0, 0, OnButtonPress),
   
// };
// #define NUM_PUSH_BUTTONS        (sizeof(g_psPushButtons) /   \
//                                  sizeof(g_psPushButtons[0]))
// uint32_t g_ui32ButtonState;

//*****************************************************************************
//
// The third panel, which demonstrates the canvas widget.
//
//*****************************************************************************
// change this to show how to make a simple plot
tPushButtonWidget g_psPlotButtons[] =
{
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 1, 0,
                            &g_sKentec320x240x16_SSD2119, 0, 23, 64, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrMidnightBlue, ClrBlack, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "M.Speed", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 2, 0,
                            &g_sKentec320x240x16_SSD2119, 64, 23, 64, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrMidnightBlue, ClrBlack, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "M.Power", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 3, 0,
                            &g_sKentec320x240x16_SSD2119, 128, 23, 64, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrMidnightBlue, ClrBlack, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "Amb.Light", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 4, 0,
                            &g_sKentec320x240x16_SSD2119, 192, 23, 64, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrMidnightBlue, ClrBlack, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "Temp.", 0, 0, 0, 0, OnSensorButtonPress),
    RectangularButtonStruct(g_psPanels + 2, 0, 0,
                            &g_sKentec320x240x16_SSD2119, 256, 23, 64, 33,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrMidnightBlue, ClrBlack, ClrWhite, ClrSilver,
                            &g_sFontFixed6x8, "Distance", 0, 0, 0, 0, OnSensorButtonPress),
};

#define NUM_PLOT_BUTTONS        (sizeof(g_psPlotButtons) /   \
                                 sizeof(g_psPlotButtons[0]))
uint32_t g_ui32CurrentPlot = 10;

Canvas(g_sCanvas3, g_psPanels + 2, g_psPlotButtons, 0, &g_sKentec320x240x16_SSD2119, 50,
       57, 200, 140, CANVAS_STYLE_OUTLINE | CANVAS_STYLE_APP_DRAWN, 0, ClrGray,
       0, 0, 0, 0, OnCanvasPaint);



//*****************************************************************************
//
// An array of canvas widgets, one per panel.  Each canvas is filled with
// black, overwriting the contents of the previous panel.
//
//*****************************************************************************
tCanvasWidget g_psPanels[] =
{
    CanvasStruct(0, 0, &g_sIntroduction, &g_sKentec320x240x16_SSD2119, 0, 24,
                 320, 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_psSliders, &g_sKentec320x240x16_SSD2119, 0, 24,
                 320, 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sCanvas3, &g_sKentec320x240x16_SSD2119, 0, 24, 320,
                 216, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};

//*****************************************************************************
//
// The number of panels.
//
//*****************************************************************************
#define NUM_PANELS              (sizeof(g_psPanels) / sizeof(g_psPanels[0]))

//*****************************************************************************
//
// The names for each of the panels, which is displayed at the bottom of the
// screen.
//
//*****************************************************************************
char *g_pcPanei32Names[] =
{
    "     Motor Control     ",
    "     System Status     ",
    "     Sensor Data     ",
};

//*****************************************************************************
//
// The buttons and text across the bottom of the screen.
//
//*****************************************************************************
RectangularButton(g_sPrevious, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 190,
                  50, 50, PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  &g_sFontCm20, "-", g_pui8Blue50x50, g_pui8Blue50x50Press, 0, 0,
                  OnPrevious);

// Canvas(g_sTitle, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 50, 190, 220, 50,
//        CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrSilver,
//        &g_sFontCm20, 0, 0, 0);

RectangularButton(g_sNext, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 270, 190,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, &g_sFontCm20, "+", g_pui8Blue50x50,
                  g_pui8Blue50x50Press, 0, 0, OnNext);

//*****************************************************************************
//
// The panel that is currently being displayed.
//
//*****************************************************************************
uint32_t g_ui32Panel;

//*****************************************************************************
//
// Handles presses of the previous panel button.
//
//*****************************************************************************
void
OnPrevious(tWidget *psWidget)
{
    if(MotorState == ESTOP) return;
    // There is nothing to be done if the first panel is already being displayed.
    if(g_ui32Panel == 0) {
        return;
    }

    // Remove the current panel.
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    // Decrement the panel index.
    g_ui32Panel--;

    // Add and draw the new panel.
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    // Set the title of this panel.
    UpdateBanner();

    // See if this is the first panel.
    if(g_ui32Panel == 0)
    {
        // Clear the previous button from the display since the first panel is being displayed.
        PushButtonImageOff(&g_sPrevious);
        PushButtonTextOff(&g_sPrevious);
        PushButtonFillOn(&g_sPrevious);
    }

    // See if the previous panel was the last panel.
    if(g_ui32Panel == (NUM_PANELS - 2)) {
        // Display the next button.
        PushButtonImageOn(&g_sNext);
        PushButtonTextOn(&g_sNext);
        PushButtonFillOff(&g_sNext);
    }
    WidgetPaint((tWidget *)&g_sPrevious);
    WidgetPaint((tWidget *)&g_sNext);
}

//*****************************************************************************
//
// Handles presses of the next panel button.
//
//*****************************************************************************
void
OnNext(tWidget *psWidget)
{
    if(MotorState == ESTOP) return;
    // There is nothing to be done if the last panel is already being displayed.
    if(g_ui32Panel == (NUM_PANELS - 1)) {
        return;
    }

    // Remove the current panel.
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    // Increment the panel index.
    g_ui32Panel++;

    // Add and draw the new panel.
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    // Set the title of this panel.
    UpdateBanner();

    // See if the previous panel was the first panel.
    if(g_ui32Panel == 1) {
        // Display the previous button.
        PushButtonImageOn(&g_sPrevious);
        PushButtonTextOn(&g_sPrevious);
        PushButtonFillOff(&g_sPrevious);
    }

    // See if this is the last panel.
    if(g_ui32Panel == (NUM_PANELS - 1)) {
        // Clear the next button from the display since the last panel is being displayed.
        PushButtonImageOff(&g_sNext);
        PushButtonTextOff(&g_sNext);
        PushButtonFillOn(&g_sNext);
        // WidgetPaint((tWidget *)&g_sNext);
        // WidgetPaint((tWidget *)&g_sPrevious);
    }
    
    WidgetPaint((tWidget *)&g_sPrevious);
    WidgetPaint((tWidget *)&g_sNext);
}

//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void
OnIntroPaint(tWidget *psWidget, tContext *psContext)
{

    if(MotorState == FAULT) {
        GrContextFontSet(psContext, &g_sFontCm18);
        GrContextForegroundSet(psContext, ClrSilver);
        GrStringDrawCentered(psContext, "An Emergency Stop Has Been Triggered.", -1, 160, 32, 0);
        GrStringDrawCentered(psContext, "You must acknowledge the fault before", -1, 160, 55, 0);
        GrStringDrawCentered(psContext, "starting the motor.", -1, 160, 75, 0);
    }

}

void
OnFaultLatched(void) {
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

void
OnFaultAcknowledgeButtonPress(tWidget *psWidget) {
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


//*****************************************************************************
//
// Handles paint requests for the canvas demonstration widget.
//
//*****************************************************************************
void
OnCanvasPaint(tWidget *psWidget, tContext *psContext)
{
    if(MotorState == ESTOP) return;
    // define and draw the scale
    uint32_t xMax = PLOT_SIZE;
    uint32_t yMax = 520;
    GrContextFontSet(&sContext, &g_sFontCm20);
    GrStringDrawCentered(&sContext, "520", -1,
                         10, 33, 0);
    GrStringDrawCentered(&sContext, "0", -1,
                         10, 160, 0);
    GrStringDrawCentered(&sContext, "-5s", -1,
                         30, 175, 0);
    GrStringDrawCentered(&sContext, "0s", -1,
                         210, 175, 0);


    // define the size of the canvas - Eg: 20, 27, 200, 140
    uint32_t canvas_xMin = 50;
    uint32_t canvas_xRange = 200;
    uint32_t canvas_yMin = 57;
    uint32_t canvas_yRange = 140;

    // clear current plot
    tRectangle clearRect;
    clearRect.i16XMin = canvas_xMin + 1;
    clearRect.i16YMin = canvas_yMin + 1;
    clearRect.i16XMax = canvas_xMin + canvas_xRange - 2;
    clearRect.i16YMax = canvas_yMin + canvas_yRange - 2;

    GrContextForegroundSet(psContext, ClrBlack);
    GrRectFill(psContext, &clearRect);
    if (!plotToggle || g_ui32Panel != 2) {
        return;
    }
    // iterate through the data and draw a rectangle on the screen
    uint32_t ui32Idx;

    int prev_x = -1;
    int prev_y = -1;

    for(ui32Idx = 0; ui32Idx < PLOT_SIZE; ui32Idx += 1)
    {
        // get the value of each element and convert to plotting coordinate
        float display_y = g_plot_data[g_ui32CurrentPlot][ui32Idx];
        display_y = canvas_yMin + canvas_yRange - (display_y/yMax)*canvas_yRange;

        // get the current index, denoting time
        float display_x = ui32Idx;
        display_x = (display_x/xMax)*canvas_xRange + canvas_xMin;

        int x = (int)display_x;
        int y = (int)display_y;

        if (prev_x != -1) {
            GrContextForegroundSet(psContext, ClrBlue);
            GrLineDraw(psContext, prev_x, prev_y, x, y);
        }

        prev_x = x;
        prev_y = y;
    }
}

//*****************************************************************************
//
// Handles change notifications for the check box widgets.
//
//*****************************************************************************
// void
// OnCheckChange(tWidget *psWidget, uint32_t bSelected)
// {
//     uint32_t ui32Idx;

//     //
//     // Find the index of this check box.
//     //
//     for(ui32Idx = 0; ui32Idx < NUM_CHECK_BOXES; ui32Idx++)
//     {
//         if(psWidget == (tWidget *)(g_psCheckBoxes + ui32Idx))
//         {
//             break;
//         }
//     }

//     //
//     // Return if the check box could not be found.
//     //
//     if(ui32Idx == NUM_CHECK_BOXES)
//     {
//         return;
//     }
//     //
//     // Set the matching indicator based on the selected state of the check box.
//     //
//     CanvasImageSet(g_psCheckBoxIndicators + ui32Idx,
//                    bSelected ? g_pui8LightOn : g_pui8LightOff);
//     WidgetPaint((tWidget *)(g_psCheckBoxIndicators + ui32Idx));

// }

void OnStartStopButtonPress(tWidget *psWidget) {
    if (MotorState == IDLE) {
        PushButtonTextSet((tPushButtonWidget *)&g_psStartStopButton, "Stop Motor");
        PushButtonFillColorSet((tPushButtonWidget *)&g_psStartStopButton, ClrRedMask);
        // g_psStartStopButton->ui32Style &= PB_STYLE_PRESSED;
        // g_psStartStopButton->sBase->ui32Flags |= WIDGET_FLAG_DISABLED;
        MotorState = RUNNING;
    } else {
        PushButtonTextSet((tPushButtonWidget *)&g_psStartStopButton, "Start Motor");
        PushButtonFillColorSet((tPushButtonWidget *)&g_psStartStopButton, ClrLightGreen);
        MotorState = IDLE;
    }
    WidgetPaint((tWidget *)&g_psStartStopButton);

    UpdateBanner();    
}

void OnSensorButtonPress(tWidget *psWidget) {
    // if(MotorState == ESTOP) return;
    int ui32Idx;
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

    if(g_ui32CurrentPlot == ui32Idx && plotToggle) {
        plotToggle = !plotToggle;
        return;
    }
    PushButtonFillColorSet((tPushButtonWidget *)(g_psPlotButtons + ui32Idx), ClrAqua);
    WidgetPaint((tWidget *)(g_psPlotButtons + ui32Idx));
    plotToggle = true;

    g_ui32CurrentPlot = ui32Idx;

// RectangularButtonStruct(g_psPanels + 2, g_psPlotButtons + 5, 0,
//                     &g_sKentec320x240x16_SSD2119, 256, 23, 64, 33,
//                     PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
//                     ClrMidnightBlue, ClrBlack, ClrWhite, ClrSilver,
//                     &g_sFontFixed6x8, "Distance", 0, 0, 0, 0, OnSensorButtonPress),
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

//*****************************************************************************
//
// Handles press notifications for the push button widgets.
//
//*****************************************************************************
// void
// OnButtonPress(tWidget *psWidget)
// {
//     uint32_t ui32Idx;

//     //
//     // Find the index of this push button.
//     //
//     for(ui32Idx = 0; ui32Idx < NUM_PUSH_BUTTONS; ui32Idx++)
//     {
//         if(psWidget == (tWidget *)(g_psPushButtons + ui32Idx))
//         {
//             break;
//         }
//     }

//     //
//     // Return if the push button could not be found.
//     //
//     if(ui32Idx == NUM_PUSH_BUTTONS)
//     {
//         return;
//     }

//     //
//     // Toggle the state of this push button indicator.
//     //
//     g_ui32ButtonState ^= 1 << ui32Idx;

//     //
//     // Set the matching indicator based on the selected state of the check box.
//     //
//     CanvasImageSet(g_psPushButtonIndicators + ui32Idx,
//                    (g_ui32ButtonState & (1 << ui32Idx)) ? g_pui8LightOn :
//                    g_pui8LightOff);
//     WidgetPaint((tWidget *)(g_psPushButtonIndicators + ui32Idx));
//     OnFaultLatched();

// }

//*****************************************************************************
//
// Handles notifications from the slider controls.
//
//*****************************************************************************
void
OnSliderChange(tWidget *psWidget, int32_t i32Value)
{
    static char pcCanvasText[5];
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

    //
    // Is this the widget whose value we mirror in the canvas widget and the
    // locked slider?
    //
    // if(psWidget == (tWidget *)&g_psSliders[SLIDER_CANVAS_VAL_INDEX])
    // {
    //     //
    //     // Yes - update the canvas to show the slider value.
    //     //
    //     // usprintf(pcCanvasText, "%3d%%", i32Value);
    //     // CanvasTextSet(&g_sSliderValueCanvas, pcCanvasText);
    //     // WidgetPaint((tWidget *)&g_sSliderValueCanvas);

    //     //
    //     // Also update the value of the locked slider to reflect this one.
    //     //
    //     SliderValueSet(&g_psSliders[SLIDER_LOCKED_INDEX], i32Value);
    //     WidgetPaint((tWidget *)&g_psSliders[SLIDER_LOCKED_INDEX]);
    // }

    // if(psWidget == (tWidget *)&g_psSliders[SLIDER_TEXT_VAL_INDEX])
    // {
    //     //
    //     // Yes - update the canvas to show the slider value.
    //     //
    //     usprintf(pcSliderText, "%3d%%", i32Value);
    //     SliderTextSet(&g_psSliders[SLIDER_TEXT_VAL_INDEX], pcSliderText);
    //     WidgetPaint((tWidget *)&g_psSliders[SLIDER_TEXT_VAL_INDEX]);
    // }


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

void vPlotTimerCallback(TimerHandle_t xTimer)
{
    g_elapsed_ticks++;
    if(g_elapsed_ticks % 10 == 9) {
        ClockUpdate();
    }
    g_count++;
    uint32_t new_val = urand();

    for (int i = 0; i < PLOT_SIZE - 1; i++) {
        g_plot_data[g_ui32CurrentPlot][i] = g_plot_data[g_ui32CurrentPlot][i + 1];
    }

    // g_plot_data[PLOT_SIZE - 1] = new_val % 520;
    uint32_t add_val = new_val % 2 ? new_val % 20 : -(new_val % 20);
    g_plot_data[g_ui32CurrentPlot][PLOT_SIZE - 1] = (g_plot_data[g_ui32CurrentPlot][PLOT_SIZE - 1] + add_val) % 520;



    if (MotorState == ESTOP) { return; }
    if(plotToggle && new_val % 20 < 1 && g_ui32Panel == 2) {
        MotorState = ESTOP;
        g_count = 0;
        UpdateBanner();
    }

    if (plotToggle && g_ui32Panel == 2) {
        xTaskNotifyGive(xDisplayTaskHandle);
    }
}

void GuiDashboard_Init(void)
{
     xTaskCreate( prvDisplayDemoTask,
                 "Hello",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 tskIDLE_PRIORITY + 1,
                 &xDisplayTaskHandle );
                 
    TimerHandle_t xPlotTimer;
    xPlotTimer = xTimerCreate(
        "PlotTimer",
        pdMS_TO_TICKS(100),
        pdTRUE,
        NULL,
        vPlotTimerCallback
    );
    xTimerStart(xPlotTimer, 0);
}

void GuiDashboard_Update(void)
{
    /* TODO: show motor state, faults, speed input, thresholds etc */
}

static void prvDisplayDemoTask( void *pvParameters )
{
    //
    // The FPU should be enabled because some compilers will use floating-
    // point registers, even for non-floating-point code.  If the FPU is not
    // enabled this will cause a fault.  This also ensures that floating-
    // point operations could be added to this application and would work
    // correctly and use the hardware floating-point unit.  Finally, lazy
    // stacking is enabled for interrupt handlers.  This allows floating-
    // point instructions to be used within interrupt handlers, but at the
    // expense of extra stack usage.
    //
    FPUEnable();
    FPULazyStackingEnable();

    //
    // Initialize the display driver.
    //
    Kentec320x240x16_SSD2119Init(configCPU_CLOCK_HZ);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    UpdateBanner();

    //
    // Put the application name in the middle of the banner.
    //
    // GrContextFontSet(&sContext, &g_sFontCm20);
    // GrStringDrawCentered(&sContext, "Motor Control", -1,
    //                      GrContextDpyWidthGet(&sContext) / 2, 8, 0);


    //
    // Initialize the touch screen driver and have it route its messages to the
    // widget tree.
    //
    TouchScreenInit(configCPU_CLOCK_HZ);
    TouchScreenCallbackSet(WidgetPointerMessage);


        //
    // Add the first panel to the widget tree.
    //
    g_ui32Panel = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);
    // CanvasTextSet(&g_sTitle, g_pcPanei32Names[0]);
    //
    // Add the title block and the previous and next buttons to the widget
    // tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    // WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sTitle);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);



    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop forever handling widget messages.
    //
    for (;;)
    {
        if (ulTaskNotifyTake(pdTRUE, 0))
        {
            if (g_ui32Panel == 2) {
                WidgetPaint((tWidget *)&g_sCanvas3);
            }
            
        }

        if(MotorState != ESTOP) {
            //
            // Process any messages in the widget message queue.
            //

            WidgetMessageQueueProcess();
        }

        if (MotorState == ESTOP) {
            if(g_count >= 30) { 
                MotorState = FAULT;
                OnFaultLatched();
                g_count = 0;    
            }
        }


    }
}

