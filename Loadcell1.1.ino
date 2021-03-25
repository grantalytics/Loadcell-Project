/*
 ********************************************************************************
  Project         :  Scaleboard w/ Display
  Author          :  Grant Kunnemann
  Date Created    :  8-19-2020
  Purpose         :  Scale board compatible with the Medcare/Handicare patient lifts.
                   
  Instructions    :  Plug 24V DC battery into the 2- pin male connector on the back
                     of the board. Plug the loadcell into the 4- pin male connector
                     on the back of the board. Press "Weigh" button to turn on
                     display and zero button to tare the loadcell. The slide switch
                     at the bottom of the board may be used to display kg or lbs.
                     To calibrate, hold both buttons down before turning the power
                     on and continue to hold for 10 seconds. Once the spinning
                     animation starts, you may let go of the buttons and follow the
                     on-screen instructions.
                     
  Revision History:

  Date        Author       REF        Description
09/22/2020    Grant Kunnemann     increasing accuracy     Added Copyright statement. Changed "stabilizingtime" from 5000 to 3000. Increase samples in use from 16 to 32 to increase accuracy
               
         A
        ---
     F |   | B
        -G-
     E |   | C
        ---
        D
 © 2020, Lone Star Integrated Technologies. All rights reserved.
*/


#include <HX711_ADC.h>               // For HX711 Loadcell chip
#include <EEPROM.h>                  // For saving Calibration data
#include <SevenSegment.h>            // Seven Segment Display

#define FEATURE_COUNT 2    // Number of times to try each feature
#define DIGIT_COUNT 4     // Number of digits suuported in screen
#define WAIT 100      // Delay between frames

//Display
const int CLKdisplay = 6;
const int DIN_Display = 10;
const int LOAD = 5;

//Loadcell
const int DOUT = A3;                  
const int CLKLoadcell = A4;          

//Button inputs
const int slide_unit_SW = A5;         //Slide switch to switch between Kg's and lb's
const int TARE_SW = 8;               //Tare push button
const int WEIGHT_SW = 9;    //Weight button*****Acts as on/off reset button

//Location to store calibration data
const int calVal_eepromAddress = 0;  

//Setting up inputs
int buttonstate_TARE = 0;            
int buttonstate_WEIGHT = 0;
int switchstate = 0;

long stabilizingtime = 3000;
boolean _tare = false;
static boolean newDataReady = 0;  

//Debounce for button presses
unsigned long LastDebounceTime = 0;
unsigned long DebounceDelay = 50;


int Loadcell_Startup = 0;    //Will have to move setup data for loadcell into the loop code. This will only activate setup code once.
int digitLocation[4] = {0, 8, 17, 25};

float CalVal = EEPROM.get(calVal_eepromAddress, CalVal);     //Pull Calibration factor from EEPROM
float NewCalVal;

HX711_ADC Loadcell(DOUT,CLKLoadcell);      // Construct Loadcell
SevenSegment screen(CLKdisplay,DIN_Display, LOAD);

void setup() {
 
  Loadcell.setSamplesInUse(32);   //Override default 16 sample rate
  pinMode(slide_unit_SW, INPUT);  //Setting up button INPUTS
  pinMode(TARE_SW, INPUT);
  pinMode(WEIGHT_SW, INPUT);
  screen.begin("AY0438","8.8|8.8");
  screen.clear();
  buttonstate_TARE = digitalRead(TARE_SW);
  buttonstate_WEIGHT = digitalRead(WEIGHT_SW);
  if(buttonstate_TARE == HIGH && buttonstate_WEIGHT == HIGH)
    {
      Loadcell.begin();

      Loadcell.start(stabilizingtime,_tare);
      Loadcell.setCalFactor(1.0);
      boolean _resume = false;
  while (_resume == false) {
      Loadcell.update();
      Loadcell.tare();
      if( Loadcell.getTareStatus() == true)
      {
        _resume = true;
      }
  }

      spin();

      screen.clear();
      while (digitalRead(WEIGHT_SW) == LOW)
      {
          char buf[] = "PLACE 50lb WEIGHT ON SCALE AND PRESS WEIGH BUTTON   ";

          for (char* p = buf; p < buf + strlen(buf); p++)
          {
            screen.print(p);
            delay(WAIT * 5);
            if(digitalRead(WEIGHT_SW) == HIGH)
            {
              break;
            }
 
          }

         screen.clear();
      }
      float known_mass = 50;

      Loadcell.update();
      screen.clear();
      Loadcell.refreshDataSet();
      NewCalVal = Loadcell.getNewCalibration(known_mass);
      EEPROM.put(calVal_eepromAddress,NewCalVal);
      screen.print("done");
     }
   }
 


void loop() {
buttonstate_WEIGHT = digitalRead(WEIGHT_SW); // check if Weight button has been pressed to turn on loadcell display
while(Loadcell_Startup == 1 || buttonstate_WEIGHT == HIGH) //while loop to enable weight pushbutton to start display. It will keep running after button is depressed. The display will only stop once the unit is powered down
{
  if (Loadcell_Startup == 0)
  {
    Loadcell.begin();                               //initialize loadcell and power up
    Loadcell.start(5000, true);         //Start loadcell after 5 seconds of stabilize time and taring
    if (Loadcell.getTareTimeoutFlag() || Loadcell.getSignalTimeoutFlag())  //If plugged incorrectly will display error
    {
      screen.print("Err");
      while (1);
    }
    Loadcell.setCalFactor(CalVal);    //Setting Calibration Factor
    Loadcell_Startup = 1;                          //Ending this loop after one cycle. Does not need to be looped continuously
  }
while(buttonstate_TARE != HIGH ) // Keep looping code unless the tare button is pressed. Then tare the scale and continue looping
{
    char temp[8];
    Loadcell.update();
    float i = Loadcell.getData();                  //Placeholder for loadcell readings
    switchstate = digitalRead(slide_unit_SW);      //Check which side the slide switch is switched to.
    if (switchstate == HIGH)                     //If it is switched to the KG side convert the reading to KGs.
    {                                            //If it is on the lb's side. Do nothing
        i = i * (0.45359237);
    }
    dtostrf(i, 4, 1, temp);
    screen.print(temp);
    newDataReady = 0;     //switch data to old and loop to check if there is new data available
    if(digitalRead(TARE_SW) == HIGH)
    {
        Loadcell.tare();  
    }
    break;
    }
    if(buttonstate_TARE == HIGH){
      Loadcell.tare();  


  }
  }
}

// Do spin animation
void spin(){

  for (int t = 0; t < FEATURE_COUNT; t++){
    for (int s = 0; s < 6; s++){

      // Turn on specific segments
      for (int d = 0; d < DIGIT_COUNT; d++){
        screen.setSegment(digitLocation[d] + s, true);
      }

      // Send display information to screen & wait
      screen.display();
      delay(WAIT);

      // Turn the segments back off internally
      for (int d = 0; d < DIGIT_COUNT; d++){
        screen.setSegment(digitLocation[d] + s, false);
      }
    }
  }
}
