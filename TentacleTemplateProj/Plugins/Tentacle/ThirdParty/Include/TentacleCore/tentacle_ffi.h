/*
 * Tentacle core - C ABI.
 *
 * Runs Tentacle in-process: the platform connections, the event pipeline, and
 * the engine session protocol, with no desktop app and no separate executable.
 * Load tentacle_ffi.dll (Windows), libtentacle_ffi.so (Linux), or
 * libtentacle_ffi.dylib (macOS), or link the static library.
 *
 * Usage:
 *   1. tcl_core_create once, when the game starts.
 *   2. tcl_connection_open for each session (a plugin normally holds one).
 *      A connection behaves like an engine WebSocket to the Tentacle app: the
 *      first frame must be a Session_Hello, the core answers Session_Welcome,
 *      and from then on it carries the ordinary engine message vocabulary.
 *   3. tcl_connection_send frames and tcl_connection_poll frames back, from
 *      the game's own tick or from a reader thread that sleeps in
 *      tcl_connection_wait. Nothing calls back into the host.
 *   4. tcl_connection_close each connection, then tcl_core_destroy.
 *
 * Frames are UTF-8 JSON objects, passed with an explicit length and never
 * NUL-terminated. Memory never changes hands: the host owns every buffer it
 * passes, and the core never returns memory for the host to free.
 *
 * Every function is safe to call with a null handle, and none ever unwinds or
 * aborts: a failure inside the core is reported through the return value.
 */

#ifndef TENTACLE_FFI_H
#define TENTACLE_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* An opaque running core. */
typedef struct TclCore TclCore;

/* An opaque connection to a core. */
typedef struct TclConnection TclConnection;

/* tcl_connection_send results. */
#define TCL_OK 0
#define TCL_ERR_NULL (-1)
#define TCL_ERR_NOT_UTF8 (-2)
#define TCL_ERR_CLOSED (-3)
#define TCL_ERR_INTERNAL (-100)

/* The core's version, as a NUL-terminated UTF-8 string with static lifetime. */
const char *tcl_version(void);

/*
 * Start a core. config_json is a UTF-8 JSON object of len bytes; NULL or 0
 * means the defaults. Returns NULL when the config is not valid JSON or the
 * core cannot start.
 */
TclCore *tcl_core_create(const uint8_t *config_json, size_t len);

/*
 * Stop a core and free it, closing every connection. Connections stay valid
 * handles (reporting closed) until each is closed. Must not race any other
 * call on the same core.
 */
void tcl_core_destroy(TclCore *core);

/* Open a connection. Its first frame must be a Session_Hello. */
TclConnection *tcl_connection_open(TclCore *core);

/*
 * Hand the core one frame of len bytes. Returns TCL_OK, TCL_ERR_NULL,
 * TCL_ERR_NOT_UTF8, TCL_ERR_CLOSED, or TCL_ERR_INTERNAL.
 */
int32_t tcl_connection_send(TclConnection *connection, const uint8_t *frame, size_t len);

/*
 * Copy the next frame into buf. Returns its length (always positive), 0 when
 * nothing is waiting, or the negated buffer size the next frame needs when
 * capacity is too small; that frame stays queued, so grow the buffer and poll
 * again. buf may be NULL when capacity is 0.
 */
int64_t tcl_connection_poll(TclConnection *connection, uint8_t *buf, size_t capacity);

/*
 * Block until a frame is waiting or the connection is closed, or until
 * timeout_ms passes. Returns 1 when there is a frame or a close to act on, 0
 * when the time ran out (or connection is NULL). Safe to call from a reader
 * thread while another thread sends.
 */
int32_t tcl_connection_wait(TclConnection *connection, uint32_t timeout_ms);

/*
 * 1 once either side has closed the connection (or it is NULL), 0 while it is
 * open. Frames queued before the core closed it can still be polled.
 */
int32_t tcl_connection_is_closed(TclConnection *connection);

/*
 * Close a connection, ending its session, and free the handle. The pointer is
 * invalid afterwards. Must not race any other call on the same connection.
 */
void tcl_connection_close(TclConnection *connection);

#ifdef __cplusplus
}
#endif

#endif /* TENTACLE_FFI_H */
