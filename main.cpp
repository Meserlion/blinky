#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdint.h>
#define LED_PIN PB5
#define NUM_PATTERNS 5

static inline void led_on(void)  { PORTB |= (1 << LED_PIN); }
static inline void led_off(void) { PORTB &= ~(1 << LED_PIN); }

// PB5 has no hardware PWM timer on the ATmega328P, so brightness is
// faked with software PWM: one 2ms period split into an on/off ratio.
static inline void soft_pwm_cycle(uint8_t duty_percent)
{
	uint8_t off_percent = 100 - duty_percent;

	led_on();
	for (uint8_t i = 0; i < duty_percent; i++)
	{
		_delay_us(20);
	}

	led_off();
	for (uint8_t i = 0; i < off_percent; i++)
	{
		_delay_us(20);
	}
}

// Hold a brightness level for ~30ms by repeating the PWM cycle.
static inline void hold_brightness(uint8_t duty_percent)
{
	for (uint8_t i = 0; i < 15; i++)
	{
		soft_pwm_cycle(duty_percent);
	}
}

// 8-bit Galois LFSR (taps 0xB8) for cheap pseudo-randomness, since a bare
// ATmega328P has no true entropy source without extra wiring.
static uint8_t lfsr_state;

static uint8_t lfsr_next(void)
{
	uint8_t lsb = lfsr_state & 1;
	lfsr_state >>= 1;
	if (lsb)
	{
		lfsr_state ^= 0xB8;
	}
	return lfsr_state;
}

static void pattern_heartbeat(void)
{
	while (1)
	{
		led_on();
		_delay_ms(80);
		led_off();
		_delay_ms(120);

		led_on();
		_delay_ms(80);
		led_off();
		_delay_ms(600);
	}
}

static void pattern_breathe(void)
{
	while (1)
	{
		for (uint8_t duty = 0; duty <= 100; duty++)
		{
			hold_brightness(duty);
		}
		for (uint8_t duty = 100; duty > 0; duty--)
		{
			hold_brightness(duty - 1);
		}
		_delay_ms(300);
	}
}

static void morse_dot(void)  { led_on(); _delay_ms(150); led_off(); _delay_ms(150); }
static void morse_dash(void) { led_on(); _delay_ms(450); led_off(); _delay_ms(150); }

static void pattern_sos(void)
{
	while (1)
	{
		morse_dot(); morse_dot(); morse_dot();
		_delay_ms(300);
		morse_dash(); morse_dash(); morse_dash();
		_delay_ms(300);
		morse_dot(); morse_dot(); morse_dot();
		_delay_ms(1000);
	}
}

static void pattern_strobe(void)
{
	while (1)
	{
		for (uint8_t i = 0; i < 8; i++)
		{
			led_on();
			_delay_ms(40);
			led_off();
			_delay_ms(40);
		}
		_delay_ms(800);
	}
}

static void pattern_candle(void)
{
	while (1)
	{
		uint8_t duty = 60 + (lfsr_next() % 40);
		hold_brightness(duty);
	}
}

int main(void)
{
	DDRB |= (1 << LED_PIN);
	led_off();

	// Persist a boot counter in EEPROM so each reset picks a different
	// pattern (deterministic cycling, not true randomness).
	uint8_t boot_count = eeprom_read_byte((uint8_t *)0);
	eeprom_update_byte((uint8_t *)0, boot_count + 1);

	lfsr_state = boot_count | 1;

	switch (boot_count % NUM_PATTERNS)
	{
		case 0: pattern_heartbeat(); break;
		case 1: pattern_breathe(); break;
		case 2: pattern_sos(); break;
		case 3: pattern_strobe(); break;
		default: pattern_candle(); break;
	}
}
