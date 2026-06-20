#define F_CPU 8000000UL
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

// --- Serial Communication ---
#define BAUD 9600
#define MY_UBRR F_CPU/16/BAUD-1

// --- Stepper Motor Pin Definitions ---
#define X_STEPPER_PORT PORTD
#define X_STEPPER_DDR DDRD
#define X_STEPPER_IN1_PIN PD2
#define X_STEPPER_IN2_PIN PD3
#define X_STEPPER_IN3_PIN PD4
#define X_STEPPER_IN4_PIN PD5

#define Y_STEPPER_PORT_D PORTD
#define Y_STEPPER_DDR_D DDRD
#define Y_STEPPER_IN1_PIN PD6
#define Y_STEPPER_IN2_PIN PD7

#define Y_STEPPER_PORT_B PORTB
#define Y_STEPPER_DDR_B DDRB
#define Y_STEPPER_IN3_PIN PB0 
#define Y_STEPPER_IN4_PIN PB1

// --- Servo Pin Definition ---
#define SERVO_PIN PB2

// --- Calibration Constants ---
#define UNIT_STEPS 500
#define MAX_X 1000 // Maximum X coordinate
#define MAX_Y 1000 // Maximum Y coordinate

// --- Stepper sequence (Half-step) ---
const uint8_t stepper_sequence[] = {
    0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001
};

// --- Function Prototypes ---
void USART_Init(unsigned int ubrr);
void USART_Transmit(unsigned char data);
unsigned char USART_Receive(void);
void USART_PrintString(const char *str);
void setup_pins();
void step_motor(int motor_x, int motor_y, int steps_x, int steps_y);
void draw_line(int steps_x, int steps_y);
void timerservo_init(void);
void servo_set_angle(uint32_t angle);
void process_gcode_command(char* command);
void execute_movement(int x_steps, int y_steps);
void draw_diamond_pattern(void);

// --- Global variables ---
int current_x = 0;
int current_y = 0;
int servo_angle = 0;

int main(void) {
    // Initialize peripherals
    USART_Init(MY_UBRR);
    setup_pins();
    timerservo_init();
    servo_set_angle(10); // Start with pen up
    
    // Send ready message
    USART_PrintString("Ready\n");
    
    char buffer[32];
    uint8_t index = 0;
    
    while (1) {
        // Read serial data
        unsigned char received = USART_Receive();
        
        if (received == '\n' || received == '\r') {
            if (index > 0) {
                buffer[index] = '\0'; // Null terminate
                process_gcode_command(buffer);
                index = 0;
            }
        } else if (index < sizeof(buffer) - 1) {
            buffer[index++] = received;
        }
    }
    return 0;
}

// --- Serial Communication Functions ---
void USART_Init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART_Transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

unsigned char USART_Receive(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void USART_PrintString(const char *str) {
    while (*str) {
        USART_Transmit(*str++);
    }
}

// --- G-code Processing ---
void process_gcode_command(char *command) {
    // Simple G-code parser
    if (strcmp(command, "M3") == 0) {
        // Pen down
        servo_set_angle(160);
        servo_angle = 160;
        USART_PrintString("OK\n");
    } else if (strcmp(command, "M5") == 0) {
        // Pen up
        servo_set_angle(10);
        servo_angle = 10;
        USART_PrintString("OK\n");
    } else if (strcmp(command, "G28") == 0) {
        // Home command return to origin
        execute_movement(-current_x, -current_y);
        current_x = 0;
        current_y = 0;
        USART_PrintString("OK\n");
    } else if (strcmp(command, "DIAMOND") == 0) {
        // Draw diamond pattern
        draw_diamond_pattern();
        USART_PrintString("OK\n");
    } else if (strncmp(command, "G1", 2) == 0) {
        // G1 movement command: G1 X100 Y200
        char *ptr = command;
        int x_target = current_x;
        int y_target = current_y;
        
        // Parse coordinates
        while (*ptr) {
            if (*ptr == 'X' || *ptr == 'x') {
                x_target = atoi(ptr + 1);
            } else if (*ptr == 'Y' || *ptr == 'y') {
                y_target = atoi(ptr + 1);
            }
            ptr++;
        }
        
        // Boundary check
        if (x_target > MAX_X) x_target = MAX_X;
        if (x_target < -MAX_X) x_target = -MAX_X;
        if (y_target > MAX_Y) y_target = MAX_Y;
        if (y_target < -MAX_Y) y_target = -MAX_Y;
        
        // Calculate steps to move
        int x_steps = x_target - current_x;
        int y_steps = y_target - current_y;
        
        // Execute movement
        execute_movement(x_steps, y_steps);
        
        // Update current position
        current_x = x_target;
        current_y = y_target;
        USART_PrintString("OK\n");
    } else {
        USART_PrintString("ERR: Unknown command\n");
    }
}

// New function to draw diamond pattern using four corners
void draw_diamond_pattern(void) {
    // South West corner
    execute_movement(-MAX_X, -MAX_Y);
    current_x = -MAX_X;
    current_y = -MAX_Y;
    _delay_ms(500);
    
    // North West corner
    execute_movement(0, MAX_Y * 2);
    current_x = -MAX_X;
    current_y = MAX_Y;
    _delay_ms(500);
    
    // North East corner
    execute_movement(MAX_X * 2, 0);
    current_x = MAX_X;
    current_y = MAX_Y;
    _delay_ms(500);
    
    // South East corner
    execute_movement(0, -MAX_Y * 2);
    current_x = MAX_X;
    current_y = -MAX_Y;
    _delay_ms(500);
    
    // Back to South West to complete diamond
    execute_movement(-MAX_X * 2, 0);
    current_x = -MAX_X;
    current_y = -MAX_Y;
    _delay_ms(500);
    
    // Return to center
    execute_movement(MAX_X, MAX_Y);
    current_x = 0;
    current_y = 0;
}

void execute_movement(int x_steps, int y_steps) {
    if (x_steps != 0 || y_steps != 0) {
        draw_line(x_steps, y_steps);
        _delay_ms(100);
    }
}

// --- Hardware Control Functions ---
void setup_pins() {
    // Stepper pins
    X_STEPPER_DDR |= (1 << X_STEPPER_IN1_PIN) | (1 << X_STEPPER_IN2_PIN) |
                     (1 << X_STEPPER_IN3_PIN) | (1 << X_STEPPER_IN4_PIN);
    Y_STEPPER_DDR_D |= (1 << Y_STEPPER_IN1_PIN) | (1 << Y_STEPPER_IN2_PIN);
    Y_STEPPER_DDR_B |= (1 << Y_STEPPER_IN3_PIN) | (1 << Y_STEPPER_IN4_PIN);
    
    // Servo pin
    DDRB |= (1 << SERVO_PIN);
}

void timerservo_init(void) {
    TCCR1A = (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1 = 19999;
}

void servo_set_angle(uint32_t angle) {
    uint16_t ocr_value = 1000 + (angle * 1000) / 180;
    OCR1B = ocr_value;
    _delay_ms(15);
}

void step_motor(int motor_x, int motor_y, int steps_x, int steps_y) {
    static int sequence_index_x = 0;
    static int sequence_index_y = 0;
    
    if (motor_x) {
        X_STEPPER_PORT &= ~((1 << X_STEPPER_IN1_PIN) | (1 << X_STEPPER_IN2_PIN) |
                            (1 << X_STEPPER_IN3_PIN) | (1 << X_STEPPER_IN4_PIN));
        sequence_index_x = (sequence_index_x + steps_x + 8) % 8;
        uint8_t current_step = stepper_sequence[sequence_index_x];
        
        if (current_step & (1 << 0)) X_STEPPER_PORT |= (1 << X_STEPPER_IN1_PIN);
        if (current_step & (1 << 1)) X_STEPPER_PORT |= (1 << X_STEPPER_IN2_PIN);
        if (current_step & (1 << 2)) X_STEPPER_PORT |= (1 << X_STEPPER_IN3_PIN);
        if (current_step & (1 << 3)) X_STEPPER_PORT |= (1 << X_STEPPER_IN4_PIN);
    }
    
    if (motor_y) {
        Y_STEPPER_PORT_D &= ~((1 << Y_STEPPER_IN1_PIN) | (1 << Y_STEPPER_IN2_PIN));
        Y_STEPPER_PORT_B &= ~((1 << Y_STEPPER_IN3_PIN) | (1 << Y_STEPPER_IN4_PIN));
        sequence_index_y = (sequence_index_y + steps_y + 8) % 8;
        uint8_t current_step = stepper_sequence[sequence_index_y];
        
        if (current_step & (1 << 0)) Y_STEPPER_PORT_D |= (1 << Y_STEPPER_IN1_PIN);
        if (current_step & (1 << 1)) Y_STEPPER_PORT_D |= (1 << Y_STEPPER_IN2_PIN);
        if (current_step & (1 << 2)) Y_STEPPER_PORT_B |= (1 << Y_STEPPER_IN3_PIN);
        if (current_step & (1 << 3)) Y_STEPPER_PORT_B |= (1 << Y_STEPPER_IN4_PIN);
    }
    _delay_ms(1);
}

void draw_line(int steps_x, int steps_y) {
    if (steps_x == 0 && steps_y == 0) return;
    int dx = abs(steps_x);
    int dy = -abs(steps_y);
    int sx = (steps_x > 0) ? 1 : -1;
    int sy = (steps_y > 0) ? 1 : -1;
    int err = dx + dy;
    
    while (1) {
        if (steps_x == 0 && steps_y == 0) break;
        int e2 = 2 * err;
        
        if (e2 >= dy) {
            err += dy;
            step_motor(1, 0, sx, 0);
            steps_x -= sx;
        }
        if (e2 <= dx) {
            err += dx;
            step_motor(0, 1, 0, sy);
            steps_y -= sy;
        }
    }
}