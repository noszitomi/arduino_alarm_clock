# arduino_alarm_clock


## Components:
* Arduino UNO microcontroller board
* RTC module, I2C interface
* I2C to LCD Display interface module
* LCD display, 16x2 characters
* 4 Push-buttons
* 4 resistors for pulling up pushbuttons output to +5V (3.3 kOhm)
* Buzzer
* 1 series resistor for buzzer (100 Ohm)
* LED
* 1 series resistor for LED (330 Ohm)
* Battery pack 12V (8 x 1.5V batteries)
* Lot of wire

## Features

### Time setting:

“Time set” button (blue) should be pressed long (on the display the word “SET” appears in front of the time). Then with down (black) or up (grey) buttons the time can be set by increasing or decreasing the minutes. By pressing the up or down buttons shortly, the minutes change by one minute each press. By pressing the buttons long the minutes change by 10 minutes. By pressing the time set button again, it saves the time.

### Alarm time setting:

Alarm set button (red) should be pressed long (on the display the word “SET” appears in front of the alarm time). Then with the down (balck) or up (grey) buttons the alarm time can be set by increasing or decreasing the minutes. By pressing the up or down buttons shortly the minutes change by one minute each press. By pressing the button long, the minutes change by 10 minutes.  By pressing the alarm set button again it saves the alarm time. And turns the alarm on automatically. The set alarm time is not stored in the memory of the RTC module. After reboot it re-initializes to the current time. 

### Alarm ON/OFF:

By pressing the alarm set button shortly the alarm can be switched on or off. When it is on in the bottom right corner the word “ON” appears. When you set a new alarm time, it is automatically turned on.

### When the alarm goes off:

When the alarm goes off the buzzer starts “beeping” and a LED lights up, for 3 minutes long. It can be turned off any time by pressing the alarm set button. (Or back again by pressing it one more time).

### What happens if the batteries in the power supply die:

Since there a separate RTC module is used, as long as the battery in the RTC is not dead, it stores the time in its memory.

Please check the attached video as well!

## Software Description

Uses general Arduino code structure.

### Setup() function:
* setting pin modes
* initialize RTC
* load initial time from RTC

### Loop() function:
* Collecting button event(s): short or long-press. Uses getButtonEvent()
* Feeds state machine with the collected event
* 3 run-state: Normal, Time setting, Alarm time setting
* State-machine functions:_process_NORMAL(), process_TIME(), process_ALARM()
* Displays Time (read from RTC) and Alarm time
* Drives outputs (LED and buzzer) if the current time and alarm time match (3 minutes long)

### Used special libraries:
* Newliquidcrystal_1.3.5 (I2C driven LCD display)
* ds3231-master (RTC driver)

## *[This project was originally created in 2018]*
