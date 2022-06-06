/************************************************************************/
/* INCLUDES                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "arm_math.h"
#include "touch/touch.h"
#include "imgs/playbtn.h"
#include "imgs/replaybtn.h"
#include "imgs/wheelbtn.h"
#include "imgs/cancelbtn.h"
#include "imgs/confirmbtn.h"
#include "imgs/returnbtn.h"
#include "imgs/cronometro.h"
#include "imgs/wheelimg.h"
#include "imgs/logoimg.h"
#include "imgs/distimg.h"
#include "imgs/velintimg.h"
#include "imgs/acel_img.h"
#include "imgs/desacel_img.h"
#include "imgs/stable_img.h"
#include "imgs/ready_img.h"
#include "imgs/stopbtn.h"

/************************************************************************/
/* DEFINES PINOS                                                        */
/************************************************************************/

#define SIMULADOR_PIO			PIOA
#define SIMULADOR_PIO_ID		ID_PIOA
#define SIMULADOR_IDX			19
#define SIMULADOR_IDX_MASK		(1 << SIMULADOR_IDX)

#define BUT_PIO					PIOA
#define BUT_PIO_ID				ID_PIOA
#define BUT_PIO_IDX				11
#define BUT_PIO_IDX_MASK		(1u << BUT_PIO_IDX)


#define RED_PIO					PIOB
#define RED_PIO_ID				ID_PIOB
#define RED_IDX					2
#define RED_IDX_MASK			(1 << RED_IDX)

#define GREEN_PIO				PIOC
#define GREEN_PIO_ID			ID_PIOC
#define GREEN_IDX				30
#define GREEN_IDX_MASK			(1 << GREEN_IDX)

#define BLUE_PIO				PIOC
#define BLUE_PIO_ID				ID_PIOC
#define BLUE_IDX				17
#define BLUE_IDX_MASK			(1 << BLUE_IDX)


/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/

#define LV_HOR_RES_MAX          (320)
#define LV_VER_RES_MAX          (240)
//#define RAMP

#define VEL_MAX_KMH				5.0f
#define VEL_MIN_KMH				1.0f


LV_FONT_DECLARE(dseg20);
LV_FONT_DECLARE(dseg25);
LV_FONT_DECLARE(dseg30);
LV_FONT_DECLARE(dseg35);
LV_FONT_DECLARE(dseg40);
LV_FONT_DECLARE(dseg50);

static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv; 
static lv_indev_drv_t indev_drv;


/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

#define TASK_RTC_STACK_SIZE					(4096 / sizeof(portSTACK_TYPE))
#define TASK_RTC_STACK_PRIORITY				(tskIDLE_PRIORITY)

#define TASK_TC_STACK_SIZE					(4096 / sizeof(portSTACK_TYPE))
#define TASK_TC_STACK_PRIORITY				(tskIDLE_PRIORITY)

#define TASK_CALC_STACK_SIZE				(4096 / sizeof(portSTACK_TYPE))
#define TASK_CALC_STACK_PRIORITY			(tskIDLE_PRIORITY)

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

/*------------------------------------------------- GLOBAIS ------------------------------------------------*/

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

SemaphoreHandle_t xSemaphoreRTC;
volatile uint32_t current_hour, current_min, current_sec;
volatile uint32_t current_year, current_month, current_day, current_week;

SemaphoreHandle_t xSemaphoreTC;
volatile uint32_t alarm_sec, alarm_h, alarm_min;

SemaphoreHandle_t xSemaphoredT;
SemaphoreHandle_t xSemaphoreVelMedia;

static lv_obj_t * scr1;  // screen 1
static lv_obj_t * scr2;  // screen 2

int segundos_vel_media = 0;
double RAIO	= 0.58/2;
char buf[32];

SemaphoreHandle_t xSemaphorePlay;
SemaphoreHandle_t xSemaphoreStop;
SemaphoreHandle_t xSemaphoreReplay;
SemaphoreHandle_t xSemaphoreWheel;
SemaphoreHandle_t xSemaphoreReturn;
SemaphoreHandle_t xSemaphoreConfirm;
SemaphoreHandle_t xSemaphoreCancel;
	
QueueHandle_t xQueueBut;


static lv_obj_t * labelCron;
static lv_obj_t * labelDist;
static lv_obj_t * labelDistText;
static lv_obj_t * labelClock;
static lv_obj_t * labelVelMed;
static lv_obj_t * labelVelMedText;
static lv_obj_t * labelVelInst;
static lv_obj_t * labelVelInstText;

/*------------------------------------------------------- PROTOTYPES -----------------------------------------------------*/
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);
void RTC_Handler(void);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);


/* ------------------------------------- HANDLERS DAS TELAS -----------------------------------------------------------------*/


static void play_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphorePlay, xHigherPriorityTaskWoken);
	}
}

static void stop_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreStop, xHigherPriorityTaskWoken);
	}
}

static void replay_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreReplay, xHigherPriorityTaskWoken);
	}
}

static void wheel_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGive(xSemaphoreWheel); 
	}
}

static void return_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGive(xSemaphoreReturn);
	}
}

static void confirm_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		RAIO = (atoi(buf)*0.0254)/2.0;
		printf("Option: %2.1f", RAIO);
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGive(xSemaphoreConfirm);
	}
}

static void cancel_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGive(xSemaphoreCancel);
	}
}

static void drop_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	if(code == LV_EVENT_VALUE_CHANGED) {
		lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
		LV_LOG_USER("Option: %s", buf);
	}
}


/* ---------------------------------------------- TODOS OS ELEMENTOS DA TELA 1 -------------------------------------------------------------------*/

void create_scr(lv_obj_t * screen) {
	static lv_style_t style;
	lv_style_init(&style);
	lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN );
	
	// -------------------- BUTTON INITS --------------------
	
	lv_obj_t * play_logo = lv_imgbtn_create(screen);
	lv_obj_t * replay_logo = lv_imgbtn_create(screen);
	lv_obj_t * wheel_logo = lv_imgbtn_create(screen);
	lv_obj_t * cronometro_img = lv_img_create(screen);
	lv_obj_t * company_logo = lv_img_create(screen);
	lv_obj_t * dist_img = lv_img_create(screen);
	lv_obj_t * velinst_img = lv_img_create(screen);
	
	// -------------------- PLAY BUTTON --------------------
	
	lv_obj_add_event_cb(play_logo, play_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(play_logo, LV_ALIGN_BOTTOM_LEFT, 15, 60);
	lv_imgbtn_set_src(play_logo, LV_IMGBTN_STATE_RELEASED, &playbtn, NULL, NULL);
	lv_obj_add_style(play_logo, &style, LV_STATE_PRESSED);
	
	
	// -------------------- REPLAY BUTTON --------------------
	
	lv_obj_add_event_cb(replay_logo, replay_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(replay_logo, play_logo, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_imgbtn_set_src(replay_logo, LV_IMGBTN_STATE_RELEASED, &replaybtn, NULL, NULL);
	lv_obj_add_style(replay_logo, &style, LV_STATE_PRESSED);
	
	// -------------------- WHEEL BUTTON --------------------
	
	lv_obj_add_event_cb(wheel_logo, wheel_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(wheel_logo, replay_logo, LV_ALIGN_OUT_RIGHT_TOP, -35, 0);
	lv_imgbtn_set_src(wheel_logo, LV_IMGBTN_STATE_RELEASED, &wheelbtn, NULL, NULL);
	lv_obj_add_style(wheel_logo, &style, LV_STATE_PRESSED);
	
	// -------------------- CRONOMETRO IMG --------------------
	
	lv_img_set_src(cronometro_img, &cronometro);
	lv_obj_align(cronometro_img, LV_ALIGN_LEFT_MID, 15, 0);
	
	// -------------------- CRONOMETRO LABEL --------------------
	
	labelCron = lv_label_create(screen);
	lv_obj_align(labelCron, LV_ALIGN_LEFT_MID, 85 , 0);
	lv_obj_set_style_text_font(labelCron, &dseg30, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelCron, lv_color_black(), LV_STATE_DEFAULT);
	lv_obj_add_style(labelCron, &style, 0);
	
	// ---------------------- DIST IMG ----------------------
	
	lv_img_set_src(dist_img, &distimg);
	lv_obj_align(dist_img, LV_ALIGN_RIGHT_MID, -90, 0);
	
	// -------------------- DIST LABEL --------------------
	
	labelDist = lv_label_create(screen);
	labelDistText = lv_label_create(screen);
	lv_obj_align(labelDist, LV_ALIGN_RIGHT_MID, -45 , 0);
	lv_obj_align_to(labelDistText, labelDist, LV_ALIGN_OUT_RIGHT_TOP, 5, 0);
	lv_obj_set_style_text_font(labelDist, &dseg20, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelDist, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text(labelDistText, "KM");
	lv_obj_add_style(labelDist, &style, 0);
	
	// -------------------- VEL. MED. LABEL --------------------
	
	labelVelMed = lv_label_create(screen);
	labelVelMedText = lv_label_create(screen);
	lv_obj_align(labelVelMed, LV_ALIGN_TOP_RIGHT, -20 , 12);
	lv_obj_align_to(labelVelMedText, labelVelMed, LV_ALIGN_OUT_BOTTOM_MID, -40, 27);
	lv_obj_set_style_text_font(labelVelMed, &dseg30, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelVelMed, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelVelMed, "%2.1f", 0.0);
	lv_label_set_text(labelVelMedText, "AVG km/h");
	lv_obj_add_style(labelVelMed, &style, 0);
	
	// ----------------------- RELÓGIO LABEL ---------------------------------
	
	labelClock = lv_label_create(screen);
	lv_obj_align(labelClock, LV_ALIGN_TOP_LEFT, 55 , 5);
	lv_obj_set_style_text_font(labelClock, &dseg20, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelClock, lv_color_black(), LV_STATE_DEFAULT);
	
	// ---------------------- VEL. INST. IMG ----------------------
	
	lv_img_set_src(velinst_img, &velintimg);
	lv_obj_align(velinst_img, LV_ALIGN_TOP_LEFT, 10, 38);
	
	// -------------------- VEL. INST. LABEL --------------------
	
	labelVelInst = lv_label_create(screen);
	labelVelInstText = lv_label_create(screen);
	lv_obj_align(labelVelInst, LV_ALIGN_TOP_LEFT, 70, 32);
	lv_obj_align_to(labelVelInstText, labelVelInst, LV_ALIGN_OUT_BOTTOM_RIGHT, 120, 20);
	lv_obj_set_style_text_font(labelVelInst, &dseg50, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelVelInst, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelVelInst, "%2.1f", 32);
	lv_label_set_text(labelVelInstText, "km/h");
	lv_obj_add_style(labelVelInst, &style, 0);
	
	// ----------------------- LOGO IMG ---------------------------------
	
	lv_img_set_src(company_logo, &logoimg);
	lv_obj_align(company_logo, LV_ALIGN_TOP_LEFT, 10, 0);
	
	lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

/* --------------------------------------------------- TODOS OS ELEMENTOS DA TELA 2 ------------------------------------------------------------------*/

void create_scr2(lv_obj_t * screen) {

	static lv_style_t style;
	lv_style_init(&style);
	lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN );
	
	
	// -------------------- BUTTON INITS --------------------
	
	lv_obj_t * cancel_logo = lv_imgbtn_create(screen);
	lv_obj_t * confirm_logo = lv_imgbtn_create(screen);
	lv_obj_t * return_logo = lv_imgbtn_create(screen);
	lv_obj_t * dropdown = lv_dropdown_create(screen);
	lv_obj_t * wheel2_logo = lv_img_create(screen);
	
	// -------------------- RETURN BUTTON --------------------

	lv_obj_add_event_cb(return_logo, return_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(return_logo, LV_ALIGN_TOP_LEFT, 15, 5);
	lv_imgbtn_set_src(return_logo, LV_IMGBTN_STATE_RELEASED, &returnbtn, NULL, NULL);
	lv_obj_add_style(return_logo, &style, LV_STATE_PRESSED);
	
	// -------------------- WHEEL 2 BUTTON --------------------

	lv_img_set_src(wheel2_logo, &wheelimg);
	lv_obj_align(wheel2_logo, LV_ALIGN_TOP_RIGHT, -10, 5);
	
	// -------------------- CONFIRM BUTTON --------------------

	lv_obj_add_event_cb(confirm_logo, confirm_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(confirm_logo, LV_ALIGN_BOTTOM_LEFT, 15, 50);
	lv_imgbtn_set_src(confirm_logo, LV_IMGBTN_STATE_RELEASED, &confirmbtn, NULL, NULL);
	lv_obj_add_style(confirm_logo, &style, LV_STATE_PRESSED);
	
	// -------------------- CANCEL BUTTON --------------------

	lv_obj_add_event_cb(cancel_logo, cancel_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(cancel_logo, confirm_logo, LV_ALIGN_OUT_RIGHT_TOP, 90, 0);
	lv_imgbtn_set_src(cancel_logo, LV_IMGBTN_STATE_RELEASED, &cancelbtn, NULL, NULL);
	lv_obj_add_style(cancel_logo, &style, LV_STATE_PRESSED);
	
	// -------------------- DROP BUTTON --------------------

	lv_dropdown_set_options(dropdown, "17\n"
	"18\n"
	"19\n"
	"20");
	lv_obj_align(dropdown, LV_ALIGN_CENTER, -20, 0);
	lv_obj_add_event_cb(dropdown, drop_handler, LV_EVENT_ALL, NULL);
	
	lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

}


/* ----------------------------------------------------------- TASKS -----------------------------------------------------------*/


static void task_lcd(void *pvParameters) {
	int px, py;
	
	scr1  = lv_obj_create(NULL);
	create_scr(scr1);
	lv_scr_load(scr1);

	for (;;)  {
		lv_tick_inc(50);
		lv_task_handler();
		vTaskDelay(50);
		if (xSemaphoreTake(xSemaphoreWheel, 0)){
			printf("CLIQUEI \n");
			scr2  = lv_obj_create(NULL);
			create_scr2(scr2);
			lv_scr_load(scr2);
		}
			
		if (xSemaphoreTake(xSemaphoreReturn, 0)) {
			lv_scr_load(scr1);
		}
		
		if ((xSemaphoreTake(xSemaphoreConfirm, 0)) || (xSemaphoreTake(xSemaphoreCancel, 0))){
			lv_scr_load(scr1);
		}
	}
}


float kmh_to_hz(float vel, float raio) {
	float f = vel / (2*PI*raio*3.8);
	return(f);
}

static void task_simulador(void *pvParameters) {

	pio_set_output(PIOC, PIO_PC31, 1, 0, 0);

	float vel = VEL_MAX_KMH;
	float f;
	int ramp_up = 1;

	while(1){
		pio_clear(PIOC, PIO_PC31);
		delay_ms(5);
		pio_set(PIOC, PIO_PC31);
		#ifdef RAMP
		if (ramp_up) {
			//printf("[SIMU] ACELERANDO %d \n", (int) (10*vel));
			vel += 0.3;
		} else {
			//printf("[SIMU] DESACELERANDO %d \n",  (int) (10*vel));
			vel -= 0.3;
		}

		if (vel > VEL_MAX_KMH)
		ramp_up = 0;
		else if (vel < VEL_MIN_KMH)
		ramp_up = 1;
		#endif
		f = kmh_to_hz(vel, RAIO);
		int t = 1000*(1.0/f);
		//printf("TEMPO : %d", t);
		delay_ms(t);
	}
}




static void task_RTC(void *pvParameters) {
	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_SR_SEC|RTC_SR_ALARM);

	//xSemaphoreTake(xSemaphoreRTC, 1000
	for (;;) {
		rtc_get_time(RTC, &current_hour,&current_min, &current_sec);

		/* aguarda por tempo inderteminado até a liberacao do semaforo */
		if (xSemaphoreTake(xSemaphoreRTC, 1000 / portTICK_PERIOD_MS)){
			lv_label_set_text_fmt(labelClock, "%02d:%02d", current_hour, current_min);
			} else {
			lv_label_set_text_fmt(labelClock, "%02d %02d", current_hour, current_min);
		}
		
	}
	
}


static void task_TC(void *pvParameters) {	
	int play_ativado = 0;
	
	static lv_style_t style;
	lv_style_init(&style);
	
	for (;;) {
		
		if (xSemaphoreTake(xSemaphorePlay, 0)){
			play_ativado =! play_ativado;
			if (play_ativado){
				TC_init(TC0, ID_TC1, 1, 1);
				tc_start(TC0, 1);				
			} else {
				tc_stop(TC0, 1);
			}
		}
				
			//play_ativado = 1;
				
			//lv_obj_t * ready_logo = lv_img_create(scr1);
			//lv_img_set_src(ready_logo, &ready_img);
			//lv_obj_align(ready_logo, LV_ALIGN_BOTTOM_RIGHT, 70, 15);
			//
			//lv_obj_t * stop_logo = lv_img_create(scr1);
			//lv_obj_add_event_cb(stop_logo, stop_handler, LV_EVENT_ALL, NULL);
			//lv_obj_align(stop_logo, LV_ALIGN_BOTTOM_LEFT, 15, 60);
			//lv_imgbtn_set_src(stop_logo, LV_IMGBTN_STATE_RELEASED, &stopbtn, NULL, NULL);
			//lv_obj_add_style(stop_logo, &style, LV_STATE_PRESSED);	

		
		if (xSemaphoreTake(xSemaphoreReplay,0)){
			tc_stop(TC0, 1);
			alarm_sec = 0;
			alarm_min = 0;
			alarm_h = 0;
		}


		/* aguarda por tempo inderteminado até a liberacao do semaforo */
		if (xSemaphoreTake(xSemaphoreTC, 1000)){
			lv_label_set_text_fmt(labelCron, "%02d:%02d", alarm_h, alarm_min);
		} else {
			lv_label_set_text_fmt(labelCron, "%02d %02d", alarm_h, alarm_min);
		}
		
	}
	
}

void SIMULADOR_callback(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoredT, &xHigherPriorityTaskWoken);
	
}


static void task_calculos(void *pvParameters) {
	
	//printf("DESCIDA\n");
	TC_init(TC1, ID_TC4, 1, 1);
	tc_start(TC1, 1);
	

	int counter = 0;
	double dist = 0;
	double tempo_antigo_s = 0;
	double dt;
	double v;
	double v_metros;
	int pulsos;
	double vel_total = 0;
	double vel_anterior = 0;
	double vel_media = 0;
	
	RTT_init(100, 0, RTT_MR_RTTINCIEN);
	int toggle = 0;

	for(;;){
		if (xSemaphoreTake(xSemaphoredT, 5000)){
			pulsos = rtt_read_timer_value(RTT);
			double tempo_s = pulsos/100.0;
			
			dt = tempo_s - tempo_antigo_s;
			tempo_antigo_s = tempo_s;
			
			double f = 1/(double)dt;
			double w = 2*PI/dt;  // vel angular
			double v_metros = (RAIO*w); //vel linear em [m/s]
			double v = (RAIO*w)*3.6; //vel linear em [km/h]

			dist += (2*PI*RAIO*dt)/1000.0; //era em metros, coloquei pra km
			lv_label_set_text_fmt(labelDist, "%.1f", dist);
			

			double acel = (v_metros - vel_anterior)/(double)dt ;// aceleração em metros/s^2
			
			
			vel_anterior = v_metros;
			
			
			if (xSemaphoreTake(xSemaphoreVelMedia, 0)){
				vel_media = vel_total/(double)counter;
				lv_label_set_text_fmt(labelVelMed, "%2.1f", vel_media);
				printf("Velocidade média [km/h]: %2.1f \n", vel_media);
				vel_total = 0;
				segundos_vel_media = 0;
				counter = 0;
			} else {
				counter+=1;
				vel_total += v;
			}
			
			
			printf("%2.1f \n", dt);
			printf("VEL [km/h]: %2.1f \n", v);
			lv_label_set_text_fmt(labelVelInst, "%2.1f", v);

			printf("Distância [km]: %2.1f \n", dist);
			printf("Aceleração [m/s^2]: %2.1f \n", acel);
			
			if( acel >= 0.1) {
				lv_obj_t * acel_logo = lv_img_create(scr1);
				lv_img_set_src(acel_logo, &acel_img);
				lv_obj_align(acel_logo, LV_ALIGN_TOP_RIGHT, -100, 15);
				pio_clear(GREEN_PIO, GREEN_IDX_MASK);
				pio_set(BLUE_PIO, BLUE_IDX_MASK);
				pio_set(RED_PIO, RED_IDX_MASK);
			}
			
			else if (acel < 0) {
				lv_obj_t * desacel_logo = lv_img_create(scr1);
				lv_img_set_src(desacel_logo, &desacel_img);
				lv_obj_align(desacel_logo, LV_ALIGN_TOP_RIGHT, -100, 15);
				pio_clear(RED_PIO, RED_IDX_MASK);
				pio_set(BLUE_PIO, BLUE_IDX_MASK);
				pio_set(GREEN_PIO, GREEN_IDX_MASK);
			}
			
			else if (acel == 0) {
				lv_obj_t * stable_acel_logo = lv_img_create(scr1);
				lv_img_set_src(stable_acel_logo, &stable_img);
				lv_obj_align(stable_acel_logo, LV_ALIGN_TOP_RIGHT, -100, 15);
				pio_clear(BLUE_PIO, BLUE_IDX_MASK);
				pio_set(RED_PIO, RED_IDX_MASK);
				pio_set(GREEN_PIO, GREEN_IDX_MASK);
			}
			
		} else {
			lv_label_set_text_fmt(labelVelInst, "%2.1f", 0.0);
			vel_total += 0;
			counter +=1;
		}
		
	}
	
}


/* ---------------------------------------------- HANDLERS DOS PERIFÉRICOS: TC, RTT E RTC ----------------------------------------------*/


void TC1_Handler(void) {
	
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
		
	if (alarm_sec == 59 && alarm_min != 59){
		alarm_min += 1;
		alarm_sec = 0;
	} if (alarm_min == 59 && alarm_sec == 59) {
		alarm_h += 1; 
		alarm_min = 0;
		alarm_sec = 0;
	}
	else{
		alarm_sec += 1;
	}
	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreTC, &xHigherPriorityTaskWoken);

}

void TC4_Handler(void) {
	volatile uint32_t status = tc_get_status(TC1, 1);

	segundos_vel_media += 1;
	if (segundos_vel_media > 10){
		segundos_vel_media = 0;
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreVelMedia, &xHigherPriorityTaskWoken);
	} else {
		segundos_vel_media++;
	}
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		
	}
}


void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreRTC, &xHigherPriorityTaskWoken);
		
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}


/* ---------------------------------------------------- INITS DOS PERIFÉRICOS: TC, RTT E RTC ----------------------------------------- */


void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}

void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

/* -------------------------------------------------------------- INIT DO PINO CONFIGURADO PARA LEITURA DO SINAL --------------------------- */

void init(void) {
	pmc_enable_periph_clk(BUT_PIO_ID);
	pio_set_input(BUT_PIO,BUT_PIO_IDX_MASK,PIO_DEFAULT);
	pio_set_debounce_filter(BUT_PIO, BUT_PIO_IDX_MASK, 60);
		
	pmc_enable_periph_clk(BLUE_PIO_ID);
	pmc_enable_periph_clk(RED_PIO_ID);
	pmc_enable_periph_clk(GREEN_PIO_ID);
	
	pio_set_output(BLUE_PIO, BLUE_IDX_MASK, 1, 0, 0);
	pio_set_output(RED_PIO, RED_IDX_MASK, 1, 0, 0);
	pio_set_output(GREEN_PIO, GREEN_IDX_MASK, 1, 0, 0);		
		
	pio_handler_set(BUT_PIO,
	BUT_PIO_ID,
	BUT_PIO_IDX_MASK,
	PIO_IT_FALL_EDGE,
	SIMULADOR_callback);
		
	pio_enable_interrupt(BUT_PIO, BUT_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO);

		
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 4); // Prioridade 4
	
	
	//pmc_enable_periph_clk(SIMULADOR_PIO_ID);
	//pio_set_input(SIMULADOR_PIO,SIMULADOR_IDX_MASK,PIO_DEFAULT);
	
	//pio_handler_set(SIMULADOR_PIO,
	//SIMULADOR_PIO_ID,
	//SIMULADOR_IDX_MASK,
	//PIO_IT_FALL_EDGE,
	//SIMULADOR_callback);
	//
	//pio_enable_interrupt(SIMULADOR_PIO, SIMULADOR_IDX_MASK);
	//pio_get_interrupt_status(SIMULADOR_PIO);
	//
	//NVIC_EnableIRQ(SIMULADOR_PIO_ID);
	//NVIC_SetPriority(SIMULADOR_PIO_ID, 4); // Prioridade 4
}


/* ------------------------------------------------- CONFIGS DO LCD E CONSOLE  -------------------------------------------------------- */

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


/* --------------------------------------------------- PORT LVGL   ------------------------------------------------------ */

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


/* ----------------------------------------------------- MAIN ---------------------------------------------------------------------  */

int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();
	init();
	
	xSemaphoreRTC = xSemaphoreCreateBinary();
	if (xSemaphoreRTC == NULL)
		printf("falha em criar o semaforo \n");	

	xSemaphoreTC = xSemaphoreCreateBinary();
	if (xSemaphoreTC == NULL)
		printf("falha em criar o semaforo \n");

	xSemaphoredT = xSemaphoreCreateBinary();
	if (xSemaphoredT == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphorePlay = xSemaphoreCreateBinary();
	if (xSemaphorePlay == NULL)
		printf("falha em criar o semaforo \n");
		
	xSemaphoreStop = xSemaphoreCreateBinary();
	if (xSemaphoreStop == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreReplay = xSemaphoreCreateBinary();
	if (xSemaphoreReplay == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreWheel = xSemaphoreCreateBinary();
	if (xSemaphoreWheel == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreConfirm = xSemaphoreCreateBinary();
	if (xSemaphoreConfirm == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreCancel = xSemaphoreCreateBinary();
	if (xSemaphoreCancel == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreReturn = xSemaphoreCreateBinary();
	if (xSemaphoreReturn == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreVelMedia = xSemaphoreCreateBinary();
	if (xSemaphoreReturn == NULL)
		printf("falha em criar o semaforo \n");

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();
	
	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}

	if (xTaskCreate(task_calculos, "CALCULOS", TASK_CALC_STACK_SIZE, NULL, TASK_CALC_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}

// 	if (xTaskCreate(task_simulador, "Simulador", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
// 		printf("Failed to create simulador task\r\n");
// 	}
// 	
	if (xTaskCreate(task_RTC, "RTC", TASK_RTC_STACK_SIZE, NULL, TASK_RTC_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create test led task\r\n");
	}

	if (xTaskCreate(task_TC, "TC", TASK_TC_STACK_SIZE, NULL, TASK_TC_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create test led task\r\n");
	}		
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}