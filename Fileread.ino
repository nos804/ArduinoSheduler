#include <SD.h>					//SD card library
#include <SPI.h>
#include <QueueList.h>

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

void setup()
{

	Serial.begin(9600);

	Serial.print("Initializing card...");

	// declare default CS pin as OUTPUT
	pinMode(4, OUTPUT);

	if (!SD.begin(4)) {
		Serial.println("initialization of the SD card failed!");
		return;
	}
	Serial.println("initialization of the SDcard is done.");


	//myfile = SD.open("textFile.txt", FILE_WRITE);


	//if (myfile)
	//{
	//	Serial.print("Writing to the text file...");
	//	myfile.println("Congratulations! You have successfully wrote on the text file.");

	//	myfile.close(); // close the file:
	//	Serial.println("done closing.");
	//}
	//else
	//{
	//	// if the file didn't open, report an error:
	//	Serial.println("error opening the text file!");
	//}

	// //re-open the text file for reading:
	//myfile = SD.open("textFile.txt");
	//if (myfile)
	//{
	//	Serial.println("textFile.txt:");

	//	// read all the text written on the file
	//	while (myfile.available())
	//	{
	//		Serial.write(myfile.read());
	//	}
	//	// close the file:
	//	myfile.close();
	//}
	//else
	//{
	//	// if the file didn't open, report an error:
	//	Serial.println("error opening the text file!");
	//}
}

void readFile()
{

	char temp;
	String line = "";
	// re-open the text file for reading:
	myfile = SD.open("SOUNDSCH.txt");
	if (myfile)
	{
		Serial.println("SOUNDSCH.txt:");

		// read all the text written on the file
		while (myfile.available())
		{
			temp = myfile.read();
			//Serial.write(temp);
			//delay(100);
			line += temp;
			if (temp == '\n')
			{
				parseString(line);
				line = "";
				Serial.print("The amount of RAM left: ");
				Serial.println(freeRam());
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
	//const char *line = input.c_str();
	char delimit[] = " :”“\",\r\n";
	char *token;

	/* get the first token */
	token = strtok(&input[0], delimit);

	/* walk through other tokens */
	int i = 0;
	while (token != NULL)
	{
		//Serial.println(token);
		delay(10);
		//Serial.print("i is ");
		//Serial.println(i);
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
	/*Serial.print("Day is: ");
	Serial.println(day);
	Serial.print("Hour is: ");
	Serial.println(hour);
	Serial.print("Minute is: ");
	Serial.println(minute);*/
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

int freeRam() {
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void loop()
{
	readFile();
	Serial.println(Everyday.command.isEmpty());
	while (!Everyday.command.isEmpty())
	{
		Serial.print("Command: ");
		Serial.println(Everyday.command.pop());
		Serial.print("Hour: ");
		Serial.println(Everyday.hour.pop());
		Serial.print("Minute: ");
		Serial.println(Everyday.minute.pop());
	}
	while (1);
}
