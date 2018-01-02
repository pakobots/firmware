/*
 * This file is part of Pako Bots division of Origami 3 Firmware.
 *
 *  Pako Bots division of Origami 3 Firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pako Bots division of Origami 3 Firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Pako Bots division of Origami 3 Firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ws_server.h"

#include "freertos/FreeRTOS.h"
#include "hwcrypto/sha.h"
#include "esp_system.h"
#include "wpa2/utils/base64.h"
#include <string.h>
#include <stdlib.h>
#include "robot.h"

#define WS_CLIENT_KEY_L  24  /**< \brief Length of the Client Key*/
#define SHA1_RES_L   20  /**< \brief SHA1 result*/
#define WS_STD_LEN   125  /**< \brief Maximum Length of standard length frames*/
#define WS_SPRINTF_ARG_L 4  /**< \brief Length of sprintf argument for string (%.*s)*/

/** \brief Opcode according to RFC 6455*/
typedef enum {
    WS_OP_CON = 0x0,                             /*!< Continuation Frame*/
    WS_OP_TXT = 0x1,                             /*!< Text Frame*/
    WS_OP_BIN = 0x2,                             /*!< Binary Frame*/
    WS_OP_CLS = 0x8,                             /*!< Connection Close Frame*/
    WS_OP_PIN = 0x9,                             /*!< Ping Frame*/
    WS_OP_PON = 0xa                             /*!< Pong Frame*/
} WS_OPCODES;

//reference to the RX TX queues
extern QueueHandle_t WebSocket_rx_queue;
extern QueueHandle_t WebSocket_tx_queue;

const char WS_sec_WS_keys[] = "Sec-WebSocket-Key:";
const char WS_sec_conKey[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char WS_srv_hs[] = "HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.*s\r\n\r\n";

void
ws_server_queue_data(char *data, size_t length)
{
    WebSocket_frame_t frame;
    frame.payload = malloc(length + 1);
    frame.payload_length = length;

    memcpy(frame.payload, data, length);
    xQueueSend(WebSocket_tx_queue,&frame, 0);
}


void ws_server_write_data(struct netconn *conn, char *p_data, size_t length)
{
    //check if we have an open connection
    //currently only frames with a payload length <WS_STD_LEN are supported
    if (conn == NULL || length > WS_STD_LEN)
    {
        return;
    }

    //netconn_write result buffer
    err_t result;

    //prepare header
    WS_frame_header_t hdr;
    hdr.FIN = 0x1;
    hdr.payload_length = length;
    hdr.mask = 0;
    hdr.reserved = 0;
    hdr.opcode = WS_OP_TXT;

    // printf("max len:%d mess len %d\n", WS_STD_LEN, length);

    //send header
    result = netconn_write(conn, &hdr, sizeof(WS_frame_header_t), NETCONN_COPY);

    //check if header was send
    if (result != ERR_OK)
    {
        return;
    }

    //send payload
    netconn_write(conn, p_data, length, NETCONN_COPY);
}


static void ws_server_receive_data_empty(char *buf, WS_frame_header_t *p_frame_hdr)
{
}


void ws_server_receive_data(char *buf, WS_frame_header_t *p_frame_hdr)
{
    char *p_buf;
    char *p_payload;
    uint16_t i;

    //get payload length
    if (p_frame_hdr->payload_length <= WS_STD_LEN)
    {
        //get beginning of mask or payload
        p_buf = (char *)&buf[sizeof(WS_frame_header_t)];

        //check if content is masked
        if (p_frame_hdr->mask)
        {
            //allocate memory for decoded message
            p_payload = malloc(
                p_frame_hdr->payload_length + 1);

            //check if malloc succeeded
            if (p_payload != NULL)
            {
                //decode playload
                for (i = 0; i < p_frame_hdr->payload_length;
                     i++)
                {
                    p_payload[i] = (p_buf + WS_MASK_L)[i]
                                   ^ p_buf[i % WS_MASK_L];
                }

                //add 0 terminator
                p_payload[p_frame_hdr->payload_length] = 0;
            }
        }
        else
        {
            //content is not masked
            p_payload = p_buf;
        }

        //do stuff
        if ( (p_payload != NULL) && (p_frame_hdr->opcode == WS_OP_TXT) )
        {
            //prepare FreeRTOS message
            WebSocket_frame_t __ws_frame;
            __ws_frame.frame_header = *p_frame_hdr;
            __ws_frame.payload_length = p_frame_hdr->payload_length;
            __ws_frame.payload = p_payload;

            //send message
            if ( !xQueueSend(WebSocket_rx_queue,&__ws_frame,0) )
            {
                free(__ws_frame.payload);
            }
        }
    }
}


static bool ws_server_conn_init(struct netconn *conn, struct netbuf *inbuf)
{
    //message buffer
    char *buf;

    //pointer to buffer (multi purpose)
    char *p_buf;

    //Pointer to SHA1 input
    char *p_SHA1_Inp;

    //Pointer to SHA1 result
    char *p_SHA1_result;

    //multi purpose number buffer
    uint16_t i;

    //will point to payload (send and receive
    char *p_payload;

    //allocate memory for SHA1 input
    p_SHA1_Inp = malloc( WS_CLIENT_KEY_L + sizeof(WS_sec_conKey) );

    //allocate memory for SHA1 result
    p_SHA1_result = malloc(SHA1_RES_L);

    //Check if malloc suceeded
    if ( (p_SHA1_Inp != NULL) && (p_SHA1_result != NULL) )
    {
        //receive handshake request
        if (netconn_recv(conn, &inbuf) == ERR_OK)
        {
            //read buffer
            netbuf_data(inbuf, (void * *)&buf, &i);

            //write static key into SHA1 Input
            for (i = 0; i < sizeof(WS_sec_conKey); i++)
            {
                p_SHA1_Inp[i + WS_CLIENT_KEY_L] = WS_sec_conKey[i];
            }

            //find Client Sec-WebSocket-Key:
            p_buf = strstr(buf, WS_sec_WS_keys);

            //check if needle "Sec-WebSocket-Key:" was found
            if (p_buf != NULL)
            {
                //get Client Key
                for (i = 0; i < WS_CLIENT_KEY_L; i++)
                {
                    p_SHA1_Inp[i] = *(p_buf + sizeof(WS_sec_WS_keys) + i);
                }

                // calculate hash
                esp_sha(SHA1, (unsigned char *)p_SHA1_Inp, strlen(p_SHA1_Inp),
                        (unsigned char *)p_SHA1_result);

                //hex to base64
                p_buf = (char *)_base64_encode( (unsigned char *)p_SHA1_result,
                                                SHA1_RES_L, (size_t *)&i );

                //free SHA1 input
                free(p_SHA1_Inp);

                //free SHA1 result
                free(p_SHA1_result);

                //allocate memory for handshake
                p_payload = malloc(
                    sizeof(WS_srv_hs) + i - WS_SPRINTF_ARG_L);

                //check if malloc suceeded
                if (p_payload != NULL)
                {
                    //prepare handshake
                    sprintf(p_payload, WS_srv_hs, i - 1, p_buf);

                    //send handshake
                    netconn_write(conn, p_payload, strlen(p_payload),
                                  NETCONN_COPY);

                    //free inbuf
                    netbuf_delete(inbuf);

                    //free base 64 encoded sec key
                    free(p_buf);

                    //free handshake memory
                    free(p_payload);

                    return true;
                }
            }
        }
    }

    return false;
}


static void ws_server_close(struct netconn *conn, struct netbuf *inbuf)
{
    netbuf_delete(inbuf);
    netconn_close(conn);
    netconn_delete(conn);
}


static void ws_server_netconn_serve(ws_server_handler_t *handler)
{
    struct netconn *conn = handler->conn;
    struct netbuf *inbuf = NULL;
    ws_server_rx_func rx = handler->rx != NULL ? handler->rx : ws_server_receive_data_empty;
    char *buf;
    uint16_t i;
    WS_frame_header_t *p_frame_hdr;

    if ( !ws_server_conn_init(conn, inbuf) )
    {
        ws_server_close(conn, inbuf);
        return;
    }

    if (handler->opened != NULL)
    {
        handler->opened();
    }

    //Wait for new data
    while (netconn_recv(conn, &inbuf) == ERR_OK)
    {
        //read data from inbuf
        netbuf_data(inbuf, (void * *)&buf, &i);

        //get pointer to header
        p_frame_hdr = (WS_frame_header_t *)buf;

        //check if clients wants to close the connection
        if (p_frame_hdr->opcode == WS_OP_CLS)
        {
            break;
        }

        rx(buf, p_frame_hdr);
        netbuf_delete(inbuf);
    }

    if (handler->closed != NULL)
    {
        handler->closed();
    }

    ws_server_close(conn, inbuf);
}


void ws_server_task(void *pvParameters)
{
    ws_server_handler_t *handler = (ws_server_handler_t *)pvParameters;
    //connection references
    struct netconn *conn, *newconn;

    //set up new TCP listener
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, handler->port);
    netconn_listen(conn);

    //wait for connections
    while (netconn_accept(conn, &newconn) == ERR_OK)
    {
        handler->conn = newconn;
        ws_server_netconn_serve(handler);
        handler->conn = NULL;
    }

    //close connection
    netconn_close(conn);
    netconn_delete(conn);
}
