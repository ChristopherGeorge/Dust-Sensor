

int gTempCmd  = 0b00000011; //variable initialized to binary 3
int gHumidCmd = 0b00000101; // variable initialized to binary 5
//------------------------------------------------------------------------------//


//------------------Variables needed for dust sensor -----------------------//
int dustPin=0; //value for dust sensor pin
double dustVal=0; //voltage value for dust sensor return
double dustDensity=0; // variable needed to calculate dust density
int ledPower=2; //Arduino board pin number for power to the LED. The LED is used to light the dust.
int delayTime=280; //in milliseconds. 
int delayTime2=40;
float offTime=9680;
//----------------------------------------------------------------------------------//

//------------- function needed for humidity/temperature settings ----------------//  
int shiftIn(int dataPin, int clockPin, int numBits)
{
   int ret = 0;
   int i;

   for (i=0; i<numBits; ++i)
   {
      digitalWrite(clockPin, HIGH);
      delay(10);  // 
	  ret = ret*2 + digitalRead(dataPin);
      digitalWrite(clockPin, LOW);
   }

   return(ret);
}

void sendCommandSHT(int command, int dataPin, int clockPin)
{
  int ack;

  // Transmission Start
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  digitalWrite(dataPin, HIGH);
  digitalWrite(clockPin, HIGH);
  digitalWrite(dataPin, LOW);
  digitalWrite(clockPin, LOW);
  digitalWrite(clockPin, HIGH);
  digitalWrite(dataPin, HIGH);
  digitalWrite(clockPin, LOW);
           
  // The command (3 msb are address and must be 000, and last 5 bits are command)
  shiftOut(dataPin, clockPin, MSBFIRST, command);

  // Verify we get the correct acknowledgement (ack) from sensor 
  digitalWrite(clockPin, HIGH);
  pinMode(dataPin, INPUT);
  ack = digitalRead(dataPin);
  if (ack != LOW)
    Serial.println("Ack Error -- ack is not LOW");
  digitalWrite(clockPin, LOW);
  ack = digitalRead(dataPin);
  if (ack != HIGH)
     Serial.println("Ack Error -- ack is not High");
}

void waitForResultSHT(int dataPin)
{
  int i;
  int ack;
  
  pinMode(dataPin, INPUT);
  
  for(i= 0; i < 100; ++i)
  {
    delay(10);
    ack = digitalRead(dataPin);

    if (ack == LOW)
      break;
  }
  
  if (ack == HIGH)
    Serial.println("Ack Error should not be HIGH");
}

int getData16SHT(int dataPin, int clockPin)
{
  int val;
  
  // Get the most significant bits
  pinMode(dataPin, INPUT);
  pinMode(clockPin, OUTPUT);
  val = shiftIn(dataPin, clockPin, 8);
  val *= 256;

  // Send the required ack
  pinMode(dataPin, OUTPUT);
  digitalWrite(dataPin, HIGH);
  digitalWrite(dataPin, LOW);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
           
  // Get the lest significant bits
  pinMode(dataPin, INPUT);
  val |= shiftIn(dataPin, clockPin, 8);

  return val;
}

void skipCrcSHT(int dataPin, int clockPin)
{
  // Skip acknowledge to end trans (no CRC)
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  digitalWrite(dataPin, HIGH);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
}

//--------------------------end functions for humidity and temp sensors -----------------------------//

void setup()
{
   Serial.begin(9600); // open serial
   pinMode(ledPower,OUTPUT);  //pin defs for temp/humidity sensors
   pinMode(4, OUTPUT);              //pin defs for temp/humidity sensors

}

void loop()
{
  int theDataPin  = 10;
  int theClockPin = 11;
  char cmd = 0;
  int ack;
  
  while (Serial.available() > 0)
  {
      cmd = Serial.read();
      
      
      
      {
      //Read Temperature
         {
           int Tval;
           int temp;
           
           sendCommandSHT(gTempCmd, theDataPin, theClockPin); //activate the temp sensor
           waitForResultSHT(theDataPin); //wait for results
           Tval = getData16SHT(theDataPin, theClockPin); //send results to val variable
           skipCrcSHT(theDataPin, theClockPin);
           Serial.print("Temp is:");
           
           temp = -40.0 + 0.018 * (float)Tval; // conversion to Fahreinheit
           Serial.print("  ");
           Serial.print(temp, DEC);
           Serial.println(" degrees F.");         
           
         
         delay (500); //wait half a second (500 ms)
            //***********************************Read Humidity***********************///
           int Hval;
           int humid;
           sendCommandSHT(gHumidCmd, theDataPin, theClockPin); //activate the humidity sensor
           waitForResultSHT(theDataPin); //wait for results
           Hval = getData16SHT(theDataPin, theClockPin);
           skipCrcSHT(theDataPin, theClockPin);
           Serial.print("Humidity is: ");
           humid = -4.0 + 0.0405 * Hval + -0.0000028 * Hval * Hval;
           Serial.print("  ");
           Serial.println(humid, DEC);
                           
         //******************************read dust sensor***************************//
         digitalWrite(ledPower, LOW); // power on the LED
         delayMicroseconds(delayTime); //delay lets the LED stay on long enough for the photo sensor to get a reading.
         dustVal=analogRead(dustPin); // read the dust value via pin 5 on the sensor
         dustVal = 5 * (dustVal/1024);   //converts value from a number between 0 and 1024 (which is returned by the
                                                            //to a voltage value   
         dustDensity = dustVal * 0.5/0.1; //uses manufacturer specs to convert dust voltage value to 
         
         delayMicroseconds(delayTime2);
         digitalWrite(ledPower,HIGH); // turn the LED off
         delayMicroseconds(offTime);
         delay(3000);
         Serial.print("Dust voltage is: ");
         Serial.print(" ");
         Serial.println(dustVal);
         Serial.print("Dust density is: " 
         Serial.print(" ");
         Serial.println(dustDensity);
         }
   }
}
