/*
 * =================================================================================
 * WAREHOUSE ROBOT - MODULE 4: GRIPPER CONTROL (DEFINITIVE - TWO-WIRE STATUS)
 * This version uses two separate status output pins for unambiguous feedback.
 * MCU: ATmega16
 * Clock: 8MHz
 * =================================================================================
 */

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

// --- Pin Definitions --- //
#define LIMIT_SWITCH_OPEN_PIN   PINA0
#define LIMIT_SWITCH_CLOSED_PIN PINA1

#define CMD_IN_PIN              PINB0
#define STATUS_IS_OPEN_PIN      PB1   // Dedicated "I am Open" signal
#define STATUS_IS_CLOSED_PIN    PB2   // Dedicated "I am Closed" signal

#define MOTOR_A_PIN             PD0
#define MOTOR_B_PIN             PD1

int main(void) {
    DDRA = 0x00;
    DDRB = (1 << STATUS_IS_OPEN_PIN) | (1 << STATUS_IS_CLOSED_PIN); // Set both status pins as output
    DDRD = 0xFF;
    
    PORTD = 0x00; // Motor off
    PORTB = 0x00; // All status low

    while (1) {
        // --- Part 1: Motor Control ---
        // This logic just runs the motor based on the command.
        if (PINB & (1 << CMD_IN_PIN)) { // If command is HIGH ("CLOSE")
             PORTD |= (1 << MOTOR_A_PIN);
             PORTD &= ~(1 << MOTOR_B_PIN);
        } else { // If command is LOW ("OPEN")
             PORTD &= ~(1 << MOTOR_A_PIN);
             PORTD |= (1 << MOTOR_B_PIN);
        }

        // --- Part 2: Status Reporting ---
        // This logic continuously reports the state of the buttons. It's separate from motor control.
        if (PINA & (1 << LIMIT_SWITCH_OPEN_PIN)) {
            PORTD = 0x00; // Stop motor
            PORTB |= (1 << STATUS_IS_OPEN_PIN); // Set OPEN status HIGH
        } else {
            PORTB &= ~(1 << STATUS_IS_OPEN_PIN); // Set OPEN status LOW
        }
        
        if (PINA & (1 << LIMIT_SWITCH_CLOSED_PIN)) {
            PORTD = 0x00; // Stop motor
            PORTB |= (1 << STATUS_IS_CLOSED_PIN); // Set CLOSED status HIGH
        } else {
            PORTB &= ~(1 << STATUS_IS_CLOSED_PIN); // Set CLOSED status LOW
        }
        
        _delay_ms(10);
    }
}
