/*

Copyright (c) 2015.  All rights reserved.
An Open Source Arduino based OSD project for controlling 5.8 GHz Boscam rx5808 modules.
These are used as FPV ground stations / FPV Goolges

This project has been realsized alread without OSD based on the TVlib.
Please check this project as reference:

http://code.google.com/p/rx5808-pro/

The "look" and feel is the same, but the code has been fully rewriten.

The rewrite took place since the GUI is differently build with the MAX7456.
Beside that the  rx5808-pro implementation had to work with smaller memory footprint since the
frame buffer has been held in the Adruino.

Since no Ardunino frame buffer is required for the MAX7456, the ram
can be used for "cleaner" data and channel handling inside data structures.

Thanks for the great starting point with from the arducam-osd project.

http://code.google.com/p/arducam-osd/wiki/minimosd


Program  : rx5808_pro_osd (MinimOSD [and variants] Firmware)
Version  : V1.0, Sept 1th 2015
Author(s): Marko Hoepken
Coauthor: CHECK

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>

*/

/* ************************************************************ */
/* **************** MAIN PROGRAM - MODULES ******************** */
/* ************************************************************ */

#undef PROGMEM 
#define PROGMEM __attribute__(( section(".progmem.data") )) 

#undef PSTR 
#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];})) 


/* **********************************************/
/* ***************** INCLUDES *******************/

//#define membug 

// AVR Includes
#include <FastSerial.h> // better steam

// Get the common arduino functions
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "wiring.h"
#endif

#ifdef membug
#include <MemoryFree.h>
#endif

// Configurations

#include "ArduCam_Max7456.h"


/* *************************************************/
/* ***************** DEFINITIONS *******************/

//OSD Hardware 
//#define ArduCAM328
#define MinimOSD

#define TELEMETRY_SPEED  57600  // How fast our MAVLink telemetry is coming to Serial port
#define BOOTTIME         2000   // Time in milliseconds that we show boot loading bar and wait user input

// switches
#define KEY_A 0 // RX
#define KEY_B 1 // TX
#define KEY_UP 2
#define KEY_DOWN 1
#define KEY_MID 3

#define RSSIMAX 75 // 75% threshold, when channel is printed in spectrum
#define STATE_SEEK_FOUND 0
#define STATE_SEEK 1
#define STATE_SCAN 2
#define STATE_MANUAL 3

#define START_STATE STATE_SEEK
#define MAX_STATE STATE_MANUAL

#define KEY_DEBOUNCE 10
#define CHANNEL_BAND_SIZE 8
#define CHANNEL_MIN_INDEX 0
#define CHANNEL_MAX_INDEX 31

#define CHANNEL_MAX 31
#define CHANNEL_MIN 0

// Screen settings (use smaller NTSC size)
#define SCEEEN_X_MAX 30
#define SCREEN_Y_MAX 13


// Menu settings
#define MENU_MODE_SELECTION_X 6
#define MENU_MODE_SELECTION_Y 2
#define MENU_MODE_SELECTION_HEADER 3
#define MENU_MODE_SELECTION_ENTRY 4
// band scanner gemetry
#define BAND_SCANNER_SPECTRUM_X_MIN 2
#define BAND_SCANNER_SPRCTRUM_Y_MAX 30
#define BAND_SCANNER_SPECTRUM_Y_MIN 12
#define BAND_SCANNER_SPRCTRUM_Y_MAX 5


// Objects and Serial definitions
FastSerialPort0(Serial); // just for character update
OSD osd; //OSD object 
//SimpleTimer  mavlinkTimer;

uint8_t state = START_STATE;



/* **********************************************/
/* ***************** SETUP() *******************/

void setup() 
{
    unplugSlaves();
    osd.init();
    osd.setPanel(1,10);
    osd.openPanel();
    //osd.printf("%i",freeMem()); 
    //osd.printf_P(PSTR("\x20\x20\x20\x20\x20\x20\xba\xbb\xbc\xbd\xbe|\x20\x20\x20\x20\x20\x20\xca\xcb\xcc\xcd\xce|ArduCAM OSD v2.2"));    
    osd.closePanel();

    //screen_mode_selection();  
    screen_band_scanner();
//    osd.clear();
//    mavlinkTimer.Enable();

} // END of setup();


    int8_t menu=1;
/************************************************/
/*                 MAIN LOOP                    */
/************************************************/
void loop() 
{

    uint8_t key_pressed = get_key();
    
    if(key_pressed == KEY_MID)
    {
        uploadFont(); // will not return
    }    
    if(key_pressed == KEY_UP)
    {
        menu++;
        if(menu > 4)
        {
            menu=1;
        }
    }   
    if(key_pressed == KEY_DOWN)
    {
        menu--;
        if(menu < 1)
        {
            menu=4;
        }        
    }     
    #if 0
    set_cursor
        (
        MENU_MODE_SELECTION_X+1, 
        MENU_MODE_SELECTION_Y + MENU_MODE_SELECTION_HEADER,
        MENU_MODE_SELECTION_ENTRY,
        menu
        );
    #endif
    delay(100); // debounce
    
}


/************************************************/
/*              SUB ROUTINES                    */
/************************************************/

void osd_print (uint8_t x, uint8_t y, char string[30])
{
    osd.setPanel(x-1,y-1);  
    osd.openPanel();
    osd.printf("%s",string); 
    osd.closePanel(); 
}


/*******************/
/*   MODE SCREEN   */
/*******************/
void screen_mode_selection(void)
{
    uint8_t y=MENU_MODE_SELECTION_Y;
    osd_print(MENU_MODE_SELECTION_X,y++,"\x03\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x04");
    osd_print(MENU_MODE_SELECTION_X,y++,"\x02 MODE SELECTION \x02");
    osd_print(MENU_MODE_SELECTION_X,y++,"\x07\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x08");
    osd_print(MENU_MODE_SELECTION_X,y++,"\x02  AUTO SEARCH   \x02");
    osd_print(MENU_MODE_SELECTION_X,y++,"\x02  BAND SCANNER  \x02");
    osd_print(MENU_MODE_SELECTION_X,y++,"\x02  MANUEL MODE   \x02");
    osd_print(MENU_MODE_SELECTION_X,y++,"\x02  SETUP         \x02");
    osd_print(MENU_MODE_SELECTION_X,y++,"\x05\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x06");
}
/*******************/
/*   BAND SCANNER   */
/*******************/
void screen_band_scanner(void)
{

    osd_print(BAND_SCANNER_SPECTRUM_X_MIN,1,"\x03\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x04");
    osd_print(BAND_SCANNER_SPECTRUM_X_MIN,2,"\x02       BAND SCANNER      \x02");
    osd_print(BAND_SCANNER_SPECTRUM_X_MIN,3,"\x05\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x06");
    osd_print(BAND_SCANNER_SPECTRUM_X_MIN,SCREEN_Y_MAX-3,"\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f\x1f");
    osd_print(BAND_SCANNER_SPECTRUM_X_MIN,SCREEN_Y_MAX-2,"\x09\x0d\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0a\x0c\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0b\x0d");    
}



// cursor handling for menue
void set_cursor(uint8_t x_offset, uint8_t y_offset, uint8_t entry, uint8_t pos)
{
    uint8_t y=0;
    for(y=1;
        y<=entry;
        y++)
        {
            uint8_t y_pos=y-1;
            if(pos == y) // set arrow
            {
                osd_print(x_offset,y_pos+y_offset,"\x80");
            }
            else // clear arrow
            {
                 osd_print(x_offset,y_pos+y_offset,"  ");           
            }
        }
}

uint8_t get_key (void)
{   
    uint8_t sw_dir_a2b = 0;
    uint8_t sw_dir_b2a = 0;    
    // try both directions
    // KEY_A -> KEY_B
    pinMode(KEY_A, OUTPUT);
    pinMode(KEY_B, INPUT);
    digitalWrite(KEY_B, INPUT_PULLUP);
    digitalWrite(KEY_A, LOW);
    // check if the LOW will get to port
    if(digitalRead(KEY_B) == 0)
    {
        sw_dir_a2b=1;
    }
    // KEY_B -> KEY_A
    pinMode(KEY_B, OUTPUT);
    pinMode(KEY_A, INPUT);
    digitalWrite(KEY_A, INPUT_PULLUP);
    digitalWrite(KEY_B, LOW);
    // check if the LOW will get to port
    if(digitalRead(KEY_A) == 0)
    {
        sw_dir_b2a=1;
    }    
    // turn off key driver
    pinMode(KEY_A, INPUT);
    digitalWrite(KEY_A, INPUT_PULLUP);    
    pinMode(KEY_B, INPUT);
    digitalWrite(KEY_B, INPUT_PULLUP);
    // check results
    // 0 = no key
    // 1 = Key 1
    // 2 = Key 2
    // 3 = both keys, or bypass key
    if(sw_dir_a2b && sw_dir_b2a)
    {
        return (3);
    }
    else if (sw_dir_a2b)
    {
        return (1);
    }
    else if (sw_dir_b2a)
    {
        return (2);
    }
    else
    {
        return (0);
    }    
}

void unplugSlaves(){
    //Unplug list of SPI

    digitalWrite(MAX7456_SELECT,  HIGH); // unplug OSD
}

void uploadFont()
{
    uint16_t byte_count = 0;
    byte bit_count;
    byte ascii_binary[0x08];

    // move these local to prevent ram usage
    uint8_t character_bitmap[0x40];
    int font_count = 0;

    osd.clear();
    osd.setPanel(1,1);
    osd.openPanel();
    osd.printf_P(PSTR("Update CharSet")); 
    osd.closePanel();
    delay(1000);

    #define TELEMETRY_SPEED  57600 
    Serial.begin(TELEMETRY_SPEED);    
    //Serial.printf_P(PSTR("Ready for Font\n"));
    Serial.println("Ready for Font upload");

    while(font_count < 255) { 
        int8_t incomingByte = Serial.read();
        switch(incomingByte) // parse and decode mcm file
        {
        case 0x0d: // carridge return, end of line
            //Serial.println("cr");
            if (bit_count == 8 && (ascii_binary[0] == 0x30 || ascii_binary[0] == 0x31))
            {
                // turn 8 ascii binary bytes to single byte '01010101' = 0x55
                // fill in 64 bytes of character data
                // made this local to prevent needing a global
                byte ascii_byte;

                ascii_byte = 0;

                if (ascii_binary[0] == 0x31) // ascii '1'
                    ascii_byte = ascii_byte + 128;

                if (ascii_binary[1] == 0x31)
                    ascii_byte = ascii_byte + 64;

                if (ascii_binary[2] == 0x31)
                    ascii_byte = ascii_byte + 32;

                if (ascii_binary[3] == 0x31)
                    ascii_byte = ascii_byte + 16;

                if (ascii_binary[4] == 0x31)
                    ascii_byte = ascii_byte + 8;

                if (ascii_binary[5] == 0x31)
                    ascii_byte = ascii_byte + 4;

                if (ascii_binary[6] == 0x31)
                    ascii_byte = ascii_byte + 2;

                if (ascii_binary[7] == 0x31)
                    ascii_byte = ascii_byte + 1;

                character_bitmap[byte_count] = ascii_byte;
                byte_count++;
                bit_count = 0;
            }
            else
                bit_count = 0;
            break;
        case 0x0a: // line feed, ignore
            //Serial.println("ln");   
            break;
        case 0x30: // ascii '0'
        case 0x31: // ascii '1' 
            ascii_binary[bit_count] = incomingByte;
            bit_count++;
            break;
        default:
            break;
        }

        // we have one completed character
        // write the character to NVM 
        if(byte_count == 64)
        {
            osd.write_NVM(font_count, character_bitmap);    
            byte_count = 0;
            font_count++;
//            Serial.printf_P(PSTR("Char Done\n"));
            Serial.print(font_count, DEC);
            Serial.println(" chars done");
        }
    }
    Serial.println("Font update done, please reboot.");
    while(1); // wait ENDLESS

    //  character_bitmap[]
}



