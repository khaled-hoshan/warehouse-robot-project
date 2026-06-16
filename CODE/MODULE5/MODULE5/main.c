/*
 * =================================================================================
 * WAREHOUSE ROBOT - MODULE 5: SEQUENCER (DEFINITIVE GOLDEN MASTER VERSION)
 * This is the final version for the main MCU, with all features integrated.
 * - Two-wire handshake for gripper status
 * - Gripper enable pin to prevent startup motion
 * - Dedicated "Waiting for Action" indicator LED
 * MCU: ATmega16
 * Clock: 8MHz
 * =================================================================================
 */

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

// --- Pin Definitions ---//
// Inputs (PORTA)
#define START_BUTTON_PIN       PINA0
#define OBSTACLE_DETECTED_PIN  PINA1
#define LOCATION_ID_PIN        PINA2
#define GRIPPER_IS_OPEN_PIN    PINA3 // Listens for "Open" confirmation from M4
#define GRIPPER_IS_CLOSED_PIN  PINA4 // Listens for "Close" confirmation from M4
#define HOME_ID_PIN            PINA5 // MOVED TO PA5

// Outputs (PORTB)
#define ENABLE_LINE_FOLLOWER_PIN   PB0
#define ENABLE_OBSTACLE_DET_PIN    PB1
#define ENABLE_LOCATION_ID_PIN     PB2
#define COMMAND_GRIPPER_PIN        PB3 // Command TO M4 (1=Close, 0=Open)
#define ENABLE_GRIPPER_PIN         PB4 // Gripper Enable Pin

// Status LEDs (PORTB & PORTD)
#define BUSY_LED_PIN               PB5
#define COMPLETE_LED_PIN           PB6
#define ERROR_LED_PIN              PB7
#define WAITING_FOR_GRIPPER_LED_PIN PD3 // The new "Action Required" LED

// LCD Pins
#define LCD_DATA_PORT PORTC
#define LCD_DATA_DDR  DDRC
#define LCD_CTRL_PORT PORTD
#define LCD_CTRL_DDR  DDRD
#define LCD_RS_PIN    PD0
#define LCD_RW_PIN    PD1
#define LCD_E_PIN     PD2

// --- State Machine Definition --- //
typedef enum {
    IDLE,
    NAVIGATING_TO_SHELF,
    AT_SHELF_STOPPED,
    GRIPPER_OPENING,
    PICKING_ITEM,
    GRIPPER_CLOSING,
    NAVIGATING_TO_HOME,
    AT_HOME_STOPPED,
    RELEASING_ITEM,
    TASK_COMPLETE,
    EMERGENCY_STOP
} RobotState;

// --- Function Prototypes for LCD ---
void lcd_command(unsigned char cmd); void lcd_char(unsigned char data); void lcd_init(); void lcd_string(char* str); void lcd_clear(); void lcd_set_cursor(uint8_t row, uint8_t col);

// --- Main Program --- //
int main(void) {
    // --- I/O Initialization ---
    DDRA = 0x00;  // PORTA is all inputs
    DDRB = 0xFF;  // PORTB is all outputs
    // Set PORTD pins for LCD control and our new LED as outputs
    DDRD = (1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << WAITING_FOR_GRIPPER_LED_PIN);
    
    lcd_init();
    RobotState currentState = IDLE;
    
    while (1) {
        // High-priority obstacle check
        if ((PINA & (1 << OBSTACLE_DETECTED_PIN)) &&
            (currentState == NAVIGATING_TO_SHELF || currentState == NAVIGATING_TO_HOME)) {
            currentState = EMERGENCY_STOP;
        }

        switch (currentState) {
            case IDLE:
                lcd_set_cursor(0, 0); lcd_string("State: IDLE      ");
                lcd_set_cursor(1, 0); lcd_string("Press START...");
                PORTB = 0x00; // Turn off all PORTB signals
                PORTD &= ~(1 << WAITING_FOR_GRIPPER_LED_PIN); // Turn off waiting LED
                if (PINA & (1 << START_BUTTON_PIN)) {
                    _delay_ms(50);
                    if (PINA & (1 << START_BUTTON_PIN)) {
                        currentState = NAVIGATING_TO_SHELF;
                        lcd_clear();
                    }
                }
                break;

            case NAVIGATING_TO_SHELF:
                lcd_set_cursor(0, 0); lcd_string("State: Navigating");
                lcd_set_cursor(1, 0); lcd_string("Target: Shelf A  ");
                PORTB = (1 << ENABLE_LINE_FOLLOWER_PIN) | (1 << ENABLE_OBSTACLE_DET_PIN) | (1 << ENABLE_LOCATION_ID_PIN) | (1 << BUSY_LED_PIN);
                if (PINA & (1 << LOCATION_ID_PIN)) {
                    currentState = AT_SHELF_STOPPED;
                }
                break;
                
            case AT_SHELF_STOPPED:
                lcd_set_cursor(0, 0); lcd_string("State: Arrived   ");
                lcd_set_cursor(1, 0); lcd_string("At Shelf A       ");
                PORTB = (1 << BUSY_LED_PIN); // Line follower off, but still busy
                _delay_ms(1000);
                currentState = GRIPPER_OPENING;
                break;

            case GRIPPER_OPENING:
                lcd_set_cursor(0, 0); lcd_string("State: Gripper   ");
                lcd_set_cursor(1, 0); lcd_string("Opening...       ");
                PORTD |= (1 << WAITING_FOR_GRIPPER_LED_PIN); // Turn ON "Waiting" LED
                // Send OPEN command (LOW) and ENABLE the gripper module
                PORTB = (0 << COMMAND_GRIPPER_PIN) | (1 << ENABLE_GRIPPER_PIN) | (1 << BUSY_LED_PIN);
                if (PINA & (1 << GRIPPER_IS_OPEN_PIN)) { // Wait for dedicated OPEN signal
                    _delay_ms(500); 
                    currentState = PICKING_ITEM;
                }
                break;

            case PICKING_ITEM:
                 PORTD &= ~(1 << WAITING_FOR_GRIPPER_LED_PIN); // Turn OFF "Waiting" LED
                 lcd_set_cursor(0, 0); lcd_string("State: Gripper   ");
                 lcd_set_cursor(1, 0); lcd_string("Picking up item...");
                 _delay_ms(1000); 
                 currentState = GRIPPER_CLOSING;
                 break;

            case GRIPPER_CLOSING:
                lcd_set_cursor(0, 0); lcd_string("State: Gripper   ");
                lcd_set_cursor(1, 0); lcd_string("Closing...       ");
                PORTD |= (1 << WAITING_FOR_GRIPPER_LED_PIN); // Turn ON "Waiting" LED
                // Send CLOSE command (HIGH) and ENABLE the gripper module
                PORTB = (1 << COMMAND_GRIPPER_PIN) | (1 << ENABLE_GRIPPER_PIN) | (1 << BUSY_LED_PIN);
                if (PINA & (1 << GRIPPER_IS_CLOSED_PIN)) { // Wait for dedicated CLOSE signal
                     _delay_ms(500);
                    currentState = NAVIGATING_TO_HOME;
                }
                break;

            case NAVIGATING_TO_HOME:
                PORTD &= ~(1 << WAITING_FOR_GRIPPER_LED_PIN); // Turn OFF "Waiting" LED
                lcd_set_cursor(0, 0); lcd_string("State: Returning ");
                lcd_set_cursor(1, 0); lcd_string("To Home          ");
                PORTB = (1 << ENABLE_LINE_FOLLOWER_PIN) | (1 << ENABLE_OBSTACLE_DET_PIN) | (1 << ENABLE_LOCATION_ID_PIN) | (1 << BUSY_LED_PIN);
                if (PINA & (1 << HOME_ID_PIN)) {
                    currentState = AT_HOME_STOPPED;
                }
                break;

            case AT_HOME_STOPPED:
                lcd_set_cursor(0, 0); lcd_string("State: Arrived   ");
                lcd_set_cursor(1, 0); lcd_string("At Home          ");
                PORTB = (1 << BUSY_LED_PIN);
                _delay_ms(1000);
                currentState = RELEASING_ITEM;
                break;

            case RELEASING_ITEM:
                lcd_set_cursor(0, 0); lcd_string("State: Gripper   ");
                lcd_set_cursor(1, 0); lcd_string("Releasing item...");
                PORTD |= (1 << WAITING_FOR_GRIPPER_LED_PIN); // Turn ON "Waiting" LED
                // Send OPEN command (LOW) and ENABLE the gripper module
                PORTB = (0 << COMMAND_GRIPPER_PIN) | (1 << ENABLE_GRIPPER_PIN) | (1 << BUSY_LED_PIN);
                if (PINA & (1 << GRIPPER_IS_OPEN_PIN)) { // Wait for dedicated OPEN signal
                    currentState = TASK_COMPLETE;
                }
                break;

            case TASK_COMPLETE:
                PORTD &= ~(1 << WAITING_FOR_GRIPPER_LED_PIN); // Turn OFF "Waiting" LED
                lcd_set_cursor(0, 0); lcd_string("State: Complete! ");
                lcd_set_cursor(1, 0); lcd_string("                 ");
                PORTB = (1 << COMPLETE_LED_PIN); // Turn off BUSY, turn on COMPLETE
                _delay_ms(3000); 
                currentState = IDLE;
                lcd_clear();
                break;

            case EMERGENCY_STOP:
                PORTD &= ~(1 << WAITING_FOR_GRIPPER_LED_PIN); // Turn OFF "Waiting" LED
                lcd_set_cursor(0, 0); lcd_string("!!! EMERGENCY !!!");
                lcd_set_cursor(1, 0); lcd_string("Obstacle Detected");
                PORTB = (1 << ERROR_LED_PIN); // All other enables off, ERROR on
                break;
        }
        _delay_ms(10);
    }
}

// --- Paste all the LCD functions here ---
void lcd_command(unsigned char cmd) { LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (cmd & 0xF0); LCD_CTRL_PORT &= ~(1 << LCD_RS_PIN); LCD_CTRL_PORT |= (1 << LCD_E_PIN); _delay_us(1); LCD_CTRL_PORT &= ~(1 << LCD_E_PIN); _delay_us(200); LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (cmd << 4); LCD_CTRL_PORT |= (1 << LCD_E_PIN); _delay_us(1); LCD_CTRL_PORT &= ~(1 << LCD_E_PIN); _delay_ms(2); }
void lcd_char(unsigned char data) { LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data & 0xF0); LCD_CTRL_PORT |= (1 << LCD_RS_PIN); LCD_CTRL_PORT |= (1 << LCD_E_PIN); _delay_us(1); LCD_CTRL_PORT &= ~(1 << LCD_E_PIN); _delay_us(200); LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data << 4); LCD_CTRL_PORT |= (1 << LCD_E_PIN); _delay_us(1); LCD_CTRL_PORT &= ~(1 << LCD_E_PIN); _delay_ms(2); }
void lcd_init() { LCD_DATA_DDR = 0xFF; LCD_CTRL_DDR |= (1 << LCD_E_PIN) | (1 << LCD_RW_PIN) | (1 << LCD_RS_PIN); LCD_CTRL_PORT &= ~(1 << LCD_RW_PIN); _delay_ms(20); lcd_command(0x33); lcd_command(0x32); lcd_command(0x28); lcd_command(0x0C); lcd_command(0x06); lcd_command(0x01); _delay_ms(2); }
void lcd_string(char* str) { for (int i = 0; str[i] != 0; i++) { lcd_char(str[i]); } }
void lcd_clear() { lcd_command(0x01); _delay_ms(2); }
void lcd_set_cursor(uint8_t row, uint8_t col) { uint8_t temp = (row == 0) ? 0x80 + col : 0xC0 + col; lcd_command(temp); }


