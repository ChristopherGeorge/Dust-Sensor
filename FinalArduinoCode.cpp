
//I would like to credit Maurice Ribble
// who wrote most of the SHT-15 code used here and published it 4-3-2008
// and http://sensorapp.net/?p=479
//Also, the code and project here from Chris Nafis, 
// was very influential in this project
//http://www.howmuchsnow.com/arduino/airquality/

#include <avr.io>; // included to use log function for dew point calculation



// These are commands we need to send to SHT-15 to control it
int gTempCmd  = 0b00000011;
int gHumidCmd = 0b00000101;
//------------------------------------------------------------------------------//


//------------------Variables needed for dust sensor -----------------------//
int dustPin=0;        //value for dust sensor pin
double dustVal=0;     //voltage value for dust sensor return
double dustDensity=0; // density value in milligrams per cubic meter
double microDustDensity=0; //density value in micrograms per cubit meter
double dustPPM=0;     //density of dust in parts per million
double dustPPB=0;    // density of dust in parts per billion
int ledPower=2;       //Value of sensor pin
int delayTime=280;   //keeps LED on for sensor read
int delayTime2=40; 
float offTime=9680;
//------------------------------------------------------------------------------------------//


//------------- functions needed for humidity/temperature settings ----------------//  

// Shift IN function. It removes the first 3 zeros from the left side of the 
// 8-bit command line, returning a 5-bit command line. Either 00011 or 00101.
// this is required to meet manufacture guidelines on how to send a command
// to the sensor
int shiftIn(int dataPin, int clockPin, int numBits)
{
   int ret = 0;
   int i;

   for (i=0; i<numBits; ++i)
   {
      digitalWrite(clockPin, HIGH);                       // turn on clock
      delay(10);                                          // this delay smooths out the reading of the sensor
                                                          // It most likely calms the effect of bouncing
                                                          // Bouncing is the rapid switching from on to off of 
                                                          // an electronic component, such as the sensor.
                                                                              
      ret = ret*2 + digitalRead(dataPin);                 // get info from sensor, 
                                                          // double the last return and add it to the new return
                                                          // this is the shift. if ret = 0, it remains 0. if return = 1
                                                          // it is shifted.
                                                                              
      digitalWrite(clockPin, LOW);                        //turn off clock
   }

   return(ret); //returns 5 bit command line
}

//-------------------
void sendCommandSHT(int command, int dataPin, int clockPin)
{
  int ack;

  // Transmission Start -- effectively flips the off on switch on the clock and the data pins
  pinMode(dataPin, OUTPUT);                //set data pin to output
  pinMode(clockPin, OUTPUT);               //set clock pin to output 
  digitalWrite(dataPin, HIGH);             //turn on data pin
  digitalWrite(clockPin, HIGH);            //turn on clock pin
  digitalWrite(dataPin, LOW);              //turn off data pin
  digitalWrite(clockPin, LOW);             //turn off clock pin
  digitalWrite(clockPin, HIGH);            //turn on clock pin
  digitalWrite(dataPin, HIGH);             //turn on data pin
  digitalWrite(clockPin, LOW);             //turn off clock pin
           
  // The command (3 msb are address and must be 000, and last 5 bits are command)
  shiftOut(dataPin, clockPin, MSBFIRST, command);
  
  // this is a function built into Arduino code. 
  // It sends the data to a shift register, which is a device used to expand the 
  //number of digital outputs. This function sends the data to two pins
  // and gives the order of the data (most significant bit first), and gives the command
  // bits (00000011 or  00000101) to be shifted
 
  // Verify we get the correct acknowledgement (ack) from sensor 
  digitalWrite(clockPin, HIGH);           //turn on clock
  pinMode(dataPin, INPUT);                //set data pin to input
  ack = digitalRead(dataPin);             //check for voltage in data pin 
                                          //-- should be LOW or off
  if (ack != LOW)
    Serial.println("Ack Error -- ack is not LOW");
  
  digitalWrite(clockPin, LOW);             // turn off clock
  ack = digitalRead(dataPin);              // read data pin
  if (ack != HIGH)                         // if ack is LOW, kick out error message
     Serial.println("Ack Error -- ack is not High");
}



//The controller has to wait for the measurement to complete, which can take up to 320ms.
//This function waits 10 ms and pings the dataPin to see if there is data available. 
//If the dataPin returns LOW, then we're done waiting and data is available.
//This is a more efficient use of delay than simply waiting a full 320ms. by 
//repeatedly bugging the dataPin to see if it is done, we only wait
//slightly longer than length of time it takes the sensor to grab the measurement.
//Call this a nagging function. Inefficient in real life, but very efficient in
//electronics
void waitForResultSHT(int dataPin)
{
  int i;                      //counter variable
  int ack;                    //acknowledgement variable, to hold value of ping
  
  pinMode(dataPin, INPUT); //sets the dataPin to input mode
  
  for(i= 0; i < 100; ++i)           //loop produces 
  {
    delay(10);
    ack = digitalRead(dataPin);     //Checks to see if there is 
                                    //any voltage being applied to the dataPin
   
    if (ack == LOW)                       // a return of LOW means no voltage. No 
                                          // voltage means the sensor's measurement 
                                          // is done.               
                                          // if ack == HIGH, then the sensor is still
                                          // working, return to the top of the loop 
      break;
  }
  
  // If, after the loop finishes (after 1 full second of pinging), ack is still HIGH,
  // something is probably wrong with the board, most likely 
  // that the sensor isn't connected properly.
  // This code tests for that.
  if (ack == HIGH)
    Serial.println("Ack error in wait mode -- Ack should not be HIGH after 1 second.");
}


//Retrieves the data from the sensor.
//First it shifts the bits to sent the appropriate command to the sensor
//Then it sens an ack signal, flip the datapin on and off, 
//then flip the clock on and off
//(the ack signal is specified by the sensor manufacturer)
//then get the data
int getData16SHT(int dataPin, int clockPin)
{
  int val;
  
  // Get the most significant bits
  pinMode(dataPin, INPUT);
  pinMode(clockPin, OUTPUT);
  val = shiftIn(dataPin, clockPin, 8);    //Shift the bits, baby.
  val *= 256; //multiplies the value by 256. 
                     //This is so the least significant bits can be retrieved later.

  // Send the required ack
  pinMode(dataPin, OUTPUT);         //set dataPin to output  
  digitalWrite(dataPin, HIGH);          //flip dataPin on and off 
  digitalWrite(dataPin, LOW);           
  digitalWrite(clockPin, HIGH);        // flip clock on and off 
                                                           // (clock previously set to output)
  digitalWrite(clockPin, LOW);
           
  // Get the lest significant bits
  pinMode(dataPin, INPUT);
  val |= shiftIn(dataPin, clockPin, 8); //shift val back to least sig bits

  return val; 
  //returns the data as a number that is then converted into RH or temp,
  //Depending on conversions done later in the code.
}

//The sensor comes with a checksum capability,
//which double checks the data stream for
//bad bits and eliminates them.
//This function turns that error checking off.

void skipCrcSHT(int dataPin, int clockPin)
{
  // Skip acknowledge to end trans (no CRC)
  pinMode(dataPin, OUTPUT);           //set data and clock to output
  pinMode(clockPin, OUTPUT);

  digitalWrite(dataPin, HIGH);            //set data signal to High, clock to high,   
  digitalWrite(clockPin, HIGH);           //then clock to low. Signal set by
  digitalWrite(clockPin, LOW);            //manufacturer to tell sensor not to send
}                                                            // ack signal

//--------------------------end functions for humidity and temp sensors -----------------------------//



//Arduino programming requires two functions in each program.
//Void setup() is the first, and it establishes an open serial port between the computer and the Arduino board.
//It can also be used, as it is here, to set up pins on the Arduino board to output information to the serial port
//The serial port is used to feed data to the computer, which can then be ported to a web site.

void setup() 
{
   Serial.begin(9600); // open serial
   pinMode(ledPower,OUTPUT);  //pin defs for temp/humidity sensors
   pinMode(4, OUTPUT);              //pin defs for temp/humidity sensors

}

void loop()
{
  int theDataPin  = 10; //assigns pin 10 on Arduino board to data
  int theClockPin = 11; //assigns pin 11 on Arduino board to clock. Pin 11 is a digital read 
  int ack;
  int tempF = 0; //variable for Fahrenheit temperature
  int tempC = 0; //variable for Celsius temp
  double val = 0; 
  int humid = 0; 
  double humidReal = 0;
  double dewPointP1;
  double dewPointP2;
  double dewPt;
  
  
  //Sets up infinite loop of reading data from sensors
  while (1)
  {
       //**************Read Temperature*********************//
                       
           sendCommandSHT(gTempCmd, theDataPin, theClockPin);           //read temperature sensor
           
           waitForResultSHT(theDataPin);                                //wait for sensor 
                                                                        //to complete measurement
           
           val = getData16SHT(theDataPin, theClockPin);                 //send value from temp sensor to variable
           
           skipCrcSHT(theDataPin, theClockPin);
           Serial.print("Temp is:");
           tempF = -40.0 + 0.018 * val;   // conversion to Fahreinheit per manufacturer
           tempC = -40 + 0.01 * val;       // conversion to Celsius per manufacturer
           Serial.print("  ");
           Serial.print(tempF, DEC);
           Serial.print(" deg. F/");
           Serial.print(" ");
           Serial.print(tempC,DEC);
           Serial.println(" C");
           Serial.print("The approximate dewpoint is: ");
           Serial.print(dewPt, 2);
           Serial.println(" C");
           Serial.println("");         
           delay (3000); //wait three seconds (3000 ms)
           
           
      //***************************Read Humidity*******************//
            val = 0; //clear val of temp data
                    
           sendCommandSHT(gHumidCmd, theDataPin, theClockPin);          //read humidity sensor
           waitForResultSHT(theDataPin);
           val = getData16SHT(theDataPin, theClockPin);                 //assign val value of humidity sensor    
           skipCrcSHT(theDataPin, theClockPin);
           Serial.print("Relative humidity is: ");
           humid = -4 + (0.0405 * val) + (-0.0000028 * val * val);            // conversion of number between 0 and 1023
                                                                              // to humidity value. Manufacturer supplied
                                                                              // conversion factor. This factor has not been
                                                                              // independently tested.

           humidReal = (tempC-25)*(0.01+0.00008*val) + humid;                 //conversion to temperature related
                                                                              //humidity, or a real relative humidity
                                                                              //relative humidity is relative to the total
                                                                              //water vapor capacity of the air.
                                                                              //Water vapor capacity of air changes
                                                                              //with temperature -- the capacity rises as
                                                                              //temp rises, so the most accurate
                                                                              //relative humidity calculation takes
                                                                              //temperature into account

           dewPointP1 = log(humidReal/100) + (17.62 * tempC)/(243.12 + tempC);
           dewPointP2 = 17.62 - (log(humidReal/100) - (17.62 * tempC)/(243.12 + tempC));
           dewPt = 243.12 * (dewPointP1/dewPointP2); 

           //This is an approximate dew point
           //which was provided by the manufacturer
           //These calculations are only valid above water (not above ice)
           // in temperatures between 0 and 50C. 

           Serial.print(" ");
           Serial.print(humid, DEC);
           Serial.println("%");
           Serial.print("Temp-adjusted relative humidity is: ");
           Serial.print(humidReal, 2);
           Serial.println("%");
           Serial.println("");  
          delay (3000);
         
         //*******************read dust sensor********************//
         digitalWrite(ledPower,LOW);               // power on the LED
         delayMicroseconds(delayTime);
         dustVal=analogRead(dustPin);              // read the dust value via pin 5 on the sensor
         dustVal = 5 * (dustVal/1024)-1.05;        // analog pin kicks out number between 0 and 1024, 
                                                   // representing 0V to 5V and
                                                   // subtracting min voltage output (0.9V)
                                                   // Calculation converts number to voltage
                                                   // And then subtracts minimum,
                                                   // no dust reading. (1.05V)
                                                   
         dustDensity = dustVal / (0.5/.1);         // from manufacturer, represents 0.5V/.1 milligrams/m^3
         microDustDensity = dustVal / (0.5/100);   // converstion to micrograms/m^3
         dustPPM = 24.45 * (dustDensity/28.01);    // converts milligrams/m^3 to ppm
         dustPPB = 1000 * dustPPM;                 // converts to PPB, the measurement used by 
                                                   // The San Joaquine Air Pollution Control District
                                                   
         delayMicroseconds(delayTime2);
         digitalWrite(ledPower,HIGH);              // turn the LED off
         delayMicroseconds(offTime);
         delay(100);
         Serial.print("Dust sensor voltage return is: ");
         Serial.print(" ");
         Serial.print(dustVal);
         Serial.println("V");
         Serial.print("Dust density is: "); 
         Serial.print(microDustDensity);
         Serial.println(" micrograms/cubic meter");
         Serial.print("Dust in parts per million is: "); 
         Serial.print(dustPPM);
         Serial.println(" ppm");
         Serial.print("Dust in parts per billion is: "); 
         Serial.print(dustPPB);
         Serial.println(" ppb");
         Serial.println("");
         delay(3000);        
}
}

