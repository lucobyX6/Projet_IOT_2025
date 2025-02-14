/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
#include "spi.h"
#include "debug.h"
//#include "init.h"
#include "lmic.h"
#include "hal.h"
#include "oslmic.h"
#include "lorabase.h"

#include <bme680/bme68x_necessary_functions.h>
#include "string.h"
#include "cayenne_lpp.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
// application router ID (LSBF) < ------- IMPORTANT
static const u1_t APPEUI[8] = {0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// unique device ID (LSBF) < ------- IMPORTANT
static const u1_t DEVEUI[8] = {0xDD, 0xD5, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
// device-specific AES key (derived from device EUI (MSBF))
static const u1_t DEVKEY[16] = {0x3A, 0xB2, 0x6C, 0x0E, 0x37, 0x52, 0xD6, 0xE0, 0xAD, 0x6F, 0x57, 0x65, 0x67, 0x16, 0xAC, 0x44};

cayenne_lpp_t lpp;
struct bme68x_data data;
int temp;
//APPEUI,DEVEUI must be copied from thethingsnetwork application datas in LSB format
//-----------------------------------------------------------------------------------------
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// counter
static int cnt = 0;
// static osjob_t hellojob;
static void hellofunc ( osjob_t* j){
	//say hello
	debug_str("Hello World\r\n");
	//log counter
	debug_val("cnt == ",cnt);
	//toggle Led
	debug_led(++cnt & 1);
	//reshedule jub every second
	os_setTimedCallback(j, os_getTime()+sec2osticks(1), hellofunc);
}


// provide application router ID (8 bytes, LSBF)
void os_getArtEui (u1_t* buf) {
	memcpy(buf, APPEUI, 8);
}
// provide device ID (8 bytes, LSBF)
void os_getDevEui (u1_t* buf) {
	memcpy(buf, DEVEUI, 8);
}
// provide device key (16 bytes)
void os_getDevKey (u1_t* buf) {
	memcpy(buf, DEVKEY, 16);
}
void initsensor(){
	HAL_ADC_Init(&hadc1);
// Here you init your sensors
}



void initfunc (osjob_t* j) {
	// intialize sensor hardware
	initsensor();
	// reset MAC state
	LMIC_reset();
	// start joining
	LMIC_startJoining();
	// init done - onEvent() callback will be invoked...
}



static osjob_t reportjob;
// report sensor value every minute
//remplacer ce qui suis avec fonction librairie cayenne
static void reportfunc (osjob_t* j) {
	bme68x_start(&data, &hi2c1);

		if (bme68x_single_measure(&data) == 0)
		{

			// Measurement is successful, so continue with IAQ
			data.iaq_score = bme68x_iaq(); // Calculate IAQ


			//data.temperature, data.pressure, data.humidity, data.iaq_score, data.gas_resistance;
			temp = data.temperature;
²
			cayenne_lpp_reset(&lpp);

			cayenne_lpp_add_temperature(&lpp, 1, data.temperature);

			LMIC_setTxData2(1, getBuffer(&lpp), lpp->cursor /* Taille du curseur */ , 0);


		}



		// reschedule job in 60 seconds
		os_setTimedCallback(j, os_getTime()+sec2osticks(15), reportfunc);
}



//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////
void onEvent (ev_t ev) {
	debug_event(ev);
	switch(ev) {
	// network joined, session established
		case EV_JOINING:
			debug_str("try joining\r\n");
//			blinkfunc(&blinkjob);
			break;
		case EV_JOINED:
			// kick-off periodic sensor job
			reportfunc(&reportjob);
			debug_led(1);
			break;
		case EV_JOIN_FAILED:
			debug_str("join failed\r\n");
			break;
		case EV_SCAN_TIMEOUT:
			debug_str("EV_SCAN_TIMEOUT\r\n");
			break;
		case EV_BEACON_FOUND:
			debug_str("EV_BEACON_FOUND\r\n");
			break;
		case EV_BEACON_MISSED:
			debug_str("EV_BEACON_MISSED\r\n");
			break;
		case EV_BEACON_TRACKED:
			debug_str("EV_BEACON_TRACKED\r\n");
			break;
		case EV_RFU1:
			debug_str("EV_RFU1\r\n");
			break;
		case EV_REJOIN_FAILED:
			debug_str("EV_REJOIN_FAILED\r\n");
			break;
		case EV_TXCOMPLETE:
			debug_str("EV_TXCOMPLETE (includes waiting for RX windows)\r\n");
			if (LMIC.txrxFlags & TXRX_ACK)
				debug_str("Received ack\r\n");
			if (LMIC.dataLen) {
				debug_str("Received ");
				debug_val("Data length", LMIC.dataLen);
				debug_str(" bytes of payload\r\n");
			}
			break;
		case EV_LOST_TSYNC:
			debug_str("EV_LOST_TSYNC\r\n");
			break;
		case EV_RESET:
			debug_str("EV_RESET\r\n");
			break;
		case EV_RXCOMPLETE:
			// data received in ping slot
			debug_str("EV_RXCOMPLETE \r\n");
			break;
		case EV_LINK_DEAD:
			debug_str("EV_LINK_DEAD \r\n");
			break;
		case EV_LINK_ALIVE:
			debug_str("EV_LINK_ALIVE \r\n");
			break;
		default:
			debug_str("Unknown event \r \n");
			reportfunc(&reportjob);
			break;
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM7_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_SPI3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim7);
  __HAL_SPI_ENABLE(&hspi3);
  osjob_t initjob;
  os_init();
  debug_init();

//  os_setCallback(&hellojob, hellofunc);
  os_setCallback(&initjob, initfunc);

  os_runloop();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
