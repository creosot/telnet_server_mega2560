#define		TWO_MONITOR
#define		IP_ADDRESS_MON_1	10,239,238,35 //teatralnay 1 192,168,1,3
#define		MON_PORT	1515
#define		IP_ADDRESS_MON_2	10,239,238,51 //rogojskii wal
#define		str(a) #a
#define		xstr(a) str(a)
#define		IP_ADDRESS_STRING(a,b,c,d) xstr(a)"." xstr(b)"." xstr(c)"." xstr(d)
#define		XIP_ADDRESS_STRING(abcd) IP_ADDRESS_STRING(abcd)

#define		IP_ADDRESS_SMTP	172,25,0,12
#define		SMTP_PORT	25

#define COUNT_MONITOR	2
#define LENGTH_MONITOR_STATUS 10

#include <EEPROM.h>
#include <Wire.h>
#include <UIPEthernet.h>
#include <BH1750.h>

BH1750 lightMeter;

//EthernetServer server(23);
EthernetClient client;

typedef struct monitor
{
	float light_value;
	uint8_t bright_value;
	char status_host[LENGTH_MONITOR_STATUS];
	char status_light_sensor[LENGTH_MONITOR_STATUS];
	const char *gid_name;
} monitor_t;

//smtp
void send_mail();
uint8_t connect_to_smtp_server(monitor_t);
uint8_t waitForReply();
int sendAndWaitForReply(const char* txtToSend);
int sendAndWaitForReply(const __FlashStringHelper* txtToSend);
void justCopy(const __FlashStringHelper* txtToSend);
void justCopy(const char* txtToSend);
void copyValueToMON(uint8_t num);

const char *gids_CURRENT_str[] = { "MSCF06819B#179", "0987654321" };
unsigned long next;
unsigned long time_sendmail_mon = 0;
unsigned long interval_sending_mail = 0;
const char h_off[] PROGMEM = "HOST OFF";
const char h_on[] PROGMEM = "HOST ON";
const char * const host_status[] PROGMEM =
{   
	h_off,
	h_on
};
const char ls_off[] PROGMEM  =  "LS OFF";
const char ls_on[] PROGMEM  =  "LS ON";
const char * const ls_status[] PROGMEM =
{   
	ls_off,
	ls_on
};
const char receiver_mail_1[] =  "ikatkov@russoutdoor.ru";

monitor_t mon = { 0 };
uint8_t status_host = 0;
uint8_t status_ls = 0;
uint8_t mac[6] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };

void setup()
{
	Serial.begin(9600);
	delay(1000);
	Wire.begin();
	while (1) {
		Serial.println("Ethernet Init");
		int ret = Ethernet.begin(mac);
		if (ret != 0)
		{
			Serial.println("Ethernet Ok");
			break;
		}
		Serial.println("Not DHCP adress");
		Serial.print("localIP: ");
		Serial.println(Ethernet.localIP());
	}
	Serial.print("localIP: ");
	Serial.println(Ethernet.localIP());
	Serial.print("subnetMask: ");
	Serial.println(Ethernet.subnetMask());
	Serial.print("gatewayIP: ");
	Serial.println(Ethernet.gatewayIP());
	Serial.print("dnsServerIP: ");
	Serial.println(Ethernet.dnsServerIP());
	Serial.println("");
	Serial.println("");
	delay(5000);	
	
	send_mail();
}

void loop()
{
	//telnet_server();
}

void send_mail()
{
	Serial.println(F("Connect to smtp server:"));
	Serial.print(XIP_ADDRESS_STRING(IP_ADDRESS_SMTP) "/" xstr(SMTP_PORT));
	for (uint8_t m = 0; m < 2; m++)
	{
		copyValueToMON(m);
		for (uint8_t i = 0; i < 10; i++)
		{
			if (!connect_to_smtp_server(mon))
			{
				Serial.print(i);
				Serial.println(F(": send email failed")); 
			}
			else
			{
				Serial.println(F("Email sucefully send"));
				break;
			}
		}	 
	}
	
	Serial.println(F("close connect"));
	Serial.println("");
	client.stop();	
}

uint8_t connect_to_smtp_server(monitor_t mon)
{
//	if (client.connect(IPAddress(IP_ADDRESS_SMTP), SMTP_PORT))
	if(client.connect(IPAddress(IP_ADDRESS_SMTP), SMTP_PORT))
	{
		Serial.println(" -> connected");
		client.flush();
		if (!waitForReply())
		{
			return 0;
		}
		if (!sendAndWaitForReply(F("EHLO")))
		{
			return 0;
		}
		String from = String(F("MAIL FROM:<"));
		from += mon.gid_name;
		from += F("_light_sensor>");
		if (!sendAndWaitForReply(from.c_str()))
		{
			return 0;
		}
		String rcpt_to = String(F("RCPT TO:<"));
		rcpt_to += receiver_mail_1;   //receiver mail
		rcpt_to += F(">");
		if (!sendAndWaitForReply(rcpt_to.c_str()))
		{
			return 0;
		}
//		if (!sendAndWaitForReply(F("RCPT TO:<digital_monitoring@russoutdoor.ru>")))
//		{
//			return 0;
//		}
		if (!sendAndWaitForReply(F("DATA")))
		{
			return 0;
		}
		client.println(F("MIME-Version: 1.0"));
		justCopy(F("From: light_sensor@russoutdoor.ru"));
		String rcpt_all = String("To: ");
		rcpt_all += receiver_mail_1;
		justCopy(rcpt_all.c_str());
//		justCopy(F("To: digital_monitoring@russoutdoor.ru, ikatkov@russoutdoor.ru"));
		String subject = String("Subject: ");
		subject += mon.gid_name;
		justCopy(subject.c_str());
		client.println(F("Content-type: text/plain; charset=us-ascii"));
		justCopy(F(""));      //this separates the headers from the body.
		justCopy(mon.status_host);
		justCopy(mon.status_light_sensor);
		String bright = String("BRI=");
		bright += mon.bright_value;
		justCopy(bright.c_str());
		if (!sendAndWaitForReply(F("."))) //this means EOF to the smtp server.
		{
			return 0;
		}
		if (!sendAndWaitForReply(F("QUIT")))
		{
			return 0;
		}				
		return 1;		
	}
	else {
		Serial.println(F(" -> connect failed"));
		return 0;
	}
}

uint8_t	waitForReply() {
	next = millis() + 5000;
	while (client.available() == 0)
	{
		if (millis() > next)
			return 0;
	}
	int size;
	while ((size = client.available()) > 0)
	{
		uint8_t* msg = (uint8_t*)malloc(size);
		size = client.read(msg, size);
		Serial.write(msg, size);
		free(msg);
	}
	return 1;
}

int sendAndWaitForReply(const char* txtToSend) {
	client.println(txtToSend);
	Serial.println(txtToSend);
	if (!waitForReply())
	{
		return 0;
	}
	return 1;
}

int sendAndWaitForReply(const __FlashStringHelper* txtToSend) {
	client.println(txtToSend);
	Serial.println(txtToSend);
	if (!waitForReply())
	{
		return 0;
	}
	return 1;
}

void justCopy(const __FlashStringHelper* txtToSend) {
	client.println(txtToSend);
	Serial.println(txtToSend);
}

void justCopy(const char* txtToSend) {
	client.println(txtToSend);
	Serial.println(txtToSend);
}

void copyValueToMON(uint8_t num)
{
	mon.gid_name = gids_CURRENT_str[num];
	char c;
	uint16_t ptr = pgm_read_word((int)&host_status[num]);
	for (uint8_t i = 0; i < LENGTH_MONITOR_STATUS - 1; i++)
	{
		c = pgm_read_byte(ptr++);
		mon.status_host[i] = c;
		if (!c)
			break;
		
	}
	mon.status_host[LENGTH_MONITOR_STATUS - 1] = 0;
	
	ptr = pgm_read_word((int)&ls_status[num]);
	for (uint8_t i = 0; i < LENGTH_MONITOR_STATUS - 1; i++)
	{
		c = pgm_read_byte(ptr++);
		mon.status_light_sensor[i] = c;
		if (!c)
			break;
		
	}
	mon.status_light_sensor[LENGTH_MONITOR_STATUS - 1] = 0;
	mon.bright_value = 55;
}