// Simple strand test for Adafruit Dot Star RGB LED strip.
// This is a basic diagnostic tool, NOT a graphics demo...helps confirm
// correct wiring and tests each pixel's ability to display red, green
// and blue and to forward data down the line.  By limiting the number
// and color of LEDs, it's reasonably safe to power a couple meters off
// the Arduino's 5V pin.  DON'T try that with other code!
#include<stdio.h>
#include <Adafruit_DotStar.h>
// Because conditional #includes don't work w/Arduino sketches...
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
//#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET

#define NUM_PIXELS 26 // Number of LEDs in strip
//#define LED_PIN 13
#define MAX_RAND 6

#define RANDOM_COLOR { random(0xAAFFCC)}

// Here's how to control the LEDs from any two pins:
#define DATAPIN    11 //green wire
#define CLOCKPIN   13 //yellow wire
Adafruit_DotStar strip = Adafruit_DotStar(
  NUM_PIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
// The last parameter is optional -- this is the color data order of the
// DotStar strip, which has changed over time in different production runs.
// Your code just uses R,G,B colors, the library then reassigns as needed.
// Default is DOTSTAR_BRG, so change this if you have an earlier strip.

// Hardware SPI is a little faster, but must be wired to specific pins
// (Arduino Uno = pin 11 for data, 13 for clock, other boards are different).
//Adafruit_DotStar strip = Adafruit_DotStar(NUM_PIXELS, DOTSTAR_BRG);

void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  Serial.begin(115200);
  Serial.println("start");
//  pinMode(LED_PIN,OUTPUT);
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}

// Runs 10 LEDs at a time auint32_t strip, cycling through red, green and blue.
// This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.
int      head  = 0, tail = 1-NUM_PIXELS; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)

#define DELAY_MAX   100
#define DELAY_MIN   5
int ms_delay = ((DELAY_MAX - DELAY_MIN) / 2 ) + DELAY_MIN;
int delay_increment = (DELAY_MAX - DELAY_MIN) / NUM_PIXELS;


void sendPixelArray(uint32_t * send_arr){
  static uint32_t curr_pixels[NUM_PIXELS];
  //loop over all pixels in array
  for (int i = 0; i < NUM_PIXELS; i++){
    //if pixel changed, then update it and update our static array
    if(curr_pixels[i] != send_arr[i]){
      curr_pixels[i] = send_arr[i];
      strip.setPixelColor(i, send_arr[i]);
    }
  }
  strip.show();                     // Refresh strip
}

void printHex(uint32_t val){
  char charVal[6];
  sprintf(charVal, "%06X", val);
  Serial.println(charVal);
}

//returns and integer color
uint32_t averagePixels(uint32_t* arr_in , long num_pixels){
  uint32_t ave_R= 0;
  uint32_t ave_B =0;
  uint32_t ave_G = 0;
  uint32_t temp;
  for(int j = 0; j < num_pixels; j++){
//    ave_R += ( arr_in[ j ]  & 0xFF0000 )>>16;
//    ave_G += ( arr_in[ j ]  & 0x00FF00 )>>8;
    temp = arr_in[ j ] ;
    temp = temp & 0x0000FF;
    ave_B += temp; 
    temp = arr_in[ j ] ;
    temp = temp & 0x00FF00;
    ave_G += temp; 
    temp = arr_in[ j ] ;
    temp = temp & 0xFF0000;
    ave_R += temp; 
  }
//  Serial.println("break");
  ave_R /= num_pixels;
  ave_G /= num_pixels;
  ave_B /= num_pixels;
//  ave_R = ave_R << 16;
//  ave_G = ave_G << 8;
  ave_R &= 0xFF0000;
  ave_G &= 0x00FF00;
  ave_B &= 0x0000FF;
  temp = ave_R + ave_G + ave_B;

  return temp; 
}

void AveDownSampleArrays( uint32_t * virt_arr, uint32_t * real_arr, int pixel_scaling, int real_len){
   int j;
   uint32_t ave;
   int offset;
   //loop through our virtual array
  for (int i = 0; i < real_len; i++){
    ave = 0;
    offset = i*pixel_scaling;
    real_arr[i] = averagePixels( &(virt_arr[offset]) , pixel_scaling);
  }
}

int UpdateDelay(int loc_delay){
  static int loc_delay_increment = 2;
   loc_delay += loc_delay_increment;
   if(loc_delay > DELAY_MAX){
    loc_delay_increment = -1*loc_delay_increment;
   }else if(loc_delay < DELAY_MIN){
    loc_delay_increment = -1*loc_delay_increment;
   }
   Serial.println(loc_delay);
   return loc_delay;
}

//This drops a pulse into the strip
void InitiatePulse( uint32_t* arr, int num_pixels){
  uint32_t color = RANDOM_COLOR;
  for(int i = 0; i < num_pixels; i++){
    arr[i] = color;
  }
}

#define COUNTER_MAX 6
//This just moves the pixels down the array 
void StripPropagateBasic(uint32_t* arr, int len){
    //move the entire array up one
  for(int i = len-1; i >0; i--){
    arr[i] = arr[i-1];
  }
 arr[0] = 0; //clear out the 0th pixel
}


//initial pattern where one color fills the fan blades fully, then another color chases it out
void ColorChaserBasic(){
  while(1){
    int color_increment = MAX_RAND - random(MAX_RAND);
    
  //  Serial.println("A");
    strip.setPixelColor(head, color); // 'On' pixel at head
    strip.setPixelColor(tail, 0);     // 'Off' pixel at tail
    strip.show();                     // Refresh strip
    delay(ms_delay);                        // Pause 20 milliseconds (~50 FPS)
  
     ms_delay += delay_increment;
  //   delay_increment+=2;
  //   if(ms_delay > DELAY_MAX){
  //    delay_increment = -1*delay_increment;
  //   }else if(ms_delay < DELAY_MIN){
  //    delay_increment = -1*delay_increment;
  //   }
    
    if(++head >= NUM_PIXELS) {         // Increment head index.  Off end of strip?
      head = 0;                       //  Yes, reset head index to start
  //    if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
        color = RANDOM_COLOR ;            //   Yes, reset to red
        ms_delay = DELAY_MIN;
    }
    if(++tail >= NUM_PIXELS) tail = 0; // Increment, reset tail index
  }
}


#define UPSCALER  2 //numver of virtual pixels per real pixel
void loop() {
  
  static int counter = 0;
  //this is our array of pixels that is manupulated and then sent out
  static uint32_t pixels_arr[NUM_PIXELS];
  static uint32_t virtual_arr[NUM_PIXELS * UPSCALER];

//  ColorChaserBasic();


  StripPropagateBasic(virtual_arr, NUM_PIXELS * UPSCALER);
  if(counter++ >=UPSCALER*2 ) {
    InitiatePulse(virtual_arr, 2);
//    counter = random(COUNTER_MAX);
    counter = 0;
  }
  AveDownSampleArrays( virtual_arr, pixels_arr,UPSCALER, NUM_PIXELS);
//  ms_delay = UpdateDelay(ms_delay);

  sendPixelArray(pixels_arr);
//  ms_delay = UpdateDelay(ms_delay);
  delay(ms_delay); //pause between loops
}
