//Arduino code to register a footsetep
//By Srijay Kasturi, (c) 2013

#include <Wire.h> // Used for I2C
#include <SoftwareSerial.h>

// The SparkFun breakout board defaults to 1, set to 0 if SA0 jumper on the bottom of the board is set
#define MMA8452_ADDRESS 0x1D  // 0x1D if SA0 is high, 0x1C if low

//Define a few of the registers that we will be accessing on the MMA8452
#define OUT_X_MSB 0x01
#define XYZ_DATA_CFG  0x0E
#define WHO_AM_I   0x0D
#define CTRL_REG1  0x2A

#define GSCALE 2 // Sets full-scale range to +/-2, 4, or 8g. Used to calc real g values.

//accelerometer
float xOldGVal    = -0.01;
float xOldGChange = -0.01;
float xNewGChange = -0.01;
float yOldGVal    = -0.01;
float yOldGChange = -0.01;
float yNewGChange = -0.01;
float zOldGVal    = -0.01;
float zOldGChange = -0.01;
float zNewGChange = -0.01;
boolean tapDone   = false;
int rounds        = 0;
int tapAmt        = 0;

//vibrator
int vibrPin = 9;

//distance
const int distPin = 3;
long distVolt, distInches;
int distSum = 0;
int distReading = 25;

void setup()
{
  Serial.begin(9600);
  delay(2000);
  Serial.println("start1");
  Wire.begin(); //Join the bus as a master
  Serial.println("start");
  initMMA8452(); //Test and intialize the MMA8452
  pinMode(vibrPin, OUTPUT);
  pinMode(distPin, INPUT);
  int i = 0;
  while(i < 5) {
    distRead();
    i++;
    analogWrite(vibrPin, 0);
  }
}

void loop()
{ 
  delay(1000); 
  int accelCount[3];  // Stores the 12-bit signed value
  readAccelData(accelCount);  // Read the x/y/z adc values

  // Now we'll calculate the accleration value into actual g's
  float accelG[3];  // Stores the real accel value in g's
  for (int i = 0 ; i < 3 ; i++)
  {
    accelG[i] = (float) accelCount[i] / ((1<<12)/(2*GSCALE));  // get actual g value, this depends on scale being set
  }
  tapRead(accelG[0], accelG[1], accelG[2]);  
  //distRead();
  Serial.println();

  delay(10);  // Delay here for visibility
}


//ACCELEROMETER CODE
//
//DO NOT TOUCH

void tapRead(float xNewGVal, float yNewGVal, float zNewGVal) {

  //compare z axis
  //  if (tapDone == false) {
  zOldGChange = zNewGChange;
  //  }
  zNewGChange = zOldGVal - zNewGVal;
  //  if (tapDone == false) {
  zOldGVal = zNewGVal;
  //  }
  float zComp = zOldGChange - zNewGChange;
  float zOldThresh = zOldGChange * 1.5;
  float zNewThresh = zNewGChange * 1.5;
  zComp = abs(zComp);
  zOldThresh = abs(zOldThresh);
  zNewThresh = abs(zNewThresh);

  //debugInt("rounds", rounds, true);
  debugInt("tapDone", tapAmt, true);
  //debugFloat("zO", zOldGChange, false);
  //debugFloat("zN", zNewGChange, false);
  //debugFloat("zOT", zOldThresh, false);
  //debugFloat("zNT", zNewThresh, true);
  if (zNewThresh >= .5){
    /*  //distRead(true);
     //digitalWrite(9, HIGH);
     //delay(500);
     //digitalWrite(9, LOW);
     tapDone = true;
     tapAmt++;
     zNewThresh = zOldThresh;
     debugFloat("zNTc", zNewThresh, true);
     debugBoolean("tD", tapDone, true);*/
    distRead();
    tapDone = true;
  }
  else {
    tapDone = false;
  }
  rounds++;

  //commence printing of needed value
}

void readAccelData(int *destination)
{
  byte rawData[6];  // x/y/z accel register data stored here

  readRegisters(OUT_X_MSB, 6, rawData);  // Read the six raw data registers into data array

  // Loop to calculate 12-bit ADC and g value for each axis
  for(int i = 0; i < 3 ; i++)
  {
    int gCount = (rawData[i*2] << 8) | rawData[(i*2)+1];  //Combine the two 8 bit registers into one 12-bit number
    gCount >>= 4; //The registers are left align, here we right align the 12-bit integer

    // If the number is negative, we have to make it so manually (no 12-bit data type)
    if (rawData[i*2] > 0x7F)
    {  
      gCount = ~gCount + 1;
      gCount *= -1;  // Transform into negative 2's complement #
    }

    destination[i] = gCount; //Record this gCount into the 3 int array
  }
}

// Initialize the MMA8452 registers 
// See the many application notes for more info on setting all of these registers:
// http://www.freescale.com/webapp/sps/site/prod_summary.jsp?code=MMA8452Q
void initMMA8452()
{
  byte c = readRegister(WHO_AM_I);  // Read WHO_AM_I register
  if (c == 0x2A) // WHO_AM_I should always be 0x2A
  {  
    Serial.println("MMA8452Q is online...");
  }
  else
  {
    Serial.print("Could not connect to MMA8452Q: 0x");
    Serial.println(c, HEX);
    while(1) ; // Loop forever if communication doesn't happen
  }

  MMA8452Standby();  // Must be in standby to change registers

  // Set up the full scale range to 2, 4, or 8g.
  byte fsr = GSCALE;
  if(fsr > 8) fsr = 8; //Easy error check
  fsr >>= 2; // Neat trick, see page 22. 00 = 2G, 01 = 4A, 10 = 8G
  writeRegister(XYZ_DATA_CFG, fsr);

  //The default data rate is 800Hz and we don't modify it in this example code

  MMA8452Active();  // Set to active to start reading
}

// Sets the MMA8452 to standby mode. It must be in standby to change most register settings
void MMA8452Standby()
{
  byte c = readRegister(CTRL_REG1);
  writeRegister(CTRL_REG1, c & ~(0x01)); //Clear the active bit to go into standby
}

// Sets the MMA8452 to active mode. Needs to be in this mode to output data
void MMA8452Active()
{
  byte c = readRegister(CTRL_REG1);
  writeRegister(CTRL_REG1, c | 0x01); //Set the active bit to begin detection
}

// Read bytesToRead sequentially, starting at addressToRead into the dest byte array
void readRegisters(byte addressToRead, int bytesToRead, byte * dest)
{
  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(addressToRead);
  Wire.endTransmission(false); //endTransmission but keep the connection active

    Wire.requestFrom(MMA8452_ADDRESS, bytesToRead); //Ask for bytes, once done, bus is released by default

  while(Wire.available() < bytesToRead); //Hang out until we get the # of bytes we expect

  for(int x = 0 ; x < bytesToRead ; x++)
    dest[x] = Wire.read();
}

// Read a single byte from addressToRead and return it as a byte
byte readRegister(byte addressToRead)
{
  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(addressToRead);
  Wire.endTransmission(false);   //endTransmission but keep the connection active

    Wire.requestFrom(MMA8452_ADDRESS, 1); //Ask for 1 byte, once done, bus is released by default

  while(!Wire.available()) ; //Wait for the data to come back
  return Wire.read(); //Return this one byte
}

// Writes a single byte (dataToWrite) into addressToWrite
void writeRegister(byte addressToWrite, byte dataToWrite)
{
  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(addressToWrite);
  Wire.write(dataToWrite);
  Wire.endTransmission(); //Stop transmitting
}

void vibrate(int length) {
  analogWrite(vibrPin, (length * 4 + 100));
  //delay(1000);
  //analogWrite(vibrPin, 0);  
}

//DISTANCE CODE
//
//
void distRead() {
  for(int i = 0; i < distReading ; i++)
  {
    distVolt = analogRead(distPin)/2;
    distSum += distVolt;
    delay(5);
  }  
  int time = millis();
  distInches = distSum/distReading;
  debugInt("dist", distInches, true);
  vibrate(distInches);
  //reset sample total
  distSum = 0;
  delay(500);
}

//DEBUGGING CODE
//
//DO NOT TOUCH
void debugInt(String varName, int varValue, boolean name) {
  if(name == true) {
    Serial.print(varName);
    Serial.print(": ");
  }
  if(varValue < 10 && varValue > 0) {
    Serial.print("0");
  }
  Serial.print(varValue);
  Serial.print(" ");
}
void debugFloat(String varName, float varValue, boolean name) {
  if(name == true) {
    Serial.print(varName);
    Serial.print(": ");
  }
  if(varValue >= 0) {
    Serial.print("+");
  }
  Serial.print(varValue);
  Serial.print(" ");
}
void debugBoolean(String varName, boolean varValue, boolean name) {
  if(name == true) {
    Serial.print(varName);
    Serial.print(": ");
  }
  Serial.print(varValue);
  Serial.print(" ");
}
`
