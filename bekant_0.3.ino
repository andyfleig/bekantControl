/*
 * Copyright 2017 Andreas Fleig (andy DOT fleig AT gmail DOT com)
 *
 * All rights reserved.
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <LiquidCrystal.h>
#include "IRremote.h"
#include <NewPing.h>

// for lcd:
int backLight = 13;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// for ultra-sonic-sensor:
int trigger=7;
int echo=6;
long height=0;
long old_height = 0;
int max_distance = 300;
NewPing sonar(trigger, echo, max_distance);

// for bekant controlling:
int timer_1to150 = 0;
boolean changed = false;
boolean upTo1 = false;
boolean downTo2 = false;
int up_position = 115;
int down_position = 72;

// for ir-control:
int receiver = 10;
bool pressed = false;
IRrecv irrecv(receiver);
decode_results results;

void setup() {
  // init lcd:
  lcd.begin(16,2);
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, LOW);

  // init ultra-sonic-sensor:
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);


  // start ir-receiver
  irrecv.enableIRIn();
}

void loop() {

  // LCD/height part:
  showHeight();

  // respond to IR-Signal:
  changeHeight();
}

void showHeight() {
  lcd.setCursor(0, 0);
  lcd.print("Height:");

  digitalWrite(trigger, LOW);
  delay(5);
  digitalWrite(trigger, HIGH);
  delay(10);
  digitalWrite(trigger, LOW);
  // measure height:
  unsigned int uS = sonar.ping();
  old_height = height;
  // calculate height from sonar-time:
  // add 60cm because ultra-sonic-sensor is 60cm above the ground
  height = uS / US_ROUNDTRIP_CM + 60;
  if (height >= max_distance || height <= 0) {
    // if measured value is invalid
    // do nothing
  } else {
    // if value has changed more than 1 or ir-button is pressed
    if (old_height + 1 < height || old_height - 1 > height || pressed) {
      changed = true;
    } else {
      height = old_height;
      changed = false;
    }
    lcd.setCursor(0, 1);
    lcd.print(height);
    lcd.print(" cm     ");
  }
  // delay 150ms so we do not run into any problems caused by too fast software for ir-receiver
  delay(150);
  if (changed) {
    // "timer" needed for disabling lcd-background (it is just a counter which counts in ~150ms (because of the delay) steps)
    timer_1to150 = 0;
    // enable lcd-background:
    digitalWrite(backLight, LOW);
  } else {
    // "reset" timer from time to time to avoid overflow
    if (timer_1to150 > 200) {
      timer_1to150 = 21;
    } else {
      timer_1to150++;
    }
  }
  if (timer_1to150 > 120) {
    // disable lcd-background:
    digitalWrite(backLight, HIGH);
  }
}

/*
* Change height of bekant-table controled by ir-remote:
* Button UP: move table up
* Button DOWN: move table down
* Button 1: move to top-position
* Button 2: move to down-position
* Button STOP: stop current movement of the table
*
*
*
*/
void changeHeight() {
  if ((upTo1 && height < up_position) || (downTo2 && height > down_position)) {
    // do nothing
  } else {
    pressed = false;
    upTo1 = false;
    downTo2 = false;
  }
  if (irrecv.decode(&results)) {
    if (results.value == 0xFF906F) {
      // UP:
      digitalWrite(8, HIGH);
      digitalWrite(9, LOW);
      pressed = true;
    } else if (results.value == 0xFFE01F) {
      // DOWN:
      digitalWrite(8, LOW);
      digitalWrite(9, HIGH);
      pressed = true;
    } else  if (results.value == 0xFFFFFFFF) {
      // REPEAT:
      pressed = true;
    } else if (results.value == 0xFFE21D) {
       // STOP:
       pressed = false;
       upTo1 = false;
       downTo2 = false;
    } else if (results.value == 0xFF30CF) {
      // 1:
      upTo1 = true;
      digitalWrite(8, HIGH);
      digitalWrite(9, LOW);
    } else if (results.value == 0xFF18E7) {
      // 2:
      downTo2 = true;
      digitalWrite(8, LOW);
      digitalWrite(9, HIGH);
    } else {
      pressed = false;
    }
    irrecv.resume();
  }
  if (!pressed) {
    // stop table if no ir-button is pressed:
    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
  }
}
