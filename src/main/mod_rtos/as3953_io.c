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
 * as3953_io.c
 *
 * Created Apr, 2013
 *     Author: Alexander Münn
 */

#include <const.h>
#include <types.h>
#include <hw.h>

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

PRIVATE xSemaphoreHandle sem_capdu_ready;
PRIVATE xSemaphoreHandle sem_rapdu_ready;
PRIVATE xSemaphoreHandle sem_rapdu_send;

enum Notification;
enum ComState;
enum RequestType;

PRIVATE void as3953_isr(void *InstancePtr);
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
submit_rapdu(void)
{
	if (!state.pending_i_blk_response) return;
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
state_init() {
	memset(&state, 0x00, sizeof(state));
	xil_printf("DD state_init\n");

	state_enter(WAIT_FOR_PCD);

	state.fwt = (iso14443_fwt_in_us(dev_ctx.default_fwi) / 1000);

	if (!state.fwt) state.fwt = 1;

	state.block_number = 1;
}

/**
 *
 */
PRIVATE void
as3953_isr(void *InstancePtr)
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
		ir_cause &= ~(MAIN_IRQ_POWER_UP | MAIN_IRQ_WAKE_UP);
	}

	if (ir_cause & MAIN_IRQ_ACTIVE) {
		state_init();
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
fill_response_and_send( Block *response, const Block *req,
		enum Iso14443_Block bt, const u8 *data, u16 size)
{
	response->pcb_plain = 0x02;
	response->pcb.type = bt;
	response->pcb.with_cid = req->pcb.with_cid;

	if (bt != S_WTX)
		response->pcb.block_number = state.block_number;

	/* FIXME get CID from dev_ctx wich in turn gets CID from RATS register */
	response->cid = req->cid;

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
__transfer_rapdu_continue(Block *response, const Block *req)
{
	enum Iso14443_Block blk;
	u16                 bytes_left;
	u16                 max_INF_size;

	state.transfer.done += state.transfer.bytes;

	/* Total I-Blk size is limited by maximum accepted frame size of PCD (FSD),
	 * i.e. the portion of RAPDU buffer we can put in one information field
	 * is limited by FSD minus prologue field size.
	 *
	 * FIXME magic number
	 * XXX   static value gives some frame bytes away, if optional prologue
	 *       fields (CID, NAD) are unused.
	 */
	max_INF_size = dev_ctx.max_frame_size - 3;

	bytes_left = __rapdu->length - state.transfer.done;

	/* Use I-Block chaining if remaining RAPDU bytes won't fit into one
	 * I-Block. */
	blk = bytes_left > max_INF_size ? I_BLK_C : I_BLK;

	state.transfer.bytes = MIN(bytes_left, max_INF_size);

	fill_response_and_send(response, req, blk,
			__rapdu->v + state.transfer.done,
			state.transfer.bytes);
}

PRIVATE void
__transfer_rapdu_start(Block *response, const Block *req)
{
	state.transfer.done = 0;
	state.transfer.bytes = 0;

	__transfer_rapdu_continue(response, req);
}

PRIVATE void
state__wait_for_pcd()
{
	if (has_been_notified(START_RX))
		state_enter(RECEIVING);

	return;
}

PRIVATE void
state__receiving(Block *request)
{
	static bool await_prologue;
	static bool request_complete;
	const Array *fifo_data = NULL;
	u8 offset;

	/* ------------------------------------------------------------------ */
	/* State initialization                                               */
	/* ------------------------------------------------------------------ */
	if (state.is_fresh) {
		await_prologue = true;
		request_complete = false;
		iso14443_block_reset(request);
	}

	/* ------------------------------------------------------------------ */
	/* Notification handling                                              */
	/* ------------------------------------------------------------------ */
	if (has_been_notified(WTR_LVL)) {
		fifo_data = as3953_fifo_fetch(24);
		goto work;
	}
	else if (has_been_notified(END_RX)) {
		/* fetch remaining fifo bytes */
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
	/* Nr (1): Do not do anything on unavailable fifo data */
	if (fifo_data == NULL || !fifo_data->length) goto error;
	/* Nr (2): handle prologue data field on each new ISO14443 block. */
	else if (await_prologue) {
		await_prologue = false;
		offset = iso14443_parse_request(
				request, fifo_data, &(state.block_number));

		if (!offset || request->type == INVALID) goto error;
	}
	/* Nr (3): Copy payload of non empty I-Blocks into CAPDU buffer */
	if ((request->type == CAPDU_COMPLETE)
	||  (request->type == CAPDU_TRANSFER))
	{
		if (!__capdu) goto error;

		array_append(__capdu,
		             &fifo_data->v[offset],
		             fifo_data->length - offset);
	}
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
state__sending(const Block *response)
{
	static bool start_blk = false;
	static u8   data_sent;
	bool        data_refill = false;
	u8          bytes_load = 0;

	/* a single shot to start sending response */
	if (state.is_fresh) {
		start_blk = true;
	}

	if (has_been_notified(WTR_LVL) && (data_sent < response->INF_size)) {
		data_refill = true;
	}
	/* catch fifo underflow? */
	if (has_been_notified(END_TX)) {
		state_enter(WAIT_FOR_PCD);
		return;
	}

	if (start_blk) {
		data_sent = 0;

		as3953_fifo_reset();

		as3953_fifo_add((u8 *)response, 1
				+ response->pcb.with_cid
				+ response->pcb.with_nad);
		as3953_fifo_prepare(as3953_fifo->length
				+ (response->INF ? response->INF_size : 0));

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
		/* Trigger fifo transmission once for each I-BLK. */
	}

	if (start_blk) {
		start_blk = false;
		as3953_fifo_push();
	}
}

PRIVATE void
state__handle_request(const Block *req, Block *response)
{
	static const u8 wtxm = 0x01;
	bool rapdu_ready;

	switch (req->type) {
	case I_BLK_PRESENCE:
		/* confirm presence check with an empty I-block */
		fill_response_and_send(response, req, I_BLK, NULL, 0);
		break;
	case R_NAK_PRESENCE:
	case CAPDU_TRANSFER:
		fill_response_and_send(response, req, R_ACK, NULL, 0);
		break;
	case RAPDU_CONTINUE:
		__transfer_rapdu_continue(response, req);
		break;
	case R_BLK_RESEND:
		state_enter(SENDING);
		break;
	case S_WTX_CONFIRM:
		// update state.fwt and fall through to CAPDU_COMPLETE
	case CAPDU_COMPLETE:
		if (state.is_fresh && req->type != S_WTX_CONFIRM) {
			/* Unblock application waiting for Command APDU */
			state.pending_i_blk_response = true;
			release_capdu_ctx();
		}

		rapdu_ready = !wait_for_rapdu();

		/* TODO some time has been passed. May be there are some
		 * important notifications, e.g. START_RX? */

		if (rapdu_ready) {
			state.pending_i_blk_response = false;
			__transfer_rapdu_start(response, req);
		}
		else {
			fill_response_and_send(response, req, S_WTX,
					&wtxm, 1);
		}
		break;
	default:
		/* error here */
		state_enter(WAIT_FOR_PCD);
		break;
	}
}

PUBLIC void
as3953_io_state_machine(void *param)
{
	static u8 _running = 0;

	Block request;
	Block response;

	if (_running++) halt();

	state_init();

	loop {
		switch (state.is) {
		case WAIT_FOR_PCD:
			state__wait_for_pcd();
			break;
		case RECEIVING:
			state__receiving(&request);
			break;
		case HANDLE_REQUEST:
			state__handle_request(&request, &response);
			break;
		case SENDING:
			state__sending(&response);
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

	array_truncate(__rapdu);

	_static--;

	return __capdu;
}

/** You have one shot to send data. */
PRIVATE void
iso14443_send(void)
{
	static u8 _mutex = 0;

	if (_mutex++) return;

	array_truncate(__capdu);

	/* If the PCD does not expect any data, you are out. I'm sorry. */
	submit_rapdu();

	_mutex--;

	return;
}

PUBLIC err_t
mod_iso14443_io()
{
	static const struct module_io _io = {
		.transmit     = iso14443_send,
		.receive      = iso14443_recveive,
	};

	int Status;
	u8  r;

	u8 conf_word[] = { 0x2c, 0x00, 0x00, 0x00 };

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

	Status = as3953_spi_init(conf_word, as3953_isr);
	if (Status)
	{
		return E_HW;
	}

	as3953_exec(CMD_SET_DEFAULT);
	as3953_exec(CMD_CLEAR_FIFO);

	as3953_register_read(REG_MASK_MAIN_IRQ, &r);
	xil_printf("II [r] REG_MASK_MAIN_IRQ: 0x%02x\n", r);

	/* Everything went well.  */
	return module_hal_io_set(&_io);
}

