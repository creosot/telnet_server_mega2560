#include <EEPROM.h>
#include <Wire.h>
#include <UIPEthernet.h>
#include <BH1750.h>
#include <stdlib.h> 


BH1750 lightMeter;

EthernetServer server(23);
EthernetClient client;

uint8_t telnet_server();
void clearTelnetSymbolBuffer();
void telnet_close_connection();
void display_main_print();
void display_l();
void l_print();
void printLux();
void lightStream();
void display_g();
void g_print();
void default_print();
uint8_t readLightSettingFromEEPROM();
bool inputLightSetting();
void printLightSettings();
void writeLightSettingToEEPROM();
void printLightSettingsFromEEPROM();
void EEPROM_long_write(int addr, long num);

//long EEMEM min_light_level_addr;
//long EEMEM max_light_level_addr;
//long EEMEM min_bright_level_addr;
//long EEMEM max_bright_level_addr;
long light_led_setting_EEPROM[4] PROGMEM;
const char array_1[] PROGMEM = "MIN LIGHT level(0<x):  ";
const char array_2[] PROGMEM = "MAX LIGHT level(x<65000):  ";
const char array_3[] PROGMEM = "MIN LED level(0<x):  ";
const char array_4[] PROGMEM = "MAX LED level(x<100):  ";
const char* const input_messages[] PROGMEM = {
	array_1,
	array_2,
	array_3,
	array_4,
};
long light_led_setting[4] = { 200, 2500, 20, 80 };
char print_buffer[40];
uint8_t mac[6] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
uint8_t count_s = 0;
#define textBuffSize 4
char textBuff[textBuffSize];
uint8_t charsReceived = 0;
unsigned long timeOfLastActivity;  //time in milliseconds of last activity
unsigned long allowedConnectTime = 300000;  //five minutes
float lux = -2;

void setup()
{
	Serial.begin(9600);
	delay(1000);
	//BH Init
//	Wire.begin();
//	delay(1000);
//	Serial.print(F("\n\rController Init\n\r"));
//	delay(1000);
//	
//	while (1) {
//		Serial.print(F("BH Init\n\r"));
//		lightMeter.begin();
//		delay(500);
//		lux = lightMeter.readLightLevel();
//		if (lux != -2)
//		{
//			Serial.println(F("BH1750 Ok"));
//			break;
//		}
//		delay(5000);
//	}
//	
//	//Ethernet Init
//	while (1) {
//		Serial.println(F("Ethernet Init"));
//		int ret = Ethernet.begin(mac);
//		if (ret != 0)
//		{
//			Serial.println(F("Ethernet Ok"));
//			break;
//		}
//		Serial.println(F("Not DHCP adress"));
//		Serial.print(F("localIP: "));
//		Serial.println(Ethernet.localIP());
//	}
//	Serial.print(F("localIP: "));
//	Serial.println(Ethernet.localIP());
//	Serial.print(F("subnetMask: "));
//	Serial.println(Ethernet.subnetMask());
//	Serial.print(F("gatewayIP: "));
//	Serial.println(Ethernet.gatewayIP());
//	Serial.print(F("dnsServerIP: "));
//	Serial.println(Ethernet.dnsServerIP());
//	Serial.println("");
//	Serial.println("");
//	
//	server.begin();
	
	Serial.println("Read EEPROM");
	
	uint8_t res = readLightSettingFromEEPROM();
	switch (res)
	{
	case 0:	Serial.println("OK"); break;
	case 1: Serial.println("eepron not prog"); break;
	case 2: Serial.println("error min>max"); break;	
	default:
		break;
	}
	printLightSettingsFromEEPROM();
	Serial.println();
	inputLightSetting();
	printLightSettings();
	writeLightSettingToEEPROM();
	printLightSettingsFromEEPROM();
}

void loop()
{
	
	//telnet_server();
}

bool inputLightSetting()
{
	long digit[4];
	for (uint8_t i = 0; i < 4; i++)
	{
		char rx_byte;
		char in[6] = { 0 };
		uint8_t in_count = 0;
		strcpy_P(print_buffer, (char*)pgm_read_word(&(input_messages[i])));
		Serial.print(print_buffer);
		Serial.print(F("\tCurrent value: "));
		Serial.println(light_led_setting[i]);
		Serial.println(F("Input number or Enter to leave the current value"));
		Serial.print("input: ");
		while (true)
		{
			if (Serial.available() > 0) {
				rx_byte = Serial.read();          // get the character
			
				if((rx_byte >= '0') && (rx_byte <= '9')) {
					Serial.print(rx_byte);
					in[in_count++] = rx_byte;
					if (in_count == 5)
						break;
				}
				else if((rx_byte == '\n') || (rx_byte == '\r'))
				{
					break;
				} 
				else
				{
					Serial.print(rx_byte);
					Serial.println(F(" Not a number."));
					Serial.print("input: ");
					for (uint8_t i = 0; i < in_count; i++)
					{
						Serial.print(in[i]);
					}
				} 
			}  
		}
		if (in_count)
		{
			digit[i] = strtol(in, 0, 0);
			Serial.print("\n\rnumber is: ");
			Serial.println(digit[i]);
		}
		else
		{
			digit[i] = light_led_setting[i];
			Serial.print(F("\n\rnumber is : "));
			Serial.println(light_led_setting[i]);
		}
		Serial.println();
	}
	if (digit[0] > digit[1] || digit[2] > digit[3])
	{
		Serial.println(F("Error MIN value > MAX value.\n\rthe values are not accepted enter again"));
		return false;
	}
	for (uint8_t i = 0; i < 4; i++)
	{
		light_led_setting[i] = digit[i];
	}
	return true;
}

void printLightSettings()
{
	Serial.print(F("\n\rCurrent values are set to:\n\r"));
	for (uint8_t i = 0; i < 4; i++)
	{
		strcpy_P(print_buffer, (char*)pgm_read_word(&(input_messages[i])));
		Serial.print(print_buffer);
		Serial.println(light_led_setting[i]);
	}
}

void printLightSettingsFromEEPROM()
{
	Serial.print(F("\n\rEEPROM light settings values:\n\r"));
	for (uint8_t i = 0; i < 4; i++)
	{
		strcpy_P(print_buffer, (char*)pgm_read_word(&(input_messages[i])));
		Serial.print(print_buffer);
		Serial.println(eeprom_read_dword((uint32_t *) light_led_setting_EEPROM[i]));
	}
}

void EEPROM_long_write(int addr, long num) 
{
	byte raw[4];
	(long&)raw = num;
	for(byte i = 0 ; i < 4 ; i++) EEPROM.write(addr + i, raw[i]);
}

void writeLightSettingToEEPROM()
{
	for (uint8_t i = 0; i < 4; i++)
	{
		EEPROM_long_write(light_led_setting_EEPROM[i], light_led_setting[i]);
	}
	//eeprom_write_block((void*)&light_led_setting, light_led_setting_EEPROM, sizeof(light_led_setting));
}

uint8_t readLightSettingFromEEPROM()
{
	long max, min;
	min = eeprom_read_dword((uint32_t *) light_led_setting_EEPROM[0]);
	max = eeprom_read_dword((uint32_t *) light_led_setting_EEPROM[1]);
	if (min == 0xffffffff || max == 0xffffffff)
		return 1; //eepron not prog
	if(min > max)
		return 2; //error min>max
	light_led_setting[0] = min;
	light_led_setting[1] = max;
	
	min = eeprom_read_dword((uint32_t *) light_led_setting_EEPROM[2]);
	max = eeprom_read_dword((uint32_t *) light_led_setting_EEPROM[3]);
	if (min == 0xffffffff || max == 0xffffffff)
		return 1; //eepron not prog
	if(min > max)
		return 2;  //error min>max
	light_led_setting[2] = min;
	light_led_setting[3] = max;
	return 0;
}

void printLux()
{
	lux = lightMeter.readLightLevel();
	if (lux == -2)
	{
		client.print(F("\tLight sensor not answer\n\r"));
	}
	else
	{
		client.print(F("\tLight = "));
		client.println(lux);
	}
}

void lightStream()
{
	printLux();
}

uint8_t telnet_server()
{
	client = server.available();
	if (client) 
	{
		Serial.println(F("client connected"));
		bool flg_display_main_print = false;
		client.flush();
		timeOfLastActivity = millis();
		while(client.connected())	
		{
			if (!flg_display_main_print)
			{
				display_main_print();
				client.print(F(">>"));
			}
			flg_display_main_print = true;
			
			if (millis() - timeOfLastActivity > allowedConnectTime) {
				client.print(F("\n\rTimeout disconnect.\n\r"));
				telnet_close_connection();
				return 2;
			}
			
			if (client.available()) //Возвращает количество непрочитанных байтов, принятых клиентом от сервера
			{
				textBuff[charsReceived] = (char) client.read();
				
				if (textBuff[2] == 'q')
				{
					textBuff[0] = textBuff[3];
					textBuff[1] = 0;
					textBuff[2] = 0;
					textBuff[3] = 0; 
					charsReceived = 0;
				}
				
				switch (textBuff[0])
				{
				case 'l':
				case 'L':
					display_l(); break;
				case 'g':
				case 'G':
					display_g(); break;
				case 'c':
				case 'C':
					telnet_close_connection(); return 1;
				default:
					default_print(); break;
				}
			}
			if (textBuff[3] == 'l' || textBuff[3] == 'L')
			{
				lightStream(); 
				delay(1000);
			}
		}
	}
	return 0;
}

void telnet_close_connection()
{
	client.println(F("\nBye.\n"));
	delay(1000);
	client.stop();
	Serial.println(F("client stop"));
}

void display_main_print()
{
	clearTelnetSymbolBuffer();
	client.print(F("\n\rSymbol and Enter\n\rl: LIGHTs setting\n\rg: GIDs setting\n\rc: Close connection\n\r"));
}

void clearTelnetSymbolBuffer()
{
	charsReceived = 0;
	for (uint8_t i = 0; i < textBuffSize; i++)
	{
		textBuff[i] = 0;	 
	}
}

void display_l()
{
	if (charsReceived == 0)
	{
		charsReceived = 1;
		l_print(); 
	}
	else
	{
		switch (textBuff[1])
		{
		case '1':
			printLux(); l_print(); break;
		case '2':
			l_print(); break;
		case '3':
			l_print(); break;
		case '4':
			textBuff[3] = textBuff[0]; textBuff[0] = 0; charsReceived = 2; break;	
		case 'm':
		case 'M':
			display_main_print(); break;
		default:
			default_print(); break;
		}
	}
}

void l_print()
{
	client.print(F("\n\r1: Light sensor value\n\r2: Bright display\n\r3: MAX MIN parameters\n\r4: stream\n\rm: main display\n\r"));
}

void display_g()
{
	if (charsReceived == 0)
	{
		charsReceived = 1;
		g_print(); 
	}
	else
	{
		switch (textBuff[1])
		{
		case '1':
			g_print(); break;
		case '2':
			g_print(); break;
		case 'm':
		case 'M':
			display_main_print(); break;
		default:
			default_print(); break;
		}
	}
}

void g_print()
{
	client.print(F("\n\rGIDA: \n\rGIDB: \n\r1: set GIDA\n\r2: set GIDB\n\rm: main display\n\r"));
}

void default_print()
{
	if (textBuff[charsReceived] == '\n' || textBuff[charsReceived] == '\r')
	{
		client.print(F(">")); 
	}
}

//uint32_t readLong(int address)
//{
//	return eeprom_read_dword((unsigned long *) address);
//}
