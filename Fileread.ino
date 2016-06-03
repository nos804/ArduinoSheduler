#include <SD.h>					//SD card library
#include <SPI.h>
#include <QueueList.h>
#include <Time.h>				//Used to store information from UTP server
#include <Timezone.h>			//Used to convert UTC to local time with DST correction
#include <EthernetUdp2.h>		//Used to pull data from NIST server
#include <Ethernet2.h>			//Ethernet library

#define CARDCS 4     // Card chip select pin
#define ETHERNETCS 10

byte mac[] = {
	0x90, 0xA2, 0xDA, 0x10, 0x6D, 0xB0
}; //physical mac address

File myfile;

struct DayShedule {
	QueueList<int> hour;
	QueueList<int> minute;
	QueueList<String> command;
};

DayShedule Sunday;
DayShedule Monday;
DayShedule Tuesday;
DayShedule Wednesday;
DayShedule Thursday;
DayShedule Friday;
DayShedule Saturday;
DayShedule Weekday;
DayShedule Weekend;
DayShedule Everyday;

unsigned long previousMillis = 0;        // will store last time Arduino was updated
unsigned long interval = 35000;           // interval at which to take over (milliseconds)

										  /* us.pool.ntp.org NTP server
										  (Set to your time server of choice) */
IPAddress timeServer = IPAddress(132, 163, 4, 101);

/* Set this to the offset (in seconds) to your local time
This example is GMT - 4 */
const long timeZoneOffset = 0L;

/* Syncs to NTP server every 15 seconds for testing,
set to 1 hour or more to be reasonable */
unsigned int ntpSyncTime = 300;

/* ALTER THESE VARIABLES AT YOUR OWN RISK */
// local port to listen for UDP packets
unsigned int localPort = 8888;
// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE = 48;
// Buffer to hold incoming and outgoing packets
byte packetBuffer[NTP_PACKET_SIZE];
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
// Keeps track of how long ago we updated the NTP server
unsigned long ntpLastUpdate = 0;
// Check last time clock displayed (Not in Production)
time_t prevDisplay = 0;

//////////////////////


TimeChangeRule usEDT = { "EDT", Second, Sun, Mar, 2, -240 };  //UTC - 4 hours
TimeChangeRule usEST = { "EST", First, Sun, Nov, 2, -300 };   //UTC - 5 hours
Timezone usEastern(usEDT, usEST);
time_t eastern, utc;
TimeChangeRule *tcr;

void setup()
{
	Serial.begin(9600);

	Serial.print("Initializing card...");

	// declare default CS and ethernet pin as OUTPUT
	pinMode(CARDCS, OUTPUT); // change this to 53 on a mega  // don't follow this!!
	pinMode(ETHERNETCS, OUTPUT); // change this to 53 on a mega  // don't follow this!!
	digitalWrite(ETHERNETCS, HIGH); // Add this line
	digitalWrite(CARDCS, HIGH); // Add this line

	if (!SD.begin(4)) {
		Serial.println("initialization of the SD card failed!");
		return;
	}
	Serial.println("initialization of the SDcard is done.");

	Serial.println("Trying Ethernet");
	//Ethernet.begin(mac, ip, dnServer, gateway, subnet);
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		//no point in carrying on, so do nothing forevermore:
		for (;;)
			;
	}
	Serial.println(Ethernet.localIP());

	while (!getTimeAndDate()) { // && trys<10) {
		Serial.println("Trying server");
	}

	utc = now();    //current time from the Time Library
	eastern = usEastern.toLocal(utc, &tcr);
	Serial.print("The time zone is: ");
	Serial.println(tcr->abbrev);
}

int getTimeAndDate() {
	int flag = 0;
	Udp.begin(localPort);
	sendNTPpacket(timeServer);
	delay(1000);
	if (Udp.parsePacket()) {
		Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
		unsigned long highWord, lowWord, epoch;
		highWord = word(packetBuffer[40], packetBuffer[41]);
		lowWord = word(packetBuffer[42], packetBuffer[43]);
		epoch = highWord << 16 | lowWord;
		epoch = epoch - 2208988800 + timeZoneOffset;
		flag = 1;
		setTime(epoch);
		ntpLastUpdate = now();
	}
	return flag;
}

// Do not alter this function, it is used by the system
unsigned long sendNTPpacket(IPAddress & address)
{
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	packetBuffer[0] = 0b11100011;
	packetBuffer[1] = 0;
	packetBuffer[2] = 6;
	packetBuffer[3] = 0xEC;
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
	Udp.beginPacket(address, 123);
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}

// Clock display of the time and date (Basic)
void clockDisplay() {
	Serial.print(hour());
	printDigits(minute());
	printDigits(second());
	Serial.print(" ");
	Serial.print(day());
	Serial.print(" ");
	Serial.print(month());
	Serial.print(" ");
	Serial.print(year());
	Serial.println();
}

//Function to print time with time zone
void printTime(time_t t, char *tz)
{
	sPrintI00(hour(t));
	sPrintDigits(minute(t));
	sPrintDigits(second(t));
	Serial.print(' ');
	Serial.print(dayShortStr(weekday(t)));
	Serial.print(' ');
	sPrintI00(day(t));
	Serial.print(' ');
	Serial.print(monthShortStr(month(t)));
	Serial.print(' ');
	Serial.print(year(t));
	Serial.print(' ');
	Serial.print(tz);
	Serial.println();
}

//Print an integer in "00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintI00(int val)
{
	if (val < 10) Serial.print('0');
	Serial.print(val, DEC);
	return;
}

//Print an integer in ":00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintDigits(int val)
{
	Serial.print(':');
	if (val < 10) Serial.print('0');
	Serial.print(val, DEC);
}

// Utility function for clock display: prints preceding colon and leading 0
void printDigits(int digits) {
	Serial.print(":");
	if (digits < 10)
		Serial.print('0');
	Serial.print(digits);
}

void readFile()
{
	char temp;
	String line = "";
	// open the text file for reading:
	myfile = SD.open("SOUNDSCH.txt");
	if (myfile)
	{
		Serial.println("SOUNDSCH.txt:");

		// read all the text written on the file
		while (myfile.available())
		{
			temp = myfile.read();
			line += temp;
			if (temp == '\n')
			{
				parseString(line);
				line = "";
				/*Serial.print("The amount of RAM left: ");
				Serial.println(freeRam());*/
			}
		}
		// close the file:
		myfile.close();
	}
	else
	{
		// if the file didn't open, report an error:
		Serial.println("error opening the text file!");
		delay(1000);
	}
}

void parseString(String input)
{
	int hour = 0, minute = 0, day = 0;
	String command;
	char delimit[] = " :”“\",\r\n";	//characters to remove from the string
	char *token;					//pointer to store tokenized string

	/* get the first token */
	token = strtok(&input[0], delimit);

	/* walk through other tokens */
	int i = 0;
	while (token != NULL)
	{
		if (i == 0)
		{
			hour = atoi(token);
		}
		else if (i == 1)
		{
			minute = atoi(token);
		}
		else if (i == 2)
		{
			command = token;
		}
		else if (i == 3)
		{
			day = atoi(token);
		}
		i++;
		token = strtok(NULL, delimit);
	}
	structify(hour, minute, command, day);
}

void structify(int hour, int minute, String command, int day)
{
	switch (day) {
	case 0:
		//Sunday
		Sunday.hour.push(hour);
		Sunday.minute.push(minute);
		Sunday.command.push(command);
		break;
	case 1:
		//Monday
		Monday.hour.push(hour);
		Monday.minute.push(minute);
		Monday.command.push(command);
		break;
	case 2:
		//Tuesday
		Tuesday.hour.push(hour);
		Tuesday.minute.push(minute);
		Tuesday.command.push(command);
		break;
	case 3:
		//Wednesday
		Wednesday.hour.push(hour);
		Wednesday.minute.push(minute);
		Wednesday.command.push(command);
		break;
	case 4:
		//Thursday
		Thursday.hour.push(hour);
		Thursday.minute.push(minute);
		Thursday.command.push(command);
		break;
	case 5:
		//Friday
		Friday.hour.push(hour);
		Friday.minute.push(minute);
		Friday.command.push(command);
		break;
	case 6:
		//Saturday
		Saturday.hour.push(hour);
		Saturday.minute.push(minute);
		Saturday.command.push(command);
		break;
	case 97:
		//Weekends
		Weekend.hour.push(hour);
		Weekend.minute.push(minute);
		Weekend.command.push(command);
		break;
	case 98:
		//Weekdays
		Weekday.hour.push(hour);
		Weekday.minute.push(minute);
		Weekday.command.push(command);
		break;
	case 99:
		//Everyday
		Everyday.hour.push(hour);
		Everyday.minute.push(minute);
		Everyday.command.push(command);
		break;
	}
}

void checkEventSchedule()
{
	utc = now();    //current time from the Time Library
	eastern = usEastern.toLocal(utc, &tcr);
	//printTime(eastern, tcr->abbrev);
	bool isWeekday = 0;
	switch (weekday(eastern)) {
	case 2: case 3: case 4: case 5: case 6:
		isWeekday = true;
		break;
	case 1: case 7:
		isWeekday = false;
		break;
	}
	if(Everyday.hour.peek() == hour(eastern) && Everyday.minute.peek() == minute(eastern))
	{
		Serial.println("Everyday event");
		runEvent(Everyday);
	}
	else if(isWeekday == true && (Weekday.hour.peek() == hour(eastern) && Weekday.minute.peek() == minute(eastern)))
	{
		Serial.println("Weekday event");
		runEvent(Weekday);
	}
	else if(isWeekday == false && ((Weekend.hour.peek() == hour(eastern) && Weekend.minute.peek() == minute(eastern))))
	{
		Serial.println("Weekend event");
		runEvent(Weekend);
	}
	//else if (!Sunday.command.isEmpty() &&((Sunday.hour.peek() == hour(eastern)) && (Sunday.minute.peek() == minute(eastern))))
	//{
	//	Serial.println("Sunday day event");
	//	runEvent(Sunday);
	//}
	switch (weekday(eastern))
	{
	case 1:
		dayEventCheck(Sunday);
		break;
	case 2:
		dayEventCheck(Monday);
		break;
	case 3:
		dayEventCheck(Tuesday);
		break;
	case 4:
		dayEventCheck(Wednesday);
		break;
	case 5:
		dayEventCheck(Thursday);
		break;
	case 6:
		dayEventCheck(Friday);
		break;
	case 7:
		dayEventCheck(Saturday);
		break;
	}
}

void dayEventCheck(struct DayShedule& queue)
{
	if (!queue.command.isEmpty() && ((queue.hour.peek() == hour(eastern)) && (queue.minute.peek() == minute(eastern))))
	{
		Serial.print(weekday(eastern));
		Serial.println(" day event.");
		runEvent(queue);
	}
}

void runEvent(struct DayShedule& queue)
{
	String tempString = queue.command.peek();
	progressQueue(queue);
	runCommand(tempString);
}

//void runEverydayEvent()
//{
//	int tempHour, tempMinute;
//	String tempString;
//	//Set temp variables and pop them off queues
//	tempString = Everyday.command.pop();
//	tempHour = Everyday.hour.pop();
//	tempMinute = Everyday.minute.pop();
//	//push variables to back of queue
//	Everyday.command.push(tempString);
//	Everyday.hour.push(tempHour);
//	Everyday.minute.push(tempMinute);
//	//Run the event command
//	runCommand(tempString);
//}
//
//void runWeekdayEvent()
//{
//	int tempHour, tempMinute;
//	String tempString;
//	//Set temp variables and pop them off queues
//	tempString = Weekday.command.pop();
//	tempHour = Weekday.hour.pop();
//	tempMinute = Weekday.minute.pop();
//	//push variables to back of queue
//	Weekday.command.push(tempString);
//	Weekday.hour.push(tempHour);
//	Weekday.minute.push(tempMinute);
//	//Run the event command
//	runCommand(tempString);
//}
//
//void runWeekendEvent()
//{
//	int tempHour, tempMinute;
//	String tempString;
//	//Set temp variables and pop them off queues
//	tempString = Weekend.command.pop();
//	tempHour = Weekend.hour.pop();
//	tempMinute = Weekend.minute.pop();
//	//push variables to back of queue
//	Weekend.command.push(tempString);
//	Weekend.hour.push(tempHour);
//	Weekend.minute.push(tempMinute);
//	//Run the event command
//	runCommand(tempString);
//}
//
//void runDayEvent()
//{
//	int tempHour, tempMinute;
//	String tempString;
//	switch (weekday(eastern)) {
//	case 1:
//		//Set temp variables and pop them off queues
//		tempString = Sunday.command.pop();
//		tempHour = Sunday.hour.pop();
//		tempMinute = Sunday.minute.pop();
//		//push variables to back of queue
//		Sunday.command.push(tempString);
//		Sunday.hour.push(tempHour);
//		Sunday.minute.push(tempMinute);
//		//Run the event command
//		runCommand(tempString);
//		break;
//	case 2:
//		//Set temp variables and pop them off queues
//		tempString = Monday.command.pop();
//		tempHour = Monday.hour.pop();
//		tempMinute = Monday.minute.pop();
//		//push variables to back of queue
//		Monday.command.push(tempString);
//		Monday.hour.push(tempHour);
//		Monday.minute.push(tempMinute);
//		//Run the event command
//		runCommand(tempString);
//		break;
//	case 3:
//		//Set temp variables and pop them off queues
//		tempString = Tuesday.command.pop();
//		tempHour = Tuesday.hour.pop();
//		tempMinute = Tuesday.minute.pop();
//		//push variables to back of queue
//		Tuesday.command.push(tempString);
//		Tuesday.hour.push(tempHour);
//		Tuesday.minute.push(tempMinute);
//		//Run the event command
//		runCommand(tempString);
//		break;
//	case 4:
//		//Set temp variables and pop them off queues
//		tempString = Wednesday.command.pop();
//		tempHour = Wednesday.hour.pop();
//		tempMinute = Wednesday.minute.pop();
//		//push variables to back of queue
//		Wednesday.command.push(tempString);
//		Wednesday.hour.push(tempHour);
//		Wednesday.minute.push(tempMinute);
//		//Run the event command
//		runCommand(tempString);
//		break;
//	case 5:
//		//Set temp variables and pop them off queues
//		tempString = Thursday.command.pop();
//		tempHour = Thursday.hour.pop();
//		tempMinute = Thursday.minute.pop();
//		//push variables to back of queue
//		Thursday.command.push(tempString);
//		Thursday.hour.push(tempHour);
//		Thursday.minute.push(tempMinute);
//		//Run the event command
//		runCommand(tempString);
//		break;
//	case 6:
//		//Set temp variables and pop them off queues
//		tempString = Friday.command.pop();
//		tempHour = Friday.hour.pop();
//		tempMinute = Friday.minute.pop();
//		//push variables to back of queue
//		Friday.command.push(tempString);
//		Friday.hour.push(tempHour);
//		Friday.minute.push(tempMinute);
//		//Run the event command
//		runCommand(tempString);
//		break;
//	case 7:
//		//Set temp variables and pop them off queues
//		tempString = Saturday.command.pop();
//		tempHour = Saturday.hour.pop();
//		tempMinute = Saturday.minute.pop();
//		//push variables to back of queue
//		Saturday.command.push(tempString);
//		Saturday.hour.push(tempHour);
//		Saturday.minute.push(tempMinute);
//		//Run the event command
//		runCommand(tempString);
//		break;
//	}
//}

void runCommand(String input)
{
	Serial.println("Trying to run command");
	if (input == "BUZZ")
	{
		Serial.println("Buzzing");
	}
	else if (input == "MUTE")
	{
		Serial.println("MUTING");
	}
	else if (input == "STRETCH")
	{
		Serial.println("Stretching");
	}
	else if (input == "PLAYHIGH")
	{
		Serial.println("Starting music high volume");
	}
	else if (input == "PLAYLOW")
	{
		Serial.println("Starting music low volume");
	}
	else if (input == "HIGHVOL")
	{
		Serial.println("Going to high volume");
	}
	else if (input == "LOWVOL")
	{
		Serial.println("Going to low volume");
	}
	else if (input == "BUZZTHENSTRETCH")
	{
		Serial.println("Buzzing, muting music, playing stretches");
	}
	else if (input == "BUZZTHENHIGH")
	{
		Serial.println("Buzzing and making music high");
	}
	else if (input == "BUZZTHENLOW")
	{
		Serial.println("Buzzing and making music low");
	}
}

void progressQueue(struct DayShedule& queue)
{
	int tempHour, tempMinute;
	String tempString;
	tempHour = queue.hour.pop();
	tempMinute = queue.minute.pop();
	tempString = queue.command.pop();
	queue.hour.push(tempHour);
	queue.minute.push(tempMinute);
	queue.command.push(tempString);
}

void queueToCurrent()
{
	int queueSize = 0;
	if (!Everyday.command.isEmpty())
	{
		while ((Everyday.hour.peek() < hour(eastern)) && (queueSize < Everyday.command.count()))
		{
			progressQueue(Everyday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left everyday: ");
	//Serial.println(freeRam());
	queueSize = 0;
	if (!Weekday.command.isEmpty())
	{
		while ((Weekday.hour.peek() < hour(eastern)) && (queueSize < Weekday.command.count()))
		{
			progressQueue(Weekday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left weekday: ");
	//Serial.println(freeRam());
	queueSize = 0;
	if ((!Weekend.command.isEmpty()) && (queueSize < Weekend.command.count()))
	{
		while (Weekend.hour.peek() < hour(eastern))
		{
			progressQueue(Weekend);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left weekend: ");
	//Serial.println(freeRam());
	queueSize = 0;
	/*Serial.print("Sunday queue size: ");
	Serial.println(Sunday.command.count());*/
	if (!Sunday.command.isEmpty())
	{
		while ((Sunday.hour.peek() < hour(eastern)) && (queueSize < Sunday.command.count()))
		{
			progressQueue(Sunday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left sunday: ");
	//Serial.println(freeRam());
	queueSize = 0;
	if (!Monday.command.isEmpty())
	{
		while ((Monday.hour.peek() < hour(eastern)) && (queueSize < Monday.command.count()))
		{
			progressQueue(Monday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left monday: ");
	//Serial.println(freeRam());
	queueSize = 0;
	if (!Tuesday.command.isEmpty()) {
		while ((Tuesday.hour.peek() < hour(eastern)) && (queueSize < Tuesday.command.count()))
		{
			progressQueue(Tuesday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left tuesday: ");
	//Serial.println(freeRam());
	queueSize = 0;
	if (!Wednesday.command.isEmpty())
	{
		while ((Wednesday.hour.peek() < hour(eastern)) && (queueSize < Wednesday.command.count()))
		{
			progressQueue(Wednesday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left wednesday: ");
	//Serial.println(freeRam());
	queueSize = 0;
	if (!Thursday.command.isEmpty())
	{
		while ((Thursday.hour.peek() < hour(eastern)) && (queueSize < Thursday.command.count()))
		{
			progressQueue(Thursday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left thursday: ");
	//Serial.println(freeRam());
	queueSize = 0;
	if (!Friday.command.isEmpty())
	{
		while ((Friday.hour.peek() < hour(eastern)) && (queueSize < Friday.command.count()))
		{
			progressQueue(Friday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left friday: ");
	//Serial.println(freeRam());
	queueSize = 0;
	if (!Saturday.command.isEmpty())
	{
		while ((Saturday.hour.peek() < hour(eastern)) && (queueSize < Saturday.command.count()))
		{
			progressQueue(Saturday);
			queueSize++;
		}
	}
	//Serial.print("The amount of RAM left saturday: ");
	//Serial.println(freeRam());
}

int freeRam() {
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void loop()
{
	char output[3] = "";
	readFile();
	//Serial.println(Everyday.command.isEmpty());
	//while (!Everyday.command.isEmpty())
	//Serial.print("Command: ");
	//Serial.println(Everyday.command.peek());
	//Serial.println(Everyday.hour.peek());
	//Serial.print("minute: ");
	//Serial.println(Everyday.minute.peek());
	while (1) {
		checkEventSchedule();
		//Serial.print("The amount of RAM left: ");
		//Serial.println(freeRam());
		delay(1000);
		queueToCurrent();
		/*Serial.print("The next event is: ");
		Serial.print(Weekday.hour.peek());
		sprintf(output, "%02d", Weekday.minute.peek());
		Serial.print(":");
		Serial.println(output);*/
	}
}
