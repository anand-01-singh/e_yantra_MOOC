#define F_CPU 14745600UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// ================= LCD =================
#define RS 0
#define RW 1
#define EN 2

#define lcd_port PORTC
#define lcd_ddr  DDRC

#define sbit(reg,bit) reg |= (1<<bit)
#define cbit(reg,bit) reg &= ~(1<<bit)

// ================= BUZZER =================
#define BUZZER_PIN 3

volatile uint8_t buzzer_state = 0;   // 0 = OFF, 1 = ON

void buzzer_pin_config()
{
    DDRC  |=  (1<<BUZZER_PIN);
    PORTC &= ~(1<<BUZZER_PIN);
}

void buzzer_on()
{
    PORTC |=  (1<<BUZZER_PIN);
    buzzer_state = 1;
}

void buzzer_off()
{
    PORTC &= ~(1<<BUZZER_PIN);
    buzzer_state = 0;
}

void buzzer_toggle()
{
    if(buzzer_state == 0)
        buzzer_on();
    else
        buzzer_off();
}

// ================= MOTOR =================

#define MOTOR_DIR_DDR  DDRA
#define MOTOR_DIR_PORT PORTA

// Right motor pins
#define motors_RF_pin  0   // Right Forward
#define motors_RB_pin  1   // Right Backward

// Left motor pins
#define motors_LF_pin  2   // Left Forward
#define motors_LB_pin  3   // Left Backward

#define MOTOR_PWM_DDR  DDRL

#define motors_pwm_R_pin  3   // OC5A -> Right motor PWM
#define motors_pwm_L_pin  4   // OC5B -> Left  motor PWM

#define DEFAULT_SPEED  250
#define TURN_SPEED     250

volatile char current_cmd = 'S';

// ================= LCD FUNCTIONS =================

void lcd_port_config()
{
    lcd_ddr  |=  0xF7;
    lcd_port &=  0x08;
}

void lcd_wr_command(unsigned char cmd)
{
    unsigned char t;

    // High nibble
    t = cmd & 0xF0;
    lcd_port &= 0x0F;
    lcd_port |= t;

    cbit(lcd_port, RS);
    cbit(lcd_port, RW);

    sbit(lcd_port, EN);
    _delay_ms(5);
    cbit(lcd_port, EN);

    // Low nibble
    cmd = (cmd & 0x0F) << 4;
    lcd_port &= 0x0F;
    lcd_port |= cmd;

    cbit(lcd_port, RS);
    cbit(lcd_port, RW);

    sbit(lcd_port, EN);
    _delay_ms(5);
    cbit(lcd_port, EN);
}

void lcd_set_4bit()
{
    _delay_ms(1);

    for(int i = 0; i < 3; i++)
    {
        lcd_port = 0x30;
        sbit(lcd_port, EN);
        _delay_ms(5);
        cbit(lcd_port, EN);
    }

    lcd_port = 0x20;
    sbit(lcd_port, EN);
    _delay_ms(5);
    cbit(lcd_port, EN);
}

void lcd_init()
{
    lcd_set_4bit();

    lcd_wr_command(0x28);   // 4-bit, 2 lines, 5x7 font
    lcd_wr_command(0x01);   // Clear display
    lcd_wr_command(0x06);   // Entry mode: increment, no shift
    lcd_wr_command(0x0E);   // Display ON, cursor ON, blink OFF
    lcd_wr_command(0x80);   // Set cursor to home
}

void lcd_cursor(char row, char col)
{
    if(row == 1) lcd_wr_command(0x80 + col - 1);
    if(row == 2) lcd_wr_command(0xC0 + col - 1);
}

void lcd_char(char row, char col, char ch)
{
    lcd_cursor(row, col);

    // High nibble
    char t = ch & 0xF0;
    lcd_port &= 0x0F;
    lcd_port |= t;

    sbit(lcd_port, RS);
    cbit(lcd_port, RW);

    sbit(lcd_port, EN);
    _delay_ms(5);
    cbit(lcd_port, EN);

    // Low nibble
    ch = (ch & 0x0F) << 4;
    lcd_port &= 0x0F;
    lcd_port |= ch;

    sbit(lcd_port, RS);
    cbit(lcd_port, RW);

    sbit(lcd_port, EN);
    _delay_ms(5);
    cbit(lcd_port, EN);
}

void lcd_string(char row, char col, const char *str)
{
    while(*str != '\0')
    {
        lcd_char(row, col, *str);
        str++;
        col++;
    }
}

void lcd_clear()
{
    lcd_wr_command(0x01);
    _delay_ms(2);
}

// ================= MOTOR FUNCTIONS =================

void motors_pin_config()
{
    MOTOR_DIR_DDR  |=  0x0F;
    MOTOR_DIR_PORT &=  0xF0;
}

void pwm_pin_config()
{
    MOTOR_PWM_DDR |= (1<<motors_pwm_R_pin) | (1<<motors_pwm_L_pin);
}

void timer_pwm_init()
{
    // Phase-correct PWM, 8-bit, non-inverting on OC5A and OC5B
    TCCR5A = (1<<COM5A1) | (1<<COM5B1) | (1<<WGM50);
    TCCR5B = (1<<CS51);   // Prescaler = 8

    TCNT5  = 0;
    OCR5A  = 0;
    OCR5B  = 0;
}

void set_speed(unsigned char right, unsigned char left)
{
    OCR5A = right;
    OCR5B = left;
}

void motor_stop()
{
    MOTOR_DIR_PORT &= 0xF0;
    set_speed(0, 0);
}

void motor_forward(unsigned char speed)
{
    // Both motors backward: RB + LB
    MOTOR_DIR_PORT = (MOTOR_DIR_PORT & 0xF0) | (1<<motors_RB_pin) | (1<<motors_LB_pin);
    set_speed(speed, speed);
}

void motor_backward(unsigned char speed)
{
    // Both motors forward: RF + LF
    MOTOR_DIR_PORT = (MOTOR_DIR_PORT & 0xF0) | (1<<motors_RF_pin) | (1<<motors_LF_pin);
    set_speed(speed, speed);
}

/*
 * Turn LEFT
 */
void motor_right(unsigned char speed)
{
    MOTOR_DIR_PORT = (MOTOR_DIR_PORT & 0xF0) | (1<<motors_RF_pin) | (1<<motors_LB_pin);
    set_speed(speed, speed);
}

/*
 * Turn RIGHT
 */
void motor_left(unsigned char speed)
{
    MOTOR_DIR_PORT = (MOTOR_DIR_PORT & 0xF0) | (1<<motors_LF_pin) | (1<<motors_RB_pin);
    set_speed(speed, speed);
}

// ================= UART3 =================

void uart3_init()
{
    UCSR3B = 0x00;
    UCSR3A = 0x00;
    UCSR3C = 0x06;   // 8 data bits, 1 stop bit, no parity

    // Baud rate 9600 @ 14.7456 MHz => UBRR = 95 = 0x5F
    UBRR3L = 0x5F;
    UBRR3H = 0x00;

    UCSR3B = 0x98;   // RX enable, TX enable, RX interrupt enable
}

// ================= BLUETOOTH ISR =================
/*
 * APP BUTTON  =>  ACTUAL ROBOT MOTION  (remapped here to fix mismatch)
 * ---------------------------------------------------------------
 *  App 'F' (Forward)   =>  motor_left()     (robot turns LEFT)
 *  App 'B' (Backward)  =>  motor_right()    (robot turns RIGHT)
 *  App 'L' (Left)      =>  motor_backward() (robot goes BACKWARD)
 *  App 'R' (Right)     =>  motor_forward()  (robot goes FORWARD)
 *  App 'Y' (Horn)      =>  toggle buzzer
 *  App 'S' (Stop)      =>  motor_stop() + buzzer_off()
 */

ISR(USART3_RX_vect)
{
    current_cmd = UDR3;

    switch(current_cmd)
    {
        case 'F': motor_left(TURN_SPEED);        break;  // App:Forward  => Robot:Left
        case 'B': motor_right(TURN_SPEED);       break;  // App:Backward => Robot:Right
        case 'L': motor_backward(DEFAULT_SPEED); break;  // App:Left     => Robot:Backward
        case 'R': motor_forward(DEFAULT_SPEED);  break;  // App:Right    => Robot:Forward

        case 'Y': buzzer_toggle(); break;  // One press ON, next press OFF

        case 'S':
            motor_stop();
            buzzer_off();
            break;
    }
}

// ================= LCD STATUS =================
// LCD always shows ACTUAL robot action

void lcd_show(char cmd)
{
    lcd_clear();

    lcd_string(1, 1, "CMD:");
    lcd_char(1, 6, cmd);

    switch(cmd)
    {
        case 'F': lcd_string(2, 1, "Forward");  break;
        case 'B': lcd_string(2, 1, "Backward"); break;
        case 'L': lcd_string(2, 1, "Left Turn");   break;
        case 'R': lcd_string(2, 1, "Right Turn");    break;

        case 'Y':
            if(buzzer_state)
                lcd_string(2, 1, "Horn ON");
            else
                lcd_string(2, 1, "Horn OFF");
            break;

        case 'S': lcd_string(2, 1, "Stop"); break;

        default:  lcd_string(2, 1, "Unknown"); break;
    }
}

// ================= INIT =================

void init_devices()
{
    cli();   // Disable global interrupts during init

    lcd_port_config();
    lcd_init();

    buzzer_pin_config();

    pwm_pin_config();
    timer_pwm_init();

    motors_pin_config();
    motor_stop();

    uart3_init();

    sei();   // Enable global interrupts
}

// ================= MAIN =================

int main(void)
{
    init_devices();

    lcd_string(1, 1, "Firebird V BT");
    lcd_string(2, 1, "Waiting...");

    char last = 'S';

    while(1)
    {
        if(current_cmd != last)
        {
            lcd_show(current_cmd);
            last = current_cmd;
        }
    }
}