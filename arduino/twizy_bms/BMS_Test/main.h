void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

void publishMsg(char topic[10], char msg[50]);
void publishMsg(char topic[10], int value);
void publishMsg(char topic[10], double value);
