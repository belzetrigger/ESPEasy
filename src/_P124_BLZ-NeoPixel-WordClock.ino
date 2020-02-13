//#ifdef USES_P124
//#include section
//include libraries here. For example:
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#include <ArduinoJson.h>

//#######################################################################################################
//#################################### Plugin 124: NeoPixel word clock #######################################
//#######################################################################################################
/*
Commands:
*brightness 
/control?cmd=BLZWC_SETLDR,90  [B*] --> set ldr  and controls so brightness ... [NB*]
*alarm
BLZWC_ALARMBREATH   [NB]--> turn toggle  breath mode [NB]
BLZWC_ALARMBREATH,(1|0)  [NB]--> turn on|off breathing alarm
BLZWC_ALARM  [B*]--> turn alarm in toggle mode [NB]
BLZWC_ALARM,(1|0) [B*] --> turn on|off alarm


/control?cmd=BLZWC_CLOCK [NB]   --> just print status as Json

*for testing
T
/control?cmd=BLZWC_STATUS   --> just print status as Json

/control?cmd=BLZWC_SETTIME,(HH),(MM))  [B*]--> fakes clock in, to play with diff. times
/control?cmd=BLZWC_TEST_ALL(,r,g,b) [B*]--> turns on all Leds from 0 to max, optional color
/control?BLZWC_TEST(,pos,r,g,b)  [B*]--> turns on single led optional position, red, green, blue
/control?BLZWC_TEST,6,1,10,10

*for fun
/control?cmd=BLZWC_KITT,(1,0) [NB*]--> KnightRiders KITT or a snake walks through
/control?cmd=BLZWC_TEXT,foo [NB] -> prints text on matrix[NB]


B  - blocking
B* - Blocking but fast enough....
NB - None Blocking
NB* - partly blocking
***********************************************/
//uncomment one of the following as needed
//#ifdef PLUGIN_BUILD_DEVELOPMENT
//#ifdef PLUGIN_BUILD_TESTING


// *************** LED **********************
#define P124_LED_TYPE NEO_GRB + NEO_KHZ800


#define P124_NELEMS(x) (sizeof(x) / sizeof((x)[0]))

#define P124_MAX_LED 115
#define P124_COLUMNS 11
#define P124_ROWS 10
#define P124_MAX_WC_WORDS 24
#define P124_MAX_WC_WORD_LENGTH 40
// ************* Text stuff
#define P124_PIXEL_PER_CHAR 6

// ************* Breath stuff
//max led intensity (1-255)
#define P124_BREATH_MAX_BRIGHT 130
//Inhalation time in milliseconds.
#define P124_BREATH_INHALE 2000
#define P124_BREATH_PULSE P124_BREATH_INHALE / 1000 * 50 / P124_BREATH_MAX_BRIGHT
//Rest Between Inhalations.
#define P124_BREATH_REST 3000

// ******** Setting for Brightness ****

/**
  *  brightness value use in really dark ambiance
  */
static int P124_BRGHTNS_Mode_ExtraLow = 30;
/**
  * brightness value use in dark ambiance
 */
static int P124_BRGHTNS_Mode_Low = 60;
/**
  * brightness value use normal ambiance
 */
static int P124_BRGHTNS_Mode_Normal = 100;
/**
  * brightness value use in light ambiance
 */
static int P124_BRGHTNS_Mode_High = 150;
/**
  * brightness value use in really light ambiance. 
 */
static int P124_BRGHTNS_Mode_ExtraHigh = 180;

// ************* Status Color
// https://experience.sap.com/fiori-design-web/colors/#semantic-colors
unsigned long P124_STATUS_INFO_COLOR = (0x0F40ac); //#427cac
unsigned long P124_STATUS_NEUTRAL_COLOR = (0x112B32); //0x5E696E
unsigned long P124_STATUS_POSITIVE_COLOR = (0x107D08); //2B7D2B
unsigned long P124_STATUS_CRITICAL_COLOR = (0xE78C07);
unsigned long P124_STATUS_NEGATIVE_COLOR = (0xBB0000);

/**
 * @brief  constants for different run modes of this plugin
 * @note   keep in mind c++ -> its an int, but for JSON convert it directly to int
 * @retval None
 */
enum P124_RUN_TYPE
{
  /**
   * @brief  is turned off  
  */
  P124TypeOff,
  /**
   * @brief  clock including alarm and breath should work
   */
  P124TypeClock,
  /**
   * @brief  simple test function loop over *all* leds, all means from 0 to MAX_LED
   */
  P124TypeTestLoop,
  /**
    * @brief  turn on all LEDS from 0 to max
    
   */
  P124TypeTestAll,
  /**
   * @brief  litte red ligth like Kit from Knight Rider
   */
  P124TypeKnightRider,
  /**
   * @brief  breath in and out
   */
  P124TypeBreath,
  /**
   * @brief  show a scrolling text
   */
  P124TypeText,
  /**
   * @brief  simple mode, where we pass by time from extern
   */
  P124TypeTestClock
};

// max value allowed for color, for adafruit r:255 would the brightest, to protect wire / usb port set lower value
#define COLOR_MAX 185
// min value allowed for color, under 5 its not visible
#define COLOR_MIN 5

// min threshold for brightness
#define BRIGHTNESS_MIN 5
// max threshold for brightness
#define BRIGHTNESS_MAX 255

// bool bit mask to store mutliple bools in one int
const uint isLdrAutoMask = 1 << 0;    //0
const uint withMinutesMask = 1 << 1;  //2
const uint withAlarmMask = 1 << 2;    //4
const uint withStatusMask = 1 << 3;   // 8
const uint isSnakeStyleMask = 1 << 4; // 16

/**
 * @brief  ldrauto, withMinute,Alarm,Status, Snake
 * @note   
 * @retval 
 */
#define P124_BOOLEANS PCONFIG(0)
 
// **** color
 
//#define P124_COLOR_RED PCONFIG(0)
#define P124_COLOR_RED_GREEN PCONFIG(1)
//#define P124_COLOR_GREEN PCONFIG(1)
#define P124_COLOR_BLUE PCONFIG(2)

// default color values for init
#define P124_COLOR_RED_DFLT 0
#define P124_COLOR_GREEN_DFLT 180
#define P124_COLOR_BLUE_DFLT 0

// **** ldr
//#define P124_LDR_AUTO PCONFIG(10)
#define P124_LDR_VALUE PCONFIG(3)
#define P124_LDR_XTR_LOW_AND_LOW PCONFIG(4)
#define P124_LDR_NRML_AND_HIGH PCONFIG(5)
#define P124_LDR_XTR_HIGH PCONFIG(6)

// **** minutes

#define P124_POS_MIN_1_AND_2 PCONFIG(7)
//#define P124_POS_MIN_2 PCONFIG(6)

#define P124_POS_MIN_3_AND_4 PCONFIG(8)
//#define P124_POS_MIN_4 PCONFIG(8)

// default values for minutes used on init
#define P124_WITH_MIN_DFLT 1
#define P124_POS_MIN_1_DFLT 112
#define P124_POS_MIN_2_DFLT 111
#define P124_POS_MIN_3_DFLT 110
#define P124_POS_MIN_4_DFLT 114

// **** alarm
#define P124_POS_ALARM_STATUS PCONFIG(9)
// default values for alarm used on init
#define P124_WITH_ALARM_DFLT 1
#define P124_POS_ALARM_DFLT 113

// **** status
// default values for status used on init
#define P124_POS_STATUS_DFLT 113
#define P124_WITH_STATUS_DLFT 1

// **** snake style
// default values for snake style  used on init
#define P124_SNAKE_STYLE_DFLT 1
// all labels used for config page, do not change order!
const String P124_WC_LABLES[]{"min: after", "min: before",
                              //min
                              "min: five", "min: ten", "min: quarter", "min: twenty", "min: half", "min: threequarter",
                              //hour
                              "one", "ones", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
                              //clock
                              "it", "is", "oclock"};
//default values for words  used on init
#define P124_SPOS_AFTER_DFLT "41,40,39,38"
#define P124_SPOS_BEFORE_DFLT "37,36,35"
// minutes
//"min: five", "min: ten", "min: quarter", "min: twenty", "min: half", "min: threequarter",
#define P124_SPOS_MIN_FIVE_DFLT "7,8,9,10"
#define P124_SPOS_MIN_TEN_DFLT "21,20,19,18"
#define P124_SPOS_MIN_QUARTER_DFLT "26,27,28,29,30,31,32"
#define P124_SPOS_MIN_TWENTY_DFLT "17,16,15,14,13,12"
#define P124_SPOS_MIN_HALF_DFLT "44,45,46,47"
#define P124_SPOS_MIN_THREEQUARTER_DFLT "22,23,24,25,26,27,28,29,30,31,32"
// hours
//  "one", "ones", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
#define P124_SPOS_H_ONE_DFLT "63,62,61,60"
#define P124_SPOS_H_ONES_DFLT "63,62,61,60"
#define P124_SPOS_H_TWO_DFLT "65,64,63,62"
#define P124_SPOS_H_THREE_DFLT "67,68,69,70"
#define P124_SPOS_H_FOUR_DFLT "77,78,79,80"
#define P124_SPOS_H_FIVE_DFLT "73,74,75,76"
#define P124_SPOS_H_SIX_DFLT "108,107,106,105,104"
#define P124_SPOS_H_SEVEN_DFLT "60,59,58,57,56,55"
#define P124_SPOS_H_EIGHT_DFLT "89,90,91,92"
#define P124_SPOS_H_NINE_DFLT "84,83,82,81"
#define P124_SPOS_H_TEN_DFLT "93,94,95,96"
#define P124_SPOS_H_ELEVEN_DFLT "87,86,85"
#define P124_SPOS_H_TWELVE_DFLT "49,50,51,52,53"

// words hour
#define P124_SPOS_IT_DFLT "0,1"
#define P124_SPOS_IS_DFLT "3,4,5"
#define P124_SPOS_CLOCK_DFLT "101,100,99"

//*** index for words, a map might be better ...
// idx
#define P124_SPOS_AFTER_IDX 0
#define P124_SPOS_BEFORE_IDX 1
// minutes
//"min: five", "min: ten", "min: quarter", "min: twenty", "min: half", "min: threequarter",
#define P124_SPOS_MIN_FIVE_IDX 2
#define P124_SPOS_MIN_TEN_IDX 3
#define P124_SPOS_MIN_QUARTER_IDX 4
#define P124_SPOS_MIN_TWENTY_IDX 5
#define P124_SPOS_MIN_HALF_IDX 6
#define P124_SPOS_MIN_THREEQUARTER_IDX 7
// hours
//  "one", "ones", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
#define P124_SPOS_H_ONE_IDX 8
#define P124_SPOS_H_ONES_IDX 9
#define P124_SPOS_H_TWO_IDX 10
#define P124_SPOS_H_THREE_IDX 11
#define P124_SPOS_H_FOUR_IDX 12
#define P124_SPOS_H_FIVE_IDX 13
#define P124_SPOS_H_SIX_IDX 14
#define P124_SPOS_H_SEVEN_IDX 15
#define P124_SPOS_H_EIGHT_IDX 16
#define P124_SPOS_H_NINE_IDX 17
#define P124_SPOS_H_TEN_IDX 18
#define P124_SPOS_H_ELEVEN_IDX 19
#define P124_SPOS_H_TWELVE_IDX 20

// words hour
#define P124_SPOS_IT_IDX 21
#define P124_SPOS_IS_IDX 22
#define P124_SPOS_CLOCK_IDX 23

// array to map default values to correct position of config fields
const String P124_WC_WORDS_POS_DFLT[]{
    // words
    P124_SPOS_AFTER_DFLT, P124_SPOS_BEFORE_DFLT,
    //minutes
    P124_SPOS_MIN_FIVE_DFLT,
    P124_SPOS_MIN_TEN_DFLT,
    P124_SPOS_MIN_QUARTER_DFLT,
    P124_SPOS_MIN_TWENTY_DFLT,
    P124_SPOS_MIN_HALF_DFLT,
    P124_SPOS_MIN_THREEQUARTER_DFLT,

    //hours
    P124_SPOS_H_ONE_DFLT,
    P124_SPOS_H_ONES_DFLT,
    P124_SPOS_H_TWO_DFLT,
    P124_SPOS_H_THREE_DFLT,
    P124_SPOS_H_FOUR_DFLT,
    P124_SPOS_H_FIVE_DFLT,
    P124_SPOS_H_SIX_DFLT,
    P124_SPOS_H_SEVEN_DFLT,
    P124_SPOS_H_EIGHT_DFLT,
    P124_SPOS_H_NINE_DFLT,
    P124_SPOS_H_TEN_DFLT,
    P124_SPOS_H_ELEVEN_DFLT,
    P124_SPOS_H_TWELVE_DFLT,
    //    "one", "ones", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
    // words hour
    P124_SPOS_IT_DFLT, P124_SPOS_IS_DFLT, P124_SPOS_CLOCK_DFLT};

// the base value of arguments used to store config
#define P124_ARG_NAME "p124_wc"

// ****************************************************
// *    ESP Easy Standard definitons
// ****************************************************
#define PLUGIN_124
#define PLUGIN_ID_124 124
#define PLUGIN_NAME_124 "Output - Word Clock(BLZ)"

#define PLUGIN_VALUENAME1_124 "usedRGB"
#define PLUGIN_VALUENAME2_124 "currentBrightnessmode"
#define PLUGIN_VALUENAME3_124 "currentRunMode"
#define PLUGIN_VALUENAME4_124 "lastLDR"

// wc own struct to store plugin data and function in one place
struct P124_data_struct : public PluginTaskData_base
{

  P124_data_struct() {}
  ~P124_data_struct() { reset(); }

  void reset()
  {
    String log = F(PLUGIN_NAME_124);
    log += F("#reset data");
    addLog(LOG_LEVEL_INFO, log);

    // reset pixels
    if (p124_pixels != nullptr)
    {
      p124_pixels -> clear();
      delete p124_pixels;
      p124_pixels = nullptr;
    }
    // reset matrix
    if (p124_matrix != nullptr)
    {
      p124_matrix -> clear();
      delete p124_matrix;
      p124_matrix = nullptr;
    }
    // TODO should we put all value back to default/null?
  }
  /**
  * @brief  init matrix and neo pixel 
  * @param  *event: 
  * @retval None
  */
  void init(struct EventStruct *event)
  {
    String log = F(PLUGIN_NAME_124);
    log += F("init data");
    addLog(LOG_LEVEL_INFO, log);
    reset(); //TODO really needed to reset?
    if (!p124_pixels)
    {
      p124_pixels = new Adafruit_NeoPixel(P124_MAX_LED, CONFIG_PIN1, P124_LED_TYPE);
      p124_pixels->begin(); // This initializes the NeoPixel library.
    }
    if (!p124_matrix)
    {
      p124_matrix = new Adafruit_NeoMatrix(P124_COLUMNS, P124_ROWS, CONFIG_PIN1,
                                           NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
                                               NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                                           P124_LED_TYPE);
      p124_matrix->begin(); // This initializes the NeoPixel library.
      p124_matrix->setTextWrap(false);

      curRunType = P124TypeClock;
      // if we are on an init AFTER config changes, try to rebuild time
    }
    set(event);
    sanitizeColor();
  }

  void set(struct EventStruct *event)
  {
    p124_intToBools(P124_BOOLEANS, isLdrAutomatic, isMinuteEnabled,
               isAlarmEnabled, isStatusEnabled,
               isSnakeStyle);
    uint8 h, l;
    p124_intToByte(P124_COLOR_RED_GREEN, h, l);
    red = h;
    green = l;
    p124_intToByte(P124_COLOR_RED_GREEN, h, l);
    blue = h;

    
    p124_intToByte(P124_POS_MIN_1_AND_2, h, l);
    pos_min_1 = h;
    pos_min_2 = l;

    p124_intToByte(P124_POS_MIN_3_AND_4, h, l);
    pos_min_3 = h;
    pos_min_4 = l;

    p124_intToByte(P124_POS_ALARM_STATUS, h, l);
    pos_alarm = h;

    pos_status = l;

    // TODO  ....
    String log = F(PLUGIN_NAME_124);

    addLog(LOG_LEVEL_INFO, log);
  }

  boolean isInitialized() const
  {
    return p124_pixels != nullptr && p124_matrix != nullptr;
  }
  /**
   * @brief  reads configured lines from CustomTaskSettings
   * @note   
   * @param  taskIndex: 
   * @retval None
   */
  void loadDisplayLines(byte taskIndex)
  {
    String log = F(PLUGIN_NAME_124);
    log += F(" loadDisplayLines ");
    addLog(LOG_LEVEL_INFO, log);
    char deviceTemplate[P124_MAX_WC_WORDS][P124_MAX_WC_WORD_LENGTH];
    LoadCustomTaskSettings(taskIndex, (byte *)&deviceTemplate, sizeof(deviceTemplate));
    
    for (byte varNr = 0; varNr < P124_MAX_WC_WORDS; varNr++)
    {
      displayLines[varNr] = deviceTemplate[varNr];
      //int pos[P124_COLUMNS] = {-1};
      transform(deviceTemplate[varNr], wordPositions[varNr]);
      //wordPositions[varNr] = pos;
#ifdef P124_DEBUG_CONF_TO_POSITION
      log = F("transfomed result: ");
      for (int i = 0; i < 5; i++)
      {
        log += F(" i: ");
        log += String(i);
        log += F("=");
        log += wordPositions[varNr][i];
        log += F("; ");
      }
#endif
      addLog(LOG_LEVEL_DEBUG, log);
    }
  }

  // ****************************************************
  // *     (scroll) TEXT
  // ****************************************************

  /**
* ressts postions, rotation, size, ...
* ? should we init text
* *dsdsd
* TODO: kkk
* 
*/
  void initText(const char *msg)
  {
    curRunType = P124TypeText;
    next_update = millis() + 120;
    resetAllPixel(true); // to set also minutes to black
    p124_matrix->clear();
    p124_matrix->setTextWrap(false); // we don't wrap text so it scrolls nicely
    p124_matrix->setTextSize(1);     // standard font size -> 8px hight
    p124_matrix->setRotation(0);
    uint b = sanitizeBrightness(curBrightnessMode + 10);
    p124_matrix->setBrightness(b);
    textScrollX = p124_matrix->width(); // put position to the right
    textScrollPart = 0;
    textScrollingMax = 0;
    textColor = p124_matrix->Color(250, 0, 0);

    int msgLength = strlen(msg);
    int msgSize = (msgLength * P124_PIXEL_PER_CHAR) + (2 * P124_PIXEL_PER_CHAR); // CACULATE message length;
    textScrollingMax = (msgSize) + p124_matrix->width();                         // ADJUST Displacement for message length;

    String log = F(PLUGIN_NAME_124);
    log += F("#initText:");
    log += msg;
    log += F(" msgLength:");
    log += msgLength;
    log += F(" pixelSize:");
    log += msgSize;
    log += F(" scrollingMax:");
    log += textScrollingMax;
    log += F(" textScrollX:");
    log += textScrollX;

    log += F(" brightness:");
    log += b;

    addLog(LOG_LEVEL_DEBUG, log);
    text = msg;
  }

  /**
 * shows given text as scrolling pixels on the matrix
 *  
 * @param  *string: msg to show
 * @retval None
 */
  void scrollTextNonBlocking()
  {
    next_update = millis() + 100;

    //* Change Color with Each Pass of Complete Message
    // PartyMode
    // uint16_t c = vp123_matrix->Color(127, 60*v_p123_text_scrollX, 0);
    uint16_t c = textColor; //p124_colors[textScrollPart];
    // dim color, matrix is using gfx as a base, this is only 16bit color so we can not use our helper functions
    p124_matrix->setBrightness(curBrightnessMode);
    p124_matrix->setTextColor(c);

    // TODO minutes and alarm are missing!!!
    // did not work well - try first this, after that init back to matrix
    //fp123_pixelResetAndBlack();
    //Plugin_123_pixels->show();

    // back to normal matrix
    p124_matrix->fillScreen(0); // BLANK the Entire Screen;

    p124_matrix->setCursor(textScrollX, 2); // Set Starting Point for Text String;
    p124_matrix->print(text);               // Set the Message String;

    // ** SCROLL TEXT FROM RIGHT TO LEFT BY MOVING THE CURSOR POSITION
    if (--textScrollX < -textScrollingMax)
    {
      String log = F(PLUGIN_NAME_124);
      log += F("#scrollTextNonBlocking:time for next run. ");
      log += F("color: ");
      log += String(c);
      addLog(LOG_LEVEL_DEBUG, log);
      // *  ADJUST FOR MESSAGE LENGTH
      // Decrement x by One AND Compare New Value of x to -scrollingMax;
      // This Animates (moves) the text by one pixel to the Left;
      next_update = millis() + 300;
      textScrollX = p124_matrix->width(); // After Scrolling by scrollingMax pixels, RESET Cursor Position and Start String at New Position on the Far Right;
      textScrollPart++;                   // INCREMENT COLOR/REPEAT LOOP COUNTER AFTER MESSAGE COMPLETED;
    }
    p124_matrix->show(); //show must be called sep.
    if (textScrollPart >= 2)
    {
      String log = F(PLUGIN_NAME_124);
      log += F("#scrollTextNonBlocking:time to leave");
      addLog(LOG_LEVEL_DEBUG, log);
      initText("");
      curRunType = P124TypeClock;
      resetAllPixel(true);
      time_needs_update = true;
      next_update = millis() + 100;
    }
  }

  // ****************************************************
  // *     KNIGHT RIDER / K.I.T.T.
  // ****************************************************

  /**
  * @brief  switch on or off K.I.T.T led
  * @note   
  * @param  on: 1=on, everthing else =off
  * @retval None
  */
  void toggleKnightRider(int on)
  {
    String log = F(PLUGIN_NAME_124);
    log += F("#toggleKnightRider:on=");
    log += String(on);
    addLog(LOG_LEVEL_DEBUG, log);
    if (on == 1)
    {
      //turn off
      initKnightRider(1);
      //setPixelColor(pos_alarm, ALARM_COLOR, true);
      isKROn = true;
    }
    else
    {
      //setPixelColor(pos_alarm, 0, true);
      //initKnightRider(1); // set back to init values
      // change back to clock;
      //resetAllPixel(false, true);
      isKROn = false;
      curRunType = P124TypeClock;
      time_needs_update=true;
      next_update = millis() + 20;
     
    }
  }

  /**
  * @brief  toggles full matrix to K.I.T.T.
  * @note   
  * @retval None
  */
  void toggleKnightRider()
  {

    if (isKROn == true)
    {
      //turn off
      toggleKnightRider(0);
    }
    else
    {
      toggleKnightRider(1);
    }
  }

  /**
  * @brief  does the work need to init kr
  * @note   
  * @retval None
  
 */
  void initKnightRider(uint cycles)
  {
    String log = F(PLUGIN_NAME_124);
    log += F("#initKnightRider:speed=");
    //log += String(cycles);
    //addLog(LOG_LEVEL_DEBUG, log);
    curRunType = P124TypeKnightRider;
    curKrWalkToRight = true;
    krCycles = cycles;
    curKrCycles = 0;
    uint krStartOffset = 0; //fp123_matrixGetStartOffset(event);
    uint krEndOffset = 5;   //fp123_matrixGetEndOffset(event);
    curKrPos = krStartOffset; // TODO use offsets
    curKrWalkToRight = true;    
    //log = F(PLUGIN_NAME_124);
    //log += F("#knightRider:speed");
    log += String(krSpeed);
    log += F(":width:");
    log += String(krWidth);
    log += F(":cur cycles:");
    log += String(curKrCycles);
    log += F(":pos:");
    log += String(curKrPos);
    log += F(":startOffset:");
    log += String(krStartOffset);
    log += F(":endOffset:");
    log += String(krEndOffset);
    log += F(":color:");
    log += String(KR_COLOR);
    log += F(":right:");
    log += String(curKrWalkToRight);

    addLog(LOG_LEVEL_INFO, log);
    //TODO resetAllPixel(false);
    next_update = millis() + 100;
  }

   /**
  * @brief  
  * @note   
  * * color: must be set before to KR_COLOR 
  * * cycles: must be set before to curKrCycles
  * @retval None
  
 */
  void knightRiderNonBlocking()
  {
    String log = F(PLUGIN_NAME_124);
    log += F("#knightRiderNonBlocking:");
    next_update = millis() + 100;
    uint start = krStartOffset;
    uint end = (P124_MAX_LED - krEndOffset);
    log += F(" start: ");
    log += String(start);
    log += F(" end: ");
    log += String(end);


    if (curKrPos >= start && curKrPos <= end)
    {
      // set color to head; idx = count
      resetAllPixel(false);
      // setPixelColor(curKrPos, KR_COLOR, false);
      uint32_t dimedColor = KR_COLOR;
      for (int x = curKrPos; x > start && x < end; (curKrWalkToRight?x--:x++) )
      {
        setPixelColor(x, dimedColor);
        //based on last color from pos before -> on start -> head
        dimedColor = dimColor(dimedColor, 2);
      }
      //higher or lower cur. position depending on direction
      (curKrWalkToRight?curKrPos++:curKrPos--);
      //log = String(curKrPos);
      //addLog(LOG_LEVEL_DEBUG, log);
    }
    else
    {

      if (curKrWalkToRight)
      {
        if (isMinuteEnabled == true)
        {
          //TODO Bang to minutes is not non blocking
          log += F("#2 bang to minutes: ");
          log += F(" pos: ");
          log += String(curKrPos);
          //addLog(LOG_LEVEL_DEBUG, log);

          setPixelColor(pos_min_1, KR_COLOR, false);
          setPixelColor(pos_min_2, KR_COLOR, false);
          setPixelColor(pos_min_3, KR_COLOR, false);
          setPixelColor(pos_min_4, KR_COLOR, false);
          p124_pixels->show();
          delay(krSpeed / 2);
          int dc = dimColor(KR_COLOR, krWidth);
          setPixelColor(pos_min_1, dc, false);
          setPixelColor(pos_min_2, dc, false);
          setPixelColor(pos_min_3, dc, false);
          setPixelColor(pos_min_4, dc, false);
          p124_pixels->show();
          delay(krSpeed / 2);
          setPixelColor(pos_min_1, dc, false);
          setPixelColor(pos_min_2, dc, false);
          setPixelColor(pos_min_3, dc, false);
          setPixelColor(pos_min_4, dc, false);
          //p124_pixels->show();
          //delay(krSpeed);
        
        }
        curKrPos--; // reduce 1 to fit to our rule
        // change direction
        curKrWalkToRight = !curKrWalkToRight;
        log += F("#3 change direction: ");
        log += F(" pos: ");
        log += String(curKrPos);
        log += F(" curKrWalkToRight: ");
        log += String(curKrWalkToRight);
        addLog(LOG_LEVEL_DEBUG, log);
      }
      else
      {
        log += F("#4 done with this cycle: ");
        log += String(curKrCycles);
        log += F(" pos: ");
        log += String(curKrPos);
        addLog(LOG_LEVEL_DEBUG, log);
        //done with cycle
        curKrCycles++;

        //test
        toggleKnightRider(0);
        resetAllPixel(false);
        time_needs_update = true;
      }

    } //if else (curKrPos <= (P124_MAX_LED - krEndOffset))

    
    p124_pixels->show();
    next_update = millis() + krSpeed;
    return;
  }

// ****************************************************
  // *     CLOCK
  // ****************************************************

 /**
  * @brief  turn LED of clock on/of
  * @note   
  * @param  on: 1=on, 0=off
  * @retval None
  */
 void toggleClock(int on)
  {
      next_update = millis() +20;
      String log = F(PLUGIN_NAME_124);
      log += F("#toggleClock:on=");
      log += String(on);
      addLog(LOG_LEVEL_DEBUG, log);
      if (on == 1)
      {
        curRunType = P124TypeClock;
        time_needs_update = true;
        // should we reset all other flags?
        
      }
      else
      {
        curRunType = P124TypeOff;
        resetAllPixel(true, false);
        
        
      }
    
    
  }

  /**
  * @brief  toggles the clock on/off
  * @note   
  * @retval None
  */
  void toggleClock()
  {

    if (curRunType == P124TypeOff)
    {
      //turn off
      toggleClock(1);
    }
    else
    {
      toggleClock(0);
    }
  }
  
  // ****************************************************
  // *     BREATH
  // ****************************************************

  /**
 * @brief  (re)sets stuff for breathing
 */
  void initBreathing()
  {
    curRunType = P124TypeBreath;
    isBreathOn = true;
    curBreathIn = true;
    curBreathBrightness = 5;
    setPixelColor(pos_alarm, 0, true);

    next_update = millis() + 500;
  }

  /**
 * @brief  using the alarm led to let it breath
 * @note   
 * @retval None
 */
  void breathNonBlocking()
  {

    if (curBreathIn == true)
    {
      //Serial.print("+");
      // we do breath in P123_PULSE
      curBreathBrightness++;
      //= v_p123_cur_bright + (v_p123_cur_bright* P123_PULSE);
      setPixelColor(
          pos_alarm, p124_pixels->Color(0, 0, (int)curBreathBrightness), true); // yellow with a little extra red to make it warmer                       //to prevent watchdog firing.
      delayMicroseconds(P124_BREATH_PULSE * 10);                                // wait
      delay(0);
      // Plugin_123_pixels->show();

      if (curBreathBrightness == P124_BREATH_MAX_BRIGHT)
      {
        // time  breath out
        curBreathIn = false;
      }
    }
    else // breath out
    {
      //Serial.print("-");
      // we do breath out
      curBreathBrightness--;
      curBreathBrightness--;
      setPixelColor(
          pos_alarm,
          p124_pixels->Color(0, 0, (int)curBreathBrightness, 0), true); // yellow with a little extra red to make it warmer                       //to prevent watchdog firing.
                                                                        //   Plugin_123_pixels->show();
      delayMicroseconds(P124_BREATH_PULSE * 10);                        // wait
      delay(0);
      if (curBreathBrightness <= 1)
      {
        // time  to rest
        next_update = millis() + P124_BREATH_REST;
        curBreathIn = true;
        //Serial.println("REST");
      }
    }
  }

  /**
  * @brief  switch on or off alarm led
  * @note   alarm must be eneabled 
  * @param  on: 1=on, everthing else =off
  * @retval None
  */
  void toggleAlarmBreath(int on)
  {
    if (isAlarmEnabled == true) // yes we use alarm led for breath
    {

      String log = F(PLUGIN_NAME_124);
      log += F("#toggleAlarmBreath:on=");
      log += String(on);
      addLog(LOG_LEVEL_DEBUG, log);
      if (on == 1)
      {
        //turn off
        initBreathing();
        //setPixelColor(pos_alarm, ALARM_COLOR, true);
        isBreathOn = true;
      }
      else
      {
        //setPixelColor(pos_alarm, 0, true);
        initBreathing(); // set back to init values
        // change back to clock;
        curRunType = P124TypeClock;
        isBreathOn = false;
      }
    }
    else
    {
      String log = F(PLUGIN_NAME_124);
      log += F("#toggleAlarm:called but alarm is not enabled:on=");
      log += String(on);
      addLog(LOG_LEVEL_ERROR, log);
    }
  }

  /**
  * @brief  toggles the alarm light in breath style
  * @note   
  * @retval None
  */
  void toggleAlarmBreath()
  {

    if (isBreathOn == true)
    {
      //turn off
      toggleAlarmBreath(0);
    }
    else
    {
      toggleAlarmBreath(1);
    }
  }
  // ****************************************************
  // *     LDR and color BRIGHTNESS
  // ****************************************************

  /**
 * @brief  takes a color and adjust it to brightness values
 * @note   
 * @param  c: uint for color r,g orb
 * @param  brigthness: 0 to 255, but in real 20 to 130
 * @retval 
 */
  uint32 sanitizeColor(uint c, uint brigthness)
  { // see https://forums.adafruit.com/viewtopic.php?t=41143
    if (c == 0)
      return c;
    return sanitizeColor(brigthness * c / 255);
  }
  /**
  * @brief  just checks the color is in defined range
  * @note   
  * @param  c: 
  * @retval 
  
 */
  uint32_t sanitizeColor(uint c)
  {
    if (c > COLOR_MAX)
    {
      return COLOR_MAX;
    }
    else if (c < COLOR_MIN)
    {
      return COLOR_MIN;
    }

    return c;
  }
  /**
 * @brief  just verifies that brightness in deinfed range
 * @note   
 * @param  b: value to check
 * @retval 
 */
  uint32_t sanitizeBrightness(uint b)
  {
    if (b > BRIGHTNESS_MAX)
    {
      return BRIGHTNESS_MAX;
    }
    else if (b < BRIGHTNESS_MIN)
    {
      return BRIGHTNESS_MIN;
    }

    return b;
  }

  /**
 * @brief  takes value of ldr and calculates matching brightness mode for LED/Clock
 * @note   
 * @param  ldr: value from LDR Sensor
 * @retval Mode to use now, it is an int value, used later for calculation of brightness
 */
  int getBrghtnsForLdr(int ldr)
  {
    int state;
    if (ldr < 100)
    {
      state = ldrExLowBrghtns;
    }
    else if (ldr < 350)
    {
      state = ldrLowBrghtns;
    }
    else if (ldr < 500)
    {
      state = ldrNormalBrghtns;
    }
    else if (ldr < 750)
    {
      state = ldrHighBrghtns;
    }
    else if (ldr >= 850 && ldr <= 1023)
    {
      state = ldrExHighLimit;
    }
    else
    {
      String log = F(PLUGIN_NAME_124);
      log += F("#getModeForLdr:invlaid Value, set to normal. ");
      //addLog(LOG_LEVEL_INFO, log);
      state = P124_BRGHTNS_Mode_Normal;
    }
    return state;
  }
  /**
  * @brief  calcs from config color the matching current color based on ldr
  * @note   
  * @retval None
 */
  void sanitizeColor()
  {
    cur_red = sanitizeColor(red, curBrightnessMode);
    cur_green = sanitizeColor(green, curBrightnessMode);
    cur_blue = sanitizeColor(blue, curBrightnessMode);
  }

  /**
  * @brief  
  * @note   
  * @param  ldr: ldr value vfrom sensor
  * @param isAuto: if true when value comes from sensor and not from user action
  * @retval bool true if changed
  */
  bool setLdr(int ldr, bool isAuto)
  {
    String log = F(PLUGIN_NAME_124);
    log += F("#setLdr:ldr=");
    log += String(ldr);
    log += F("; isAuto: ");
    log += String(isAuto);


    bool changed = false;
    if(isAuto){
      // only do deeper check it auto is on
      if(overSampleLdr > 0 && millis() < (overSampleLdr + 60*1000) ){
        // over sample still active
        return changed;
      }

      if(overSampleLdr > 0 && millis() >= (overSampleLdr + 60*1000)){
        log += F("; timeOver");
        addLog(LOG_LEVEL_DEBUG, log);
        overSampleLdr = 0;
      }
    }
    
   
    byte oldLdr = curLdr;
    uint oldBrMode = curBrightnessMode;
    
    curLdr = ldr;
    curBrightnessMode = getBrghtnsForLdr(curLdr);
    if (curBrightnessMode != oldBrMode)
    {
      changed = true;
      log += F(":adapt colors:new mode:");
      log += String(curBrightnessMode);

      // calculate new color right now, and save time on putting color
      sanitizeColor();
      time_needs_update = true;
    }
/*
    log += F(": curLdr: ");
    log += String(curLdr);
    log += F(": changed: ");
    log += String(changed);
    log += F(": updateOnChange: ");
    log += String(updateOnChange);
    addLog(LOG_LEVEL_DEBUG, log);
    */
    // TODOD macht gar keinen sinn, update kommt 50Hz.
    // wird überschrieben, müsste also noch nen override
    // rein
    if(!isAuto && changed)
    {
      // TODO set oversample timer
      overSampleLdr = millis();
       + 60*1000;
      //updateWordClock(-1);
    }
    return changed;
  }

  // ****************************************************
  // *     TEST stuff for the CLOCK
  // ****************************************************

  //******** test all ********
  /**
  * @brief  turn on all leds from 0 to max and shows them immediately
  * @note   
  * @retval None
  */
  void testAll()
  {
    testAll(0, 0, 20);
  }

  /**
  * @brief  turn on all leds from 0 to max and shows them immediately
  * @note   
  * @param  r: red
  * @param  g: green
  * @param  b: blue
  * @retval None
  */
  void testAll(int r, int g, int b)
  {
    curRunType = P124TypeTestAll;
    String log = F(PLUGIN_NAME_124);
    log += F("#testAll:r=");
    log += String(r);
    log += F(";g=");
    log += String(g);
    log += F(";b=");
    log += String(b);
    addLog(LOG_LEVEL_DEBUG, log);
    resetAllPixel(false);
    setPixelAllColor(p124_pixels->Color(r, g, b), true);
  }
  //******** test single ********
  /**
  * @brief  turn on all leds from 0 to max and shows them immediately
  * @note 
  * @param  pos: 0 based led pixel  
  * @retval None
  */
  void testSingle(int pos)
  {
    testSingle(pos, 0, 0, 20);
  }

  /**
  * @brief  turn on all leds from 0 to max and shows them immediately
  * @note   
  * @param  pos: 0 based led pixel
  * @param  r: red
  * @param  g: green
  * @param  b: blue
  * @retval None
  */
  void testSingle(int pos, int r, int g, int b)
  {
    curRunType = P124TypeTestAll;
    String log = F(PLUGIN_NAME_124);
    log += F("#testSingle:pos=");
    log += String(pos);
    log += F(";r=");
    log += String(r);
    log += F(";g=");
    log += String(g);
    log += F(";b=");
    log += String(b);
    addLog(LOG_LEVEL_DEBUG, log);
    setPixelColor(pos, p124_pixels->Color(r, g, b), true);
  }

  // ****************************************************
  // *     TIME - some how importent for a clock :)
  // ****************************************************

  /**
  * @brief  
  * @note   set the time and flag to update wc, uses hour() and minute()
  * @retval None  
 */
  void updateTime()
  {
    updateTime(hour(), minute());
  }

  /**
  * @brief  
  * @note   set the time and flag to update wc
  * @param  h:  hours 0...24
  * @param  m: minutes 0...60
  * @retval None  
 */
  void updateTime(int h, int m)
  {
    time_needs_update = true;
    if (h < 0 || h > 24)
      h = hour();
    if (m < 0 || m > 60)
      m = minute();

    hours = h;
    minutes = m;
    String log = F(PLUGIN_NAME_124);
    log += F("#time:");
    log += String(hours);
    log += F(":");
    log += String(minutes);
    /*
    int *array = wordPositions[P124_SPOS_IT_IDX];
    for(int i = 0; i  < 5; i++){
      log+=String(i);
      log+=F(": ");
      log+=(array[i]);
      log+=F("; ");
    } */

    addLog(LOG_LEVEL_INFO, log);
    updateWordClock(-1);
  }

  /**
 * @brief  does calculation which led to show and puts it on the stripe
 * @param  hours:  as it comes from time
 * @param  minutes: minutes as it comes from time
 * @retval None
 */
  void timeToStripe()
  {

    uint32_t color = p124_pixels->Color(cur_red, cur_green, cur_blue);

    setPixelColor(wordPositions[P124_SPOS_IT_IDX], color);
    setPixelColor(wordPositions[P124_SPOS_IS_IDX], color);

    int m = minutes / 5;
    int restMin = minutes % 5;
    int h = hours;
    bool fullHour = false;
    switch (m)
    {
    case 0: // volle Stunde
      //fp123_coord2led(C_UHR, P123_NELEMS(C_UHR));
      setPixelColor(wordPositions[P124_SPOS_CLOCK_IDX], color);
      fullHour = true;
      break;
    case 1: // 5 nach
      //fp123_coord2led(M_C_FIVE, P123_NELEMS(M_C_FIVE));
      setPixelColor(wordPositions[P124_SPOS_MIN_FIVE_IDX], color);
      //fp123_coord2led(M_C_AFTER, P123_NELEMS(M_C_AFTER));
      setPixelColor(wordPositions[P124_SPOS_AFTER_IDX], color);
      break;
    case 2: // 10 nach
      //fp123_coord2led(M_C_TEN, P123_NELEMS(M_C_TEN));
      setPixelColor(wordPositions[P124_SPOS_MIN_TEN_IDX], color);
      //fp123_coord2led(M_C_AFTER, P123_NELEMS(M_C_AFTER));
      setPixelColor(wordPositions[P124_SPOS_AFTER_IDX], color);
      break;
    case 3: // viertel nach
            // fp123_coord2led(M_C_QUARTER, P123_NELEMS(M_C_QUARTER));
      setPixelColor(wordPositions[P124_SPOS_MIN_QUARTER_IDX], color);
      // fp123_coord2led(&M_C_AFTER, P123_NELEMS(M_C_AFTER)); break;
      h++;
      //  --> hours +1 for berlin style
      break;
    case 4: // 20 nach
      /*
    fp123_coord2led(M_C_TWENTY, P123_NELEMS(M_C_TWENTY));
    fp123_coord2led(M_C_AFTER, P123_NELEMS(M_C_AFTER));
    */
      // berlin style 10 vor halb
      //fp123_coord2led(M_C_TEN, P123_NELEMS(M_C_TEN));
      setPixelColor(wordPositions[P124_SPOS_MIN_TEN_IDX], color);
      //fp123_coord2led(M_C_BEFORE, P123_NELEMS(M_C_BEFORE));
      setPixelColor(wordPositions[P124_SPOS_BEFORE_IDX], color);
      //fp123_coord2led(M_C_HALF, P123_NELEMS(M_C_HALF));
      setPixelColor(wordPositions[P124_SPOS_MIN_HALF_IDX], color);
      h++;
      break;
    case 5: // 5 vor halb
      //fp123_coord2led(M_C_FIVE, P123_NELEMS(M_C_FIVE));
      setPixelColor(wordPositions[P124_SPOS_MIN_FIVE_IDX], color);
      //fp123_coord2led(M_C_BEFORE, P123_NELEMS(M_C_BEFORE));
      setPixelColor(wordPositions[P124_SPOS_BEFORE_IDX], color);
      //fp123_coord2led(M_C_HALF, P123_NELEMS(M_C_HALF));
      setPixelColor(wordPositions[P124_SPOS_MIN_HALF_IDX], color);
      h++;
      break;

    case 6: // halb
      //fp123_coord2led(M_C_HALF, P123_NELEMS(M_C_HALF));
      setPixelColor(wordPositions[P124_SPOS_MIN_HALF_IDX], color);
      h++;
      break;
    case 7: // 5 nach halb
      //fp123_coord2led(M_C_FIVE, P123_NELEMS(M_C_FIVE));
      setPixelColor(wordPositions[P124_SPOS_MIN_FIVE_IDX], color);
      //fp123_coord2led(M_C_AFTER, P123_NELEMS(M_C_AFTER));
      setPixelColor(wordPositions[P124_SPOS_AFTER_IDX], color);
      //fp123_coord2led(M_C_HALF, P123_NELEMS(M_C_HALF));
      setPixelColor(wordPositions[P124_SPOS_MIN_HALF_IDX], color);
      h++;
      break;
    case 8: // 20 vor
      //fp123_coord2led(&M_C_TWENTY, P123_NELEMS(M_C_TWENTY));
      //fp123_coord2led(&M_C_BEFORE, P123_NELEMS(M_C_BEFORE));
      //  es ist zehn nach halb zwei
      //fp123_coord2led(M_C_TEN, P123_NELEMS(M_C_TEN));
      setPixelColor(wordPositions[P124_SPOS_MIN_TEN_IDX], color);
      //fp123_coord2led(M_C_AFTER, P123_NELEMS(M_C_AFTER));
      setPixelColor(wordPositions[P124_SPOS_AFTER_IDX], color);
      //fp123_coord2led(M_C_HALF, P123_NELEMS(M_C_HALF));
      setPixelColor(wordPositions[P124_SPOS_MIN_HALF_IDX], color);
      h++;
      break;
    case 9: // viertel vor
      //uhrzeit |= ((uint32_t)1 << VIERTEL);
      //uhrzeit |= ((uint32_t)1 << VOR);
      //set_stunde(_stunde + 1, 0);
      //fp123_coord2led(M_C_THREE_QUARTER, P123_NELEMS(M_C_THREE_QUARTER));
      setPixelColor(wordPositions[P124_SPOS_MIN_THREEQUARTER_IDX], color);
      h++;
      break;
    case 10: // 10 vor
      //fp123_coord2led(M_C_TEN, P123_NELEMS(M_C_TEN));
      setPixelColor(wordPositions[P124_SPOS_MIN_TEN_IDX], color);
      //fp123_coord2led(M_C_BEFORE, P123_NELEMS(M_C_BEFORE));
      setPixelColor(wordPositions[P124_SPOS_BEFORE_IDX], color);
      h++;
      break;
    case 11: // 5 vor
      //fp123_coord2led(M_C_FIVE, P123_NELEMS(M_C_FIVE));
      setPixelColor(wordPositions[P124_SPOS_MIN_FIVE_IDX], color);
      //fp123_coord2led(M_C_BEFORE, P123_NELEMS(M_C_BEFORE));
      setPixelColor(wordPositions[P124_SPOS_BEFORE_IDX], color);
      h++;
      break;

    } //case
    //v_p123_cur_text += F(" ");
    hoursToStripe(h, fullHour, color);

    String log = F(PLUGIN_NAME_124);
    log += F("#minutes:");

    // restMin
    if (isMinuteEnabled == true)
    {
      // v_p123_cur_text += F(" ");
      if (restMin >= 1)
      {
        setPixelColor(pos_min_1, color);
        // v_p123_cur_text += F("x");
        log += F("x");
      }
      if (restMin >= 2)
      {
        setPixelColor(pos_min_2, color);
        //  v_p123_cur_text += F("x");
        log += F("x");
      }
      if (restMin >= 3)
      {
        //fp123_pixelTurnOn(P123_POS_MIN_3);
        setPixelColor(pos_min_3, color);
        // v_p123_cur_text += F("x");
        log += F("x");
      }
      if (restMin >= 4)
      {
        //fp123_pixelTurnOn(P123_POS_MIN_4);
        setPixelColor(pos_min_4, color);
        //  v_p123_cur_text += F("x");
        log += F("x");
      }
    }
    addLog(LOG_LEVEL_INFO, log);
    time_needs_update = false;
  }

  /**
 * @brief  set the leds for hours, is called fro clockSetTime
 * @param  hour: int value for hour to show, keep in mind to add+1 in case of 2:45 -> quarter to 3
 * @param  voll: if we have full hour
 * @retval None
 */
  void hoursToStripe(const unsigned int hour, const bool fullHour, uint32_t color)
  {

    switch (hour)
    {
    case 0:
      setPixelColor(wordPositions[P124_SPOS_H_TWELVE_IDX], color);
      break;
    case 1:

      if (fullHour == true)
      {
        setPixelColor(wordPositions[P124_SPOS_H_ONE_IDX], color);
      }
      else
      {
        setPixelColor(wordPositions[P124_SPOS_H_ONES_IDX], color);
      }
      break;

    case 2:
      setPixelColor(wordPositions[P124_SPOS_H_TWO_IDX], color);
      break;
    case 3:
      setPixelColor(wordPositions[P124_SPOS_H_THREE_IDX], color);
      break;
    case 4:
      setPixelColor(wordPositions[P124_SPOS_H_FOUR_IDX], color);

      break;
    case 5:
      setPixelColor(wordPositions[P124_SPOS_H_FIVE_IDX], color);
      break;
    case 6:
      setPixelColor(wordPositions[P124_SPOS_H_SIX_IDX], color);
      break;
    case 7:
      setPixelColor(wordPositions[P124_SPOS_H_SEVEN_IDX], color);
      break;
    case 8:
      setPixelColor(wordPositions[P124_SPOS_H_EIGHT_IDX], color);
      break;
    case 9:
      setPixelColor(wordPositions[P124_SPOS_H_NINE_IDX], color);
      break;
    case 10:
      setPixelColor(wordPositions[P124_SPOS_H_TEN_IDX], color);
      break;
    case 11:
      setPixelColor(wordPositions[P124_SPOS_H_ELEVEN_IDX], color);
      break;
    case 12:
      setPixelColor(wordPositions[P124_SPOS_H_TWELVE_IDX], color);
      break;
    case 13:

      if (fullHour == true)
      {
        setPixelColor(wordPositions[P124_SPOS_H_ONE_IDX], color);
      }
      else
      {
        setPixelColor(wordPositions[P124_SPOS_H_ONES_IDX], color);
      }
      break;
    case 14:
      setPixelColor(wordPositions[P124_SPOS_H_TWO_IDX], color);
      break;
    case 15:
      setPixelColor(wordPositions[P124_SPOS_H_THREE_IDX], color);
      break;
    case 16:
      setPixelColor(wordPositions[P124_SPOS_H_FOUR_IDX], color);

      break;
    case 17:
      setPixelColor(wordPositions[P124_SPOS_H_FIVE_IDX], color);
      break;
    case 18:
      setPixelColor(wordPositions[P124_SPOS_H_SIX_IDX], color);
      break;
    case 19:
      setPixelColor(wordPositions[P124_SPOS_H_SEVEN_IDX], color);
      break;
    case 20:
      setPixelColor(wordPositions[P124_SPOS_H_EIGHT_IDX], color);
      break;
    case 21:
      setPixelColor(wordPositions[P124_SPOS_H_NINE_IDX], color);
      break;
    case 22:
      setPixelColor(wordPositions[P124_SPOS_H_TEN_IDX], color);
      break;
    case 23:
      setPixelColor(wordPositions[P124_SPOS_H_ELEVEN_IDX], color);
      break;
    case 24:
      setPixelColor(wordPositions[P124_SPOS_H_TWELVE_IDX], color);
      break;
    }
  }
  /**
   * @brief  checks current settings and update the clock display as needed
   * @note   works on current Run Type!
   * @retval None
   */
  void updateWordClock(int ldr)
  {

    // DO NOT DISTURB Loop, Breath

    if (timeOutReached(next_update))
    {
      // everything, that works without clock goes there
      if (curRunType == P124TypeKnightRider)
      {
        knightRiderNonBlocking(); // does show by its self
      }
      else if (curRunType == P124TypeText)
      {
        scrollTextNonBlocking();
      }
      else
      {
        
        if(ldr > 0 && isLdrAutomatic)
        {
          setLdr(ldr, true);
        }
        // check for time, alarm/breath, ...
        if (curRunType == P124TypeBreath)
        {
          breathNonBlocking();
        }

        if (time_needs_update)
        {
          resetAllPixel(false, true);
          timeToStripe();

          //check alarm state, even if we ignore it here, but KITT or other mode might have removed it, so bring it back
          if(isAlarmOn){
            toggleAlarm(isAlarmOn);
          }
          checkStatus();
          p124_pixels->show();
        }
      }

      //TODO fp123_brightnessAdjust(event, (int)UserVar[event->BaseVarIndex + 3], true);
    }

   
  }
  // ****************************************************
  // *     simple ALARM 
  // ****************************************************
  /**
  * @brief  switch on or off alarm led
  * @note   alarm must be eneabled 
  * @param  on: 1=on, everthing else =off
  * @retval None
  */
  void toggleAlarm(int on)
  {
    if (isAlarmEnabled == true)
    {

      String log = F(PLUGIN_NAME_124);
      log += F("#toggleAlarm:on=");
      log += String(on);
      addLog(LOG_LEVEL_DEBUG, log);
      if (on == 1)
      {
        //turn on
        setPixelColor(pos_alarm, ALARM_COLOR, true);
        isAlarmOn = true;
      }
      else
      {
        //turn off
        setPixelColor(pos_alarm, 0, true);
        isAlarmOn = false;
      }
    }
    else
    {
      String log = F(PLUGIN_NAME_124);
      log += F("#toggleAlarm:called but alarm is not enabled:on=");
      log += String(on);
      addLog(LOG_LEVEL_ERROR, log);
    }
  }

  /**
  * @brief  toggles the alarm light - means if on -> call again to switch off
  * @note   
  * @retval None
  */
  void toggleAlarm()
  {

    if (isAlarmOn == true)
    {
      //turn off
      toggleAlarm(0);
    }
    else
    {
      toggleAlarm(1);
    }
  }

  void checkStatus(){
    if (isStatusEnabled == true && isStatusOn)
    {
      if( (curStatusTime + (60*1000)) <=  millis())
      {
        curStatusColor = 0;
        curStatusTime = 0;
        isStatusOn = false;
        setPixelColor(pos_status, 0, true);

        String log = F(PLUGIN_NAME_124);
        log += F("#checkStatus:time to switch off ");
        addLog(LOG_LEVEL_DEBUG, log);

      }
    }
  }

  void toggleStatus(unsigned long htmlColor)
  {
    toggleStatus((uint32_t)htmlColor);
  }

  void toggleStatus(uint32_t color)
  {
    if (isStatusEnabled == true)
    {
      
      if( (curStatusColor == color && color == P124_STATUS_POSITIVE_COLOR) || (curStatusColor == 0 && color == P124_STATUS_POSITIVE_COLOR)){
        // if the last color was already positive, do not update timer
        /*log = F(PLUGIN_NAME_124);
        log += F("#ignore: color: ");
        log += String(color);
        log += F("; P124_STATUS_POSITIVE_COLOR: ");
        log += String(P124_STATUS_POSITIVE_COLOR);
        addLog(LOG_LEVEL_DEBUG, log);*/
        return;
      }else{
        curStatusColor = color;
        isStatusOn=true;      
        curStatusTime = millis();
        setPixelColor(pos_status, color, true);
      }

      /*
      if (isStatusOn == true && curStatusColor == color)
      {
        //turn off? or double called
        setPixelColor(pos_status, 0);
        isStatusOn = false;
      }
      else
      {
        curStatusColor = color;
        // turn on
        
        isStatusOn = true;
      }*/
    }
  }

  // ****************************************************
  // *     util functions
  // ****************************************************

  uint32_t dimColor(uint32_t color, uint8_t width)
  {
    return (((color & 0xFF0000) / width) & 0xFF0000) + (((color & 0x00FF00) / width) & 0x00FF00) + (((color & 0x0000FF) / width) & 0x0000FF);
  }

  /**
  * @brief  
  * @note   
  * @param  *array: takes standard array with same size as columns, invalid values must be set to -1
  * @param  color: 
  * @retval None  
 */
  void setPixelColor(int *array, uint32_t color)
  {
    for (int i = 0; i < P124_COLUMNS; i++)
    {
      int val = array[i];
      if (i > -1)
      {
        setPixelColor(val, color, false);
      }
    }
  }

  /**
  * @brief  brings given rgb to this LED , does not call show!
  * @note   
  * @param  position: 0 based postion of the led/pixel to change
  * @param  color: color to use
  * 
  * @retval None
  */
  void setPixelColor(int position, uint32_t color)
  {
    setPixelColor(position, color, false);
  }

  /**
  * @brief  brings given rgb to this LED 
  * @note   
  * @param  position: 0 based postion of the led/pixel to change
  * @param  color: color to use
  * @param  withShow: if true updates the led stripe 
  * @retval None
  */
  void setPixelColor(int position, uint32_t color, bool withShow)
  {
    String log;
    /*String log = F(PLUGIN_NAME_124);
    log += F("#setPixelColor:pos: ");
    log += String(position);
    log += F(" color: ");
    log += String(color);
    log += F(" withShow: ");
    log += String(withShow);
    
    addLog(LOG_LEVEL_DEBUG, log);
    */
    if (position >= 0 && position <= P124_MAX_LED)
    {
      p124_pixels->setPixelColor(position, color);

      if (withShow)
      {
        p124_pixels->show();
      }
    }
    else if (position != -1)
    {
      log = F(PLUGIN_NAME_124);
      log += F("#setPixelColor:Error: Position out of range i: ");
      log += String(position);
      addLog(LOG_LEVEL_ERROR, log);
    }
  }
  /**
   * @brief  turns all pixel from 0 to max to same color or off
   * @note   
   * @param  color: color to use
   * @param  withShow: if true updates the led stripe 
   * @retval None
   */
  void setPixelAllColor(uint32_t color, bool withShow)
  {
    setPixelAllColor(color, withShow, false);
  }

  /**
   * @brief  turns all pixel from 0 to max to same color or off
   * @note   
   * @param  color: color to use
   * @param  withShow: if true updates the led stripe 
   * @param  skipStatusAlarm: if true ignores alarm and status 
   * @retval None
   */
  void setPixelAllColor(uint32_t color, bool withShow, bool skipStatusAlarm)
  {
   /* String log = F(PLUGIN_NAME_124);
    log += F("#setPixelAllColor:color ");
    log += color;
    log += F(" :withShow" );
    log += String(withShow);
    log += F(" :P124_MAX_LED " );
    log += String(P124_MAX_LED);
    addLog(LOG_LEVEL_DEBUG, log);
    */
    for (int i = 0; i <= P124_MAX_LED; i++)
    {
      if(!skipStatusAlarm || (i != pos_status && i != pos_alarm))
      {
        setPixelColor(i, color);
      }else{
        String log = F(PLUGIN_NAME_124);
        log += F("#setPixelAllColor:color ");
        log += color;
        log += F(" :withShow" );
        log += String(withShow);
        log += F(" :skipStatusAlarm " );
        log += String(skipStatusAlarm);
        addLog(LOG_LEVEL_DEBUG, log);
      }
    }
    if (withShow)
    {
      p124_pixels->show();
    }
  }

  /**
   * @brief  turns all pixel from 0 to max to no color or in other words off
   * @note   
   * @param  withShow: if true updates the led stripe 
   * @param  skipStatusAlarm: if true ignores alarm and status 
   * @retval None
   */
  void resetAllPixel(bool withShow, bool skipStatusAlarm)
  {
    String log = F(PLUGIN_NAME_124);
    log += F("#resetAllPixel: withShow");
    log += String(withShow);
    log += F(" skipStatusAlarm: ");
    log += String(skipStatusAlarm);
    //addLog(LOG_LEVEL_DEBUG, log);
    setPixelAllColor(0, withShow, skipStatusAlarm);
  }

  /**
   * @brief  turns all pixel from 0 to max to no color or in other words off
   * @note   
   * @param  withShow: if true updates the led stripe 
   * @retval None
   */
  void resetAllPixel(bool withShow)
  {
   resetAllPixel(withShow, false);
  }
  /**
  * @brief   takes a string and converts to int array
  * @note   
  * @param input like  " 2 4 5 12 34 123 207";
  * @param array -> used to store the values
  * @retval  the same array
  */
  int *transform(char *input, int *array)
  {
    char delimiter[] = (",;");
    char *ptr;
    String log = F(PLUGIN_NAME_124);
    log += F(": transform ");
    // initialisieren und ersten Abschnitt erstellen
    ptr = strtok(input, delimiter);
    int i = 0;
    while (ptr != NULL)
    {
      if (isdigit(*ptr))
      {
        int val = strtoul(ptr, &ptr, 10);
#ifdef P124_DEBUG_CONF_TO_POSITION
        log += F(" ");
        log += val;
#endif
        array[i] = val;
      }
      // naechsten Abschnitt erstellen
      ptr = strtok(NULL, delimiter);
      i++;
    }
    //put other pos to -1
    for (; i < P124_COLUMNS; i++)
    {
      array[i] = -1;
    }
#ifdef P124_DEBUG_CONF_TO_POSITION
    addLog(LOG_LEVEL_DEBUG, log);
#endif
    return array;
  }

  /**
   * @brief  generates a json the represents status and settings of this clock
   * @note   
   * @retval 
   */
  DynamicJsonDocument getJsonStatus()
  {
    String log = F(PLUGIN_NAME_124);
    log += F(":getJsonStatus.");
    addLog(LOG_LEVEL_INFO, log);

    DynamicJsonDocument doc(1024);
    doc["PluginId"]=PLUGIN_ID_124;
    doc["PluginName"]=PLUGIN_NAME_124;
    JsonObject rDoc = doc.createNestedObject("runMode");
    rDoc["current"]=int(curRunType);
    JsonObject rDocOp = rDoc.createNestedObject("options");
    rDocOp["P124TypeOff"]=int(P124TypeOff);
    rDocOp["P124TypeClock"]=int(P124TypeClock);
    rDocOp["P124TypeTestLoop"]=int(P124TypeTestLoop);
    rDocOp["P124TypeTestAll"]=int(P124TypeTestAll);
    rDocOp["P124TypeKnightRider"]=int(P124TypeKnightRider);
    rDocOp["P124TypeBreath"]=int(P124TypeBreath);
    rDocOp["P124TypeText"]=int(P124TypeText);
    rDocOp["P124TypeTestClock"]=int(P124TypeTestClock);


    JsonObject ldrDoc = doc.createNestedObject("LDR");
    ldrDoc["current"]=curLdr;
    ldrDoc["automatic"]=isLdrAutomatic;
    ldrDoc["overSampleLdr"]=overSampleLdr;    
    ldrDoc["overSampleLdrDelta"]=(millis()-overSampleLdr);    
    ldrDoc["curBrightnessMode"]=curBrightnessMode;
   
   
    JsonObject ldrDocOp = ldrDoc.createNestedObject("options");
    ldrDocOp["ldrExLowBrghtns"]=ldrExLowBrghtns;
    ldrDocOp["ldrLowBrghtns"]=ldrLowBrghtns;
    ldrDocOp["ldrNormalBrghtns"]=ldrNormalBrghtns;
    ldrDocOp["ldrHighBrghtns"]=ldrHighBrghtns;
    ldrDocOp["ldrExHighLimit"]=ldrExHighLimit;

    JsonObject ldrDocColor = ldrDoc.createNestedObject("color_adjusted");
    ldrDocColor["red"]=cur_red;
    ldrDocColor["green"]=cur_green;
    ldrDocColor["blue"]=cur_blue;
    ldrDocColor["rgb"]=getCurrentRgb();

    JsonObject cDoc = doc.createNestedObject("config");

// status
    JsonObject sDoc = cDoc.createNestedObject("status");
    sDoc["isStatusEnabled"]=isStatusEnabled;
    sDoc["isStatusOn"]=isStatusOn;
    sDoc["pos_status"]=pos_status;
    sDoc["statusColor"]=curStatusColor;

    // alarm
    JsonObject aDoc = cDoc.createNestedObject("alarm");
    aDoc["isAlarmEnabled"]=isAlarmEnabled;
    aDoc["isAlarmOn"]=isAlarmOn;
    aDoc["isBreathOn"]=isBreathOn;
    aDoc["pos_alarm"]=pos_alarm;
   
    
    // minutes
    JsonObject mDoc = cDoc.createNestedObject("minutes");
    mDoc["isMinuteEnabled"]=isMinuteEnabled;
    mDoc["pos_min_1"]=pos_min_1;
    mDoc["pos_min_2"]=pos_min_2;
     mDoc["pos_min_3"]=pos_min_3;
    mDoc["pos_min_4"]=pos_min_4;
    
    JsonObject spDoc = doc.createNestedObject("special");

    //knightrider
    JsonObject kDoc = spDoc.createNestedObject("knightrider");
    kDoc["isKROn"]=isKROn;
    kDoc["curKrPos"]=curKrPos;
    kDoc["curKrWalkToRight"]=curKrWalkToRight;


    
    
    return doc;
  }

  String getLogString() const
  {
    String result;
    String log = F(PLUGIN_NAME_124);
    log += F("#init:LDR Neopixel:");
    //log += String(CONFIG_PIN1);
    log += F(":R:");
    log += String(cur_red);
    log += F(":G:");
    log += String(cur_green);
    log += F(":B:");
    log += String(cur_blue);
    log += String("#ldrmode:");
    log += String(curBrightnessMode);
    log += String("#run mode:");
    log += String(curRunType);
    return result;
  }

  /**
  * @brief  takes current (adjusted values) and put them to a single rgbvalue
  * @note   
  * @retval the current used rgb value
  */
  uint16_t getCurrentRgb()
  {
    int rgb = matrixColor(cur_red, cur_green, cur_blue);
    String log = F(PLUGIN_NAME_124);
    log += F("#getCurrentRgb:");
    log += F(":R:");
    log += String(cur_red);
    log += F(":G:");
    log += String(cur_green);
    log += F(":B:");
    log += String(cur_blue);
    log += F(":=> ");
    log += String(rgb);
    addLog(LOG_LEVEL_DEBUG, log);
    return matrixColor(cur_red, cur_green, cur_blue);
  }

  //just copied from adafruit matrix
  uint16_t matrixColor(uint8_t r, uint8_t g, uint8_t b)
  {
    return ((uint16_t)(r & 0xF8) << 8) |
           ((uint16_t)(g & 0xFC) << 3) |
           (b >> 3);
  }


   // ****************************************************
  // *     my Attributes
  // ****************************************************

  // Pixel
  Adafruit_NeoPixel *p124_pixels = nullptr;
  // Matrix
  Adafruit_NeoMatrix *p124_matrix = nullptr;

  bool isMinuteEnabled = true;
  uint pos_min_1;
  uint pos_min_2;
  uint pos_min_3;
  uint pos_min_4;
  // **** knight rider
  //knightrider KITT
  bool isKROn = false;
  // how many cycle should be KITT run
  uint krCycles = 1;
  uint curKrCycles = 0;
  uint32_t KR_COLOR = p124_pixels->Color(30, 0, 0);
  bool curKrWalkToRight = true;
  int curKrPos = 0;
  uint krStartOffset = 0; 
  uint krEndOffset = 5;
  uint krSpeed = 90;
  uint krWidth = 6;
  uint32_t krOldColorVal[P124_MAX_LED]; // up to 256 lights!


  // **** Text ****
  int textScrollX = 0; //p124_matrix->width(); // put position to the right
  int textScrollPart = 0;
  int textScrollingMax = 0;
  String text;
  // ADAFRUIT NEO MATRIX like colors (16Bit), used to switch scrolling text
  uint16_t textColor; //= p124_matrix->Color(250, 0, 0);
  //p124_matrix->Color(0, 250, 0),
  //p124_matrix->Color(0, 0, 250)};

  // **** alarm
  bool isAlarmEnabled = true;
  bool isAlarmOn = false;
  uint pos_alarm = 0;
  uint32_t ALARM_COLOR = p124_pixels->Color(30, 0, 0);
  ; //= Adafruit_NeoPixel->Color(30, 0, 0);

  // **** status
  bool isStatusEnabled = true;
  bool isStatusOn = false;
  uint pos_status = 0;
  uint32_t curStatusColor =0;
  long curStatusTime =-1;

  // ******** snake
  bool isSnakeStyle;
  // current mode the clock should work on
  P124_RUN_TYPE curRunType = P124TypeClock;
  // flag, to tell we should update the time. eg. if we come back from other run mode
  bool time_needs_update = false;
  // time when the next update can be performed, so used to keep other task from interupt something
  unsigned long next_update = 0;

  // **** config values
  //  red value from config
  byte red = 0;
  //  green value from config
  byte green = 0;
  //  blue value from config
  byte blue = 0;

  // current adjusted red value, based on ldr
  byte cur_red = 0;
  // current adjusted green value, based on ldr
  byte cur_green = 0;
  // current adjusted blue value, based on ldr
  byte cur_blue = 0;

  // current ldr value, used to calculate brightness of colors
  uint curLdr = 0;
  bool isLdrAutomatic = 1;
  
  long overSampleLdr = 0; //timestamp when over sample was started
  // values for brightness modes based on ldr
  uint8_t ldrExLowBrghtns = 30;
  uint8_t ldrLowBrghtns = 60;
  uint8_t ldrNormalBrghtns = 100;
  uint8_t ldrHighBrghtns = 150;
  uint8_t ldrExHighLimit = 180;
  // current active brightness mode
  uint curBrightnessMode=ldrLowBrghtns;

  String displayLines[P124_MAX_WC_WORDS];
  int wordPositions[P124_MAX_WC_WORDS][P124_COLUMNS];

 
  //time, just keep them here, as we have test function to set time directly via comman 
  int hours = -1;
  int minutes =-1;

  // *** breath
  // stores current brightness of breathing led
  unsigned int curBreathBrightness = 1;
  // breathing turned on or off
  bool isBreathOn = false;

  // breath in or out
  bool curBreathIn = true;
};

//A plugin has to implement the following function
boolean Plugin_124(byte function, struct EventStruct *event, String &string)
{
  //function: reason the plugin was called
  //event: ??add description here??
  // string: ??add description here??

  boolean success = false;

  switch (function)
  {
  case PLUGIN_DEVICE_ADD:
  {
    //This case defines the device characteristics, edit appropriately

    Device[++deviceCount].Number = PLUGIN_ID_124;
    Device[deviceCount].Type = DEVICE_TYPE_SINGLE;  //how the device is connected
    Device[deviceCount].VType = SENSOR_TYPE_SWITCH; //using type string to have string values instead of long, but only 1 evetn->string
    Device[deviceCount].Ports = 0;
    Device[deviceCount].PullUpOption = false;
    Device[deviceCount].InverseLogicOption = false;
    Device[deviceCount].FormulaOption = false;
    // TODO looks like only 4 values allowed
    Device[deviceCount].ValueCount = 4; //number of output variables. The value should match the number of keys PLUGIN_VALUENAME1_xxx
    Device[deviceCount].SendDataOption = true; //to force reading the values and show on web
    Device[deviceCount].TimerOption = true; // to chane to value under 60s
    Device[deviceCount].TimerOptional = false;
    Device[deviceCount].GlobalSyncOption = true;
    Device[deviceCount].DecimalsOnly = false;
    break;
  }

  case PLUGIN_GET_DEVICENAME:
  {
    //return the device name
    string = F(PLUGIN_NAME_124);
    break;
  }

  case PLUGIN_GET_DEVICEVALUENAMES:
  {
    //called when the user opens the module configuration page
    //it allows to add a new row for each output variable of the plugin
    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_124));
    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_124));
    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_124));
    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_124));

//    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[4], PSTR(PLUGIN_VALUENAME5_124));
//    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[5], PSTR(PLUGIN_VALUENAME6_124));
    break;
  }
  case PLUGIN_SET_DEFAULTS:
  {

    CONFIG_PIN1 = 13;
    P124_COLOR_RED_GREEN = p124_bytesToInt(P124_COLOR_RED_DFLT, P124_COLOR_GREEN_DFLT);
    P124_COLOR_BLUE = P124_COLOR_BLUE_DFLT;
    // minutes
    P124_POS_MIN_1_AND_2 = p124_bytesToInt(P124_POS_MIN_1_DFLT, P124_POS_MIN_2_DFLT);
    P124_POS_MIN_3_AND_4 = p124_bytesToInt(P124_POS_MIN_3_DFLT, P124_POS_MIN_4_DFLT);
    // alarm
    P124_POS_ALARM_STATUS = p124_bytesToInt(P124_POS_ALARM_DFLT, P124_POS_STATUS_DFLT);
    // init min, alarm, status and snake booleans with default values
    P124_BOOLEANS = p124_boolsToInt(1, P124_WITH_MIN_DFLT, P124_WITH_ALARM_DFLT, P124_WITH_STATUS_DLFT, P124_SNAKE_STYLE_DFLT);

    char deviceTemplate[P124_MAX_WC_WORDS][P124_MAX_WC_WORD_LENGTH];
    for (byte idx = 0; idx < P321_NELEMS(P124_WC_WORDS_POS_DFLT); idx++)
    {

      String argName = P124_ARG_NAME;
      argName += idx + 1;
      if (!safe_strncpy(deviceTemplate[idx], P124_WC_WORDS_POS_DFLT[idx], P124_MAX_WC_WORD_LENGTH))
      {
        // Todo error handling
      }
    }
    SaveCustomTaskSettings(event->TaskIndex, (byte *)&deviceTemplate, sizeof(deviceTemplate));

    // LDR values to int
    P124_LDR_XTR_LOW_AND_LOW = p124_bytesToInt(P124_BRGHTNS_Mode_ExtraLow, P124_BRGHTNS_Mode_Low);
    P124_LDR_NRML_AND_HIGH = p124_bytesToInt(P124_BRGHTNS_Mode_Normal, P124_BRGHTNS_Mode_High);
    P124_LDR_XTR_HIGH = p124_bytesToInt(P124_BRGHTNS_Mode_ExtraHigh, 0);

    success = true;
    break;
  }
  case PLUGIN_WEBFORM_LOAD:
  {
    //this case defines what should be displayed on the web form, when this plugin is selected
    //The user's selection will be stored in
    //PCONFIG(x) (custom configuration)

    // Make sure not to append data to the string variable in this PLUGIN_WEBFORM_LOAD call.
    // This has changed, so now use the appropriate functions to write directly to the Streaming
    // WebServer. This takes much less memory and is faster.
    // There will be an error in the web interface if something is added to the "string" variable.

    //Use any of the following (defined at WebServer.ino):
    //addFormNote(F("not editable text added here"));
    //To add some html, which cannot be done in the existing functions, add it in the following way : addHtml(F("<TR><TD>Analog Pin:<TD>"));
    //For strings, always use the F() macro, which stores the string in flash, not in memory.

    //String dropdown[5] = { F("option1"), F("option2"), F("option3"), F("option4")};
    //addFormSelector(string, F("drop-down menu"), F("plugin_xxx_displtype"), 4, dropdown, NULL, PCONFIG(0));

    //number selection (min-value - max-value)
    //addFormNumericBox(string, F("description"), F("plugin_xxx_description"), PCONFIG(1), min - value, max - value);

    bool isLdrAuto, withMinutes, withAlarm, withStatus,
        isSnakeStyle;
    p124_intToBools(P124_BOOLEANS, isLdrAuto, withMinutes, withAlarm, withStatus,
               isSnakeStyle);

    String log = F(PLUGIN_NAME_124);
    log += F(":load:P124_BOOLEANS");
    log += String(P124_BOOLEANS);
    log += String(", withMinutes=");
    log += String(withMinutes);
    log += String(", withAlarm=");
    log += String(withAlarm);
    addLog(LOG_LEVEL_INFO, log);

    String test = String(P124_BOOLEANS);
    test += "isLdrAuto: ";
    test += String(isLdrAuto);
    test += F(", withMinutes: ");
    test += String(withMinutes);
    test += F(", withAlarm: ");
    test += String(withAlarm);
    addFormNote(test);

    addFormSubHeader(F("Color Settings"));

    uint16 loadValue = P124_COLOR_RED_GREEN;
    uint8_t h; //= ( loadValue >> 0) ;
    uint8_t l; //= ( loadValue >> 8)   ;
    p124_intToByte(loadValue, h, l);

    addFormNumericBox(F("Red"), F("p124_red"), h, 0, 255);
    addFormNumericBox(F("Green"), F("p124_green"), l, 0, 255);
    addFormNumericBox(F("Blue"), F("p124_blue"), P124_COLOR_BLUE, 0, 255);

    String colorOptions[2];
    colorOptions[0] = F("Color - RGB");
    colorOptions[1] = F("Color - HTML");
    int colorOptionValues[2] = {0, 1};
    int colorType = 0;
    addFormSelector(F("Color Type"), F("p124_color_type"), 2, colorOptions, colorOptionValues, colorType);

    //TODO: would be cool to switch on the fly, but works only on reload.
    char hex[7] = {0};
    sprintf(hex, "%02X%02X%02X", h, l, P124_COLOR_BLUE);
    addFormTextBox(String(F("HTML ")), String(F("p124_html_color")), hex, 8);

    addFormSubHeader(F("LDR Settings"));
    addFormNote(F("Settings for Adjustment of brightness based on LDR value. Please add a rule to publish ldr value to the clock device."));
    addFormCheckBox(F("automatic Mode"), F("p124_ldrAutomatic"), isLdrAuto);
    addFormNumericBox(F("LDR"), F("p124_ldr"), P124_LDR_VALUE, 0, 255);

    loadValue = P124_LDR_XTR_LOW_AND_LOW;
    h = 0; //= ( loadValue >> 0) ;
    l = 0; //= ( loadValue >> 8)   ;
    p124_intToByte(loadValue, h, l);

    addFormNumericBox(F("Extra Low"), F("p124_ldr_ex_low_brghtns"), h, 10, 255);
    addFormNumericBox(F("Low"), F("p124_ldr_low_brghtns"), l, 10, 255);

    loadValue = P124_LDR_NRML_AND_HIGH;
    p124_intToByte(loadValue, h, l);

    addFormNumericBox(F("Normal"), F("p124_ldr_normal_low_brghtns"), h, 10, 255);
    addFormNumericBox(F("High"), F("p124_ldr_high_brghtns"), l, 10, 255);

    loadValue = P124_LDR_XTR_HIGH;
    p124_intToByte(loadValue, h, l);

    addFormNumericBox(F("Extra High"), F("p124_ldr_ex_high_brghtns"), h, 10, 255);

    addFormSeparator(2);
    addFormSubHeader(F("Optional LEDs"));
    addFormNote(F("Turn ON / OFF Minutes or Alarm. And if ON, set 0 based position of the leds."));
    // minutes and alarm
    addFormCheckBox(F("With Minutes"), F("v_P124_withMinutes"), withMinutes);

    p124_intToByte(P124_POS_MIN_1_AND_2, h, l);

    addFormNumericBox(F("Minute 1 Position"), F("v_P124_POS_MIN_ONE"), h, 0, MAX_LED);
    addFormNumericBox(F("Minute 2 Position"), F("v_P124_POS_MIN_TWO"), l, 0, MAX_LED);

    p124_intToByte(P124_POS_MIN_3_AND_4, h, l);

    addFormNumericBox(F("Minute 3 Position"), F("v_P124_POS_MIN_THREE"), h, 0, MAX_LED);
    addFormNumericBox(F("Minute 4 Position"), F("v_P124_POS_MIN_FOUR"), l, 0, MAX_LED);

    p124_intToByte(P124_POS_ALARM_STATUS, h, l);

    addFormCheckBox(F("With Alarm"), F("v_P124_withAlarm"), withAlarm);
    addFormNumericBox(F("Alarm Position"), F("v_P124_POS_ALARM"), h, 0, MAX_LED);

    addFormNote(F("Defines LED to show some statuses eg. reboot startup.  Note. Still in Progress"));
    addFormCheckBox(F("With Status"), F("v_P124_withStatus"), withStatus);
    addFormNumericBox(F("Status Position"), F("v_P124_POS_STATUS"), l, 0, MAX_LED);

    addFormSeparator(2);
    addFormSubHeader(F("Other"));
    addFormNote(F("comment in, if led strip is connected in snake SNAKE_STYLE <br>---00-01-02-03-04-05-06-07-08-09--<br>--20-19-18-17-16-15-14-13-12-11-|<br> |-21-22-23<br>Snake like or ZigZag"));
    addFormCheckBox(F("Snake Style"), F("v_P124_snakestyle"), isSnakeStyle);

    addFormSeparator(2);
    addFormSubHeader(F("Word to Positions"));
    addFormNote(F("defines which LED to light up on given time"));

   // P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
    /*
    DynamicJsonDocument json = getJsonStatus();
    String value;
    serializeJsonPretty(json, value);
    addFormTextArea(F("Test"), F("123"), value,1024, 10,11);
    */
    char deviceTemplate[P124_MAX_WC_WORDS][P124_MAX_WC_WORD_LENGTH];
    LoadCustomTaskSettings(event->TaskIndex, (byte *)&deviceTemplate, sizeof(deviceTemplate));
    if (nullptr != deviceTemplate)
    {
      for (byte varNr = 0; varNr < P124_MAX_WC_WORDS; varNr++)
      {
        //addFormTextBox(String(F("MQTT Topic ")) + (varNr + 1), String(F("p037_template")) +
        //		(varNr + 1), deviceTemplate[varNr], 40);
        addFormTextBox(String(P124_WC_LABLES[varNr]), String(P124_WC_LABLES[varNr]), deviceTemplate[varNr], P124_MAX_WC_WORD_LENGTH);
      }
    }
    else
    {
      addFormNote(F("Using default positions, save first ..."));
      for (byte varNr = 0; varNr < P124_MAX_WC_WORDS; varNr++)
      {

        addFormTextBox(String(P124_WC_LABLES[varNr]), String(P124_WC_LABLES[varNr]), String(P124_WC_WORDS_POS_DFLT[varNr]), P124_MAX_WC_WORD_LENGTH);
      }
    }
    /*
    if (nullptr != P124_data)
    {
      log = F(PLUGIN_NAME_124);
      log += F(":load:start reding display lines from struct: FreeMem:");
      log += FreeMem();
      log += F(" FreeStack: ");
      log += getCurrentFreeStack();
      addLog(LOG_LEVEL_DEBUG, log);

      // TODO crashes quite often here
      P124_data->loadDisplayLines(event->TaskIndex);

      log = F(PLUGIN_NAME_124);
      log += F(":load:read them, now put to web");

      addLog(LOG_LEVEL_DEBUG, log);
      for (byte varNr = 0; varNr < P124_MAX_WC_WORDS; varNr++)
      {
        
        //addFormTextBox(String(P124_WC_LABLES[varNr]), String(P124_WC_LABLES[varNr]), P124_data->displayLines[varNr], P124_MAX_WC_WORD_LENGTH);
        addFormTextBox(String(P124_WC_LABLES[varNr]), String(P124_WC_LABLES[varNr]), P124_data->displayLines[varNr], P124_MAX_WC_WORD_LENGTH);
      }
    }
    else
    {
      addFormNote(F("Using default positions, save first ..."));
      for (byte varNr = 0; varNr < P124_MAX_WC_WORDS; varNr++)
      {

        addFormTextBox(String(P124_WC_LABLES[varNr]), String(P124_WC_LABLES[varNr]), String(P124_WC_WORDS_POS_DFLT[varNr]), P124_MAX_WC_WORD_LENGTH);
      }
    }
*/
    success = true;
    break;
  }

  case PLUGIN_WEBFORM_SAVE:
  {
    //this case defines the code to be executed when the form is submitted
    //the plugin settings should be saved to PCONFIG(x)
    //ping configuration should be read from CONFIG_PIN1 and stored
    bool isLdrAuto, withMinutes, withAlarm, withStatus,
        isSnakeStyle;

    uint8 h = getFormItemInt(F("p124_red"));
    uint8 l = getFormItemInt(F("p124_green"));
    P124_COLOR_RED_GREEN = p124_bytesToInt(h, l);
    P124_COLOR_BLUE = getFormItemInt(F("p124_blue"));
    P124_LDR_VALUE = getFormItemInt(F("p124_ldr"));

    isLdrAuto = isFormItemChecked(F("p124_ldrAutomatic"));

    h = getFormItemInt("p124_ldr_ex_low_brghtns");
    l = getFormItemInt("p124_ldr_low_brghtns");

    uint16 saveValue = p124_bytesToInt(h, l); //(lB << 8)  | ( elB & 0xff);
    P124_LDR_XTR_LOW_AND_LOW = saveValue;

    h = getFormItemInt("p124_ldr_normal_low_brghtns");
    l = getFormItemInt("p124_ldr_high_brghtns");
    P124_LDR_NRML_AND_HIGH = p124_bytesToInt(h, l);
    h = getFormItemInt("p124_ldr_ex_high_brghtns");
    P124_LDR_XTR_HIGH = p124_bytesToInt(h, 0);
    /*  addFormNumericBox(F("LDR"), F("p124_ldr_ex_low_brghtns"), "", 0, 255);
    addFormNumericBox(F("LDR"), F("p124_ldr_low_brghtns"), "", 0, 255);
    addFormNumericBox(F("LDR"), F("p124_ldr_normal_low_brghtns"), "", 0, 255);
    addFormNumericBox(F("LDR"), F("p124_ldr_ex_high_brghtns"), "", 0, 255);
    addFormNumericBox(F("LDR"), F("p124_ldr_ex_high_brghtns"), "", 0, 255);
*/
    withMinutes = isFormItemChecked(F("v_P124_withMinutes"));
    //P124_WITH_MIN = withMinutes;

    h = getFormItemInt(F("v_P124_POS_MIN_ONE"));
    l = getFormItemInt(F("v_P124_POS_MIN_TWO"));
    P124_POS_MIN_1_AND_2 = p124_bytesToInt(h, l);

    h = getFormItemInt(F("v_P124_POS_MIN_THREE"));
    l = getFormItemInt(F("v_P124_POS_MIN_FOUR"));
    P124_POS_MIN_3_AND_4 = p124_bytesToInt(h, l);
    withAlarm = isFormItemChecked(F("v_P124_withAlarm"));
    //P124_WITH_ALARM = withAlarm;
    h = getFormItemInt(F("v_P124_POS_ALARM"));
    withStatus = isFormItemChecked(F("v_P124_withStatus"));
   // P124_WITH_STATUS = withStatus;

    l = getFormItemInt(F("v_P124_POS_STATUS"));
    P124_POS_ALARM_STATUS = p124_bytesToInt(h, l);

    isSnakeStyle = isFormItemChecked(F("v_P124_snakestyle"));
    
    //P124_SNAKE_STYLE = isSnakeStyle;

    P124_BOOLEANS = p124_boolsToInt(isLdrAuto, withMinutes, withAlarm, withStatus,
                               isSnakeStyle);

    String log = F(PLUGIN_NAME_124);
    log += F(":save:P124_BOOLEANS");
    log += String(P124_BOOLEANS);
    addLog(LOG_LEVEL_INFO, log);

    P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
    if (nullptr != P124_data)
    {
      uint8 h = 0;
      uint8 l = 0;
      p124_intToByte(P124_COLOR_RED_GREEN, h, l);
      P124_data->red = h;
      P124_data->green = l;

      p124_intToByte(P124_COLOR_BLUE, h, l);
      P124_data->blue = h;
      //TODO better just recalc

      P124_data->curLdr = P124_LDR_VALUE;
      P124_data->isLdrAutomatic = isLdrAuto;

      P124_data->isMinuteEnabled = withMinutes;
      p124_intToByte(P124_POS_MIN_1_AND_2, h, l);
      P124_data->pos_min_1 = h;
      P124_data->pos_min_2 = l;
      p124_intToByte(P124_POS_MIN_3_AND_4, h, l);
      P124_data->pos_min_3 = h;
      P124_data->pos_min_4 = l;

      p124_intToByte(P124_POS_ALARM_STATUS, h, l);
      P124_data->isAlarmEnabled = withAlarm;
      P124_data->pos_alarm = h;

      P124_data->isStatusEnabled = withStatus;
      P124_data->pos_status = l;

      //P124_data->calculateMarks();
    }

    //copy lines from web
    char deviceTemplate[P124_MAX_WC_WORDS][P124_MAX_WC_WORD_LENGTH];
    String error;
    for (byte varNr = 0; varNr < P124_MAX_WC_WORDS; varNr++)
    {
      // String argName = F("p075_template");
      // argName += varNr + 1;
      String value = WebServer.arg(P124_WC_LABLES[varNr]);
      if (!safe_strncpy(deviceTemplate[varNr], value, P124_MAX_WC_WORD_LENGTH))
      {
        error += getCustomTaskSettingsError(varNr);
      }
     
    }
    if (error.length() > 0)
    {
      addHtmlError(error);
    }

    // save them
    SaveCustomTaskSettings(event->TaskIndex, (byte *)&deviceTemplate, sizeof(deviceTemplate));
    
    // *** after success, reinit / reset is force by ESPEsay, so no need to call it self
    //  so we just need to wait on next time event
    success = true;
    break;
  }
  case PLUGIN_INIT:
  {
    //this case defines code to be executed when the plugin is initialised
    initPluginTaskData(event->TaskIndex, new P124_data_struct());
    P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
    if (nullptr != P124_data)
    {
      P124_data->init(event);
      String log = F(PLUGIN_NAME_124);
      log += F("#init: isInitialized: ");
      log += String(P124_data->isInitialized());

      addLog(LOG_LEVEL_INFO, log);
      // status
      P124_data->toggleStatus(P124_STATUS_CRITICAL_COLOR);
      delay(0);
      /*
      P124_data->toggleStatus(P124_STATUS_CRITICAL_COLOR);
      delay(800);
      P124_data->toggleStatus(P124_STATUS_POSITIVE_COLOR);
      delay(800);
      P124_data->toggleStatus(P124_STATUS_NEUTRAL_COLOR);
      delay(800);
      P124_data->toggleStatus(P124_STATUS_INFO_COLOR);
      delay(800);
*/

      // load task stuff
      P124_data->loadDisplayLines(event->TaskIndex);
     // P124_data->toggleStatus(P124_STATUS_NEUTRAL_COLOR);
      //after the plugin has been initialised successfully, set success and break

      // if we are on an init AFTER config changes, try to rebuild time
      if(hour()){
        P124_data->updateTime();
      }

      addLog(LOG_LEVEL_INFO, P124_data->getLogString());
      success = true;
    }
    else
    {
      String log = F(PLUGIN_NAME_124);
      log += F(":could not init");
      addLog(LOG_LEVEL_ERROR, log);
    }
    break;
  }

  case PLUGIN_CLOCK_IN:
  {
    //code to be executed to read data
    P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
    if (nullptr == P124_data)
    {
      break;
    }

    P124_data->updateTime(hour(), minute());
    /*
    String log;
    log.reserve(128);
    log = String(PLUGIN_NAME_124);
    log += F(" 01: ");
    log += String(P124_data->displayLines[1]);

    addLog(LOG_LEVEL_DEBUG, log); */

    success = true;
    break;
  }
  case PLUGIN_READ:
  {
    //code to be executed to read data
    //It is executed according to the delay configured on the device configuration page, only once
    success = p124_writeValues(event);
    break;
  }
  case PLUGIN_WRITE:
  {
    //this case defines code to be executed when the plugin executes an action (command).
    //Commands can be accessed via rules or via http.
    //As an example, http://192.168.1.12//control?cmd=dothis
    //implies that there exists the comamnd "dothis"

    P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
    if (nullptr == P124_data)
    {
      break;
    }

    //parse string to extract the command
    String tmpString = string;
    int argIndex = tmpString.indexOf(',');
    if (argIndex)
    {
      tmpString = tmpString.substring(0, argIndex);
    }

    if (P124_data->isInitialized() == false)
    {
      String log = F(PLUGIN_NAME_124);
      log += F("#http command:");
      log += string;
      log += F("'skipped. Plugin is not initialised!!");
      addLog(LOG_LEVEL_ERROR, log);
      break;
    }

    String log = F(PLUGIN_NAME_124);
    log += F("#http full command:");
    log += string;
    log += F("; subcommand:");
    log += tmpString;

    log += F("; initialised? ");
    log += P124_data->isInitialized();
    addLog(LOG_LEVEL_INFO, log);
    

    if (tmpString.equalsIgnoreCase(F("BLZWC_SETTIME")))
    {
      int h = event->Par1;
      if (!(h >= 0 && h <= 24))
      {
        h = 23;
      }
      int m = event->Par2;
      if (!(m >= 0 && m <= 59))
      {
        m = 23;
      }
      P124_data->updateTime(h, m);
      //do something
      success = true; //set to true only if plugin has executed a command successfully
    }
    //*********** Text **************
    if (tmpString.equalsIgnoreCase(F("BLZWC_TEXT")))
    {
      argIndex = string.lastIndexOf(',');
      String txt = string.substring(argIndex + 1);
      String log = F(PLUGIN_NAME_124);
      log += F("#text:");
      log += String(txt);
      addLog(LOG_LEVEL_DEBUG, log);
      P124_data->initText(txt.c_str());
      success = true;
    }

    //*********** Knight Rider - K.I.T.T. **************
    if (tmpString.equalsIgnoreCase(F("BLZWC_KITT")))
    {
      int onOff = -1;
      //check if paramtere is there
      if( string.lastIndexOf(",") >0){ 
        onOff = event->Par1;
      }
      //check range
      if (onOff < 0 || onOff > 1)
      {
        // behave like toggle switch
        P124_data->toggleKnightRider();
      }
      else
      {
        // act like on / off switch
        P124_data->toggleKnightRider(onOff);
      }
      success = true; //set to true only if plugin has executed a command successfully
    }

    //*********** ALARM **************
    if (tmpString.equalsIgnoreCase(F("BLZWC_ALARM")))
    {
      int onOff = -1;
      //check if paramtere is there
      if( string.lastIndexOf(",") >0){ 
        onOff = event->Par1;
      }
      //check range
      if (onOff < 0 || onOff > 1)
      {
        // behave like toggle switch
        P124_data->toggleAlarm();
      }
      else
      {
        // act like on / off switch
        P124_data->toggleAlarm(onOff);
      }           
      success = true; //set to true only if plugin has executed a command successfully
    }
    //*********** LDR **************
    if (tmpString.equalsIgnoreCase(F("BLZWC_SETLDR")))
    {
      int ldr = event->Par1;
      P124_data->setLdr(ldr, false);
      success = true; //set to true only if plugin has executed a command successfully
    }
    //*********** BREATH **************
    if (tmpString.equalsIgnoreCase(F("BLZWC_ALARMBREATH")))
    {      
      int onOff = -1;
      //check if paramtere is there
      if( string.lastIndexOf(",") >0){ 
        onOff = event->Par1;
      }
      //check range
      if (onOff < 0 || onOff > 1)
      {
        // behave like toggle switch
        P124_data->toggleAlarmBreath();
      }
      else
      {
        // act like on / off switch
        P124_data->toggleAlarmBreath(onOff);
      }
      success = true; //set to true only if plugin has executed a command successfully
    }
    //*********** CLOCK **************
    if (tmpString.equalsIgnoreCase(F("BLZWC_CLOCK")))
    {      
      int onOff = -1;
      //check if paramtere is there
      if( string.lastIndexOf(",") >0){ 
        onOff = event->Par1;
      }
      //check range
      if (onOff < 0 || onOff > 1)
      {
        // behave like toggle switch
        P124_data->toggleClock();
      }
      else
      {
        // act like on / off switch
        P124_data->toggleClock(onOff);
      }
      success = true; //set to true only if plugin has executed a command successfully
    }

    if (tmpString.equalsIgnoreCase(F("BLZWC_STATUS")))
    {
      // just print json status
      success = true; //set to true only if plugin has executed a command successfully
    }

    //*********** TEST **************
    if (tmpString.equalsIgnoreCase(F("BLZWC_TEST_ALL")))
    {
      int r = event->Par1;
      int g = event->Par2;
      int b = event->Par3;
      if (NULL == r || NULL == g || NULL == b)
      {

        P124_data->testAll();
      }
      else
      {
        P124_data->testAll(r, g, b);
      }
      success = true;
    }

    if (tmpString.equalsIgnoreCase(F("BLZWC_TEST")))
    {
      int pos = event->Par1;
      int r = event->Par2;
      int g = event->Par3;
      int b = event->Par4;
      if (pos < 0 && pos > P124_MAX_LED)
      {
        String log = F(PLUGIN_NAME_124);
        log += F("#BLZWC_TEST:position wrong or missing, set to pos 1: pos=");
        log += pos;
        addLog(LOG_LEVEL_ERROR, log);
      }

      if ((r < 0 && r >= 255) ||
          (g < 0 && g >= 255) ||
          (b < 0 && b >= 255))
      {
        String log = F(PLUGIN_NAME_124);
        log += F("#BLZWC_TEST:wrong or missing color.");
        log += pos;
        addLog(LOG_LEVEL_ERROR, log);
        P124_data->testSingle(pos);
      }
      else
      {
        P124_data->testSingle(pos, r, g, b);
      }
      success = true;
    }
    if(success ){
        // get json response
      DynamicJsonDocument json = P124_data->getJsonStatus();
      String output;
      serializeJson(json, output);
      printToWebJSON=true;
      
      SendStatus(event->Source, output); // send http response to controller (JSON format)
      // update values/UserVars
      p124_writeValues(event);
    }
    
    break;
  }

  case PLUGIN_EXIT:
  {
    String log = F(PLUGIN_NAME_124);
    log += F("#PLUGIN_EXIT:cleanUp");
    addLog(LOG_LEVEL_INFO, log);
    P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
    if (nullptr != P124_data)
    {
      P124_data -> resetAllPixel(true,false);
      P124_data -> reset();
    }
    
    //perform cleanup tasks here. For example, free memory
    clearPluginTaskData(event->TaskIndex);
    break;
  }

  case PLUGIN_ONCE_A_SECOND:
  {
    // TODO WifiConnected might take some time ... so just check every 10 secs?
    //code to be executed once a second. Tasks which do not require fast response can be added here
    if (!WiFiConnected())
    {
      String log = F(PLUGIN_NAME_124);
      log += F(": WiFi NOT Connected: ");
      addLog(LOG_LEVEL_ERROR, log);

      P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
      if (nullptr == P124_data)
      {
        break;
      }
      P124_data->toggleStatus(P124_STATUS_NEGATIVE_COLOR);

      p124_writeValues(event);
    
    }else{
      P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
      if (nullptr == P124_data)
      {
        break;
      }
      P124_data->toggleStatus(P124_STATUS_POSITIVE_COLOR);
    }

    success = true;
    break;
  }

  case PLUGIN_TEN_PER_SECOND:
  {
    //code to be executed 10 times per second. Tasks which require fast response can be added here
    //be careful on what is added here. Heavy processing will result in slowing the module down!

    success = true;
    break;
  }
  case PLUGIN_FIFTY_PER_SECOND:
  {
    P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
    if (nullptr == P124_data)
    {
      String log = F(PLUGIN_NAME_124);
      log += F(": Did not find data for plugin: ");
      addLog(LOG_LEVEL_ERROR, log);
      break;
    }
    if (timeOutReached(P124_data->next_update))
    {
      uint ldr =UserVar[event->BaseVarIndex + 3];
      P124_data->updateWordClock(ldr);
    }
   
    success = true;
    break;
  }

  } // switch
  return success;

} //function

/**
 * @brief  takes internal value and put them to UserVar
 * @note   
 * @param  *event: 
 * @retval true if successfully written 
 */
bool p124_writeValues(struct EventStruct *event)
{
  bool success =false;
  P124_data_struct *P124_data = static_cast<P124_data_struct *>(getPluginTaskData(event->TaskIndex));
  if (nullptr != P124_data)
  {
     
      UserVar[event->BaseVarIndex + 0] = (int)P124_data->getCurrentRgb();
      UserVar[event->BaseVarIndex + 1] = (int)P124_data->curBrightnessMode;
      UserVar[event->BaseVarIndex + 2] = (int)P124_data->curRunType;
     //LDR UserVar[event->BaseVarIndex + 3] = (int)P124_data->cur_green;
    success = true;
  }
  return success;
}

/**
 * @brief  bit shift to get from 16bit int booleans
 * @note   
 * @param  value16: 
 * @param  isLdrAuto: 
 * @param  withMinutes: 
 * @param  withAlarm: 
 * @param  withStatus: 
 * @param  isSnakeStyle: 
 * @retval None
 */
void p124_intToBools(uint16 value16,
                bool &isLdrAuto, bool &withMinutes, bool &withAlarm, bool &withStatus,
                bool &isSnakeStyle)
{

  isLdrAuto = value16 & isLdrAutoMask;
  withMinutes = value16 & withMinutesMask;
  withAlarm = value16 & withAlarmMask;
  withStatus = value16 & withStatusMask;
  isSnakeStyle = value16 & isSnakeStyleMask;
  String log = F(PLUGIN_NAME_124);
  log += F(":p124_intToBools:value16=");
  log += String(value16);
  log += String(" test=");
  log += value16 & withMinutesMask;
  log += String(", withMinutes=");
  log += String(withMinutes);
  log += String(", withAlarm=");
  log += String(withAlarm);
  log += String(", withStatus=");
  log += String(withStatus);
  log += String(", isSnakeStyle=");
  log += String(isSnakeStyle);

  addLog(LOG_LEVEL_INFO, log);
}
/**
 * @brief  takes bools and stores them in a int value
 * @note   
 * @param  isLdrAuto: 
 * @param  withMinutes: 
 * @param  withAlarm: 
 * @param  withStatus: 
 * @param  isSnakeStyle: 
 * @retval 
 */
uint16 p124_boolsToInt(bool isLdrAuto, bool withMinutes, bool withAlarm, bool withStatus,
                  bool isSnakeStyle)
{
  uint16 result = 0;
  if (isLdrAuto)
    result = result & (~isLdrAutoMask) | isLdrAutoMask;
  if (withMinutes)
    result = result & (~withMinutesMask) | withMinutesMask;
  if (withAlarm)
    result = result & (~withAlarmMask) | withAlarmMask;
  if (withStatus)
    result = result & (~withStatusMask) | withStatusMask;
  if (isSnakeStyle)
    result = result & (~isSnakeStyleMask) | isSnakeStyleMask;
  String log = F(PLUGIN_NAME_124);
  log += F(":p124_boolsToInt:result=");
  log += String(result);
  log += String(", withMinutes=");
  log += String(withMinutes);
  log += String(", withAlarm=");
  log += String(withAlarm);
  log += String(", withStatus=");
  log += String(withStatus);
  log += String(", isSnakeStyle=");
  log += String(isSnakeStyle);

  addLog(LOG_LEVEL_INFO, log);
  return result;
}

/**
 * @brief  bit shift to get from 16bit int , two 8bits
 * @note   
 * @param  value16: 
 * @param  resultHigh: 
 * @param  reusltLow: 
 * @retval None
 */
void p124_intToByte(uint16 value16, uint8 &resultHigh, uint8 &reusltLow)
{

  reusltLow = (value16 >> 0);
  resultHigh = (value16 >> 8);
}

/**
 * @brief  takes to 8bit values and creates 16bit
 * @note   
 * @param  high: 
 * @param  low: 
 * @retval 
 */
uint16 p124_bytesToInt(uint8 high, uint8 low)
{

  uint16 result = (high << 8) | (low & 0xff);
  return result;
}
//#endif // USES_P124x