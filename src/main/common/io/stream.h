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
    along with FlexCOS; if not itcan be viewed here:
    http://www.freertos.org/a00114.html and also obtained by writing to AGSI,
    contact details for whom are available on the FlexCOS WEB site.

*/

#pragma once

#define stream_type(ptr, type, member) \
	container_of(ptr, type, member)

/* XXX use dedicated init function instead? */
#define ARRAY_STREAM_IN(buff, len) {               \
	.impl = { .ops = array_stream_in_impl },   \
	.data = (buff),                            \
	.bytes_left = (len) }

struct stream_out;
struct stream_out_ops;
struct stream_in;
struct stream_in_ops;

enum Stream_Status {
	STREAM_CLOSED = 0x00,
	STREAM_OPEN   = 0x01
};

/**
 *  Basic Output Stream Interface.
 *
 *  Implementing at least 'put' or 'write' method is mentdatory.
 *  Providing a dedicated 'close' operation or abitility to fetch bytes from
 *  other input streams is optional.
 *
 *  Do not call these methods directly. Use stream_* accesors instead.
 */
struct stream_out_ops {
	u32 (*put)(struct stream_out *, u8);
	u32 (*write)(struct stream_out *, u8 *, u32);
	u32 (*fetch_from)(struct stream_out *, struct stream_in *, u32);
	void (*close)(struct stream_out *);
};

/**
 *  Basic Input Stream Interface.
 *
 */
struct stream_in_ops {
	u32 (*get) (struct stream_in *, u8 *);
	u32 (*read)(struct stream_in *, u8 *, u32);
	u32 (*skip)(struct stream_in *, u32);
	u32 (*push_to)(struct stream_in *, struct stream_out *, u32);
};

struct stream_out {
	const struct stream_out_ops *ops;
	/* XXX do we need that? */
	u8 status;
};

struct stream_in {
	const struct stream_in_ops *ops;
};

struct array_stream_in {
	/* XXX what about a macro usage here, e.g. */
	/* #define __extend__STREAM_IN  struct stream_in istream[;] */
	struct stream_in impl;

	/* instance data */
	const u8  *data;
	u32 bytes_left;
};

extern const struct stream_in_ops *const array_stream_in_impl;

u32 stream_skip_native(struct stream_in *, u32);
u32 stream_read_native(struct stream_in *, u8 *, u32);
u32 stream_write_native(struct stream_out *, u8 *, u32);
u32 stream_transfer_native(struct stream_in *, struct stream_out *, u32);

u32 array_stream__get(struct stream_in *, u8 *);

/**
 *  Read a single character from an input stream.
 *
 *  @return One if character has been read, zero otherwise.
 */
static inline u32
stream_get(struct stream_in *is, u8 *c)
{
	return is->ops->get(is, c);
}

/**
 *  Directly transfer some bytes from an input stream to an output stream.
 *
 *  If any of both streams does supports streaming transfer we are going to use
 *  it, otherwise fall back to native transfer method.
 *
 *  @return Number of bytes, that have been written to output stream.
 *  
 *  @NOTE on implementations: Transfer will cause side effects on stream 
 *  synchronisation if not aware of output stream capacity, i.e. bytes already
 *  taken from an input stream that won't fit into the output stream are
 *  basically lost.
 *
 *  @XXX alternative name: stream_pipe() or stream_bypass()?
 */
static inline u32
stream_transfer(struct stream_in *is, struct stream_out *os, u32 bytes)
{
	if (os->ops->fetch_from)
		return os->ops->fetch_from(os, is, bytes);
	else if (is->ops->push_to)
		return is->ops->push_to(is, os, bytes);
	else
		return stream_transfer_native(is, os, bytes);
}

/**
 *  Discard some certain number of bytes from this stream.
 *
 *  @return Number of bytes that have been omited.
 */
static inline u32
stream_skip(struct stream_in *is, u32 bytes) {
	if (is->ops->skip)
		return is->ops->skip(is, bytes);
	else
		return stream_skip_native(is, bytes);
}

/**
 *  Read some bytes from an input stream into a buffer.
 *
 *  If the input stream supports reading into a buffer use its implementation,
 *  otherwise fallback to sequential character reading.
 *
 *  @return number of bytes that have been actually read. If this value equals
 *          input parameter everything is fine. 
 *  XXX     If not...well, don't know why. There is no errno or some similar
 *          variable righ now :(
 */
static inline u32
stream_read(struct stream_in *is, u8 *buff, u32 bytes)
{
	if (is->ops->read)
		return is->ops->read(is, buff, bytes);
	else
		return stream_read_native(is, buff, bytes);
}

/**
 *  Write one single character to a stream.
 *
 *  @return one if the character has been written, otherwise 0.
 */
static inline u32
stream_put(struct stream_out *os, u8 c)
{
	if (os->ops->put)
		return os->ops->put(os, c);
	else if (os->ops->write)
		return os->ops->write(os, &c, 1);
	else
		return 0;
}

static inline u32
stream_write(struct stream_out *os, u8 *data, u32 bytes)
{
	if (os->ops->write)
		return os->ops->write(os, data, bytes);
	else
		return stream_write_native(os, data, bytes);
}

/**
 *  Write a two byte value onto a stream in big endianess format.
 *
 *  @return number of bytes that have been put, usually two on success.
 */
static inline u32
stream_put_word(struct stream_out *os, u16 w)
{
	return os->ops->put(os, (w >> 8) & 0xFF) +
	       os->ops->put(os, w & 0xFF);
}

static inline void
stream_close(struct stream_out *os)
{
	if (os->ops->close) os->ops->close(os);
}
