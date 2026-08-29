// PIC16F877A - Traffic Light Controller

unsigned int west_cnt = 0;
unsigned int south_cnt = 0;

void display_numbers(unsigned int west_sec, unsigned int south_sec) {
    unsigned char w_tens = west_sec / 10;
    unsigned char w_ones = west_sec % 10;
    unsigned char s_tens = south_sec / 10;
    unsigned char s_ones = south_sec % 10;

    // 1. Display Tens (Q1 & Q3 active LOW)
    PORTD = (s_tens << 4) | (w_tens & 0x0F);
    PORTC = 0b11110101;
    Delay_ms(100);

    // 2. Display Ones (Q2 & Q4 active LOW)
    PORTD = (s_ones << 4) | (w_ones & 0x0F);
    PORTC = 0b11111010;
    Delay_ms(100);
}

unsigned char wait_one_second() {
    unsigned int ms;
    for (ms = 0; ms < 166; ms++) {
        if ((PORTA & 0x01) == 1) return 1;
        display_numbers(west_cnt, south_cnt);
    }
    return 0;
}

unsigned char run_phase(unsigned int duration, unsigned char dec_west, unsigned char dec_south) {
    unsigned int i;
    for (i = 0; i < duration; i++) {
        if (wait_one_second() == 1) return 1;

        if (dec_west == 1 && west_cnt > 0) west_cnt--;
        if (dec_south == 1 && south_cnt > 0) south_cnt--;
    }
    return 0;
}

void manual_yellow_safety(unsigned char active_street) {
    unsigned int seconds, ms;
    west_cnt = 3;
    south_cnt = 3;

    if (active_street == 0) {
        PORTB = 0b00001010; // West Yellow (RB1), South Red (RB3)
    } else {
        PORTB = 0b00010001; // West Red (RB0), South Yellow (RB4)
    }

    for (seconds = 0; seconds < 3; seconds++) {
        for (ms = 0; ms < 166; ms++) {
            if ((PORTA & 0x01) == 0) return;
            display_numbers(west_cnt, south_cnt);
        }
        if (west_cnt > 0) west_cnt--;
        if (south_cnt > 0) south_cnt--;
    }
}

void main() {
    unsigned char current_street;

    ADCON1 = 0x06;  // Set PORTA as digital
    TRISA = 0xFF;   // PORTA Inputs
    TRISB = 0x00;   // PORTB Outputs (LEDs)
    TRISC = 0x00;   // PORTC Outputs (PNP Control)
    TRISD = 0x00;   // PORTD Outputs (7447 Inputs)

    PORTB = 0x00;
    PORTC = 0xFF;   // Disable displays
    PORTD = 0x00;

    while(1) {
        // ==========================================
        // 1. AUTOMATIC MODE (RA0 == 0)
        // ==========================================
        if ((PORTA & 0x01) == 0) {

            // Phase 1: West Red (15s total: 12s Green + 3s Yellow for South)
            PORTB = 0b00100001; // West Red (RB0), South Green (RB5)
            west_cnt = 15;
            south_cnt = 12;
            if (run_phase(12, 1, 1) == 1) continue;

            PORTB = 0b00010001; // West Red (RB0), South Yellow (RB4)
            south_cnt = 3;
            if (run_phase(3, 1, 1) == 1) continue;

            // Phase 2: South Red (23s total: 20s Green + 3s Yellow for West)
            PORTB = 0b00001100; // West Green (RB2), South Red (RB3)
            west_cnt = 20;
            south_cnt = 23;
            if (run_phase(20, 1, 1) == 1) continue;

            PORTB = 0b00001010; // West Yellow (RB1), South Red (RB3)
            west_cnt = 3;
            if (run_phase(3, 1, 1) == 1) continue;
        }

        // ==========================================
        // 2. MANUAL MODE (RA0 == 1)
        // ==========================================
        else {
            current_street = (PORTA & 0x02) >> 1;

            if (current_street == 0) {
                PORTB = 0b00001100; // West Green (RB2), South Red (RB3)

                while (((PORTA & 0x01) == 1) && (((PORTA & 0x02) >> 1) == 0)) {
                    display_numbers(0, 0);
                }

                if ((PORTA & 0x01) == 1) {
                    manual_yellow_safety(0);
                }
            }
            else {
                PORTB = 0b00100001; // West Red (RB0), South Green (RB5)

                while (((PORTA & 0x01) == 1) && (((PORTA & 0x02) >> 1) == 1)) {
                    display_numbers(0, 0);
                }

                if ((PORTA & 0x01) == 1) {
                    manual_yellow_safety(1);
                }
            }
        }
    }
}