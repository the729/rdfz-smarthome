void setup() {
    Serial.begin(9600);
    
    pinMode(13, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(11, OUTPUT);
}

void loop() {
    pollModule();
}

void bool_changed(char index, boolean new_state)
{
    //
    if (index == 0) {
        if (new_state)
            digitalWrite(13, HIGH);
        else
            digitalWrite(13, LOW);
    }
    if (index == 1) {
        if (new_state)
            digitalWrite(12, HIGH);
        else
            digitalWrite(12, LOW);
    }
}

void data_changed(int new_data)
{
    //
    new_data = constrain(new_data, 0, 100);
    new_data = new_data * 255 / 100;
    analogWrite(11, new_data);
}




















/*==========================================================================
       DO NOT MODIFY CODE BELOW THIS LINE!
       Unless you know what you are doing. 
       Module communication code below.
============================================================================*/

#define MOD_PKT_START   0x7E
#define MOD_PKT_FIN     0x7C
#define MOD_PKT_ESCAPE  0x7A
#define MOD_PKT_MAX_LEN 16

uint8_t cmd_buf[MOD_PKT_MAX_LEN];
char mode_state;
boolean bool_state[4];
uint16_t data_state[2];

void pollModule()
{
    char cmdlen = mod_recv_cmd();
    uint8_t cmd_send[4];
    if (!cmdlen) return;
    
    if (cmd_buf[0] == 0xFE) {
        mod_send_cmd("\x90\x66\x66\x00\x01", 5);
    } else if (cmd_buf[0] == 0x91) {
        // Nothing to do
    }
    if ((cmd_buf[0] & 0xF0) == 0x20) {
        mode_state = (cmd_buf[0] & 0x0F);
        report_mode_bool();
    }
    if ((cmd_buf[0] & 0xF0) == 0x60) {
        bool_state[(cmd_buf[0] & 0x06)>>1] = (cmd_buf[0] & 0x01);
        report_mode_bool();
        bool_changed((cmd_buf[0] & 0x06)>>1, (cmd_buf[0] & 0x01));
    }
    if (cmd_buf[0] == 0xA2 && cmd_buf[1] == 1) {
        data_state[1] = ((uint16_t)(cmd_buf[2]) << 8) + cmd_buf[3];
        report_data(1);
        data_changed((int)(data_state[1]));
    }
}

void set_bool(char index, boolean new_state)
{
    if (index > 3 || index < 0) return;
    bool_state[index] = new_state;
    report_mode_bool();
}

void set_data(int new_data)
{
    data_state[0] = (uint16_t)new_data;
    report_data(0);
}

void set_data(char new_data)
{
    data_state[0] = (uint16_t)(short)new_data;
    report_data(0);
}

void set_data(short new_data)
{
    data_state[0] = (uint16_t)new_data;
    report_data(0);
}

void report_mode_bool()
{
    uint8_t cmd_send[2];
    cmd_send[0] = 0xB1;
    cmd_send[1] = (mode_state << 4) | (bool_state[3] ? 0x08 : 0x00) | (bool_state[2] ? 0x04 : 0x00) | (bool_state[1] ? 0x02 : 0x00) | (bool_state[0] ? 0x01 : 0x00);
    mod_send_cmd((char *)cmd_send, 2);
}

void report_data(char idx)
{
    uint8_t cmd_send[4];
    
    if (idx != 0 && idx != 1) return;
    
    cmd_send[0] = 0xB2;
    cmd_send[1] = idx;
    cmd_send[2] = (data_state[idx]>>8);
    cmd_send[3] = (data_state[idx] & 0xFF);
    mod_send_cmd((char *)cmd_send, 4);
}

int mod_recv_cmd()
{
    static char recv_state = 0;
    static char recv_escape = 0;
    static char p = 0;
    while (Serial.available()) {
        uint8_t c = Serial.read();
        
        if (c == MOD_PKT_START) {
            // Received packet start, begin receiving
            recv_state = 1;
            p = 0;
            recv_escape = 0;
        } else if (c == MOD_PKT_FIN) {
            recv_state = 0;
            recv_escape = 0;
            return p;
        } else if (c == MOD_PKT_ESCAPE) {
            recv_escape = 1;
        } else {
            c = c ^ recv_escape;
            recv_escape = 0;
            if (recv_state && p < MOD_PKT_MAX_LEN) {
                cmd_buf[p++] = c;
            }
        }
    }
    return 0;
}

void mod_send_cmd(const char * cmd, char len)
{
    static uint8_t send_buf[MOD_PKT_MAX_LEN];
    static char p;
    
    p = 0;
    send_buf[p++] = 0x7E;
    for (char i=0; i<len && p<MOD_PKT_MAX_LEN; i++) {
        if (cmd[i] == MOD_PKT_START || cmd[i] == MOD_PKT_FIN || cmd[i] == MOD_PKT_ESCAPE) {
            send_buf[p++] = MOD_PKT_ESCAPE;
            send_buf[p++] = cmd[i] ^ 0x01;
        } else {
            send_buf[p++] = cmd[i];
        }
    }
    send_buf[p++] = 0x7C;
    Serial.write(send_buf, p);
}
