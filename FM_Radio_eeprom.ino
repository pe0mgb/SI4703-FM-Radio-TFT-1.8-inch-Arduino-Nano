
// FM-Radio with SI4703
// Version 1.1
// Date: 2022-03-17
// Author: Gert PE0MGB
//
// Based on all the work done by Matthias Hertel, http://www.mathertel.de
// More documentation and source code is available at http://www.mathertel.de/Arduino
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// NO WARRANTY OF ANY KIND IS PROVIDED
//
/*                                                   Green Tab         Red Tab      Rotary Encoder
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | Nano       |    SI4703       | Rotary Encoder |       128*160  |      128*160   |     Button     |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | 5V         |                 |                |       5V       |       5V       |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | 3V3        |       3V3       |                |       LED      |       LED      |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | GND        |       GND       |       2        |       GND      |       GND      |       2        |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | A0         |                 |                |    CS via 1K   |     CS via 1K  |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | A1         |                 |                |                |                |       1        |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | A2         |                 |       3        |                |                |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | A3         |                 |       1        |                |                |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | A4 - SDA   |      SDIO       |                |                |                |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | A5 - SCL   |      SCLK       |                |                |                |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | D2         |      RST        |                |                |                |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | D4         |                 |                |                |                |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | D7         |                 |                |   RST via 1K   |     RST via 1K |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | D10        |                 |                |    A0 via 1K   |     D/C via 1K |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | D11        |                 |                |   SDA via 1K   |     Din via 1K |                |
  |------------|-----------------|----------------|----------------|----------------|----------------|
  | D13        |                 |                |   SCK via !k   |     CLK via 1K |                |
  |------------|-----------------|---=------------|----------------|----------------|----------------|
*/

#include <Wire.h>
#include <radio.h>
#include <SPI.h>
#include <TFT_ST7735.h>
#include <RotaryEncoder.h>
#include <RDSParser.h>
#include <OneButton.h>
#include <si4703.h>
#include <EEPROM.h>

int eeAddress0 = 0;   //Location frequency data
int eeAddress1 = 2;   //Location volume data
int eeAddress2 = 4;   //Location menu step data
int eeAddress3 = 6;   //Location preset data
int eeAddress4 = 8;   //Location to check for blanc EEprom.
int BlancCheck;
int fprom;
int volprom;
int oldpush = 1;
int oldi_sidx;
int Rssiold = 0;
int vol = 0;  // volume level
int pos = 0;
int newPos;
int push = 0;  // button starts at volume
int firsttimeafterpush = 0;
int freq;
int meteryRssi = 100;  //offset y-ax
int meteryVol  = 100;  //offset y-ax
char s[12];


TFT_ST7735 tft = TFT_ST7735();  // Invoke library, pins defined in User_Setup

// Setup a RotaryEncoder for pins A2 and A3:
RotaryEncoder encoder(A2, A3);

// Setup the Button of the RotaryEncoder on pin A1.
OneButton button(A1, true);

// Define some stations available at your locations here:
// like 89.40 MHz as 8940

RADIO_FREQ preset[] = {
  8930,  // 00 West
  9890,  // 01 NPO R1
  9260,  // 02 NPO R2
  9680,  // 03 NPO R3-FM
  9470,  // 04 NPO R4
  9340,  // 05 RIJNMOND
  9050,  // 06 SUBLIME
  9130,  // 07 BNR
  9520,  // 08 SLAM
  9620,  // 09 ZFM
  9760,  // 10 DECIBEL
  10040, // 11 QMUSIC
  10150, // 12 SKYRADIO
  10270, // 13 RADIO 538
  10320, // 14 VERONICA
  10380, // 15 RADIO 10
  10460, // 16 100% NL
  9220   // 17 L-FM
};
int    i_sidx;
int    i_max = 17;        // Last preset station number

SI4703   radio;    // Create an instance of a SI4703 chip radio.

// get a RDS parser
RDSParser rds;

// Update the Frequency on the display.
void DisplayFrequency(RADIO_FREQ f)  { //DisplayFrequency  ================================
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  tft.fillRect(5, 40, 150, 16, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.println(s);
} // End DisplayFrequency ==============================================================


// Update RDS on the display.
void DisplayRDS(char *name) {  // DisplayRDS ==============================================
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  bool found = false;
  for (uint8_t n = 0; n < 8; n++)
    if (name[n] != ' ') found = true;
  if (found) {
    tft.setTextSize(2);
    tft.setCursor(0, 40);
    tft.fillRect(5, 40, 150, 16, TFT_BLACK);
    tft.setCursor(35, 40);
    tft.println(name);
    tft.setTextSize(1);
  }
} // END DisplayRDS =======================================================================


void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}

void Drawbox() { //DrawBox  ===============================================================
  tft.fillRect(157, 0,  2, 60, TFT_RED);
  tft.fillRect(  0, 0,  2, 60, TFT_RED);
  tft.fillRect(  0, 31, 159, 2, TFT_RED);
  tft.fillRect(  0, 0, 159, 2, TFT_RED);
  tft.fillRect(  0, 60, 159, 2, TFT_RED);
  tft.fillRect( 55, 80, 50, 2, TFT_RED);
  tft.fillRect( 55, 60,  2, 20, TFT_RED);
  tft.fillRect(103, 60,  2, 20, TFT_RED);
}// End DrawBox ===========================================================================

void SMeterdisplay()  { //SMeterdisplay  dBuV =============================================
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  RADIO_INFO ri;
  radio.getRadioInfo(&ri);
  if (((ri.rssi - Rssiold) >= 4) or ((ri.rssi - Rssiold) <= -4) or (firsttimeafterpush == 0))   {
    Rssiold = ri.rssi;
    tft.fillRect(0, meteryRssi, 160, 24, TFT_BLACK);
    for (int i = 0; i < 80; i = i + 10) {
      tft.fillRect(((i * 2.2) + 1), meteryRssi + 10, 2, 10, TFT_GREEN);
    }
    tft.setTextSize(1);
    tft.setCursor(0, meteryRssi);
    tft.print("0");
    tft.setCursor(62, meteryVol - 10);
    tft.print("      ");
    tft.setCursor(70, meteryRssi - 10);
    tft.print("dBuV  ");
    for (int i = 10; i < 70; i = i + 10) {
      tft.setCursor(i * 2.2 - 4, meteryRssi);
      tft.print(i);
    }
    tft.fillRect(1, meteryRssi + 20, (ri.rssi * 2.2 + 3), 4, TFT_RED);
    tft.fillRect( ((ri.rssi * 2.2) + 3), meteryRssi + 20, (155 - (ri.rssi * 2.2) ), 4, TFT_GREEN);
  }
}// SMeterdisplay==========================================================================


void volumemeter()  { //Volumemeter  ======================================================
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.fillRect(0, meteryVol, 160, 24, TFT_BLACK);
  for (int i = 0; i < 90; i = i + 10) {
    tft.fillRect(((i * (1.875)) + 1), meteryVol + 10, 2, 10, TFT_GREEN);
  }
  tft.setTextSize(1);
  tft.setCursor(0, meteryVol);
  tft.print("0");
  tft.setCursor(62, meteryVol - 10);
  tft.print("Volume");
  for (int i = 10; i < 90; i = i + 10) {
    tft.setCursor((i * (1.875)) - 4, meteryVol);
    tft.print(i / 5);
  }
  tft.fillRect(1, meteryVol + 20, (vol * (9.375) + 1), 4, TFT_RED);
  tft.fillRect((vol * (9.375) + 2), meteryVol + 20, (150 - (vol * (9.375) - 2) ), 4, TFT_GREEN);
} // End Volumemeter  =====================================================================

void StereoMono() {  //StereoMono  =========================================================
  RADIO_INFO ri;
  radio.getRadioInfo(&ri);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(62, 68);
  tft.print("      ");
  tft.setCursor(62, 68);
  tft.println(ri.stereo ? "Stereo" : " Mono");
}  // End StereoMono  =====================================================================

void pushVolume() { //  ===================================================================
  if (firsttimeafterpush == 0) {
    tft.setCursor(20, 78);
    tft.print("  ");
    firsttimeafterpush = 1;
    tft.setTextSize(1);
    tft.setCursor(5, 68);
    tft.print("       ");
    tft.setCursor(5, 68);
    tft.print("Volume");
    volumemeter();
  }//if
  else {
    button.tick();
    newPos = encoder.getPosition();
    if (pos != newPos) {
      if (newPos > pos ) {
        // increase volume
        vol = radio.getVolume();
        if (vol < 15) radio.setVolume(++vol);
      } else if (newPos < pos ) {
        // decrease volume
        vol = radio.getVolume();
        if (vol > 0) radio.setVolume(--vol);
      }
      pos = newPos;
      volumemeter();
    }//if
  }//else
}// End pushVolume ========================================================================

void pushTune() { //========================================================================
  if (firsttimeafterpush == 0) {
    tft.setCursor(20, 78);
    tft.print("  ");
    tft.setTextSize(1);
    tft.setCursor(5, 68);
    tft.print("       ");
    tft.setCursor(5, 68);
    tft.print("Tune");
    freq = radio.getFrequency();
    SMeterdisplay();
    firsttimeafterpush = 1;
  }//if
  else {
    tft.setTextSize(1);
    button.tick();
    newPos = encoder.getPosition();

    if (pos != newPos) {
      if (newPos > pos ) {
        // Tune up
        if (radio.getMaxFrequency() > radio.getFrequency()) {
          freq = freq + 10;
          radio.setFrequency(freq);
        }
        else {
          newPos = newPos - 1;
          encoder.setPosition(newPos);
        }
      } else if (newPos < pos ) {
        // Tune down
        if (radio.getMinFrequency() < radio.getFrequency()) {
          freq = freq - 10;
          radio.setFrequency(freq);
        }
        else {
          newPos = newPos + 1;
          encoder.setPosition(newPos);
        }
      } // if
    }
    pos = newPos;
  }//else
}// End pushTune  ==============================================================================

void pushPreset() {   //========================================================================
  if (firsttimeafterpush == 0) {
    tft.setCursor(20, 78);
    tft.print("  ");
    tft.setTextSize(1);
    tft.setCursor(5, 68);
    tft.print("       ");
    tft.setCursor(5, 68);
    tft.print("Preset");
    tft.setCursor(20, 78);
    tft.print(i_sidx);
    radio.setFrequency(preset[i_sidx]);
    SMeterdisplay();
    firsttimeafterpush = 1;
  }//if
  else {
    tft.setTextSize(1);
    button.tick();
    newPos = encoder.getPosition();
    if (pos != newPos) {
      if (newPos > pos ) {
        // next preset
        if (i_sidx < i_max) {
          i_sidx++;
        }
        else  i_sidx = 0;
        radio.setFrequency(preset[i_sidx]);

      } else if (newPos < pos ) {
        // previous preset
        if (i_sidx > 0) {
          i_sidx--;
          radio.setFrequency(preset[i_sidx]);
        } // if
        else {
          if (i_sidx == 0) {
            i_sidx = i_max;
          }
        }
        radio.setFrequency(preset[i_sidx]);
      }
      tft.setCursor(20, 78);
      tft.print("  ");
      tft.setCursor(20, 78);
      tft.print(i_sidx);
      pos = newPos;
    }
  }//else
}//End pushPreset  ==========================================================================

void pushScan() {   //========================================================================
  if (firsttimeafterpush == 0) {
    tft.setCursor(20, 78);
    tft.print("  ");
    firsttimeafterpush = 1;
    tft.setTextSize(1);
    tft.setCursor(5, 68);
    tft.print("       ");
    tft.setCursor(5, 68);
    tft.print("Scan");
    freq = radio.getFrequency();
    SMeterdisplay();
  }//if
  else {
    tft.setTextSize(1);
    button.tick();
    newPos = encoder.getPosition();
    if (pos != newPos) {
      if (newPos > pos ) {
        radio.seekUp(false);
      }
      else if (newPos < pos ) {
        radio.seekDown(false);
      } // else
    } //if
    pos = newPos;
  } //else
} //End pushScan  ========================================================================



void click() { //After push button  ======================================================
  push++;
  firsttimeafterpush = 0;
  if  (push >= 5) {
    push = 1;
  }
}  // =====================================================================================

// The Interrupt Service Routine for Pin Change Interrupt 1
// This routine will only be called on any signal change on A2 and A3: exactly where we need to check.
ISR(PCINT1_vect) {
  encoder.tick(); // just call tick() to check the state.
}

//  Setup =================================================================================
void setup() {
  // open the Serial port
  Serial.begin(115200);
  //Serial.println("Radio SI4703");
  delay(500);

  //Setup display
  tft.init();
  tft.setRotation(3);
  //tft.setRotation(1); // Rotate 90
  //tft.setRotation(2); // Rotate 180
  //tft.setRotation(3); // Rotate 270
  tft.fillScreen(TFT_BLACK);

  // Initialize the Radio
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable();

  EEPROM.get(eeAddress4, BlancCheck );

  if (BlancCheck == 778) {
    EEPROM.get(eeAddress0, fprom);// load last used frequency from EEPROM
    EEPROM.get(eeAddress1, vol);// load last used volume from EEPROM
    EEPROM.get(eeAddress2, push);// load last used menu step from EEPROM
    EEPROM.get(eeAddress3, i_sidx);// load last used preset step from EEPROM
    delay(100);
  } else { // first run with blanc / new eeprom (new Nano)
    fprom = 8750;
    vol = 2;
    push = 1;
    i_sidx = 0;
    EEPROM.put(eeAddress4, 778);
  }

  radio.setBandFrequency(RADIO_BAND_FM, fprom);
  delay(100);

  radio.setVolume(vol);

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(DisplayRDS);

  // You may have to modify the next 2 lines if using other pins than A2 and A3
  PCICR |= (1 << PCIE1);    // This enables Pin Change Interrupt 1 that covers the Analog input pins or Port C.
  PCMSK1 |= (1 << PCINT10) | (1 << PCINT11);  // This enables the interrupt for pin 2 and 3 of Port C.

  // Openingsscreen
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_YELLOW, TFT_RED);
  tft.setTextSize(2);
  tft.setCursor(20, 35);
  tft.println("FM - Radio");
  tft.setCursor(45, 65);
  tft.println("PE0MGB");
  delay(3000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  // Draw lines and other fixed info
  Drawbox();

  // setup button
  button.attachClick(click);

} // End Setup ================================================================================

void loop() {  //==============================================================================
  button.tick();
  unsigned long now = millis();
  static unsigned long nextFreqTime = 0;
  static unsigned long stereoInfoTime = 0;
  static unsigned long putEepromTime = 0;

  // some internal static values for parsing the input
  static RADIO_FREQ lastf = 0;
  RADIO_FREQ f = 0;

  switch (push)
  {
    case 1: pushVolume(); break;
    case 2: pushPreset(); break;
    case 3: pushTune(); break;
    case 4: pushScan(); break;
    default: break;
  }

  // check for RDS data
  radio.checkRDS();

  // update the display for stereo info
  if (now > stereoInfoTime) {
    StereoMono();
    if (push != 1) SMeterdisplay();
    stereoInfoTime = now + 1200;
  } ///if

  // update the display from time to time
  if (now > nextFreqTime) {
    f = radio.getFrequency();
    if (f != lastf) {
      radio.clearRDS  ();
      // print current tuned frequency
      DisplayFrequency(f);
      lastf = f;
    } // if
    nextFreqTime = now + 400;
  } // if

  if (now > putEepromTime) {
    f = radio.getFrequency();
    if (f != fprom) {
      EEPROM.put(eeAddress0, f); // Save last used frequency in Eeprom
      delay(100);
      fprom = f;
    } // if

    vol = radio.getVolume();
    if (vol != volprom) {
      EEPROM.put(eeAddress1, vol); // Save last used volume in Eeprom
      delay(100);
      volprom = vol;
    } // if

    if (push != oldpush) {
      EEPROM.put(eeAddress2, push); // Save last used menu step in Eeprom
      delay(100);
      oldpush = push;
    } // if

    if (i_sidx != oldi_sidx) {
      EEPROM.put(eeAddress3, i_sidx); // Save last used preset step in Eeprom
      delay(100);
      oldi_sidx = i_sidx;
    } // if

    putEepromTime = now + 5000;
  } //if

} // End loop
// End.
