/*
 * ===================================================================
 * Warehouse Robot - Module 1: Line Following & Navigation
 * This code is for the ATmega16 inside the 'MODULE1_LINE_FOLLOWER' subcircuit.
 * MCU: ATmega16
 * Clock: 8MHz
 * ===================================================================
 */

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

// --- Pin Definitions --- //
// Handshake Input (PORTA)
#define ENABLE_MOTORS_PIN       PINA0 // Reads "GO" command from M5

// Sensor Inputs (PORTB)
#define SENSOR_L_PIN            PINB0
#define SENSOR_C_PIN            PINB1
#define SENSOR_R_PIN            PINB2

// Motor Outputs (PORTD)
#define R_MOTOR_A               PD0  // L293D IN1
#define R_MOTOR_B               PD1  // L293D IN2
#define L_MOTOR_A               PD2  // L293D IN3
#define L_MOTOR_B               PD3  // L293D IN4

// --- Helper Functions for Motor Control ---
void go_straight() {
    // Left Motor Forward
    PORTD |= (1 << L_MOTOR_A);
    PORTD &= ~(1 << L_MOTOR_B);
    // Right Motor Forward
    PORTD |= (1 << R_MOTOR_A);
    PORTD &= ~(1 << R_MOTOR_B);
}

void turn_left() {
    // Left Motor Stop
    PORTD &= ~((1 << L_MOTOR_A) | (1 << L_MOTOR_B));
    // Right Motor Forward
    PORTD |= (1 << R_MOTOR_A);
    PORTD &= ~(1 << R_MOTOR_B);
}

void turn_right() {
    // Left Motor Forward
    PORTD |= (1 << L_MOTOR_A);
    PORTD &= ~(1 << L_MOTOR_B);
    // Right Motor Stop
    PORTD &= ~((1 << R_MOTOR_A) | (1 << R_MOTOR_B));
}

void stop_motors() {
    // Stop both motors
    PORTD &= ~((1 << L_MOTOR_A) | (1 << L_MOTOR_B) | (1 << R_MOTOR_A) | (1 << R_MOTOR_B));
}


// --- Main Program --- //
int main(void) {
    // --- I/O Initialization ---
    DDRA = 0x00; // PORTA is all input
    DDRB = 0x00; // PORTB is all input
    DDRD = 0xFF; // PORTD is all output for motors

    while (1) {
        // First, check if Module 5 has enabled us.
        if (PINA & (1 << ENABLE_MOTORS_PIN)) {
            // --- Motors are ENABLED ---
            
            // Read all three sensors at once
            uint8_t left_on =   (PINB & (1 << SENSOR_L_PIN));
            uint8_t center_on = (PINB & (1 << SENSOR_C_PIN));
            uint8_t right_on =  (PINB & (1 << SENSOR_R_PIN));

            // Implement our logic table
            if (center_on) { // 010 condition
                go_straight();
            }
            else if (left_on) { // 100 condition
                turn_left();
            }
            else if (right_on) { // 001 condition
                turn_right();
            }
            else { // 000 condition
                stop_motors();
            }
        } else {
            // --- Motors are DISABLED ---
            stop_motors();
        }
        _delay_ms(10); // A small delay is good practice
    }
}

