
// =================================================================
//                  Knokoo FES150 Pedal Control
//                      PIC12F1572 Firmware
//                      Philipp Schilk 2020
//
// This was written fairly quickly without much second thought.
// Please don't look to closely.
// =================================================================


// ======== PIC Fuses ==========================================================

// CONFIG1
#pragma config FOSC = INTOSC    // (INTOSC oscillator; I/O function on CLKIN pin)
#pragma config WDTE = ON        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOREN = OFF    // Low Power Brown-out Reset enable bit (LPBOR is disabled)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)


// ======== Config =============================================================

#define DEBOUNCE_TIME  25  // Time inputs are ignored after input in 10s of ms
#define DBLPRESS_TIME  400 // Max interval to count as 'double press' in 10s of ms
#define BLINK_TIME     100 // LED Blink period in 10s of ms

// Coefficients to determine min and max length:
// Min length in seconds: ADCSCALE_BASE / 100
// Max length in seconds: (ADCSCALE_BASE + ADCSCALE_MULT*2^10) / 100
#define ADCSCALE_MULT 90
#define ADCSCALE_BASE 12000

// ======== Code ===============================================================

#include <pic.h>
#include <stdint.h>

uint32_t debounce_count;
uint8_t debounce_prevstate;

uint32_t on_timer;
uint32_t press_timer;

uint32_t blink_timer;
uint8_t  blink_state;

uint32_t result;

typedef enum{
    state_off,
    state_timer,
    state_on         
}STATE_T;

STATE_T state;

int main(int argc, char** argv) {
    CLRWDT();
    
    // ===== WDT & CLK =====
    // Using internal MF OSC at 500khz
    OSCCONbits.SPLLEN = 0;
    OSCCONbits.IRCF = 0b0111;
    OSCCONbits.SCS = 0b10;
    
    // Have WDT reset every 512ms
    WDTCONbits.WDTPS = 0b01001;
    
    // ===== Config GPIO =====
    TRISAbits.TRISA0 = 1; // Pedal Input
    TRISAbits.TRISA1 = 1; // Pedal Sense Input
    TRISAbits.TRISA2 = 0; // KNOKOO Ouput
    TRISAbits.TRISA4 = 1; // Poti Input
    TRISAbits.TRISA5 = 0; // LED Output
    
    // Clear all outputs
    LATA = 0x0; 
    
    // Set RA4 as only analog input
    ANSELA = 0x0;
    ANSELAbits.ANSA4 = 1;
    
    // Disable all pullups, all pins push-pull,
    // no slewrate limiting
    WPUA = 0x0;
    ODCONA = 0x0;
    SLRCONA = 0x0;
    
    // ===== Config ADC ======
    ADCON0bits.CHS = 0b00011; // Select CH3 (RA4)
    
    ADCON1bits.ADFM = 1; // Right-justified result
    ADCON1bits.ADCS = 0b000; // Conversion Clk Fosc/2
    ADCON1bits.ADPREF = 0b00; // VDD is positive voltage refernce
    
    ADCON2bits.TRIGSEL = 0; // No Automatic triggering
    
    ADCON0bits.ADON = 1; // Turn on ADC
    
    ADCON0bits.GO = 1; // Start conversion
    
    // ===== Setup Variables =====
    debounce_count = 0;
    debounce_prevstate = PORTAbits.RA0;
    state = state_off;
    blink_state = 0;
    
    // ===== Config Regular interrupt =====
    // Setup Timer 0 to overflow at aprox. 120Hz and trigger interrupt
    OPTION_REGbits.TMR0CS = 0; // Timer 0 clocked from Fosc/4
    OPTION_REGbits.PSA = 0;    // Timer 0 has prescalar
    OPTION_REGbits.PS = 0b01;  // Timer 0 prescalar is /4
    
    INTCONbits.GIE = 1;    // Interupts enabled.
    INTCONbits.TMR0IE = 1; // Timer 0 interrupt enabled
    
    while(1){
        NOP();
    }
    return (EXIT_SUCCESS);
}

void UI(uint8_t did_press){
    // If you close your eyes, you can't see this mess of a state machine.
    switch(state){
        case state_off:
            LATAbits.LATA2 = 0;
            LATAbits.LATA5 = 0;
            
            if(did_press){
                // Go to timer state.
                state = state_timer;
                
                // Start timer
                on_timer = ADCSCALE_BASE+(ADCSCALE_MULT*result);
                
                // Reset Blinking
                blink_state = 1;
                blink_timer = BLINK_TIME;
                
                // Start double-press timer
                press_timer = DBLPRESS_TIME;
            }
            break;

        case state_timer:
            
            LATAbits.LATA2 = 1;
            LATAbits.LATA5 = blink_state;
            
            if(blink_timer == 0){
                blink_timer = BLINK_TIME;
                
                blink_state = (blink_state) ? 0 : 1;    
            } else {
                blink_timer--;
            }
            
            if(did_press){
                if(press_timer == 0){
                    // Turn off
                    state = state_off;
                    break;
                } else {
                    // Go to always on
                    state = state_on;
                    break;
                }
            }
            
            if(on_timer == 0){
                state = state_off;
                break;
            } else {
                on_timer--;
            }
            
            if(press_timer != 0){
                press_timer--;
            }
            break;
            
        case state_on:
            
            PORTAbits.RA2 = 1;
            PORTAbits.RA5 = 1;
            
            if(did_press){
                state = state_off;
            }
            
            break;
        default:
            state = state_off;
    }

}

void __interrupt() ISR(void){
    CLRWDT(); // pet the watchdog..
    
    // Handle ADC
    if(!ADCON0bits.GO_nDONE){
        // ADC Conversion finished
        result = (ADRESH << 8) | ADRESL;
        
        // Start next conversion
        ADCON0bits.GO_nDONE = 1;
    }
    
    uint8_t did_press = 0;
    
	// Handle De-bounce
	if(debounce_count == 0){ // Ready for next event
        // Determine if input changed and we should time out again
        if(debounce_prevstate != PORTAbits.RA0){
            debounce_count = DEBOUNCE_TIME;
            debounce_prevstate = PORTAbits.RA0;
            
            if(PORTAbits.RA0){
                did_press = 1;
            }
        }
        
    } else { // Still timed out
        debounce_count--;
        did_press = 0;
    }
    
    // If there is a pedal connected, handle the UI
    if(!PORTAbits.RA1){
        UI(did_press);
    } else {
        // Otherwise, just power on all the time
        PORTAbits.RA2 = 1;
        PORTAbits.RA5 = 0;
    }
    
    // Clear flags and reset GIE
    INTCONbits.TMR0IF = 0;
    INTCONbits.GIE = 1;
}
