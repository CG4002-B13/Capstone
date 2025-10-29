//CONSTANTS FOR ESP32

//I2C addresses
#define MPU_ADDR 0x70
#define MAX_ADDR 0x36

#define VOICE_DATA "esp32/voice_data"
#define VOICE_RESULT "ultra96/voice_result"
//#define GESTURE_DATA "esp32/gesture_data"
#define COMMAND "esp32/command"

//MAX17043 constants
// #ifdef __AVR__
//   #define ALR_PIN 2
// #else
//   #define ALR_PIN D2
// #endif
#define LOW_BATTERY 10

#define GREEN_PIN 23
#define RED_PIN 19
#define BLUE_PIN 18

#define BUTTON_SCREENSHOT 9 //D5
#define BUTTON_SELECT 27 //D4
#define BUTTON_DELETE 10 //D6
#define BUTTON_MOVE 13 //D7
#define BUTTON_ROTATE 5 //D8
#define BUZZER 2 //D9

#define MIC_WS 25 //D2
#define MIC_SCK 26 //D3
#define MIC_SD 35 //A3 - READ ONLY

#define SAMPLING_RATE 8000 //Hz
#define BITS_PER_SAMPLE 16
#define CHANNELS 1
#define RECORD_TIME 2 //seconds
#define HEADER_SIZE 44

#define DEBOUNCE 500 //constant debouncing of 2 seconds
#define STREAMING_DEBOUNCE 200

//buzzer constants
#define NOTE_DURATION 100
//Music notes
#define NOTE_D2  73
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
