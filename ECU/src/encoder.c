/*
 * encoder.c — Quadrature wheel encoder decoding via GPIO interrupts
 *
 * Pins (from device tree):
 *   ENC_L_A  GPIO4   Both-edge interrupt
 *   ENC_L_B  GPIO5   Both-edge interrupt
 *   ENC_R_A  GPIO6   Both-edge interrupt
 *   ENC_R_B  GPIO7   Both-edge interrupt
 *
 * Decode strategy (X4 — all edges of A and B):
 *   A-edge ISR: if A==B → forward (+1), else reverse (-1)
 *   B-edge ISR: if A!=B → forward (+1), else reverse (-1)
 */

#include "encoder.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(encoder, LOG_LEVEL_INF);

/* ── Hardware bindings ─────────────────────────────────────── */

static const struct gpio_dt_spec enc_l_a =
	GPIO_DT_SPEC_GET(DT_NODELABEL(enc_l_a), gpios);
static const struct gpio_dt_spec enc_l_b =
	GPIO_DT_SPEC_GET(DT_NODELABEL(enc_l_b), gpios);
static const struct gpio_dt_spec enc_r_a =
	GPIO_DT_SPEC_GET(DT_NODELABEL(enc_r_a), gpios);
static const struct gpio_dt_spec enc_r_b =
	GPIO_DT_SPEC_GET(DT_NODELABEL(enc_r_b), gpios);

/* ── State ─────────────────────────────────────────────────── */

static volatile int32_t ticks_l;
static volatile int32_t ticks_r;

static struct gpio_callback enc_l_a_cb;
static struct gpio_callback enc_l_b_cb;
static struct gpio_callback enc_r_a_cb;
static struct gpio_callback enc_r_b_cb;

/* ── ISRs ──────────────────────────────────────────────────── */

static void enc_l_a_isr(const struct device *dev,
			 struct gpio_callback *cb, uint32_t pins)
{
	int a = gpio_pin_get_dt(&enc_l_a);
	int b = gpio_pin_get_dt(&enc_l_b);

	ticks_l += (a == b) ? 1 : -1;
}

static void enc_l_b_isr(const struct device *dev,
			 struct gpio_callback *cb, uint32_t pins)
{
	int a = gpio_pin_get_dt(&enc_l_a);
	int b = gpio_pin_get_dt(&enc_l_b);

	ticks_l += (a != b) ? 1 : -1;
}

static void enc_r_a_isr(const struct device *dev,
			 struct gpio_callback *cb, uint32_t pins)
{
	int a = gpio_pin_get_dt(&enc_r_a);
	int b = gpio_pin_get_dt(&enc_r_b);

	ticks_r += (a == b) ? 1 : -1;
}

static void enc_r_b_isr(const struct device *dev,
			 struct gpio_callback *cb, uint32_t pins)
{
	int a = gpio_pin_get_dt(&enc_r_a);
	int b = gpio_pin_get_dt(&enc_r_b);

	ticks_r += (a != b) ? 1 : -1;
}

/* ── Public API ────────────────────────────────────────────── */

int encoder_init(void)
{
	/* Descriptor table — makes the init loop compact and uniform */
	const struct {
		const struct gpio_dt_spec *spec;
		struct gpio_callback      *cb;
		gpio_callback_handler_t    handler;
	} pins[] = {
		{ &enc_l_a, &enc_l_a_cb, enc_l_a_isr },
		{ &enc_l_b, &enc_l_b_cb, enc_l_b_isr },
		{ &enc_r_a, &enc_r_a_cb, enc_r_a_isr },
		{ &enc_r_b, &enc_r_b_cb, enc_r_b_isr },
	};

	for (int i = 0; i < ARRAY_SIZE(pins); i++) {
		gpio_pin_configure_dt(pins[i].spec, GPIO_INPUT | GPIO_PULL_UP);
		gpio_pin_interrupt_configure_dt(pins[i].spec,
						GPIO_INT_EDGE_BOTH);
		gpio_init_callback(pins[i].cb, pins[i].handler,
				   BIT(pins[i].spec->pin));
		gpio_add_callback(pins[i].spec->port, pins[i].cb);
	}

	LOG_INF("Encoders initialized (X4 quadrature)");
	return 0;
}

int32_t encoder_get_left(void)
{
	return ticks_l;
}

int32_t encoder_get_right(void)
{
	return ticks_r;
}

void encoder_reset(void)
{
	ticks_l = 0;
	ticks_r = 0;
}
