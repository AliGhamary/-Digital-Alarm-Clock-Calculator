#include "mbed.h"
#include "i2c.h"
//DigitalOut CLK(p24); // clock is at p24
//DigitalOut CLR(p25); // clear is at p25
//DigitalOut DATA(p30); // data is at p30

//row scanning
DigitalIn R1(p11);
DigitalIn R2(p12);
DigitalIn R3(p13);
DigitalIn R4(p14);

DigitalOut C1(p29);
DigitalOut C2(p28);
DigitalOut C3(p27);
DigitalOut C4(p26);

//LED for Alarm
DigitalOut LED(p25);

//project, initialize all functions to be used
static unsigned char wr_lcd_mode(char c, char mode);
unsigned char lcd_command(char c);
unsigned char lcd_data(char c);
void lcd_init(void);
void lcd_backlight(char on);
void lcdLoad(char a[], char c, int size, int line, int select);
void lcdPrev(char a[], char c, int size, int select);
void fillArr(char word[], char ch, int size, int choice);
void clearArr();
void controller();
void setClock(int select);
int getPad(int length);
int getChar();
void displayAlphabet();
int rowScan(int colNum);
int rowPressed();
int getSize(int result);
int getNum(int rowNum, int colNum);
int getOpNum();
int calcMode();
int normalMode();

static char lcdLine[20]; //array to fill each line of LCD
static int arrCursor = 0; //cursor location, used to fill lcdline array

static int prevCursor = 0; //previous cursor location

#define decode_bcd(x) ((x >> 4) * 10 + (x & 0x0F)) 
#define encode_bcd(x) ((((x / 10) & 0x0F) << 4) + (x % 10)) 

// LCD Commands
// -------------------------------------------------------------------------
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80
// flags for display entry mode
// -------------------------------------------------------------------------
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00
// flags for display on/off and cursor control
// -------------------------------------------------------------------------
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00
// flags for display/cursor shift
// -------------------------------------------------------------------------
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00
// flags for function set
// -------------------------------------------------------------------------
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00
///*********************************************
//Project : I2C to LCD Interface-Routine
//Port PCF8574 : 7 6 5 4 3 2 1 0
// D7 D6 D5 D4 BL EN RW RS
//**********************************************/
#define PCF8574T 0x27

I2C i2c(p9, p10);// SDA, SCL//

int main() {
	
	//set columns high to do row scanning
	C1 = 1;
	C2 = 1;
	C3 = 1;
	C4 = 1;
	
	//initialize column and rows for getting inputs
	int colNum = 1;
	int rowNum = 0;
	
	lcd_init();
	
	//display alphabet for part b
	displayAlphabet();
	
	//test calculator mode for part c
	//calcMode();
	
	//call controller function for rest of parts
	controller();

	//test to see if LCD works
	//lcdLoad("hello world", ' ', sizeof("hello world"), 2, 0);		
		
}

void lcdLoad(char a[], char c, int size, int line, int select){
	int location;
	switch(line){ //select which line to print text on
		case 0: location = 0; break;
		case 1: location = 40; break;
		case 2: location = 20; break;
		case 3: location = 60; break;
	}
	
	for(int i = 0; i < location; i++){
		lcd_command(0x14); //shift cursor right
	}
	
	
	if(select == 0){ //print data when select = 0
		for(int i = 0; i < size - 1; i++){
			location++;
			lcd_data(a[i]);
		}
	}
	
	else{ //if select is 1 print a character
		location++;
		lcd_data(c);
	}
	
	prevCursor = location; //save cursor
	
	while (location != 0){
		location--;
		lcd_command(0x10); //shift cursor left
	}
		
}

//load previous data from previous cursor
void lcdPrev(char a[], char c, int size, int select){
	int location = prevCursor;
	for(int i = 0; i < location; i++){
		lcd_command(0x14);
	}
	
	if(select == 0){
		for(int i = 0; i < size-1; i++){
			location++;
			lcd_data(a[i]);
		}
	}else{
			location++;
			lcd_data(c);
	}
	
	prevCursor = location;
	
	while(location !=0){
			location--;
			lcd_command(0x10);
	}

}

//fill lcd with array
void fillArr(char stuff[], char c, int size, int select){
	if(select == 0){ //load lcdline array with data in stuff array based on cursor
		for(int i = 0; i < size - 1; i++){
			lcdLine[arrCursor] = stuff[i]; //load LCD with the data in stuff
			arrCursor++;
		}
	}
	else{
		lcdLine[arrCursor] = c;
		arrCursor++;
	}
}

void clearArr(){ //clear array
	sprintf(lcdLine, "%d", 0);
	arrCursor = 0;
}

static unsigned char wr_lcd_mode(char c, char mode) {
 char ret = 1;
 char seq[5];
 static char backlight = 8;
 if (mode == 8) {
 backlight = (c != 0) ? 8 : 0;
 return 0;
 }
 mode |= backlight;
 seq[0] = mode; // EN=0, RW=0, RS=mode
 seq[1] = (c & 0xF0) | mode | 4; // EN=1, RW=0, RS=1
 seq[2] = seq[1] & ~4; // EN=0, RW=0, RS=1
 seq[3] = (c << 4) | mode | 4; // EN=1, RW=0, RS=1
 seq[4] = seq[3] & ~4; // EN=0, RW=0, RS=1
 i2c.start();
 i2c.write(PCF8574T << 1);
 uint8_t i;
 for (i = 0; i < 5; i++) {
 i2c.write(seq[i]);
 wait(0.002);
 }
 ret = 0;
 i2c.stop();
 if (!(mode & 1) && c <= 2)
 wait(0.2); // CLS and HOME
 return ret;
}
unsigned char lcd_command(char c) {
 wr_lcd_mode(c, 0);
}
unsigned char lcd_data(char c) {
 wr_lcd_mode(c, 1);
}
void lcd_init(void) {
 char i;

 // High-Nibble von Byte 8 = Display Control:
 // 1DCB**** D: Disp on/off; C: Cursor on/off B: blink on/off
 char init_sequenz[] = { 0x33, 0x32, 0x28, 0x0C, 0x06, 0x01 };
 wait(1); // Delay power-up
 for (i = 0; i < sizeof(init_sequenz); i++) {
 lcd_command(init_sequenz[i]);
 }
}
void lcd_backlight(char on) {
 wr_lcd_mode(on, 8);
}

//controller to initialize clock, temp sensor, and mode switch
void controller(){
	int toggleMode = 0;
	
	//initialize clock
	i2c.start(); /* The following Enables the Oscillator */ 
	i2c.write(0xD0); /* address the part to write */ 
	i2c.write(0x00); /* position the address pointer to 0 */ 
	i2c.write(0x00); /* write 0 to the secs register, clear the CH bit */ 
	i2c.stop();
	
	//initialize temp sensor
	i2c.start();
	i2c.write(0x90);
	i2c.write(0xac); 
	i2c.write(2); 
	i2c.stop();
	
	setClock(0);
	setClock(1);
	
	//control logic to switch between modes. press 3 characters and then F to switch.
	while(1){
		if(toggleMode == 0){
				lcd_command(0x01);
				prevCursor = 0;
				toggleMode = normalMode();
		}else if(toggleMode == 1){
				lcd_command(0x01);
				prevCursor = 0;
				toggleMode = calcMode();
		}
	}
}
	
//part B
void displayAlphabet(){
	lcd_command(0x01);
	lcdLoad("ABCDEFGHIJKLM", ' ', sizeof("ABCDEFGHIJKLM"), 0, 0);
	lcdLoad("NOPQRSTUVWXYZ", ' ', sizeof("ABCDEFGHIJKLM"), 1, 0);
	wait(2);
	lcd_command(0x01);
	lcdLoad("ABCDEFGHIJKLM", ' ', sizeof("ABCDEFGHIJKLM"), 0, 0);
	lcdLoad("NOPQRSTUVWXYZ", ' ', sizeof("ABCDEFGHIJKLM"), 1, 0);
	lcdLoad("abcdefghijklm", ' ', sizeof("ABCDEFGHIJKLM"), 2, 0);
	lcdLoad("nopqrstuvwxyz", ' ', sizeof("ABCDEFGHIJKLM"), 3, 0);
	wait(2);
	lcd_command(0x01);
	lcdLoad("0123456789.", ' ', sizeof("0123456789."), 0, 0);
	lcdLoad(".9876543210", ' ', sizeof("0123456789."), 1, 0);
	wait(2);
	lcd_command(0x01);
	prevCursor = 0;
	
}

//part C
void setClock(int select){
	int Hour; //hours
	int Min; //minutes
	int Day; //days
	int Dte; //date
	int Mon; //month
	int Yr;	 //year
	int ampm; //morning/afternoon
	
	if(select == 0){
			lcdLoad("Setting Clock Time", ' ', sizeof("setting clock time"), 1, 0);
			wait(2);
			lcd_command(0x01);
			
			do{
				//print "clock time hour?, user enters hour
				lcdLoad("Clock Time Hour?", ' ', sizeof("clock time hour?"), 0, 0);
				lcdLoad("Enter 1-12", ' ', sizeof("Enter 1-12"), 1, 0);
				lcdLoad(" ", ' ', sizeof(" "), 2, 0);
				Hour = getPad(2);
				lcd_command(0x01);
			}while(Hour < 1 || Hour > 12); //dummy proof condition, only allow correct values to be entered
			
			do{
				//print "clock time minutes?", user enters minutes
				lcdLoad("Clock Time Minutes?", ' ', sizeof("clock time minutes?"), 0, 0);
				lcdLoad("Enter 1-59", ' ', sizeof("Enter 1-59"), 1, 0);
				lcdLoad(" ", ' ', sizeof(" "), 2, 0);
				Min = getPad(2);
				lcd_command(0x01);
			}while(Min > 59); //dummy proof condition, only allow correct values to be entered
			
			do{
				//print "clock time am or pm?", user enters A for AM or P for PM
				lcdLoad("Clock Time AM or PM?", ' ', sizeof("clock time am or pm?"), 0, 0);
				lcdLoad("Enter A or B", ' ', sizeof("Enter A or B"), 1, 0);
				lcdLoad(" ", ' ', sizeof(" "), 2, 0);
				ampm = getChar();
				lcd_command(0x01);
			}while(ampm != 10 && ampm != 11); //dummy proof condition, only allow correct values to be entered
		 
		 do{
			//print "clock time month?", user enters month
			lcdLoad("Clock Time Month?", ' ', sizeof("clock time month?"), 0, 0);
			lcdLoad("Enter 1-12", ' ', sizeof("Enter 1-12"), 1, 0);
			 lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Mon = getPad(2);
			lcd_command(0x01);
		 }while(Mon > 12 || Mon < 1); //dummy proof condition, only allow correct values to be entered
		
		do{
			//print "clock time date?", user enters day of the month
			lcdLoad("Clock Time Date?", ' ', sizeof("clock time date?"), 0, 0);
			lcdLoad("Enter 1-31", ' ', sizeof("Enter 1-31"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Dte = getPad(2);
			lcd_command(0x01);
		}while(Dte > 31 || Dte < 1); //dummy proof condition, only allow correct values to be entered
		
		do{
			//print "clock time day?", user enters day
			lcdLoad("Clock Time Day?", ' ', sizeof("clock time day?"), 0, 0);
			lcdLoad("Enter 1-7", ' ', sizeof("Enter 1-7"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Day = getPad(1);
			lcd_command(0x01);
		}while(Day > 7 || Day < 1); //dummy proof condition, only allow correct values to be entered
		
		do{
			//print "clock time year?", use enters year
			lcdLoad("Clock Time Year?", ' ', sizeof("clock time year?"), 0, 0);
			lcdLoad("Enter 2000-2099", ' ', sizeof("Enter 2000-2099"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Yr = getPad(4);
			lcd_command(0x01);
		}while(Yr < 2000 || Yr > 2099); //dummy proof condition, only allow correct values to be entered
		
		if(ampm == 10){
			ampm = 0x40;
		}else if(ampm == 11){
			ampm = 0x60;
		}
		
		//initialize clock
		i2c.start();
		i2c.write(0xD0);
		i2c.write(0x00); // write register address 1st clock register
		i2c.write(encode_bcd(0)); // start at 0 seconds
		i2c.write(encode_bcd(Min)); // minutes
		i2c.write( ampm | encode_bcd(Hour)); // hours
		i2c.write(encode_bcd(Day)); // days
		i2c.write(encode_bcd(Dte)); // day of month
		i2c.write(encode_bcd(Mon)); // month
		i2c.write(encode_bcd(Yr % 2000)); // year, 2000
		i2c.start();
		i2c.write(0xD0); 
		i2c.write(0x0e); // write register address control register
		i2c.write(0x20); // enable osc, bbsqi
		i2c.write(0); // clear the osf, alarm flags
		i2c.stop();
			
	}else{
		//print "setting alarm 1 time"
		lcdLoad("Setting alarm 1 time", ' ', sizeof("Setting alarm 1 time"), 1, 0);
		wait(5);
		lcd_command(0x01);
		do{
			//print "Alarm 1 time hour?"
			lcdLoad("Alarm 1 time hour? ", ' ', sizeof("Alarm 1 time hour? "), 0, 0);
			lcdLoad("Enter 1-12", ' ', sizeof("Enter 1-12"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Hour = getPad(2);
			lcd_command(0x01);
		}while(Hour > 12 || Hour < 1); //dummy proof condition, only allow correct values to be entered
		do{
			//print "Alarm 1 time min?"
			lcdLoad("Alarm 1 time min? ", ' ', sizeof("Alarm 1 time min? "), 0, 0);
			lcdLoad("Enter 1-59", ' ', sizeof("Enter 1-5"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Min = getPad(2);
			lcd_command(0x01);
		}while(Min > 59); //dummy proof condition, only allow correct values to be entered
		do{ 
			//print "Alarm 1 time am or pm?"
			lcdLoad("Alarm 1 time am or pm ", ' ', sizeof("Alarm 1 time am or pm "), 0, 0);
			lcdLoad("Enter A or B", ' ', sizeof("Enter A or B"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			ampm = getChar(); 
			lcd_command(0x01);
		}while(ampm != 10 && ampm != 11); //dummy proof condition, only allow correct values to be entered
		do{
			//print "Alarm 1 time month?"
			lcdLoad("Alarm 1 time month? ", ' ', sizeof("Alarm 1 time month? "), 0, 0);
			lcdLoad("Enter 1-12", ' ', sizeof("Enter 1-12"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Mon = getPad(2);
			lcd_command(0x01);
		}while(Mon > 12 || Mon < 1); //dummy proof condition, only allow correct values to be entered
		do{
			//print "Alarm 1 time date?"
			lcdLoad("Alarm 1 time date? ", ' ', sizeof("Alarm 1 time date? "), 0, 0);
			lcdLoad("Enter 1-31", ' ', sizeof("Enter 1-31"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Dte = getPad(2);
			lcd_command(0x01);
		}while(Dte > 31 || Dte < 1); //dummy proof condition, only allow correct values to be entered
		do{
			//print "Alarm 1 time day?"
			lcdLoad("Alarm 1 time day? ", ' ', sizeof("Alarm 1 time day? "), 0, 0);
			lcdLoad("Enter 1-7", ' ', sizeof("Enter 1-7"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Day = getPad(1);
			lcd_command(0x01);
		}while(Day > 7 || Day < 1); //dummy proof condition, only allow correct values to be entered
		do{
			//print "Alarm 1 time year?"
			lcdLoad("Alarm 1 time year? ", ' ', sizeof("Alarm 1 time year? "), 0, 0);
			lcdLoad("Enter 2000-2099", ' ', sizeof("Enter 2000-2099"), 1, 0);
			lcdLoad(" ", ' ', sizeof(" "), 2, 0);
			Yr = getPad(4);
			lcd_command(0x01);
		}while(Yr < 2000 || Yr > 2099); //dummy proof condition, only allow correct values to be entered
		
		if(ampm == 10){
			ampm = 0x40;
		}
		else if(ampm == 11){
			ampm = 0x60;
		}
		// writing the alarm data
		i2c.start();
		i2c.write(0xD0);
		i2c.write(0x07); // write register address 1st clock register
		i2c.write(encode_bcd(0)); // start at 0 seconds
		i2c.write(encode_bcd(Min)); // the 57th minute
		i2c.write( ampm | encode_bcd(Hour)); // the 11th hour pm
		i2c.write(0x00 | encode_bcd(Dte)); // the 28th day of month
		i2c.start();
		i2c.write(0xD0); 
		i2c.write(0x0e); // write register address control register
		i2c.write(0x20); // enable osc, bbsqi
		i2c.write(0); // clear the osf, alarm flags
		i2c.stop();
	
	}		
}

//function to get one character from keypad
int getChar(){
	int count = 0;
	int first = 0;
	int num = 0;
	int colNum = 1;
	char chars[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'}; //initialize characters
	while(count < 1){
		first = rowScan(colNum);
		if(first != 0){
			first = getNum(first, colNum);
			count++;
			lcdPrev(" ", chars[first], 1, 1);
		}
		
		if(colNum == 4){ 
			colNum = 1;
		}
		else{
			colNum++;
		}
		wait(0.03);
	}
	
	num = first;
	return num;
}

//get data from keypad
int getPad(int length){
	int count = 0;
	int first = 0;
	int second = 0;
	int third = 0;
	int fourth = 0;
	int num = 0;
	int colNum = 1;
	
	while(count < length){
		if(count == 0){
			first = rowScan(colNum);
			if(first != 0){
				first = getNum(first, colNum);
				if(first < 10){
					count++;
					//printf("first");
					char c = first + '0';
					lcdPrev(" ", c, 1, 1);
				}
			}
		}
		else if(count == 1){
			second = rowScan(colNum);
			if(second != 0){
				second = getNum(second, colNum);
				if(second < 10){
					count++;
					//printf("second");
					char c = second + '0';
					lcdPrev(" ", c, 1, 1);
				}
			}
		}
		else if(count == 2){
			third = rowScan(colNum);
			if(third != 0){
				third = getNum(third, colNum);
				if(third < 10){
					count++;
					//printf("third");
					char c = third + '0';
					lcdPrev(" ", c, 1, 1);
				}
			}
		}
		else if(count == 3){
			fourth = rowScan(colNum);
			if(fourth != 0){
				fourth = getNum(fourth, colNum);
				if(fourth < 10){
					count++;
					//printf("fourth");
					char c = fourth + '0';
					lcdPrev(" ", c, 1, 1);
				}
			}
		}
		
		if(colNum == 4){ 
			colNum = 1;
		}
		else{
			colNum++;
		}
		wait(0.03);
		
	}
	switch(length){
		case 1: num = first; break;
		case 2: num = first*10 + second; break;
		case 3: num = first*100 + second*10 + third; break;
		case 4: num = first*1000 + second*100 + third*10 + fourth; break;
	}
	return num;
}

//part D
static int alarmRing = 0;

//normal mode
int normalMode(){
	int Sec;
	int Min;
	int Hour;
	int Day;
	int Dte;
	int Mon;
	int Yr;
	int ampm;
	int alarmMin;
	int alarmHour;
	int alarmDte;
	int alarmAMPM;
	int alarmFlag = 0; //set the alarm flag
	
	//read values from clock
	i2c.start(); // start i2c 
	i2c.write(0xD0); // the address of the slave
	i2c.write(0x00); // the starting address to read (will increment automatically after each ACK)
	i2c.start(); // repeated i2c start
	i2c.write(0xD1); // we want to read from the slave
	Sec = i2c.read(1); // read the seconds register, send ACK
	Min = i2c.read(1); // read the minutes register, send ACK
	Hour = i2c.read(1); // read the hour register, send ACK
	Day = i2c.read(1); // read the day register, send ACK
	Dte = i2c.read(1); // read the Date register, send ACK
	Mon = i2c.read(1); // read the month register, send ACK
	Yr = i2c.read(0); // read the year register, send NACK to end the communication
	i2c.stop(); // stop the i2c 
	
	i2c.start(); // start i2c 
	i2c.write(0xD0); // the address of the slave
	i2c.write(0x08); // the starting address to read (will increment automatically after each ACK)
	i2c.start(); // repeated i2c start
	i2c.write(0xD1); // we want to read from the slave
	alarmMin = i2c.read(1); // read the minutes register, send ACK
	alarmHour = i2c.read(1); // read the hour register, send ACK
	alarmDte = i2c.read(0); // read the Date register, send NACK
	i2c.stop(); // stop the i2c 
	
	i2c.start(); // start i2c 
	i2c.write(0xD0); // the address of the slave
	i2c.write(0x0f); // the starting address to read (will increment automatically after each ACK)
	i2c.start(); // repeated i2c start
	i2c.write(0xD1); // we want to read from the slave
	alarmFlag = i2c.read(0); // read the alarm flag register, send NACK
	i2c.stop(); // stop the i2c 
	
	alarmFlag = alarmFlag & 0x01; // alarm flag = bit 0
	if (alarmRing == 0){
		alarmRing = alarmFlag;
	}
	
	//decode values from clock
	ampm = (Hour & 0x20) >> 5;
	Sec = decode_bcd(Sec);
	Min = decode_bcd(Min);
	Hour = decode_bcd(Hour & 0x1F);
	Day = decode_bcd(Day);
	Dte = decode_bcd(Dte);
	Mon = decode_bcd(Mon);
	Yr = decode_bcd(Yr);
	
	//decode alarm values
	alarmAMPM = (alarmHour & 0x20) >> 5;
	alarmMin = decode_bcd(alarmMin);
	alarmHour = decode_bcd(alarmHour & 0x1F);
	alarmDte = decode_bcd(alarmDte);
	
	//read temperature values
	int temp;
	int upByte;
	int lowByte;
	
	i2c.start();
	i2c.write(0x90);
	i2c.write(0x51);
	i2c.stop();
	
	i2c.start();
	i2c.write(0x90);
	i2c.write(0xaa);
	
	i2c.start(); 
	i2c.write(0x91); 
	upByte = i2c.read(1); // send an ACK after read
	lowByte = i2c.read(0); // send a NACK after read
	temp = (upByte << 8) + lowByte;
	i2c.stop();
	
	//values
	float tempF;
	float tempC;
	
	//temp conversions
	tempC = temp >> 4;
	if (tempC > 0x7FF){ // if negative
		tempC = -1 * ( ((int) (2 << 12) - tempC) * 0.0625) ; // 2's complement convert
	}else{
		tempC = tempC * 0.0625;
	}
	
	tempF = (tempC * 1.8) + 32;
	
	//alarm expired
	if (alarmFlag == 1){
		lcdLoad("Alarm 1 expired", ' ', sizeof("Alarm 1 expired"), 0, 0);
		LED = 1;
	}
	
	char tmp[3]; //temp array
	int tsize = sizeof(tmp); //size of temp array
	sprintf(tmp, "%d", Hour); //print hour
	if(Hour < 10){
		fillArr(tmp, '0', tsize, 1);
		fillArr(tmp, ' ', tsize-1, 0);
	}
	else{
		fillArr(tmp, ' ', tsize, 0);
	}
	fillArr(" ", ':', 1, 1);
	
	sprintf(tmp, "%d", Min); //print minutes
	
	if(Min < 10){
		fillArr(tmp, '0', tsize, 1);
		fillArr(tmp, ' ', tsize-1, 0);
	}
	else{
		fillArr(tmp, ' ', tsize, 0);
	}
	
	
	if(ampm == 1){ //print AM, PM
		fillArr("PM ", ' ', sizeof("pm "), 0);
	}
	else{
		fillArr("AM ", ' ', sizeof("am "), 0);
	}
	
	switch(Mon){ //select month
		case 1: fillArr("Jan ", ' ', sizeof("Jan "), 0);break;
		case 2: fillArr("Feb ", ' ', sizeof("Feb "), 0);break;
		case 3: fillArr("Mar ", ' ', sizeof("Mar "), 0);break;
		case 4: fillArr("Apr ", ' ', sizeof("Apr "), 0);break;
		case 5: fillArr("May ", ' ', sizeof("May "), 0);break;
		case 6: fillArr("Jun ", ' ', sizeof("Jun "), 0);break;
		case 7: fillArr("Jul ", ' ', sizeof("Jul "), 0);break;
		case 8: fillArr("Aug ", ' ', sizeof("Aug "), 0);break;
		case 9: fillArr("Sep ", ' ', sizeof("Sep "), 0);break;
		case 10: fillArr("Oct ", ' ', sizeof("Oct "), 0);break;
		case 11: fillArr("Nov ", ' ', sizeof("Nov "), 0);break;
		case 12: fillArr("Dec ", ' ', sizeof("Dec "), 0);break;
	}
	
	sprintf(tmp, "%d", Dte); //print day of month
	
	if(Dte < 10){
		fillArr(tmp, '0', tsize, 1);
		fillArr(tmp, ' ', tsize-1, 0);
	}
	else{
		fillArr(tmp, ' ', tsize, 0);
	}
	
	fillArr(", ", ' ', sizeof(", "), 0);
	
	char year[5]; //array for year
	sprintf(year, "%d", 2000+Yr);
	int ysize = sizeof(year); //size of year[]
	fillArr(year, ' ', ysize, 0);
	
	lcdLoad(lcdLine, ' ', sizeof(lcdLine) + 1, 1, 0);
	if (alarmFlag == 1){
		LED = 0;
	}
	clearArr();
	
	
	
	char tempCelsius[5]; //array for Celsius
	int size = sizeof(tempCelsius);
	sprintf(tempCelsius, "%f", tempC);
	
	char tempFahrenheit[5]; //array for Fahrenheit
	sprintf(tempFahrenheit, "%f", tempF);
	
	fillArr("Temp: ", ' ', sizeof("Temp: "), 0);
	fillArr(tempCelsius, ' ', size, 0);
	fillArr(" ",0xDF, 1, 1);
	fillArr(" ",'C', 1, 1);
	fillArr(" ",'(', 1, 1);
	fillArr(tempFahrenheit, ' ', size, 0);
	fillArr(" ",0xDF, 1, 1);
	fillArr(" ",'F', 1, 1);
	fillArr(" ",')', 1, 1);
	
	
	lcdLoad(lcdLine, ' ', sizeof(lcdLine) + 1, 2, 0);
	if (alarmFlag == 1){
		LED = 1;
		wait(0.5);
		LED = 0;
	}
	clearArr();
	
	
	
	int chr;
	int colNum =1;
	float counter = 0;

	
	while(counter < 1.25){
		chr = rowScan(colNum);
		if(chr != 0){
			chr = getNum(chr, colNum);
			if(chr == 15){
				if(alarmFlag == 1){
					i2c.start();
					i2c.write(0xD0);
					i2c.write(0x0f);
					i2c.write(0x00);
					i2c.stop();
					return 0;
				}
				return 1;
			}
			else if(chr == 14 && alarmRing == 0){
				//display alarm settings
				lcd_command(0x01); //clear screen
				lcdLoad("Alarm 1 setting", ' ', sizeof("Alarm 1 setting"), 0,0);
				char tmp[3];
				int tsize = sizeof(tmp);
				sprintf(tmp, "%d", alarmHour);
				
				if(alarmHour < 10){
					fillArr(tmp, '0', tsize, 1);
					fillArr(tmp, ' ', tsize-1, 0);
				}
				else{
					fillArr(tmp, ' ', tsize, 0);
				}
				fillArr(" ", ':', 1, 1);
				
				sprintf(tmp, "%d", alarmMin);
				if(alarmMin < 10){
					fillArr(tmp, '0', tsize, 1);
					fillArr(tmp, ' ', tsize-1, 0);
				}
				else{
					fillArr(tmp, ' ', tsize, 0);
				}
				
				if(alarmAMPM == 1){
					fillArr("pm ", ' ', sizeof("pm "), 0);
				}
				else{
					fillArr("am ", ' ', sizeof("am "), 0);
				}
				
				
				switch(Mon){
					case 1: fillArr("Jan ", ' ', sizeof("Jan "), 0);break;
					case 2: fillArr("Feb ", ' ', sizeof("Feb "), 0);break;
					case 3: fillArr("Mar ", ' ', sizeof("Mar "), 0);break;
					case 4: fillArr("Apr ", ' ', sizeof("Apr "), 0);break;
					case 5: fillArr("May ", ' ', sizeof("May "), 0);break;
					case 6: fillArr("Jun ", ' ', sizeof("Jun "), 0);break;
					case 7: fillArr("Jul ", ' ', sizeof("Jul "), 0);break;
					case 8: fillArr("Aug ", ' ', sizeof("Aug "), 0);break;
					case 9: fillArr("Sep ", ' ', sizeof("Sep "), 0);break;
					case 10: fillArr("Oct ", ' ', sizeof("Oct "), 0);break;
					case 11: fillArr("Nov ", ' ', sizeof("Nov "), 0);break;
					case 12: fillArr("Dec ", ' ', sizeof("Dec "), 0);break;
				}
				
				sprintf(tmp, "%d", alarmDte);
				if(alarmDte < 10){
					fillArr(tmp, '0', tsize, 1);
					fillArr(tmp, ':', tsize-1, 0);
				}
				else{
					fillArr(tmp, ' ', tsize, 0);
				}
				
				fillArr(", ", ' ', sizeof(", "), 0);
				
				char yrs[5];
				sprintf(yrs, "%d", 2000+Yr);
				int ysize = sizeof(yrs);
				fillArr(yrs, ' ', ysize, 0);
				
				lcdLoad(lcdLine, ' ', sizeof(lcdLine) + 1, 1, 0);
				
				
				clearArr();
				
				do{
					chr = rowScan(colNum);
					if(chr != 0){
						chr = getNum(chr, colNum);
					}
					if(colNum == 4){ 
						colNum = 1;
					}
					else{
						colNum++;
					}
					wait(0.03);
				}while(chr != 14 && chr != 15);
				
				switch(chr){
					case 14: return 0;
					case 15: return 1;
				}
			}
		}
		
		if(colNum == 4){ 
			colNum = 1;
		}
		else{
			colNum++;
		}
		wait(0.03);
		counter += 0.03;
	}
	// if alarm flag is set, then write onto LCD and flash the external LED
	//display  current date and temp
	
	return 0;
}


//calculator mode
//to get back to normal mode enter 3 digits then press F
int calcMode(){
	int op1;
	int op2;
	int opNum;
	int result = 0;
  int colNum = 1;
	
	
	op1 = getPad(3); //get 3 inputs from the pad

	result = op1;
	
	opNum = getOpNum(); //get number from function
	
	
	while(opNum != 14 && opNum != 15){ //when these options are not pressed do these.
		switch(opNum){ //define functions
			case 10: lcdPrev(" ", '+', 1, 1 );break;
			case 11: lcdPrev(" ", '-', 1, 1 );break;
			case 12: lcdPrev(" ", '*', 1, 1 );break;
			case 13: lcdPrev(" ", '/', 1, 1 );break;
		}

		op2 = getPad(3); //get 3 numbers from the pad
   
		switch(opNum){ //perform functions
				case 10: result = result + op2; break;// add
				case 11: result = result - op2; break;// subtract
				case 12: result = result * op2; break;// multiply
				case 13: result = result / op2; break;// divide
			  
		}

		opNum = getOpNum(); //get number from function
	}
	
	if (opNum == 15){
		//go to normal mode and clear screen
		lcd_command(0x01);
		return 0;
	}
	else if(opNum == 14){ //display result
		//displayResult(result);
		char test[7];
		//print result
		sprintf(test, "=%d", result);
		lcdLoad(test, ' ', getSize(result), 3, 0);
		wait(0.1);
		opNum = getOpNum(); //get number from function
			if (opNum == 15){
		//go to normal mode and clear screen
		lcd_command(0x01);
		return 0;
	}
		while(rowScan(colNum) == 0){
				if(colNum == 4){
						colNum = 1;
				}
				else{
						colNum++;
				}
				wait(0.05);
		}
		lcd_command(0x01); //clear screen
		return 1;
	}		
}


//keypad logic
//get size of result
int getSize(int result){
	int counter = 6;
	int outcome = 0;
	int startingDiv = 100000;
	if(result == 0){
		return 3;
	}
	
	while(outcome == 0){
		outcome = result/startingDiv;
		
		if(outcome != 0){
			if (result < 0){
				return counter + 3;
			}
			else{
				return counter + 2;
			}
		}
		else{
			counter--;
			startingDiv /= 10;
		}
			
		
	}
}

//set keypad orientation
int getNum(int rowNum, int colNum){
		int numOrder[4][4] = {{1,2,3,10}, 
				 	{4,5,6,11}, 
					 {7,8,9,12}, 
					 {0,15,14,13}};
		
		return numOrder[rowNum-1][colNum-1];
}

//row scanning for keypad
int rowScan(int colNum){
	int rowNum = 0;
	if(colNum == 1){
		C1 = 0;
	}
	else if(colNum == 2){
		C2 = 0;
	}
	else if(colNum == 3){
		C3 = 0;
	}
	else if(colNum == 4){
		C4 = 0;
	}
	
	rowNum = rowPressed();
	
	C1 = 1;
	C2 = 1;
	C3 = 1;
	C4 = 1;
	
	return rowNum;
}

//check if row is pressed
int rowPressed(){
	int num = 0;
	if(R1 == 0){
		num = 1;
	}
	else if(R2 == 0){
		num = 2;
	}
	else if(R3 == 0){
		num = 3;
	}
	else if(R4 == 0){
		num = 4;
	}
	return num;
}


//get the number and save it
int getOpNum(){
	int op = 0;
	int colNum = 1;
	while(op < 10){
		op = rowScan(colNum);
		if(op != 0){
			op = getNum(op, colNum);
		}
		
		if(colNum == 4){ 
			colNum = 1;
		}
		else{
			colNum++;
		}
		wait(0.03);
	}
	return op;
}



