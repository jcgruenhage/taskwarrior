// TaskChampion
//
// This file defines the C interface to libtaskchampion.  This is a thin wrapper around the Rust
// `taskchampion` crate.  Refer to the documentation for that crate at
// https://docs.rs/taskchampion/latest/taskchampion/ for API details.  The comments in this file
// focus mostly on the low-level details of passing values to and from TaskChampion.
//
// # Overview
//
// This library defines four major types used to interact with the API, that map directly to Rust
// types.
//
//  * TCReplica - see https://docs.rs/taskchampion/latest/taskchampion/struct.Replica.html
//  * TCTask - see https://docs.rs/taskchampion/latest/taskchampion/struct.Task.html
//  * TCServer - see https://docs.rs/taskchampion/latest/taskchampion/trait.Server.html
//  * TCWorkingSet - see https://docs.rs/taskchampion/latest/taskchampion/struct.WorkingSet.html
//
// It also defines a few utility types:
//
//  * TCString - a wrapper around both C (NUL-terminated) and Rust (always utf-8) strings.
//  * TC…List - a list of objects represented as a C array
//  * see below for the remainder
//
// # Safety
//
// Each type contains specific instructions to ensure memory safety.  The general rules are as
// follows.
//
// No types in this library are threadsafe.  All values should be used in only one thread for their
// entire lifetime.  It is safe to use unrelated values in different threads (for example,
// different threads may use different TCReplica values concurrently).
//
// ## Pass by Pointer
//
// Several types such as TCReplica and TCString are "opaque" types and always handled as pointers
// in C. The bytes these pointers address are private to the Rust implementation and must not be
// accessed from C.
//
// Pass-by-pointer values have exactly one owner, and that owner is responsible for freeing the
// value (using a `tc_…_free` function), or transferring ownership elsewhere.  Except where
// documented otherwise, when a value is passed to C, ownership passes to C as well.  When a value
// is passed to Rust, ownership stays with the C code.  The exception is TCString, ownership of
// which passes to Rust when it is used as a function argument.
//
// The limited circumstances where one value must not outlive another, due to pointer references
// between them, are documented below.
//
// ## Pass by Value
//
// Types such as TCUuid and TC…List are passed by value, and contain fields that are accessible
// from C.  C code is free to access the content of these types in a _read_only_ fashion.
//
// Pass-by-value values that contain pointers also have exactly one owner, responsible for freeing
// the value or transferring ownership.  The tc_…_free functions for these types will replace the
// pointers with NULL to guard against use-after-free errors.  The interior pointers in such values
// should never be freed directly (for example, `tc_string_free(tcuda.value)` is an error).
//
// TCUuid is a special case, because it does not contain pointers.  It can be freely copied and
// need not be freed.
//
// ## Lists
//
// Lists are a special kind of pass-by-value type.  Each contains `len` and `items`, where `items`
// is an array of length `len`.  Lists, and the values in the `items` array, must be treated as
// read-only.  On return from an API function, a list's ownership is with the C caller, which must
// eventually free the list.  List data must be freed with the `tc_…_list_free` function.  It is an
// error to free any value in the `items` array of a list.

#ifndef TASKCHAMPION_H
#define TASKCHAMPION_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif  // __cplusplus

// ***** TCResult *****
//
// A result from a TC operation.  Typically if this value is TC_RESULT_ERROR,
// the associated object's `tc_.._error` method will return an error message.
enum TCResult
#ifdef __cplusplus
    : int32_t
#endif  // __cplusplus
{
  TC_RESULT_ERROR = -1,
  TC_RESULT_OK = 0,
};
#ifndef __cplusplus
typedef int32_t TCResult;
#endif  // __cplusplus

// ***** TCString *****
//
// TCString supports passing strings into and out of the TaskChampion API.
//
// # Rust Strings and C Strings
//
// A Rust string can contain embedded NUL characters, while C considers such a character to mark
// the end of a string.  Strings containing embedded NULs cannot be represented as a "C string"
// and must be accessed using `tc_string_content_and_len` and `tc_string_clone_with_len`.  In
// general, these two functions should be used for handling arbitrary data, while more convenient
// forms may be used where embedded NUL characters are impossible, such as in static strings.
//
// # UTF-8
//
// TaskChampion expects all strings to be valid UTF-8. `tc_string_…` functions will fail if given
// a `*TCString` containing invalid UTF-8.
//
// # Safety
//
// The `ptr` field may be checked for NULL, where documentation indicates this is possible.  All
// other fields in a TCString are private and must not be used from C.  They exist in the struct
// to ensure proper allocation and alignment.
//
// When a `TCString` appears as a return value or output argument, ownership is passed to the
// caller.  The caller must pass that ownership back to another function or free the string.
//
// Any function taking a `TCString` requires:
//  - the pointer must not be NUL;
//  - the pointer must be one previously returned from a tc_… function; and
//  - the memory referenced by the pointer must never be modified by C code.
//
// Unless specified otherwise, TaskChampion functions take ownership of a `TCString` when it is
// given as a function argument, and the caller must not use or free TCStrings after passing them
// to such API functions.
//
// A TCString with a NULL `ptr` field need not be freed, although tc_free_string will not fail
// for such a value.
//
// TCString is not threadsafe.
typedef struct TCString {
  void *ptr;    // opaque, but may be checked for NULL
  size_t _u1;   // reserved
  size_t _u2;   // reserved
  uint8_t _u3;  // reserved
} TCString;

// Create a new TCString referencing the given C string.  The C string must remain valid and
// unchanged until after the TCString is freed.  It's typically easiest to ensure this by using a
// static string.
//
// NOTE: this function does _not_ take responsibility for freeing the given C string.  The
// given string can be freed once the TCString referencing it has been freed.
//
// For example:
//
// ```text
// char *url = get_item_url(..); // dynamically allocate C string
// tc_task_annotate(task, tc_string_borrow(url)); // TCString created, passed, and freed
// free(url); // string is no longer referenced and can be freed
// ```
EXTERN_C struct TCString tc_string_borrow(const char *cstr);

// Create a new TCString by cloning the content of the given C string.  The resulting TCString
// is independent of the given string, which can be freed or overwritten immediately.
EXTERN_C struct TCString tc_string_clone(const char *cstr);

// Create a new TCString containing the given string with the given length. This allows creation
// of strings containing embedded NUL characters.  As with `tc_string_clone`, the resulting
// TCString is independent of the passed buffer, which may be reused or freed immediately.
//
// The length should _not_ include any trailing NUL.
//
// The given length must be less than half the maximum value of usize.
EXTERN_C struct TCString tc_string_clone_with_len(const char *buf, size_t len);

// Get the content of the string as a regular C string.  The given string must be valid.  The
// returned value is NULL if the string contains NUL bytes or (in some cases) invalid UTF-8.  The
// returned C string is valid until the TCString is freed or passed to another TC API function.
//
// In general, prefer [`tc_string_content_with_len`] except when it's certain that the string is
// valid and NUL-free.
//
// This function takes the TCString by pointer because it may be modified in-place to add a NUL
// terminator.  The pointer must not be NULL.
//
// This function does _not_ take ownership of the TCString.
EXTERN_C const char *tc_string_content(const struct TCString *tcstring);

// Get the content of the string as a pointer and length.  The given string must not be NULL.
// This function can return any string, even one including NUL bytes or invalid UTF-8.  The
// returned buffer is valid until the TCString is freed or passed to another TaskChampio
// function.
//
// This function takes the TCString by pointer because it may be modified in-place to add a NUL
// terminator.  The pointer must not be NULL.
//
// This function does _not_ take ownership of the TCString.
EXTERN_C const char *tc_string_content_with_len(const struct TCString *tcstring, size_t *len_out);

// Free a TCString.  The given string must not be NULL.  The string must not be used
// after this function returns, and must not be freed more than once.
EXTERN_C void tc_string_free(struct TCString *tcstring);

// ***** TCStringList *****
//
// TCStringList represents a list of strings.
//
// The content of this struct must be treated as read-only.
typedef struct TCStringList {
  // number of strings in items
  size_t len;
  // reserved
  size_t _u1;
  // TCStringList representing each string. These remain owned by the TCStringList instance and will
  // be freed by tc_string_list_free.  This pointer is never NULL for a valid TCStringList, and the
  // *TCStringList at indexes 0..len-1 are not NULL.
  struct TCString *items;
} TCStringList;

// Free a TCStringList instance.  The instance, and all TCStringList it contains, must not be used
// after this call.
//
// When this call returns, the `items` pointer will be NULL, signalling an invalid TCStringList.
EXTERN_C void tc_string_list_free(struct TCStringList *tcstrings);

// ***** TCUuid *****
//
// TCUuid is used as a task identifier.  Uuids do not contain any pointers and need not be freed.
// Uuids are typically treated as opaque, but the bytes are available in big-endian format.
typedef struct TCUuid {
  uint8_t bytes[16];
} TCUuid;

// Length, in bytes, of the string representation of a UUID (without NUL terminator)
#define TC_UUID_STRING_BYTES 36

// Parse the given string as a UUID.  Returns TC_RESULT_ERROR on parse failure or if the given
// string is not valid.
EXTERN_C TCResult tc_uuid_from_str(struct TCString s, struct TCUuid *uuid_out);

// Create a new, randomly-generated UUID.
EXTERN_C struct TCUuid tc_uuid_new_v4(void);

// Create a new UUID with the nil value.
EXTERN_C struct TCUuid tc_uuid_nil(void);

// Write the string representation of a TCUuid into the given buffer, which must be
// at least TC_UUID_STRING_BYTES long.  No NUL terminator is added.
EXTERN_C void tc_uuid_to_buf(struct TCUuid tcuuid, char *buf);

// Return the hyphenated string representation of a TCUuid.  The returned string
// must be freed with tc_string_free.
EXTERN_C struct TCString tc_uuid_to_str(struct TCUuid tcuuid);

// TCUuidList represents a list of uuids.
//
// The content of this struct must be treated as read-only.
typedef struct TCUuidList {
  // number of uuids in items
  size_t len;
  // reserved
  size_t _u1;
  // Array of uuids. This pointer is never NULL for a valid TCUuidList.
  struct TCUuid *items;
} TCUuidList;

// Free a TCUuidList instance.  The instance, and all TCUuids it contains, must not be used after
// this call.
//
// When this call returns, the `items` pointer will be NULL, signalling an invalid TCUuidList.
EXTERN_C void tc_uuid_list_free(struct TCUuidList *tcuuids);

// ***** TCAnnotation *****
//
// TCAnnotation contains the details of an annotation.
//
// # Safety
//
// An annotation must be initialized from a tc_.. function, and later freed
// with `tc_annotation_free` or `tc_annotation_list_free`.
//
// Any function taking a `*TCAnnotation` requires:
//  - the pointer must not be NUL;
//  - the pointer must be one previously returned from a tc_… function;
//  - the memory referenced by the pointer must never be modified by C code; and
//  - ownership transfers to the called function, and the value must not be used
//    after the call returns.  In fact, the value will be zeroed out to ensure this.
//
// TCAnnotations are not threadsafe.
typedef struct TCAnnotation {
  // Time the annotation was made.  Must be nonzero.
  time_t entry;
  // Content of the annotation.  Must not be NULL.
  TCString description;
} TCAnnotation;

// Free a TCAnnotation instance.  The instance, and the TCString it contains, must not be used
// after this call.
EXTERN_C void tc_annotation_free(struct TCAnnotation *tcann);

// ***** TCAnnotationList *****
//
// TCAnnotationList represents a list of annotations.
//
// The content of this struct must be treated as read-only.
typedef struct TCAnnotationList {
  // number of annotations in items
  size_t len;
  // reserved
  size_t _u1;
  // Array of annotations. These remain owned by the TCAnnotationList instance and will be freed by
  // tc_annotation_list_free.  This pointer is never NULL for a valid TCAnnotationList.
  struct TCAnnotation *items;
} TCAnnotationList;

// Free a TCAnnotationList instance.  The instance, and all TCAnnotations it contains, must not be
// used after this call.
//
// When this call returns, the `items` pointer will be NULL, signalling an invalid TCAnnotationList.
EXTERN_C void tc_annotation_list_free(struct TCAnnotationList *tcanns);

// ***** TCUda *****
//
// TCUda contains the details of a UDA.
typedef struct TCUda {
  // Namespace of the UDA.  For legacy UDAs, this may have a NULL ptr field.
  struct TCString ns;
  // UDA key.  Must not be NULL.
  struct TCString key;
  // Content of the UDA.  Must not be NULL.
  struct TCString value;
} TCUda;

// Free a TCUda instance.  The instance, and the TCStrings it contains, must not be used
// after this call.
EXTERN_C void tc_uda_free(struct TCUda *tcuda);

// ***** TCUdaList *****
//
// TCUdaList represents a list of UDAs.
//
// The content of this struct must be treated as read-only.
typedef struct TCUdaList {
  // number of UDAs in items
  size_t len;
  // reserved
  size_t _u1;
  // Array of UDAs. These remain owned by the TCUdaList instance and will be freed by
  // tc_uda_list_free.  This pointer is never NULL for a valid TCUdaList.
  struct TCUda *items;
} TCUdaList;

// Free a TCUdaList instance.  The instance, and all TCUdas it contains, must not be used after
// this call.
//
// When this call returns, the `items` pointer will be NULL, signalling an invalid TCUdaList.
EXTERN_C void tc_uda_list_free(struct TCUdaList *tcudas);

// ***** TCKV *****
//
// TCKV contains a key/value pair that is part of a task.
//
// Neither key nor value are ever NULL.  They remain owned by the TCKV and
// will be freed when it is freed with tc_kv_list_free.
typedef struct TCKV {
  struct TCString key;
  struct TCString value;
} TCKV;

// ***** TCKVList *****
//
// TCKVList represents a list of key/value pairs.
//
// The content of this struct must be treated as read-only.
typedef struct TCKVList {
  // number of key/value pairs in items
  size_t len;
  // reserved
  size_t _u1;
  // Array of TCKV's. These remain owned by the TCKVList instance and will be freed by
  // tc_kv_list_free.  This pointer is never NULL for a valid TCKVList.
  struct TCKV *items;
} TCKVList;

// Free a TCKVList instance.  The instance, and all TCKVs it contains, must not be used after
// this call.
//
// When this call returns, the `items` pointer will be NULL, signalling an invalid TCKVList.
EXTERN_C void tc_kv_list_free(struct TCKVList *tckvs);

// ***** TCStatus *****
//
// The status of a task, as defined by the task data model.
#ifdef __cplusplus
typedef enum TCStatus : int32_t {
#else   // __cplusplus
typedef int32_t TCStatus;
enum TCStatus {
#endif  // __cplusplus
  TC_STATUS_PENDING = 0,
  TC_STATUS_COMPLETED = 1,
  TC_STATUS_DELETED = 2,
  TC_STATUS_RECURRING = 3,
  // Unknown signifies a status in the task DB that was not
  // recognized.
  TC_STATUS_UNKNOWN = -1,
#ifdef __cplusplus
} TCStatus;
#else   // __cplusplus
};
#endif  // __cplusplus

// ***** TCServer *****
//
// TCServer represents an interface to a sync server.  Aside from new and free, a server
// has no C-accessible API, but is designed to be passed to `tc_replica_sync`.
//
// ## Safety
//
// TCServer are not threadsafe, and must not be used with multiple replicas simultaneously.
typedef struct TCServer TCServer;

// Create a new TCServer that operates locally (on-disk).  See the TaskChampion docs for the
// description of the arguments.
//
// On error, a string is written to the error_out parameter (if it is not NULL) and NULL is
// returned.  The caller must free this string.
//
// The server must be freed after it is used - tc_replica_sync does not automatically free it.
EXTERN_C struct TCServer *tc_server_new_local(struct TCString server_dir,
                                              struct TCString *error_out);

// Create a new TCServer that connects to a remote server.  See the TaskChampion docs for the
// description of the arguments.
//
// On error, a string is written to the error_out parameter (if it is not NULL) and NULL is
// returned.  The caller must free this string.
//
// The server must be freed after it is used - tc_replica_sync does not automatically free it.
EXTERN_C struct TCServer *tc_server_new_sync(struct TCString url, struct TCUuid client_id,
                                             struct TCString encryption_secret,
                                             struct TCString *error_out);

// Create a new TCServer that connects to the Google Cloud Platform.  See the TaskChampion docs
// for the description of the arguments.
//
// On error, a string is written to the error_out parameter (if it is not NULL) and NULL is
// returned.  The caller must free this string.
//
// The server must be freed after it is used - tc_replica_sync does not automatically free it.
EXTERN_C struct TCServer *tc_server_new_gcp(struct TCString bucket, struct TCString credential_path,
                                            struct TCString encryption_secret,
                                            struct TCString *error_out);

// Free a server.  The server may not be used after this function returns and must not be freed
// more than once.
EXTERN_C void tc_server_free(struct TCServer *server);

// ***** TCReplica *****
//
// A replica represents an instance of a user's task data, providing an easy interface
// for querying and modifying that data.
//
// # Error Handling
//
// When a `tc_replica_..` function that returns a TCResult returns TC_RESULT_ERROR, then
// `tc_replica_error` will return the error message.
//
// # Safety
//
// The `*TCReplica` returned from `tc_replica_new…` functions is owned by the caller and
// must later be freed to avoid a memory leak.
//
// Any function taking a `*TCReplica` requires:
//  - the pointer must not be NUL;
//  - the pointer must be one previously returned from a tc_… function;
//  - the memory referenced by the pointer must never be modified by C code; and
//  - except for `tc_replica_free`, ownership of a `*TCReplica` remains with the caller.
//
// Once passed to `tc_replica_free`, a `*TCReplica` becomes invalid and must not be used again.
//
// TCReplicas are not threadsafe.
typedef struct TCReplica TCReplica;

// ***** TCReplicaOpType *****
enum TCReplicaOpType
#ifdef __cplusplus
    : uint32_t
#endif  // __cplusplus
{
  Create = 0,
  Delete = 1,
  Update = 2,
  UndoPoint = 3,
};
#ifndef __cplusplus
typedef uint32_t TCReplicaOpType;
#endif  // __cplusplus

// ***** TCReplicaOp *****
struct TCReplicaOp {
  TCReplicaOpType operation_type;
  void *inner;
};

typedef struct TCReplicaOp TCReplicaOp;

// ***** TCReplicaOpList *****
struct TCReplicaOpList {
  struct TCReplicaOp *items;
  size_t len;
  size_t capacity;
};

typedef struct TCReplicaOpList TCReplicaOpList;

// Create a new TCReplica with an in-memory database.  The contents of the database will be
// lost when it is freed with tc_replica_free.
EXTERN_C struct TCReplica *tc_replica_new_in_memory(void);

// Create a new TCReplica with an on-disk database having the given filename.  On error, a string
// is written to the error_out parameter (if it is not NULL) and NULL is returned.  The caller
// must free this string.
EXTERN_C struct TCReplica *tc_replica_new_on_disk(struct TCString path, bool create_if_missing,
                                                  struct TCString *error_out);

// Add an UndoPoint, if one has not already been added by this Replica.  This occurs automatically
// when a change is made.  The `force` flag allows forcing a new UndoPoint even if one has already
// been created by this Replica, and may be useful when a Replica instance is held for a long time
// and used to apply more than one user-visible change.
EXTERN_C TCResult tc_replica_add_undo_point(struct TCReplica *rep, bool force);

// Get a list of all uuids for tasks in the replica.
//
// Returns a TCUuidList with a NULL items field on error.
//
// The caller must free the UUID list with `tc_uuid_list_free`.
EXTERN_C struct TCUuidList tc_replica_all_task_uuids(struct TCReplica *rep);

// Get a list of all tasks in the replica.
//
// Returns a TCTaskList with a NULL items field on error.
EXTERN_C struct TCTaskList tc_replica_all_tasks(struct TCReplica *rep);

// Undo local operations in storage.
//
// If undone_out is not NULL, then on success it is set to 1 if operations were undone, or 0 if
// there are no operations that can be done.
EXTERN_C TCResult tc_replica_commit_undo_ops(struct TCReplica *rep, TCReplicaOpList tc_undo_ops,
                                             int32_t *undone_out);

// Delete a task.  The task must exist.  Note that this is different from setting status to
// Deleted; this is the final purge of the task.
//
// Deletion may interact poorly with modifications to the same task on other replicas. For
// example, if a task is deleted on replica 1 and its description modified on replica 1, then
// after both replicas have fully synced, the resulting task will only have a `description`
// property.
EXTERN_C TCResult tc_replica_delete_task(struct TCReplica *rep, struct TCUuid tcuuid);

// Get the latest error for a replica, or a string with NULL ptr if no error exists.  Subsequent
// calls to this function will return NULL.  The rep pointer must not be NULL.  The caller must
// free the returned string.
EXTERN_C struct TCString tc_replica_error(struct TCReplica *rep);

// Expire old, deleted tasks.
//
// Expiration entails removal of tasks from the replica. Any modifications that occur after
// the deletion (such as operations synchronized from other replicas) will do nothing.
//
// Tasks are eligible for expiration when they have status Deleted and have not been modified
// for 180 days (about six months). Note that completed tasks are not eligible.
EXTERN_C TCResult tc_replica_expire_tasks(struct TCReplica *rep);

// Get an existing task by its UUID.
//
// Returns NULL when the task does not exist, and on error.  Consult tc_replica_error
// to distinguish the two conditions.
EXTERN_C struct TCTask *tc_replica_get_task(struct TCReplica *rep, struct TCUuid tcuuid);

// Return undo local operations until the most recent UndoPoint.
EXTERN_C TCReplicaOpList tc_replica_get_undo_ops(struct TCReplica *rep);

// Create a new task.  The task must not already exist.
//
// Returns the task, or NULL on error.
EXTERN_C struct TCTask *tc_replica_import_task_with_uuid(struct TCReplica *rep,
                                                         struct TCUuid tcuuid);

// Create a new task.  The task must not already exist.
//
// Returns the task, or NULL on error.
EXTERN_C struct TCTask *tc_replica_new_task(struct TCReplica *rep, enum TCStatus status,
                                            struct TCString description);

// Get the number of local, un-synchronized operations (not including undo points), or -1 on
// error.
EXTERN_C int64_t tc_replica_num_local_operations(struct TCReplica *rep);

// Get the number of undo points (number of undo calls possible), or -1 on error.
EXTERN_C int64_t tc_replica_num_undo_points(struct TCReplica *rep);

// Rebuild this replica's working set, based on whether tasks are pending or not.  If `renumber`
// is true, then existing tasks may be moved to new working-set indices; in any case, on
// completion all pending tasks are in the working set and all non- pending tasks are not.
EXTERN_C TCResult tc_replica_rebuild_working_set(struct TCReplica *rep, bool renumber);

// Synchronize this replica with a server.
//
// The `server` argument remains owned by the caller, and must be freed explicitly.
EXTERN_C TCResult tc_replica_sync(struct TCReplica *rep, struct TCServer *server,
                                  bool avoid_snapshots);

// Get the current working set for this replica.  The resulting value must be freed
// with tc_working_set_free.
//
// Returns NULL on error.
EXTERN_C struct TCWorkingSet *tc_replica_working_set(struct TCReplica *rep);

// Free a replica.  The replica may not be used after this function returns and must not be freed
// more than once.
EXTERN_C void tc_replica_free(struct TCReplica *rep);

// Return description field of old task field of ReplicaOp.
EXTERN_C struct TCString tc_replica_op_get_old_task_description(struct TCReplicaOp *op);

// Return old value field of ReplicaOp.
EXTERN_C struct TCString tc_replica_op_get_old_value(struct TCReplicaOp *op);

// Return property field of ReplicaOp.
EXTERN_C struct TCString tc_replica_op_get_property(struct TCReplicaOp *op);

// Return timestamp field of ReplicaOp.
EXTERN_C struct TCString tc_replica_op_get_timestamp(struct TCReplicaOp *op);

// Return uuid field of ReplicaOp.
EXTERN_C struct TCString tc_replica_op_get_uuid(struct TCReplicaOp *op);

// Return value field of ReplicaOp.
EXTERN_C struct TCString tc_replica_op_get_value(struct TCReplicaOp *op);

// Free a vector of ReplicaOp.  The vector may not be used after this function returns and must not
// be freed more than once.
EXTERN_C void tc_replica_op_list_free(struct TCReplicaOpList *oplist);

// ***** TCTask *****
//
// A task, as publicly exposed by this library.
//
// A task begins in "immutable" mode.  It must be converted to "mutable" mode
// to make any changes, and doing so requires exclusive access to the replica
// until the task is freed or converted back to immutable mode.
//
// An immutable task carries no reference to the replica that created it, and can be used until it
// is freed or converted to a TaskMut.  A mutable task carries a reference to the replica and
// must be freed or made immutable before the replica is freed.
//
// All `tc_task_..` functions taking a task as an argument require that it not be NULL.
//
// When a `tc_task_..` function that returns a TCResult returns TC_RESULT_ERROR, then
// `tc_task_error` will return the error message.
//
// # Safety
//
// A task is an owned object, and must be freed with tc_task_free (or, if part of a list,
// with tc_task_list_free).
//
// Any function taking a `*TCTask` requires:
//  - the pointer must not be NUL;
//  - the pointer must be one previously returned from a tc_… function;
//  - the memory referenced by the pointer must never be modified by C code; and
//  - except for `tc_{task,task_list}_free`, ownership of a `*TCTask` remains with the caller.
//
// Once passed to tc_task_free, a `*TCTask` becomes  invalid and must not be used again.
//
// TCTasks are not threadsafe.
typedef struct TCTask TCTask;

// Get the annotations for the task.
//
// The caller must free the returned TCAnnotationList instance.  The TCStringList instance does not
// reference the task and the two may be freed in any order.
EXTERN_C struct TCAnnotationList tc_task_get_annotations(struct TCTask *task);

// Get all dependencies for a task.
EXTERN_C struct TCUuidList tc_task_get_dependencies(struct TCTask *task);

// Get a task's description.
EXTERN_C struct TCString tc_task_get_description(struct TCTask *task);

// Get the entry timestamp for a task (when it was created), or 0 if not set.
EXTERN_C time_t tc_task_get_entry(struct TCTask *task);

// Get the named legacy UDA from the task.
//
// Returns NULL if the UDA does not exist.
EXTERN_C struct TCString tc_task_get_legacy_uda(struct TCTask *task, struct TCString key);

// Get all UDAs for this task.
//
// All TCUdas in this list have a NULL ns field.  The entire UDA key is
// included in the key field.  The caller must free the returned list.
EXTERN_C struct TCUdaList tc_task_get_legacy_udas(struct TCTask *task);

// Get the modified timestamp for a task, or 0 if not set.
EXTERN_C time_t tc_task_get_modified(struct TCTask *task);

// Get a task's status.
EXTERN_C enum TCStatus tc_task_get_status(struct TCTask *task);

// Get the tags for the task.
//
// The caller must free the returned TCStringList instance.  The TCStringList instance does not
// reference the task and the two may be freed in any order.
EXTERN_C struct TCStringList tc_task_get_tags(struct TCTask *task);

// Get the underlying key/value pairs for this task.  The returned TCKVList is
// a "snapshot" of the task and will not be updated if the task is subsequently
// modified.  It is the caller's responsibility to free the TCKVList.
EXTERN_C struct TCKVList tc_task_get_taskmap(struct TCTask *task);

// Get the named UDA from the task.
//
// Returns a TCString with NULL ptr field if the UDA does not exist.
EXTERN_C struct TCString tc_task_get_uda(struct TCTask *task, struct TCString ns,
                                         struct TCString key);

// Get all UDAs for this task.
//
// Legacy UDAs are represented with an empty string in the ns field.
EXTERN_C struct TCUdaList tc_task_get_udas(struct TCTask *task);

// Get a task's UUID.
EXTERN_C struct TCUuid tc_task_get_uuid(struct TCTask *task);

// Get a task property's value, or NULL if the task has no such property, (including if the
// property name is not valid utf-8).
EXTERN_C struct TCString tc_task_get_value(struct TCTask *task, struct TCString property);

// Get the wait timestamp for a task, or 0 if not set.
EXTERN_C time_t tc_task_get_wait(struct TCTask *task);

// Check if a task has the given tag.  If the tag is invalid, this function will return false, as
// that (invalid) tag is not present. No error will be reported via `tc_task_error`.
EXTERN_C bool tc_task_has_tag(struct TCTask *task, struct TCString tag);

// Check if a task is active (started and not stopped).
EXTERN_C bool tc_task_is_active(struct TCTask *task);

// Check if a task is blocked (depends on at least one other task).
EXTERN_C bool tc_task_is_blocked(struct TCTask *task);

// Check if a task is blocking (at least one other task depends on it).
EXTERN_C bool tc_task_is_blocking(struct TCTask *task);

// Check if a task is waiting.
EXTERN_C bool tc_task_is_waiting(struct TCTask *task);

// Convert an immutable task into a mutable task.
//
// The task must not be NULL. It is modified in-place, and becomes mutable.
//
// The replica must not be NULL. After this function returns, the replica _cannot be used at all_
// until this task is made immutable again.  This implies that it is not allowed for more than one
// task associated with a replica to be mutable at any time.
//
// Typically mutation of tasks is bracketed with `tc_task_to_mut` and `tc_task_to_immut`:
//
// ```text
//     tc_task_to_mut(task, rep);
//     success = tc_task_done(task);
//     tc_task_to_immut(task, rep);
//     if (!success) { ... }
// ```
EXTERN_C void tc_task_to_mut(struct TCTask *task, struct TCReplica *tcreplica);

// Add an annotation to a mutable task.  This call takes ownership of the
// passed annotation, which must not be used after the call returns.
EXTERN_C TCResult tc_task_add_annotation(struct TCTask *task, struct TCAnnotation *annotation);

// Add a dependency.
EXTERN_C TCResult tc_task_add_dependency(struct TCTask *task, struct TCUuid dep);

// Add a tag to a mutable task.
EXTERN_C TCResult tc_task_add_tag(struct TCTask *task, struct TCString tag);

// Mark a task as deleted.
EXTERN_C TCResult tc_task_delete(struct TCTask *task);

// Mark a task as done.
EXTERN_C TCResult tc_task_done(struct TCTask *task);

// Remove an annotation from a mutable task.
EXTERN_C TCResult tc_task_remove_annotation(struct TCTask *task, int64_t entry);

// Remove a dependency.
EXTERN_C TCResult tc_task_remove_dependency(struct TCTask *task, struct TCUuid dep);

// Remove a UDA fraom a mutable task.
EXTERN_C TCResult tc_task_remove_legacy_uda(struct TCTask *task, struct TCString key);

// Remove a tag from a mutable task.
EXTERN_C TCResult tc_task_remove_tag(struct TCTask *task, struct TCString tag);

// Remove a UDA fraom a mutable task.
EXTERN_C TCResult tc_task_remove_uda(struct TCTask *task, struct TCString ns, struct TCString key);

// Set a mutable task's description.
EXTERN_C TCResult tc_task_set_description(struct TCTask *task, struct TCString description);

// Set a mutable task's entry (creation time).  Pass entry=0 to unset
// the entry field.
EXTERN_C TCResult tc_task_set_entry(struct TCTask *task, time_t entry);

// Set a legacy UDA on a mutable task.
EXTERN_C TCResult tc_task_set_legacy_uda(struct TCTask *task, struct TCString key,
                                         struct TCString value);

// Set a mutable task's modified timestamp.  The value cannot be zero.
EXTERN_C TCResult tc_task_set_modified(struct TCTask *task, time_t modified);

// Set a mutable task's status.
EXTERN_C TCResult tc_task_set_status(struct TCTask *task, enum TCStatus status);

// Set a UDA on a mutable task.
EXTERN_C TCResult tc_task_set_uda(struct TCTask *task, struct TCString ns, struct TCString key,
                                  struct TCString value);

// Set a mutable task's property value by name.  If value.ptr is NULL, the property is removed.
EXTERN_C TCResult tc_task_set_value(struct TCTask *task, struct TCString property,
                                    struct TCString value);

// Set a mutable task's wait timestamp.  Pass wait=0 to unset the wait field.
EXTERN_C TCResult tc_task_set_wait(struct TCTask *task, time_t wait);

// Start a task.
EXTERN_C TCResult tc_task_start(struct TCTask *task);

// Stop a task.
EXTERN_C TCResult tc_task_stop(struct TCTask *task);

// Convert a mutable task into an immutable task.
//
// The task must not be NULL.  It is modified in-place, and becomes immutable.
//
// The replica passed to `tc_task_to_mut` may be used freely after this call.
EXTERN_C void tc_task_to_immut(struct TCTask *task);

// Get the latest error for a task, or a string NULL ptr field if the last operation succeeded.
// Subsequent calls to this function will return NULL.  The task pointer must not be NULL.  The
// caller must free the returned string.
EXTERN_C struct TCString tc_task_error(struct TCTask *task);

// Free a task.  The given task must not be NULL.  The task must not be used after this function
// returns, and must not be freed more than once.
//
// If the task is currently mutable, it will first be made immutable.
EXTERN_C void tc_task_free(struct TCTask *task);

// ***** TCTaskList *****
//
// TCTaskList represents a list of tasks.
//
// The content of this struct must be treated as read-only: no fields or anything they reference
// should be modified directly by C code.
//
// When an item is taken from this list, its pointer in `items` is set to NULL.
typedef struct TCTaskList {
  // number of tasks in items
  size_t len;
  // reserved
  size_t _u1;
  // Array of pointers representing each task. These remain owned by the TCTaskList instance and
  // will be freed by tc_task_list_free.  This pointer is never NULL for a valid TCTaskList.
  // Pointers in the array may be NULL after `tc_task_list_take`.
  struct TCTask **items;
} TCTaskList;

// Free a TCTaskList instance.  The instance, and all TCTaskList it contains, must not be used after
// this call.
//
// When this call returns, the `items` pointer will be NULL, signalling an invalid TCTaskList.
EXTERN_C void tc_task_list_free(struct TCTaskList *tasks);

// Take an item from a TCTaskList.  After this call, the indexed item is no longer associated
// with the list and becomes the caller's responsibility, just as if it had been returned from
// `tc_replica_get_task`.
//
// The corresponding element in the `items` array will be set to NULL.  If that field is already
// NULL (that is, if the item has already been taken), this function will return NULL.  If the
// index is out of bounds, this function will also return NULL.
//
// The passed TCTaskList remains owned by the caller.
EXTERN_C struct TCTask *tc_task_list_take(struct TCTaskList *tasks, size_t index);

// ***** TCWorkingSet *****
//
// A TCWorkingSet represents a snapshot of the working set for a replica.  It is not automatically
// updated based on changes in the replica.  Its lifetime is independent of the replica and it can
// be freed at any time.
//
// To iterate over a working set, search indexes 1 through largest_index.
//
// # Safety
//
// The `*TCWorkingSet` returned from `tc_replica_working_set` is owned by the caller and
// must later be freed to avoid a memory leak.  Its lifetime is independent of the replica
// from which it was generated.
//
// Any function taking a `*TCWorkingSet` requires:
//  - the pointer must not be NUL;
//  - the pointer must be one previously returned from `tc_replica_working_set`
//  - the memory referenced by the pointer must never be accessed by C code; and
//  - except for `tc_replica_free`, ownership of a `*TCWorkingSet` remains with the caller.
//
// Once passed to `tc_replica_free`, a `*TCWorkingSet` becomes invalid and must not be used again.
//
// TCWorkingSet is not threadsafe.
typedef struct TCWorkingSet TCWorkingSet;

// Get the UUID for the task at the given index.  Returns true if the UUID exists in the working
// set.  If not, returns false and does not change uuid_out.
EXTERN_C bool tc_working_set_by_index(struct TCWorkingSet *ws, size_t index,
                                      struct TCUuid *uuid_out);

// Get the working set index for the task with the given UUID.  Returns 0 if the task is not in
// the working set.
EXTERN_C size_t tc_working_set_by_uuid(struct TCWorkingSet *ws, struct TCUuid uuid);

// Get the working set's largest index.
EXTERN_C size_t tc_working_set_largest_index(struct TCWorkingSet *ws);

// Get the working set's length, or the number of UUIDs it contains.
EXTERN_C size_t tc_working_set_len(struct TCWorkingSet *ws);

// Free a TCWorkingSet.  The given value must not be NULL.  The value must not be used after this
// function returns, and must not be freed more than once.
EXTERN_C void tc_working_set_free(struct TCWorkingSet *ws);

#endif /* TASKCHAMPION_H */
