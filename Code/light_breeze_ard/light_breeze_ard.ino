// Simple strand test for Adafruit Dot Star RGB LED strip.
// This is a basic diagnostic tool, NOT a graphics demo...helps confirm
// correct wiring and tests each pixel's ability to display red, green
// and blue and to forward data down the line.  By limiting the number
// and color of LEDs, it's reasonably safe to power a couple meters off
// the Arduino's 5V pin.  DON'T try that with other code!

#include <Adafruit_DotStar.h>
// Because conditional #includes don't work w/Arduino sketches...
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
//#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET

#define NUMPIXELS 26 // Number of LEDs in strip
//#define LED_PIN 13

#define RANDOM_COLOR { random(0xAAFFCC)}

// Here's how to control the LEDs from any two pins:
#define DATAPIN    11 //green wire
#define CLOCKPIN   13 //yellow wire
Adafruit_DotStar strip = Adafruit_DotStar(
  NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
// The last parameter is optional -- this is the color data order of the
// DotStar strip, which has changed over time in different production runs.
// Your code just uses R,G,B colors, the library then reassigns as needed.
// Default is DOTSTAR_BRG, so change this if you have an earlier strip.

// Hardware SPI is a little faster, but must be wired to specific pins
// (Arduino Uno = pin 11 for data, 13 for clock, other boards are different).
//Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BRG);

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

// Runs 10 LEDs at a time along strip, cycling through red, green and blue.
// This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.

int      head  = 0, tail = 1-NUMPIXELS; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)

#define MAX_RAND  50
#define DELAY_MAX   150
#define DELAY_MIN   5
int ms_delay = ((DELAY_MAX - DELAY_MIN) / 2 ) + DELAY_MIN;
int delay_increment = (DELAY_MAX - DELAY_MIN) / NUMPIXELS;

//this is our array of pixels that is manupulated and then sent out
int pixels_arr[NUMPIXELS];
#define UPSCALER  4 //numver of virtual pixels per real pixel
int virtual_arr[NUMPIXELS * UPSCALER];

void sendPixelArray(){
  static int curr_pixels[NUMPIXELS];
  //loop over all pixels in array
  for (int i = 0; i < NUMPIXELS; i++){
    //if pixel changed, then update it and update our static array
    if(curr_pixels[i] != pixels_arr[i]){
      curr_pixels[i] = pixels_arr[i];
      strip.setPixelColor(i, pixels_arr[i]);
    }
  }
  strip.show();                     // Refresh strip
}

//returns and integer color
int averagePixels(int* arr_in , int num_pixels){
  int ave_R= 0;
  int ave_B =0;
  int ave_G = 0;
  for(int j = 0; j < num_pixels; j++){
    ave_R += ( arr_in[ j ] ) & 0xFF0000 ;
    ave_G += ( arr_in[ j ] ) & 0x00FF00 ;
    ave_B += ( arr_in[ j ] ) & 0x0000FF ;
  }
  ave_R /= num_pixels;
  ave_G /= num_pixels;
  ave_B /= num_pixels;
  return ave_R + ave_G + ave_B;
}

void AveDownSampleArrays( int * virt_arr, int * real_arr, int pixel_scaling, int real_len){
   int j;
   int ave;
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
void InitiatePulse( int* arr, int num_pixels){
  int color = RANDOM_COLOR;
  for(int i = 0; i < num_pixels; i++){
    arr[i] = color;
  }
}

#define COUNTER_MAX 6
//This just moves the pixels down the array 
void StripPropagateBasic(){
  static int counter = 0;
    //move the entire array up one
  for(int i = NUMPIXELS-1; i >0; i--){
    pixels_arr[i] = pixels_arr[i-1];
  }
  pixels_arr[0] = 0;
  if(counter++ >=COUNTER_MAX ) {
    InitiatePulse(pixels_arr, 2);
    counter = random(COUNTER_MAX);
  }
//  ms_delay = UpdateDelay(ms_delay);
  sendPixelArray();
  delay(ms_delay); //pause between loops
}


//initial pattern where one color fills the fan blades fully, then another color chases it out
void ColorChaserBasic(){
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
  
  if(++head >= NUMPIXELS) {         // Increment head index.  Off end of strip?
    head = 0;                       //  Yes, reset head index to start
//    if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
      color = RANDOM_COLOR ;            //   Yes, reset to red
      ms_delay = DELAY_MIN;
  }
  if(++tail >= NUMPIXELS) tail = 0; // Increment, reset tail index
  
}

void loop() {
//  ColorChaserBasic();
  StripPropagateBasic();
  
}
