/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "socket.h"
#include "dhcp.h"
#include "httpServer.h"
#include "MQTTClient.h"
#include "mqtt_interface.h"
#include "dns.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
		#define DHCP_SOCKET     0
		#define DNS_SOCKET      6
		#define HTTP_SOCKET     2
		#define SOCK_TCPS       0
		#define SOCK_UDPS       1
		#define PORT_TCPS       5000
		#define PORT_UDPS       3000
		#define MAX_HTTPSOCK    6
		#define SOCKET_PING 		2
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
	int check[6];
	int check1;
	int8_t sock;
	uint8_t status;
	int8_t ret=2;
	int dem =0;
	char rep[10];
	//IP v� port cua server MQTT
	uint8_t server_ip[4] ;
	int SERVER_PORT  = 1883;
	//IP v� port cua goolge
	//uint8_t server_ip[4] = {8, 8, 8, 8};
	//#define SERVER_PORT 80
	uint8_t tcp_socket = 0; 
	int ret_mqtt;
	int ret_mqtt1;
	unsigned char broker_mqtt[] = "broker.hivemq.com";
	char topic_send[] = "your_topic_send";
	char topic_recv[] = "your_topic_recv";
	char *mes_recv1 ;
	char mes_recv[256];
	//---------DNS de truy xuat IP--------------------
	uint8_t dns_server[4] = {8, 8, 8, 8};  // Google DNS
	uint8_t tx_buf[512];  // ho?c �t nh?t l� 256 byte (khuy?n ngh? 512)
	//
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
	//---------struct du lieu --------------
	wiz_NetInfo net_info = {
			.mac  = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED },
			.dhcp = NETINFO_DHCP
	};
	MQTTClient client;
	Network network;
	struct Parametter_Uart{
		int baudrate;
		uint8_t stop_bit;
		uint8_t Parity_En;
		uint8_t Parity_Se;
		uint16_t Length;
		
	}Para_Uart;
	
	//----------watch dog ----
//	void MX_IWDG_Init(void)
//	{
//    hiwdg.Instance = IWDG;
//    hiwdg.Init.Prescaler = IWDG_PRESCALER_64; // chia LSI (~40kHz)
//    hiwdg.Init.Reload = 1875;  // timeout = 1875 � 64 / 40_000 = ~3 gi�y
//    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
//    {
//        Error_Handler();
//    }
//	}
	//--------------function-------------------
	unsigned char sendbuf[512], readbuf[512];  // buffer cho MQTT

	void wizchipSelect(void) {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	}

	void wizchipUnselect(void) {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	}

	void wizchipReadBurst(uint8_t* buff, uint16_t len) {
			HAL_SPI_Receive(&hspi1, buff, len, HAL_MAX_DELAY);
	}

	void wizchipWriteBurst(uint8_t* buff, uint16_t len) {
			HAL_SPI_Transmit(&hspi1, buff, len, HAL_MAX_DELAY);
	}

	uint8_t wizchipReadByte(void) {
			uint8_t byte;
			wizchipReadBurst(&byte, sizeof(byte));
			return byte;
	}

	void wizchipWriteByte(uint8_t byte) {
			wizchipWriteBurst(&byte, sizeof(byte));
	}

	volatile bool ip_assigned = false;

	void Callback_IPAssigned(void) {
			ip_assigned = true;
	}

	void Callback_IPConflict(void) {
			ip_assigned = false;
	}

	uint8_t dhcp_buffer[1024];
	uint8_t dns_buffer[1024];
	void W5500Init() {
			// Register W5500 callbacks
			reg_wizchip_cs_cbfunc(wizchipSelect, wizchipUnselect);
			reg_wizchip_spi_cbfunc(wizchipReadByte, wizchipWriteByte);
			reg_wizchip_spiburst_cbfunc(wizchipReadBurst, wizchipWriteBurst);

			uint8_t rx_tx_buff_sizes[] = {2, 2, 2, 2, 2, 2, 2, 2};
			wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);

			// set MAC address before using DHCP
			setSHAR(net_info.mac);
			DHCP_init(DHCP_SOCKET, dhcp_buffer);

			reg_dhcp_cbfunc(
					Callback_IPAssigned,
					Callback_IPAssigned,
					Callback_IPConflict
			);

			uint32_t ctr = 10000;
			while((!ip_assigned) && (ctr > 0)) {
					DHCP_run();
					ctr--;
			}
			if(!ip_assigned) {
					return;
			}

			getIPfromDHCP(net_info.ip);
			getGWfromDHCP(net_info.gw);
			getSNfromDHCP(net_info.sn);
			
			wizchip_setnetinfo(&net_info);
	}
	void get_ip(unsigned char *broker)
	{
		//tao socket DNS 
		DNS_init(DNS_SOCKET, tx_buf);  // tx_buf l� buffer 512 byte
		//g�n dia chi IP cua broker vao bien server_ip
		DNS_run(dns_server, broker, server_ip);
	}
//	int w5500_read(Network* n, unsigned char* buffer, int len, long timeout_ms) {
//			return recv(n->my_socket, buffer, len);
//	}

	int w5500_read(Network* n, unsigned char* buffer, int len, long timeout_ms)
	{
    uint32_t tickstart = HAL_GetTick();
    uint16_t recv_len = 0;
		//check[5] = 0;

    while (recv_len == 0) {
        recv_len = getSn_RX_RSR(n->my_socket);
        if (HAL_GetTick() - tickstart > 1000) {
            return 0;  // timeout
        }
    }

    if(recv_len > 0) return recv(n->my_socket, buffer, len);
		return 1;
	}
//	int w5500_write(Network* n, unsigned char* buffer, int len, long timeout_ms) {
//			return send(n->my_socket, buffer, len);
//	}
	int w5500_write(Network* n, unsigned char* buffer, int len, long timeout_ms) {
			uint32_t tick_start = HAL_GetTick();
			while (getSn_TX_FSR(n->my_socket) < len) {
					if ((HAL_GetTick() - tick_start) > 1000)
							return -1;
			}
			return send(n->my_socket, buffer, len);
	}


	int8_t connectMQTT(Network *n, uint8_t sock, uint8_t *ip, uint16_t port)
	{
			n->my_socket = sock;

			// G�n h�m d?c / ghi cho network
			n->mqttread = w5500_read;
			n->mqttwrite = w5500_write;

			// M? socket TCP
			int8_t s = socket(sock, Sn_MR_TCP, 50000, 0);
			if (s != sock) return -1;

			ret = connect(sock, ip, port);
			if (ret != SOCK_OK) return -2;

			// �?i d?n khi k?t n?i th�nh c�ng
			uint32_t t = HAL_GetTick();
			while (getSn_SR(sock) != SOCK_ESTABLISHED) {
					if (HAL_GetTick() - t > 3000) return -3; // timeout
			}

			return 1; // OK
	}
	void connect_to_server()
	{
		connectMQTT(&network, tcp_socket, server_ip, SERVER_PORT);

		MQTTClientInit(&client, &network, 3000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));
		MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
		data.MQTTVersion = 3;
		//data.keepAliveInterval = 30; 
		data.clientID.cstring = "STM32_Client";
		data.username.cstring = "your_username";
		data.password.cstring = "your_password";

		check[0] = MQTTConnect(&client, &data);
	}
	void pub_message(char *topic, char *mes)
	{
		MQTTMessage message;
		message.qos = QOS0;  // ho?c QOS1, QOS2 t�y broker h? tr?
		message.retained = 0;
		message.dup = 0;
		message.payload = mes;
		message.payloadlen = strlen(mes);

		MQTTPublish(&client, topic , &message);
	}
	void UART1_config()
	{
		//config UART
		RCC -> APB2ENR |= (1<<2)|(1<<14); //EN CLK USART1, PA
		GPIOA -> CRH &= ~(0xFF << 4); //clear PA9,10
		GPIOA -> CRH |= (9<<4); // PA9 TX, Output AF mode 
		GPIOA-> CRH |= (8<<8); // PA10 RX, Input floating mode
		//calculation and set baudrate
		uint32_t CLK_APB = HAL_RCC_GetPCLK2Freq();
		float brr = (float)CLK_APB/(16*(float)Para_Uart.baudrate);
		int div_man = (int)brr;
		int div_fra = round(brr*10) - div_man*10;
		USART1->BRR = (div_man << 4) | (div_fra<<0); 
		//set stop bit
		if(Para_Uart.stop_bit == 1) USART1 -> CR2 &= ~(0b11 << 12); //bit 00 12:13
		else if(Para_Uart.stop_bit == 2) USART1 -> CR2 |= (0b10 << 12);
//		//en parity 
//		if(Para_Uart.Parity_En == 1) USART1 -> CR1 |= (1<<10);
//		else if(Para_Uart.Parity_En == 0) USART1 -> CR1 &= ~ (1<<10);
//		//select parity
//		if( Para_Uart.Parity_En == 1 && Para_Uart.Parity_Se == 1) USART1 -> CR1 |= (1<<9); //ODD
//		else if( Para_Uart.Parity_En == 1 && Para_Uart.Parity_Se == 0) USART1 -> CR1 &= ~(1<<9); //EVEN
		if(Para_Uart.Parity_Se == 0 ) USART1 -> CR1 &= ~(0b11 << 9);
		else if(Para_Uart.Parity_Se == 1 ) {USART1 -> CR1 &= ~(0b11 << 9); USART1 -> CR1 |= (0b11 << 9);}
		else if(Para_Uart.Parity_Se == 2 ) { USART1 -> CR1 &= ~(0b11 << 9); USART1 -> CR1 |= (0b10 << 9);}
		
		//set length
		if(Para_Uart.Length == 7) USART1 -> CR1 &= ~(1<<12); //8 bit = 7 bit data + (1bit patiry)
		else if(Para_Uart.Length == 8) USART1 -> CR1 |= (1<<12); // 9 bit = 8 bit data + (1 bit parity)
	
		USART1->CR1 |= (1<<2)|(1<<3)|(1<<5)|(1<<13); // enable TX, RX, USART 
		NVIC->ISER[1] |= (1<<5); // enable global interrupt USART1 
	}
	void check_set()
	{
			char *mes = mes_recv; 
			char *token;
			int dem = 0;
			
			token=strtok(mes_recv,":");
			while(token != NULL )
			{
				++dem;
				switch (dem) {
					case 2:
						Para_Uart.baudrate = atoi(token);
						break;
					case 3:
						Para_Uart.stop_bit = atoi(token);
						break;
//					case 4:
//						Para_Uart.Parity_En = atoi(token);
//						break;
					case 4:
						Para_Uart.Parity_Se = atoi(token);
						break;
					case 5:
						Para_Uart.Length = atoi(token);
						break;
				}
				token = strtok(NULL, ":");
			}
			UART1_config();
  }
	void send_char (char data) 
	{ 
		while ((USART1->SR & (1<<6)) ==0) {}
		USART1-> DR = data; 
	} 
	void send_data (char *str)  //gui chuoi den terminal
	{while (*str) send_char(*str++);
	{
	// get a byte data from USART  
	} }
	void messageArrived(MessageData* md)
	{
    MQTTMessage* message = md->message;

    memset(mes_recv, 0, sizeof(mes_recv));
    memcpy(mes_recv, message->payload, message->payloadlen);
    mes_recv[message->payloadlen] = '\0';
		
//		
		if(strstr(mes_recv, "COM_SET" ) != NULL ) {check_set(); pub_message(topic_recv, "OK");}
		else {send_data(mes_recv); sprintf(rep ,"OK %d", dem++); pub_message(topic_recv, rep );}
		//else if(Para_Uart.baudrate != 0) {send_data(mes_recv); sprintf(rep ,"OK %d", dem++); pub_message(topic_recv, rep );}
		//else pub_message(topic_recv, "CHUA SET UART");
//		else 	{sprintf(rep ,"OK %d", dem++); pub_message(topic_recv, rep );}
	}
	void mesduytri(MessageData *md)
	{
		MQTTMessage* message = md->message;
	}
	void UART1_Init()
	{
		//config UART
		RCC -> APB2ENR |= (1<<2)|(1<<14); //EN CLK USART1, PA
		GPIOA -> CRH &= ~(0xFF << 4); //clear PA9,10
		GPIOA -> CRH |= (9<<4); // PA9 TX, Output AF mode 
		GPIOA-> CRH |= (8<<8); // PA10 RX, Input floating mode
		//
		USART1->BRR = (234 << 4) | (4 <<0); // baudrate 9600 
		USART1->CR1 |= (1<<2)|(1<<3)|(1<<5)|(1<<13); // enable TX, RX, USART 
		NVIC->ISER[1] |= (1<<5); // enable global interrupt USART1 
	}
	char rx_data[256];
	int i=0;
	void USART1_IRQHandler (void) 
	{  
		rx_data[i] = USART1 -> DR;
		//-------------YC1----------------- //gui tu terminal den vxl, gui so 1 thi led don sang, 0 led don tat
		//if(USART1 -> DR == '1') {GPIOB -> ODR &= ~(1<<11);}
		//else if(USART1 -> DR == '0') {GPIOB -> ODR |= (1<<11);}
		//------------------------------------
		i++;
		if(i==5) {i=0; pub_message(topic_recv, "CONNECTING");
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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
	check[0] = 2;
	W5500Init();
//	get_ip(broker_mqtt);
//	connect_to_server();
//	MQTTSubscribe(&client, topic_send, QOS0, messageArrived);
	//MQTTSubscribe(&client, "duytri", QOS0, 
	//pub_message(topic_recv,"HELLO FROM STM");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	//MX_IWDG_Init();  // B?t d?u watchdog
	int last,now;
  while (1)
  {
//		last = HAL_GetTick();
//		//check1++;
//		ret_mqtt = getSn_SR(tcp_socket);
//		if(ret_mqtt == 0x17 ) {ret = MQTTYield(&client, 1000);} // nhan v� xu l� message tu broker
//		//HAL_IWDG_Refresh(&hiwdg);}
//		//HAL_IWDG_Refresh(&hiwdg);
//		if(ret != SUCCESS ) { check[3] = 5; }
//		//else pub_message(topic_send,"CONNECTING");
//		now = HAL_GetTick();
//		check[4] = client.isconnected;
//		check[1]++;
//		while(now - last < 4000 ) {now = HAL_GetTick();} //delay 1s
//		 pub_message(topic_send,"a");
////		check1++;
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
#ifdef USE_FULL_ASSERT
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
