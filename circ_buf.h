//Ref to https://github.com/AndersKaloer/Ring-Buffer
#ifndef _CIRC_BUF_H_
#define _CIRC_BUF_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct circ_buf circ_buf_t;

struct circ_buf
{
    size_t max_len;
    int8_t * raw;
    size_t tail_index;
    size_t head_index;
    pthread_mutex_t lock_;


    bool (*init)(circ_buf_t * buf, size_t len);
    void (*del)(circ_buf_t * buf);
    size_t (*read_bytes)(circ_buf_t * buf, void * data, size_t len);
    size_t (*write_bytes)(circ_buf_t *buf,void * data, size_t len);
    bool (*read_byte)(circ_buf_t * buf, int8_t * data);
    bool (*write_byte)(circ_buf_t * buf,int8_t * data);
    bool (*peek)(circ_buf_t * buf, int8_t *data, size_t index);
    size_t (*size)(circ_buf_t * buf);
    void (*lock)(circ_buf_t * buf);
    void (*unlock)(circ_buf_t * buf);
};

bool init_circ_buf(circ_buf_t * buf, size_t len);
void del_circ_buf(circ_buf_t * buf);
size_t read_bytes_from_circ_buf(circ_buf_t * buf, void * data, size_t len);
size_t write_bytes_to_circ_buf(circ_buf_t * buf, void * data, size_t len);
bool read_byte_form_circ_buf(circ_buf_t * buf, int8_t * data);
bool write_byte_to_circ_buf(circ_buf_t * buf,int8_t * data);
bool circ_buf_peek(circ_buf_t * buf, int8_t *data, size_t index);
void lock_circ_buf(circ_buf_t * buf);
void unlock_circ_buf(circ_buf_t * buf);
bool circ_buf_is_full(circ_buf_t *buf);
size_t circ_buf_num_items(circ_buf_t *buf);
bool circ_buf_is_empty(circ_buf_t *buf);

#endif