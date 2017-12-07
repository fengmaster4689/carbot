# carbot

This was created as the Final Project in our freshmen EE class. The main features for the carbot is being capable of detecting and following objects in front of it, as well as switching to manual control mode where the bot is controlled with a simple controller.

Automatic Follow:
utilizes 2 ultrasonic sensors in the front to detect an object's distance away from it, displaying the measurements onto the LCD screen, then following the object depending on which sensor detects it.

Manual Control:
a seperate breadboard is connected to the mainboard via a long wire, which connects to an analog pin on the arduino. The breadboard has 4 buttons for forward, backward, left, and right. Each button has a different resistor connected to it, causing different voltage when it is pressed. The arduino then reads the detected voltage and commands the motor driver to respond. 

Schematics:
![CarBot](https://github.com/fengmaster4689/carbot/blob/master/carbot.PNG)
Parts Used: Arduino Uno, 2x 9V-Battery, 2 Ultrasonic Sensor, LCD Screen, Motor Driver, 4 DC mtoor, Potentiometer, 4x buttons, and multiple resistors.

[![CarBot](https://github.com/MichaelJWelsh/carbot/blob/master/youtube.png)](https://www.youtube.com/watch?v=_SSJAW3uo78 "CarBot")
