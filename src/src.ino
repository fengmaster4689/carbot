#include <string.h>
#include <ctype.h>
#include <math.h>

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

/* ========================================================================================================
 *
 *                                                GLOBAL VARIABLES
 *
 * ======================================================================================================== */

/* Ultrasonic sensor pins. */
const int LEFT_TRIG_PIN = 7;
const int LEFT_ECHO_PIN = 6;
const int RIGHT_TRIG_PIN = 9;
const int RIGHT_ECHO_PIN = 8;

/* Motor pins. Note that both the front and back wheels on either side are connected to the SAME pins. */
const int LEFT_MOTOR_FIRST_PIN = 2;
const int LEFT_MOTOR_SECOND_PIN = 3;
const int LEFT_MOTOR_SPEED_PIN = 10;
const int RIGHT_MOTOR_FIRST_PIN = 4;
const int RIGHT_MOTOR_SECOND_PIN = 5;
const int RIGHT_MOTOR_SPEED_PIN = 11;

/* LCD I2C bus address. */
const int LCD_I2C_ADDRESS = 0x3F;

/* LCD object used throughout program. */
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 2, 1, 0, 4, 5, 6, 7);

/* Controller mode toggle pin (analog). */
const int CONTROLLER_TOGGLE_PIN = 3;

/* Controller mode read pin (analog). */
const int CONTROLLER_READ_PIN = 2;

/* Used for determining movement requested by user of manual mode. */
typedef enum ControllerAction {
    CONTROLLER_NO_MOVEMENT,
    CONTROLLER_FORWARD,
    CONTROLLER_BACKWARD,
    CONTROLLER_LEFT,
    CONTROLLER_RIGHT
} ControllerAction;

/* ========================================================================================================
 *
 *                                              ULTRASONIC SENSOR I/O
 *
 * ======================================================================================================== */

/* Returns the distance of the closest object in front of the ultrasonic sensor (in centimeters). */
unsigned pulse_left_ultrasonic_sensor(void);
unsigned pulse_right_ultrasonic_sensor(void);

/* Alias for pulse_left/right. */
unsigned write_ultrasonic_sensor(int trig_pin, int echo_pin);

/* ========================================================================================================
 *
 *                                                   MOTOR I/O
 *
 * ======================================================================================================== */

/* Makes the chassis move forwards/backwards for the given duration. */
void move_forward(unsigned milliseconds);
void move_backward(unsigned milliseconds);

/* Rotates the chassis for the given duration. */
void rotate_left(unsigned milliseconds);
void rotate_right(unsigned milliseconds);

/* Alias for rotate_left/right and move_forward/backward. */
void write_motors(unsigned milliseconds, int low_left_pin, int high_left_pin, int low_right_pin, int high_right_pin);

/* ========================================================================================================
 *
 *                                                   LCD I/O
 *
 * ======================================================================================================== */

/* Prints the message out to the LCD screen. 2x16 array of characters possible. */
void print_message(const char *message);

/* Prints out the distance measured by each ultrasonic sensor on a 1 second interval. */
void print_distances(unsigned left_distance, unsigned right_distance);

/* ========================================================================================================
 *
 *                                                CONTROLLER I/O
 *
 * ======================================================================================================== */

/* Checks to see if the controller mode is toggled. */
bool controller_mode_toggled(void);

/* Returns the requested movement by controller if any. */
ControllerAction poll_controller_action(void);

/* ========================================================================================================
 *
 *                                           MAIN SETUP/LOOP FUNCTIONS
 *
 * ======================================================================================================== */

/* The ultrasonic sensors, motors, and LCD require setup. */
void setup(void) {
    pinMode(LEFT_TRIG_PIN, OUTPUT);
    pinMode(LEFT_ECHO_PIN, INPUT);
    pinMode(RIGHT_TRIG_PIN, OUTPUT);
    pinMode(RIGHT_ECHO_PIN, INPUT);

    pinMode(LEFT_MOTOR_FIRST_PIN, OUTPUT);
    pinMode(LEFT_MOTOR_SECOND_PIN, OUTPUT);
    pinMode(LEFT_MOTOR_SPEED_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_FIRST_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_SECOND_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_SPEED_PIN, OUTPUT);

    lcd.begin(16, 2);
    lcd.setBacklightPin(3, POSITIVE);
    lcd.setBacklight(HIGH);
    lcd.home();

    delay(2000);
}

/* Use controller if available, otherwise try to follow object in field of vision. Print distances measured. */
void loop(void) {
    if (controller_mode_toggled()) {
        ControllerAction action = poll_controller_action();

        if (action == CONTROLLER_FORWARD) {
            move_forward(10);
        } else if (action == CONTROLLER_BACKWARD) {
            move_backward(10);
        } else if (action == CONTROLLER_LEFT) {
            rotate_left(10);
        } else if (action == CONTROLLER_RIGHT) {
            rotate_right(10);
        } else {
            print_distances(pulse_left_ultrasonic_sensor(), pulse_right_ultrasonic_sensor());
        }
    } else {
        unsigned left_distance = pulse_left_ultrasonic_sensor();
        unsigned right_distance = pulse_right_ultrasonic_sensor();

        print_distances(left_distance, right_distance);

        if (left_distance < 35 && left_distance > 5 && right_distance < 35 && right_distance > 5) {
            move_forward(100);
        } else if (left_distance < 5 && left_distance > 1 && right_distance < 5 && right_distance > 1) {
            move_backward(100);
        } else if (left_distance > 5) {
            rotate_right(100);
        } else if (right_distance > 5) {
            rotate_left(100);
        }
    }
}

/* ========================================================================================================
 *
 *                                              ULTRASONIC SENSOR I/O
 *
 * ======================================================================================================== */

unsigned pulse_left_ultrasonic_sensor(void) {
    return write_ultrasonic_sensor(LEFT_TRIG_PIN, LEFT_ECHO_PIN);
}

unsigned pulse_right_ultrasonic_sensor(void) {
    return write_ultrasonic_sensor(RIGHT_TRIG_PIN, RIGHT_ECHO_PIN);
}

unsigned write_ultrasonic_sensor(int trig_pin, int echo_pin) {
    /* Essentially this function takes the rolling average over a series of readings to get a more accurate
     * distance measured. These ultrasonic sensors are cheap and old so this is 100% necessary. */
    static const int MAX_SAMPLE_SIZE = 10;
    static const int THRESHOLD = 20;
    static const int BYPASS_POOL_SIZE = 3;

    int current_sample_size = 0;
    int sum = 0;
    for (int j = 0; j < MAX_SAMPLE_SIZE; ++j) {
        /* Filter out unwanted noise by briefly waiting. */
        digitalWrite(trig_pin, LOW);
        delayMicroseconds(10);

        /* Create a 5 microsecond pulse. */
        digitalWrite(trig_pin, HIGH);
        delayMicroseconds(5);
        digitalWrite(trig_pin, LOW);

        /* Speed of sound is 340 m/s, or 0.034 centimeters/microsecond. Pulse has to travel to the object and
         * back, meaning it has to travel twice the distance, so divide by 2. */
        unsigned distance = pulseIn(echo_pin, HIGH, 2000) * 0.034 / 2;

        /* Discard readings outside threshold. */
        if (current_sample_size < BYPASS_POOL_SIZE || abs(distance - sum / current_sample_size) <= THRESHOLD) {
            sum += distance;
            ++current_sample_size;
        }
    }

    return sum / current_sample_size;
}

/* ========================================================================================================
 *
 *                                                   MOTOR I/O
 *
 * ======================================================================================================== */

void move_forward(unsigned milliseconds) {
    write_motors(milliseconds, LEFT_MOTOR_FIRST_PIN, LEFT_MOTOR_SECOND_PIN, RIGHT_MOTOR_FIRST_PIN, RIGHT_MOTOR_SECOND_PIN);
}

void move_backward(unsigned milliseconds) {
    write_motors(milliseconds, LEFT_MOTOR_SECOND_PIN, LEFT_MOTOR_FIRST_PIN, RIGHT_MOTOR_SECOND_PIN, RIGHT_MOTOR_FIRST_PIN);
}

void rotate_left(unsigned milliseconds) {
    write_motors(milliseconds, LEFT_MOTOR_FIRST_PIN, LEFT_MOTOR_SECOND_PIN, RIGHT_MOTOR_SECOND_PIN, RIGHT_MOTOR_FIRST_PIN);
}

void rotate_right(unsigned milliseconds) {
    write_motors(milliseconds, LEFT_MOTOR_SECOND_PIN, LEFT_MOTOR_FIRST_PIN, RIGHT_MOTOR_FIRST_PIN, RIGHT_MOTOR_SECOND_PIN);
}

void write_motors(unsigned milliseconds, int low_left_pin, int high_left_pin, int low_right_pin, int high_right_pin) {
    /* Basically, the direction of electric potential/current determines the direction the wheels spin. */
    digitalWrite(low_left_pin, LOW);
    digitalWrite(high_left_pin, HIGH);
    digitalWrite(low_right_pin, LOW);
    digitalWrite(high_right_pin, HIGH);

    /* Set the speed for the duration. */
    analogWrite(LEFT_MOTOR_SPEED_PIN, 255);
    analogWrite(RIGHT_MOTOR_SPEED_PIN, 255);
    delay(milliseconds);

    /* Set the speed back to 0. */
    analogWrite(LEFT_MOTOR_SPEED_PIN, 0);
    analogWrite(RIGHT_MOTOR_SPEED_PIN, 0);
}

/* ========================================================================================================
 *
 *                                                   LCD I/O
 *
 * ======================================================================================================== */

void print_message(const char *message) {
    lcd.clear();

    /* Print top row, then bottom. */
    for (int i = 0, length = strlen(message); i < length; ++i) {
        if (i == 16) {
            lcd.setCursor(0, 1);
        }
        lcd.print(message[i]);
    }
}

void print_distances(unsigned left_distance, unsigned right_distance) {
    static unsigned long change_in_time = 0;

    if (millis() - change_in_time > 1000) {
        change_in_time = millis();

        char msg[32];
        sprintf(msg, "LEFT:  %6u cmRIGHT: %6u cm", pulse_left_ultrasonic_sensor(), pulse_right_ultrasonic_sensor());
        print_message(msg);
    }
}

/* ========================================================================================================
 *
 *                                                CONTROLLER I/O
 *
 * ======================================================================================================== */

bool controller_mode_toggled(void) {
    return analogRead(CONTROLLER_TOGGLE_PIN) > 900;
}

ControllerAction poll_controller_action(void) {
    int state = analogRead(CONTROLLER_READ_PIN);

    /* These values are based on the potential difference across different resistors on the controller. */
    if (state > 950) {
        return CONTROLLER_FORWARD;
    } else if (state > 850) {
        return CONTROLLER_LEFT;
    } else if (state > 490) {
        return CONTROLLER_BACKWARD;
    } else if (state > 320) {
        return CONTROLLER_RIGHT;
    } else {
        return CONTROLLER_NO_MOVEMENT;
    }
}

