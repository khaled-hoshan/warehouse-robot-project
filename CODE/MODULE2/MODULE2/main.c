/*
 * ===================================================================
 * Warehouse Robot - Module 2: Obstacle Detection
 * This code is for the ATmega16 inside the 'MODULE2_OBSTACLE_DETECTION' subcircuit.
 * It uses Timer1 Input Capture to measure the HC-SR04 echo pulse.
 * MCU: ATmega16
 * Clock: 8MHz
 * ===================================================================
 */

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// --- Pin Definitions --- //
#define ENABLE_IN_PIN          PINA0
#define OBSTACLE_FOUND_PIN     PA1

#define TRIG_PIN               PD0
#define ECHO_PIN               PD6 // Must be ICP1 pin

// --- Configuration --- //
#define OBSTACLE_THRESHOLD_CM  20 // If object is closer than this, send alert

// --- Global variables for the interrupt ---
volatile uint16_t timer_start = 0;
volatile uint16_t timer_end = 0;
volatile uint8_t measurement_ready = 0;

// --- Main Program --- //
int main(void) {
    uint32_t pulse_duration = 0;
    uint16_t distance_cm = 0;

    // --- I/O Initialization ---
    DDRA = (1 << OBSTACLE_FOUND_PIN); // PA1 is output, PA0 is input
    DDRD = (1 << TRIG_PIN);           // PD0 is output, PD6 is input

    // --- Timer1 Initialization for Input Capture ---
    TCCR1A = 0; // Normal Timer1 mode
    // Set prescaler to 8 (so timer ticks every 1us) and set initial edge to RISING
    TCCR1B = (1 << ICES1) | (1 << CS11); 
    TIMSK = (1 << TICIE1); // Enable Timer1 Input Capture Interrupt

    sei(); // Enable global interrupts

    while (1) {
        if (PINA & (1 << ENABLE_IN_PIN)) {
            // --- Module is ENABLED ---
            
            // 1. Send a 10us trigger pulse
            PORTD |= (1 << TRIG_PIN);
            _delay_us(10);
            PORTD &= ~(1 << TRIG_PIN);

            // 2. Wait for the ISR to tell us a new measurement is ready
            if (measurement_ready) {
                // Calculate duration in microseconds (since prescaler is 8, 1 tick = 1us)
                if (timer_end > timer_start) {
                    pulse_duration = timer_end - timer_start;
                } else { // Handle timer overflow
                    pulse_duration = (65535 - timer_start) + timer_end;
                }
                
                // 3. Calculate distance
                // Speed of sound is 343 m/s or 34300 cm/s.
                // Time for 1cm = 1 / 34300 = 29.15 us.
                // Round trip time for 1cm = 58.3 us.
                // Distance (cm) = Pulse Duration (us) / 58.3
                distance_cm = pulse_duration / 58;

                // 4. Compare to threshold and set output pin
                if (distance_cm < OBSTACLE_THRESHOLD_CM && distance_cm > 0) {
                    PORTA |= (1 << OBSTACLE_FOUND_PIN); // Obstacle FOUND!
                } else {
                    PORTA &= ~(1 << OBSTACLE_FOUND_PIN); // No obstacle
                }
                
                measurement_ready = 0; // Reset flag for next measurement
            }
        } else {
            // --- Module is DISABLED ---
            PORTA &= ~(1 << OBSTACLE_FOUND_PIN); // Ensure alert is OFF
        }
        _delay_ms(100); // Wait 100ms between pings
    }
}

// --- Interrupt Service Routine for Timer1 Input Capture ---
ISR(TIMER1_CAPT_vect) {
    // Check if we were waiting for a RISING edge
    if (TCCR1B & (1 << ICES1)) {
        timer_start = ICR1; // Save the timer value when the pulse started
        TCCR1B &= ~(1 << ICES1); // Switch to detect FALLING edge next
    } else { // We were waiting for a FALLING edge
        timer_end = ICR1; // Save the timer value when the pulse ended
        TCCR1B |= (1 << ICES1);  // Switch back to detect RISING edge for the next cycle
        measurement_ready = 1;   // Set flag to tell main loop data is ready
    }
}

