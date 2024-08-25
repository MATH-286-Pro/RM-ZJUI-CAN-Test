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
#include "can.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rc.h"
#include "motors.h"
#include "buzzer.h"
#include "pid.h"
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
extern RC_Type rc;
extern float Shooter_Velocity[2]; // 单位: rpm
extern float Loader_Velocity;     // 单位: rpm
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_DMA_Init();
  MX_USART3_UART_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM7_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  //GPIO 点灯
  HAL_GPIO_WritePin(GPIOH, LED_R_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOH, LED_G_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOH, LED_B_Pin, GPIO_PIN_RESET);

  // 亮红灯
  HAL_GPIO_WritePin(GPIOH, LED_R_Pin, GPIO_PIN_SET);

  // 初始化
  // Buzzer_beep();
  Dbus_Init();     // 初始化DJI遥控器
  Enable_Motors(); // 初始化DJI电机
  HAL_Delay(1000);

  // PID 参数填装
  pid_type_def shoot_pram_up;    // PID 初始化
  pid_type_def shoot_pram_down;  // PID 初始化
  pid_type_def shoot_pram_trig;  // PID 初始化
  static const float PID_ARG[3] = {80.0f,0.0f,30.0f};
  static const float PID_ARG_trig[3] = {50.0f,0.0f,40.0f};
  static const float PID_MAX_OUT = 1000.0f;
  static const float PID_MAX_IOUT = 0.0f; 

  static const float shoot_rpm = 300; // 发射速度
  float load_rpm = 400.0f; // 装弹速度

  // PID 初始化
  PID_init(&shoot_pram_up, PID_POSITION, PID_ARG, PID_MAX_OUT, PID_MAX_IOUT);
  PID_init(&shoot_pram_down, PID_POSITION, PID_ARG, PID_MAX_OUT, PID_MAX_IOUT);
  PID_init(&shoot_pram_trig, PID_POSITION, PID_ARG_trig, 1000.0f, 0.0f);

  // 亮绿灯
  HAL_GPIO_WritePin(GPIOH, LED_R_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOH, LED_G_Pin, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    if(rc.sw1 == 1){HAL_GPIO_WritePin(GPIOH,LED_B_Pin,GPIO_PIN_SET);}
    else{HAL_GPIO_WritePin(GPIOH,LED_B_Pin,GPIO_PIN_RESET);}
    HAL_GPIO_TogglePin(GPIOH,LED_G_Pin);

    // 8-下摩擦轮 (正方向) 
    // 7-上摩擦轮 (负方向)
    // 6-拨弹盘

    PID_calc(&shoot_pram_up, Shooter_Velocity[0], shoot_rpm*-(rc.sw1/2));
    PID_calc(&shoot_pram_down, Shooter_Velocity[1], shoot_rpm*(rc.sw1/2));
    PID_calc(&shoot_pram_trig, Loader_Velocity, load_rpm*rc.RY);

    Gimbal_CAN_Tx(0,shoot_pram_trig.out,shoot_pram_up.out,shoot_pram_down.out); // 根据 CAN 分析仪发现没有发送 CAN 信号
    HAL_Delay(50);

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
