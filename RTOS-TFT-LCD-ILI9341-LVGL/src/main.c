/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"
#include "playbtn.h"
#include "replaybtn.h"
#include "wheelbtn.h"
#include "cronometro.h"

/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/

#define LV_HOR_RES_MAX          (320)
#define LV_VER_RES_MAX          (240)

LV_FONT_DECLARE(dseg20);
LV_FONT_DECLARE(dseg30);
LV_FONT_DECLARE(dseg35);

static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv; 
static lv_indev_drv_t indev_drv;

static lv_obj_t * labelPlay;
static lv_obj_t * labelReplay;
static lv_obj_t * labelWheel;
static lv_obj_t * labelCron;
static lv_obj_t * labelDist;


volatile int play_clicked = 0;
volatile int replay_clicked = 0;
volatile int wheel_clicked = 0;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/

static void play_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		if (play_clicked == 0){
			play_clicked = 1;
			} else {
			play_clicked = 0;
		}
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void replay_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		if (replay_clicked == 0){
			replay_clicked = 1;
			} else {
			replay_clicked = 0;
		}
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void wheel_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		if (wheel_clicked == 0){
			wheel_clicked = 1;
			} else {
			wheel_clicked = 0;
		}
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

void lv_tela_1(void) {
	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_bg_color(&style, lv_color_white());
	
	// -------------------- BUTTON INITS --------------------
	
	lv_obj_t * play_logo = lv_imgbtn_create(lv_scr_act());
	lv_obj_t * replay_logo = lv_imgbtn_create(lv_scr_act());
	lv_obj_t * wheel_logo = lv_imgbtn_create(lv_scr_act());
	lv_obj_t * cronometro_img = lv_img_create(lv_scr_act());
	
	// -------------------- PLAY BUTTON --------------------

	lv_obj_add_event_cb(play_logo, play_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(play_logo, LV_ALIGN_BOTTOM_LEFT, 15, 60);
	lv_imgbtn_set_src(play_logo, LV_IMGBTN_STATE_RELEASED, &playbtn, NULL, NULL);
	lv_obj_add_style(play_logo, &style, 0);
	
	// -------------------- REPLAY BUTTON --------------------
	
	lv_obj_add_event_cb(replay_logo, replay_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(replay_logo, play_logo, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_imgbtn_set_src(replay_logo, LV_IMGBTN_STATE_RELEASED, &replaybtn, NULL, NULL);
	lv_obj_add_style(replay_logo, &style, 0);
	
	// -------------------- WHEEL BUTTON --------------------
	
	lv_obj_add_event_cb(wheel_logo, replay_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(wheel_logo, replay_logo, LV_ALIGN_OUT_RIGHT_TOP, -35, 0);
	lv_imgbtn_set_src(wheel_logo, LV_IMGBTN_STATE_RELEASED, &wheelbtn, NULL, NULL);
	lv_obj_add_style(wheel_logo, &style, 0);
	
	// -------------------- CRONOMETRO IMG --------------------
	
	lv_img_set_src(cronometro_img, &cronometro);
	lv_obj_align(cronometro_img, LV_ALIGN_LEFT_MID, 15, 0);
	
	// -------------------- CRONOMETRO LABEL --------------------
	
	labelCron = lv_label_create(lv_scr_act());
	lv_obj_align(labelCron, LV_ALIGN_LEFT_MID, 85 , 0);
	lv_obj_set_style_text_font(labelCron, &dseg30, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelCron, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelCron, "%02d:%02d", 01,54);
	lv_obj_add_style(labelCron, &style, 0);
	
	// -------------------- DIST LABEL --------------------
	
	labelDist = lv_label_create(lv_scr_act());
	lv_obj_align(labelDist, LV_ALIGN_RIGHT_MID, -40 , 0);
	lv_obj_set_style_text_font(labelDist, &dseg30, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelDist, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelDist, "%02d", 12); // FALTA ADICIONAR O "KM" AQUI!! e RECOMENDO FAZER UMA DSEG25 PRA CÁ!
	lv_obj_add_style(labelDist, &style, 0);

	lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
	int px, py;

	lv_tela_1();

	for (;;)  {
		lv_tick_inc(50);
		lv_task_handler();
		vTaskDelay(50);
	}
}

/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	data->point.x = px;
	data->point.y = py;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();

	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}
