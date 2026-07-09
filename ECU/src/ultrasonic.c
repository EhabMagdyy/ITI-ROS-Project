/*
 * ultrasonic.c — HC-SR04 distance measurement via GPIO ISR + RTOS state machine
 *
 * Race condition prevention:
 *   Each sensor has a state machine (IDLE / ARMED / CAPTURING).
 *   The ISR only acts when the trigger thread has explicitly moved the
 *   state to ARMED or CAPTURING.  Any edge received while IDLE is silently
 *   discarded, so noise between measurements cannot corrupt results.
 *
 *   Flow per measurement cycle:
 *     Thread                        ISR
 *     ──────────────────────────    ───────────────────────────────────
 *     k_sem_reset()                 (flush any stale gives)
 *     state = ARMED
 *     fire TRIG pulse
 *     k_sem_take(K_MSEC(30)) ──┐
 *                               │   rising edge → state==ARMED?
 *                               │     record rise timestamp
 *                               │     state = CAPTURING
 *                               │
 *                               │   falling edge → state==CAPTURING?
 *                               │     compute distance
 *                               │     state = IDLE
 *                               └── k_sem_give()
 *     state = IDLE (disarm)
 *     if timeout: dist = 999
 *     k_msleep(100)
 *
 * With GPIO_PULL_DOWN on ECHO pins:
 *   - No sensor connected → pin stays LOW → no rising edge → timeout → 999 cm
 *   - Real HC-SR04 connected → echo pulse drives pin HIGH → ISR fires correctly
 *
 * Pins (device tree, gpio1 = GPIO32+):
 *   US_FRONT_TRIG  GPIO35  Output
 *   US_FRONT_ECHO  GPIO36  Input (pull-down, both-edge ISR)
 *   US_BACK_TRIG   GPIO37  Output
 *   US_BACK_ECHO   GPIO38  Input (pull-down, both-edge ISR)
 *   US_LEFT_TRIG   GPIO39  Output
 *   US_LEFT_ECHO   GPIO40  Input (pull-down, both-edge ISR)
 *   US_RIGHT_TRIG  GPIO41  Output
 *   US_RIGHT_ECHO  GPIO42  Input (pull-down, both-edge ISR)
 */

#include "ultrasonic.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ultrasonic, LOG_LEVEL_INF);

/* ── Sensor state machine ──────────────────────────────────── */

typedef enum {
	US_IDLE = 0,   /* Between measurements — ISR ignores all edges */
	US_ARMED,      /* TRIG fired — ISR waiting for rising edge     */
	US_CAPTURING,  /* Rising edge caught — ISR waiting for falling  */
} us_state_t;

/* ── Per-sensor data ───────────────────────────────────────── */

static volatile us_state_t us_state[US_COUNT];   /* State machine       */
static volatile uint32_t   us_rise_cyc[US_COUNT]; /* Rising edge timestamp */
static volatile uint32_t   us_dist_cm[US_COUNT];  /* Latest distance (cm)  */

static struct k_sem         us_sem[US_COUNT];      /* Thread ↔ ISR sync   */
static struct gpio_callback us_echo_cb[US_COUNT];

/* ── Hardware bindings ─────────────────────────────────────── */

static const struct gpio_dt_spec us_trig[US_COUNT] = {
	GPIO_DT_SPEC_GET(DT_NODELABEL(us_front_trig), gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(us_back_trig),  gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(us_left_trig),  gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(us_right_trig), gpios),
};

static const struct gpio_dt_spec us_echo[US_COUNT] = {
	GPIO_DT_SPEC_GET(DT_NODELABEL(us_front_echo), gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(us_back_echo),  gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(us_left_echo),  gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(us_right_echo), gpios),
};

/* ── ISR ───────────────────────────────────────────────────── */

static void echo_isr_generic(int idx)
{
	if (us_state[idx] == US_ARMED) {
		/*
		 * First interrupt after TRIG = rising edge (HC-SR04 drives
		 * ECHO HIGH to signal start of pulse). Latch timestamp and
		 * advance state — do NOT call gpio_pin_get_dt() here; reading
		 * the pin level inside an ISR on ESP32-S3 is unreliable.
		 */
		us_rise_cyc[idx] = k_cycle_get_32();
		us_state[idx]    = US_CAPTURING;

	} else if (us_state[idx] == US_CAPTURING) {
		/*
		 * Second interrupt = falling edge (HC-SR04 drives ECHO LOW
		 * to signal end of pulse). Compute distance and signal thread.
		 */
		uint32_t elapsed_us =
			k_cyc_to_us_near32(k_cycle_get_32() - us_rise_cyc[idx]);

		uint32_t dist = elapsed_us / 58U;

		us_dist_cm[idx] = (elapsed_us < 116U || dist > 400U)
				  ? 999U : dist;

		us_state[idx] = US_IDLE;
		k_sem_give(&us_sem[idx]);

	}
	/* US_IDLE: discard — spurious edge between cycles */
}

/* One wrapper per sensor (Zephyr requires a unique function pointer) */
static void echo_isr_0(const struct device *d, struct gpio_callback *c, uint32_t p)
{
	ARG_UNUSED(d); ARG_UNUSED(c); ARG_UNUSED(p);
	echo_isr_generic(US_FRONT);
}
static void echo_isr_1(const struct device *d, struct gpio_callback *c, uint32_t p)
{
	ARG_UNUSED(d); ARG_UNUSED(c); ARG_UNUSED(p);
	echo_isr_generic(US_BACK);
}
static void echo_isr_2(const struct device *d, struct gpio_callback *c, uint32_t p)
{
	ARG_UNUSED(d); ARG_UNUSED(c); ARG_UNUSED(p);
	echo_isr_generic(US_LEFT);
}
static void echo_isr_3(const struct device *d, struct gpio_callback *c, uint32_t p)
{
	ARG_UNUSED(d); ARG_UNUSED(c); ARG_UNUSED(p);
	echo_isr_generic(US_RIGHT);
}

typedef void (*echo_isr_fn)(const struct device *, struct gpio_callback *, uint32_t);

static const echo_isr_fn echo_isr_table[US_COUNT] = {
	echo_isr_0, echo_isr_1, echo_isr_2, echo_isr_3,
};

/* ── Trigger Thread (1 per sensor) ────────────────────────── */

static void us_trigger_thread(void *arg1, void *arg2, void *arg3)
{
	int idx = (int)(intptr_t)arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		/*
		 * 1. Flush any stale semaphore give from a previous cycle
		 *    (e.g. if a spurious edge arrived during k_msleep).
		 */
		k_sem_reset(&us_sem[idx]);

		/*
		 * 2. Transition to ARMED — ISR will now accept a rising edge.
		 *    Do this BEFORE firing TRIG so no edge is missed.
		 */
		us_state[idx] = US_ARMED;

		/* 3. Fire 10 µs TRIG pulse */
		gpio_pin_set_dt(&us_trig[idx], 1);
		k_busy_wait(10);
		gpio_pin_set_dt(&us_trig[idx], 0);

		/*
		 * 4. Block until ISR gives the semaphore (measurement done)
		 *    or until 30 ms timeout (≈ 5 m max range).
		 */
		int ret = k_sem_take(&us_sem[idx], K_MSEC(30));

		/*
		 * 5. Disarm — any further ISR edges are discarded until
		 *    the next cycle arms the state machine again.
		 */
		us_state[idx] = US_IDLE;

		if (ret == -EAGAIN) {
			/* Timeout: no echo received — sensor absent / out of range */
			us_dist_cm[idx] = 999U;
		}

		/* HC-SR04 needs ≥ 60 ms between pings; yield CPU during wait */
		k_msleep(100);
	}
}

/* ── Thread infrastructure ─────────────────────────────────── */

#define US_THREAD_STACK_SIZE  2048
#define US_THREAD_PRIORITY    5

K_THREAD_STACK_DEFINE(us_stack_0, US_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(us_stack_1, US_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(us_stack_2, US_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(us_stack_3, US_THREAD_STACK_SIZE);

static struct k_thread us_thread_data[US_COUNT];

static k_thread_stack_t *const us_stacks[US_COUNT] = {
	us_stack_0, us_stack_1, us_stack_2, us_stack_3,
};

/* ── Public API ────────────────────────────────────────────── */

int ultrasonic_init(void)
{
	for (int i = 0; i < US_COUNT; i++) {
		/* Init semaphore: count=0, max=1 */
		k_sem_init(&us_sem[i], 0, 1);

		/* Start in IDLE — ISR ignores edges until thread arms it */
		us_state[i] = US_IDLE;

		/* Configure TRIG as output (LOW) */
		gpio_pin_configure_dt(&us_trig[i], GPIO_OUTPUT_INACTIVE);

		/*
		 * Configure ECHO as input.
		 * GPIO_PULL_DOWN is baked into the DT overlay flags so that
		 * an unconnected pin reads LOW and never triggers the ISR.
		 */
		gpio_pin_configure_dt(&us_echo[i], GPIO_INPUT);

		/* Register both-edge ISR */
		gpio_pin_interrupt_configure_dt(&us_echo[i],
						GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&us_echo_cb[i], echo_isr_table[i],
				   BIT(us_echo[i].pin));
		gpio_add_callback(us_echo[i].port, &us_echo_cb[i]);

		/* Spawn trigger thread AFTER hardware is ready */
		k_thread_create(&us_thread_data[i],
				us_stacks[i],
				US_THREAD_STACK_SIZE,
				us_trigger_thread,
				(void *)(intptr_t)i, NULL, NULL,
				US_THREAD_PRIORITY,
				0,
				K_NO_WAIT);
	}

	LOG_INF("Ultrasonics initialized (4 sensors, ISR + state machine)");
	return 0;
}

uint32_t ultrasonic_get_dist(int idx)
{
	if (idx < 0 || idx >= US_COUNT) {
		return 999U;
	}
	return us_dist_cm[idx];
}
