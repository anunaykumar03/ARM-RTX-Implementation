/* The KCD Task Template File */
#include "common.h"
#include "k_msg.h"

#define MAX_CMD_LEN 64

void kcd_task(void)
{
    /* not implemented yet */
    // request mailbox
    if (mbx_create(KCD_MBX_SIZE) != RTX_OK){
        tsk_exit();
    }

    static task_t cmd_reg_map[62];              // 10 numbers + 26 capital letters + 26 lowercase letters
    U64 cmd_reg_bitmap = 0;                     // bit is 1 if corresponding cmd id has been registered

    static U8 cmd_str[MAX_CMD_LEN];             // concatenate command string
    U32 cmd_str_idx = 0;

    U32 recv_buf_len = sizeof(RTX_MSG_HDR) + 1; // KEY_IN and KEY_REG messages should have data len = 1
    U8 recv_buf[recv_buf_len];                  // holds current message to be processed
    task_t* sender_tid;

    while (1){
        if (recv_msg(sender_tid, recv_buf, recv_buf_len) != RTX_OK) {
            continue;
        }
        RTX_MSG_HDR* msg_header = (RTX_MSG_HDR *)recv_buf;
        U8* msg_data = recv_buf + sizeof(RTX_MSG_HDR);

        if (msg_header->type == KCD_REG){
            // register command identifier
            U8 cmd_id = *msg_data;
            if (cmd_id >= '0' && cmd_id <= '9'){
                cmd_reg_map[cmd_id - '0'] = *sender_tid; // map '0'-'9' to 0-9
                cmd_reg_bitmap |= (1 << (cmd_id - '0'));
            }
            else if (cmd_id >= 'A' && cmd_id <= 'Z'){
                cmd_reg_map[cmd_id - 55] = *sender_tid; // map A-Z to 10-35
                cmd_reg_bitmap |= (1 << (cmd_id - 55));
            }
            else if (cmd_id >= 'a' && cmd_id <= 'z'){
                cmd_reg_map[cmd_id - 61] = *sender_tid; // map a-z to 36-61
                cmd_reg_bitmap |= (1 << (cmd_id - 61));
            }
        }
        else if (msg_header->type == KEY_IN && *sender_tid == TID_UART_IRQ){
            if (*msg_data == 0xA){ // enter key
                // send command string
                if (cmd_str[0] != '%' || cmd_str_idx == 0){
                    // send "Invalid Command"
                    SER_PutStr(0, " Invalid Command\n\r");
                }
                else {
                    // compose message
                    U8* send_buf[cmd_str_idx + sizeof(RTX_MSG_HDR)];

                    RTX_MSG_HDR* send_hdr = (RTX_MSG_HDR *)send_buf;
                    send_hdr->length = cmd_str_idx-1 + sizeof(RTX_MSG_HDR); // -1 since '%' is not sent
                    send_hdr->type = KCD_CMD;

                    U8* send_body = send_buf + sizeof(RTX_MSG_HDR);
                    for (int i = 1; i < cmd_str_idx; i++){
                        send_body[i-1] = cmd_str[i];
                    }

                    // get which task is receiver
                    U8 cmd_id = cmd_str[1];
                    task_t receiver_tid;
                    U8 err_flag = 0;
                    if (cmd_id >= '0' && cmd_id <= '9'){
                        receiver_tid = cmd_reg_map[cmd_id - '0'];
                        if ((cmd_reg_bitmap >> (cmd_id - '0')) & 0x1 != 1 || g_tcbs[receiver_tid].state == DORMANT){
                            // command id not registered or task is dormant (task no longer exists)
                            err_flag = 1;
                        }
                    }
                    else if (cmd_id >= 'A' && cmd_id <= 'Z'){
                        receiver_tid = cmd_reg_map[cmd_id - 55];
                        if ((cmd_reg_bitmap >> (cmd_id - 55)) & 0x1 != 1 || g_tcbs[receiver_tid].state == DORMANT){
                            // command id not registered or task is dormant (task no longer exists)
                            err_flag = 1;
                        }
                    }
                    else if (cmd_id >= 'a' && cmd_id <= 'z'){
                        receiver_tid = cmd_reg_map[cmd_id - 61];
                        if ((cmd_reg_bitmap >> (cmd_id - 61)) & 0x1 != 1 || g_tcbs[receiver_tid].state == DORMANT){
                            // command id not registered or task is dormant (task no longer exists)
                            err_flag = 1;
                        }
                    }

                    // send message
                    if (err_flag == 1 || send_msg(receiver_tid, send_buf) != RTX_OK){
                        // send "Command cannot be processed"
                        SER_PutStr(0, "Command cannot be processed\n\r");
                    }
                }
                cmd_str_idx = 0;
                continue;
            }

            if (cmd_str_idx == MAX_CMD_LEN) {
                cmd_str_idx = 0;
                // send "Invalid Command"
                SER_PutStr(0, " Invalid Command\n\r");
                continue;
            }
            cmd_str[cmd_str_idx] = *msg_data;
            cmd_str_idx++;
        }
    }
}
