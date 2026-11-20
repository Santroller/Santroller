#include "wm_reports.h"
#include "wm_crypto.h"
#include "inet.h"
#include "wii_extension_backend.h"

#include <stdlib.h>
#include <string.h>

struct report * report_queue_push(struct wiimote_state * state)
{
  struct queued_report * rpt;

  //allocate new report
  rpt = (struct queued_report *)malloc(sizeof(struct queued_report));
  memset(rpt, 0, sizeof(struct queued_report));

  //append to the end of the queue
  if (state->sys.queue_end != NULL)
  {
    state->sys.queue_end->next = rpt;
  }
  else
  {
    state->sys.queue = rpt;
  }
  state->sys.queue_end = rpt;

  return &(rpt->rpt);
}

struct report * report_queue_peek(struct wiimote_state * state)
{
  if (state->sys.queue == NULL) return NULL; //empty queue

  return &(state->sys.queue->rpt);
}

void report_queue_pop(struct wiimote_state * state)
{
  struct queued_report * rpt;

  if (state->sys.queue == NULL) return; //nothing to remove

  //remove from the queue
  rpt = state->sys.queue;
  if (state->sys.queue_end == rpt)
  {
    state->sys.queue_end = NULL;
  }
  state->sys.queue = rpt->next;

  //free report mem
  free(rpt);
}

void report_queue_push_ack(struct wiimote_state *state, uint8_t report, uint8_t result)
{
  //push acknowledgement report x22
  struct report * rpt = report_queue_push(state);
  rpt->len = 6;
  rpt->data.io = 0xa1;
  rpt->data.type = 0x22;

  struct report_ack * ack = (struct report_ack *)rpt->data.buf;
  ack->report = report;
  ack->result = result;
}

void report_queue_push_status(struct wiimote_state * state)
{
  //push status report x20
  struct report * rpt = report_queue_push(state);
  rpt->len = 8;
  rpt->data.io = 0xa1;
  rpt->data.type = 0x20;

  struct report_status * status = (struct report_status *)rpt->data.buf;
  status->low_battery         = state->sys.low_battery;
  status->extension_connected = state->sys.extension_connected;
  status->speaker_enabled     = state->sys.speaker_enabled;
  status->ircam_enabled       = state->sys.ircam_enabled;
  status->led_1               = state->sys.led_1;
  status->led_2               = state->sys.led_2;
  status->led_3               = state->sys.led_3;
  status->led_4               = state->sys.led_4;
  status->battery_level       = state->sys.battery_level;
}

void report_format_mem_resp(struct wiimote_state * state, struct report * rpt,
  int size, int error, uint16_t addr, uint8_t * buf, bool encrypt)
{
  struct report_mem_resp * resp = (struct report_mem_resp *)rpt->data.buf;

  rpt->len = 23;
  rpt->data.io = 0xa1;
  rpt->data.type = 0x21;

  resp->size = size - 1;
  resp->error = error;
  resp->addr = htons(addr);
  if (buf != NULL) //buf will be null for error reports
  {
    memcpy(resp->data, buf, size);

    if (encrypt)
    {
      ext_encrypt_bytes(&state->sys.extension_crypto_state, resp->data, addr & 0x7, size);
    }
  }
}

void report_append_buttons(struct wiimote_state * state, uint8_t * buf)
{
  struct report_buttons * rpt = (struct report_buttons *)buf;

  rpt->left  = state->usr.left;
  rpt->right = state->usr.right;
  rpt->down  = state->usr.down;
  rpt->up    = state->usr.up;
  rpt->one   = state->usr.one;
  rpt->two   = state->usr.two;
  rpt->a     = state->usr.a;
  rpt->b     = state->usr.b;
  rpt->plus  = state->usr.plus;
  rpt->minus = state->usr.minus;
  rpt->home  = state->usr.home;
}

void report_append_accelerometer(struct wiimote_state * state, uint8_t * buf)
{
  struct report_accelerometer * rpt = (struct report_accelerometer *)buf;

  rpt->buttons.accel_0 = state->usr.accel_x;
  rpt->buttons.accel_1 = (state->usr.accel_z & 0x2) |
                         ((state->usr.accel_y >> 1) & 0x1);

  rpt->x = state->usr.accel_x >> 2;
  rpt->y = state->usr.accel_y >> 2;
  rpt->z = state->usr.accel_z >> 2;
}

void report_append_ir_10(struct wiimote_state * state, uint8_t * buf)
{
  struct report_ir_basic * rpt = (struct report_ir_basic *)buf;

  rpt->x1_lo = state->usr.ir_object[0].x;
  rpt->y1_lo = state->usr.ir_object[0].y;
  rpt->x1_hi = state->usr.ir_object[0].x >> 8;
  rpt->y1_hi = state->usr.ir_object[0].y >> 8;

  rpt->x2_lo = state->usr.ir_object[1].x;
  rpt->y2_lo = state->usr.ir_object[1].y;
  rpt->x2_hi = state->usr.ir_object[1].x >> 8;
  rpt->y2_hi = state->usr.ir_object[1].y >> 8;

  rpt->x3_lo = state->usr.ir_object[2].x;
  rpt->y3_lo = state->usr.ir_object[2].y;
  rpt->x3_hi = state->usr.ir_object[2].x >> 8;
  rpt->y3_hi = state->usr.ir_object[2].y >> 8;

  rpt->x4_lo = state->usr.ir_object[3].x;
  rpt->y4_lo = state->usr.ir_object[3].y;
  rpt->x4_hi = state->usr.ir_object[3].x >> 8;
  rpt->y4_hi = state->usr.ir_object[3].y >> 8;

}

void report_append_ir_12(struct wiimote_state * state, uint8_t * buf)
{
  struct report_ir_ext * rpt = (struct report_ir_ext *)buf;
  int i;

  for (i=0; i<4; i++)
  {
    rpt->obj[i].x_lo = state->usr.ir_object[i].x;
    rpt->obj[i].y_lo = state->usr.ir_object[i].y;
    rpt->obj[i].x_hi = state->usr.ir_object[i].x >> 8;
    rpt->obj[i].y_hi = state->usr.ir_object[i].y >> 8;
    rpt->obj[i].size = state->usr.ir_object[i].size;
  }

}

void report_append_interleaved(struct wiimote_state * state, uint8_t * buf)
{
  struct report_interleaved * rpt = (struct report_interleaved *)buf;
  int i;

  if (state->sys.reporting_mode == 0x3e)
  {
    //only the top 8 bits of each axis fit, like X and Y (wiibrew Interleaved Mode): Z<5:4>, Z<7:6>
    rpt->buttons.accel_0 = state->usr.accel_z >> 6;
    rpt->buttons.accel_1 = state->usr.accel_z >> 8;
    rpt->accel = state->usr.accel_x >> 2;

    for (i=0; i<2; i++)
    {
      rpt->obj[i].x_lo = state->usr.ir_object[i].x;
      rpt->obj[i].y_lo = state->usr.ir_object[i].y;
      rpt->obj[i].x_hi = state->usr.ir_object[i].x >> 8;
      rpt->obj[i].y_hi = state->usr.ir_object[i].y >> 8;
      rpt->obj[i].size = state->usr.ir_object[i].size;
      rpt->obj[i].x_min = state->usr.ir_object[i].xmin;
      rpt->obj[i].y_min = state->usr.ir_object[i].ymin;
      rpt->obj[i].x_max = state->usr.ir_object[i].xmax;
      rpt->obj[i].y_max = state->usr.ir_object[i].ymax;
      rpt->obj[i].intensity = state->usr.ir_object[i].intensity;
    }

    state->sys.reporting_mode = 0x3f;
  }
  else
  {
    //Z<1:0>, Z<3:2> of the top 8 bits
    rpt->buttons.accel_0 = state->usr.accel_z >> 2;
    rpt->buttons.accel_1 = state->usr.accel_z >> 4;
    rpt->accel = state->usr.accel_y >> 2;

    for (i=0; i<2; i++)
    {
      rpt->obj[i].x_lo = state->usr.ir_object[i+2].x;
      rpt->obj[i].y_lo = state->usr.ir_object[i+2].y;
      rpt->obj[i].x_hi = state->usr.ir_object[i+2].x >> 8;
      rpt->obj[i].y_hi = state->usr.ir_object[i+2].y >> 8;
      rpt->obj[i].size = state->usr.ir_object[i+2].size;
      rpt->obj[i].x_min = state->usr.ir_object[i+2].xmin;
      rpt->obj[i].y_min = state->usr.ir_object[i+2].ymin;
      rpt->obj[i].x_max = state->usr.ir_object[i+2].xmax;
      rpt->obj[i].y_max = state->usr.ir_object[i+2].ymax;
      rpt->obj[i].intensity = state->usr.ir_object[i+2].intensity;
    }

    state->sys.reporting_mode = 0x3e;
  }
}

// A Motion Plus in passthrough mode squeezes the extension's 6 bytes so byte 5 bits 1-0 can
// mark the frame as extension data (bit 1 clear) and byte 4 bit 0 can flag the extension as
// connected. The bits that displaces move elsewhere (wiibrew Wiimote/Extension_Controllers/
// Wii_Motion_Plus, matching Linux hid-wiimote's decoding):
//  0x05 nunchuk: the accelerometer LSBs are dropped, AZ<2:1> AY<1> AX<1> BC BZ fill byte 5 bits 7-2
//  0x07 classic: BDU and BDL move to bit 0 of bytes 0 and 1, replacing the left stick LSBs
static void repack_passthrough(uint8_t mode, uint8_t *ext)
{
  if (mode == 0x05)
  {
    uint8_t in4 = ext[4], in5 = ext[5];
    ext[5] = (uint8_t)(((in4 & 0x01) << 7) |        // AZ<2>
                       (((in5 >> 7) & 0x01) << 6) | // AZ<1>
                       (((in5 >> 5) & 0x01) << 5) | // AY<1>
                       (((in5 >> 3) & 0x01) << 4) | // AX<1>
                       (((in5 >> 1) & 0x01) << 3) | // BC
                       ((in5 & 0x01) << 2));        // BZ
  }
  else
  {
    uint8_t in5 = ext[5];
    ext[0] = (uint8_t)((ext[0] & 0xfe) | (in5 & 0x01));        // BDU
    ext[1] = (uint8_t)((ext[1] & 0xfe) | ((in5 >> 1) & 0x01)); // BDL
    ext[5] = (uint8_t)(in5 & 0xfc);
  }
  ext[4] |= 0x01; // extension connected
}

void report_append_extension(struct wiimote_state * state, uint8_t * buf, uint8_t bytes)
{
  bool interleaved_motionplus = state->sys.extension_report_type == 0x05 ||
                                state->sys.extension_report_type == 0x07;

  if (state->sys.extension_stream_valid &&
      !(interleaved_motionplus && state->sys.extension_report))
  {
    uint8_t count = bytes;
    if (count > state->sys.extension_stream_size)
      count = state->sys.extension_stream_size;
    // plain bytes first, as passthrough repacking happens before encryption
    for (uint8_t i = 0; i < count; i++)
      buf[i] = state->sys.register_a4[0x08 + i];
    memset(buf + count, 0, bytes - count);
    if (interleaved_motionplus && count >= 6)
      repack_passthrough(state->sys.extension_report_type, buf);
    if (state->sys.extension_encrypted)
    {
      for (uint8_t i = 0; i < count; i++)
        buf[i] = wii_extension_backend_encrypt(&state->sys.extension_crypto_state, (uint8_t)(0x08 + i), buf[i]);
    }
    if (interleaved_motionplus)
      state->sys.extension_report = 1;
    return;
  }

  //a600fe = 0x04 activate motionplus, 0x05 activate nunchuk passthrough, 0x07 activate classic passthrough
      //if no other extension, send 0x20

  //a600f0 = 0x55 deactivate motionplus
      //send report 0x20 twice (once for unplugged, once for plugged in)

  //0xa400fa contents
  //0000 A420 0000   Nunchuk
  //0000 A420 0101    Classic
  //0000 A420 0405    WMP
  //0000 A420 0505    WMP nunchuk
  //0000 A420 0705    WMP classic

  //0000 A620 0005    Inactive WMP
  //      ^=6         Deactivated WMP

  //these should be set to the the address offset of the extension data
  //and the length in bytes of the extesnion data
  //right now, they are always the same in all situations
  int addr_offset = 0x08, length = 6;

  bool motionplus_report = state->sys.extension_report_type == 0x04 ||
                           (interleaved_motionplus && state->sys.extension_report);
  if (motionplus_report)
  {
    struct report_ext_motionplus * rpt = (struct report_ext_motionplus *)buf;

    rpt->yaw_hi = state->usr.motionplus.yaw_down >> 8;
    rpt->yaw_lo = state->usr.motionplus.yaw_down;
    rpt->roll_hi = state->usr.motionplus.roll_left >> 8;
    rpt->roll_lo = state->usr.motionplus.roll_left;
    rpt->pitch_hi = state->usr.motionplus.pitch_left >> 8;
    rpt->pitch_lo = state->usr.motionplus.pitch_left;

    rpt->yaw_slow = state->usr.motionplus.yaw_slow;
    rpt->pitch_slow = state->usr.motionplus.pitch_slow;
    rpt->roll_slow = state->usr.motionplus.roll_slow;

    rpt->ext = interleaved_motionplus ? 1 : 0;
    rpt->unused_0 = 1;
    if (interleaved_motionplus)
      state->sys.extension_report = 0;
  }


  if (state->sys.extension_encrypted)
  {
    ext_encrypt_bytes(&state->sys.extension_crypto_state, buf, addr_offset, length);
  }
}
