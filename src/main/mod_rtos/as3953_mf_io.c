/*
    FlexCOS - Copyright (C) 2013 AGSI, Department of Computer Science, FU-Berlin

    FOR MORE INFORMATION AND INSTRUCTION PLEASE VISIT
    http://www.inf.fu-berlin.de/groups/ag-si/smart.html


    This file is part of the FlexCOS project.

    FlexCOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 3) as published by the
    Free Software Foundation.

    Some parts of this software are from different projects. These files carry
    a different license in their header.

    FlexCOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details. You should have received a copy of the GNU General Public License
    along with FlexCOS; if not it can be viewed here:
    http://www.gnu.org/licenses/gpl-3.0.txt and also obtained by writing to
    AGSI, contact details for whom are available on the FlexCOS WEB site.

*/

/**
 * as3593_mf_io.c
 *
 * Created on Jun 12, 2013
 *     Author: kbeilke
 */

#include <const.h>
#include <types.h>
#include <hw.h>

#include <string.h>

#include <FreeRTOS.h>
#include <timers.h>

#include <semphr.h>
#include <queue.h>
#include <modules.h>


#include <math.h>

#include <array.h>
#include <buffers.h>

#include "iso14443_4.h"
#include "as3953.h"
#include "as3953_io.h"

/**
 ************************************************************************
 * Types for raw bitstream handling, required for MIFARE with Crypto1
 */
typedef struct bit_stream BitStream;

struct __packed bit_stream {
	const u8  *INF;
	u16 INF_size;
	u16 INF_bits;
};

static inline void
bitstream_reset(BitStream *blk) {
	memset(blk, 0x00, sizeof(*blk));
}

PRIVATE void as3953_isr_3(void *InstancePtr);


/**
 ************************************************************************
 */

PRIVATE xSemaphoreHandle sem_capdu_ready;
PRIVATE xSemaphoreHandle sem_rapdu_ready;
PRIVATE xSemaphoreHandle sem_rapdu_send;

enum Notification;
enum ComState;
enum RequestType;

PRIVATE void as3953_isr_handle_aux();

PRIVATE inline void notify_from_isr(u16);
PRIVATE inline void notify_state_machine(enum Notification);
PRIVATE inline bool has_been_notified(enum Notification);


enum Config {
	FWT_CUT_OFF = 50,  /* decrement FWT by this value */
};

/**
 * Define intern communication states.
 *
 * Default state is IDLE. Transition from IDLE to RECEIVE does only occur
 * on (partial) received I-blocks.
 */
enum ComState {
	WAIT_FOR_PCD,
	RECEIVING,
	HANDLE_REQUEST,
	SENDING,          /* only relevant for sending RADPU */
};

/* commands for state transition */
enum Notification {
	POWER    = MAIN_IRQ_POWER_UP,
	ACTIVE   = MAIN_IRQ_ACTIVE,
	WAKEUP   = MAIN_IRQ_WAKE_UP,
	START_RX = MAIN_IRQ_START_RX,
	END_RX   = MAIN_IRQ_END_RX,
	END_TX   = MAIN_IRQ_END_TX,
	WTR_LVL  = MAIN_IRQ_FIFO_WATER,
	AUX_ERR  = MAIN_IRQ_AUX,
};

typedef struct as3953_io_app_ctx  AppCtx;
typedef struct as3953_io_state    State;
typedef struct as3953_io_dev_ctx  DevCtx;

struct as3953_io_app_ctx {
	Array *const capdu;
	Array *const rapdu;
};

struct as3953_io_dev_ctx {
	u8 max_frame_size;
	u8 default_fwi;
};

struct __packed as3953_io_state {
	enum ComState is : 2;
	u8 pending_i_blk_response  : 1;
	/* protect fresh attribute against accidetial modification */
	union {
		const u8 is_fresh;
		u8 set_fresh;
	};
	u16 fwt;
	u8  block_number;
	u32 notifications;
	struct {
		u16 done;
		u16 bytes;
	} transfer;
};

PRIVATE State state = {0};

PRIVATE DevCtx dev_ctx = {
	/* Hacky: this value should been set to FSD after querying
	 * RATS register on WakeUp/ACTIVE/PowerUp? */
	.max_frame_size = 32,
	.default_fwi    = 10
};

PRIVATE inline void
state_enter(enum ComState s) {
	state.is = s;
	state.set_fresh = 1;
	return;
}

PRIVATE inline void wait_for_capdu() {
	xSemaphoreTake(sem_capdu_ready, portMAX_DELAY);
	return;
}

PRIVATE inline err_t wait_for_rapdu() {
	portTickType fwt = (state.fwt > FWT_CUT_OFF) ? state.fwt - FWT_CUT_OFF : 1;

	return xSemaphoreTake(sem_rapdu_ready, fwt / portTICK_RATE_MS) == pdTRUE ?
		E_GOOD : E_FAILED;
}

PRIVATE inline void
submit_rapdu_3(void)
{
	xSemaphoreGive(sem_rapdu_ready);
	return;
}

PRIVATE inline void
release_capdu_ctx(void)
{
	xSemaphoreGive(sem_capdu_ready);
	return;
}

PRIVATE void
state_init_3() {
	memset(&state, 0x00, sizeof(state));
	xil_printf("DD state_init\n");

	state_enter(WAIT_FOR_PCD);

	state.fwt = (iso14443_fwt_in_us(dev_ctx.default_fwi) / 1000);

	if (!state.fwt) state.fwt = 1;
}

/**
 *
 */
PRIVATE void
as3953_isr_3(void *InstancePtr)
{
	u8 ir_cause = 0;

	if (as3953_register_read(REG_MAIN_IRQ, &ir_cause))
		return;

	/*
	 * We do not know any details of this interrupt. This may happen.
	 */
	if (ir_cause == 0) return;

	/* Ignore POWER_UP and WAKE_UP for further notification but respect
	 * due to initialization aspects */
	if (ir_cause & (MAIN_IRQ_POWER_UP | MAIN_IRQ_WAKE_UP)) {
		// TODO maybe not useful
		u8 r;
		as3953_register_read(REG_MODE_DEFINE, &r);
		xil_printf("II [r] REG_MODE_DEFINE: 0x%02x\n", r);
		ir_cause &= ~(MAIN_IRQ_POWER_UP | MAIN_IRQ_WAKE_UP);
	}

	if (ir_cause & MAIN_IRQ_ACTIVE) {
		state_init_3();
		ir_cause &= ~MAIN_IRQ_ACTIVE;
	}

	if (ir_cause & MAIN_IRQ_AUX) {
		as3953_isr_handle_aux();
	}

	notify_from_isr(ir_cause);
}

/**
 * Prepare response context for sending state.
 */
PRIVATE void
fill_response_and_send_3( BitStream *response,// const Block *req,
		//enum Iso14443_Block bt,
		const u8 *data, u16 size)
{
	response->INF = data;
	response->INF_size = size;

	state_enter(SENDING);

	return;
}

/**
 *
 */
PRIVATE void
as3953_isr_handle_aux()
{
	u8 ir_aux, r;

	if (as3953_register_read(REG_AUX_IRQ, &ir_aux))
		return;

	switch(ir_aux) {
	case AUX_IRQ_FIFO_ERR:
		ir_aux &= ~AUX_IRQ_FIFO_ERR;
		as3953_register_read(REG_FIFO_STATUS_2, &r);
		xil_printf("[aux] fifo_err: 0x%02x\n", r);
	case 0x00:
		break;
	default:
		xil_printf("[aux] unhandled: 0x%02x\n", ir_aux);
		break;
	}

	return;
}

PRIVATE inline void notify_from_isr(u16 n) {
	state.notifications |= n;
}

PRIVATE inline void notify_state_machine(enum Notification n) {
	state.notifications |= n;
}

PRIVATE bool
has_been_notified(enum Notification n)
{
	if (state.notifications & n) {
		portENTER_CRITICAL();
		state.notifications &= ~n;
		portEXIT_CRITICAL();
		return true;
	}
	else
		return false;
}

PRIVATE void
state__wait_for_pcd()
{
	if (has_been_notified(START_RX))
		state_enter(RECEIVING);

	return;
}

PRIVATE void
state__receiving_3(BitStream *request)
{
	//static bool await_prologue;
	static bool request_complete;
	const Array *fifo_data = NULL;
	u8 offset;

	/* ------------------------------------------------------------------ */
	/* State initialization                                               */
	/* ------------------------------------------------------------------ */
	if (state.is_fresh) {
		//await_prologue = true;
		request_complete = false;
		bitstream_reset(request);
	}

	/* ------------------------------------------------------------------ */
	/* Notification handling                                              */
	/* ------------------------------------------------------------------ */
	if (has_been_notified(WTR_LVL)) {
		fifo_data = as3953_fifo_fetch(24);
		goto work;
	}
	else if (has_been_notified(END_RX)) {
		/* fetch remaining FIFO bytes */
		fifo_data = as3953_fifo_fetch(32);
		request_complete = true;

		/* XXX the original purpose was here to start a timer unblocking
		 * send method after Frame Delay Time has been expired. */
		// notify_state_machine(FDT_EXPIRED);
		goto work;
	}
	/* Nr (0): Do nothing, unless there have been data relevant interrupts */
	return;
	/* ------------------------------------------------------------------ */
	/* At least the real working part                                     */
	/* ------------------------------------------------------------------ */
work:
	offset = 0;
	/* Nr (1): Do not do anything on unavailable FIFO data */
	if (fifo_data == NULL || !fifo_data->length) goto error;
	/* Nr (2): handle prologue data field on each new ISO14443 block. */
	if (!__capdu) goto error;

	offset = 0;
	array_append(__capdu,
		     &fifo_data->v[offset],
		     fifo_data->length - offset);
	/* Nr (5): Transition into PROC_REQUEST state on each complete request */
	if (request_complete)
		state_enter(HANDLE_REQUEST);

	return;
error:
	state_enter(WAIT_FOR_PCD);
	return;
}


/**
 * In sending state we are going to send ISO14443 response parameter ASAP.
 *
 * @NOTE There is no validation about well-formed block structure, i.e. points
 * to an INF data part with length greater than zero the data will be send. In
 * addition each response will be sent in one part only. The caller needs to
 * care about local hardware limitations and PCD capabilities.
 *
 * @NOTE Keep in mind, that this method is part of messaging state machine. As
 * long as we don't do a state transition, this handler is getting recalled.
 */
PRIVATE void
state__sending_3(const BitStream *response)
{
	static bool start_blk = false;
	static u8   data_sent;
	bool        data_refill = false;
	u8          bytes_load = 0;

	/* a single shot to start sending response */
	if (state.is_fresh) {
		start_blk = true;
	}

	if ((data_sent < response->INF_size) && has_been_notified(WTR_LVL)) {
		data_refill = true;
	}
	/* catch FIFO underflow? */
	if (has_been_notified(END_TX)) {
		state_enter(WAIT_FOR_PCD);
		return;
	}

	if (start_blk) {
		data_sent = 0;

		as3953_fifo_reset();

		as3953_fifo_prepare_bits(response->INF_bits);

		if (response->INF) {
			bytes_load = as3953_fifo_add(
					response->INF,
					response->INF_size);
		}
	}
	/* Once a Water Level Interrupt has been raised we have to refill FIFO
	 * with with outstanding payload data. */
	else if (data_refill) {
		as3953_fifo_reset();
		bytes_load = as3953_fifo_add(
				response->INF + data_sent,
				MIN(24, response->INF_size - data_sent));
	}

	if (start_blk || bytes_load) {
		as3953_fifo_commit();
		data_sent += bytes_load;
		/* Trigger FIFO transmission once for each I-BLK. */
	}

	if (start_blk) {
		start_blk = false;
		as3953_fifo_push();
	}
}

PRIVATE void
state__handle_request_3(const BitStream *req, BitStream *response)
{
	bool rapdu_ready;
	u8 padding_bits;

	release_capdu_ctx();

	rapdu_ready = !wait_for_rapdu();

	/* TODO some time has been passed. May be there are some
	 * important notifications, e.g. START_RX? */

	if (rapdu_ready) {
		padding_bits = (__rapdu->length * 8) % 9;
		response->INF = __rapdu->v;
		response->INF_size = __rapdu->length;
		response->INF_bits = __rapdu->length * 8 - padding_bits;

		state_enter(SENDING);
	}
	else {
	/* error here */
		state_enter(WAIT_FOR_PCD);
	}
}

PUBLIC void
as3953_io_state_machine_3(void *param)
{
	static u8 _running = 0;

	BitStream request;
	BitStream response;

	if (_running++) halt();

	state_init_3();

	loop {
		switch (state.is) {
		case WAIT_FOR_PCD:
			state__wait_for_pcd();
			break;
		case RECEIVING:
			state__receiving_3(&request);
			break;
		case HANDLE_REQUEST:
			state__handle_request_3(&request, &response);
			break;
		case SENDING:
			state__sending_3(&response);
			break;
		}

		state.set_fresh = (state.is_fresh % 2) * 2;

		vTaskDelay(5);
	}
}

/**
 * Wait for at
 */
PRIVATE const struct array *
iso14443_recveive(void)
{
	static u8 _static = 0;

	if (_static++) return NULL;

	wait_for_capdu();

	array_reset(__rapdu);

	_static--;

	return __capdu;
}

/** You have one shot to send data. */
PRIVATE void
iso14443_3_send(void)
{
	static u8 _mutex = 0;

	if (_mutex++) return;

	array_reset(__capdu);

	/* If the PCD does not expect any data, you are out. I'm sorry. */
	submit_rapdu_3();

	_mutex--;

	return;
}


PUBLIC err_t
mod_iso14443_io_3()
{
	static const struct module_io _io = {
		.transmit     = iso14443_3_send,
		.receive      = iso14443_recveive,
	};

	int Status;
	u8  r;

	u8 conf_word[] = { 0x2C,
			0x01,// not 14443-4 compatible
			0x2F,// 14443 level 3 protocol mode, RX Bitstream mode
			0x80 // TX Bitstream mode

	};

	vSemaphoreCreateBinary(sem_capdu_ready);
	vSemaphoreCreateBinary(sem_rapdu_ready);
	vSemaphoreCreateBinary(sem_rapdu_send);

	if ((sem_capdu_ready == NULL)
	||  (sem_rapdu_ready == NULL)
	||  (sem_rapdu_send  == NULL))
		return E_SYSTEM;

	xQueueReset(sem_capdu_ready);
	xQueueReset(sem_rapdu_ready);
	xQueueReset(sem_rapdu_send);

	Status = as3953_spi_init(conf_word, as3953_isr_3);
	if (Status)
	{
		return E_HW;
	}

	// Do not call set default, as this will restore iso14443-4 mode
	//as3953_exec(CMD_SET_DEFAULT);
	as3953_exec(CMD_CLEAR_FIFO);

	as3953_register_read(REG_MASK_MAIN_IRQ, &r);
	xil_printf("II [r] REG_MASK_MAIN_IRQ: 0x%02x\n", r);

	as3953_register_write(REG_MODE_DEFINE, 0x1F);
	as3953_register_read(REG_MODE_DEFINE, &r);
	xil_printf("II [r] REG_MODE_DEFINE: 0x%02x\n", r);

	/*
	 * Enable the interrupt for the AS3953 device.
	 */
	vPortEnableInterrupt(AS3953_INTR_ID);

	/* Everything went well.  */
	return module_hal_io_set(&_io);
}
