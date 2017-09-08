/** @file Generic FIFO circular buffer implementation */

#ifndef CBUF_H_
#define CBUF_H_

#include <stdbool.h>
#include <stdint.h>

/** @brief Memory storage type for indexing specific elements of the buffer.
 * A buffer can only store the number of elements representable cbuf_idx_t. */
typedef uint_fast32_t cbuf_idx_t;

/** @brief Memory storage type for each individual element in a buffer. */
typedef uint8_t cbuf_elem_t;

/** @brief Memory storage element for a single FIFO buffer */
typedef struct {
	cbuf_elem_t *buf;	//!<- Bulk data for buffer contents
	cbuf_idx_t	head;	//!<- Index of last element that has been written to
	cbuf_idx_t	tail;	//!<- Index of last element that has been read from
	cbuf_idx_t	elems;	//!<- Total number of elements in buffer
} cbuf_s;

/** @brief Initialize a buffer
 * @param cb Pointer to the cbuf_s structure to initialize
 * @param pool Pointer to an array of cbuf_elem_t elements
 * @param elems The number of elements in pool */
void cbuf_new(cbuf_s * const cb, cbuf_elem_t * const pool, const cbuf_idx_t elems);

/** @brief Returns the number of used elements in a buffer cb */
cbuf_idx_t cbuf_elems_used(cbuf_s * const cb);

/** @brief Returns the number of free elements in a buffer cb */
cbuf_idx_t cbuf_elems_free(cbuf_s * const cb);

/** @brief Flushes (removes) all pending data from a buffer cb */
void cbuf_flush(cbuf_s * const cb);

/** @brief Writes an element data to a buffer cb if space is available.
 * If the buffer is full the input data will be discarded and the buffer will
 * be left unmodified
 * @return True on write success, false otherwise */
bool cbuf_write(cbuf_s * const cb, cbuf_elem_t const data);

/** @brief Reads an element from a buffer cb if data is available.
 * If the buffer is empty data will be unmodified
 * @return True on read success, false otherwise */
bool cbuf_read(cbuf_s * const cb, cbuf_elem_t * const data);

/** @brief Peeks at (reads, but does not mark as read) from a buffer cb
 * If the buffer is empty data will be unmodified
 * @return True on read success, false otherwise */
bool cbuf_peek(cbuf_s * const cb, cbuf_elem_t * const data);

#endif /* CBUF_H_ */
