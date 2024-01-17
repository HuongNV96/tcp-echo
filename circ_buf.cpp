#include "circ_buf.h"
#include <stdlib.h>
#include <string.h>

bool circ_buf_is_full(circ_buf_t *buf) {
  return ((buf->head_index - buf->tail_index) & (buf->max_len - 1)) == (buf->max_len - 1);
}

size_t circ_buf_num_items(circ_buf_t *buf) {
  return ((buf->head_index - buf->tail_index) & (buf->max_len - 1));
}

bool circ_buf_is_empty(circ_buf_t *buf) {
  return (buf->head_index == buf->tail_index);
}

bool init_circ_buf(circ_buf_t * buf, size_t len) {
    buf->max_len = len;
    if ((buf->max_len & (buf->max_len - 1)) != 0) {
        perror("len must power of 2");
        return false;
    }
    buf->raw = (int8_t *)malloc(buf->max_len);
    buf->tail_index = 0;
    buf->head_index = 0;

    if (pthread_mutex_init(&buf->lock_, NULL) != 0) {
        perror("\n mutex init has failed\n");
        return false;
    }

    return true;
}
void del_circ_buf(circ_buf_t * buf) {
    free(buf->raw);
    pthread_mutex_destroy(&buf->lock_);

}
size_t read_bytes_from_circ_buf(circ_buf_t * buf, void * data, size_t len) {
    if(circ_buf_is_empty(buf)) {
        return 0;
    }

    int8_t *data_ptr = (int8_t*)data;
    size_t total = 0;
    while((total < len) && buf->read_byte(buf, data_ptr)) {
        total++;
        data_ptr++;
    }
    return total;
}
size_t write_bytes_to_circ_buf(circ_buf_t * buf, void * data, size_t len) {
    size_t i;
    for(i = 0; i < len; i++) {
        write_byte_to_circ_buf(buf,(int8_t *)data + i);
    }
    return len;
}

bool read_byte_form_circ_buf(circ_buf_t * buf, int8_t * data)
{
    if(circ_buf_is_empty(buf)) {
        return false;
    }

    *data = buf->raw[buf->tail_index];
    buf->tail_index = ((buf->tail_index + 1) & (buf->max_len - 1));
    return true;
}

bool write_byte_to_circ_buf(circ_buf_t * buf,int8_t * data) {
    if(circ_buf_is_full(buf)) {
        buf->tail_index = ((buf->tail_index + 1) & buf->max_len);
    }

    buf->raw[buf->head_index] = *data;
    buf->head_index = ((buf->head_index + 1) & (buf->max_len - 1));
    return true;
}

bool circ_buf_peek(circ_buf_t * buf, int8_t *data, size_t index) {
    if(index >= circ_buf_num_items(buf)) {
        return false;
    }
  
    size_t data_index = ((buf->tail_index + index) & (buf->max_len - 1));
    *data = buf->raw[data_index];
    return true;
}
void lock_circ_buf(circ_buf_t * buf) {
    pthread_mutex_lock(&buf->lock_);
}
void unlock_circ_buf(circ_buf_t * buf) {
    pthread_mutex_unlock(&buf->lock_);
}