/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

/** @file log.c
 * @brief The log object.
 *
 * This file implements everything needed for logging: the log object and its functions.
 *
 * @author Navit Team
 * @date 2005-2014
 */

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>
#include "file.h"
#include "item.h"
#include "event.h"
#include "callback.h"
#include "debug.h"
#include "xmlconfig.h"
#include "log.h"
#ifndef HAVE_API_WIN32_BASE
#include <errno.h>
#endif
struct log_data {
    int len;
    int max_len;
    char *data;
};

struct log {
    NAVIT_OBJECT
    FILE *f;
    int overwrite;
    int empty;
    int lazy;
    int mkdir;
    int flush_size;
    int flush_time;
    struct event_timeout *timer;
    struct callback *timer_callback;
#ifdef HAVE_SYS_TIME_H
    struct timeval last_flush;
#endif
    char *filename;
    char *filename_ex1;
    char *filename_ex2;
    struct log_data header;
    struct log_data data;
    struct log_data trailer;
};

/**
 * @brief Stores formatted time to a string.
 *
 * This function obtains local system time, formats it as specified in {@code fmt} and stores it in buffer.
 * Format strings follow the same syntax as those for {@code strftime()}.
 *
 * @param buffer A preallocated buffer that will receive the formatted time
 * @param size Size of the buffer, in bytes
 * @param fmt The format string
 * @return Nothing
 */
static void strftime_localtime(char *buffer, int size, char *fmt) {
    time_t t;
    struct tm *tm;

    t=time(NULL);
    tm=localtime(&t);
    strftime(buffer, size - 1, fmt, tm);
}

/**
 * @brief Expands placeholders in a filename
 *
 * This function examines the {@code log->filename} and replaces any placeholders
 * found in it with date, time or an incremental number. If an incremental number is specified, the function
 * will ensure the filename is unique. The expanded filename will be stored {@code log->filename_ex2}.
 * The function uses {@code log->filename_ex1} to store the partly-expanded filename.
 *
 * @param this_ The log object.
 */
static void expand_filenames(struct log *this_) {
    char *pos,buffer[4096];
    int i;

    strftime_localtime(buffer, 4096, this_->filename);
    this_->filename_ex1=g_strdup(buffer);
    if ((pos=strstr(this_->filename_ex1,"%i"))) {
#ifdef HAVE_API_ANDROID
        pos[1]='d';
#endif
        i=0;
        do {
            g_free(this_->filename_ex2);
            this_->filename_ex2=g_strdup_printf(this_->filename_ex1,i++);
        } while (file_exists(this_->filename_ex2));
#ifdef HAVE_API_ANDROID
        pos[1]='i';
#endif
    } else
        this_->filename_ex2=g_strdup(this_->filename_ex1);
}

/**
 * @brief Sets the time at which the log buffer was last flushed.
 *
 * This function sets {@code log->last_flush} to current time.
 *
 * @param this_ The log object.
 */
static void log_set_last_flush(struct log *this_) {
#ifdef HAVE_SYS_TIME_H
    gettimeofday(&this_->last_flush, NULL);
#endif
}

/**
 * @brief Opens a log file.
 *
 * This function opens the log file for {@code log}.
 * The file name must be specified by {@code log->filename_ex2} before this function is called.
 * <p>
 * {@code log->overwrite} specifies the behavior if the file exists: if true,
 * an existing file will be overwritten, else it will be appended to.
 * <p>
 * If the directory specified in the filename does not exist and the {@code log->mkdir}
 * is true, it will be created.
 * <p>
 * After the function returns, {@code log->f} will contain the file handle (or NULL, if
 * the operation failed) and {@code log->empty} will indicate if the file is empty.
 * {@code log->last_flush} will be updated with the current time.
 *
 * @param this_ The log object.
 */
static void log_open(struct log *this_) {
    char *mode;
    if (this_->overwrite)
        mode="w";
    else
        mode="r+";
    if (this_->mkdir)
        file_mkdir(this_->filename_ex2, 2);
    this_->f=fopen(this_->filename_ex2, mode);
    if (! this_->f)
        this_->f=fopen(this_->filename_ex2, "w");
    if (! this_->f)
        return;
    if (!this_->overwrite)
        fseek(this_->f, 0, SEEK_END);
    this_->empty = !ftell(this_->f);
    log_set_last_flush(this_);
}

/**
 * @brief Closes a log file.
 *
 * This function writes the trailer to a log file, flushes it and closes the log file for {@code log}.
 *
 * @param this_ The log object.
 */
static void log_close(struct log *this_) {
    if (! this_->f)
        return;
    if (this_->trailer.len)
        fwrite(this_->trailer.data, 1, this_->trailer.len, this_->f);
    fflush(this_->f);
    fclose(this_->f);
    this_->f=NULL;
}

/**
 * @brief Flushes the buffer of a log.
 *
 * This function writes buffered log data to the log file associated with {@code log}
 * and updates {@code log->last_flush} with the current time.
 * <p>
 * If {@code log->lazy} is true, this function will open the file if needed, else
 * the file must be opened with {@code log_open()} prior to calling this function.
 * <p>
 * If the file is empty, the header will be written first, followed by the buffer data.
 * {@code log->empty} will be set to zero if header or data are written to the file.
 *
 * @param this_ The log object.
 * @param flags Flags to control behavior of the function:
 * <br>
 * {@code log_flag_replace_buffer}: ignored
 * <br>
 * {@code log_flag_force_flush}: ignored
 * <br>
 * {@code log_flag_keep_pointer}: keeps the file pointer at the start position of the new data
 * <br>
 * {@code log_flag_keep_buffer}: prevents clearing of the buffer after a successful write (default is to clear the buffer).
 * <br>
 * {@code log_flag_truncate}: truncates the log file at the current position. On the Win32 Base API, this flag has no effect.
 */
static void log_flush(struct log *this_, enum log_flags flags) {
    long pos;
    if (this_->lazy && !this_->f) {
        if (!this_->data.len)
            return;
        log_open(this_);
    }
    if (! this_->f)
        return;
    if (this_->empty) {
        if (this_->header.len)
            fwrite(this_->header.data, 1, this_->header.len, this_->f);
        if (this_->header.len || this_->data.len)
            this_->empty=0;
    }
    fwrite(this_->data.data, 1, this_->data.len, this_->f);
#ifndef HAVE_API_WIN32_BASE
    if (flags & log_flag_truncate) {
        pos=ftell(this_->f);
        if(ftruncate(fileno(this_->f), pos) <0)
            dbg(lvl_error,"Error on fruncate (%s)", strerror(errno));
    }
#endif
    if (this_->trailer.len) {
        pos=ftell(this_->f);
        if (pos > 0) {
            fwrite(this_->trailer.data, 1, this_->trailer.len, this_->f);
            fseek(this_->f, pos, SEEK_SET);
        }
    }
    if (flags & log_flag_keep_pointer)
        fseek(this_->f, -this_->data.len, SEEK_CUR);
    fflush(this_->f);
    if (!(flags & log_flag_keep_buffer)) {
        g_free(this_->data.data);
        this_->data.data=NULL;
        this_->data.max_len=this_->data.len=0;
    }
    log_set_last_flush(this_);
}

/**
 * @brief Determines if the maximum buffer size of a log has been exceeded.
 *
 * This function examines the size of the data buffer to determine if it exceeds
 * the maximum size specified in {@code log->flush_size} and thus needs to be flushed.
 *
 * @param this_ The log object.
 * @return True if the cache needs to be flushed, false otherwise.
 */
static int log_flush_required(struct log *this_) {
    return this_->data.len > this_->flush_size;
}

/**
 * @brief Rotates a log file.
 *
 * This function rotates a log by stopping and immediately restarting it.
 * Stopping flushes the buffer and closes the file; restarting determines the
 * new file name and opens the file as needed (depending on the lazy member).
 * <p>
 * Depending on the file name format and how the function was called, a new log
 * file will be created or the old log file will be reused (appended to or
 * overwritten, depending on {@code log->overwrite}): if the file name includes an
 * incremental number, the new file will always have a different name. If a
 * previous call to {@code log_change_required()} returned true, the new file
 * will also have a different name. In all other cases the new file will have
 * the same name as the old one, causing the old file to be overwritten or
 * appended to.
 *
 * @param this_ The log object.
 */
static void log_change(struct log *this_) {
    log_flush(this_,0);
    log_close(this_);
    expand_filenames(this_);
    if (! this_->lazy)
        log_open(this_);
}

/**
 * @brief Determines if the log must be rotated.
 *
 * This function expands the date and time placeholders in {@code log->filename}
 * to determine if the resulting part of the filename has changed.
 *
 * @param this_ The log object.
 * @return True if the date/time-dependent part of the filename has changed, false otherwise.
 */
static int log_change_required(struct log *this_) {
    char buffer[4096];

    strftime_localtime(buffer, 4096, this_->filename);
    return (strcmp(this_->filename_ex1, buffer) != 0);
}

/**
 * @brief Determines if the flush interval of a log has elapsed and flushes the buffer if needed.
 *
 * This function calculates the difference between current time and {@code log->last_flush}.
 * If it is greater than or equal to {@code log->flush_time}, the buffer is flushed.
 *
 * @param this_ The log object.
 */
static void log_timer(struct log *this_) {
#ifdef HAVE_SYS_TIME_H
    struct timeval tv;
    int delta;
    gettimeofday(&tv, NULL);
    delta=(tv.tv_sec-this_->last_flush.tv_sec)*1000+(tv.tv_usec-this_->last_flush.tv_usec)/1000;
    dbg(lvl_debug,"delta=%d flush_time=%d", delta, this_->flush_time);
    if (this_->flush_time && delta >= this_->flush_time*1000)
        log_flush(this_,0);
#endif
}

/**
 * @brief Gets an attribute
 *
 * @param this_ The log object.
 * @param attr_type The attribute type to return
 * @param attr Points to a struct attr to store the attribute
 * @param iter An attribute iterator
 * @return True for success, false for failure
 */
int log_get_attr(struct log *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}


/**
 * @brief Creates and initializes a new log object.
 *
 * @param parent The parent object.
 * @param attrs Points to an array of pointers to attributes for the new log object
 * @return The new log object, or NULL if creation fails.
 */
struct log *
log_new(struct attr * parent,struct attr **attrs) {
    struct log *ret=g_new0(struct log, 1);
    struct attr *data,*overwrite,*lazy,*mkdir,*flush_size,*flush_time;
    struct file_wordexp *wexp;
    char *filename, **wexp_data;

    dbg(lvl_debug,"enter");
    ret->func=&log_func;
    navit_object_ref((struct navit_object *)ret);
    data=attr_search(attrs, attr_data);
    if (! data)
        return NULL;
    filename=data->u.str;
    wexp=file_wordexp_new(filename);
    if (wexp && file_wordexp_get_count(wexp) > 0) {
        wexp_data=file_wordexp_get_array(wexp);
        filename=wexp_data[0];
    }
    if (filename)
        ret->filename=g_strdup(filename);
    if (wexp)
        file_wordexp_destroy(wexp);
    overwrite=attr_search(attrs, attr_overwrite);
    if (overwrite)
        ret->overwrite=overwrite->u.num;
    lazy=attr_search(attrs, attr_lazy);
    if (lazy)
        ret->lazy=lazy->u.num;
    mkdir=attr_search(attrs, attr_mkdir);
    if (mkdir)
        ret->mkdir=mkdir->u.num;
    flush_size=attr_search(attrs, attr_flush_size);
    if (flush_size)
        ret->flush_size=flush_size->u.num;
    flush_time=attr_search(attrs, attr_flush_time);
    if (flush_time)
        ret->flush_time=flush_time->u.num;
    if (ret->flush_time) {
        dbg(lvl_debug,"interval %d", ret->flush_time*1000);
        ret->timer_callback=callback_new_1(callback_cast(log_timer), ret);
        ret->timer=event_add_timeout(ret->flush_time*1000, 1, ret->timer_callback);
    }
    expand_filenames(ret);
    if (ret->lazy)
        log_set_last_flush(ret);
    else
        log_open(ret);
    ret->attrs=attr_list_dup(attrs);
    return ret;
}

/**
 * @brief Sets the header for a log file.
 *
 * This function sets the header, which is to be inserted into any log file before
 * the actual log data.
 *
 * @param this_ The log object.
 * @param data The header data.
 * @param len Size of the header data to be copied, in bytes.
 */
void log_set_header(struct log *this_, char *data, int len) {
    this_->header.data=g_malloc(len);
    this_->header.max_len=this_->header.len=len;
    memcpy(this_->header.data, data, len);
}

/**
 * @brief Sets the trailer for a log file.
 *
 * This function sets the trailer, which is to be added to any log file after
 * the actual log data.
 *
 * @param this_ The log object.
 * @param data The trailer data.
 * @param len Size of the trailer data to be copied, in bytes.
 */
void log_set_trailer(struct log *this_, char *data, int len) {
    this_->trailer.data=g_malloc(len);
    this_->trailer.max_len=this_->trailer.len=len;
    memcpy(this_->trailer.data, data, len);
}

/**
 * @brief Writes to a log.
 *
 * This function appends data to a log. It rotates the log, if needed, before
 * adding the new data. After adding, the log is flushed if the buffer exceeds
 * its maximum size or if the {@code log_flag_force_flush} flag is set.
 *
 * @param this_ The log object.
 * @param data Points to a buffer containing the data to be appended.
 * @param len Length of the data to be appended, in bytes.
 * @param flags Flags to control behavior of the function:
 * <br>
 * {@code log_flag_replace_buffer}: discards any data in the buffer not yet written to the log file
 * <br>
 * {@code log_flag_force_flush}: forces a flush of the log after appending the data
 * <br>
 * {code log_flag_keep_pointer}: ignored
 * <br>
 * {@code log_flag_keep_buffer}: ignored
 * <br>
 * {@code log_flag_truncate}: ignored
 */
void log_write(struct log *this_, char *data, int len, enum log_flags flags) {
    dbg(lvl_debug,"enter");
    if (log_change_required(this_)) {
        dbg(lvl_debug,"log_change");
        log_change(this_);
    }
    if (flags & log_flag_replace_buffer)
        this_->data.len=0;
    if (this_->data.len + len > this_->data.max_len) {
        dbg(lvl_info,"overflow");
        this_->data.max_len+=16384; // FIXME: what if len exceeds this->data.max_len by more than 16384 bytes?
        this_->data.data=g_realloc(this_->data.data,this_->data.max_len);
    }
    memcpy(this_->data.data+this_->data.len, data, len);
    this_->data.len+=len;
    if (log_flush_required(this_) || (flags & log_flag_force_flush))
        log_flush(this_, flags);
}

/**
 * @brief Returns the data buffer of a log object and its length.
 *
 * @param this_ The log object.
 * @param len Points to an int which will receive the length of the buffer.
 * This can be NULL, in which case no information on buffer length will be stored.
 * @return Pointer to the data buffer.
 */
void *log_get_buffer(struct log *this_, int *len) {
    if (len)
        *len=this_->data.len;
    return this_->data.data;
}


/**
 * @brief Writes a formatted string to a log.
 *
 * This function formats a string in a fashion similar to {@code printf()} and related functions
 * and writes it to a log using {@code log_write()}.
 *
 * @param this_ The log object.
 * @param fmt The format string.
 * @param ... Additional arguments must be specified for each placeholder in the format string.
 */
void log_printf(struct log *this_, char *fmt, ...) {
    char buffer[LOG_BUFFER_SIZE];
    int size;
    va_list ap;

    va_start(ap, fmt);

    // Format the string and write it to the log
    size = g_vsnprintf(buffer, LOG_BUFFER_SIZE, fmt, ap);
    log_write(this_, buffer, size, 0);

    va_end(ap);
}

/**
 * @brief Destroys a log object and frees up its memory.
 *
 * @param this_ The log object.
 */
void log_destroy(struct log *this_) {
    dbg(lvl_debug,"enter");
    attr_list_free(this_->attrs);
    callback_destroy(this_->timer_callback);
    event_remove_timeout(this_->timer);
    log_flush(this_,0);
    log_close(this_);
    g_free(this_);
}

struct object_func log_func = {
    attr_log,
    (object_func_new)log_new,
    (object_func_get_attr)navit_object_get_attr,
    (object_func_iter_new)navit_object_attr_iter_new,
    (object_func_iter_destroy)navit_object_attr_iter_destroy,
    (object_func_set_attr)navit_object_set_attr,
    (object_func_add_attr)navit_object_add_attr,
    (object_func_remove_attr)navit_object_remove_attr,
    (object_func_init)NULL,
    (object_func_destroy)log_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};

