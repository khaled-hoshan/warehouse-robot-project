/*
 * ===================================================================
 * Warehouse Robot - Module 3: Location ID (Simple & Robust Version)
 * This code is for the ATmega16 inside the 'MODULE3_LOCATION' subcircuit.
 * MCU: ATmega16
 * Clock: 8MHz
 * ===================================================================
 */

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>

// --- Pin Definitions --- //
// Handshake Interface (PORTA)
#define ENABLE_IN_PIN          PINA0 // Reads "Start Scanning" from M5
#define FOUND_OUT_PIN          PA1   // Sends "I Found It!" to M5

// Location ID Inputs (PORTB)
#define ID_PORT_IN             PINB // We will read the lower 4 bits of PORTB

// Feedback LED (PORTC)
#define LED_OUT_PIN            PC0

// --- Configuration --- //
// Let's define "Shelf A" as the binary code 1010 (Decimal 10)
// The robot's sensors read: Bit3=1, Bit2=0, Bit1=1, Bit0=0
#define TARGET_LOCATION_CODE   0b00001010

// --- Main Program --- //
int main(void) {
    // --- I/O Initialization ---
    DDRA = (1 << FOUND_OUT_PIN);  // PA1 is output, PA0 is input
    DDRB = 0x00;                  // All of PORTB is input
    DDRC = (1 << LED_OUT_PIN);    // PC0 is output for the LED

    while (1) {
        // First, check if Module 5 has enabled us to scan.
        if (PINA & (1 << ENABLE_IN_PIN)) {
            // --- Scanning is Active ---

            // Read the 4 bits from the ID pins. We only care about the lower 4 bits.
            uint8_t currentLocationCode = ID_PORT_IN & 0x0F; // Mask to get only PB0-PB3

            // Compare the code we see with our target code
            if (currentLocationCode == TARGET_LOCATION_CODE) {
                // MATCH FOUND!
                // 1. Report success to Module 5 by setting the pin HIGH.
                PORTA |= (1 << FOUND_OUT_PIN);
                // 2. Turn on the local feedback LED.
                PORTC |= (1 << LED_OUT_PIN);
            } else {
                // This is not the target location.
                // 1. Ensure our success signal is LOW.
                PORTA &= ~(1 << FOUND_OUT_PIN);
                // 2. Turn off the LED.
                PORTC &= ~(1 << LED_OUT_PIN);
            }

        } else {
            // --- Scanning is Inactive ---
            // Module 5 told us to stop scanning, so we must reset our signals.
            PORTA &= ~(1 << FOUND_OUT_PIN); // Set FOUND signal LOW
            PORTC &= ~(1 << LED_OUT_PIN);   // Turn off LED
        }

        _delay_ms(50); // Check the inputs every 50ms
    }
}

