#include <SD.h>					//SD card library
#include <SPI.h>
#include <QueueList.h>

File myfile;

struct DayShedule {
	QueueList<int> hour;
};

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
			}
		}
		// close the file:
		myfile.close();
		while (1);
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
	int hour, minute, day;
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
		Serial.println(token);
		delay(10);
		Serial.print("i is ");
		Serial.println(i);
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
	int i = 0;
	i++;
}

void loop()
{
	readFile();
	while (1);
}
